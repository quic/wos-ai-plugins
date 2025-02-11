// =============================================================================
//
// Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
//
// SPDX-License-Identifier: BSD-3-Clause
//
// =============================================================================


#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>

#include <Genie/GenieDialog.h>
#include <Genie/GenieCommon.h>

#include "http.h"

#define CXXOPTS_VECTOR_DELIMITER '\0'

#include "httplib.h"
#include "json.hpp"

enum Model {
    LLAMA2_7B,
    LLAMA3_1_8B
};

std::string config;
std::string host = "127.0.0.1";
std::string port = "8002";
std::string config_path = "";
Model model;

std::vector<uint32_t> tokens;

std::unordered_set<std::string> commandLineArguments;
std::unordered_map<std::string, std::pair<bool, bool>> m_options;

std::map<Model, std::string> modelEnumToStringMap = {
    {LLAMA2_7B, "llama-v2-7b"},
    {LLAMA3_1_8B, "llama-v3.1-8b"},
};

std::string modelToString(Model model) {
    return modelEnumToStringMap[model];
}

Model stringToModel(const std::string& modelStr) {
    for (const auto& pair : modelEnumToStringMap) {
        if (pair.second == modelStr) {
            return pair.first;
        }
    }
    std::cout << "Invalid model string : " << modelStr << std::endl;
    std::cout << std::flush;
    exit(1);
}


std::string getModelConfig(Model model) {
    if (model == LLAMA2_7B) {
        return "./configs/llama2-7b/llama2-7b-htp-windows.json";
    }
    else if (model == LLAMA3_1_8B) {
        return "./configs/llama3p1-8b/llama3p1-8b-htp-windows.json";
    }
}

bool isSet(const std::string& name) {
    auto sought = m_options.find(name);
    return (sought != m_options.end()) && (sought->second).first;
}

bool isRequired(const std::string& name) {
    auto sought = m_options.find(name);
    return (sought != m_options.end()) && (sought->second).second;
}

void addOption(const std::string& name, bool set, bool isRequired) {
    m_options.emplace(name, std::make_pair(set, isRequired));
}

std::streamsize getFileSize(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    return file.tellg();
}

bool checkFileExistsAndReadable(const std::ifstream& fileStream, const std::string& fileName) {
    bool res = fileStream.good();
    if (!res) {
        std::cout << std::setw(24) << "File " << fileName << " doesn't exists or is in bad shape."
            << std::endl;
    }
    return res;
}

void printUsage(const char* program) {
    std::cout << "Usage:\n" << program << " [options]\n" << std::endl;
    std::cout << "Options:" << std::endl;

    int width = 88;

    std::cout << std::left << std::setw(width) << "  -h, --help";
    std::cout << "Show this help message and exit.\n" << std::endl;

    std::cout << std::setw(width) << "  -s SERVER or --server SERVER";
    std::cout << "Host or IP to use. Defaults to localhost\n" << std::endl;

    std::cout << std::setw(width) << "  -p PORT or --port PORT";
    std::cout << "Port to use. Defaults to 8002\n" << std::endl;

    std::cout << std::setw(width) << "  --config config";
    std::cout << "Config to use.\n" << std::endl;

    std::cout << std::setw(width) << "  --model model";
    std::cout << "Model to use. Possible values: llama-v2-7b, llama-v3.1-8b\n" << std::endl;

    std::cout << "Provides three HTTP APIs:\n" << std::endl;
    std::cout << "1. Health check: [Get] /api/health\n" << std::endl;
    std::cout << "2. Generate one token: [Post] /api/generate" << std::endl;
    std::cout << "\t Example Input Json:" << std::endl;
    std::cout << "\t\t {" << std::endl;
    std::cout << "\t\t\t \"ask\": \"Hello, tell me a tiny story\"" << std::endl;
    std::cout << "\t\t }" << std::endl << std::endl;
    std::cout << "\t Example Output Json:" << std::endl;
    std::cout << "\t\t {" << std::endl;
    std::cout << "\t\t\t \"generated_text\": \"Hi\"" << std::endl;
    std::cout << "\t\t }" << std::endl << std::endl;
    std::cout << "3. Generate stream API: [Post] /api/generate_stream\n" << std::endl;
    std::cout << "\t Example Input Json:" << std::endl;
    std::cout << "\t\t {" << std::endl;
    std::cout << "\t\t\t \"ask\": \"write a program to add numbers\"" << std::endl;
    std::cout << "\t\t\t \"system_prompt\": \"act as a code generator\"" << std::endl;
    std::cout << "\t\t }" << std::endl << std::endl;
    std::cout << "\t Example Output stream:" << std::endl;
    std::cout << "\t\t   Sure, I'd be happy to help! Here is a simple program ..." << std::endl;
}

