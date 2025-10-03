#include "SELinuxManager.h"
#include "common.h"
#include "json11.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include "utils.h"

SELinuxManager::SELinuxManager() 
    : process_manager_(std::make_unique<ProcessManager>()), 
      sudo_validated_(false) 
{
    m_env_parser.load_env_file(Utils::get_env_secret_file_path());
    sudo_password_ = m_env_parser.get(CONST_KEY_SUDO_PSWD, "PASSWORD_TEST");
}

SELinuxManager::~SELinuxManager() {
}

SELinuxInfo SELinuxManager::getSELinuxInfo() {
    SELinuxInfo info;
    info.status = SELinuxStatus::UNKNOWN;
    info.configured = false;
    
    // Check if SELinux is installed by looking for sestatus command
    auto [exit_code, output] = executeSELinuxCommandWithOutput("which", {"sestatus"});
    if (exit_code != 0) {
        info.status = SELinuxStatus::NOT_INSTALLED;
        info.error_message = "SELinux utilities not found";
        return info;
    }
    
    // Get current SELinux status
    auto [status_code, status_output] = executeSELinuxCommandWithOutput("sestatus");
    if (status_code == 0) {
        std::istringstream iss(status_output);
        std::string line;
        
        while (std::getline(iss, line)) {
            if (line.find("SELinux status:") != std::string::npos) {
                if (line.find("enabled") != std::string::npos) {
                    info.configured = true;
                }
                else if (line.find("disabled") != std::string::npos) {
                    info.status = SELinuxStatus::DISABLED;
                }
            } else if (line.find("SELinuxfs mount:") != std::string::npos) {
                size_t pos = line.find(":");
                if (pos != std::string::npos) {
                    info.mount_point = line.substr(pos + 2);
                }
            } else if (line.find("SELinux root directory:") != std::string::npos) {
                size_t pos = line.find(":");
                if (pos != std::string::npos) {
                    info.root_directory = line.substr(pos + 2);
                }
            } else if (line.find("Loaded policy name:") != std::string::npos) {
                size_t pos = line.find(":");
                if (pos != std::string::npos) {
                    info.policy = line.substr(pos + 2);
                }
            } else if (line.find("Current mode:") != std::string::npos) {
                size_t pos = line.find(":");
                if (pos != std::string::npos) {
                    info.mode = line.substr(pos + 2);
                    if (info.mode == "enforcing") {
                        info.status = SELinuxStatus::ENFORCING;
                    } else if (info.mode == "permissive") {
                        info.status = SELinuxStatus::PERMISSIVE;
                    } else if (info.mode == "disabled") {
                        info.status = SELinuxStatus::DISABLED;
                    }
                }
            } else if (line.find("Mode from config file:") != std::string::npos) {
                size_t pos = line.find(":");
                if (pos != std::string::npos) {
                    info.config_mode = line.substr(pos + 2);
                }
            } else if (line.find("Policy MLS status:") != std::string::npos) {
                size_t pos = line.find(":");
                if (pos != std::string::npos) {
                    info.mls_status = line.substr(pos + 2);
                }
            } else if (line.find("Policy deny_unknown status:") != std::string::npos) {
                size_t pos = line.find(":");
                if (pos != std::string::npos) {
                    info.deny_unknown_status = line.substr(pos + 2);
                }
            } else if (line.find("Memory protection checking:") != std::string::npos) {
                size_t pos = line.find(":");
                if (pos != std::string::npos) {
                    info.memory_protection = line.substr(pos + 2);
                }
            } else if (line.find("Max kernel policy version:") != std::string::npos) {
                size_t pos = line.find(":");
                if (pos != std::string::npos) {
                    info.max_kernel_policy_version = line.substr(pos + 2);
                }
            }
        }
    } else {
        // Fallback to getenforce
        auto [enforce_code, enforce_output] = executeSELinuxCommandWithOutput("getenforce");
        if (enforce_code == 0) {
            std::string mode = enforce_output;
            mode.erase(std::remove(mode.begin(), mode.end(), '\n'), mode.end());
            info.mode = mode;
            
            if (mode == "Enforcing") {
                info.status = SELinuxStatus::ENFORCING;
            } else if (mode == "Permissive") {
                info.status = SELinuxStatus::PERMISSIVE;
            } else if (mode == "Disabled") {
                info.status = SELinuxStatus::DISABLED;
            }
            
            // Check if configured by looking at config file
            std::ifstream config_file("/etc/selinux/config");
            if (config_file.is_open()) {
                std::string line;
                while (std::getline(config_file, line)) {
                    if (line.find("SELINUX=") != std::string::npos) {
                        info.configured = true;
                        break;
                    }
                }
                config_file.close();
            }
        }
    }
    
    return info;
}

