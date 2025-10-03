#include "DockerManager.h"
#include "resourceextractor.h"
#include "utils.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include "json11.hpp"
#include "MarkdownToHtml.h"
#include <thread>
#include <chrono>
#include <sys/stat.h>
#include <unistd.h>
#include <chrono>
#include <cstdlib>
#include <set>
#include "EnvConfig.hpp"
#include "dotenv.hpp"

namespace fs = std::filesystem;

bool DockerManager::validateSudoPassword() {
    if (sudo_password_.empty()) {
        return false;
    }
    
    // Check if we have a recent validation (within timeout)
    auto now = std::chrono::steady_clock::now();
    if (sudo_validated_ && 
        (now - last_sudo_validation_) < SUDO_TIMEOUT) {
        return true;
    }
    
    try {
        bool _sudo_ok = Utils::test_sudo(sudo_password_);
        
        if (_sudo_ok) {
            sudo_validated_ = true;
            last_sudo_validation_ = now;
            broadcastLog("auth", "Sudo password validated successfully", "info");
            return true;
        }
    } catch (const std::exception& e) {
        broadcastLog("auth", "Sudo password validation failed: " + std::string(e.what()), "error");
    }
    
    sudo_validated_ = false;
    broadcastLog("auth", "Invalid sudo password provided", "error");
    return false;
}

bool DockerManager::requiresSudoPermission(const std::string& operation) {
    const std::vector<std::string> sudo_operations = {
        "install_docker",
        "uninstall_docker", 
        "install_docker_compose",
        "uninstall_docker_compose",
        "start_service",
        "stop_service", 
        "restart_service",
        "enable_service",
        "disable_service",
        "cleanup_system"
    };
    
    return std::find(sudo_operations.begin(), sudo_operations.end(), operation) != sudo_operations.end();
}

crow::response DockerManager::createUnauthorizedResponse(const std::string& operation) {
    crow::json::wvalue response;
    response["success"] = false;
    response["error"] = "Unauthorized: Invalid or missing sudo password";
    response["operation"] = operation;
    response["message"] = "This operation requires valid sudo credentials. Please provide the correct password.";
    
    broadcastLog(operation, "Operation denied: Invalid sudo credentials", "error");
    
    return crow::response(401, response);
}

DockerManager::DockerManager()
    : process_manager_(std::make_unique<ProcessManager>())
    , current_progress_{InstallationStatus::NOT_STARTED, 0, "Ready"}
    , docker_install_path_(/* std::filesystem::current_path().string() *//* std::string(".") + */ "/usr/local/bin")
    // , docker_compose_install_path_(/* std::filesystem::current_path().string() *//* std::string(".") + */ "/usr/local/bin")
    , installation_in_progress_(false)
{
    
    {
        ProcessManager _pm;
        std::string _str;
        // auto [_pid, _ret] = _pm.startProcessBlocking(
        //     "rm",
        //     {
        //         "-r",
        //         // "-f",
        //         "/tmp/metainstaller*"
        //     },
        //     {},
        //     [&_str](const std::string& _s){
        //         _str += _s;
        //     }
        // );
        auto _ret = system("rm -rf /tmp/metainstaller*");
        if(0 != _ret)
        {
            crow::logger(crow::LogLevel::Error) << "remove tmp directories failed.";
        }
    }

    
    // temp_dir_ = "/tmp/meta_installer";
    
    // std::filesystem::create_directories(docker_compose_install_path_);

    m_env_parser.load_env_file(Utils::get_env_secret_file_path());
    sudo_password_ = m_env_parser.get(CONST_KEY_SUDO_PSWD, "PASSWORD_TEST");
    return;
}

DockerManager::~DockerManager() {
    if (!temp_dir_.empty() && fs::exists(temp_dir_)) {
        fs::remove_all(temp_dir_);
    }
}

bool DockerManager::installDocker(std::function<void(const InstallationProgress&)> progressCallback) {
    // if (installation_in_progress_) {
    //     broadcastLog("docker_install", "Installation already in progress", "warning");
    //     return false;
    // }

    if(isDockerRunning())
    {
        stopDockerService();
    }
    
    installation_in_progress_ = true;
    progress_callback_ = progressCallback;
    
    broadcastLog("docker_install", "Starting Docker installation process", "info");
    
    try {
        
        updateProgress(InstallationStatus::EXTRACTING, 5, "creating docker installation directory");
        {
            // std::filesystem::create_directories(docker_install_path_);
            ProcessManager _pm;
            auto [_pid, _status] = _pm.startProcessBlockingAsRoot(
                "mkdir",
                {
                    "-p",
                    docker_install_path_,
                },
                {},
                sudo_password_,
                nullptr
            );
            if (0 != _status) {
                updateProgress(InstallationStatus::FAILED, 0, "Failed to create docker installation diretory");
                installation_in_progress_ = false;
                return false;
            }
        }


        updateProgress(InstallationStatus::EXTRACTING, 10, "Extracting Docker binary from embedded resources");
        
        temp_dir_ = Utils::create_temp_path("/tmp");
        std::string dockerBinaryPath = extractDockerBinary();
        if (dockerBinaryPath.empty()) {
            std::filesystem::remove_all(temp_dir_);
            updateProgress(InstallationStatus::FAILED, 0, "Failed to extract Docker binary", "Unable to extract embedded Docker archive");
            installation_in_progress_ = false;
            return false;
        }

        updateProgress(InstallationStatus::INSTALLING, 40, "Installing Docker binaries and components");
        
        // Install all Docker binaries
        bool _ret_install = installDockerBinaries(dockerBinaryPath);
        std::filesystem::remove_all(temp_dir_);
        if(!_ret_install) {
            updateProgress(InstallationStatus::FAILED, 0, "Failed to install Docker binaries", "Permission denied or target path not accessible");
            installation_in_progress_ = false;
            return false;
        }
        
        
        // // Install systemd service files
        // if (!installSystemdServices()) {
        //     updateProgress(InstallationStatus::FAILED, 0, "Failed to install systemd services", "Unable to install docker.service and docker.socket files");
        //     installation_in_progress_ = false;
        //     return false;
        // }

        updateProgress(InstallationStatus::CONFIGURING, 80, "Configuring Docker environment");
        
        if (!setupDockerEnvironment()) {
            updateProgress(InstallationStatus::FAILED, 0, "Failed to configure Docker environment", "Unable to set up Docker group or service");
            installation_in_progress_ = false;
            return false;
        }

        updateProgress(InstallationStatus::COMPLETED, 100, "Docker installation completed successfully");
        broadcastLog("docker_install", "Docker installation completed successfully", "success");
        installation_in_progress_ = false;
        return true;

    } catch (const std::exception& e) {
        updateProgress(InstallationStatus::FAILED, 0, "Installation failed with exception", e.what());
        broadcastLog("docker_install", "Installation failed: " + std::string(e.what()), "error");
        installation_in_progress_ = false;
        return false;
    }
}

// bool DockerManager::installDockerCompose(std::function<void(const InstallationProgress&)> progressCallback) {
//     // if (installation_in_progress_) {
//         // return false;
//     // }

//     installation_in_progress_ = true;
//     progress_callback_ = progressCallback;
    
//     try {
//         updateProgress(InstallationStatus::EXTRACTING, 10, "Extracting Docker Compose from embedded resources");
        
//         std::string dockerComposePath = extractDockerComposeBinary();
//         if (dockerComposePath.empty()) {
//             updateProgress(InstallationStatus::FAILED, 0, "Failed to extract Docker Compose", "Unable to find Docker Compose in embedded resources");
//             installation_in_progress_ = false;
//             return false;
//         }

//         updateProgress(InstallationStatus::INSTALLING, 60, "Installing Docker Compose");
        
//         std::string targetPath = docker_compose_install_path_ + "/docker-compose";
//         if (!fs::copy_file(dockerComposePath, targetPath, fs::copy_options::overwrite_existing)) {
//             updateProgress(InstallationStatus::FAILED, 0, "Failed to install Docker Compose", "Permission denied or target path not accessible");
//             installation_in_progress_ = false;
//             return false;
//         }

//         // Make executable
//         fs::permissions(targetPath, 
//             fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec | fs::perms::others_read | fs::perms::others_exec);

//         updateProgress(InstallationStatus::COMPLETED, 100, "Docker Compose installation completed successfully");
//         installation_in_progress_ = false;
//         return true;

//     } catch (const std::exception& e) {
//         updateProgress(InstallationStatus::FAILED, 0, "Docker Compose installation failed", e.what());
//         installation_in_progress_ = false;
//         return false;
//     }
// }

std::string DockerManager::extractDockerBinary() {
    try {
        // Extract the 7z archive containing Docker
        std::string sevenZipPath = Utils::get_7z_executable_path();


        std::string dockerArchivePath = ResourceExtractor::write_resource_to_path(":/resources/docker-28.3.1.7z", temp_dir_);
        
        // Extract Docker archive
        // std::string extractDir = Utils::path_join_multiple({temp_dir_, "docker_extracted"});
        // fs::create_directories(extractDir);

        auto [pid, ret_code] = process_manager_->startProcessBlocking(
            sevenZipPath,
            {"-o"+temp_dir_, "x", dockerArchivePath},
            {},
            nullptr
        );

        if(ret_code != 0) {
            return "";
        }

        // Find the docker binary in extracted files
        for (const auto& entry : fs::recursive_directory_iterator(temp_dir_)) {
            if (entry.is_directory() && entry.path().filename() == "docker") {
                return entry.path().string();
            }
        }

        return "";
    } catch (const std::exception& e) {
        crow::logger(crow::LogLevel::Critical) << "extract docker binaries failed... " << e.what();
        return "";
    }
}

std::string DockerManager::extractDockerComposeBinary() {
    try {
        // Similar to Docker extraction, but look for docker-compose binary
        std::string extractDir = temp_dir_ + "/docker_extracted";
        
        for (const auto& entry : fs::recursive_directory_iterator(extractDir)) {
            if (entry.is_regular_file() && 
                (entry.path().filename() == "docker-compose" || 
                 entry.path().filename().string().find("docker-compose") != std::string::npos)) {
                return entry.path().string();
            }
        }

        return "";
    } catch (const std::exception& e) {
        return "";
    }
}

bool DockerManager::installDockerBinaries(const std::string& _path) {
    try {
        // Get the extracted Docker directory
        std::string extractDir = _path;
        
        // List of Docker binaries to install
        std::vector<std::string> binaries = {
            "docker", "dockerd", "containerd", "containerd-shim-runc-v2", 
            "ctr", "docker-init", "docker-proxy", "runc", "docker-compose"
        };
        
        // Install each binary
        for (const std::string& binary : binaries) {
            std::string sourcePath = Utils::path_join_multiple({extractDir, binary});
            std::string targetPath = Utils::path_join_multiple({docker_install_path_, binary});
            
            // Check if source binary exists
            if (fs::exists(sourcePath)) {
                
                // Make executable
                fs::permissions(
                    sourcePath, 
                    fs::perms::owner_all |
                    fs::perms::group_read | fs::perms::group_exec |
                    fs::perms::others_read | fs::perms::others_exec
                );
                
                // Copy binary to system location
                auto [pid, ret_code] = process_manager_->startProcessBlockingAsRoot(
                    "cp",
                    {"-af", sourcePath, targetPath},
                    {},
                    sudo_password_,
                    nullptr
                );

                if (ret_code != 0) return false;
                
                // Make executable
                // auto [pid2, ret_code2] = process_manager_->startProcessBlockingAsRoot(
                //     "chmod",
                //     {"+x", targetPath},
                //     {},
                //     sudo_password_,
                //     nullptr
                // );
                // if (ret_code2 != 0) return false;
            }
        }

        updateProgress(InstallationStatus::INSTALLING, 50, "Installing docker compose plugin");
        
        // create cli-plugins directory for docker-compose
        std::string _path_docker_compose = "/usr/lib/docker/cli-plugins";
        auto [pid, ret_code] = process_manager_->startProcessBlockingAsRoot(
            "mkdir",
            {
                "-p",
                _path_docker_compose
            },
            {},
            sudo_password_,
            nullptr
        );
        
        
        // copy docker-compose to cli-plugins directory
        std::tie(pid, ret_code) = process_manager_->startProcessBlockingAsRoot(
            "cp",
            {
                "-af",
                Utils::path_join_multiple({extractDir, "docker-compose"}),
                _path_docker_compose
            },
            {},
            sudo_password_,
            nullptr
        );
        
        updateProgress(InstallationStatus::INSTALLING, 60, "Installing docker service files");
        // copy docker service and socket to systemd directory
        std::tie(pid, ret_code) = process_manager_->startProcessBlockingAsRoot(
            "cp",
            {
                "-af",
                Utils::path_join_multiple({extractDir, "docker.service"}),
                Utils::path_join_multiple({extractDir, "docker.socket"}),
                "/etc/systemd/system/"
            },
            {},
            sudo_password_,
            nullptr
        );
        
        if (ret_code != 0) return false;
        
        updateProgress(InstallationStatus::INSTALLING, 70, "reloading systemd...");
        // Reload systemd daemon
        std::tie(pid, ret_code) = process_manager_->startProcessBlockingAsRoot(
            "/usr/bin/systemctl",
            {"daemon-reload"},
            {},
            sudo_password_,
            nullptr
        );

        if (ret_code != 0) return false;
        
        return true;

    } catch (const std::exception& e) {
        crow::logger(crow::LogLevel::Critical) << "docker binary installation fail::" << e.what() << "\n";
        return false;
    }
}

