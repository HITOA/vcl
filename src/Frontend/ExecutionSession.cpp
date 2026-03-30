#include <VCL/Frontend/ExecutionSession.hpp>

#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/TargetParser/Host.h>

#include <cmath>


VCL::ExecutionSession::ExecutionSession() : lastError{ llvm::Error::success() } {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    if (auto r = llvm::orc::SelfExecutorProcessControl::Create(); !r)
        throw std::runtime_error{ llvm::toString(r.takeError()) };
    else
        session = std::make_unique<llvm::orc::ExecutionSession>(std::move(*r));

    auto jtmb = llvm::orc::JITTargetMachineBuilder::detectHost();

    if (!jtmb)
        throw std::runtime_error{ llvm::toString(jtmb.takeError()) };

    if (auto r = jtmb->getDefaultDataLayoutForTarget(); !r)
        throw std::runtime_error{ llvm::toString(r.takeError()) };
    else
        layout = std::make_unique<llvm::DataLayout>(std::move(*r));

    mangle = std::make_unique<llvm::orc::MangleAndInterner>(*session, *layout);
    linkingLayer = std::make_unique<llvm::orc::RTDyldObjectLinkingLayer>(*session, [](const llvm::MemoryBuffer& buffer) -> 
        std::unique_ptr<llvm::RuntimeDyld::MemoryManager> {
        return std::make_unique<llvm::SectionMemoryManager>();
    });

    compileLayer = std::make_unique<llvm::orc::IRCompileLayer>(*session, *linkingLayer, std::make_unique<llvm::orc::ConcurrentIRCompiler>(std::move(*jtmb)));

    main = &session->createBareJITDylib("Main");
    gdbListener = llvm::JITEventListener::createGDBRegistrationListener();
}

VCL::ExecutionSession::ExecutionSession(ExecutionSession&& other) : session{ std::move(other.session) }, layout{ std::move(other.layout) },
        mangle{ std::move(other.mangle) }, linkingLayer{ std::move(other.linkingLayer) }, compileLayer{ std::move(other.compileLayer) }, main{ other.main },
        gdbListener{ other.gdbListener }, lastError{ std::move(other.lastError) } {
    other.session = nullptr;
}

VCL::ExecutionSession::~ExecutionSession() {
    if (session != nullptr)
        llvm::cantFail(session->endSession());
}

bool VCL::ExecutionSession::SubmitModule(llvm::orc::ThreadSafeModule&& module) {
    llvm::Error err = compileLayer->add(*main, std::move(module));
    if (err)
        lastError = std::move(err);
    return !err;
}

void* VCL::ExecutionSession::Lookup(llvm::StringRef name) {
    if (auto r = session->lookup({ main }, name); !r) {
        lastError = std::move(r.takeError());
        return nullptr;
    } else
        return r.get().getAddress().toPtr<void*>();
}

bool VCL::ExecutionSession::DefineSymbolPtr(llvm::StringRef name, void* ptr) {
    llvm::orc::ExecutorSymbolDef symbol{
        llvm::orc::ExecutorAddr::fromPtr(ptr),
        llvm::JITSymbolFlags::None
    };

    llvm::Error err = main->define(llvm::orc::absoluteSymbols({
        { (*mangle)(name), symbol }
    }));
    if (err)
        lastError = std::move(err);
    return !err;
}

void VCL::ExecutionSession::EnableGDBListener() {
    linkingLayer->registerJITEventListener(*gdbListener);
}

void VCL::ExecutionSession::DisableGDBListener() {
    linkingLayer->unregisterJITEventListener(*gdbListener);
}

void VCL::ExecutionSession::DefineDefaultMemIntrinsic() {
    llvm::orc::SymbolMap symbolMap{ 2 };

    symbolMap[session->intern("memset")] = llvm::orc::ExecutorSymbolDef{
        llvm::orc::ExecutorAddr::fromPtr(&memset),
        llvm::JITSymbolFlags::Exported
    };


    symbolMap[session->intern("memcpy")] = llvm::orc::ExecutorSymbolDef{
        llvm::orc::ExecutorAddr::fromPtr(&memcpy),
        llvm::JITSymbolFlags::Exported
    };

    lastError = main->define(llvm::orc::absoluteSymbols(symbolMap));
}