SELinuxStatus SELinuxManager::getStatus() {
    return getSELinuxInfo().status;
}

bool SELinuxManager::isSELinuxInstalled() {
    auto [exit_code, output] = executeSELinuxCommandWithOutput("which", {"sestatus"});
    return exit_code == 0;
}

bool SELinuxManager::isSELinuxEnabled() {
    SELinuxInfo info = getSELinuxInfo();
    return info.status == SELinuxStatus::ENFORCING || info.status == SELinuxStatus::PERMISSIVE;
}

SELinuxOperationResult SELinuxManager::disableSELinux(bool permanent) {
    if (!isSELinuxInstalled()) {
        return SELinuxOperationResult::NOT_SUPPORTED;
    }
    
    SELinuxStatus current_status = getStatus();
    if (current_status == SELinuxStatus::DISABLED) {
        return SELinuxOperationResult::SUCCESS;
    }
    
    // First set to permissive mode temporarily
    if (current_status == SELinuxStatus::ENFORCING) {
        auto result = setPermissiveMode();
        if (result != SELinuxOperationResult::SUCCESS) {
            return result;
        }
    }
    
    // If permanent disable is requested, update config file
    if (permanent) {
        if (!updateSELinuxConfig("disabled")) {
            return SELinuxOperationResult::FAILURE;
        }
    }
    
    return SELinuxOperationResult::SUCCESS;
}

SELinuxOperationResult SELinuxManager::enableSELinux() {
    if (!isSELinuxInstalled()) {
        return SELinuxOperationResult::NOT_SUPPORTED;
    }
    
    SELinuxStatus current_status = getStatus();
    if (current_status != SELinuxStatus::DISABLED) {
        return SELinuxOperationResult::SUCCESS;
    }
    
    // Update config to enabling mode (will require reboot)
    if (!updateSELinuxConfig("enforcing")) {
        return SELinuxOperationResult::FAILURE;
    }
    
    return SELinuxOperationResult::SUCCESS;
}

SELinuxOperationResult SELinuxManager::setEnforcingMode() {
    if (!isSELinuxInstalled()) {
        return SELinuxOperationResult::NOT_SUPPORTED;
    }
    
    auto [exit_code, output] = executeSELinuxCommandWithOutput("setenforce", {"1"});
    if (exit_code == 0) {
        return SELinuxOperationResult::SUCCESS;
    }
    
    // Check if sudo authentication failed
    if (output.find("Authentication failure") != std::string::npos || 
        output.find("incorrect password") != std::string::npos) {
        return SELinuxOperationResult::AUTH_FAILED;
    }
    
    return SELinuxOperationResult::FAILURE;
}

SELinuxOperationResult SELinuxManager::setPermissiveMode() {
    if (!isSELinuxInstalled()) {
        return SELinuxOperationResult::NOT_SUPPORTED;
    }
    
    auto [exit_code, output] = executeSELinuxCommandWithOutput("setenforce", {"0"});
    if (exit_code == 0) {
        return SELinuxOperationResult::SUCCESS;
    }
    
    // Check if sudo authentication failed
    if (output.find("Authentication failure") != std::string::npos || 
        output.find("incorrect password") != std::string::npos) {
        return SELinuxOperationResult::AUTH_FAILED;
    }
    
    return SELinuxOperationResult::FAILURE;
}

SELinuxOperationResult SELinuxManager::restoreSELinuxContext(const std::string& path) {
    if (!isSELinuxInstalled()) {
        return SELinuxOperationResult::NOT_SUPPORTED;
    }
    
    struct stat path_stat;
    if (stat(path.c_str(), &path_stat) != 0) {
        return SELinuxOperationResult::FAILURE;
    }
    
    std::vector<std::string> args;
    if (S_ISDIR(path_stat.st_mode)) {
        args = {"-R", path};
    } else {
        args = {path};
    }
    
    auto [exit_code, output] = executeSELinuxCommandWithOutput("restorecon", args);
    if (exit_code == 0) {
        return SELinuxOperationResult::SUCCESS;
    }
    
    if (output.find("Authentication failure") != std::string::npos || 
        output.find("incorrect password") != std::string::npos) {
        return SELinuxOperationResult::AUTH_FAILED;
    }
    
    return SELinuxOperationResult::FAILURE;
}

