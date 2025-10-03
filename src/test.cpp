#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <crow.h>
#include "test.h"
#include "utils.h"
#include "httplib.h"
#include "json11.hpp"
#include "dotenv.hpp"
#include "EnvConfig.hpp"
#include <chrono>
#include <fstream>


/*
 * Test class that loads .env the same way the main app does,
 * discovers the REST port via EnvConfig, and runs all test methods
 * when requested.
 */
Test::Test() {

    EnvParser _parser;
    _parser.load_env_file(CONST_FILE_ENV_SECRET_PRIVATE);
    auto _password = _parser.get(CONST_KEY_SUDO_PSWD, "PASSWORD_TEST");

    // Create base URL
    int rest_port = EnvConfig::get_int_value(EnvKey::REST_PORT);
    if (rest_port <= 0) {
        crow::logger(crow::LogLevel::ERROR) << "Invalid REST port: " << rest_port ;
        // We'll handle this error in the test methods
        base_url = "";
    } else {
        std::stringstream url_ss;
        url_ss << "http://127.0.0.1:" << rest_port;
        base_url = url_ss.str();
    }

    // Register tests here
    tests.push_back({"sudo", [this, _password]() { return this->REST_test_sudo(_password); }});
    tests.push_back({"docker_install", [this, _password]() { return this->REST_test_docker_install(_password); }});
    tests.push_back({"docker_uninstall", [this, _password]() { return this->REST_test_docker_uninstall(_password); }});
    tests.push_back({"project_load", [this]() { return this->REST_test_project_load(); }});
    tests.push_back({"project_remove", [this]() { return this->REST_test_project_remove(); }});
    tests.push_back({"project_start", [this]() { return this->REST_test_project_start(); }});
    tests.push_back({"project_stop", [this]() { return this->REST_test_project_stop(); }});
    tests.push_back({"project_restart", [this]() { return this->REST_test_project_restart(); }});
    tests.push_back({"project_services", [this]() { return this->REST_test_project_services(); }});

    EnvConfig::ensure_env_file_exists(".env");
    EnvParser parser;
    parser.load_env_file(".env");
}

    // Run all registered tests and return true if all passed
bool Test::run_all() {
    bool all_passed = true;
    crow::logger(crow::LogLevel::Info) << "Running " << tests.size() << " tests...";
    for (const auto& [name, fn] : tests) {
        crow::logger(crow::LogLevel::Info) << "[TEST] " << name << " ... ";
        bool ok = false;
        try {
            ok = fn();
        } catch (const std::exception& ex) {
            crow::logger(crow::LogLevel::Info) << "EXCEPTION: " << ex.what() ;
            ok = false;
        } catch (...) {
            crow::logger(crow::LogLevel::Info) << "UNKNOWN EXCEPTION";
            ok = false;
        }

        if (ok) {
            crow::logger(crow::LogLevel::Info) << "PASS --> " << name;
        } else {
            crow::logger(crow::LogLevel::Info) << "FAIL --> " << name;
            all_passed = false;
        }
    }
    crow::logger(crow::LogLevel::Info) << "Tests completed. " << (all_passed ? "ALL PASSED" : "SOME FAILED") ;
    return all_passed;
}

// Existing sudo test upgraded to return bool
bool Test::REST_test_sudo(const std::string& _password) {
    assertm(!base_url.empty(), "Base URL is empty");
    
    httplib::Client client(base_url.c_str());
    client.set_connection_timeout(5); // 5 seconds

    json11::Json json_data = json11::Json::object{
        {"password", _password}
    };
    std::string json_str = json_data.dump();

    auto res = client.Post("/api/test/sudo", json_str, "application/json");
    if (res) {
        crow::logger(crow::LogLevel::Info) << "Test Status: " << res->status ;
        crow::logger(crow::LogLevel::Info) << "Test Response: " << res->body ;
        // Consider 2xx as success
        return (res->status >= 200 && res->status < 300);
    } else {
        crow::logger(crow::LogLevel::ERROR) << "Error: " << httplib::to_string(res.error()) ;
        return false;
    }
}

