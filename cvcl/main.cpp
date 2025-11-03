/**
 * This tool is mainly made for debugging pruposes.
 */

#include <VCL/Core/SourceManager.hpp>
#include <VCL/Core/Diagnostic.hpp>
#include <VCL/Frontend/CompilerContext.hpp>
#include <VCL/Frontend/ExecutionSession.hpp>
#include <VCL/Frontend/CompilerInstance.hpp>
#include <VCL/Frontend/TextDiagnosticConsumer.hpp>
#include <VCL/Frontend/FrontendActions.hpp>
#include <VCL/Lex/Lexer.hpp>
#include <VCL/Lex/TokenStream.hpp>
#include <VCL/AST/ASTContext.hpp>
#include <VCL/Sema/Sema.hpp>
#include <VCL/Parse/Parser.hpp>
#include <VCL/CodeGen/CodeGenModule.hpp>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <filesystem>


struct Options {
    std::string inputFilename{};
    bool optimize = true;
    bool generateDebugInformation = false;
    std::string dumpIrFilename{};
    std::string dumpObjFilename{};
    std::string entryPoint = "Main";
};

bool GetOptions(int argc, const char** argv, Options& options) {
    int idx = 1;
    if (argc <= idx) {
        std::cout << "Missing argument(s). Use -h for help." << std::endl;
        return false;
    }

    while (idx < argc) {
        if (strcmp(argv[idx], "-h") == 0 || strcmp(argv[idx], "--help") == 0) {
            std::cout << 
            "\n-h, --help: show this help message." 
            "\n-i, --input <filename>: input source file for pre compilation." 
            "\n-e, --entry-point <name>: name of the program entry point (default is \"Main\")."
            "\n--dump-ir <dir>: dump input file(s) module(s) readable ir into this directory."
            "\n--dump-obj <file>: dump object to disk."
            "\n--no-optimization: disable any optimization on the ir." 
            "\n-g: generate debug information." << std::endl;
            return false;
        }

        if (strcmp(argv[idx], "-g") == 0) {
            ++idx;
            options.generateDebugInformation = true;
            continue;
        }

        if (strcmp(argv[idx], "-i") == 0 || strcmp(argv[idx], "--inputs") == 0) {
            ++idx;
            if (idx >= argc) {
                std::cout << "Missing filename for -i." << std::endl;
                return false;
            }
            options.inputFilename = argv[idx];
            ++idx;
            continue;
        }

        if (strcmp(argv[idx], "--dump-ir") == 0) {
            ++idx;
            if (idx >= argc) {
                std::cout << "Missing filename for --dump-ir." << std::endl;
                return false;
            }
            options.dumpIrFilename = argv[idx];
            ++idx;
            continue;
        }


        if (strcmp(argv[idx], "--dump-obj") == 0) {
            ++idx;
            if (idx >= argc) {
                std::cout << "Missing filename for --dump-obj." << std::endl;
                return false;
            }
            options.dumpObjFilename = argv[idx];
            ++idx;
            continue;
        }

        if (strcmp(argv[idx], "-e") == 0 || strcmp(argv[idx], "--entry-point") == 0) {
            ++idx;
            if (idx >= argc) {
                std::cout << "Missing name for -e." << std::endl;
                return false;
            }
            options.entryPoint = argv[idx];
            ++idx;
            continue;
        }

        if (strcmp(argv[idx], "--no-optimization") == 0) {
            options.optimize = false;
            ++idx;
            continue;
        }

        std::cout << "Wrong argument \"" << argv[idx] << "\". This argument does not exist." << std::endl;
        return false;
    }

    return true;
}


int main(int argc, const char* argv[]) {
    Options options{};
    if (!GetOptions(argc, argv, options))
        return 0;

    if (options.inputFilename.empty()) {
        std::cout << "No input filenames was given." << std::endl;
        return 0;
    }

    VCL::TextDiagnosticConsumer diagnosticConsumer{};
    VCL::CompilerContext cc{};
    VCL::ExecutionSession session{};
    cc.GetInvocation().GetDiagnosticOptions().SetDiagnosticConsumer(&diagnosticConsumer);

    cc.CreateDiagnosticEngine();
    cc.CreateSourceManager();
    cc.CreateIdentifierTable();
    cc.CreateTarget();
    cc.CreateLLVMContext();

    VCL::Source* source = cc.GetSourceManager().LoadFromDisk(options.inputFilename);

    VCL::EmitLLVMAction act{};

    std::shared_ptr<VCL::CompilerInstance> instance = cc.CreateInstance();
    instance->BeginSource(source);
    bool b = instance->ExecuteAction(act);
    instance->EndSource();

    if (!b)
        return -1;

    if (!options.dumpIrFilename.empty()) {
        std::ofstream irOutFile{ options.dumpIrFilename, std::ios::binary | std::ios::trunc };
        std::string str{};
        llvm::raw_string_ostream out{ str };
        act.GetModule().getModuleUnlocked()->print(out, nullptr);
        irOutFile << str;
        irOutFile.close();
        std::cout << "ir written to '" << options.dumpIrFilename << "'" << std::endl;
    }
    
    if (!session.SubmitModule(act.MoveModule())) {
        std::cout << llvm::toString(session.ConsumeLastError()) << std::endl;
        return -1;
    }

    void* entryPoint = session.Lookup(options.entryPoint);
    if (!entryPoint) {
        std::cout << llvm::toString(session.ConsumeLastError()) << std::endl;
        return -1;
    }
    
    return 0;
}