#include <vcl/jit.hpp>

#include "llvmheader.hpp"
#include "jittype.hpp"
#include "jitbuiltins.hpp"

#include <stack>
#include <stdexcept>


#undef DEF
#define DEF(name, symbol, ...) name,
enum class BinaryOpType {
    BINARY_OPERATOR_DEF
    MAX
};

enum class UnaryOpType {
    UNARY_OPERATOR_DEF
    MAX
};

static bool InitializeTarget() {
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        initialized &= !llvm::InitializeNativeTarget();
        initialized &= !llvm::InitializeNativeTargetAsmPrinter();
    }
    return initialized;
}

static llvm::AllocaInst* CreateEntryBlockAlloca(llvm::BasicBlock* block, std::string_view name, llvm::Type* type) {
    llvm::IRBuilder<> tmpBuilder{ block, block->begin() };
    return tmpBuilder.CreateAlloca(type, nullptr, name);
}

class JIT {
public:
    JIT(std::unique_ptr<llvm::orc::ExecutionSession> session, 
        llvm::orc::JITTargetMachineBuilder jtmb, 
        llvm::DataLayout layout) : 
        session{ std::move(session) }, mangle{ *this->session, layout }, layout{ layout },
        linkingLayer{ *this->session, []() { return std::make_unique<llvm::SectionMemoryManager>(); } },
        dumpObjectTransform{ llvm::orc::DumpObjects() },
        objectLayer{ *this->session, linkingLayer, 
            [&transform = this->dumpObjectTransform, dumpObjects = &dumpObjs](std::unique_ptr<llvm::MemoryBuffer> buf)
            -> llvm::Expected<std::unique_ptr<llvm::MemoryBuffer>> {
                if (*dumpObjects)
                    return transform(std::move(buf));
                return std::move(buf);
            } },
        compileLayer{ *this->session, objectLayer, std::make_unique<llvm::orc::ConcurrentIRCompiler>(std::move(jtmb)) },
        main{ this->session->createBareJITDylib("<main>") }, dumpObjs{ false } {
        main.addGenerator(llvm::cantFail(
            llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(layout.getGlobalPrefix())
        ));
    }

    llvm::Error AddModule(llvm::orc::ThreadSafeModule module, llvm::orc::ResourceTrackerSP rt = nullptr) {
        if (!rt)
            rt = main.getDefaultResourceTracker();
        return compileLayer.add(rt, std::move(module));
    }

    llvm::Expected<llvm::orc::ExecutorSymbolDef> Lookup(std::string_view str) {
        return session->lookup({&main}, str);
    }

    void BindGlobalVariable(std::string_view name, void* ptr) {
        auto symbol = llvm::orc::ExecutorSymbolDef{ llvm::orc::ExecutorAddr::fromPtr(ptr), llvm::JITSymbolFlags::None };
        auto err = main.define(
            llvm::orc::absoluteSymbols({
                { mangle(name), symbol }
            })
        );
        if (err)
            throw std::runtime_error{ std::format("(LLVM): {}", llvm::toString(std::move(err))) };
    }

    void DumpObjects() {
        dumpObjs = true;
    }

private:
    std::unique_ptr<llvm::orc::ExecutionSession> session;

    llvm::orc::MangleAndInterner mangle;
    llvm::DataLayout layout;

    llvm::orc::RTDyldObjectLinkingLayer linkingLayer;
    llvm::orc::ObjectTransformLayer::TransformFunction dumpObjectTransform;
    llvm::orc::ObjectTransformLayer objectLayer;
    llvm::orc::IRCompileLayer compileLayer;

    llvm::orc::JITDylib& main;
    bool dumpObjs;
};

struct VCL::JITContext::LLVMJITData {
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<JIT> jit;

    //Analysis Manager
    std::unique_ptr<llvm::LoopAnalysisManager> lam;
    std::unique_ptr<llvm::FunctionAnalysisManager> fam;
    std::unique_ptr<llvm::CGSCCAnalysisManager> cgam;
    std::unique_ptr<llvm::ModuleAnalysisManager> mam;