// bool DockerManager::installSystemdServices() {
//     try {
//         // Docker service file content
//         std::string dockerServiceContent = R"([Unit]
// Description=Docker Application Container Engine
// Documentation=https://docs.docker.com
// After=network-online.target firewalld.service containerd.service time-set.target
// Wants=network-online.target containerd.service
// Requires=docker.socket

// [Service]
// Type=notify
// ExecStart=)" + docker_install_path_ + R"(/dockerd -H fd:// --containerd=/run/containerd/containerd.sock
// ExecReload=/bin/kill -s HUP $MAINPID
// TimeoutStartSec=0
// RestartSec=2
// Restart=always
// StartLimitBurst=3
// StartLimitInterval=60s
// LimitNOFILE=infinity
// LimitNPROC=infinity
// LimitCORE=infinity
// TasksMax=infinity
// Delegate=yes
// KillMode=process
// OOMScoreAdjust=-500

// [Install]
// WantedBy=multi-user.target
// )";

//         // Docker socket file content  
//         std::string dockerSocketContent = R"([Unit]
// Description=Docker Socket for the API
// PartOf=docker.service

// [Socket]
// ListenStream=/var/run/docker.sock
// SocketMode=0660
// SocketUser=root
// SocketGroup=docker

// [Install]
// WantedBy=sockets.target
// )";

//         // Write service files
//         auto [pid1, ret_code1] = process_manager_->startProcessBlockingAsRoot(
//             "tee",
//             {"/etc/systemd/system/docker.service"},
//             {},
//             sudo_password_,
//             [&](const std::string& line) {
//                 process_manager_->writeToProcess(dockerServiceContent);
//             }
//         );
//         if (ret_code1 != 0) return false;

//         auto [pid2, ret_code2] = process_manager_->startProcessBlockingAsRoot(
//             "tee",
//             {"/etc/systemd/system/docker.socket"},
//             {},
//             sudo_password_,
//             [&](const std::string& line) {
//                 process_manager_->writeToProcess(dockerSocketContent);
//             }
//         );
//         if (ret_code2 != 0) return false;

//         // Reload systemd daemon
//         auto [pid, ret_code] = process_manager_->startProcessBlockingAsRoot(
//             "/usr/bin/systemctl",
//             {"daemon-reload"},
//             {},
//             sudo_password_,
//             nullptr
//         );

//         return ret_code == 0;
//     } catch (const std::exception& e) {
//         return false;
//     }
// }

bool DockerManager::setupDockerEnvironment() {
    try {

        updateProgress(InstallationStatus::CONFIGURING, 80, "creating docker group");
        // Create docker group
        if (!createDockerGroup()) {
            return false;
        }
        
        updateProgress(InstallationStatus::CONFIGURING, 85, "adding user to docker group");
        // Add current user to docker group
        if (!addUserToDockerGroup()) {
            return false;
        }
        
        updateProgress(InstallationStatus::CONFIGURING, 85, "adding docker to system path");
        // Add to system PATH if needed
        if (!addToSystemPath(docker_install_path_)) {
            return false;
        }
        
        updateProgress(InstallationStatus::CONFIGURING, 90, "enabling docker service");
        // Enable and start docker socket
        std::string _str_systemctl;
        auto [pid, ret_code] = process_manager_->startProcessBlockingAsRoot(
            "/usr/bin/systemctl",
            {
                "enable",
                "--now",
                "docker.service"
                // "docker.socket"
            },
            {},
            sudo_password_,
            [&_str_systemctl](const std::string& _str){
                _str_systemctl += _str;
            }
        );
        if(0 != ret_code)
        {
            crow::logger(crow::LogLevel::Error) << "systemctl out:: '" << _str_systemctl << "'\n";
        }

        std::tie(pid, ret_code) = process_manager_->startProcessBlockingAsRoot(
            "/usr/bin/systemctl",
            {
                "enable",
                "--now",
                "docker.socket"
                // "docker.socket"
            },
            {},
            sudo_password_,
            [&_str_systemctl](const std::string& _str){
                _str_systemctl += _str;
            }
        );
        if(0 != ret_code)
        {
            crow::logger(crow::LogLevel::Error) << "systemctl out:: '" << _str_systemctl << "'\n";
        }
        
        return (ret_code == 0);
    } catch (const std::exception& e) {
        return false;
    }
}

bool DockerManager::createDockerGroup() {
    try {
        // Check if docker group already exists using getent command
        auto [pid, ret_code] = process_manager_->startProcessBlocking(
            "/usr/bin/getent",
            {"group", "docker"},
            {},
            nullptr
        );
        
        if (ret_code == 0) {
            return true; // Group already exists
        }

        // Create docker group
        auto [pid2, ret_code2] = process_manager_->startProcessBlockingAsRoot(
            "/usr/sbin/groupadd",
            {"docker"},
            {},
            sudo_password_,
            nullptr
        );

        return ret_code2 == 0;
    } catch (const std::exception& e) {
        return false;
    }
}

bool DockerManager::addUserToDockerGroup(const std::string& username) {
    try {
        std::string user = username;
        if (user.empty()) {
            // Get current username using whoami command
            std::string output;
            auto [pid, ret_code] = process_manager_->startProcessBlocking(
                "/usr/bin/whoami",
                {},
                {},
                [&output](const std::string& line) {
                    output += line;
                }
            );
            
            if (ret_code != 0 || output.empty()) {
                return false;
            }
            
            // Remove newline
            if (!output.empty() && output.back() == '\n') {
                output.pop_back();
            }
            user = output;
        }

        auto [pid2, ret_code2] = process_manager_->startProcessBlockingAsRoot(
            "/usr/sbin/usermod",
            {"-aG", "docker", user},
            {},
            sudo_password_,
            nullptr
        );

        return ret_code2 == 0;
    } catch (const std::exception& e) {
        return false;
    }
}
bool DockerManager::addToSystemPath(const std::string& path) {
    // This is a simplified version - in practice, you might want to modify
    // system-wide PATH or user's shell profile
    return true;
}

DockerInfo DockerManager::getDockerInfo() {
    DockerInfo info;
    // broadcastLog(__FUNCTION__, "getting docker info", "info");
    try {
        if (!isCommandAvailable("docker")) {
            info.installed = false;
            info.error_message = "Docker binary not found in PATH";
            return info;
        }

        std::string output = executeCommandWithOutput("docker", {"version", "--format", "json"});
        if(output.empty()) {
            info.installed = false;
            info.error_message = "Failed to get Docker version information";
            return info;
        }

        // Parse JSON output using json11
        std::string parse_error;
        json11::Json json = json11::Json::parse(output, parse_error);
        
        if (!parse_error.empty()) {
            info.installed = false;
            info.error_message = "Failed to parse Docker version JSON: " + parse_error;
            return info;
        }

        info.installed = true;
        
        // Extract client and server version information
        auto client = json["Client"];
        auto server = json["Server"];
        
        if (!client.is_null()) {
            info.version = client["Version"].string_value();
            info.api_version = client["ApiVersion"].string_value();
            info.go_version = client["GoVersion"].string_value();
            info.git_commit = client["GitCommit"].string_value();
            info.build_time = client["BuildTime"].string_value();
            info.os = client["Os"].string_value();
            info.arch = client["Arch"].string_value();
            info.experimental = client["Experimental"].bool_value();
        }
        
        if (!server.is_null()) {
            // Use server info if available, otherwise keep client info
            if (info.version.empty()) info.version = server["Version"].string_value();
            if (info.api_version.empty()) info.api_version = server["ApiVersion"].string_value();
            info.min_api_version = server["MinAPIVersion"].string_value();
            info.kernel_version = server["KernelVersion"].string_value();
        }


    } catch (const std::exception& e) {
        info.installed = false;
        info.error_message = e.what();
    }

    return info;
}

DockerComposeInfo DockerManager::getDockerComposeInfo() {
    DockerComposeInfo info;
    
    try {
        // if (!isCommandAvailable("docker-compose")) {
        //     info.installed = false;
        //     info.error_message = "Docker Compose binary not found in PATH";
        //     return info;
        // }

        std::string output = executeCommandWithOutput("docker", {"compose", "version", "--short"});
        if (!output.empty()) {
            info.installed = true;
            info.version = output;
            // Remove newline
            if (!info.version.empty() && info.version.back() == '\n') {
                info.version.pop_back();
            }
        } else {
            info.installed = false;
            info.error_message = "Failed to get Docker Compose version";
        }

    } catch (const std::exception& e) {
        info.installed = false;
        info.error_message = e.what();
    }

    return info;
}

bool DockerManager::isDockerInstalled() {
    return isCommandAvailable("docker");
}

bool DockerManager::isDockerComposeInstalled() {
    return isCommandAvailable("docker-compose");
}

bool DockerManager::isDockerRunning() {
    try {
        std::string output = executeCommandWithOutput("docker", {"info"});
        return !output.empty();
    } catch (const std::exception& e) {
        return false;
    }
}

bool DockerManager::startDockerService() {
    try {
        auto [pid, ret_code] = process_manager_->startProcessBlockingAsRoot(
            "/usr/bin/systemctl",
            {
                "start",
                "docker.socket",
                "docker.service"
            },
            {},
            sudo_password_,
            nullptr
        );
        return ret_code == 0;
    } catch (const std::exception& e) {
        return false;
    }
}

bool DockerManager::stopDockerService() {
    try {
        auto [pid, ret_code] = process_manager_->startProcessBlockingAsRoot(
            "/usr/bin/systemctl",
            {
                "stop",
                "docker.socket",
                "docker.service"
            },
            {},
            sudo_password_,
            nullptr
        );
        return ret_code == 0;
    } catch (const std::exception& e) {
        return false;
    }
}

bool DockerManager::restartDockerService() {
    try {
        auto [pid, ret_code] = process_manager_->startProcessBlockingAsRoot(
            "/usr/bin/systemctl",
            {"restart", "docker"},
            {},
            sudo_password_,
            nullptr
        );
        return ret_code == 0;
    } catch (const std::exception& e) {
        return false;
    }
}

std::string DockerManager::getDockerServiceStatus() {
    try {
        std::string output;
        auto [pid, ret_code] = process_manager_->startProcessBlocking(
            "/usr/bin/systemctl",
            {"status", "docker", "--no-pager"},
            {},
            // sudo_password_,
            [&](const std::string& line) {
                output += line;
            }
        );
        return output;
    } catch (const std::exception& e) {
        return std::string("Error: ") + e.what();
    }
}