// Test method for /api/docker/install REST endpoint
bool Test::REST_test_docker_install(const std::string& _password) {
    assertm(!base_url.empty(), "Base URL is empty");
    
    httplib::Client client(base_url.c_str());
    client.set_connection_timeout(5); // 5 seconds
    // client.set_read_timeout(1000);
    // client.set_write_timeout(1000);
    // client.set_max_timeout(1000);

    // Now attempt to install Docker
    auto res = client.Post("/api/docker/install");
    if (res) {
        crow::logger(crow::LogLevel::Info) << "Test Status: " << res->status << " ";
        crow::logger(crow::LogLevel::Info) << "test Response: " << res->body << " ";
        // Consider 2xx as success
        return (res->status >= 200 && res->status < 300);
    } else {
        crow::logger(crow::LogLevel::ERROR) << "Error: " << httplib::to_string(res.error()) ;
        return false;
    }
}

// Run a specific test by name and return true if it passed
bool Test::run_test(const std::string& _test_name) {
    // Find the test by name
    auto it = std::find_if(tests.begin(), tests.end(),
        [&_test_name](const std::pair<std::string, std::function<bool()>>& test) {
            return test.first == _test_name;
        });

    // If test not found, log error and return false
    if (it == tests.end()) {
        crow::logger(crow::LogLevel::ERROR) << "Test not found: " << _test_name;
        return false;
    }

    // Extract test function
    const auto& [name, fn] = *it;

    crow::logger(crow::LogLevel::Info) << "[TEST] " << name << " ... ";

    bool ok = false;
    try {
        ok = fn();
    } catch (const std::exception& ex) {
        crow::logger(crow::LogLevel::Info) << "EXCEPTION: " << ex.what();
        ok = false;
    } catch (...) {
        crow::logger(crow::LogLevel::Info) << "UNKNOWN EXCEPTION";
        ok = false;
    }

    if (ok) {
        crow::logger(crow::LogLevel::Info) << "PASS --> " << name;
    } else {
        crow::logger(crow::LogLevel::Info) << "FAIL --> " << name;
    }

    return ok;
}

// Function that main will call to run tests if --test flag is passed
// void run_all_tests() {
//     Test t;
//     bool ok = t.run_all();
//     if (ok) {
//         crow::logger(crow::LogLevel::Info) << "All tests passed.";
//         std::exit(EXIT_SUCCESS);
//     } else {
//         crow::logger(crow::LogLevel::ERROR) << "Some tests failed.";
//         std::exit(EXIT_FAILURE);
//     }
// }

// Run tests based on input argument
bool Test::run_from_test_arg(const std::string& _test_arg) {

    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (_test_arg == "all") {
        return run_all();
    }
    
    // Parse comma-separated test names
    std::vector<std::string> test_names;
    size_t start = 0;
    size_t end = _test_arg.find(',');
    while (end != std::string::npos) {
        test_names.push_back(_test_arg.substr(start, end - start));
        start = end + 1;
        end = _test_arg.find(',', start);
    }
    test_names.push_back(_test_arg.substr(start));
    
    // Run each test
    bool all_passed = true;
    for (const auto& test_name : test_names) {
        if (!run_test(test_name)) {
            all_passed = false;
        }
    }
    
    return all_passed;
}

// Test method for /api/docker/uninstall REST endpoint
bool Test::REST_test_docker_uninstall(const std::string& _password) {
    assertm(!base_url.empty(), "Base URL is empty");
    
    httplib::Client client(base_url.c_str());
    client.set_connection_timeout(5); // 5 seconds

    // Now attempt to uninstall Docker
    auto res = client.Delete("/api/docker/uninstall");
    if (res) {
        crow::logger(crow::LogLevel::Info) << "Test Status: " << res->status << " ";
        crow::logger(crow::LogLevel::Info) << "Test Response: " << res->body << " ";
        // Consider 2xx as success
        return (res->status >= 200 && res->status < 300);
    } else {
        crow::logger(crow::LogLevel::ERROR) << "Error: " << httplib::to_string(res.error()) ;
        return false;
    }
}

// Return list of test names
std::vector<std::string> Test::get_test_names() const {
    std::vector<std::string> test_names;
    for (const auto& [name, fn] : tests) {
        test_names.push_back(name);
    }
    return test_names;
}