    //Pass Builder
    std::unique_ptr<llvm::PassBuilder> pb;

    //Pass Manager
    std::unique_ptr<llvm::LoopPassManager> lpm;
    std::unique_ptr<llvm::FunctionPassManager> fpm;
    std::unique_ptr<llvm::CGSCCPassManager> cgpm;
    std::unique_ptr<llvm::ModulePassManager> mpm;
};

struct VCL::Module::LLVMModuleData {
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;
};

template<typename T, size_t S>
class FixedSizeStack {
public:
    inline void Push(T& value) {
        values[c++] = value;
    }

    inline T& Pop() {
        return values[--c];
    }

    inline T& Top() {
        return values[c - 1];
    }
private:
    T values[S];
    size_t c;
};

namespace VCL {
    class ModuleBuilder : public VCL::ASTVisitor {
    public:
        ModuleBuilder() = delete;
        ModuleBuilder(VCL::Module* module) : m{ module->module.get() }, c{ module->context->jit.get() } {
            
        }
        ~ModuleBuilder() {

        }

        void VisitProgram(ASTProgram* node) override {
            currentScope = 0;
            for (size_t i = 0; i < node->statements.size(); ++i) {
                node->statements[i]->Accept(this);
            }
        }

        void VisitCompoundStatement(ASTCompoundStatement* node) override {
            ++currentScope;
            
            for (size_t i = 0; i < node->statements.size(); ++i)
                node->statements[i]->Accept(this);

            for (auto& s : namedVariables)
                if (s.second.Top().scope == currentScope)
                    s.second.Pop();

            --currentScope;
        }

        void VisitVariableAssignment(ASTVariableAssignment* node) override {
            if (!namedVariables.count(node->name))
                throw std::runtime_error{ std::format("Undefined variable \"{}\".", node->name) };

            VarInfo& v = namedVariables[node->name].Top();

            node->expression->Accept(this);

            llvm::Value* value = values.top();
            values.pop();

            if (v.scope == 0) {
                if (v.v->isConstant())
                    throw std::runtime_error{ std::format("\"{}\" variable is immutable.", node->name) };
                value = JITType::CastRHSToLHS(v.v->getType(), value, m->builder.get());
                m->builder->CreateStore(value, v.v);
            } else {
                value = JITType::CastRHSToLHS(v.a->getType(), value, m->builder.get());
                m->builder->CreateStore(value, v.a);
            }
        }

        void VisitVariableDeclaration(ASTVariableDeclaration* node) override {
            llvm::Type* type = JITType::GetType(node->type, c->context.get());
            
            if (currentScope == 0) {
                bool isExtern = (node->type.qualifiers & ASTTypeInfo::QualifierFlag::IN) || 
                    (node->type.qualifiers & ASTTypeInfo::QualifierFlag::OUT);
                
                if (node->expression && isExtern)
                    throw std::runtime_error{ std::format("Cannot initialize extern variable \"{}\".", node->name) };
                
                llvm::Constant* initializer = nullptr;
                if (node->expression) {
                    node->expression->Accept(this);
                    llvm::Value* v = values.top();
                    values.pop();
                    if (!llvm::isa<llvm::Constant>(v))
                        throw std::runtime_error{ "Global variable initializer must be constant." };
                    initializer = llvm::cast<llvm::Constant>(v);
                    if (type->isVectorTy())
                        initializer = llvm::ConstantVector::get(initializer);
                }

                llvm::GlobalVariable* v = new llvm::GlobalVariable{
                    *m->module,
                    type,
                    false,
                    isExtern ? llvm::GlobalValue::ExternalLinkage : llvm::GlobalValue::PrivateLinkage,
                    initializer,
                    node->name
                };

                if (node->type.qualifiers & ASTTypeInfo::QualifierFlag::IN)
                    v->setConstant(true);

                VarInfo vInfo{ nullptr, v, currentScope };

                if (!namedVariables.count(node->name))
                    namedVariables[node->name] = FixedSizeStack<VarInfo, 32>{};
                namedVariables[node->name].Push(vInfo);
            } else {
                llvm::BasicBlock* bb = m->builder->GetInsertBlock();

                llvm::Value* initValue;
                if (node->expression) {
                    node->expression->Accept(this);
                    initValue = values.top();
                    values.pop();
                }else {
                    initValue = llvm::ConstantFP::get(*c->context, llvm::APFloat(0.0f));
                }

                initValue = JITType::CastRHSToLHS(type, initValue, m->builder.get());

                llvm::AllocaInst* a = CreateEntryBlockAlloca(bb, node->name, type);
                m->builder->CreateStore(initValue, a);

                VarInfo vInfo{ a, nullptr, currentScope };

                if (!namedVariables.count(node->name))
                    namedVariables[node->name] = FixedSizeStack<VarInfo, 32>{};
                namedVariables[node->name].Push(vInfo);
            }
        }

