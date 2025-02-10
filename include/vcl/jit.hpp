#pragma once

#include <vcl/ast.hpp>
#include <vcl/parser.hpp>

#include <string>
#include <string_view>
#include <memory>
#include <iostream>


namespace VCL {

    class Module;

    class JITContext {
    public:
        JITContext();
        ~JITContext();

        void Reset();
        void AddModule(std::unique_ptr<Module> module);
        void* Lookup(std::string_view name);
        void BindGlobalVariable(std::string_view name, void* ptr);

        void DumpObjects();

        static std::shared_ptr<JITContext> Create();

    private:
        friend class Module;
        friend class ModuleBuilder;
        struct LLVMJITData;
        std::unique_ptr<LLVMJITData> jit;
    };

    class Module {
    public:
        Module() = delete;
        Module(std::string_view name, std::shared_ptr<JITContext> context);
        ~Module();

        void BindSource(const std::string& src);
        void BindAST(std::unique_ptr<ASTProgram> program);

        void Serialize(const std::string& filename);
        void Deserialize(const std::string& filename);

        void Optimize();

        std::string Dump();


        static std::unique_ptr<Module> Create(std::string_view name, std::shared_ptr<JITContext> context);

    private:
        friend class JITContext;
        friend class ModuleBuilder;
        struct LLVMModuleData;
        std::unique_ptr<LLVMModuleData> module;
        std::shared_ptr<JITContext> context;
    };

    static size_t GetMaxVectorWidth();

}