SELinuxOperationResult SELinuxManager::applyDefaultContext(const std::string& path) {
    return restoreSELinuxContext(path);
}

void SELinuxManager::setSudoPassword(const std::string& password) {
    sudo_password_ = password;
    sudo_validated_ = false;
}

bool SELinuxManager::validateSudoPassword() {
    if (sudo_password_.empty()) {
        return false;
    }
    
    auto [pid, exit_code] = process_manager_->startProcessBlockingAsRoot(
        "echo", {"test"}, {}, sudo_password_, nullptr);
    
    sudo_validated_ = (exit_code == 0);
    return sudo_validated_;
}

std::string SELinuxManager::executeSELinuxCommand(const std::string& command, const std::vector<std::string>& args) {
    auto [exit_code, output] = executeSELinuxCommandWithOutput(command, args);
    return output;
}

std::tuple<int, std::string> SELinuxManager::executeSELinuxCommandWithOutput(const std::string& command, const std::vector<std::string>& args) {
    std::string output;
    int exit_code = -1;
    
    auto output_callback = [&output](const std::string& data) {
        output += data;
    };
    
    if (requiresSudo(command)) {
        if (sudo_password_.empty()) {
            return std::make_tuple(-1, "Sudo password required");
        }
        
        auto [pid, status] = process_manager_->startProcessBlockingAsRoot(
            command, args, {}, sudo_password_, output_callback);
        exit_code = status;
    } else {
        auto [pid, status] = process_manager_->startProcessBlocking(
            command, args, {}, output_callback);
        exit_code = status;
    }
    
    return std::make_tuple(exit_code, output);
}

bool SELinuxManager::requiresSudo(const std::string& command) {
    // Commands that typically require root privileges
    const std::vector<std::string> sudo_commands = {
        "setenforce", "restorecon", "semanage", "setsebool", 
        "chcon", "fixfiles", "load_policy"
    };
    
    return std::find(sudo_commands.begin(), sudo_commands.end(), command) != sudo_commands.end();
}

SELinuxStatus SELinuxManager::parseSELinuxStatus(const std::string& output) {
    if (output.find("Enforcing") != std::string::npos) {
        return SELinuxStatus::ENFORCING;
    } else if (output.find("Permissive") != std::string::npos) {
        return SELinuxStatus::PERMISSIVE;
    } else if (output.find("Disabled") != std::string::npos) {
        return SELinuxStatus::DISABLED;
    }
    return SELinuxStatus::UNKNOWN;
}

bool SELinuxManager::updateSELinuxConfig(const std::string& new_mode) {
    if (new_mode != "enforcing" && new_mode != "permissive" && new_mode != "disabled") {
        return false;
    }
    
    std::string config_path = "/etc/selinux/config";
    std::ifstream config_file(config_path);
    if (!config_file.is_open()) {
        return false;
    }
    
    std::vector<std::string> lines;
    std::string line;
    bool found_selinux_line = false;
    
    while (std::getline(config_file, line)) {
        if (line.find("SELINUX=") == 0) {
            lines.push_back("SELINUX=" + new_mode);
            found_selinux_line = true;
        } else {
            lines.push_back(line);
        }
    }
    config_file.close();
    
    if (!found_selinux_line) {
        lines.push_back("SELINUX=" + new_mode);
    }
    
    // Requires sudo to write to /etc/selinux/config
    if (sudo_password_.empty()) {
        return false;
    }
    
    // Create temporary file with new content
    std::string temp_path = "/tmp/selinux_config_tmp";
    std::ofstream temp_file(temp_path);
    if (!temp_file.is_open()) {
        return false;
    }
    
    for (const auto& l : lines) {
        temp_file << l << std::endl;
    }
    temp_file.close();
    
    // Copy temporary file to actual config location with sudo
    // auto [exit_code, output] = executeSELinuxCommandWithOutput("cp", {temp_path, config_path});
    auto [_pid, _status_code] = process_manager_->startProcessBlockingAsRoot("cp", {temp_path, config_path}, {}, sudo_password_);

    executeSELinuxCommandWithOutput("rm", {temp_path});
    
    return _status_code == 0;
}