        void VisitFunctionArgument(ASTFunctionArgument* node) override {}

        void VisitFunctionPrototype(ASTFunctionPrototype* node) override {
            if (JITBuiltins::IsBuiltinFunction(node->name))
                throw std::runtime_error{ std::format("The function \"{}\" is builtin.", node->name) };

            llvm::Function* function = m->module->getFunction(node->name);

            if (function)
                return;
            
            std::vector<llvm::Type*> argsType{};

            for (auto& arg : node->arguments)
                argsType.push_back(JITType::GetType(arg->type, c->context.get()));

            llvm::FunctionType* functionType = llvm::FunctionType::get(JITType::GetType(node->type, c->context.get()), argsType, false);

            function = llvm::Function::Create(functionType, llvm::GlobalValue::ExternalLinkage, node->name, m->module.get());

            uint32_t idx = 0;
            for (auto& arg : function->args())
                arg.setName(node->arguments[idx++]->name);
        }

        void VisitFunctionDeclaration(ASTFunctionDeclaration* node) override {
            node->prototype->Accept(this);

            llvm::Function* function = m->module->getFunction(node->prototype->name);

            if (!function)
                return;

            if (!function->empty())
                throw std::runtime_error{ std::format("Function \"{}\" redefinition is not allowed!", node->prototype->name) };
            
            //function->setLinkage(llvm::GlobalValue::InternalLinkage);

            llvm::BasicBlock* basicBlock = llvm::BasicBlock::Create(*c->context, "entry", function);
            m->builder->SetInsertPoint(basicBlock);
            
            ++currentScope;

            for (auto& arg : function->args()) {
                llvm::AllocaInst* a = CreateEntryBlockAlloca(basicBlock, arg.getName(), arg.getType());
                VarInfo vInfo{ a, nullptr, currentScope };
                m->builder->CreateStore(&arg, a);
                if (!namedVariables.count(std::string_view{ arg.getName() }))
                    namedVariables[std::string_view{ arg.getName() }] = FixedSizeStack<VarInfo, 32>{};
                namedVariables[std::string_view{ arg.getName() }].Push(vInfo);
            }

            node->body->Accept(this);

            for (auto& arg : function->args()) {
                namedVariables[std::string_view{ arg.getName() }].Pop();
            }

            --currentScope;

            //Check if missing return stmt
            for (auto& bb : *function) {
                llvm::Instruction* terminator = bb.getTerminator();
                if (terminator) continue;
                if (function->getReturnType()->isVoidTy()) {
                    m->builder->SetInsertPoint(&bb);
                    m->builder->CreateRetVoid();
                } else {
                    throw std::runtime_error{ std::format("Function \"{}\" is missing return statement.", node->prototype->name) };
                }
            }


            llvm::verifyFunction(*function);
        }

        void VisitReturnStatement(ASTReturnStatement* node) override {
            node->expression->Accept(this);
            m->builder->CreateRet(values.top());
            values.pop();
        }