// Return help string with usage information and available tests
std::string Test::help() {
    std::string help_str = "Test Usage:\n";
    help_str += "  --test all                    : Run all available tests\n";
    help_str += "  --test <test_name>            : Run a specific test\n";
    help_str += "  --test test1,test2,test3      : Run multiple tests (comma-separated)\n\n";
    help_str += "Available tests:\n";
    
    for (const auto& [name, fn] : tests) {
        help_str += "  " + name + "\n";
    }
    
    return help_str;
}

// Test method for /api/projects/<string>/remove REST endpoint
bool Test::REST_test_project_remove() {
    assertm(!base_url.empty(), "Base URL is empty");
    
    httplib::Client client(base_url.c_str());
    client.set_connection_timeout(5); // 5 seconds

    // First, we need to load a project to test removing it
    // Generate a unique project name for testing
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::string project_name = "test_project_remove_" + std::to_string(now);
    
    // Path to the example project archive
    std::string archive_path = "../example_project.7z";
    if(!std::filesystem::exists(archive_path))
    {
        archive_path = "example_project.7z";
    }
    
    // Check if the archive file exists
    std::ifstream file(archive_path);
    if (!file.good()) {
        crow::logger(crow::LogLevel::ERROR) << "Test archive file not found: " << archive_path;
        return false;
    }
    file.close();

    // Prepare JSON payload for loading project
    json11::Json json_data_load = json11::Json::object{
        {"archive_path", archive_path},
        {"project_name", project_name},
        {"password", "secret"}  // Password for the encrypted archive
    };
    std::string json_str_load = json_data_load.dump();

    // Load the project first
    auto res_load = client.Post("/api/projects/load", json_str_load, "application/json");
    if (!res_load || res_load->status < 200 || res_load->status >= 300) {
        crow::logger(crow::LogLevel::ERROR) << "Failed to load project for remove test";
        if (res_load) {
            crow::logger(crow::LogLevel::ERROR) << "Load Status: " << res_load->status;
            crow::logger(crow::LogLevel::ERROR) << "Load Response: " << res_load->body;
        } else {
            crow::logger(crow::LogLevel::ERROR) << "Load Error: " << httplib::to_string(res_load.error());
        }
        return false;
    }

    // Now test the remove endpoint
    json11::Json json_data_remove = json11::Json::object{
        {"remove_files", true}
    };
    std::string json_str_remove = json_data_remove.dump();
    
    auto res = client.Delete(("/api/projects/" + project_name + "/remove").c_str(), json_str_remove, "application/json");
    if (res) {
        crow::logger(crow::LogLevel::Info) << "Project Remove Test Status: " << res->status;
        crow::logger(crow::LogLevel::Info) << "Project Remove Test Response: " << res->body;
        
        // Consider 2xx as success
        return (res->status >= 200 && res->status < 300);
    } else {
        crow::logger(crow::LogLevel::ERROR) << "Error: " << httplib::to_string(res.error());
        return false;
    }
}

