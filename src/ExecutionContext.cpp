#include "ExecutionContext.hpp"

#include <VCL/NativeTarget.hpp>

#include <fstream>
#include <iostream>

#include <llvm/ExecutionEngine/JITEventListener.h>

#include <cmath>


VCL::ExecutionContext::ExecutionContext() : context{}, dumpObject{ false } {
    context = llvm::orc::ThreadSafeContext{ std::make_unique<llvm::LLVMContext>() };

    std::shared_ptr<NativeTarget> target = NativeTarget::GetInstance();
    
    if (auto r = llvm::orc::SelfExecutorProcessControl::Create(); !r)
        throw std::runtime_error{ std::format("{}", llvm::toString(r.takeError())) };
    else
        session = std::make_unique<llvm::orc::ExecutionSession>(std::move(*r));
    

    auto jtmb = llvm::orc::JITTargetMachineBuilder::detectHost();
    
    if (!jtmb)
        throw std::runtime_error{ std::format("{}", llvm::toString(jtmb.takeError())) };

    if (auto r = jtmb->getDefaultDataLayoutForTarget(); !r)
        throw std::runtime_error{ std::format("{}", llvm::toString(r.takeError())) };
    else
        layout = std::make_unique<llvm::DataLayout>(std::move(*r));

    mangle = std::make_unique<llvm::orc::MangleAndInterner>(*session, *layout);

    linkingLayer = std::make_unique<llvm::orc::RTDyldObjectLinkingLayer>(*session, []() {
        return std::make_unique<llvm::SectionMemoryManager>();
    });

    dumpObjectTransformFunction = std::make_unique<llvm::orc::ObjectTransformLayer::TransformFunction>(
        std::bind(&ExecutionContext::DumpObject, this, std::placeholders::_1)
    );
    dumpObjectLayer = std::make_unique<llvm::orc::ObjectTransformLayer>(*session, *linkingLayer,
        [&](std::unique_ptr<llvm::MemoryBuffer> buf)
        -> llvm::Expected<std::unique_ptr<llvm::MemoryBuffer>> {
            return dumpObjectTransformFunction->operator()(std::move(buf));
        }
    );
    compileLayer = std::make_unique<llvm::orc::IRCompileLayer>(*session, *dumpObjectLayer, 
        std::make_unique<llvm::orc::ConcurrentIRCompiler>(std::move(*jtmb)));

    main = &session->createBareJITDylib("Main");

    gdbListener = llvm::JITEventListener::createGDBRegistrationListener();

    DefineIntrinsic();
}

VCL::ExecutionContext::~ExecutionContext() {
    llvm::cantFail(session->endSession());
}

void VCL::ExecutionContext::AddModule(llvm::orc::ThreadSafeModule module, llvm::orc::ResourceTrackerSP rt) {
    if (!rt)
        rt = main->getDefaultResourceTracker();
    llvm::Error err = compileLayer->add(rt, std::move(module));
    if (err)
        throw std::runtime_error{ std::format("{}", llvm::toString(std::move(err))) };
}

void VCL::ExecutionContext::Clear() {
    llvm::cantFail(main->clear());
    DefineIntrinsic();
}

llvm::orc::ExecutorSymbolDef VCL::ExecutionContext::Lookup(std::string_view name) {
    if (auto r = session->lookup({ main }, name); !r)
        throw std::runtime_error{ std::format("{}", llvm::toString(r.takeError())) };
    else
        return *r;
}

void VCL::ExecutionContext::DefineExternSymbolPtr(std::string_view name, void* buffer) {
    llvm::orc::ExecutorSymbolDef symbol{
        llvm::orc::ExecutorAddr::fromPtr(buffer),
        llvm::JITSymbolFlags::None
    };
    llvm::Error err = main->define(llvm::orc::absoluteSymbols({
        { mangle->operator()(name), symbol }
    }));
    if (err)
        throw std::runtime_error{ std::format("{}", llvm::toString(std::move(err))) };
}

void VCL::ExecutionContext::SetDumpObject(std::filesystem::path directory, std::string_view identifier) {
    if (!std::filesystem::is_directory(directory))
        throw std::runtime_error{ "Execution context given dump directory isn't a valid directory" };
    dumpObject = true;
    dumpObjectDirectory = directory;
    dumpObjectIdentifier = identifier;
}

void VCL::ExecutionContext::EnableDebugInformation() {
    linkingLayer->registerJITEventListener(*gdbListener);
}

void VCL::ExecutionContext::DisableDebugInformation() {
    linkingLayer->unregisterJITEventListener(*gdbListener);
}

llvm::orc::ThreadSafeContext& VCL::ExecutionContext::GetTSContext() {
    return context;
}

llvm::Expected<std::unique_ptr<llvm::MemoryBuffer>> VCL::ExecutionContext::DumpObject(std::unique_ptr<llvm::MemoryBuffer> buf) {
    if (!dumpObject)
        return std::move(buf);

    std::filesystem::path dumpFilename = dumpObjectDirectory / std::format("{}.o", dumpObjectIdentifier);
    std::ofstream dumpOutFile{ dumpFilename, std::ios::binary | std::ios::trunc };
    dumpOutFile.write(buf->getBufferStart(), buf->getBufferSize());

    return std::move(buf);
}

#define ADD_MATH_SYMBOL(name, ptr) symbolMap[session->intern(name)] = llvm::orc::ExecutorSymbolDef{ \
        llvm::orc::ExecutorAddr::fromPtr(ptr), \
        llvm::JITSymbolFlags::Exported \
    }

void VCL::ExecutionContext::DefineIntrinsic() {
    llvm::orc::SymbolMap symbolMap{ 22 };

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

    llvm::Error err = main->define(llvm::orc::absoluteSymbols(symbolMap));
}