        void VisitUnaryExpression(ASTUnaryExpression* node) override {
            node->expression->Accept(this);
            llvm::Value* exp = values.top();
            values.pop();

            UnaryOpType op = (UnaryOpType)node->op;
            switch (op) {
                case UnaryOpType::PLUS:
                    values.push(exp);
                    break;
                case UnaryOpType::MINUS:
                    values.push(m->builder->CreateFNeg(exp));
                    break;
                default:
                    break;
            }
        }

        void VisitBinaryExpression(ASTBinaryExpression* node) override {
            node->rhs->Accept(this);
            node->lhs->Accept(this);
            llvm::Value* lhs = values.top();
            values.pop();
            llvm::Value* rhs = values.top();
            values.pop();

            rhs = JITType::CastRHSToLHS(lhs->getType(), rhs, m->builder.get());            

            BinaryOpType op = (BinaryOpType)node->op;
            switch (op) {
                case BinaryOpType::ADDITION:
                    values.push(m->builder->CreateFAdd(lhs, rhs));
                    break;
                case BinaryOpType::SUBSTRACTION:
                    values.push(m->builder->CreateFSub(lhs, rhs));
                    break;
                case BinaryOpType::MULTIPLICATION:
                    values.push(m->builder->CreateFMul(lhs, rhs));
                    break;
                case BinaryOpType::DIVISION:
                    values.push(m->builder->CreateFDiv(lhs, rhs));
                    break;
                case BinaryOpType::SUPERIOR:
                {
                    llvm::Value* v = m->builder->CreateFCmpOGT(lhs, rhs);
                    values.push(v);
                    break;
                }
                case BinaryOpType::INFERIOR:
                {
                    llvm::Value* v = m->builder->CreateFCmpOLT(lhs, rhs);
                    values.push(v);
                    break;
                }
                case BinaryOpType::SUPERIOREQUAL:
                {
                    llvm::Value* v = m->builder->CreateFCmpOGE(lhs, rhs);
                    values.push(v);
                    break;
                }
                case BinaryOpType::INFERIOREQUAL:
                {
                    llvm::Value* v = m->builder->CreateFCmpOLE(lhs, rhs);
                    values.push(v);
                    break;
                }
                case BinaryOpType::EQUAL:
                {
                    llvm::Value* v = m->builder->CreateFCmpOEQ(lhs, rhs);
                    values.push(v);
                    break;
                }
                case BinaryOpType::NOTEQUAL:
                {
                    llvm::Value* v = m->builder->CreateFCmpONE(lhs, rhs);
                    values.push(v);
                    break;
                }
                default:
                    throw std::runtime_error{ "Unsupported binary operator." };
            }
        }

        void VisitLiteralExpression(ASTLiteralExpression* node) override {
            switch (node->type) {
                case ASTTypeInfo::TypeName::FLOAT:
                    values.push(llvm::ConstantFP::get(*c->context, llvm::APFloat(node->fValue)));
                    break;
                default:
                    break;
            }
        }

        void VisitVariableExpression(ASTVariableExpression* node) override {
            if (!namedVariables.count(node->name))
                throw std::runtime_error{ std::format("Undefined variable \"{}\".", node->name) };
            
            VarInfo& v = namedVariables[node->name].Top();

            if (v.scope == 0)
                values.push(m->builder->CreateLoad(v.v->getValueType(), v.v, node->name));
            else
                values.push(m->builder->CreateLoad(v.a->getAllocatedType(), v.a, node->name));
        }

        void VisitFunctionCall(ASTFunctionCall* node) override {
            if (JITBuiltins::IsBuiltinFunction(node->name)) {
                std::vector<llvm::Value*> argsv{};
                for (auto& expr : node->arguments) {
                    expr->Accept(this);
                    argsv.push_back(values.top());
                    values.pop();
                }

                values.push(JITBuiltins::CallBuiltinFunction(node->name, argsv, m->builder.get()));
            } else {
                llvm::Function* callee = m->module->getFunction(node->name);

                if (!callee)
                    throw std::runtime_error{ std::format("Unknown function referenced \"{}\".", node->name) };

                if (callee->arg_size() != node->arguments.size())
                    throw std::runtime_error{ std::format("The function \"{}\" was expecting {} argument(s) but was given {}.",
                        node->name, callee->arg_size(), node->arguments.size()) };

                std::vector<llvm::Value*> argsv{};
                for (auto& expr : node->arguments) {
                    expr->Accept(this);
                    argsv.push_back(values.top());
                    values.pop();
                }

                values.push(m->builder->CreateCall(callee, argsv));
            }
        }