// Test method for /api/projects/<string>/start REST endpoint
bool Test::REST_test_project_start() {
    assertm(!base_url.empty(), "Base URL is empty");
    
    httplib::Client client(base_url.c_str());
    client.set_connection_timeout(5); // 5 seconds

    // First, we need to load a project to test starting it
    // Generate a unique project name for testing
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::string project_name = "test_project_start_" + std::to_string(now);
    
    // Path to the example project archive
    std::string archive_path = "../example_project.7z";
    if(!std::filesystem::exists(archive_path))
    {
        archive_path = "example_project.7z";
    }
    
    // Check if the archive file exists
    std::ifstream file(archive_path);
    if (!file.good()) {
        crow::logger(crow::LogLevel::ERROR) << "Test archive file not found: " << archive_path;
        return false;
    }
    file.close();

    // Prepare JSON payload for loading project
    json11::Json json_data_load = json11::Json::object{
        {"archive_path", archive_path},
        {"project_name", project_name},
        {"password", "secret"}  // Password for the encrypted archive
    };
    std::string json_str_load = json_data_load.dump();

    // Load the project first
    auto res_load = client.Post("/api/projects/load", json_str_load, "application/json");
    if (!res_load || res_load->status < 200 || res_load->status >= 300) {
        crow::logger(crow::LogLevel::ERROR) << "Failed to load project for start test";
        if (res_load) {
            crow::logger(crow::LogLevel::ERROR) << "Load Status: " << res_load->status;
            crow::logger(crow::LogLevel::ERROR) << "Load Response: " << res_load->body;
        } else {
            crow::logger(crow::LogLevel::ERROR) << "Load Error: " << httplib::to_string(res_load.error());
        }
        return false;
    }

    // Now test the start endpoint
    auto res = client.Post(("/api/projects/" + project_name + "/start").c_str());
    if (res) {
        crow::logger(crow::LogLevel::Info) << "Project Start Test Status: " << res->status;
        crow::logger(crow::LogLevel::Info) << "Project Start Test Response: " << res->body;
        
        // Consider 2xx as success
        bool success = (res->status >= 200 && res->status < 300);
        
        // Clean up by stopping and unloading the project
        if (success) {
            crow::logger(crow::LogLevel::Info) << "Project started successfully, attempting to stop and unload...";
            auto stop_res = client.Post(("/api/projects/" + project_name + "/stop").c_str());
            if (stop_res) {
                crow::logger(crow::LogLevel::Info) << "Stop Test Status: " << stop_res->status;
            } else {
                crow::logger(crow::LogLevel::WARNING) << "Failed to make stop request: " << httplib::to_string(stop_res.error());
            }
            
            auto unload_res = client.Post(("/api/projects/" + project_name + "/unload").c_str());
            if (unload_res) {
                crow::logger(crow::LogLevel::Info) << "Unload Test Status: " << unload_res->status;
            } else {
                crow::logger(crow::LogLevel::WARNING) << "Failed to make unload request: " << httplib::to_string(unload_res.error());
            }
        }
        
        return success;
    } else {
        crow::logger(crow::LogLevel::ERROR) << "Error: " << httplib::to_string(res.error());
        return false;
    }
}

// Test method for /api/projects/<string>/stop REST endpoint
bool Test::REST_test_project_stop() {
    assertm(!base_url.empty(), "Base URL is empty");
    
    httplib::Client client(base_url.c_str());
    client.set_connection_timeout(5); // 5 seconds

    // First, we need to load and start a project to test stopping it
    // Generate a unique project name for testing
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::string project_name = "test_project_stop_" + std::to_string(now);
    
    // Path to the example project archive
    std::string archive_path = "../example_project.7z";
    if(!std::filesystem::exists(archive_path))
    {
        archive_path = "example_project.7z";
    }
    
    // Check if the archive file exists
    std::ifstream file(archive_path);
    if (!file.good()) {
        crow::logger(crow::LogLevel::ERROR) << "Test archive file not found: " << archive_path;
        return false;
    }
    file.close();

    // Prepare JSON payload for loading project
    json11::Json json_data_load = json11::Json::object{
        {"archive_path", archive_path},
        {"project_name", project_name},
        {"password", "secret"}  // Password for the encrypted archive
    };
    std::string json_str_load = json_data_load.dump();

    // Load the project first
    auto res_load = client.Post("/api/projects/load", json_str_load, "application/json");
    if (!res_load || res_load->status < 200 || res_load->status >= 300) {
        crow::logger(crow::LogLevel::ERROR) << "Failed to load project for stop test";
        if (res_load) {
            crow::logger(crow::LogLevel::ERROR) << "Load Status: " << res_load->status;
            crow::logger(crow::LogLevel::ERROR) << "Load Response: " << res_load->body;
        } else {
            crow::logger(crow::LogLevel::ERROR) << "Load Error: " << httplib::to_string(res_load.error());
        }
        return false;
    }

    // Start the project
    auto res_start = client.Post(("/api/projects/" + project_name + "/start").c_str());
    if (!res_start || res_start->status < 200 || res_start->status >= 300) {
        crow::logger(crow::LogLevel::ERROR) << "Failed to start project for stop test";
        if (res_start) {
            crow::logger(crow::LogLevel::ERROR) << "Start Status: " << res_start->status;
            crow::logger(crow::LogLevel::ERROR) << "Start Response: " << res_start->body;
        } else {
            crow::logger(crow::LogLevel::ERROR) << "Start Error: " << httplib::to_string(res_start.error());
        }
        
        // Clean up by unloading the project
        auto unload_res = client.Post(("/api/projects/" + project_name + "/unload").c_str());
        if (unload_res) {
            crow::logger(crow::LogLevel::Info) << "Unload Test Status: " << unload_res->status;
        } else {
            crow::logger(crow::LogLevel::WARNING) << "Failed to make unload request: " << httplib::to_string(unload_res.error());
        }
        
        return false;
    }

    // Now test the stop endpoint
    auto res = client.Post(("/api/projects/" + project_name + "/stop").c_str());
    if (res) {
        crow::logger(crow::LogLevel::Info) << "Project Stop Test Status: " << res->status;
        crow::logger(crow::LogLevel::Info) << "Project Stop Test Response: " << res->body;
        
        // Consider 2xx as success
        bool success = (res->status >= 200 && res->status < 300);
        
        // Clean up by unloading the project
        auto unload_res = client.Post(("/api/projects/" + project_name + "/unload").c_str());
        if (unload_res) {
            crow::logger(crow::LogLevel::Info) << "Unload Test Status: " << unload_res->status;
        } else {
            crow::logger(crow::LogLevel::WARNING) << "Failed to make unload request: " << httplib::to_string(unload_res.error());
        }
        
        return success;
    } else {
        crow::logger(crow::LogLevel::ERROR) << "Error: " << httplib::to_string(res.error());
        
        // Clean up by unloading the project
        auto unload_res = client.Post(("/api/projects/" + project_name + "/unload").c_str());
        if (unload_res) {
            crow::logger(crow::LogLevel::Info) << "Unload Test Status: " << unload_res->status;
        } else {
            crow::logger(crow::LogLevel::WARNING) << "Failed to make unload request: " << httplib::to_string(unload_res.error());
        }
        
        return false;
    }
}