std::vector<std::string> split(const std::string& str) {
    std::vector<std::string> words;

    std::string::size_type pos = 0;
    std::string::size_type prev = 0;
    while ((pos = str.find(',', pos)) != std::string::npos) {
        std::string word = str.substr(prev, pos - prev);
        if (word.length() > 0) {
            words.push_back(word);
        }
        prev = ++pos;
    }
    std::string word = str.substr(prev, pos - prev);
    if (word.length() > 0) {
        words.push_back(word);
    }

    return words;
}

bool parseCommandLineInput(int argc, char** argv) {
    bool invalidParam = false;
    std::string arg;
    /*if (argc == 1) {
        printUsage(argv[0]);
        std::exit(EXIT_SUCCESS);
    }*/
    for (int i = 1; i < argc; i++) {
        arg = argv[i];
        commandLineArguments.insert(arg);
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            std::exit(EXIT_SUCCESS);
        }
        else if (arg == "-s" || arg == "--server") {
            if (++i >= argc) {
                invalidParam = true;
                break;
            }
            host = argv[i];
            addOption("--server", true, false);
        }
        else if (arg == "-p" || arg == "--port") {
            if (++i >= argc) {
                invalidParam = true;
                break;
            }
            port = argv[i];
            addOption("--port", true, false);
        }
        else if (arg == "-c" || arg == "--config") {
            if (++i >= argc) {
                invalidParam = true;
                break;
            }
            config_path = argv[i];
            addOption("--config", true, false);
        }
        else if (arg == "-m" || arg == "--model") {
            if (++i >= argc) {
                invalidParam = true;
                break;
            }
            model = stringToModel(argv[i]);
            addOption("--model", true, false);
        }
        
        else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return false;
        }
    }
    if (invalidParam) {
        std::cerr << "ERROR: Invalid parameter for argument: " << arg << std::endl;
        printUsage(argv[0]);
        return false;
    }

    return true;
}


class Dialog {
public:
    class Config {
    public:
        Config(const std::string& config) {
            int32_t status = GenieDialogConfig_createFromJson(config.c_str(), &m_handle);
            if ((GENIE_STATUS_SUCCESS != status) || (!m_handle)) {
                throw std::runtime_error("Failed to create the dialog config.");
            }
        }

        ~Config() {
            int32_t status = GenieDialogConfig_free(m_handle);
            if (GENIE_STATUS_SUCCESS != status) {
                std::cerr << "Failed to free the dialog config." << std::endl;
            }
        }

        GenieDialogConfig_Handle_t operator()() const { return m_handle; }

    private:
        GenieDialogConfig_Handle_t m_handle = NULL;
    };

    Dialog(Config config) {
        int32_t status = GenieDialog_create(config(), &m_handle);
        if ((GENIE_STATUS_SUCCESS != status) || (!m_handle)) {
            throw std::runtime_error("Failed to create the dialog.");
        }
    }

    ~Dialog() {
        int32_t status = GenieDialog_free(m_handle);
        if (GENIE_STATUS_SUCCESS != status) {
            std::cerr << "Failed to free the dialog." << std::endl;
        }
    }

    void reset() {
        int32_t status = GenieDialog_reset(m_handle);
        if (GENIE_STATUS_SUCCESS != status) {
            throw std::runtime_error("Failed to reset the dialog.");
        }
    }

    bool query(const std::string prompt,
        const GenieDialog_QueryCallback_t callback,
        void * userData) {
        int32_t status = GenieDialog_query(m_handle,
            prompt.c_str(),
            GenieDialog_SentenceCode_t::GENIE_DIALOG_SENTENCE_COMPLETE,
            callback,
            userData);
        if (GENIE_STATUS_SUCCESS != status) {
            //throw std::runtime_error("Failed to query.");
            return false;
        }
        return true;
    }

    void save(const std::string name) {
        int32_t status = GenieDialog_save(m_handle, name.c_str());
        if (GENIE_STATUS_SUCCESS != status) {
            throw std::runtime_error("Failed to save.");
        }
    }

    void restore(const std::string name) {
        int32_t status = GenieDialog_restore(m_handle, name.c_str());
        if (GENIE_STATUS_SUCCESS != status) {
            throw std::runtime_error("Failed to restore.");
        }
    }

private:
    GenieDialog_Handle_t m_handle = NULL;
};


// HTTP
httplib::Server svr;

// Genie
#define GENIE_CHECK_STATUS(exit_code, msg)                                   \
  do {                                                                       \
    if (GENIE_STATUS_SUCCESS != status) {                                    \
      std::cerr << "ERROR at line " << __LINE__ << ": " << msg << std::endl; \
      exit(exit_code);                                                       \
    }                                                                        \
  } while (0)