    private:
        struct VarInfo {
            llvm::AllocaInst* a;
            llvm::GlobalVariable* v;
            uint32_t scope;
        };

        Module::LLVMModuleData* m;
        JITContext::LLVMJITData* c;

        std::unordered_map<std::string_view, FixedSizeStack<VarInfo, 32>> namedVariables;
        std::stack<llvm::Value*> values;
        uint32_t currentScope;
    };
}

VCL::JITContext::JITContext() : 
    jit{ std::make_unique<LLVMJITData>() } {
    Reset();
    if (!InitializeTarget())
        throw std::runtime_error{ "Could not initialize target." };

    auto epc = llvm::orc::SelfExecutorProcessControl::Create();
    if (auto err = epc.takeError())
        throw std::runtime_error{ std::format("(LLVM): {}", llvm::toString(std::move(err))) };

    auto es = std::make_unique<llvm::orc::ExecutionSession>(std::move(*epc));

    //llvm::orc::JITTargetMachineBuilder jtmb{ es->getExecutorProcessControl().getTargetTriple() };
    auto jtmb = llvm::orc::JITTargetMachineBuilder::detectHost(); //Seems better has it correctly add supported features
    if (auto err = jtmb.takeError())
        throw std::runtime_error{ std::format("(LLVM): {}", llvm::toString(std::move(err))) };

    auto dl = jtmb.get().getDefaultDataLayoutForTarget();
    if (auto err = dl.takeError())
        throw std::runtime_error{ std::format("(LLVM): {}", llvm::toString(std::move(err))) };

    jit->jit = std::make_unique<JIT>(std::move(es), std::move(*jtmb), std::move(*dl));
}

VCL::JITContext::~JITContext() {
    
}

std::shared_ptr<VCL::JITContext> VCL::JITContext::Create() {
    return std::make_shared<JITContext>();
}

void VCL::JITContext::Reset() {
    jit->context = std::make_unique<llvm::LLVMContext>();

    jit->lam = std::make_unique<llvm::LoopAnalysisManager>();
    jit->fam = std::make_unique<llvm::FunctionAnalysisManager>();
    jit->cgam = std::make_unique<llvm::CGSCCAnalysisManager>();
    jit->mam = std::make_unique<llvm::ModuleAnalysisManager>();

    jit->pb = std::make_unique<llvm::PassBuilder>();
    jit->pb->registerModuleAnalyses(*jit->mam);
    jit->pb->registerCGSCCAnalyses(*jit->cgam);
    jit->pb->registerFunctionAnalyses(*jit->fam);
    jit->pb->registerLoopAnalyses(*jit->lam);
    jit->pb->crossRegisterProxies(*jit->lam, *jit->fam, *jit->cgam, *jit->mam);

    jit->lpm = std::make_unique<llvm::LoopPassManager>();
    jit->fpm = std::make_unique<llvm::FunctionPassManager>();
    jit->cgpm = std::make_unique<llvm::CGSCCPassManager>();
    jit->mpm = std::make_unique<llvm::ModulePassManager>();

    jit->lpm->addPass(llvm::IndVarSimplifyPass{});
    jit->lpm->addPass(llvm::LICMPass{ llvm::LICMOptions{} });

    jit->fpm->addPass(llvm::createFunctionToLoopPassAdaptor(std::move(*jit->lpm)));

    jit->fpm->addPass(llvm::PromotePass{});
    jit->fpm->addPass(llvm::InstCombinePass{});
    jit->fpm->addPass(llvm::LoopVectorizePass{});
    jit->fpm->addPass(llvm::LoopUnrollPass{});
    jit->fpm->addPass(llvm::ReassociatePass{});
    jit->fpm->addPass(llvm::GVNPass{});
    jit->fpm->addPass(llvm::SimplifyCFGPass{});
    jit->fpm->addPass(llvm::SLPVectorizerPass{});

    jit->cgpm->addPass(llvm::createCGSCCToFunctionPassAdaptor(std::move(*jit->fpm)));

    jit->cgpm->addPass(llvm::InlinerPass{});

    jit->mpm->addPass(llvm::createModuleToPostOrderCGSCCPassAdaptor(std::move(*jit->cgpm)));
}