std::vector<std::string> DockerManager::listContainers(bool all) {
    std::vector<std::string> containers;
    try {
        std::vector<std::string> args = {"ps", "--format", "json"};
        if (all) {
            args.insert(args.begin() + 1, "-a");
        }
        
        std::string output = executeCommandWithOutput("docker", args);
        std::istringstream iss(output);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty() && line.find_first_of("{") != std::string::npos) {
                containers.push_back(line);
            }
        }
    } catch (const std::exception& e) {
        // Return empty vector on error
    }
    return containers;
}

std::vector<std::string> DockerManager::listImages() {
    std::vector<std::string> images;
    try {
        std::string output = executeCommandWithOutput("docker", {"images", "--format", "json"});
        std::istringstream iss(output);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty() && line.find_first_of("{") != std::string::npos) {
                images.push_back(line);
            }
        }
    } catch (const std::exception& e) {
        // Return empty vector on error
    }
    return images;
}

bool DockerManager::pullImage(const std::string& imageName, std::function<void(const std::string&)> progressCallback) {
    try {
        auto [pid, ret_code] = process_manager_->startProcessBlocking(
            "docker",
            {"pull", imageName},
            {},
            progressCallback
        );
        return ret_code == 0;
    } catch (const std::exception& e) {
        return false;
    }
}

std::string DockerManager::getSystemInfo() {
    try {
        return executeCommandWithOutput("docker", {"system", "info", "--format", "json"});
    } catch (const std::exception& e) {
        return std::string("Error: ") + e.what();
    }
}

std::string DockerManager::getDiskUsage() {
    try {
        std::string output = executeCommandWithOutput("docker", {"system", "df", "--format", "json"});
        
        // Parse each line as a separate JSON object and create an array
        json11::Json::array result_array;
        std::istringstream iss(output);
        std::string line;
        
        while (std::getline(iss, line)) {
            if (!line.empty()) {
                std::string parse_error;
                json11::Json json_obj = json11::Json::parse(line, parse_error);
                
                if (parse_error.empty()) {
                    result_array.push_back(json_obj);
                }
            }
        }
        
        // Convert the array to a JSON string
        json11::Json result(result_array);
        return result.dump();
    } catch (const std::exception& e) {
        return std::string("Error: ") + e.what();
    }
}

bool DockerManager::cleanupSystem() {
    try {
        auto [pid, ret_code] = process_manager_->startProcessBlocking(
            "docker",
            {"system", "prune", "-f"},
            {},
            nullptr
        );
        return ret_code == 0;
    } catch (const std::exception& e) {
        return false;
    }
}

bool DockerManager::isCommandAvailable(const std::string& command) {
    try {
        auto [pid, ret_code] = process_manager_->startProcessBlocking(
            "/usr/bin/which",
            {command},
            {},
            nullptr
        );
        return ret_code == 0;
    } catch (const std::exception& e) {
        return false;
    }
}

std::string DockerManager::executeCommandWithOutput(const std::string& command, const std::vector<std::string>& args) {
    std::string output;
    try {
        auto [pid, ret_code] = process_manager_->startProcessBlocking(
            command,
            args,
            {},
            [&output](const std::string& line) {
                output += line;
            }
        );
        
        if (ret_code == 0) {
            return output;
        }
    } catch (const std::exception& e) {
        // Return empty string on error
    }
    return "";
}

void DockerManager::updateProgress(InstallationStatus status, int percentage, const std::string& message, const std::string& error) {
    current_progress_.status = status;
    current_progress_.percentage = percentage;
    current_progress_.message = message;
    current_progress_.error_details = error;
    
    // Broadcast progress update via WebSocket
    broadcastProgress(current_progress_);
    
    if (progress_callback_) {
        progress_callback_(current_progress_);
    }
}

std::string DockerManager::get_installation_status(const InstallationStatus& _status)
{
    static const  std::unordered_map<InstallationStatus, std::string> _obj_map{
        {InstallationStatus::NOT_STARTED, "NOT_STARTED"},
        {InstallationStatus::IN_PROGRESS, "IN_PROGRESS"},
        {InstallationStatus::COMPLETED, "COMPLETED"},
        {InstallationStatus::FAILED, "FAILED"},
        {InstallationStatus::EXTRACTING, "EXTRACTING"},
        {InstallationStatus::INSTALLING, "INSTALLING"},
        {InstallationStatus::CONFIGURING, "CONFIGURING"}
    };

    if(_obj_map.end() == _obj_map.find(_status))
    {
        return "NOT_VALID_STATUS";
    }

    return _obj_map.at(_status);
}

bool DockerManager::uninstallDocker() {
    try {

        process_manager_->startProcessBlockingAsRoot(
            "/usr/bin/systemctl",
            {"disable", " --now", "docker.socket"},
            {},
            sudo_password_,
            nullptr
        );

        process_manager_->startProcessBlockingAsRoot(
            "/usr/bin/systemctl",
            {"stop", "docker.socket"},
            {},
            sudo_password_,
            nullptr
        );

        // Stop and disable Docker services
        process_manager_->startProcessBlockingAsRoot(
            "/usr/bin/systemctl",
            {"disable", " --now", "docker.service"},
            {},
            sudo_password_,
            nullptr
        );

        process_manager_->startProcessBlockingAsRoot(
            "/usr/bin/systemctl",
            {"stop", "docker.service"},
            {},
            sudo_password_,
            nullptr
        );
        
        
        
        // Remove systemd service files
        process_manager_->startProcessBlockingAsRoot(
            "rm",
            {
                "-f", 
                "/etc/systemd/system/docker.service",
                "/etc/systemd/system/docker.socket"
            },
            {},
            sudo_password_,
            nullptr
        );
        // process_manager_->startProcessBlockingAsRoot(
        //     "rm",
        //     {"/etc/systemd/system/docker.socket"},
        //     {},
        //     sudo_password_,
        //     nullptr
        // );
        
        // Reload systemd daemon
        process_manager_->startProcessBlockingAsRoot(
            "/usr/bin/systemctl",
            {"daemon-reload"},
            {},
            sudo_password_,
            nullptr
        );

        process_manager_->startProcessBlockingAsRoot(
            "groupdel",
            {"docker"},
            {},
            sudo_password_,
            nullptr
        );
        
        // Remove Docker binaries
        std::vector<std::string> binaries = {
            "docker", "dockerd", "containerd", "containerd-shim-runc-v2", 
            "ctr", "docker-init", "docker-proxy", "runc", "docker-compose"
        };
        
        for (const std::string& binary : binaries) {
            std::string binaryPath = Utils::path_join_multiple({docker_install_path_, binary}); ;
            if (fs::exists(binaryPath)) {
                process_manager_->startProcessBlockingAsRoot(
                    "rm",
                    {
                        "-f",
                        binaryPath
                    },
                    {},
                    sudo_password_,
                    nullptr
                );
            }
        }
        std::string _path_docker_compose = "/usr/lib/docker/cli-plugins/docker-compose";
        if(fs::exists(_path_docker_compose))
        {
            process_manager_->startProcessBlockingAsRoot(
                "rm",
                {
                    "-f",
                    _path_docker_compose
                },
                {},
                sudo_password_,
                nullptr
            );
        }
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

// bool DockerManager::uninstallDockerCompose() {
//     try {
//         std::string dockerComposePath = docker_compose_install_path_ + "/docker-compose";
//         if (fs::exists(dockerComposePath)) {
//             process_manager_->startProcessBlockingAsRoot(
//                 "rm",
//                 {dockerComposePath},
//                 {},
//                 sudo_password_,
//                 nullptr
//             );
//         }
//         return true;
//     } catch (const std::exception& e) {
//         return false;
//     }
// }

bool DockerManager::removeContainer(const std::string& containerId, bool force) {
    try {
        std::vector<std::string> args = {"rm"};
        if (force) {
            args.push_back("-f");
        }
        args.push_back(containerId);
        
        auto [pid, ret_code] = process_manager_->startProcessBlocking("docker", args, {}, nullptr);
        return ret_code == 0;
    } catch (const std::exception& e) {
        return false;
    }
}

bool DockerManager::removeImage(const std::string& imageId, bool force) {
    try {
        std::vector<std::string> args = {"rmi"};
        if (force) {
            args.push_back("-f");
        }
        args.push_back(imageId);
        
        auto [pid, ret_code] = process_manager_->startProcessBlocking("docker", args, {}, nullptr);
        return ret_code == 0;
    } catch (const std::exception& e) {
        return false;
    }
}

// Advanced image operations
bool DockerManager::loadImageFromFile(const std::string& filePath) {
    try {
        if (!fs::exists(filePath)) {
            return false;
        }
        
        auto [pid, ret_code] = process_manager_->startProcessBlocking(
            "docker", 
            {"load", "-i", filePath}, 
            {}, 
            nullptr
        );
        return ret_code == 0;
    } catch (const std::exception& e) {
        return false;
    }
}

bool DockerManager::saveImageToFile(const std::string& imageName, const std::string& filePath) {
    try {
        auto [pid, ret_code] = process_manager_->startProcessBlocking(
            "docker", 
            {"save", "-o", filePath, imageName}, 
            {}, 
            nullptr
        );
        return ret_code == 0;
    } catch (const std::exception& e) {
        return false;
    }
}

bool DockerManager::tagImage(const std::string& sourceImage, const std::string& targetTag) {
    try {
        auto [pid, ret_code] = process_manager_->startProcessBlocking(
            "docker", 
            {"tag", sourceImage, targetTag}, 
            {}, 
            nullptr
        );
        return ret_code == 0;
    } catch (const std::exception& e) {
        return false;
    }
}

bool DockerManager::buildImage(const std::string& dockerfilePath, const std::string& imageName, const std::string& buildContext) {
    try {
        if (!fs::exists(dockerfilePath)) {
            return false;
        }
        
        auto [pid, ret_code] = process_manager_->startProcessBlocking(
            "docker", 
            {"build", "-f", dockerfilePath, "-t", imageName, buildContext}, 
            {}, 
            nullptr
        );
        return ret_code == 0;
    } catch (const std::exception& e) {
        return false;
    }
}

// Container lifecycle management
bool DockerManager::startContainer(const std::string& containerId) {
    broadcastLog("container_start", "Starting container: " + containerId, "info");
    try {
        auto [pid, ret_code] = process_manager_->startProcessBlocking(
            "docker", 
            {"start", containerId}, 
            {}, 
            nullptr
        );
        bool success = ret_code == 0;
        if (success) {
            broadcastLog("container_start", "Container started successfully: " + containerId, "success");
        } else {
            broadcastLog("container_start", "Failed to start container: " + containerId, "error");
        }
        return success;
    } catch (const std::exception& e) {
        broadcastLog("container_start", "Exception starting container " + containerId + ": " + e.what(), "error");
        return false;
    }
}

bool DockerManager::stopContainer(const std::string& containerId, int timeout) {
    try {
        std::vector<std::string> args = {"stop"};
        if (timeout > 0) {
            args.push_back("-t");
            args.push_back(std::to_string(timeout));
        }
        args.push_back(containerId);
        
        auto [pid, ret_code] = process_manager_->startProcessBlocking("docker", args, {}, nullptr);
        return ret_code == 0;
    } catch (const std::exception& e) {
        return false;
    }
}

bool DockerManager::restartContainer(const std::string& containerId, int timeout) {
    try {
        std::vector<std::string> args = {"restart"};
        if (timeout > 0) {
            args.push_back("-t");
            args.push_back(std::to_string(timeout));
        }
        args.push_back(containerId);
        
        auto [pid, ret_code] = process_manager_->startProcessBlocking("docker", args, {}, nullptr);
        return ret_code == 0;
    } catch (const std::exception& e) {
        return false;
    }
}

bool DockerManager::pauseContainer(const std::string& containerId) {
    try {
        auto [pid, ret_code] = process_manager_->startProcessBlocking(
            "docker", 
            {"pause", containerId}, 
            {}, 
            nullptr
        );
        return ret_code == 0;
    } catch (const std::exception& e) {
        return false;
    }
}

bool DockerManager::unpauseContainer(const std::string& containerId) {
    try {
        auto [pid, ret_code] = process_manager_->startProcessBlocking(
            "docker", 
            {"unpause", containerId}, 
            {}, 
            nullptr
        );
        return ret_code == 0;
    } catch (const std::exception& e) {
        return false;
    }
}

std::string DockerManager::getContainerLogs(const std::string& containerId, int lines) {
    try {
        std::string output;
        std::vector<std::string> args = {"logs"};
        if (lines > 0) {
            args.push_back("--tail");
            args.push_back(std::to_string(lines));
        }
        args.push_back(containerId);
        
        auto [pid, ret_code] = process_manager_->startProcessBlocking(
            "docker", 
            args, 
            {}, 
            [&output](const std::string& line) {
                output += line;
            }
        );
        
        return ret_code == 0 ? output : "";
    } catch (const std::exception& e) {
        return "";
    }
}

std::vector<ContainerInfo> DockerManager::getDetailedContainers(bool all) {
    std::vector<ContainerInfo> containers;
    try {
        std::vector<std::string> args = {"ps", "--format", "json"};
        if (all) {
            args.insert(args.begin() + 1, "-a");
        }
        
        std::string output = executeCommandWithOutput("docker", args);
        if (output.empty()) {
            return containers;
        }
        
        std::istringstream iss(output);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.empty()) continue;
            
            std::string parse_error;
            json11::Json json = json11::Json::parse(line, parse_error);
            
            if (!parse_error.empty()) continue;
            
            ContainerInfo info;
            info.id = json["ID"].string_value();
            info.name = json["Names"].string_value();
            info.image = json["Image"].string_value();
            info.status = json["Status"].string_value();
            info.created = json["CreatedAt"].string_value();
            
            // Parse ports if available
            std::string ports_str = json["Ports"].string_value();
            if (!ports_str.empty()) {
                std::istringstream ports_stream(ports_str);
                std::string port;
                while (std::getline(ports_stream, port, ',')) {
                    info.ports.push_back(port);
                }
            }
            
            containers.push_back(info);
        }
    } catch (const std::exception& e) {
        // Return empty vector on error
    }
    return containers;
}