namespace fs = std::filesystem;
using strvec = std::vector<std::string>;
using opvec = std::vector<std::pair<std::string, std::string>>;

std::string get_query(Model model, std::string system_prompt, std::string prompt) {
    if (model == LLAMA3_1_8B) {
        return std::format("<|begin_of_text|><|start_header_id|>system<|end_header_id|>\n{}\n<|eot_id|><|start_header_id|>user<|end_header_id|>\n\n{}<|eot_id|><|start_header_id|>assistant<|end_header_id|>", system_prompt, prompt);
    }
    return std::format("<s>[INST]\n<<SYS>>\n{}\n<</SYS>>\n\n{}[/INST]", system_prompt, prompt);
}

std::string read_input() {
    std::string in;
    std::getline(std::cin, in, '\0');
    return in;
}


class ClientCancelledException : public std::exception {
private:
    std::string message;

public:
    ClientCancelledException(const char* msg)
        : message(msg)
    {
    }

    const char* what() const throw()
    {
        return message.c_str();
    }
};

int main(int argc, char* argv[]) {
    if (!parseCommandLineInput(argc, argv)) {
        return EXIT_FAILURE;
    }

    if (config_path == "") {
        config_path = getModelConfig(model);
    }

    std::ifstream configStream = std::ifstream(config_path);

    if (!checkFileExistsAndReadable(configStream,
        config_path)) {  // Error encountered don't go further
        return false;
    }

    std::getline(configStream, config, '\0');

    try {
        Dialog dialog{ Dialog::Config(config) };

        svr.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
            res.set_content("{}", "application/json");
            });

        svr.Post("/api/generate", [&](const httplib::Request& request, httplib::Response& res) {
            nlohmann::json requestBody = nlohmann::json::parse(request.body);
            std::string prompt = requestBody["ask"];
            std::string system_prompt = requestBody["system_prompt"];
            std::string query = get_query(model, system_prompt, prompt);
            std::time_t time = std::time(nullptr);
            fprintf(stderr, "\n%s [INFO] %s\n", std::asctime(std::localtime(&time)), query.data());

            nlohmann::json out;
            dialog.reset();
            char* response;

            GenieDialog_QueryCallback_t callback = [](const char* responseStr,
                const GenieDialog_SentenceCode_t sentenceCode,
                const void* userData) {
                    void* non_const_userData = const_cast<void*>(userData);
                    char* response = static_cast<char*>(non_const_userData);
                    response = const_cast<char*>(responseStr);
                };

            if (!dialog.query(query, callback, static_cast<void*>(&response))) {
                std::string message = "ERROR: Failed to run llama model";
                fprintf(stderr, std::format("\nERROR: Failed to query dialog").data());
            }
            out["generated_text"] = response;
            res.set_content(out.dump(), "application/json");
        });

        svr.Post("/api/generate_stream", [&](const httplib::Request& request, httplib::Response& res) {
            nlohmann::json requestBody = nlohmann::json::parse(request.body);
            std::string prompt = requestBody["ask"];
            std::string system_prompt = requestBody["system_prompt"];
            std::string query = get_query(model, system_prompt, prompt);
            std::time_t time = std::time(nullptr);
            fprintf(stderr, "\n%s [INFO] %s\n", std::asctime(std::localtime(&time)), query.data());

            res.set_chunked_content_provider(
                "text/plain",
                [&, query](size_t offset, httplib::DataSink& sink) {
                    dialog.reset();

                    auto callback = ([](const char* responseStr,
                                        const GenieDialog_SentenceCode_t sentenceCode,
                                        const void* userData) {
                        std::string str = responseStr;
                        void* non_const_userData = const_cast<void*>(userData);
                        httplib::DataSink* s = static_cast<httplib::DataSink*>(non_const_userData);
                        
                        if (!s->is_writable()) {
                            throw ClientCancelledException("Request cancelled");
                        }
                        std::cout << str << std::endl;
                        s->write(str.data(), str.length());
                    });
                    
                    try {
                        if (!dialog.query(query,
                            (GenieDialog_QueryCallback_t)callback, static_cast<void*>(&sink))) {
                            if (sink.is_writable()) {
                                std::string message = "ERROR: Failed to run llama model";
                                sink.write(message.data(), message.length());
                            }
                            fprintf(stderr, std::format("\nERROR: Failed to query dialog").data());
                        }
                    }
                    catch (ClientCancelledException& e) {
                        std::time_t result = std::time(nullptr);
                        std::cout << std::asctime(std::localtime(&result)) <<
                            " [INFO] Client cancelled the request" << std::flush;
                    }
                    
                    sink.done();
                    return false;
                }
            );
        });

        std::cout << "\nLocal HTTP Server is up at " << host << ":" << port << std::flush;
        svr.listen(host, stoi(port));
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}

