/**
 * This tool is mainly made for debugging pruposes.
 */

#include <VCL/Core/SourceManager.hpp>
#include <VCL/Core/Diagnostic.hpp>
#include <VCL/Core/DiagnosticConsumer.hpp>
#include <VCL/Core/CompilerContext.hpp>
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
    std::vector<std::string> inputFilenames{};
    bool optimize = true;
    bool generateDebugInformation = false;
    std::string outputDirectory;
    std::string dumpIrDirectory;
    std::string dumpObjFilename;
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
            "\n-i, --input <filename>: input source file for pre compilation. this argument can be repeated." 
            "\n-o, --output <dir>: output compiled bitcode into this directory."
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
            options.inputFilenames.push_back(argv[idx]);
            ++idx;
            continue;
        }

        if (strcmp(argv[idx], "--dump-ir") == 0) {
            ++idx;
            if (idx >= argc) {
                std::cout << "Missing directory for --dump-ir." << std::endl;
                return false;
            }
            options.dumpIrDirectory = argv[idx];
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

        if (strcmp(argv[idx], "-o") == 0 || strcmp(argv[idx], "--outputs") == 0) {
            ++idx;
            if (idx >= argc) {
                std::cout << "Missing directory for -o." << std::endl;
                return false;
            }
            options.outputDirectory = argv[idx];
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

    if (options.inputFilenames.size() == 0) {
        std::cout << "No input filenames was given." << std::endl;
        return 0;
    }

    VCL::TextDiagnosticConsumer diagnosticConsumer{};
    VCL::CompilerContext cc{};
    cc.GetIdentifierTable().AddKeywords();
    cc.SetDiagnosticConsumer(&diagnosticConsumer);

    VCL::Target target{};

    for (size_t i = 0; i < options.inputFilenames.size(); ++i) {
        VCL::Source* source = cc.GetSourceManager().LoadFromDisk(options.inputFilenames[i]);
        if (!source)
            return -1;

        VCL::Lexer lexer{ source->GetBufferRef(), cc };
        VCL::TokenStream stream{ lexer };
        VCL::ASTContext astContext{};
        VCL::Sema sema{ astContext, cc };
        VCL::Parser parser{ stream, sema, cc };

        if (!parser.Parse()) {
            std::cout << "compilation error" << std::endl;
            return -1;
        }

        llvm::orc::ThreadSafeContext context{ std::make_unique<llvm::LLVMContext>() };
        llvm::orc::ThreadSafeModule module{  std::make_unique<llvm::Module>( "module", *context.getContext() ), context };
        VCL::CodeGenModule cgm{ module, astContext, cc, target };
        if (!cgm.Emit()) {
            std::cout << "compilation (CodeGen) error" << std::endl;
            return -1;
        }
        cgm.GetLLVMModule().dump();

    }
}