// REST API Implementation
void SELinuxManager::registerRestEndpoints(crow::SimpleApp& app) {
    CROW_ROUTE(app, "/api/selinux/info").methods("GET"_method)
    ([this](const crow::request& req) {
        return handleGetSELinuxInfo();
    });

    CROW_ROUTE(app, "/api/selinux/status").methods("GET"_method)
    ([this](const crow::request& req) {
        return handleGetSELinuxStatus();
    });

    CROW_ROUTE(app, "/api/selinux/disable").methods("POST"_method)
    ([this](const crow::request& req) {
        return handleDisableSELinux(req);
    });

    CROW_ROUTE(app, "/api/selinux/enable").methods("POST"_method)
    ([this](const crow::request& req) {
        return handleEnableSELinux();
    });

    CROW_ROUTE(app, "/api/selinux/enforcing").methods("POST"_method)
    ([this](const crow::request& req) {
        return handleSetEnforcingMode();
    });

    CROW_ROUTE(app, "/api/selinux/permissive").methods("POST"_method)
    ([this](const crow::request& req) {
        return handleSetPermissiveMode();
    });

    CROW_ROUTE(app, "/api/selinux/restore-context").methods("POST"_method)
    ([this](const crow::request& req) {
        return handleRestoreSELinuxContext(req);
    });

    CROW_ROUTE(app, "/api/selinux/apply-context").methods("POST"_method)
    ([this](const crow::request& req) {
        return handleApplyDefaultContext(req);
    });

    // CROW_ROUTE(app, "/api/selinux/validate-sudo").methods("POST"_method)
    // ([this](const crow::request& req) {
    //     return handleValidateSudoPassword(req);
    // });
}

bool SELinuxManager::requiresSudoPermission(const std::string& operation) {
    const std::vector<std::string> sudo_operations = {
        "disable_selinux",
        "enable_selinux", 
        "set_enforcing",
        "set_permissive",
        "restore_context",
        "apply_context"
    };
    
    return std::find(sudo_operations.begin(), sudo_operations.end(), operation) != sudo_operations.end();
}

crow::response SELinuxManager::createUnauthorizedResponse(const std::string& operation) {
    crow::json::wvalue response;
    response["success"] = false;
    response["error"] = "Unauthorized: Invalid or missing sudo password";
    response["operation"] = operation;
    response["message"] = "This operation requires valid sudo credentials. Please provide the correct password.";
    
    return crow::response(crow::status::UNAUTHORIZED, response);
}

crow::response SELinuxManager::handleGetSELinuxInfo() {
    crow::response res;
    try {
        SELinuxInfo info = getSELinuxInfo();
        
        crow::json::wvalue json_response;
        json_response["status"] = static_cast<int>(info.status);
        json_response["mode"] = info.mode;
        json_response["policy"] = info.policy;
        json_response["mount_point"] = info.mount_point;
        json_response["root_directory"] = info.root_directory;
        json_response["config_mode"] = info.config_mode;
        json_response["mls_status"] = info.mls_status;
        json_response["deny_unknown_status"] = info.deny_unknown_status;
        json_response["memory_protection"] = info.memory_protection;
        json_response["max_kernel_policy_version"] = info.max_kernel_policy_version;
        json_response["configured"] = info.configured;
        json_response["error_message"] = info.error_message;
        
        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
    }
    return res;
}

crow::response SELinuxManager::handleGetSELinuxStatus() {
    crow::response res;
    try {
        SELinuxStatus status = getStatus();
        
        crow::json::wvalue json_response;
        json_response["status"] = static_cast<int>(status);
        
        switch (status) {
            case SELinuxStatus::DISABLED:
                json_response["status_name"] = "DISABLED";
                break;
            case SELinuxStatus::PERMISSIVE:
                json_response["status_name"] = "PERMISSIVE";
                break;
            case SELinuxStatus::ENFORCING:
                json_response["status_name"] = "ENFORCING";
                break;
            case SELinuxStatus::NOT_INSTALLED:
                json_response["status_name"] = "NOT_INSTALLED";
                break;
            default:
                json_response["status_name"] = "UNKNOWN";
                break;
        }
        
        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
    }
    return res;
}