// Test method for /api/projects/<string>/restart REST endpoint
bool Test::REST_test_project_restart() {
    assertm(!base_url.empty(), "Base URL is empty");
    
    httplib::Client client(base_url.c_str());
    client.set_connection_timeout(5); // 5 seconds

    // First, we need to load and start a project to test restarting it
    // Generate a unique project name for testing
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::string project_name = "test_project_restart_" + std::to_string(now);
    
    // Path to the example project archive
    std::string archive_path = "../example_project.7z";
    if(!std::filesystem::exists(archive_path))
    {
        archive_path = "example_project.7z";
    }
    
    // Check if the archive file exists
    std::ifstream file(archive_path);
    if (!file.good()) {
        crow::logger(crow::LogLevel::ERROR) << "Test archive file not found: " << archive_path;
        return false;
    }
    file.close();

    // Prepare JSON payload for loading project
    json11::Json json_data_load = json11::Json::object{
        {"archive_path", archive_path},
        {"project_name", project_name},
        {"password", "secret"}  // Password for the encrypted archive
    };
    std::string json_str_load = json_data_load.dump();

    // Load the project first
    auto res_load = client.Post("/api/projects/load", json_str_load, "application/json");
    if (!res_load || res_load->status < 200 || res_load->status >= 300) {
        crow::logger(crow::LogLevel::ERROR) << "Failed to load project for restart test";
        if (res_load) {
            crow::logger(crow::LogLevel::ERROR) << "Load Status: " << res_load->status;
            crow::logger(crow::LogLevel::ERROR) << "Load Response: " << res_load->body;
        } else {
            crow::logger(crow::LogLevel::ERROR) << "Load Error: " << httplib::to_string(res_load.error());
        }
        return false;
    }

    // Start the project
    auto res_start = client.Post(("/api/projects/" + project_name + "/start").c_str());
    if (!res_start || res_start->status < 200 || res_start->status >= 300) {
        crow::logger(crow::LogLevel::ERROR) << "Failed to start project for restart test";
        if (res_start) {
            crow::logger(crow::LogLevel::ERROR) << "Start Status: " << res_start->status;
            crow::logger(crow::LogLevel::ERROR) << "Start Response: " << res_start->body;
        } else {
            crow::logger(crow::LogLevel::ERROR) << "Start Error: " << httplib::to_string(res_start.error());
        }
        
        // Clean up by unloading the project
        auto unload_res = client.Post(("/api/projects/" + project_name + "/unload").c_str());
        if (unload_res) {
            crow::logger(crow::LogLevel::Info) << "Unload Test Status: " << unload_res->status;
        } else {
            crow::logger(crow::LogLevel::WARNING) << "Failed to make unload request: " << httplib::to_string(unload_res.error());
        }
        
        return false;
    }

    // Now test the restart endpoint
    auto res = client.Post(("/api/projects/" + project_name + "/restart").c_str());
    if (res) {
        crow::logger(crow::LogLevel::Info) << "Project Restart Test Status: " << res->status;
        crow::logger(crow::LogLevel::Info) << "Project Restart Test Response: " << res->body;
        
        // Consider 2xx as success
        bool success = (res->status >= 200 && res->status < 300);
        
        // Clean up by stopping and unloading the project
        if (success) {
            crow::logger(crow::LogLevel::Info) << "Project restarted successfully, attempting to stop and unload...";
            auto stop_res = client.Post(("/api/projects/" + project_name + "/stop").c_str());
            if (stop_res) {
                crow::logger(crow::LogLevel::Info) << "Stop Test Status: " << stop_res->status;
            } else {
                crow::logger(crow::LogLevel::WARNING) << "Failed to make stop request: " << httplib::to_string(stop_res.error());
            }
            
            auto unload_res = client.Post(("/api/projects/" + project_name + "/unload").c_str());
            if (unload_res) {
                crow::logger(crow::LogLevel::Info) << "Unload Test Status: " << unload_res->status;
            } else {
                crow::logger(crow::LogLevel::WARNING) << "Failed to make unload request: " << httplib::to_string(unload_res.error());
            }
        }
        
        return success;
    } else {
        crow::logger(crow::LogLevel::ERROR) << "Error: " << httplib::to_string(res.error());
        
        // Clean up by stopping and unloading the project
        auto stop_res = client.Post(("/api/projects/" + project_name + "/stop").c_str());
        if (stop_res) {
            crow::logger(crow::LogLevel::Info) << "Stop Test Status: " << stop_res->status;
        } else {
            crow::logger(crow::LogLevel::WARNING) << "Failed to make stop request: " << httplib::to_string(stop_res.error());
        }
        
        auto unload_res = client.Post(("/api/projects/" + project_name + "/unload").c_str());
        if (unload_res) {
            crow::logger(crow::LogLevel::Info) << "Unload Test Status: " << unload_res->status;
        } else {
            crow::logger(crow::LogLevel::WARNING) << "Failed to make unload request: " << httplib::to_string(unload_res.error());
        }
        
        return false;
    }
}

