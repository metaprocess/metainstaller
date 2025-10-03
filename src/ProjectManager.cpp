#include "ProjectManager.h"
#include "utils.h"
#include "ProcessManager.h"
#include "json11.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <random>
#include <tuple>
#include "node.hpp"
#include <iostream>
#include "sqlite3.h"
#include "dotenv.hpp"

namespace fs = std::filesystem;

ProjectManager::ProjectManager()
    : process_manager_(std::make_unique<ProcessManager>())
{

    // Set default projects directory
    projects_directory_ = Utils::path_join_multiple({Utils::get_metainstaller_home_dir(),
                                                     "projects"});
    // temp_directory_ = "/tmp/metainstaller_temp";

    // Create directories if they don't exist
    std::filesystem::create_directories(projects_directory_);
    // std::filesystem::create_directories(temp_directory_);

    // Initialize database
    database_ = std::make_unique<MetaDatabase>();
    if (!database_->initDatabase()) {
        broadcastLog("ProjectManager", "Failed to initialize database", "error");
    } else {
        // Load projects from database
        projects_ = database_->loadProjectsFromDatabase();
        broadcastLog("ProjectManager", "Loaded " + std::to_string(projects_.size()) + " projects from database", "info");
    }

    broadcastLog("ProjectManager", "ProjectManager initialized", "info");
    std::string directoryPath = database_->getSetting(CONST_KEY_LAST_BROWSING_DIRECTORY);
    if(directoryPath.empty())
    {
        database_->saveSetting(CONST_KEY_LAST_BROWSING_DIRECTORY, std::getenv("HOME"));
    }
}

ProjectManager::~ProjectManager()
{
    // Clean up any running projects
    for (const auto &pair : projects_)
    {
        // if (pair.second.is_running)
        // {
        //     stopProject(pair.first);
        // }
    }
    broadcastLog("ProjectManager", "ProjectManager destroyed", "info");
}