crow::response SELinuxManager::handleDisableSELinux(const crow::request& req) {
    crow::response res;
    try {
        if (requiresSudoPermission("disable_selinux")) {
            if (!validateSudoPassword()) {
                return createUnauthorizedResponse("disable_selinux");
            }
        }
        
        auto json_data = crow::json::load(req.body);
        bool permanent = false;
        
        if (json_data && json_data.has("permanent")) {
            permanent = json_data["permanent"].b();
        }
        
        SELinuxOperationResult result = disableSELinux(permanent);
        
        crow::json::wvalue json_response;
        json_response["success"] = (result == SELinuxOperationResult::SUCCESS);
        json_response["result"] = static_cast<int>(result);
        
        switch (result) {
            case SELinuxOperationResult::SUCCESS:
                json_response["message"] = "SELinux disabled successfully";
                break;
            case SELinuxOperationResult::FAILURE:
                json_response["message"] = "Failed to disable SELinux";
                break;
            case SELinuxOperationResult::NOT_SUPPORTED:
                json_response["message"] = "SELinux is not installed";
                break;
            case SELinuxOperationResult::AUTH_FAILED:
                json_response["message"] = "Sudo authentication failed";
                break;
        }
        
        res.code = (result == SELinuxOperationResult::SUCCESS) ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
    }
    return res;
}

crow::response SELinuxManager::handleEnableSELinux() {
    crow::response res;
    try {
        if (requiresSudoPermission("enable_selinux")) {
            if (!validateSudoPassword()) {
                return createUnauthorizedResponse("enable_selinux");
            }
        }
        
        SELinuxOperationResult result = enableSELinux();
        
        crow::json::wvalue json_response;
        json_response["success"] = (result == SELinuxOperationResult::SUCCESS);
        json_response["result"] = static_cast<int>(result);
        
        switch (result) {
            case SELinuxOperationResult::SUCCESS:
                json_response["message"] = "SELinux enabled successfully (reboot required)";
                break;
            case SELinuxOperationResult::FAILURE:
                json_response["message"] = "Failed to enable SELinux";
                break;
            case SELinuxOperationResult::NOT_SUPPORTED:
                json_response["message"] = "SELinux is not installed";
                break;
            case SELinuxOperationResult::AUTH_FAILED:
                json_response["message"] = "Sudo authentication failed";
                break;
        }
        
        res.code = (result == SELinuxOperationResult::SUCCESS) ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
    }
    return res;
}

crow::response SELinuxManager::handleSetEnforcingMode() {
    crow::response res;
    try {
        if (requiresSudoPermission("set_enforcing")) {
            if (!validateSudoPassword()) {
                return createUnauthorizedResponse("set_enforcing");
            }
        }
        
        SELinuxOperationResult result = setEnforcingMode();
        
        crow::json::wvalue json_response;
        json_response["success"] = (result == SELinuxOperationResult::SUCCESS);
        json_response["result"] = static_cast<int>(result);
        
        switch (result) {
            case SELinuxOperationResult::SUCCESS:
                json_response["message"] = "SELinux set to enforcing mode";
                break;
            case SELinuxOperationResult::FAILURE:
                json_response["message"] = "Failed to set SELinux to enforcing mode";
                break;
            case SELinuxOperationResult::NOT_SUPPORTED:
                json_response["message"] = "SELinux is not installed";
                break;
            case SELinuxOperationResult::AUTH_FAILED:
                json_response["message"] = "Sudo authentication failed";
                break;
        }
        
        res.code = (result == SELinuxOperationResult::SUCCESS) ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
    }
    return res;
}

crow::response SELinuxManager::handleSetPermissiveMode() {
    crow::response res;
    try {
        if (requiresSudoPermission("set_permissive")) {
            if (!validateSudoPassword()) {
                return createUnauthorizedResponse("set_permissive");
            }
        }
        
        SELinuxOperationResult result = setPermissiveMode();
        
        crow::json::wvalue json_response;
        json_response["success"] = (result == SELinuxOperationResult::SUCCESS);
        json_response["result"] = static_cast<int>(result);
        
        switch (result) {
            case SELinuxOperationResult::SUCCESS:
                json_response["message"] = "SELinux set to permissive mode";
                break;
            case SELinuxOperationResult::FAILURE:
                json_response["message"] = "Failed to set SELinux to permissive mode";
                break;
            case SELinuxOperationResult::NOT_SUPPORTED:
                json_response["message"] = "SELinux is not installed";
                break;
            case SELinuxOperationResult::AUTH_FAILED:
                json_response["message"] = "Sudo authentication failed";
                break;
        }
        
        res.code = (result == SELinuxOperationResult::SUCCESS) ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
    }
    return res;
}