// Test method for /api/projects/<string>/services REST endpoint
bool Test::REST_test_project_services() {
    assertm(!base_url.empty(), "Base URL is empty");
    
    httplib::Client client(base_url.c_str());
    client.set_connection_timeout(5); // 5 seconds

    // First, we need to load a project to test getting its services
    // Generate a unique project name for testing
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::string project_name = "test_project_services_" + std::to_string(now);
    
    // Path to the example project archive
    std::string archive_path = "../example_project.7z";
    if(!std::filesystem::exists(archive_path))
    {
        archive_path = "example_project.7z";
    }
    
    // Check if the archive file exists
    std::ifstream file(archive_path);
    if (!file.good()) {
        crow::logger(crow::LogLevel::ERROR) << "Test archive file not found: " << archive_path;
        return false;
    }
    file.close();

    // Prepare JSON payload for loading project
    json11::Json json_data_load = json11::Json::object{
        {"archive_path", archive_path},
        {"project_name", project_name},
        {"password", "secret"}  // Password for the encrypted archive
    };
    std::string json_str_load = json_data_load.dump();

    // Load the project first
    auto res_load = client.Post("/api/projects/load", json_str_load, "application/json");
    if (!res_load || res_load->status < 200 || res_load->status >= 300) {
        crow::logger(crow::LogLevel::ERROR) << "Failed to load project for services test";
        if (res_load) {
            crow::logger(crow::LogLevel::ERROR) << "Load Status: " << res_load->status;
            crow::logger(crow::LogLevel::ERROR) << "Load Response: " << res_load->body;
        } else {
            crow::logger(crow::LogLevel::ERROR) << "Load Error: " << httplib::to_string(res_load.error());
        }
        return false;
    }

    // Now test the services endpoint
    auto res = client.Get(("/api/projects/" + project_name + "/services").c_str());
    if (res) {
        crow::logger(crow::LogLevel::Info) << "Project Services Test Status: " << res->status;
        crow::logger(crow::LogLevel::Info) << "Project Services Test Response: " << res->body;
        
        // Consider 2xx as success
        bool success = (res->status >= 200 && res->status < 300);
        
        // Clean up by unloading the project
        auto unload_res = client.Post(("/api/projects/" + project_name + "/unload").c_str());
        if (unload_res) {
            crow::logger(crow::LogLevel::Info) << "Unload Test Status: " << unload_res->status;
        } else {
            crow::logger(crow::LogLevel::WARNING) << "Failed to make unload request: " << httplib::to_string(unload_res.error());
        }
        
        return success;
    } else {
        crow::logger(crow::LogLevel::ERROR) << "Error: " << httplib::to_string(res.error());
        
        // Clean up by unloading the project
        auto unload_res = client.Post(("/api/projects/" + project_name + "/unload").c_str());
        if (unload_res) {
            crow::logger(crow::LogLevel::Info) << "Unload Test Status: " << unload_res->status;
        } else {
            crow::logger(crow::LogLevel::WARNING) << "Failed to make unload request: " << httplib::to_string(unload_res.error());
        }
        
        return false;
    }
}