std::string ProjectManager::executeCommandWithOutput(const std::string& command, const std::vector<std::string>& args) {
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

bool ProjectManager::loadImageFromFile(const std::string& filePath) {
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

// bool ProjectManager::loadComposeFile(const std::string& composeFilePath, const std::string& projectName, const std::string& workingDir) {
//     try {
//         if (!fs::exists(composeFilePath)) {
//             return false;
//         }
        
//         // Update the existing project info with Docker Compose data
//         auto it = projects_.find(projectName);
//         if (it == projects_.end()) {
//             return false;
//         }
        
//         ProjectInfo& project = it->second;
//         project.compose_file_path = composeFilePath;
//         project.working_directory = workingDir.empty() ? fs::path(composeFilePath).parent_path().string() : workingDir;
        
//         // Parse compose file to extract services (simplified)
//         std::ifstream file(composeFilePath);
//         if (file.is_open()) {
//             std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
//             // This is a simplified service extraction - in a real implementation,
//             // you'd want to parse the YAML properly
//             std::istringstream iss(content);
//             std::string line;
//             bool in_services_section = false;
//             while (std::getline(iss, line)) {
//                 if (line.find("services:") != std::string::npos) {
//                     in_services_section = true;
//                     continue;
//                 }
//                 if (in_services_section && line.find("  ") == 0 && line.find("    ") != 0) {
//                     // This is a service name (simple heuristic)
//                     std::string service_name = line.substr(2);
//                     if (service_name.back() == ':') {
//                         service_name.pop_back();
//                         project.services.push_back(service_name);
//                     }
//                 }
//                 if (in_services_section && line.find("volumes:") != std::string::npos) {
//                     break; // End of services section
//                 }
//             }
//         }
        
//         return true;
//     } catch (const std::exception& e) {
//         return false;
//     }
// }

// bool ProjectManager::removeComposeProject(const std::string& projectName) {
//     try {
//         auto it = projects_.find(projectName);
//         if (it == projects_.end()) {
//             return false;
//         }
        
//         // Stop and remove containers first
//         composeDown(projectName, true);
//         // Note: We don't erase the project from projects_ here as that's done in unloadProject
//         return true;
//     } catch (const std::exception& e) {
//         return false;
//     }
// }

std::tuple<bool, std::string> ProjectManager::composeUp(const std::string& projectName) {
    try {
        auto it = projects_.find(projectName);
        if (it == projects_.end()) {
            return {false, "Project not found: " + projectName};
        }
        
        const auto& project = it->second;
        std::vector<std::string> args = {"compose", "-f", project.compose_file_path, "-p", projectName, "up", "-d"};
        
        std::string _out;
        auto [pid, ret_code] = process_manager_->startProcessBlocking(
            "docker", 
            args, 
            {{"COMPOSE_PROJECT_NAME", projectName}}, 
            [&_out](const std::string& _str){
                _out += _str;
            },
            project.working_directory
        );
        if(0 != ret_code)
        {
            // CROW_LOG_ERROR << __FUNCTION__ << "::" << _out;
            return {false, "Failed to start project '" + projectName + "': " + _out};
        }
        return {true, "Project '" + projectName + "' started successfully: " + _out};
    } catch (const std::exception& e) {
        return {false, "Exception occurred while starting project '" + projectName + "': " + std::string(e.what())};
    }
}

std::tuple<bool, std::string> ProjectManager::composeDown(const std::string& projectName, bool removeVolumes) {
    try {
        auto it = projects_.find(projectName);
        if (it == projects_.end()) {
            return {false, "Project not found: " + projectName};
        }
        
        const auto& project = it->second;
        std::vector<std::string> args = {"compose", "-f", project.compose_file_path, "-p", projectName, "down"};
        if (removeVolumes) {
            args.push_back("-v");
        }
        
        std::string _out;
        auto [pid, ret_code] = process_manager_->startProcessBlocking(
            "docker", 
            args, 
            {{"COMPOSE_PROJECT_NAME", projectName}}, 
            [&_out](const std::string& _str){
                _out += _str;
            },
            project.working_directory
        );
        if (ret_code == 0) {
            return {true, "Project '" + projectName + "' stoped successfully: " + _out};
        } else {
            return {false, "Failed to stop project '" + projectName + "': " + _out};
        }
    } catch (const std::exception& e) {
        return {false, "Exception occurred while stopping project '" + projectName + "': " + std::string(e.what())};
    }
}

std::tuple<bool, std::string> ProjectManager::composeRestart(const std::string& projectName) {
    try {
        auto it = projects_.find(projectName);
        if (it == projects_.end()) {
            return {false, "Project not found: " + projectName};
        }
        
        const auto& project = it->second;
        std::string _out;
        auto [pid, ret_code] = process_manager_->startProcessBlocking(
            "docker", 
            {"compose", "-f", project.compose_file_path, "-p", projectName, "restart"}, 
            {{"COMPOSE_PROJECT_NAME", projectName}}, 
            [&_out](const std::string& _str){
                _out += _str;
            },
            project.working_directory
        );
        if (ret_code == 0) {
            return {true, "Project '" + projectName + "' restarted successfully: " + _out};
        } else {
            return {false, "Failed to restart project '" + projectName + "': " + _out};
        }
    } catch (const std::exception& e) {
        return {false, "Exception occurred while restarting project '" + projectName + "': " + std::string(e.what())};
    }
}

std::tuple<bool, std::string> ProjectManager::composeSatus(const std::string& projectName) {
    try {
        auto it = projects_.find(projectName);
        if (it == projects_.end()) {
            return {false, "Project not found: " + projectName};
        }
        
        const auto& project = it->second;
        std::string output;
        auto [pid, ret_code] = process_manager_->startProcessBlocking(
            "docker", 
            {"compose", "-f", project.compose_file_path, "-p", projectName, "ps", "--format", "json"}, 
            {}, 
            [&output](const std::string& line) {
                output += line;
            },
            project.working_directory
        );
        
        if (ret_code != 0) {
            return {false, "Failed to get project status: " + output};
        }
        
        // Parse JSON output line by line
        std::istringstream iss(output);
        std::string line;
        bool allRunning = true;
        int serviceCount = 0;
        
        while (std::getline(iss, line)) {
            if (line.empty()) continue;
            
            serviceCount++;
            std::string parseError;
            json11::Json service = json11::Json::parse(line, parseError);
            
            if (!parseError.empty()) {
                return {false, "Failed to parse JSON output: " + parseError};
            }
            
            std::string state = Utils::str_to_lower(service["State"].string_value());
            if (state != "running") {
                allRunning = false;
                break;
            }
        }
        
        // If no services found, return false
        if (serviceCount == 0) {
            return {false, "No services found in project"};
        }
        
        // Return result based on whether all services are running
        if (allRunning) {
            return {true, "All services are running"};
        } else {
            return {false, "Some services are not running"};
        }
    } catch (const std::exception& e) {
        return {false, "Exception occurred while checking project status: " + std::string(e.what())};
    }
}

std::tuple<bool, std::string, std::vector<std::string>> ProjectManager::composeServices(const std::string& projectName) {
    try {
        auto it = projects_.find(projectName);
        if (it != projects_.end()) {
            return {true, "Successfully retrieved services", it->second.services};
        }
        return {false, "Project not found: " + projectName, {}};
    } catch (const std::exception& e) {
        return {false, "Exception occurred while retrieving services: " + std::string(e.what()), {}};
    }
}

std::vector<std::string> ProjectManager::listImages() {
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

bool ProjectManager::extract7zArchive(const std::string &archivePath, const std::string &extractPath,
                                      const std::string &password)
{
    try
    {
        std::string sevenZipPath = Utils::get_7z_executable_path();

        // Create extraction directory
        std::filesystem::create_directories(extractPath);

        ProcessManager pm;
        std::vector<std::string> args = {
            "-o" + extractPath,
            "-y" // assume yes to all queries
        };

        // Add password if provided
        if (!password.empty())
        {
            args.push_back("-p" + password);
        }
        args.push_back("x");         // extract with full paths
        args.push_back(archivePath); // extract with full paths

        std::string output;
        auto result = pm.startProcessBlocking(
            sevenZipPath,
            args,
            {},
            [&output](const std::string &data)
            {
                output += data;
            });
        int exitCode = std::get<1>(result);

        if (exitCode == 0)
        {
            broadcastLog("extract7zArchive", "Successfully extracted archive: " + archivePath, "info");
            return true;
        }
        else
        {
            broadcastLog("extract7zArchive", "Failed to extract archive: " + output, "error");
            return false;
        }
    }
    catch (const std::exception &e)
    {
        broadcastLog("extract7zArchive", "Exception: " + std::string(e.what()), "error");
        return false;
    }
}

bool ProjectManager::create7zArchive(const std::string &sourcePath, const std::string &archivePath,
                                     const std::string &password)
{
    try
    {
        std::string sevenZipPath = Utils::get_7z_executable_path();

        ProcessManager pm;
        std::vector<std::string> args = {
            "a", // add to archive
            archivePath,
            sourcePath + "/*",
            "-mx=9" // maximum compression
        };

        if (!password.empty())
        {
            args.push_back("-p" + password);
            args.push_back("-mhe=on"); // encrypt headers
        }

        std::string output;
        auto result = pm.startProcessBlocking(
            sevenZipPath,
            args,
            {},
            [&output](const std::string &data)
            {
                output += data;
            });

        return std::get<1>(result) == 0;
    }
    catch (const std::exception &e)
    {
        broadcastLog("create7zArchive", "Exception: " + std::string(e.what()), "error");
        return false;
    }
}

bool ProjectManager::test7zArchive(const std::string &archivePath, const std::string &password)
{
    try
    {
        std::string sevenZipPath = Utils::get_7z_executable_path();

        ProcessManager pm;
        std::vector<std::string> args = {
            "t", // test archive
            archivePath};

        if (!password.empty())
        {
            args.push_back("-p" + password);
        }

        std::string output;
        auto result = pm.startProcessBlocking(
            sevenZipPath,
            args,
            {},
            [&output](const std::string &data)
            {
                output += data;
            });

        return std::get<1>(result) == 0;
    }
    catch (const std::exception &e)
    {
        return false;
    }
}

std::vector<std::string> ProjectManager::list7zContents(const std::string &archivePath, const std::string &password)
{
    std::vector<std::string> contents;

    try
    {
        std::string sevenZipPath = Utils::get_7z_executable_path();

        ProcessManager pm;
        std::vector<std::string> args = {
            "l", // list contents
            archivePath};

        if (!password.empty())
        {
            args.push_back("-p" + password);
        }

        std::string output;
        auto result = pm.startProcessBlocking(
            sevenZipPath,
            args,
            {},
            [&output](const std::string &data)
            {
                output += data;
            });

        if (std::get<1>(result) == 0)
        {
            // Parse the output to extract file names
            std::istringstream iss(output);
            std::string line;
            bool inFileList = false;

            while (std::getline(iss, line))
            {
                // Look for the file listing section
                if (!inFileList && line.find("-----") != std::string::npos)
                {
                    inFileList = true;
                    continue;
                }

                if (inFileList && line.find("-----") != std::string::npos)
                {
                    break; // End of file list
                }

                if(!inFileList || line.empty())
                {
                    continue;
                }
                // Extract filename (last part after spaces)
                size_t lastSpace = line.find_last_of(' ');
                if (lastSpace != std::string::npos && lastSpace < line.length() - 1)
                {
                    std::string filename = line.substr(lastSpace + 1);
                    if (!filename.empty() && filename != ".")
                    {
                        contents.push_back(filename);
                    }
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        broadcastLog("list7zContents", "Exception: " + std::string(e.what()), "error");
    }

    return contents;
}

ProjectArchiveInfo ProjectManager::analyzeArchive(const std::string &archivePath, const std::string &password)
{
    ProjectArchiveInfo info;
    info.archive_path = archivePath;

    try
    {
        // Check if file exists
        if (!std::filesystem::exists(archivePath))
        {
            info.error_message = "Archive file does not exist";
            return info;
        }

        // Test archive integrity
        info.integrity_verified = test7zArchive(archivePath, password);
        if (!info.integrity_verified)
        {
            info.error_message = "Archive integrity check failed or wrong password";
            return info;
        }

        // List contents
        info.contained_files = list7zContents(archivePath, password);

        info.is_encrypted = !test7zArchive(archivePath, "");

        // Look for docker-compose.yml
        bool hasComposeFile = false;
        for (const auto &file : info.contained_files)
        {
            if (file == "docker-compose.yml" || file == "docker-compose.yaml")
            {
                hasComposeFile = true;
                break;
            }
            // Look for Docker image tar files - C++17 compatible check
            if (file.length() >= 4 && file.substr(file.length() - 4) == ".tar")
            {
                info.docker_images.push_back(file);
            } /*  else if (file.length() >= 7 && file.substr(file.length() - 7) == ".tar.gz") {
                 info.docker_images.push_back(file);
             } */
        }

        if (!hasComposeFile)
        {
            info.error_message = "No docker-compose.yml file found in archive";
        }

        broadcastLog("analyzeArchive", "Archive analyzed: " + archivePath + " (files: " + std::to_string(info.contained_files.size()) + ", images: " + std::to_string(info.docker_images.size()) + ")", "info");
    }
    catch (const std::exception &e)
    {
        info.error_message = "Exception during analysis: " + std::string(e.what());
        broadcastLog("analyzeArchive", info.error_message, "error");
    }

    return info;
}

// bool ProjectManager::validateArchiveIntegrity(const std::string &archivePath, const std::string &password)
// {
//     return test7zArchive(archivePath, password);
// }

bool ProjectManager::extractArchive(const std::string &archivePath, const std::string &extractPath, const std::string &password, std::function<void(const ProjectOperationProgress &)> progressCallback)
{

    ProjectOperationProgress progress;
    progress.status = ProjectStatus::EXTRACTING;
    progress.percentage = 0;
    progress.message = "Starting extraction...";
    progress.current_operation = "extract";

    if (progressCallback)
    {
        progressCallback(progress);
    }

    try
    {
        // Validate archive first
        progress.percentage = 10;
        progress.message = "Validating archive integrity...";
        if (progressCallback)
            progressCallback(progress);

        // if (!validateArchiveIntegrity(archivePath, password))
        // {
        //     progress.status = ProjectStatus::ERROR;
        //     progress.error_details = "Archive integrity validation failed";
        //     if (progressCallback)
        //         progressCallback(progress);
        //     return false;
        // }

        // Extract archive
        progress.percentage = 30;
        progress.message = "Extracting archive...";
        if (progressCallback)
            progressCallback(progress);

        bool success = extract7zArchive(archivePath, extractPath, password);

        if (success)
        {
            progress.status = ProjectStatus::READY;
            progress.percentage = 100;
            progress.message = "Extraction completed successfully";
        }
        else
        {
            progress.status = ProjectStatus::ERROR;
            progress.error_details = "Failed to extract archive";
        }

        if (progressCallback)
        {
            progressCallback(progress);
        }

        return success;
    }
    catch (const std::exception &e)
    {
        progress.status = ProjectStatus::ERROR;
        progress.error_details = "Exception during extraction: " + std::string(e.what());
        if (progressCallback)
        {
            progressCallback(progress);
        }
        return false;
    }
}

// std::vector<std::string> ProjectManager::parseDockerComposeImages(const std::string &composeContent)
// {
//     std::vector<std::string> images;

//     try
//     {
//         // Simple regex-based parsing for image names
//         std::regex imageRegex(R"(image:\s*['"]*([^'"\s\n]+)['"]*)", std::regex_constants::icase);
//         std::sregex_iterator iter(composeContent.begin(), composeContent.end(), imageRegex);
//         std::sregex_iterator end;

//         for (; iter != end; ++iter)
//         {
//             std::string image = (*iter)[1].str();
//             if (!image.empty())
//             {
//                 images.push_back(image);
//             }
//         }
//     }
//     catch (const std::exception &e)
//     {
//         broadcastLog("parseDockerComposeImages", "Exception: " + std::string(e.what()), "error");
//     }

//     return images;
// }

std::vector<std::string> ProjectManager::parseDockerComposeDependentFiles(const std::string &composeContent, const std::string &basePath)
{
    std::vector<std::string> files;

    try
    {
        auto root = fkyaml::node::deserialize(composeContent);

        // Check services section
        if (root.contains("services"))
        {
            auto services = root.at("services");
            for (const auto &[service_name, service_config] : services.as_map())
            {
                // Check build context
                if (service_config.contains("build"))
                {
                    if (service_config.at("build").is_string())
                    {
                        files.push_back(service_config.at("build").as_str());
                    }
                    else if (service_config.at("build").is_mapping())
                    {
                        auto build = service_config.at("build");
                        if (build.contains("context"))
                        {
                            files.push_back(build.at("context").as_str());
                        }
                        if (build.contains("dockerfile"))
                        {
                            files.push_back(build.at("dockerfile").as_str());
                        }
                    }
                }

                // Check env_file
                if (service_config.contains("env_file"))
                {
                    if (service_config.at("env_file").is_string())
                    {
                        files.push_back(service_config.at("env_file").as_str());
                    }
                    else if (service_config.at("env_file").is_sequence())
                    {
                        for (const auto &env_file : service_config.at("env_file").as_seq())
                        {
                            files.push_back(env_file.as_str());
                        }
                    }
                }

                // Check volumes
                if (service_config.contains("volumes"))
                {
                    for (const auto &volume : service_config.at("volumes").as_seq())
                    {
                        if (volume.is_string())
                        {
                            std::string vol = volume.as_str();
                            size_t colonPos = vol.find(':');
                            if (colonPos != std::string::npos)
                            {
                                std::string hostPath = vol.substr(0, colonPos);
                                if (!hostPath.empty() && hostPath[0] != '/' && hostPath[0] != '$')
                                {
                                    files.push_back(hostPath);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        broadcastLog("parseDockerComposeDependentFiles", "Exception: " + std::string(e.what()), "error");
    }

    return files;
}

std::tuple<bool, std::vector<std::string>> ProjectManager::validateDockerComposeFile(const std::string &composeFilePath, std::vector<std::string> &_list_images)
{
    std::vector<std::string> _list_services;
    try
    {
        if (!std::filesystem::exists(composeFilePath))
        {
            return {false, _list_services};
        }

        std::ifstream file(composeFilePath);
        if (!file.is_open())
        {
            return {false, _list_services};
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        auto _node = fkyaml::node::deserialize(content);
        // auto _contain_services = _node.contains("services");
        auto _node_services = _node["services"];
        // auto _is_null_dummy = _dummy.is_null();
        if (!_node_services.is_mapping())
        {
            // auto _type = _service.get_type();
            return {false, _list_services};
        }
        auto _services = _node_services.as_map();
        if (_services.size() < 1)
        {
            broadcastLog("validateDockerComposeFile", "no service found in docker file", "error");
            return {false, _list_services};
        }
        std::string _str_services;
        std::string _str_images;
        for (const auto &_service : _services)
        {
            // first is the service name and second is the node
            if(fkyaml::node_type::STRING != _service.first.get_type())
            {
                broadcastLog("validateDockerComposeFile", "invalid node type service in " + composeFilePath, "error");
                return {false, _list_services};
            }
            _list_services.push_back(_service.first.as_str());
            _str_services += _service.first.as_str();
            _str_services += ",";

            auto _node_image = _service.second["image"];
            if (!_node_image.is_string())
            {
                broadcastLog("validateDockerComposeFile", "invalid node type image in " + composeFilePath, "error");
                return {false, _list_services};
            }
            auto _str_image = _node_image.as_str();
            if (_str_image.empty())
            {
                broadcastLog("validateDockerComposeFile", "invalid node type image name in " + composeFilePath, "error");
                return {false, _list_services};
            }
            _list_images.push_back(_str_image);
            _str_images += _str_image;
            _str_images += ",";
        }
        _str_services.pop_back();
        _str_images.pop_back();
        broadcastLog("validateDockerComposeFile", "list docker-compose service = '" + _str_services + "'", "info");
        broadcastLog("validateDockerComposeFile", "list images = '" + _str_images + "'", "info");

        // Basic validation - check for required fields
        bool _ret = (_services.size() > 0 && _list_images.size() > 0);
        return {_ret, _list_services};
    }
    catch (const std::exception &e)
    {
        broadcastLog("validateDockerComposeFile", "Exception: " + std::string(e.what()), "error");
        return {false, _list_services};
    }
}

std::vector<std::string> ProjectManager::findDockerImageFiles(const std::string &projectPath)
{
    std::vector<std::string> imageFiles;

    try
    {
        for (const auto &entry : std::filesystem::recursive_directory_iterator(projectPath))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }
            std::string filename = entry.path().filename().string();
            // Check for Docker image tar files
            if (filename.length() >= 4 && filename.substr(filename.length() - 4) == ".tar")
            {
                imageFiles.push_back(entry.path().string());
            } /*  else if (filename.length() >= 7 && filename.substr(filename.length() - 7) == ".tar.gz") {
                  imageFiles.push_back(entry.path().string());
             } */
        }
    }
    catch (const std::exception &e)
    {
        broadcastLog("findDockerImageFiles", "Exception: " + std::string(e.what()), "error");
    }

    return imageFiles;
}

bool ProjectManager::loadDockerImagesFromProject(const std::string &projectPath)
{
    try
    {
        std::vector<std::string> imageFiles = findDockerImageFiles(projectPath);

        if (imageFiles.empty())
        {
            broadcastLog("loadDockerImagesFromProject", "No Docker image files found in project", "warning");
            return true; // Not an error if no images to load
        }

        bool allLoaded = true;
        for (const auto &imageFile : imageFiles)
        {
            broadcastLog("loadDockerImagesFromProject", "Loading Docker image: " + imageFile, "info");

            if (!loadImageFromFile(imageFile))
            {
                broadcastLog("loadDockerImagesFromProject", "Failed to load image: " + imageFile, "error");
                allLoaded = false;
            }
            else
            {
                broadcastLog("loadDockerImagesFromProject", "Successfully loaded image: " + imageFile, "info");
            }
        }

        return allLoaded;
    }
    catch (const std::exception &e)
    {
        broadcastLog("loadDockerImagesFromProject", "Exception: " + std::string(e.what()), "error");
        return false;
    }
}

std::string ProjectManager::createProjectDirectory(const std::string &projectName)
{
    std::string projectPath = Utils::path_join_multiple({projects_directory_, projectName});

    try
    {
        std::filesystem::create_directories(projectPath);
        return projectPath;
    }
    catch (const std::exception &e)
    {
        broadcastLog("createProjectDirectory", "Exception: " + std::string(e.what()), "error");
        return "";
    }
}

std::string ProjectManager::getProjectPath(const std::string &projectName)
{
    return Utils::path_join_multiple({projects_directory_, projectName});
}

bool ProjectManager::cleanupProjectDirectory(const std::string &projectName)
{
    try
    {
        std::string projectPath = getProjectPath(projectName);
        if (std::filesystem::exists(projectPath))
        {
            EnvParser _env;
            _env.load_env_file(CONST_FILE_ENV_SECRET_PRIVATE);
            auto _password = _env.get(CONST_KEY_SUDO_PSWD);
            ProcessManager _pm;
            const auto [_pid, _success] = _pm.startProcessBlockingAsRoot(
                "rm",
                {
                    "-rf",
                    projectPath,
                },
                {},
                _password
            );
            // std::filesystem::remove_all(projectPath);
            return (0 == _success);
        }
        return true; // Already clean
    }
    catch (const std::exception &e)
    {
        broadcastLog("cleanupProjectDirectory", "Exception: " + std::string(e.what()), "error");
        return false;
    }
}

bool ProjectManager::loadProject(const std::string &archivePath, const std::string &projectName, const std::string &password, std::function<void(const ProjectOperationProgress &)> progressCallback2)
{
    ProjectOperationProgress progress;
    progress.status = ProjectStatus::NOT_LOADED;
    progress.percentage = 0;
    progress.message = "Starting project load...";
    progress.current_operation = "load";

    auto progressCallback = [&progressCallback2](const ProjectOperationProgress &_p) -> void
    {
        if (progressCallback2)
        {
            progressCallback2(_p);
        }
    };

    try
    {
        // Check if project already exists
        if (projects_.find(projectName) != projects_.end())
        {
            progress.status = ProjectStatus::ERROR;
            progress.error_details = "Project with name '" + projectName + "' already exists";
            progressCallback(progress);
            return false;
        }

        // Analyze archive first
        progress.percentage = 5;
        progress.message = "Analyzing archive...";
        progressCallback(progress);

        ProjectArchiveInfo archiveInfo = analyzeArchive(archivePath, password);
        if (!archiveInfo.error_message.empty())
        {
            progress.status = ProjectStatus::ERROR;
            progress.error_details = archiveInfo.error_message;
            progressCallback(progress);
            return false;
        }

        // Create project directory
        progress.percentage = 10;
        progress.message = "Creating project directory...";
        progressCallback(progress);

        std::string projectPath = createProjectDirectory(projectName);
        if (projectPath.empty())
        {
            progress.status = ProjectStatus::ERROR;
            progress.error_details = "Failed to create project directory";
            progressCallback(progress);
            return false;
        }

        // Extract archive
        progress.percentage = 20;
        progress.message = "Extracting archive...";
        progress.status = ProjectStatus::EXTRACTING;
        progressCallback(progress);

        bool extracted = extractArchive(
            archivePath,
            projectPath,
            password,
            [&progress, progressCallback](const ProjectOperationProgress &extractProgress){
            progress.percentage = 20 + (extractProgress.percentage * 0.4); // 20-60%
            progress.message = extractProgress.message;
            progressCallback(progress);
        });

        if (!extracted)
        {
            cleanupProjectDirectory(projectName);
            progress.status = ProjectStatus::ERROR;
            progress.error_details = "Failed to extract archive";
            progressCallback(progress);
            return false;
        }

        // Find and validate docker-compose.yml
        progress.percentage = 65;
        progress.message = "Validating Docker Compose file...";
        progress.status = ProjectStatus::VALIDATING;
        progressCallback(progress);

        std::string composeFilePath = Utils::path_join_multiple({projectPath, "docker-compose.yml"});
        if (!std::filesystem::exists(composeFilePath))
        {
            composeFilePath = Utils::path_join_multiple({projectPath, "docker-compose.yaml"});
        }

        std::vector<std::string> _list_images;
        auto [_valid_compose, _list_services] = validateDockerComposeFile(composeFilePath, _list_images);
        if (!_valid_compose)
        {
            cleanupProjectDirectory(projectName);
            progress.status = ProjectStatus::ERROR;
            progress.error_details = "Invalid or missing docker-compose.yml file";
            progressCallback(progress);
            return false;
        }

        // Load Docker images
        progress.percentage = 70;
        progress.message = "Loading Docker images...";
        progress.status = ProjectStatus::LOADING_IMAGES;
        progressCallback(progress);

        if (!loadDockerImagesFromProject(projectPath))
        {
            broadcastLog("loadProject", "Warning: Some Docker images failed to load", "warning");
        }

        // Read compose file content for analysis
        std::ifstream composeFile(composeFilePath);
        std::string composeContent((std::istreambuf_iterator<char>(composeFile)), std::istreambuf_iterator<char>());
        composeFile.close();

        // Create project info
        ProjectInfo projectInfo;
        projectInfo.name = projectName;
        projectInfo.archive_path = archivePath;
        projectInfo.extracted_path = projectPath;
        projectInfo.compose_file_path = composeFilePath;
        projectInfo.required_images = _list_images; // parseDockerComposeImages(composeContent);
        projectInfo.services = _list_services; 
        projectInfo.working_directory = projectPath; 
        projectInfo.dependent_files = parseDockerComposeDependentFiles(composeContent, projectPath);
        projectInfo.is_loaded = true;
        // projectInfo.is_running = false;
        projectInfo.status_message = "Project loaded successfully";

        // Set timestamps
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        projectInfo.created_time = ss.str();
        projectInfo.last_modified = ss.str();

        // Store project
        projects_[projectName] = projectInfo;

        // Save to database
        if (!database_->saveProjectsToDatabase(projects_)) {
            broadcastLog("loadProject", "Warning: Failed to save project to database", "warning");
        }

        // Load compose project
        progress.percentage = 90;
        progress.message = "Registering with Docker Compose...";
        progressCallback(progress);

        // loadComposeFile(composeFilePath, projectName, projectPath);

        // Complete
        progress.status = ProjectStatus::READY;
        progress.percentage = 100;
        progress.message = "Project loaded successfully";
        progressCallback(progress);

        broadcastLog("loadProject", "Project '" + projectName + "' loaded successfully", "info");
        return true;
    }
    catch (const std::exception &e)
    {
        cleanupProjectDirectory(projectName);
        progress.status = ProjectStatus::ERROR;
        progress.error_details = "Exception during project load: " + std::string(e.what());
        progressCallback(progress);
        broadcastLog("loadProject", progress.error_details, "error");
        return false;
    }
}

bool ProjectManager::unloadProject(const std::string &projectName)
{
    try
    {
        auto it = projects_.find(projectName);
        if (it == projects_.end())
        {
            broadcastLog("unloadProject", "Project not found: " + projectName, "warning");
            return false;
        }

        // Stop project if running
        // if (it->second.is_running)
        // {
            // }
        stopProject(projectName);

        // Remove from Docker Compose projects
        // removeComposeProject(projectName);

        // Remove from our tracking
        projects_.erase(it);

        // Save to database
        if (!database_->saveProjectsToDatabase(projects_)) {
            broadcastLog("unloadProject", "Warning: Failed to save projects to database", "warning");
        }

        broadcastLog("unloadProject", "Project unloaded: " + projectName, "info");
        return true;
    }
    catch (const std::exception &e)
    {
        broadcastLog("unloadProject", "Exception: " + std::string(e.what()), "error");
        return false;
    }
}

bool ProjectManager::removeProject(const std::string &projectName, bool removeFiles)
{
    try
    {
        // Unload first
        unloadProject(projectName);

        // Remove files if requested
        if (removeFiles)
        {
            cleanupProjectDirectory(projectName);
        }

        broadcastLog("removeProject", "Project removed: " + projectName, "info");
        
        // Save to database
        if (!database_->saveProjectsToDatabase(projects_)) {
            broadcastLog("removeProject", "Warning: Failed to save projects to database", "warning");
        }
        
        return true;
    }
    catch (const std::exception &e)
    {
        broadcastLog("removeProject", "Exception: " + std::string(e.what()), "error");
        return false;
    }
}

bool ProjectManager::startProject(const std::string &projectName)
{
    try
    {
        auto it = projects_.find(projectName);
        if (it == projects_.end())
        {
            broadcastLog("startProject", "Project not found: " + projectName, "error");
            return false;
        }

        // if (it->second.is_running)
        // {
        //     broadcastLog("startProject", "Project already running: " + projectName, "warning");
        //     return true;
        // }

        // Start using embedded Docker Compose implementation
        auto [success, message] = composeUp(projectName);

        if (success)
        {
            // it->second.is_running = true;
            it->second.status_message = "Project is running";
            broadcastLog("startProject", "Project started: '" + projectName + "' " + message, "success");
        }
        else
        {
            broadcastLog("startProject", "Failed to start project: '" + projectName + "': " + message, "error");
        }

        return success;
    }
    catch (const std::exception &e)
    {
        broadcastLog("startProject", "Exception: " + std::string(e.what()), "error");
        return false;
    }
}

bool ProjectManager::stopProject(const std::string &projectName)
{
    try
    {
        auto it = projects_.find(projectName);
        if (it == projects_.end())
        {
            broadcastLog("stopProject", "Project not found: " + projectName, "error");
            return false;
        }

        // if (!it->second.is_running)
        // {
        //     broadcastLog("stopProject", "Project not running: " + projectName, "warning");
        //     return true;
        // }

        // Stop using embedded Docker Compose implementation
        auto [success, message] = composeDown(projectName, false);

        if (success)
        {
            // it->second.is_running = false;
            it->second.status_message = "Project is stopped";
            broadcastLog("stopProject", "Project stopped: '" + projectName + "' " + message, "info");
        }
        else
        {
            broadcastLog("stopProject", "Failed to stop project: '" + projectName + "': " + message, "error");
        }

        return success;
    }
    catch (const std::exception &e)
    {
        broadcastLog("stopProject", "Exception: " + std::string(e.what()), "error");
        return false;
    }
}

bool ProjectManager::restartProject(const std::string &projectName)
{
    try
    {
        auto it = projects_.find(projectName);
        if (it == projects_.end())
        {
            broadcastLog("restartProject", "Project not found: " + projectName, "error");
            return false;
        }

        // Use embedded Docker Compose implementation to restart
        auto [success, message] = composeRestart(projectName);

        if (success)
        {
            // it->second.is_running = true;
            it->second.status_message = "Project restarted";
            broadcastLog("restartProject", "Project restarted: '" + projectName + "': " + message, "info");
        }
        else
        {
            broadcastLog("restartProject", "Failed to restart project: '" + projectName + "': " + message, "error");
        }

        return success;
    }
    catch (const std::exception &e)
    {
        broadcastLog("restartProject", "Exception: " + std::string(e.what()), "error");
        return false;
    }
}

ProjectStatus ProjectManager::getProjectStatus(const std::string &projectName)
{
    auto it = projects_.find(projectName);
    if (it == projects_.end())
    {
        return ProjectStatus::NOT_LOADED;
    }

    if (it->second.is_running)
    {
        return ProjectStatus::RUNNING;
    }
    else if(it->second.is_loaded)
    {
        return ProjectStatus::READY;
    }
    else
    {
        return ProjectStatus::STOPPED;
    }
}

std::vector<ProjectInfo> ProjectManager::listProjects()
{
    std::vector<ProjectInfo> projectList;
    for (auto &pair : projects_)
    {
        auto [_running, _msg] = composeSatus(pair.first);
        pair.second.is_running = _running;
        projectList.push_back(pair.second);
    }
    return projectList;
}

ProjectInfo ProjectManager::getProjectInfo(const std::string &projectName)
{
    auto it = projects_.find(projectName);
    if (it != projects_.end())
    {
        return it->second;
    }

    // Return empty project info if not found
    ProjectInfo empty;
    empty.name = projectName;
    empty.status_message = "Project not found";
    return empty;
}

std::vector<std::string> ProjectManager::getProjectServices(const std::string &projectName)
{
    try
    {
        auto [success, message, services] = composeServices(projectName);
        if (success) {
            return services;
        } else {
            broadcastLog("getProjectServices", message, "error");
            return {};
        }
    }
    catch (const std::exception &e)
    {
        broadcastLog("getProjectServices", "Exception: " + std::string(e.what()), "error");
        return {};
    }
}

std::string ProjectManager::getProjectLogs(const std::string &projectName, const std::string &serviceName)
{
    try
    {
        auto it = projects_.find(projectName);
        if (it == projects_.end())
        {
            broadcastLog("getProjectLogs", "Project not found: " + projectName, "error");
            return "Error: Project not found: " + projectName;
        }

        const auto& project = it->second;
        
        // Build docker-compose logs command
        std::vector<std::string> args = {"compose", "-f", project.compose_file_path, "-p", projectName, "logs"};
        
        // Add service name if specified
        if (!serviceName.empty())
        {
            args.push_back(serviceName);
        }
        
        // Add follow flag to limit output (otherwise logs can be extremely large)
        args.push_back("--tail");
        args.push_back("200"); // Get last 200 lines
        
        std::string logs;
        auto [pid, ret_code] = process_manager_->startProcessBlocking(
            "docker", 
            args, 
            {{"COMPOSE_PROJECT_NAME", projectName}}, 
            [&logs](const std::string& line) {
                logs += line + "\n";
            },
            project.working_directory
        );
        
        if (ret_code == 0)
        {
            broadcastLog("getProjectLogs", "Successfully retrieved logs for project: " + projectName + 
                         (serviceName.empty() ? "" : ", service: " + serviceName), "info");
            return logs;
        }
        else
        {
            broadcastLog("getProjectLogs", "Failed to retrieve logs for project: " + projectName + 
                         (serviceName.empty() ? "" : ", service: " + serviceName), "error");
            return "Error retrieving logs for project: " + projectName + 
                   (serviceName.empty() ? "" : ", service: " + serviceName);
        }
    }
    catch (const std::exception &e)
    {
        broadcastLog("getProjectLogs", "Exception: " + std::string(e.what()), "error");
        return "Error retrieving logs: " + std::string(e.what());
    }
}

bool ProjectManager::validateRequiredImages(const std::string &projectName)
{
    try
    {
        auto it = projects_.find(projectName);
        if (it == projects_.end())
        {
            return false;
        }

        const auto &requiredImages = it->second.required_images;
        auto availableImages = listImages();

        for (const auto &requiredImage : requiredImages)
        {
            bool found = false;
            auto _iter_found = std::find(availableImages.begin(), availableImages.end(), requiredImage);
            found = (_iter_found != availableImages.end());
            // for (const auto &availableImage : availableImages)
            // {
            //     if (availableImage.find(requiredImage) != std::string::npos)
            //     {
            //         found = true;
            //         break;
            //     }
            // }
            if (!found)
            {
                broadcastLog("validateRequiredImages", "Missing required image: " + requiredImage, "error");
                return false;
            }
        }

        return true;
    }
    catch (const std::exception &e)
    {
        broadcastLog("validateRequiredImages", "Exception: " + std::string(e.what()), "error");
        return false;
    }
}

bool ProjectManager::createProjectArchive(const std::string &projectPath, const std::string &archivePath,
                                          const std::string &password,
                                          std::function<void(const ProjectOperationProgress &)> progressCallback)
{

    ProjectOperationProgress progress;
    progress.status = ProjectStatus::NOT_LOADED;
    progress.percentage = 0;
    progress.message = "Starting archive creation...";
    progress.current_operation = "create_archive";

    if (progressCallback)
    {
        progressCallback(progress);
    }

    try
    {
        // Validate source path
        if (!fs::exists(projectPath))
        {
            progress.status = ProjectStatus::ERROR;
            progress.error_details = "Source project path does not exist";
            if (progressCallback)
                progressCallback(progress);
            return false;
        }

        progress.percentage = 20;
        progress.message = "Creating archive...";
        if (progressCallback)
            progressCallback(progress);

        bool success = create7zArchive(projectPath, archivePath, password);

        if (success)
        {
            progress.percentage = 100;
            progress.message = "Archive created successfully";
            progress.status = ProjectStatus::READY;
        }
        else
        {
            progress.status = ProjectStatus::ERROR;
            progress.error_details = "Failed to create archive";
        }

        if (progressCallback)
        {
            progressCallback(progress);
        }

        return success;
    }
    catch (const std::exception &e)
    {
        progress.status = ProjectStatus::ERROR;
        progress.error_details = "Exception during archive creation: " + std::string(e.what());
        if (progressCallback)
        {
            progressCallback(progress);
        }
        return false;
    }
}

void ProjectManager::setProjectsDirectory(const std::string &directory)
{
    projects_directory_ = directory;
    fs::create_directories(projects_directory_);
}

void ProjectManager::setWebSocketConnections(std::vector<crow::websocket::connection *> *log_connections,
                                             std::vector<crow::websocket::connection *> *progress_connections)
{
    log_connections_ = log_connections;
    progress_connections_ = progress_connections;
}

void ProjectManager::broadcastLog(const std::string &operation, const std::string &message, const std::string &level)
{
    // if (!log_connections_)
    //     return;

    // try
    // {
    //     json11::Json logMessage = json11::Json::object{
    //         {"type", "log"},
    //         {"operation", operation},
    //         {"message", message},
    //         {"level", level},
    //         {"timestamp", std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
    //                                          std::chrono::system_clock::now().time_since_epoch())
    //                                          .count())}};

    //     std::string jsonStr = logMessage.dump();

    //     for (auto *conn : *log_connections_)
    //     {
    //         if (conn)
    //         {
    //             conn->send_text(jsonStr);
    //         }
    //     }
    // }
    // catch (const std::exception &e)
    // {
    //     // Avoid infinite recursion by not calling broadcastLog here
    //     std::cerr << "Error broadcasting log: " << e.what() << std::endl;
    // }


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

void ProjectManager::broadcastProgress(const ProjectOperationProgress &progress)
{
    if (!progress_connections_)
        return;

    try
    {
        json11::Json progressMessage = json11::Json::object{
            {"type", "progress"},
            {"status", projectStatusToString(progress.status)},
            {"percentage", progress.percentage},
            {"message", progress.message},
            {"operation", progress.current_operation},
            {"error", progress.error_details},
            {"timestamp", std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                                             std::chrono::system_clock::now().time_since_epoch())
                                             .count())}};

        std::string jsonStr = progressMessage.dump();

        for (auto *conn : *progress_connections_)
        {
            if (conn)
            {
                conn->send_text(jsonStr);
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error broadcasting progress: " << e.what() << std::endl;
    }
}

void ProjectManager::updateProgress(const std::string &projectName, ProjectStatus status, int percentage,
                                    const std::string &message, const std::string &operation,
                                    const std::string &error)
{
    ProjectOperationProgress progress;
    progress.status = status;
    progress.percentage = percentage;
    progress.message = message;
    progress.current_operation = operation;
    progress.error_details = error;

    project_progress_[projectName] = progress;
    broadcastProgress(progress);
}

std::string ProjectManager::projectStatusToString(ProjectStatus status)
{
    switch (status)
    {
    case ProjectStatus::NOT_LOADED:
        return "not_loaded";
    case ProjectStatus::EXTRACTING:
        return "extracting";
    case ProjectStatus::LOADING_IMAGES:
        return "loading_images";
    case ProjectStatus::VALIDATING:
        return "validating";
    case ProjectStatus::READY:
        return "ready";
    case ProjectStatus::RUNNING:
        return "running";
    case ProjectStatus::STOPPED:
        return "stopped";
    case ProjectStatus::ERROR:
        return "error";
    default:
        return "unknown";
    }
}

ProjectStatus ProjectManager::stringToProjectStatus(const std::string &status)
{
    if (status == "not_loaded")
        return ProjectStatus::NOT_LOADED;
    if (status == "extracting")
        return ProjectStatus::EXTRACTING;
    if (status == "loading_images")
        return ProjectStatus::LOADING_IMAGES;
    if (status == "validating")
        return ProjectStatus::VALIDATING;
    if (status == "ready")
        return ProjectStatus::READY;
    if (status == "running")
        return ProjectStatus::RUNNING;
    if (status == "stopped")
        return ProjectStatus::STOPPED;
    if (status == "error")
        return ProjectStatus::ERROR;
    return ProjectStatus::NOT_LOADED;
}

std::string ProjectManager::generateProjectId()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);

    std::string id;
    for (int i = 0; i < 8; ++i)
    {
        int val = dis(gen);
        id += (val < 10) ? ('0' + val) : ('a' + val - 10);
    }
    return id;
}

bool ProjectManager::isValidProjectName(const std::string &name)
{
    if (name.empty() || name.length() > 64)
    {
        return false;
    }

    // Check for valid characters (alphanumeric, dash, underscore)
    for (char c : name)
    {
        if (!std::isalnum(c) && c != '-' && c != '_')
        {
            return false;
        }
    }

    return true;
}

// REST API endpoint handlers
void ProjectManager::registerRestEndpoints(crow::SimpleApp &app)
{
    // Analyze archive endpoint
    CROW_ROUTE(app, "/api/projects/analyze").methods("POST"_method)([this](const crow::request &req)
    {
        return handleAnalyzeArchive(req);
    });

    // Load project endpoint
    CROW_ROUTE(app, "/api/projects/load").methods("POST"_method)([this](const crow::request &req)
    {
        return handleLoadProject(req);
    });

    // Unload project endpoint
    CROW_ROUTE(app, "/api/projects/<string>/unload").methods("POST"_method)([this](const std::string &projectName)
    {
        return handleUnloadProject(projectName);
    });

    // Remove project endpoint
    CROW_ROUTE(app, "/api/projects/<string>/remove").methods("DELETE"_method)([this](const crow::request &req, const std::string &projectName)
    {
        return handleRemoveProject(projectName, req);
    });

    // Start project endpoint
    CROW_ROUTE(app, "/api/projects/<string>/start").methods("POST"_method)([this](const std::string &projectName)
    {
        return handleStartProject(projectName);
    });

    // Stop project endpoint
    CROW_ROUTE(app, "/api/projects/<string>/stop").methods("POST"_method)([this](const std::string &projectName)
    {
        return handleStopProject(projectName);
    });

    // Restart project endpoint
    CROW_ROUTE(app, "/api/projects/<string>/restart").methods("POST"_method)([this](const std::string &projectName)
    {
        return handleRestartProject(projectName);
    });

    // List projects endpoint
    CROW_ROUTE(app, "/api/projects").methods("GET"_method)([this]()
    {
        return handleListProjects();
    });

    // Get project info endpoint
    CROW_ROUTE(app, "/api/projects/<string>").methods("GET"_method)([this](const std::string &projectName)
    {
        return handleGetProjectInfo(projectName);
    });

    // Get project services endpoint
    CROW_ROUTE(app, "/api/projects/<string>/services").methods("GET"_method)([this](const std::string &projectName)
    {
        return handleGetProjectServices(projectName);
    });

    // Get project logs endpoint
    CROW_ROUTE(app, "/api/projects/<string>/logs").methods("GET"_method)([this](const crow::request &req, const std::string &projectName)
    {
        return handleGetProjectLogs(projectName, req);
    });

    // Create project archive endpoint
    CROW_ROUTE(app, "/api/projects/create-archive").methods("POST"_method)([this](const crow::request &req)
    {
        return handleCreateProjectArchive(req);
    });

    // Get project status endpoint
    CROW_ROUTE(app, "/api/projects/<string>/status").methods("GET"_method)([this](const std::string &projectName)
    {
        return handleGetProjectStatus(projectName);
    });

    // Save browsing directory endpoint
    CROW_ROUTE(app, "/api/settings/browsing-directory").methods("POST"_method)([this](const crow::request &req)
    {
        return handleSaveBrowsingDirectory(req);
    });

    // Get browsing directory endpoint
    CROW_ROUTE(app, "/api/settings/browsing-directory").methods("GET"_method)([this]()
    {
        return handleGetBrowsingDirectory();
    });
}

crow::response ProjectManager::handleAnalyzeArchive(const crow::request &req)
{
    try
    {
        std::string parseError;
        auto json = json11::Json::parse(req.body, parseError);
        if (!json["archive_path"].is_string())
        {
            crow::response res(400, "Missing or invalid archive_path");
            res.set_header("Content-Type", "application/json");
            return res;
        }

        std::string archivePath = json["archive_path"].string_value();
        std::string password = json["password"].string_value();

        ProjectArchiveInfo info = analyzeArchive(archivePath, password);

        json11::Json response = json11::Json::object{
            {"success", info.error_message.empty()},
            {"archive_path", info.archive_path},
            {"is_encrypted", info.is_encrypted},
            {"integrity_verified", info.integrity_verified},
            {"contained_files", json11::Json::array(info.contained_files.begin(), info.contained_files.end())},
            {"docker_images", json11::Json::array(info.docker_images.begin(), info.docker_images.end())},
            {"error_message", info.error_message}};

        crow::response res(200, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
    catch (const std::exception &e)
    {
        json11::Json error = json11::Json::object{
            {"success", false},
            {"error", std::string(e.what())}};
        crow::response res(500, error.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}

crow::response ProjectManager::handleLoadProject(const crow::request &req)
{
    try
    {
        std::string parseError;
        auto json = json11::Json::parse(req.body, parseError);
        if (!json["archive_path"].is_string() || !json["project_name"].is_string())
        {
            crow::response res(400, "Missing archive_path or project_name");
            res.set_header("Content-Type", "application/json");
            return res;
        }

        std::string archivePath = json["archive_path"].string_value();
        std::string projectName = json["project_name"].string_value();
        std::string password = json["password"].string_value();

        if (!isValidProjectName(projectName))
        {
            crow::response res(400, "Invalid project name");
            res.set_header("Content-Type", "application/json");
            return res;
        }

        bool success = loadProject(archivePath, projectName, password,
                                   [this](const ProjectOperationProgress &progress)
                                   {
                                       broadcastProgress(progress);
                                   });

        json11::Json response = json11::Json::object{
            {"success", success},
            {"project_name", projectName}};

        crow::response res(success ? 200 : 500, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
    catch (const std::exception &e)
    {
        json11::Json error = json11::Json::object{
            {"success", false},
            {"error", std::string(e.what())}};
        crow::response res(500, error.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}

crow::response ProjectManager::handleUnloadProject(const std::string &projectName)
{
    try
    {
        bool success = unloadProject(projectName);

        json11::Json response = json11::Json::object{
            {"success", success},
            {"project_name", projectName}};

        crow::response res(success ? 200 : 404, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
    catch (const std::exception &e)
    {
        json11::Json error = json11::Json::object{
            {"success", false},
            {"error", std::string(e.what())}};
        crow::response res(500, error.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}

crow::response ProjectManager::handleRemoveProject(const std::string &projectName, const crow::request &req)
{
    try
    {
        std::string parseError;
        auto json = json11::Json::parse(req.body, parseError);
        bool removeFiles = json["remove_files"].bool_value();

        bool success = removeProject(projectName, removeFiles);

        json11::Json response = json11::Json::object{
            {"success", success},
            {"project_name", projectName}};

        crow::response res(success ? 200 : 404, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
    catch (const std::exception &e)
    {
        json11::Json error = json11::Json::object{
            {"success", false},
            {"error", std::string(e.what())}};
        crow::response res(500, error.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}

crow::response ProjectManager::handleStartProject(const std::string &projectName)
{
    try
    {
        bool success = startProject(projectName);

        json11::Json response = json11::Json::object{
            {"success", success},
            {"project_name", projectName}};

        crow::response res(success ? 200 : 500, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
    catch (const std::exception &e)
    {
        json11::Json error = json11::Json::object{
            {"success", false},
            {"error", std::string(e.what())}};
        crow::response res(500, error.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}

crow::response ProjectManager::handleStopProject(const std::string &projectName)
{
    try
    {
        bool success = stopProject(projectName);

        json11::Json response = json11::Json::object{
            {"success", success},
            {"project_name", projectName}};

        crow::response res(success ? 200 : 500, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
    catch (const std::exception &e)
    {
        json11::Json error = json11::Json::object{
            {"success", false},
            {"error", std::string(e.what())}};
        crow::response res(500, error.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}

crow::response ProjectManager::handleRestartProject(const std::string &projectName)
{
    try
    {
        bool success = restartProject(projectName);

        json11::Json response = json11::Json::object{
            {"success", success},
            {"project_name", projectName}};

        crow::response res(success ? 200 : 500, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
    catch (const std::exception &e)
    {
        json11::Json error = json11::Json::object{
            {"success", false},
            {"error", std::string(e.what())}};
        crow::response res(500, error.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}

crow::response ProjectManager::handleListProjects()
{
    try
    {
        std::vector<ProjectInfo> projects = listProjects();

        json11::Json::array projectsJson;
        for (const auto &project : projects)
        {
            projectsJson.push_back(json11::Json::object{
                {"name", project.name},
                {"archive_path", project.archive_path},
                {"extracted_path", project.extracted_path},
                {"compose_file_path", project.compose_file_path},
                {"is_loaded", project.is_loaded},
                {"is_running", project.is_running},
                {"status_message", project.status_message},
                {"created_time", project.created_time},
                {"last_modified", project.last_modified},
                {"required_images", json11::Json::array(project.required_images.begin(), project.required_images.end())},
                {"dependent_files", json11::Json::array(project.dependent_files.begin(), project.dependent_files.end())}});
        }

        json11::Json response = json11::Json::object{
            {"success", true},
            {"projects", projectsJson}};

        crow::response res(200, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
    catch (const std::exception &e)
    {
        json11::Json error = json11::Json::object{
            {"success", false},
            {"error", std::string(e.what())}};
        crow::response res(500, error.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}

crow::response ProjectManager::handleGetProjectInfo(const std::string &projectName)
{
    try
    {
        ProjectInfo project = getProjectInfo(projectName);

        if (project.name.empty() || project.status_message == "Project not found")
        {
            json11::Json error = json11::Json::object{
                {"success", false},
                {"error", "Project not found"}};
            crow::response res(404, error.dump());
            res.set_header("Content-Type", "application/json");
            return res;
        }

        json11::Json response = json11::Json::object{
            {"success", true},
            {"project", json11::Json::object{
                            {"name", project.name},
                            {"archive_path", project.archive_path},
                            {"extracted_path", project.extracted_path},
                            {"compose_file_path", project.compose_file_path},
                            {"is_loaded", project.is_loaded},
                            // {"is_running", project.is_running},
                            {"status_message", project.status_message},
                            {"created_time", project.created_time},
                            {"last_modified", project.last_modified},
                            {"required_images", json11::Json::array(project.required_images.begin(), project.required_images.end())},
                            {"dependent_files", json11::Json::array(project.dependent_files.begin(), project.dependent_files.end())}}}};

        crow::response res(200, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
    catch (const std::exception &e)
    {
        json11::Json error = json11::Json::object{
            {"success", false},
            {"error", std::string(e.what())}};
        crow::response res(500, error.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}

crow::response ProjectManager::handleGetProjectServices(const std::string &projectName)
{
    try
    {
        std::vector<std::string> services = getProjectServices(projectName);

        json11::Json response = json11::Json::object{
            {"success", true},
            {"project_name", projectName},
            {"services", json11::Json::array(services.begin(), services.end())}};

        crow::response res(200, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
    catch (const std::exception &e)
    {
        json11::Json error = json11::Json::object{
            {"success", false},
            {"error", std::string(e.what())}};
        crow::response res(500, error.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}

crow::response ProjectManager::handleGetProjectLogs(const std::string &projectName, const crow::request &req)
{
    try
    {
        std::string serviceName = req.url_params.get("service") ? req.url_params.get("service") : "";
        std::string logs = getProjectLogs(projectName, serviceName);

        json11::Json response = json11::Json::object{
            {"success", true},
            {"project_name", projectName},
            {"service_name", serviceName},
            {"logs", logs}};

        crow::response res(200, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
    catch (const std::exception &e)
    {
        json11::Json error = json11::Json::object{
            {"success", false},
            {"error", std::string(e.what())}};
        crow::response res(500, error.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}

crow::response ProjectManager::handleCreateProjectArchive(const crow::request &req)
{
    try
    {
        std::string parseError;
        auto json = json11::Json::parse(req.body, parseError);
        if (!json["project_path"].is_string() || !json["archive_path"].is_string())
        {
            crow::response res(400, "Missing project_path or archive_path");
            res.set_header("Content-Type", "application/json");
            return res;
        }

        std::string projectPath = json["project_path"].string_value();
        std::string archivePath = json["archive_path"].string_value();
        std::string password = json["password"].string_value();

        bool success = createProjectArchive(projectPath, archivePath, password,
                                            [this](const ProjectOperationProgress &progress)
                                            {
                                                broadcastProgress(progress);
                                            });

        json11::Json response = json11::Json::object{
            {"success", success},
            {"archive_path", archivePath}};

        crow::response res(success ? 200 : 500, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
    catch (const std::exception &e)
    {
        json11::Json error = json11::Json::object{
            {"success", false},
            {"error", std::string(e.what())}};
        crow::response res(500, error.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}

crow::response ProjectManager::handleGetProjectStatus(const std::string &projectName)
{
    try
    {
        ProjectStatus status = getProjectStatus(projectName);

        json11::Json response = json11::Json::object{
            {"success", true},
            {"project_name", projectName},
            {"status", projectStatusToString(status)}};

        crow::response res(200, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
    catch (const std::exception &e)
    {
        json11::Json error = json11::Json::object{
            {"success", false},
            {"error", std::string(e.what())}};
        crow::response res(500, error.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}

crow::response ProjectManager::handleSaveBrowsingDirectory(const crow::request &req)
{
    try
    {
        std::string parseError;
        auto json = json11::Json::parse(req.body, parseError);
        if (!json["directory_path"].is_string())
        {
            crow::response res(400, "Missing or invalid directory_path");
            res.set_header("Content-Type", "application/json");
            return res;
        }

        std::string directoryPath = json["directory_path"].string_value();

        // Validate directory exists
        if (!std::filesystem::exists(directoryPath))
        {
            json11::Json error = json11::Json::object{
                {"success", false},
                {"error", "Directory does not exist"}};
            crow::response res(400, error.dump());
            res.set_header("Content-Type", "application/json");
            return res;
        }

        bool success = database_->saveSetting(CONST_KEY_LAST_BROWSING_DIRECTORY, directoryPath);

        json11::Json response = json11::Json::object{
            {"success", success},
            {"directory_path", directoryPath}};

        crow::response res(success ? 200 : 500, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
    catch (const std::exception &e)
    {
        json11::Json error = json11::Json::object{
            {"success", false},
            {"error", std::string(e.what())}};
        crow::response res(500, error.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}

crow::response ProjectManager::handleGetBrowsingDirectory()
{
    try
    {
        std::string directoryPath = database_->getSetting(CONST_KEY_LAST_BROWSING_DIRECTORY);

        json11::Json response = json11::Json::object{
            {"success", !directoryPath.empty()},
            {"directory_path", directoryPath}};

        crow::response res(200, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
    catch (const std::exception &e)
    {
        json11::Json error = json11::Json::object{
            {"success", false},
            {"error", std::string(e.what())}};
        crow::response res(500, error.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}