crow::response SELinuxManager::handleRestoreSELinuxContext(const crow::request& req) {
    crow::response res;
    try {
        if (requiresSudoPermission("restore_context")) {
            if (!validateSudoPassword()) {
                return createUnauthorizedResponse("restore_context");
            }
        }
        
        auto json_data = crow::json::load(req.body);
        std::string path;
        
        if (json_data && json_data.has("path")) {
            path = json_data["path"].s();
        }
        
        if (path.empty()) {
            crow::json::wvalue error_response;
            error_response["error"] = "Path is required";
            res.code = 400;
            res.set_header("Content-Type", "application/json");
            res.write(error_response.dump());
            return res;
        }
        
        SELinuxOperationResult result = restoreSELinuxContext(path);
        
        crow::json::wvalue json_response;
        json_response["success"] = (result == SELinuxOperationResult::SUCCESS);
        json_response["result"] = static_cast<int>(result);
        
        switch (result) {
            case SELinuxOperationResult::SUCCESS:
                json_response["message"] = "SELinux context restored successfully";
                break;
            case SELinuxOperationResult::FAILURE:
                json_response["message"] = "Failed to restore SELinux context";
                break;
            case SELinuxOperationResult::NOT_SUPPORTED:
                json_response["message"] = "SELinux is not installed";
                break;
            case SELinuxOperationResult::AUTH_FAILED:
                json_response["message"] = "Sudo authentication failed";
                break;
        }
        
        res.code = (result == SELinuxOperationResult::SUCCESS) ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
    }
    return res;
}

crow::response SELinuxManager::handleApplyDefaultContext(const crow::request& req) {
    crow::response res;
    try {
        if (requiresSudoPermission("apply_context")) {
            if (!validateSudoPassword()) {
                return createUnauthorizedResponse("apply_context");
            }
        }
        
        auto json_data = crow::json::load(req.body);
        std::string path;
        
        if (json_data && json_data.has("path")) {
            path = json_data["path"].s();
        }
        
        if (path.empty()) {
            crow::json::wvalue error_response;
            error_response["error"] = "Path is required";
            res.code = 400;
            res.set_header("Content-Type", "application/json");
            res.write(error_response.dump());
            return res;
        }
        
        SELinuxOperationResult result = applyDefaultContext(path);
        
        crow::json::wvalue json_response;
        json_response["success"] = (result == SELinuxOperationResult::SUCCESS);
        json_response["result"] = static_cast<int>(result);
        
        switch (result) {
            case SELinuxOperationResult::SUCCESS:
                json_response["message"] = "Default SELinux context applied successfully";
                break;
            case SELinuxOperationResult::FAILURE:
                json_response["message"] = "Failed to apply default SELinux context";
                break;
            case SELinuxOperationResult::NOT_SUPPORTED:
                json_response["message"] = "SELinux is not installed";
                break;
            case SELinuxOperationResult::AUTH_FAILED:
                json_response["message"] = "Sudo authentication failed";
                break;
        }
        
        res.code = (result == SELinuxOperationResult::SUCCESS) ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
    }
    return res;
}

// crow::response SELinuxManager::handleValidateSudoPassword(const crow::request& req) {
//     crow::response res;
//     try {
//         std::string err;
//         auto json_data = json11::Json::parse(req.body, err);
//         std::string password;
        
//         if (!err.empty()) {
//             json11::Json error_response = json11::Json::object {
//                 {"error", "Invalid JSON format: " + err}
//             };
//             res.code = 400;
//             res.set_header("Content-Type", "application/json");
//             res.write(error_response.dump());
//             return res;
//         }
        
//         if (json_data.is_object() && json_data["password"].is_string()) {
//             password = json_data["password"].string_value();
//         }
        
//         if (password.empty()) {
//             json11::Json error_response = json11::Json::object {
//                 {"error", "Password is required"}
//             };
//             res.code = 400;
//             res.set_header("Content-Type", "application/json");
//             res.write(error_response.dump());
//             return res;
//         }
        
//         setSudoPassword(password);
//         bool validated = validateSudoPassword();
        
//         json11::Json json_response = json11::Json::object {
//             {"success", validated},
//             {"message", validated ? "Sudo password validated successfully" : "Invalid sudo password"}
//         };
        
//         res.code = validated ? 200 : 401;
//         res.set_header("Content-Type", "application/json");
//         res.write(json_response.dump());
        
//     } catch (const std::exception& e) {
//         json11::Json error_response = json11::Json::object {
//             {"error", e.what()}
//         };
//         res.code = crow::status::INTERNAL_SERVER_ERROR;
//         res.set_header("Content-Type", "application/json");
//         res.write(error_response.dump());
//     }
//     return res;
// }
