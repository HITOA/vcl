/*
* This tool is made for precompiling vcl into bytecode and debugging purposes. It also serves as an exemple on how to use the library.
*/

#include <VCL/VCL.hpp>
#include <VCL/Error.hpp>

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

class ConsoleLogger : public VCL::Logger {
public:
    void Log(VCL::Message& message) override {
        const char* severityStr[] = {
            "None",
            "Error",
            "Warning",
            "Info",
            "Debug"
        };
        int severityInt = (int)message.severity;
        printf("[%s] %s\n", severityStr[severityInt], message.message.c_str());
    };
};

int main(int argc, const char** argv) {
    Options options;
    if (!GetOptions(argc, argv, options))
        return 0;

    if (options.inputFilenames.size() == 0) {
        std::cout << "No input filenames was given." << std::endl;
        return 0;
    }

    std::shared_ptr<ConsoleLogger> logger = std::make_shared<ConsoleLogger>();

    std::unique_ptr<VCL::Parser> parser = VCL::Parser::Create(logger);

    std::unique_ptr<VCL::ExecutionSession> session = VCL::ExecutionSession::Create(logger);

    if (!options.dumpObjFilename.empty()) {
        std::filesystem::path dumpObjFilename = std::filesystem::path{ options.dumpObjFilename };
        std::filesystem::path dumpObjDir = dumpObjFilename.parent_path();
        std::string dumpObjName = dumpObjFilename.filename();
        session->SetDumpObject(dumpObjDir, dumpObjName);
    }

    for (size_t i = 0; i < options.inputFilenames.size(); ++i) {
        std::filesystem::path filepath = options.inputFilenames[i];
        std::string filename = filepath.filename();
        std::cout << "[" << ((float)(i + 1) / (float)(options.inputFilenames.size()) * 100.0f) << 
            "%] Building VCL " << filepath << std::endl;
        
        if (auto source = VCL::Source::LoadFromDisk(filepath); source.has_value()) {
            try {
                std::unique_ptr<VCL::ASTProgram> program = parser->Parse(*source);

                std::unique_ptr<VCL::Module> module = session->CreateModule(std::move(program));
                VCL::ModuleDebugInformationSettings settings{};
                settings.generateDebugInformation = options.generateDebugInformation;
                settings.optimized = options.optimize;
                module->Emit(settings);
                module->Verify();

                if (options.optimize)
                    module->Optimize();

                if (!options.dumpIrDirectory.empty()) {
                    std::filesystem::path dumpIrPath = std::filesystem::path{ options.dumpIrDirectory } / filename;
                    dumpIrPath.replace_extension(".ll");
                    std::ofstream irOutFile{ dumpIrPath, std::ios::binary | std::ios::trunc };
                    irOutFile << module->Dump();
                    irOutFile.close();
                }

                session->SubmitModule(std::move(module));

            } catch (VCL::Exception& exception) {
                logger->Error("{}: {}\n{}", exception.location.ToString(), exception.what(), exception.location.ToStringDetailed());
            } catch (std::exception& exception) {
                logger->Error("Unexpected: {}", exception.what());
            }
            
        } else {
            logger->Error("{}", source.error());
            continue;
        }
    }

    try {
        if (!session->Lookup(options.entryPoint))
            return 0;
    } catch (std::runtime_error& exception) {
        return 0;
    }
}