#define ADD_MATH_SYMBOL(name, ptr) symbolMap[session->intern(name)] = llvm::orc::ExecutorSymbolDef{ \
        llvm::orc::ExecutorAddr::fromPtr(ptr), \
        llvm::JITSymbolFlags::Exported \
    }

void VCL::ExecutionSession::DefineDefaultMathIntrinsic() {
    llvm::orc::SymbolMap symbolMap{ 44 };

    // Float
    ADD_MATH_SYMBOL("sqrtf",    &sqrtf);
    ADD_MATH_SYMBOL("sinf",     &sinf);
    ADD_MATH_SYMBOL("cosf",     &cosf);
    ADD_MATH_SYMBOL("tanf",     &tanf);
    ADD_MATH_SYMBOL("asinf",    &asinf);
    ADD_MATH_SYMBOL("acosf",    &acosf);
    ADD_MATH_SYMBOL("atanf",    &atanf);
    ADD_MATH_SYMBOL("sinhf",    &sinhf);
    ADD_MATH_SYMBOL("coshf",    &coshf);
    ADD_MATH_SYMBOL("tanhf",    &tanhf);
    ADD_MATH_SYMBOL("logf",     &logf);
    ADD_MATH_SYMBOL("log10f",   &log10f);
    ADD_MATH_SYMBOL("log2f",    &log2f);
    ADD_MATH_SYMBOL("expf",     &expf);
    ADD_MATH_SYMBOL("exp2f",    &exp2f);
    ADD_MATH_SYMBOL("fabsf",    &fabsf);
    ADD_MATH_SYMBOL("ceilf",    &ceilf);
    ADD_MATH_SYMBOL("floorf",   &floorf);
    ADD_MATH_SYMBOL("roundf",   &roundf);
    ADD_MATH_SYMBOL("powf",     &powf);
    ADD_MATH_SYMBOL("fmaf",     &fmaf);
    ADD_MATH_SYMBOL("sincosf",  &sincosf);
    // Double
    ADD_MATH_SYMBOL("sqrt",     static_cast<double(*)(double)>(&sqrt));
    ADD_MATH_SYMBOL("sin",      static_cast<double(*)(double)>(&sin));
    ADD_MATH_SYMBOL("cos",      static_cast<double(*)(double)>(&cos));
    ADD_MATH_SYMBOL("tan",      static_cast<double(*)(double)>(&tan));
    ADD_MATH_SYMBOL("asin",     static_cast<double(*)(double)>(&asin));
    ADD_MATH_SYMBOL("acos",     static_cast<double(*)(double)>(&acos));
    ADD_MATH_SYMBOL("atan",     static_cast<double(*)(double)>(&atan));
    ADD_MATH_SYMBOL("sinh",     static_cast<double(*)(double)>(&sinh));
    ADD_MATH_SYMBOL("cosh",     static_cast<double(*)(double)>(&cosh));
    ADD_MATH_SYMBOL("tanh",     static_cast<double(*)(double)>(&tanh));
    ADD_MATH_SYMBOL("log",      static_cast<double(*)(double)>(&log));
    ADD_MATH_SYMBOL("log10",    static_cast<double(*)(double)>(&log10));
    ADD_MATH_SYMBOL("log2",     static_cast<double(*)(double)>(&log2));
    ADD_MATH_SYMBOL("exp",      static_cast<double(*)(double)>(&exp));
    ADD_MATH_SYMBOL("exp2",     static_cast<double(*)(double)>(&exp2));
    ADD_MATH_SYMBOL("fabs",     static_cast<double(*)(double)>(&fabs));
    ADD_MATH_SYMBOL("ceil",     static_cast<double(*)(double)>(&ceil));
    ADD_MATH_SYMBOL("floor",    static_cast<double(*)(double)>(&floor));
    ADD_MATH_SYMBOL("round",    static_cast<double(*)(double)>(&round));
    ADD_MATH_SYMBOL("pow",      static_cast<double(*)(double, double)>(&pow));
    ADD_MATH_SYMBOL("fma",      static_cast<double(*)(double, double, double)>(&fma));
    ADD_MATH_SYMBOL("sincos",   static_cast<void(*)(double, double*, double*)>(&sincos));

    lastError = main->define(llvm::orc::absoluteSymbols(symbolMap));
}