// Test method for /api/projects/load REST endpoint
bool Test::REST_test_project_load() {
    assertm(!base_url.empty(), "Base URL is empty");
    
    httplib::Client client(base_url.c_str());
    client.set_connection_timeout(5); // 5 seconds

    // Generate a unique project name for testing
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::string project_name = "test_project_1";// + std::to_string(now);
    
    // Path to the example project archive
    std::string archive_path = "../example_project.7z";
    if(!std::filesystem::exists(archive_path))
    {
        archive_path = "example_project.7z";
    }
    
    // Check if the archive file exists
    std::ifstream file(archive_path);
    if (!file.good()) {
        crow::logger(crow::LogLevel::ERROR) << "Test archive file not found: " << archive_path;
        return false;
    }
    file.close();

    // Prepare JSON payload
    json11::Json json_data = json11::Json::object{
        {"archive_path", archive_path},
        {"project_name", project_name},
        {"password", "secret"}  // Password for the encrypted archive
    };
    std::string json_str = json_data.dump();

    // Make POST request to /api/projects/load
    auto res = client.Post("/api/projects/load", json_str, "application/json");
    if (res) {
        crow::logger(crow::LogLevel::Info) << "Project Load Test Status: " << res->status;
        crow::logger(crow::LogLevel::Info) << "Project Load Test Response: " << res->body;
        
        // Consider 2xx as success
        bool success = (res->status >= 200 && res->status < 300);
        
        // If project was loaded successfully, try to unload it to clean up
        /*
        if (success) {
            crow::logger(crow::LogLevel::Info) << "Project loaded successfully, attempting to unload...";
            auto unload_res = client.Post(("/api/projects/" + project_name + "/unload").c_str());
            if (unload_res) {
                crow::logger(crow::LogLevel::Info) << "Unload Test Status: " << unload_res->status;
                if (unload_res->status >= 200 && unload_res->status < 300) {
                    crow::logger(crow::LogLevel::Info) << "Project unloaded successfully";
                } else {
                    crow::logger(crow::LogLevel::WARNING) << "Failed to unload project: " << unload_res->body;
                }
            } else {
                crow::logger(crow::LogLevel::WARNING) << "Failed to make unload request: " << httplib::to_string(unload_res.error());
            }
        }
        */
        
        return success;
    } else {
        crow::logger(crow::LogLevel::ERROR) << "Error: " << httplib::to_string(res.error());
        return false;
    }
}
