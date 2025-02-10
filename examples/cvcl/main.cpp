/*
* This tool is made for precompiling vcl into bytecode and debugging purposes. It also serves as an exemple on how to use the library.
*/

#include <vcl/vcl.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <filesystem>


struct Options {
    std::vector<std::string> inputFilenames{};
    bool optimize = true;
    std::string outputDirectory;
    std::string dumpIrDirectory;
    bool dumpObj = false;
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
            "\n--dump-obj: dump object file(s)."
            "\n--no-optimization: disable any optimization on the ir." << std::endl;
            return false;
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
            options.dumpObj = true;
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

int main(int argc, const char** argv) {
    Options options;
    if (!GetOptions(argc, argv, options))
        return 0;

    if (options.inputFilenames.size() == 0) {
        std::cout << "No input filenames was given." << std::endl;
        return 0;
    }

    std::shared_ptr<VCL::JITContext> context = VCL::JITContext::Create();

    if (options.dumpObj)
        context->DumpObjects();

    
    try {
        for (size_t i = 0; i < options.inputFilenames.size(); ++i) {
            std::string filepath = options.inputFilenames[i];
            std::string filename = filepath.substr(filepath.find_last_of("/\\") + 1);
            std::cout << "[" << ((float)(i + 1) / (float)(options.inputFilenames.size()) * 100.0f) << 
                "%] Building VCL \"" << filepath << "\"" << std::endl;
            
            std::ifstream file{ filepath, std::ios::binary };

            if (!file.is_open()) {
                std::cout << "Couldn't open source file \"" << filepath << "\"" << std::endl;
                return 0;
            }

            std::stringstream buffer{};
            buffer << file.rdbuf();
            std::string source = buffer.str();
            file.close();

            std::unique_ptr<VCL::Module> module = VCL::Module::Create(filename, context);
            module->BindSource(source);
            
            if (options.optimize)
                module->Optimize();

            if (!options.dumpIrDirectory.empty()) {
                std::filesystem::path p = std::filesystem::path{ options.dumpIrDirectory } / filename;
                p.replace_extension(".ll");
                std::ofstream irOutFile{ p, std::ios::binary | std::ios::trunc };
                irOutFile << module->Dump();
                irOutFile.close();
            }

            if (!options.outputDirectory.empty()) {
                std::filesystem::path p = std::filesystem::path{ options.dumpIrDirectory } / filename;
                p.replace_extension(".bc");
                module->Serialize(p);
            }
            
            context->AddModule(std::move(module));
        }

        context->Lookup(options.entryPoint);

    } catch (std::runtime_error& err) {
        std::cout << "[ERROR]: " << err.what() << std::endl; 
        return 0;
    }

    return 0;
}