void VCL::JITContext::AddModule(std::unique_ptr<Module> module) {
    std::string m;
    llvm::raw_string_ostream output{ m };
    if (llvm::verifyModule(*module->module->module, &output))
        throw std::runtime_error{ std::format("(LLVM): {}", m) };
    llvm::orc::ThreadSafeModule threadSafeModule{ std::move(module->module->module), std::move(jit->context) };
    jit->context = std::make_unique<llvm::LLVMContext>();
    llvm::Error err = jit->jit->AddModule(std::move(threadSafeModule));
    if (err)
        throw std::runtime_error{ std::format("(LLVM): {}", llvm::toString(std::move(err))) };
}

void* VCL::JITContext::Lookup(std::string_view name) {
    auto e = jit->jit->Lookup(name);
    if (auto err = e.takeError())
        throw std::runtime_error{ std::format("(LLVM): {}", llvm::toString(std::move(err))) };
    return e.get().getAddress().toPtr<void*>();
}

void VCL::JITContext::BindGlobalVariable(std::string_view name, void* ptr) {
    jit->jit->BindGlobalVariable(name, ptr);
}

void VCL::JITContext::DumpObjects() {
    jit->jit->DumpObjects();
}

VCL::Module::Module(std::string_view name, std::shared_ptr<JITContext> context) :
    module{ std::make_unique<LLVMModuleData>(
        std::make_unique<llvm::Module>(name, *context->jit->context),
        std::make_unique<llvm::IRBuilder<>>(*context->jit->context)
    )}, context{ context } {
}

VCL::Module::~Module() {

}

std::unique_ptr<VCL::Module> VCL::Module::Create(std::string_view name, std::shared_ptr<JITContext> context) {
    return std::make_unique<Module>(name, context);
}

void VCL::Module::BindSource(const std::string& src) {
    std::unique_ptr<Parser> parser = Parser::Create();
    BindAST(parser->Parse(src));
}

void VCL::Module::BindAST(std::unique_ptr<ASTProgram> program) {
    ModuleBuilder builder{ this };
    program->Accept(&builder);
}

void VCL::Module::Serialize(const std::string& filename) {
    std::error_code ec;
    llvm::raw_fd_ostream out(filename, ec, llvm::sys::fs::OF_None);

    if (ec)
        throw std::runtime_error{ std::format("(LLVM): {}", ec.message()) };

    llvm::WriteBitcodeToFile(*module->module, out);
}

void VCL::Module::Deserialize(const std::string& filename) {
    auto buffOrErr = llvm::MemoryBuffer::getFile(filename);
    if (!buffOrErr)
        throw std::runtime_error{ std::format("(LLVM): {}", buffOrErr.getError().message()) };

    auto moduleOrErr = llvm::parseBitcodeFile(**buffOrErr, *context->jit->context);
    if (auto err = moduleOrErr.takeError())
        throw std::runtime_error{ std::format("(LLVM): {}", llvm::toString(std::move(err))) };

    module->module = std::move(*moduleOrErr);
}

void VCL::Module::Optimize() {
    context->jit->mpm->run(*module->module, *context->jit->mam);
}

std::string VCL::Module::Dump() {
    std::string str;
    llvm::raw_string_ostream output{ str };
    module->module->print(output, nullptr);
    return str;
}

size_t VCL::GetMaxVectorWidth() {
    return JITType::GetMaxVectorWidth();
}