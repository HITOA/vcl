#include <VCL/Frontend/ExecutionSession.hpp>


VCL::ExecutionSession::ExecutionSession() : lastError{ llvm::Error::success() } {
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
    linkingLayer = std::make_unique<llvm::orc::RTDyldObjectLinkingLayer>(*session, [](){
        return std::make_unique<llvm::SectionMemoryManager>();
    });

    compileLayer = std::make_unique<llvm::orc::IRCompileLayer>(*session, *linkingLayer, std::make_unique<llvm::orc::ConcurrentIRCompiler>(std::move(*jtmb)));

    main = &session->createBareJITDylib("Main");
    gdbListener = llvm::JITEventListener::createGDBRegistrationListener();
}

VCL::ExecutionSession::~ExecutionSession() {
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