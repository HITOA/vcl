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

#define ADD_MATH_SYMBOL(f) symbolMap[session->intern(#f)] = llvm::orc::ExecutorSymbolDef{ \
        llvm::orc::ExecutorAddr::fromPtr(&f), \
        llvm::JITSymbolFlags::Exported \
    }

void VCL::ExecutionContext::DefineIntrinsic() {
    llvm::orc::SymbolMap symbolMap{ 22 };

    ADD_MATH_SYMBOL(sqrtf);
    ADD_MATH_SYMBOL(sinf);
    ADD_MATH_SYMBOL(cosf);
    ADD_MATH_SYMBOL(tanf);
    ADD_MATH_SYMBOL(asinf);
    ADD_MATH_SYMBOL(acosf);
    ADD_MATH_SYMBOL(atanf);
    ADD_MATH_SYMBOL(sinhf);
    ADD_MATH_SYMBOL(coshf);
    ADD_MATH_SYMBOL(tanhf);
    ADD_MATH_SYMBOL(logf);
    ADD_MATH_SYMBOL(log10f);
    ADD_MATH_SYMBOL(log2f);
    ADD_MATH_SYMBOL(expf);
    //ADD_MATH_SYMBOL(exp10f);
    ADD_MATH_SYMBOL(exp2f);
    ADD_MATH_SYMBOL(fabsf);
    ADD_MATH_SYMBOL(ceilf);
    ADD_MATH_SYMBOL(floorf);
    ADD_MATH_SYMBOL(roundf);
    ADD_MATH_SYMBOL(powf);
    ADD_MATH_SYMBOL(fmaf);

    llvm::Error err = main->define(llvm::orc::absoluteSymbols(symbolMap));
}