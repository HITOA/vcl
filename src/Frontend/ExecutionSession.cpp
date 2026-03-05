#include <VCL/Frontend/ExecutionSession.hpp>

#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/TargetParser/Host.h>


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