// Docker Compose operations
bool DockerManager::loadComposeFile(const std::string& composeFilePath, const std::string& projectName, const std::string& workingDir) {
    try {
        if (!fs::exists(composeFilePath)) {
            return false;
        }
        
        DockerComposeProject project;
        project.name = projectName;
        project.compose_file_path = composeFilePath;
        project.working_directory = workingDir.empty() ? fs::path(composeFilePath).parent_path().string() : workingDir;
        
        // Parse compose file to extract services (simplified)
        std::ifstream file(composeFilePath);
        if (file.is_open()) {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            // This is a simplified service extraction - in a real implementation,
            // you'd want to parse the YAML properly
            std::istringstream iss(content);
            std::string line;
            bool in_services_section = false;
            while (std::getline(iss, line)) {
                if (line.find("services:") != std::string::npos) {
                    in_services_section = true;
                    continue;
                }
                if (in_services_section && line.find("  ") == 0 && line.find("    ") != 0) {
                    // This is a service name (simple heuristic)
                    std::string service_name = line.substr(2);
                    if (service_name.back() == ':') {
                        service_name.pop_back();
                        project.services.push_back(service_name);
                    }
                }
                if (in_services_section && line.find("volumes:") != std::string::npos) {
                    break; // End of services section
                }
            }
        }
        
        compose_projects_[projectName] = project;
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool DockerManager::removeComposeProject(const std::string& projectName) {
    try {
        auto it = compose_projects_.find(projectName);
        if (it == compose_projects_.end()) {
            return false;
        }
        
        // Stop and remove containers first
        composeDown(projectName, true);
        compose_projects_.erase(it);
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

std::vector<DockerComposeProject> DockerManager::listComposeProjects() {
    std::vector<DockerComposeProject> projects;
    for (const auto& pair : compose_projects_) {
        projects.push_back(pair.second);
    }
    return projects;
}

bool DockerManager::composeUp(const std::string& projectName, bool detached) {
    try {
        auto it = compose_projects_.find(projectName);
        if (it == compose_projects_.end()) {
            return false;
        }
        
        const auto& project = it->second;
        std::vector<std::string> args = {"compose", "-f", project.compose_file_path, "-p", projectName, "up"};
        if (detached) {
            args.push_back("-d");
        }
        
        auto [pid, ret_code] = process_manager_->startProcessBlocking(
            "docker", 
            args, 
            {{"COMPOSE_PROJECT_NAME", projectName}}, 
            nullptr
        );
        return ret_code == 0;
    } catch (const std::exception& e) {
        return false;
    }
}

bool DockerManager::composeDown(const std::string& projectName, bool removeVolumes) {
    try {
        auto it = compose_projects_.find(projectName);
        if (it == compose_projects_.end()) {
            return false;
        }
        
        const auto& project = it->second;
        std::vector<std::string> args = {"compose", "-f", project.compose_file_path, "-p", projectName, "down"};
        if (removeVolumes) {
            args.push_back("-v");
        }
        
        auto [pid, ret_code] = process_manager_->startProcessBlocking(
            "docker", 
            args, 
            {{"COMPOSE_PROJECT_NAME", projectName}}, 
            nullptr
        );
        return ret_code == 0;
    } catch (const std::exception& e) {
        return false;
    }
}

bool DockerManager::composeRestart(const std::string& projectName) {
    try {
        auto it = compose_projects_.find(projectName);
        if (it == compose_projects_.end()) {
            return false;
        }
        
        const auto& project = it->second;
        auto [pid, ret_code] = process_manager_->startProcessBlocking(
            "docker", 
            {"compose", "-f", project.compose_file_path, "-p", projectName, "restart"}, 
            {{"COMPOSE_PROJECT_NAME", projectName}}, 
            nullptr
        );
        return ret_code == 0;
    } catch (const std::exception& e) {
        return false;
    }
}

std::vector<std::string> DockerManager::composeServices(const std::string& projectName) {
    auto it = compose_projects_.find(projectName);
    if (it != compose_projects_.end()) {
        return it->second.services;
    }
    return {};
}

bool DockerManager::composeServiceAction(const std::string& projectName, const std::string& serviceName, const std::string& action) {
    try {
        auto it = compose_projects_.find(projectName);
        if (it == compose_projects_.end()) {
            return false;
        }
        
        const auto& project = it->second;
        std::vector<std::string> args = {"compose", "-f", project.compose_file_path, "-p", projectName, action, serviceName};
        
        auto [pid, ret_code] = process_manager_->startProcessBlocking(
            "docker", 
            args, 
            {{"COMPOSE_PROJECT_NAME", projectName}}, 
            nullptr
        );
        return ret_code == 0;
    } catch (const std::exception& e) {
        return false;
    }
}

// REST API endpoint implementations
void DockerManager::registerRestEndpoints(crow::SimpleApp& app) {
    // Docker information endpoints
    CROW_ROUTE(app, "/api/docker/info").methods("GET"_method)
    ([this](const crow::request& req) {
        return handleGetDockerInfo();
    });

    CROW_ROUTE(app, "/api/docker-compose/info").methods("GET"_method)
    ([this](const crow::request& req) {
        return handleGetDockerComposeInfo();
    });

    // Installation endpoints
    CROW_ROUTE(app, "/api/docker/install").methods("POST"_method)
    ([this](const crow::request& req) {
        // return handleInstallDocker();
        return handleInstallDocker();
    });

    CROW_ROUTE(app, "/api/docker/uninstall").methods("DELETE"_method)
    ([this](const crow::request& req) {
        return handleUninstallDocker();
    });

    // CROW_ROUTE(app, "/api/docker-compose/install").methods("POST"_method)
    // ([this](const crow::request& req) {
    //     return handleInstallDockerCompose();
    // });

    // CROW_ROUTE(app, "/api/docker-compose/uninstall").methods("DELETE"_method)
    // ([this](const crow::request& req) {
    //     return handleUninstallDockerCompose();
    // });

    // Service management endpoints
    CROW_ROUTE(app, "/api/docker/service/start").methods("POST"_method)
    ([this](const crow::request& req) {
        return handleStartDockerService();
    });

    CROW_ROUTE(app, "/api/docker/service/stop").methods("POST"_method)
    ([this](const crow::request& req) {
        return handleStopDockerService();
    });

    CROW_ROUTE(app, "/api/docker/service/restart").methods("POST"_method)
    ([this](const crow::request& req) {
        return handleRestartDockerService();
    });

    CROW_ROUTE(app, "/api/docker/service/status").methods("GET"_method)
    ([this](const crow::request& req) {
        return handleGetServiceStatus();
    });

    // Container and image management
    CROW_ROUTE(app, "/api/docker/containers").methods("GET"_method)
    ([this](const crow::request& req) {
        return handleListContainers(req);
    });

    CROW_ROUTE(app, "/api/docker/images").methods("GET"_method)
    ([this](const crow::request& req) {
        return handleListImages();
    });

    CROW_ROUTE(app, "/api/docker/images/pull").methods("POST"_method)
    ([this](const crow::request& req) {
        return handlePullImage(req);
    });

    CROW_ROUTE(app, "/api/docker/containers/<string>").methods("DELETE"_method)
    ([this](const crow::request& req, const std::string& containerId) {
        return handleRemoveContainer(containerId, req);
    });

    CROW_ROUTE(app, "/api/docker/images/<string>").methods("DELETE"_method)
    ([this](const crow::request& req, const std::string& imageId) {
        return handleRemoveImage(imageId, req);
    });

    // System information endpoints
    CROW_ROUTE(app, "/api/docker/system/info").methods("GET"_method)
    ([this](const crow::request& req) {
        return handleGetSystemInfo();
    });

    CROW_ROUTE(app, "/api/docker/system/df").methods("GET"_method)
    ([this](const crow::request& req) {
        return handleGetDiskUsage();
    });

    CROW_ROUTE(app, "/api/docker/system/prune").methods("POST"_method)
    ([this](const crow::request& req) {
        return handleCleanupSystem();
    });

    // Installation progress endpoint
    CROW_ROUTE(app, "/api/docker/installation/progress").methods("GET"_method)
    ([this](const crow::request& req) {
        return handleGetInstallationProgress();
    });

    // Advanced image management endpoints
    CROW_ROUTE(app, "/api/docker/images/load").methods("POST"_method)
    ([this](const crow::request& req) {
        return handleLoadImageFromFile(req);
    });

    CROW_ROUTE(app, "/api/docker/images/save").methods("POST"_method)
    ([this](const crow::request& req) {
        return handleSaveImageToFile(req);
    });

    CROW_ROUTE(app, "/api/docker/images/tag").methods("POST"_method)
    ([this](const crow::request& req) {
        return handleTagImage(req);
    });

    CROW_ROUTE(app, "/api/docker/images/build").methods("POST"_method)
    ([this](const crow::request& req) {
        return handleBuildImage(req);
    });

    // Container lifecycle endpoints
    CROW_ROUTE(app, "/api/docker/containers/<string>/start").methods("POST"_method)
    ([this](const crow::request& req, const std::string& containerId) {
        return handleStartContainer(containerId);
    });

    CROW_ROUTE(app, "/api/docker/containers/<string>/stop").methods("POST"_method)
    ([this](const crow::request& req, const std::string& containerId) {
        return handleStopContainer(containerId, req);
    });

    CROW_ROUTE(app, "/api/docker/containers/<string>/restart").methods("POST"_method)
    ([this](const crow::request& req, const std::string& containerId) {
        return handleRestartContainer(containerId, req);
    });

    CROW_ROUTE(app, "/api/docker/containers/<string>/pause").methods("POST"_method)
    ([this](const crow::request& req, const std::string& containerId) {
        return handlePauseContainer(containerId);
    });

    CROW_ROUTE(app, "/api/docker/containers/<string>/unpause").methods("POST"_method)
    ([this](const crow::request& req, const std::string& containerId) {
        return handleUnpauseContainer(containerId);
    });

    CROW_ROUTE(app, "/api/docker/containers/<string>/logs").methods("GET"_method)
    ([this](const crow::request& req, const std::string& containerId) {
        return handleGetContainerLogs(containerId, req);
    });

    CROW_ROUTE(app, "/api/docker/containers/detailed").methods("GET"_method)
    ([this](const crow::request& req) {
        return handleGetDetailedContainers(req);
    });

    // Docker Compose endpoints
    CROW_ROUTE(app, "/api/docker/compose/projects").methods("POST"_method)
    ([this](const crow::request& req) {
        return handleLoadComposeFile(req);
    });

    CROW_ROUTE(app, "/api/docker/compose/projects").methods("GET"_method)
    ([this](const crow::request& req) {
        return handleListComposeProjects();
    });

    CROW_ROUTE(app, "/api/docker/compose/projects/<string>").methods("DELETE"_method)
    ([this](const crow::request& req, const std::string& projectName) {
        return handleRemoveComposeProject(projectName);
    });

    CROW_ROUTE(app, "/api/docker/compose/projects/<string>/up").methods("POST"_method)
    ([this](const crow::request& req, const std::string& projectName) {
        return handleComposeUp(projectName, req);
    });

    CROW_ROUTE(app, "/api/docker/compose/projects/<string>/down").methods("POST"_method)
    ([this](const crow::request& req, const std::string& projectName) {
        return handleComposeDown(projectName, req);
    });

    CROW_ROUTE(app, "/api/docker/compose/projects/<string>/restart").methods("POST"_method)
    ([this](const crow::request& req, const std::string& projectName) {
        return handleComposeRestart(projectName);
    });

    CROW_ROUTE(app, "/api/docker/compose/projects/<string>/services").methods("GET"_method)
    ([this](const crow::request& req, const std::string& projectName) {
        return handleComposeServices(projectName);
    });

    CROW_ROUTE(app, "/api/docker/compose/projects/<string>/services/<string>/<string>").methods("POST"_method)
    ([this](const crow::request& req, const std::string& projectName, const std::string& serviceName, const std::string& action) {
        return handleComposeServiceAction(projectName, serviceName, req);
    });

    // Complete Docker installation endpoint (Docker + Docker Compose + Services)
    // CROW_ROUTE(app, "/api/docker/install-complete").methods("POST"_method)
    // ([this](const crow::request& req) {
    //     return handleInstallDockerComplete();
    // });
    
    // Service enablement endpoints
    CROW_ROUTE(app, "/api/docker/service/enable").methods("POST"_method)
    ([this](const crow::request& req) {
        return handleEnableDockerService();
    });
    
    CROW_ROUTE(app, "/api/docker/service/disable").methods("POST"_method)
    ([this](const crow::request& req) {
        return handleDisableDockerService();
    });
    
    // System integration status endpoint
    CROW_ROUTE(app, "/api/docker/system/integration-status").methods("GET"_method)
    ([this](const crow::request& req) {
        return handleGetSystemIntegrationStatus();
    });


    // Test endpoint
    CROW_ROUTE(app, "/api/test/sudo").methods("POST"_method)
    ([this](const crow::request& req) {
        return handleTestSudo(req);
    });
}

crow::response DockerManager::handleGetDockerInfo() {
    crow::response res;
    try {
        DockerInfo info = getDockerInfo();
        
        crow::json::wvalue json_response;
        json_response["installed"] = info.installed;
        json_response["version"] = info.version;
        json_response["api_version"] = info.api_version;
        json_response["min_api_version"] = info.min_api_version;
        json_response["git_commit"] = info.git_commit;
        json_response["go_version"] = info.go_version;
        json_response["os"] = info.os;
        json_response["arch"] = info.arch;
        json_response["kernel_version"] = info.kernel_version;
        json_response["experimental"] = info.experimental;
        json_response["build_time"] = info.build_time;
        json_response["error_message"] = info.error_message;
        
        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success
        // broadcastLog("docker_info", "Docker info retrieved successfully", "info");
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("docker_info", "Failed to retrieve Docker info: " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleGetDockerComposeInfo() {
    crow::response res;
    try {
        DockerComposeInfo info = getDockerComposeInfo();
        
        crow::json::wvalue json_response;
        json_response["installed"] = info.installed;
        json_response["version"] = info.version;
        // json_response["docker_compose_version"] = info.docker_compose_version;
        json_response["error_message"] = info.error_message;
        
        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success
        // broadcastLog("docker_compose_info", "Docker Compose info retrieved successfully", "info");
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("docker_compose_info", "Failed to retrieve Docker Compose info: " + std::string(e.what()), "error");
    }
    return res;
}

// crow::response DockerManager::handleInstallDocker() {
//     crow::response res;
//     try {
//         // if (installation_in_progress_) {
//         //     crow::json::wvalue error_response;
//         //     error_response["error"] = "Installation already in progress";
//         //     res.code = 409;
//         //     res.set_header("Content-Type", "application/json");
//         //     res.write(error_response.dump());
//         //     return res;
//         // }

//         // // Start installation in background thread
//         // std::thread([this]() {
//         //     installDocker([](const InstallationProgress& progress) {
//         //         // Progress callback can be used for WebSocket updates
//         //     });
//         // }).detach();

//         crow::json::wvalue json_response;
//         json_response["message"] = "Docker installation started";
//         json_response["status"] = "started";
        
//         res.code = 202;
//         res.set_header("Content-Type", "application/json");
//         res.write(json_response.dump());
        
//     } catch (const std::exception& e) {
//         crow::json::wvalue error_response;
//         error_response["error"] = e.what();
//         res.code = 500;
//         res.set_header("Content-Type", "application/json");
//         res.write(error_response.dump());
//     }
//     return res;
// }

crow::response DockerManager::handleUninstallDocker() {
    crow::response res;
    try {
        if (requiresSudoPermission("uninstall_docker")) {
            if (!validateSudoPassword()) {
                return createUnauthorizedResponse("uninstall_docker");
            }
        }
        bool success = uninstallDocker();
        
        crow::json::wvalue json_response;
        json_response["success"] = success;
        json_response["message"] = success ? "Docker uninstalled successfully" : "Failed to uninstall Docker";
        
        res.code = success ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success or failure
        if (success) {
            broadcastLog("docker_uninstall", "Docker uninstalled successfully", "success");
        } else {
            broadcastLog("docker_uninstall", "Failed to uninstall Docker", "error");
        }
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("docker_uninstall", "Error uninstalling Docker: " + std::string(e.what()), "error");
    }
    return res;
}

// crow::response DockerManager::handleInstallDockerCompose() {
//     crow::response res;
//     try {
//         if (requiresSudoPermission("install_docker_compose")) {
//             if (!validateSudoPassword()) {
//                 return createUnauthorizedResponse("install_docker_compose");
//             }
//         }

//         // if (installation_in_progress_) {
//         //     crow::json::wvalue error_response;
//         //     error_response["error"] = "Installation already in progress";
//         //     res.code = 409;
//         //     res.set_header("Content-Type", "application/json");
//         //     res.write(error_response.dump());
//         //     return res;
//         // }

//         std::thread([this]() {
//             installDockerCompose([](const InstallationProgress& progress) {
//                 // Progress callback
//             });
//         }).detach();

//         crow::json::wvalue json_response;
//         json_response["message"] = "Docker Compose installation started";
//         json_response["status"] = "started";
        
//         res.code = 202;
//         res.set_header("Content-Type", "application/json");
//         res.write(json_response.dump());
        
//     } catch (const std::exception& e) {
//         crow::json::wvalue error_response;
//         error_response["error"] = e.what();
//         res.code = 500;
//         res.set_header("Content-Type", "application/json");
//         res.write(error_response.dump());
//     }
//     return res;
// }

// crow::response DockerManager::handleUninstallDockerCompose() {
//     crow::response res;
//     try {
//         if (requiresSudoPermission("uninstall_docker_compose")) {
//             if (!validateSudoPassword()) {
//                 return createUnauthorizedResponse("uninstall_docker_compose");
//             }
//         }
//         bool success = uninstallDockerCompose();
        
//         crow::json::wvalue json_response;
//         json_response["success"] = success;
//         json_response["message"] = success ? "Docker Compose uninstalled successfully" : "Failed to uninstall Docker Compose";
        
//         res.code = success ? 200 : 500;
//         res.set_header("Content-Type", "application/json");
//         res.write(json_response.dump());
        
//     } catch (const std::exception& e) {
//         crow::json::wvalue error_response;
//         error_response["error"] = e.what();
//         res.code = 500;
//         res.set_header("Content-Type", "application/json");
//         res.write(error_response.dump());
//     }
//     return res;
// }

crow::response DockerManager::handleStartDockerService() {
    crow::response res;
    try {
        if (requiresSudoPermission("start_service")) {
            if (!validateSudoPassword()) {
                return createUnauthorizedResponse("start_service");
            }
        }
        bool success = startDockerService();
        
        crow::json::wvalue json_response;
        json_response["success"] = success;
        json_response["message"] = success ? "Docker service started successfully" : "Failed to start Docker service";
        
        res.code = success ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success or failure
        if (success) {
            broadcastLog("docker_service", "Docker service started successfully", "success");
        } else {
            broadcastLog("docker_service", "Failed to start Docker service", "error");
        }
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("docker_service", "Error starting Docker service: " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleStopDockerService() {
    crow::response res;
    try {
        if (requiresSudoPermission("stop_service")) {
            if (!validateSudoPassword()) {
                return createUnauthorizedResponse("stop_service");
            }
        }
        bool success = stopDockerService();
        
        crow::json::wvalue json_response;
        json_response["success"] = success;
        json_response["message"] = success ? "Docker service stopped successfully" : "Failed to stop Docker service";
        
        res.code = success ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success or failure
        if (success) {
            broadcastLog("docker_service", "Docker service stopped successfully", "success");
        } else {
            broadcastLog("docker_service", "Failed to stop Docker service", "error");
        }
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("docker_service", "Error stopping Docker service: " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleRestartDockerService() {
    crow::response res;
    try {
        if (requiresSudoPermission("restart_service")) {
            if (!validateSudoPassword()) {
                return createUnauthorizedResponse("restart_service");
            }
        }
        bool success = restartDockerService();
        
        crow::json::wvalue json_response;
        json_response["success"] = success;
        json_response["message"] = success ? "Docker service restarted successfully" : "Failed to restart Docker service";
        
        res.code = success ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success or failure
        if (success) {
            broadcastLog("docker_service", "Docker service restarted successfully", "success");
        } else {
            broadcastLog("docker_service", "Failed to restart Docker service", "error");
        }
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("docker_service", "Error restarting Docker service: " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleGetServiceStatus() {
    crow::response res;
    try {
        std::string status = getDockerServiceStatus();
        
        crow::json::wvalue json_response;
        json_response["status"] = status;
        json_response["running"] = isDockerRunning();
        
        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success
        // broadcastLog("docker_service", "Docker service status retrieved successfully", "info");
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("docker_service", "Error retrieving Docker service status: " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleListContainers(const crow::request& req) {
    crow::response res;
    try {
        bool all = req.url_params.get("all") != nullptr;
        std::vector<std::string> containers = listContainers(all);
        
        crow::json::wvalue json_response;
        json_response["containers"] = crow::json::wvalue::list();
        
        for (size_t i = 0; i < containers.size(); ++i) {
            try {
                // Parse the JSON string from Docker
                crow::json::rvalue parsed_container = crow::json::load(containers[i]);
                
                crow::json::wvalue container_obj;
                container_obj["id"] = parsed_container["ID"].s();
                container_obj["name"] = parsed_container["Names"].s();
                container_obj["image"] = parsed_container["Image"].s();
                container_obj["status"] = parsed_container["Status"].s();
                container_obj["ports"] = parsed_container["Ports"].s();
                container_obj["created"] = parsed_container["CreatedAt"].s();
                
                json_response["containers"][i] = std::move(container_obj);
            } catch (const std::exception& e) {
                // If JSON parsing fails, skip this container
                continue;
            }
        }
        
        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success
        // broadcastLog("container_list", "Container list retrieved successfully", "info");
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("container_list", "Error retrieving container list: " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleListImages() {
    crow::response res;
    try {
        std::vector<std::string> images = listImages();
        
        crow::json::wvalue json_response;
        json_response["images"] = crow::json::wvalue::list();
        
        for (size_t i = 0; i < images.size(); ++i) {
            try {
                // Parse the JSON string from Docker
                crow::json::rvalue parsed_image = crow::json::load(images[i]);
                
                crow::json::wvalue image_obj;
                image_obj["id"] = parsed_image["ID"].s();
                image_obj["repository"] = parsed_image["Repository"].s();
                image_obj["tag"] = parsed_image["Tag"].s();
                image_obj["size"] = parsed_image["Size"].s();
                image_obj["created"] = parsed_image["CreatedAt"].s();
                
                json_response["images"][i] = std::move(image_obj);
            } catch (const std::exception& e) {
                // If JSON parsing fails, skip this image
                continue;
            }
        }
        
        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success
        // broadcastLog("image_list", "Image list retrieved successfully", "info");
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("image_list", "Error retrieving image list: " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handlePullImage(const crow::request& req) {
    crow::response res;
    try {
        auto json_data = crow::json::load(req.body);
        if (!json_data) {
            crow::json::wvalue error_response;
            error_response["error"] = "Invalid JSON in request body";
            res.code = 400;
            res.set_header("Content-Type", "application/json");
            res.write(error_response.dump());
            // Log error
            broadcastLog("image_pull", "Invalid JSON in request body", "error");
            return res;
        }

        std::string imageName = json_data["image"].s();
        if (imageName.empty()) {
            crow::json::wvalue error_response;
            error_response["error"] = "Image name is required";
            res.code = 400;
            res.set_header("Content-Type", "application/json");
            res.write(error_response.dump());
            // Log error
            broadcastLog("image_pull", "Image name is required", "error");
            return res;
        }

        bool success = pullImage(imageName, [this](const std::string& _str){
            broadcastLog("IMAGE_PULL", _str, "info");
        });
        
        crow::json::wvalue json_response;
        json_response["success"] = success;
        json_response["message"] = success ? "Image pulled successfully" : "Failed to pull image";
        json_response["image"] = imageName;
        
        res.code = success ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success or failure
        if (success) {
            broadcastLog("image_pull", "Image pulled successfully: " + imageName, "success");
        } else {
            broadcastLog("image_pull", "Failed to pull image: " + imageName, "error");
        }
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("image_pull", "Error pulling image: " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleRemoveContainer(const std::string& containerId, const crow::request& req) {
    crow::response res;
    try {
        bool force = req.url_params.get("force") != nullptr;
        bool success = removeContainer(containerId, force);
        
        crow::json::wvalue json_response;
        json_response["success"] = success;
        json_response["message"] = success ? "Container removed successfully" : "Failed to remove container";
        json_response["container_id"] = containerId;
        
        res.code = success ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success or failure
        if (success) {
            broadcastLog("container_remove", "Container removed successfully: " + containerId, "success");
        } else {
            broadcastLog("container_remove", "Failed to remove container: " + containerId, "error");
        }
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("container_remove", "Error removing container " + containerId + ": " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleRemoveImage(const std::string& imageId, const crow::request& req) {
    crow::response res;
    try {
        bool force = req.url_params.get("force") != nullptr;
        bool success = removeImage(imageId, force);
        
        crow::json::wvalue json_response;
        json_response["success"] = success;
        json_response["message"] = success ? "Image removed successfully" : "Failed to remove image";
        json_response["image_id"] = imageId;
        
        res.code = success ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success or failure
        if (success) {
            broadcastLog("image_remove", "Image removed successfully: " + imageId, "success");
        } else {
            broadcastLog("image_remove", "Failed to remove image: " + imageId, "error");
        }
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("image_remove", "Error removing image " + imageId + ": " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleGetSystemInfo() {
    crow::response res;
    try {
        std::string info = getSystemInfo();
        
        // crow::json::wvalue json_response;
        // json_response["system_info"] = info;
        
        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.write(info);
        
        // Log success
        broadcastLog("system_info", "System info retrieved successfully", "info");
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("system_info", "Error retrieving system info: " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleGetDiskUsage() {
    crow::response res;
    try {
        std::string usage = getDiskUsage();
        
        // crow::json::wvalue json_response;
        // json_response["disk_usage"] = usage;
        
        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.write(usage);
        
        // Log success
        broadcastLog("disk_usage", "Disk usage retrieved successfully", "info");
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("disk_usage", "Error retrieving disk usage: " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleCleanupSystem() {
    crow::response res;
    try {
        if (requiresSudoPermission("cleanup_system")) {
            if (!validateSudoPassword()) {
                return createUnauthorizedResponse("cleanup_system");
            }
        }
        bool success = cleanupSystem();
        
        crow::json::wvalue json_response;
        json_response["success"] = success;
        json_response["message"] = success ? "System cleanup completed successfully" : "Failed to cleanup system";
        
        res.code = success ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success or failure
        if (success) {
            broadcastLog("system_cleanup", "System cleanup completed successfully", "success");
        } else {
            broadcastLog("system_cleanup", "Failed to cleanup system", "error");
        }
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("system_cleanup", "Error during system cleanup: " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleGetInstallationProgress() {
    crow::response res;
    try {
        InstallationProgress progress = getCurrentProgress();
        
        crow::json::wvalue json_response;
        json_response["status"] = static_cast<int>(progress.status);
        json_response["percentage"] = progress.percentage;
        json_response["message"] = progress.message;
        json_response["error_details"] = progress.error_details;
        json_response["installation_in_progress"] = installation_in_progress_;
        
        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success
        broadcastLog("installation_progress", "Installation progress retrieved successfully", "info");
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("installation_progress", "Error retrieving installation progress: " + std::string(e.what()), "error");
    }
    return res;
}

// Advanced image management handlers
crow::response DockerManager::handleLoadImageFromFile(const crow::request& req) {
    crow::response res;
    try {
        auto json_data = crow::json::load(req.body);
        if (!json_data) {
            crow::json::wvalue error_response;
            error_response["error"] = "Invalid JSON in request body";
            res.code = 400;
            res.set_header("Content-Type", "application/json");
            res.write(error_response.dump());
            // Log error
            broadcastLog("image_load", "Invalid JSON in request body", "error");
            return res;
        }

        std::string filePath = json_data["file_path"].s();
        if (filePath.empty()) {
            crow::json::wvalue error_response;
            error_response["error"] = "File path is required";
            res.code = 400;
            res.set_header("Content-Type", "application/json");
            res.write(error_response.dump());
            // Log error
            broadcastLog("image_load", "File path is required", "error");
            return res;
        }

        bool success = loadImageFromFile(filePath);
        
        crow::json::wvalue json_response;
        json_response["success"] = success;
        json_response["message"] = success ? "Image loaded successfully" : "Failed to load image";
        json_response["file_path"] = filePath;
        
        res.code = success ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success or failure
        if (success) {
            broadcastLog("image_load", "Image loaded successfully from: " + filePath, "success");
        } else {
            broadcastLog("image_load", "Failed to load image from: " + filePath, "error");
        }
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("image_load", "Error loading image: " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleSaveImageToFile(const crow::request& req) {
    crow::response res;
    try {
        auto json_data = crow::json::load(req.body);
        if (!json_data) {
            crow::json::wvalue error_response;
            error_response["error"] = "Invalid JSON in request body";
            res.code = 400;
            res.set_header("Content-Type", "application/json");
            res.write(error_response.dump());
            // Log error
            broadcastLog("image_save", "Invalid JSON in request body", "error");
            return res;
        }

        std::string imageName = json_data["image_name"].s();
        std::string filePath = json_data["file_path"].s();
        
        if (imageName.empty() || filePath.empty()) {
            crow::json::wvalue error_response;
            error_response["error"] = "Image name and file path are required";
            res.code = 400;
            res.set_header("Content-Type", "application/json");
            res.write(error_response.dump());
            // Log error
            broadcastLog("image_save", "Image name and file path are required", "error");
            return res;
        }

        bool success = saveImageToFile(imageName, filePath);
        
        crow::json::wvalue json_response;
        json_response["success"] = success;
        json_response["message"] = success ? "Image saved successfully" : "Failed to save image";
        json_response["image_name"] = imageName;
        json_response["file_path"] = filePath;
        
        res.code = success ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success or failure
        if (success) {
            broadcastLog("image_save", "Image saved successfully: " + imageName + " to " + filePath, "success");
        } else {
            broadcastLog("image_save", "Failed to save image: " + imageName + " to " + filePath, "error");
        }
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("image_save", "Error saving image: " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleTagImage(const crow::request& req) {
    crow::response res;
    try {
        auto json_data = crow::json::load(req.body);
        if (!json_data) {
            crow::json::wvalue error_response;
            error_response["error"] = "Invalid JSON in request body";
            res.code = 400;
            res.set_header("Content-Type", "application/json");
            res.write(error_response.dump());
            // Log error
            broadcastLog("image_tag", "Invalid JSON in request body", "error");
            return res;
        }

        std::string sourceImage = json_data["source_image"].s();
        std::string targetTag = json_data["target_tag"].s();
        
        if (sourceImage.empty() || targetTag.empty()) {
            crow::json::wvalue error_response;
            error_response["error"] = "Source image and target tag are required";
            res.code = 400;
            res.set_header("Content-Type", "application/json");
            res.write(error_response.dump());
            // Log error
            broadcastLog("image_tag", "Source image and target tag are required", "error");
            return res;
        }

        bool success = tagImage(sourceImage, targetTag);
        
        crow::json::wvalue json_response;
        json_response["success"] = success;
        json_response["message"] = success ? "Image tagged successfully" : "Failed to tag image";
        json_response["source_image"] = sourceImage;
        json_response["target_tag"] = targetTag;
        
        res.code = success ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success or failure
        if (success) {
            broadcastLog("image_tag", "Image tagged successfully: " + sourceImage + " to " + targetTag, "success");
        } else {
            broadcastLog("image_tag", "Failed to tag image: " + sourceImage + " to " + targetTag, "error");
        }
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("image_tag", "Error tagging image: " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleBuildImage(const crow::request& req) {
    crow::response res;
    try {
        auto json_data = crow::json::load(req.body);
        if (!json_data) {
            crow::json::wvalue error_response;
            error_response["error"] = "Invalid JSON in request body";
            res.code = 400;
            res.set_header("Content-Type", "application/json");
            res.write(error_response.dump());
            // Log error
            broadcastLog("image_build", "Invalid JSON in request body", "error");
            return res;
        }

        std::string dockerfilePath = json_data["dockerfile_path"].s();
        std::string imageName = json_data["image_name"].s();
        std::string buildContext = json_data["build_context"].s();
        
        if (dockerfilePath.empty() || imageName.empty()) {
            crow::json::wvalue error_response;
            error_response["error"] = "Dockerfile path and image name are required";
            res.code = 400;
            res.set_header("Content-Type", "application/json");
            res.write(error_response.dump());
            // Log error
            broadcastLog("image_build", "Dockerfile path and image name are required", "error");
            return res;
        }

        if (buildContext.empty()) {
            buildContext = ".";
        }

        bool success = buildImage(dockerfilePath, imageName, buildContext);
        
        crow::json::wvalue json_response;
        json_response["success"] = success;
        json_response["message"] = success ? "Image built successfully" : "Failed to build image";
        json_response["dockerfile_path"] = dockerfilePath;
        json_response["image_name"] = imageName;
        json_response["build_context"] = buildContext;
        
        res.code = success ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success or failure
        if (success) {
            broadcastLog("image_build", "Image built successfully: " + imageName + " from " + dockerfilePath, "success");
        } else {
            broadcastLog("image_build", "Failed to build image: " + imageName + " from " + dockerfilePath, "error");
        }
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("image_build", "Error building image: " + std::string(e.what()), "error");
    }
    return res;
}

// Container lifecycle handlers
crow::response DockerManager::handleStartContainer(const std::string& containerId) {
    crow::response res;
    try {
        bool success = startContainer(containerId);
        
        crow::json::wvalue json_response;
        json_response["success"] = success;
        json_response["message"] = success ? "Container started successfully" : "Failed to start container";
        json_response["container_id"] = containerId;
        
        res.code = success ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success or failure
        if (success) {
            broadcastLog("container_start", "Container started successfully: " + containerId, "success");
        } else {
            broadcastLog("container_start", "Failed to start container: " + containerId, "error");
        }
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("container_start", "Error starting container " + containerId + ": " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleStopContainer(const std::string& containerId, const crow::request& req) {
    crow::response res;
    try {
        int timeout = 10;
        auto timeout_param = req.url_params.get("timeout");
        if (timeout_param) {
            timeout = std::stoi(timeout_param);
        }
        
        bool success = stopContainer(containerId, timeout);
        
        crow::json::wvalue json_response;
        json_response["success"] = success;
        json_response["message"] = success ? "Container stopped successfully" : "Failed to stop container";
        json_response["container_id"] = containerId;
        json_response["timeout"] = timeout;
        
        res.code = success ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success or failure
        if (success) {
            broadcastLog("container_stop", "Container stopped successfully: " + containerId, "success");
        } else {
            broadcastLog("container_stop", "Failed to stop container: " + containerId, "error");
        }
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("container_stop", "Error stopping container " + containerId + ": " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleRestartContainer(const std::string& containerId, const crow::request& req) {
    crow::response res;
    try {
        int timeout = 10;
        auto timeout_param = req.url_params.get("timeout");
        if (timeout_param) {
            timeout = std::stoi(timeout_param);
        }
        
        bool success = restartContainer(containerId, timeout);
        
        crow::json::wvalue json_response;
        json_response["success"] = success;
        json_response["message"] = success ? "Container restarted successfully" : "Failed to restart container";
        json_response["container_id"] = containerId;
        json_response["timeout"] = timeout;
        
        res.code = success ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success or failure
        if (success) {
            broadcastLog("container_restart", "Container restarted successfully: " + containerId, "success");
        } else {
            broadcastLog("container_restart", "Failed to restart container: " + containerId, "error");
        }
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("container_restart", "Error restarting container " + containerId + ": " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handlePauseContainer(const std::string& containerId) {
    crow::response res;
    try {
        bool success = pauseContainer(containerId);
        
        crow::json::wvalue json_response;
        json_response["success"] = success;
        json_response["message"] = success ? "Container paused successfully" : "Failed to pause container";
        json_response["container_id"] = containerId;
        
        res.code = success ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success or failure
        if (success) {
            broadcastLog("container_pause", "Container paused successfully: " + containerId, "success");
        } else {
            broadcastLog("container_pause", "Failed to pause container: " + containerId, "error");
        }
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("container_pause", "Error pausing container " + containerId + ": " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleUnpauseContainer(const std::string& containerId) {
    crow::response res;
    try {
        bool success = unpauseContainer(containerId);
        
        crow::json::wvalue json_response;
        json_response["success"] = success;
        json_response["message"] = success ? "Container unpaused successfully" : "Failed to unpause container";
        json_response["container_id"] = containerId;
        
        res.code = success ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success or failure
        if (success) {
            broadcastLog("container_unpause", "Container unpaused successfully: " + containerId, "success");
        } else {
            broadcastLog("container_unpause", "Failed to unpause container: " + containerId, "error");
        }
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("container_unpause", "Error unpausing container " + containerId + ": " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleGetContainerLogs(const std::string& containerId, const crow::request& req) {
    crow::response res;
    try {
        int lines = 200;
        auto lines_param = req.url_params.get("lines");
        if (lines_param) {
            lines = std::stoi(lines_param);
        }
        
        std::string logs = getContainerLogs(containerId, lines);
        
        crow::json::wvalue json_response;
        json_response["container_id"] = containerId;
        json_response["lines"] = lines;
        json_response["logs"] = logs;
        
        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success
        broadcastLog("container_logs", "Container logs retrieved successfully for: " + containerId, "info");
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("container_logs", "Error retrieving logs for container " + containerId + ": " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleGetDetailedContainers(const crow::request& req) {
    crow::response res;
    try {
        bool all = req.url_params.get("all") != nullptr;
        std::vector<ContainerInfo> containers = getDetailedContainers(all);
        
        crow::json::wvalue json_response;
        json_response["containers"] = crow::json::wvalue::list();
        
        for (size_t i = 0; i < containers.size(); ++i) {
            const auto& container = containers[i];
            crow::json::wvalue container_json;
            container_json["id"] = container.id;
            container_json["name"] = container.name;
            container_json["image"] = container.image;
            container_json["status"] = container.status;
            container_json["created"] = container.created;
            container_json["ports"] = crow::json::wvalue::list();
            
            for (size_t j = 0; j < container.ports.size(); ++j) {
                container_json["ports"][j] = container.ports[j];
            }
            
            json_response["containers"][i] = std::move(container_json);
        }
        
        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success
        // broadcastLog("container_detailed", "Detailed container list retrieved successfully", "info");
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("container_detailed", "Error retrieving detailed container list: " + std::string(e.what()), "error");
    }
    return res;
}

// Docker Compose handlers
crow::response DockerManager::handleLoadComposeFile(const crow::request& req) {
    crow::response res;
    try {
        auto json_data = crow::json::load(req.body);
        if (!json_data) {
            crow::json::wvalue error_response;
            error_response["error"] = "Invalid JSON in request body";
            res.code = 400;
            res.set_header("Content-Type", "application/json");
            res.write(error_response.dump());
            // Log error
            broadcastLog("compose_load", "Invalid JSON in request body", "error");
            return res;
        }

        std::string composeFilePath = json_data["compose_file_path"].s();
        std::string projectName = json_data["project_name"].s();
        std::string workingDir = json_data["working_directory"].s();
        
        if (composeFilePath.empty() || projectName.empty()) {
            crow::json::wvalue error_response;
            error_response["error"] = "Compose file path and project name are required";
            res.code = 400;
            res.set_header("Content-Type", "application/json");
            res.write(error_response.dump());
            // Log error
            broadcastLog("compose_load", "Compose file path and project name are required", "error");
            return res;
        }

        bool success = loadComposeFile(composeFilePath, projectName, workingDir);
        
        crow::json::wvalue json_response;
        json_response["success"] = success;
        json_response["message"] = success ? "Compose file loaded successfully" : "Failed to load compose file";
        json_response["project_name"] = projectName;
        json_response["compose_file_path"] = composeFilePath;
        json_response["working_directory"] = workingDir;
        
        res.code = success ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success or failure
        if (success) {
            broadcastLog("compose_load", "Compose file loaded successfully: " + projectName, "success");
        } else {
            broadcastLog("compose_load", "Failed to load compose file: " + projectName, "error");
        }
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("compose_load", "Error loading compose file: " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleRemoveComposeProject(const std::string& projectName) {
    crow::response res;
    try {
        bool success = removeComposeProject(projectName);
        
        crow::json::wvalue json_response;
        json_response["success"] = success;
        json_response["message"] = success ? "Compose project removed successfully" : "Failed to remove compose project";
        json_response["project_name"] = projectName;
        
        res.code = success ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success or failure
        if (success) {
            broadcastLog("compose_remove", "Compose project removed successfully: " + projectName, "success");
        } else {
            broadcastLog("compose_remove", "Failed to remove compose project: " + projectName, "error");
        }
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("compose_remove", "Error removing compose project: " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleListComposeProjects() {
    crow::response res;
    try {
        std::vector<DockerComposeProject> projects = listComposeProjects();
        
        crow::json::wvalue json_response;
        json_response["projects"] = crow::json::wvalue::list();
        
        for (size_t i = 0; i < projects.size(); ++i) {
            const auto& project = projects[i];
            crow::json::wvalue project_json;
            project_json["name"] = project.name;
            project_json["compose_file_path"] = project.compose_file_path;
            project_json["working_directory"] = project.working_directory;
            project_json["services"] = crow::json::wvalue::list();
            
            for (size_t j = 0; j < project.services.size(); ++j) {
                project_json["services"][j] = project.services[j];
            }
            
            json_response["projects"][i] = std::move(project_json);
        }
        
        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success
        broadcastLog("compose_list", "Compose projects list retrieved successfully", "info");
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("compose_list", "Error retrieving compose projects list: " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleComposeUp(const std::string& projectName, const crow::request& req) {
    crow::response res;
    try {
        bool detached = req.url_params.get("detached") != nullptr;
        bool success = composeUp(projectName, detached);
        
        crow::json::wvalue json_response;
        json_response["success"] = success;
        json_response["message"] = success ? "Compose project started successfully" : "Failed to start compose project";
        json_response["project_name"] = projectName;
        json_response["detached"] = detached;
        
        res.code = success ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success or failure
        if (success) {
            broadcastLog("compose_up", "Compose project started successfully: " + projectName, "success");
        } else {
            broadcastLog("compose_up", "Failed to start compose project: " + projectName, "error");
        }
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("compose_up", "Error starting compose project: " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleComposeDown(const std::string& projectName, const crow::request& req) {
    crow::response res;
    try {
        bool removeVolumes = req.url_params.get("remove_volumes") != nullptr;
        bool success = composeDown(projectName, removeVolumes);
        
        crow::json::wvalue json_response;
        json_response["success"] = success;
        json_response["message"] = success ? "Compose project stopped successfully" : "Failed to stop compose project";
        json_response["project_name"] = projectName;
        json_response["remove_volumes"] = removeVolumes;
        
        res.code = success ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success or failure
        if (success) {
            broadcastLog("compose_down", "Compose project stopped successfully: " + projectName, "success");
        } else {
            broadcastLog("compose_down", "Failed to stop compose project: " + projectName, "error");
        }
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("compose_down", "Error stopping compose project: " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleComposeRestart(const std::string& projectName) {
    crow::response res;
    try {
        bool success = composeRestart(projectName);
        
        crow::json::wvalue json_response;
        json_response["success"] = success;
        json_response["message"] = success ? "Compose project restarted successfully" : "Failed to restart compose project";
        json_response["project_name"] = projectName;
        
        res.code = success ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success or failure
        if (success) {
            broadcastLog("compose_restart", "Compose project restarted successfully: " + projectName, "success");
        } else {
            broadcastLog("compose_restart", "Failed to restart compose project: " + projectName, "error");
        }
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("compose_restart", "Error restarting compose project: " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleComposeServices(const std::string& projectName) {
    crow::response res;
    try {
        std::vector<std::string> services = composeServices(projectName);
        
        crow::json::wvalue json_response;
        json_response["project_name"] = projectName;
        json_response["services"] = crow::json::wvalue::list();
        
        for (size_t i = 0; i < services.size(); ++i) {
            json_response["services"][i] = services[i];
        }
        
        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success
        broadcastLog("compose_services", "Compose services retrieved successfully for project: " + projectName, "info");
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("compose_services", "Error retrieving compose services for project: " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleComposeServiceAction(const std::string& projectName, const std::string& serviceName, const crow::request& req) {
    crow::response res;
    std::string action_str;
    try {
        auto json_data = crow::json::load(req.body);
        action_str = "start"; // default action
        
        if (json_data) {
            action_str = json_data["action"].s();
        }
        
        if (action_str.empty()) {
            action_str = "start";
        }
        
        bool success = composeServiceAction(projectName, serviceName, action_str);
        
        crow::json::wvalue json_response;
        json_response["success"] = success;
        json_response["message"] = success ? "Service action completed successfully" : "Failed to execute service action";
        json_response["project_name"] = projectName;
        json_response["service_name"] = serviceName;
        json_response["action"] = action_str;
        
        res.code = success ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        
        // Log success or failure
        if (success) {
            broadcastLog("compose_service_action", "Service action completed successfully for project: " + projectName + ", service: " + serviceName + ", action: " + action_str, "success");
        } else {
            broadcastLog("compose_service_action", "Failed to execute service action for project: " + projectName + ", service: " + serviceName + ", action: " + action_str, "error");
        }
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        
        // Log error
        broadcastLog("compose_service_action", "Error executing service action for project: " + projectName + ", service: " + serviceName + ", action: " + action_str + " - " + std::string(e.what()), "error");
    }
    return res;
}

crow::response DockerManager::handleTestSudo(const crow::request& _req) {

    crow::response res;
    
    const auto _json = crow::json::load(_req.body);
    const std::string _key_paswd{"password"};
    if(!_json.has(_key_paswd) || _json[_key_paswd].size() < 1)
    {
        crow::json::wvalue error_response;
        error_response["success"] = false;
        error_response["message"] = "no password is provided";
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        broadcastLog(__FUNCTION__, error_response.dump(), "error");
        return res;
    }

    try {
        const std::string _password = _json[_key_paswd].s();
        // Simple sudo test command
        bool _sudo_ok = Utils::test_sudo(_password);
        
        crow::json::wvalue json_response;
        
        if (true == _sudo_ok) {
            sudo_password_ = _password;
            m_env_parser.set(CONST_KEY_SUDO_PSWD, _password, Utils::get_env_secret_file_path());
            if(!EnvConfig::set_value(EnvKey::SUDO_PASSWORD, _password))
            {
                crow::json::wvalue error_response;
                error_response["success"] = false;
                error_response["message"] = "password is ok but failed to store password into file!!!";
                res.code = 500;
                res.set_header("Content-Type", "application/json");
                res.write(error_response.dump());
                broadcastLog(__FUNCTION__, error_response.dump(), "info");
                return res;
            }
        }
        
        json_response["success"] = (true == _sudo_ok);
        json_response["message"] = (true == _sudo_ok) ? "Sudo access available" : "Sudo access not available";
        res.code = (true == _sudo_ok) ? 200 : 403;
        res.set_header("Content-Type", "application/json");
        res.write(json_response.dump());
        broadcastLog(__FUNCTION__, json_response.dump(), (true == _sudo_ok) ? "info" : "error");
        
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["success"] = false;
        error_response["message"] = e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
        broadcastLog(__FUNCTION__, error_response.dump(), "error");
    }
    return res;
}

crow::response DockerManager::handleGetDocumentation() {
    crow::response res;
    try {
        // Read the comprehensive HTML documentation file
        std::string docPath = "comprehensive_docs.html";
        std::ifstream file(docPath);
        
        if (!file.is_open()) {
            // Try alternative paths
            std::vector<std::string> possiblePaths = {
                "../comprehensive_docs.html",
                "./comprehensive_docs.html"
            };
            
            bool found = false;
            for (const auto& path : possiblePaths) {
                file.open(path);
                if (file.is_open()) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                crow::json::wvalue error_response;
                error_response["error"] = "Documentation file not found";
                res.code = 404;
                res.set_header("Content-Type", "application/json");
                res.write(error_response.dump());
                return res;
            }
        }
        
        // Read the entire file content
        std::string htmlContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        
        // Set response headers
        res.code = 200;
        res.set_header("Content-Type", "text/html; charset=utf-8");
        res.set_header("Cache-Control", "public, max-age=3600"); // Cache for 1 hour
        res.set_header("X-Content-Type-Options", "nosniff");
        res.write(htmlContent);
        
    } catch (const std::exception& e) {
        crow::json::wvalue error_response;
        error_response["error"] = std::string("Failed to serve documentation: ") + e.what();
        res.code = 500;
        res.set_header("Content-Type", "application/json");
        res.write(error_response.dump());
    }
    return res;
}

crow::response DockerManager::handleInstallDocker() {
    crow::response res;
    try {
        if (requiresSudoPermission("install_docker")) {
            if (!validateSudoPassword()) {
                return createUnauthorizedResponse("install_docker");
            }
        }
        // if (installation_in_progress_) {
        //     crow::json::wvalue error_response;
        //     error_response["error"] = "Installation already in progress";
        //     res.code = 409;
        //     res.set_header("Content-Type", "application/json");
        //     res.write(error_response.dump());
        //     return res;
        // }

        bool dockerSuccess = installDocker([](const InstallationProgress& progress) {
            crow::logger(crow::LogLevel::Debug) <<"[STAT]:" << get_installation_status(progress.status) << ",[PERC]:" << progress.percentage << "%,[MSG]:" << progress.message << ",[ERR]:" << progress.error_details;
        });
        
        // bool dockerComposeSuccess = false;
        // if (dockerSuccess) {
        //     // Install Docker Compose
        //     dockerComposeSuccess = installDockerCompose([](const InstallationProgress& progress) {
        //         // Progress callback
        //     });
        // }
        // Start complete installation in background thread
        // std::thread([this]() {
        //     // Install Docker first
        // }).detach();

        crow::json::wvalue json_response;
        std::stringstream _ss;
        _ss << "docker installation " << (dockerSuccess ? "successfull" : "failed") << ".";
        // json_response["message"] = "Complete Docker installation started (Docker + Docker Compose + Services)";
        json_response["message"] = _ss.str();
        json_response["status"] = "finished";
        
        res.code = 202;
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

crow::response DockerManager::handleEnableDockerService() {
    crow::response res;
    try {
        if (requiresSudoPermission("enable_service")) {
            if (!validateSudoPassword()) {
                return createUnauthorizedResponse("enable_service");
            }
        }
        auto [pid, ret_code] = process_manager_->startProcessBlockingAsRoot(
            "/usr/bin/systemctl",
            {"enable", "docker.service"},
            {},
            sudo_password_,
            nullptr
        );
        
        auto [pid2, ret_code2] = process_manager_->startProcessBlockingAsRoot(
            "/usr/bin/systemctl",
            {"enable", "docker.socket"},
            {},
            sudo_password_,
            nullptr
        );
        
        bool success = (ret_code == 0) && (ret_code2 == 0);
        
        crow::json::wvalue json_response;
        json_response["success"] = success;
        json_response["message"] = success ? "Docker services enabled successfully" : "Failed to enable Docker services";
        
        res.code = success ? 200 : 500;
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

crow::response DockerManager::handleDisableDockerService() {
    crow::response res;
    try {
        if (requiresSudoPermission("disable_service")) {
            if (!validateSudoPassword()) {
                return createUnauthorizedResponse("disable_service");
            }
        }
        auto [pid, ret_code] = process_manager_->startProcessBlockingAsRoot(
            "/usr/bin/systemctl",
            {"disable", "docker.service"},
            {},
            sudo_password_,
            nullptr
        );
        
        auto [pid2, ret_code2] = process_manager_->startProcessBlockingAsRoot(
            "/usr/bin/systemctl",
            {"disable", "docker.socket"},
            {},
            sudo_password_,
            nullptr
        );
        
        bool success = (ret_code == 0) && (ret_code2 == 0);
        
        crow::json::wvalue json_response;
        json_response["success"] = success;
        json_response["message"] = success ? "Docker services disabled successfully" : "Failed to disable Docker services";
        
        res.code = success ? 200 : 500;
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

crow::response DockerManager::handleGetSystemIntegrationStatus() {
    crow::response res;
    try {
        crow::json::wvalue json_response;
        
        // Check if binaries are installed
        bool binaries_installed = fs::exists(docker_install_path_ + "/docker") && 
                                 fs::exists(docker_install_path_ + "/dockerd") &&
                                 fs::exists(docker_install_path_ + "/docker-compose");
        json_response["binaries_installed"] = binaries_installed;
        
        // Check if systemd service files are installed
        bool systemd_services_installed = fs::exists("/etc/systemd/system/docker.service") && 
                                         fs::exists("/etc/systemd/system/docker.socket");
        json_response["systemd_services_installed"] = systemd_services_installed;
        
        // Check if services are enabled
        auto [pid, ret_code] = process_manager_->startProcessBlocking(
            "/usr/bin/systemctl",
            {"is-enabled", "docker.service"},
            {},
            nullptr
        );
        bool docker_service_enabled = (ret_code == 0);
        json_response["docker_service_enabled"] = docker_service_enabled;
        
        auto [pid2, ret_code2] = process_manager_->startProcessBlocking(
            "/usr/bin/systemctl",
            {"is-enabled", "docker.socket"},
            {},
            nullptr
        );
        bool docker_socket_enabled = (ret_code2 == 0);
        json_response["docker_socket_enabled"] = docker_socket_enabled;
        
        // Check if docker group exists
        auto [pid3, ret_code3] = process_manager_->startProcessBlocking(
            "/usr/bin/getent",
            {"group", "docker"},
            {},
            nullptr
        );
        bool docker_group_exists = (ret_code3 == 0);
        json_response["docker_group_exists"] = docker_group_exists;
        
        // Overall status
        bool fully_integrated = binaries_installed && 
                               systemd_services_installed &&
                               docker_service_enabled &&
                               docker_socket_enabled &&
                               docker_group_exists;
        
        json_response["fully_integrated"] = fully_integrated;
        json_response["docker_running"] = isDockerRunning();
        
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
// WebSocket support implementations for DockerManager
#include "DockerManager.h"
#include <chrono>

void DockerManager::setWebSocketConnections(std::vector<crow::websocket::connection*>* log_connections, 
                                           std::vector<crow::websocket::connection*>* progress_connections) {
    log_connections_ = log_connections;
    progress_connections_ = progress_connections;
}

void DockerManager::broadcastLog(const std::string& operation, const std::string& message, const std::string& level) {
    if (!log_connections_) return;
    
    // Create JSON log message
    crow::json::wvalue log_message;
    log_message["type"] = "log";
    log_message["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    log_message["operation"] = operation;
    log_message["message"] = message;
    log_message["level"] = level;
    
    std::string json_str = log_message.dump();
    
    // Broadcast to all connected log clients
    for (auto it = log_connections_->begin(); it != log_connections_->end(); ) {
        auto* conn = *it;
        try {
            conn->send_text(json_str);
            ++it;
        } catch (const std::exception& e) {
            // Remove invalid connection
            it = log_connections_->erase(it);
        }
    }
}

void DockerManager::broadcastProgress(const InstallationProgress& progress) {
    if (!progress_connections_) return;
    
    // Create JSON progress message
    crow::json::wvalue progress_message;
    progress_message["type"] = "progress";
    progress_message["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    progress_message["status"] = static_cast<int>(progress.status);
    progress_message["percentage"] = progress.percentage;
    progress_message["message"] = progress.message;
    progress_message["error_details"] = progress.error_details;
    progress_message["installation_in_progress"] = installation_in_progress_;
    
    // Add status name for easier client-side handling
    switch (progress.status) {
        case InstallationStatus::NOT_STARTED:
            progress_message["status_name"] = "NOT_STARTED";
            break;
        case InstallationStatus::IN_PROGRESS:
            progress_message["status_name"] = "IN_PROGRESS";
            break;
        case InstallationStatus::COMPLETED:
            progress_message["status_name"] = "COMPLETED";
            break;
        case InstallationStatus::FAILED:
            progress_message["status_name"] = "FAILED";
            break;
        case InstallationStatus::EXTRACTING:
            progress_message["status_name"] = "EXTRACTING";
            break;
        case InstallationStatus::INSTALLING:
            progress_message["status_name"] = "INSTALLING";
            break;
        case InstallationStatus::CONFIGURING:
            progress_message["status_name"] = "CONFIGURING";
            break;
    }
    
    std::string json_str = progress_message.dump();
    // std::cerr << json_str << "\n";
    
    // Broadcast to all connected progress clients
    for (auto it = progress_connections_->begin(); it != progress_connections_->end(); ) {
        auto* conn = *it;
        try {
            conn->send_text(json_str);
            ++it;
        } catch (const std::exception& e) {
            // Remove invalid connection
            it = progress_connections_->erase(it);
        }
    }
}
