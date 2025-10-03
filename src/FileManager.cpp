#include "FileManager.h"
#include "utils.h"
#include "json11.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iomanip>

namespace fs = std::filesystem;

FileManager::FileManager() {
    // Set default base directory to the project root
    base_directory_ = "/";//Utils::get_metainstaller_home_dir();
    
    // Create directory if it doesn't exist
    // std::filesystem::create_directories(base_directory_);
}

FileManager::~FileManager() {
    // Cleanup if needed
}

void FileManager::registerRestEndpoints(crow::SimpleApp& app) {
    // List directory contents (basic)
    CROW_ROUTE(app, "/api/file/list/<string>").methods("GET"_method)([this](const crow::request& req, const std::string& path) {
        return handleListDirectory(req, path);
    });
    
    // List directory contents (detailed)
    CROW_ROUTE(app, "/api/file/list-detailed").methods("POST"_method)([this](const crow::request& req) {
        std::string parseError;
        auto json = json11::Json::parse(req.body, parseError);
        if (!json["path"].is_string())
        {
            json11::Json response = json11::Json::object{
                {"success", false},
                {"message", "path is necessary"}
            };
            return crow::response(400, "application/json", response.dump());
        }

        std::string _path = json["path"].string_value();

        if (_path.empty())
        {
            json11::Json response = json11::Json::object{
                {"success", false},
                {"message", "path is empty string"}
            };
            return crow::response(400, "application/json", response.dump());
        }
        return handleListDirectoryDetailed(req, _path);
    });

    // // Get file content
    // CROW_ROUTE(app, "/api/file/get/<string>").methods("GET"_method)([this](const crow::request& req, const std::string& path) {
    //     return handleGetFile(req, path);
    // });

    // // Get directory tree
    // CROW_ROUTE(app, "/api/file/tree/<string>").methods("GET"_method)([this](const crow::request& req, const std::string& path) {
    //     return handleGetDirectoryTree(req, path);
    // });

    // // Upload file
    // CROW_ROUTE(app, "/api/file/upload").methods("POST"_method)([this](const crow::request& req) {
    //     return handleFileUpload(req);
    // });

    // // Delete file
    // CROW_ROUTE(app, "/api/file/delete/<string>").methods("DELETE"_method)([this](const crow::request& req, const std::string& path) {
    //     return handleFileDelete(req, path);
    // });
}

std::vector<std::string> FileManager::listDirectory(const std::string& path) {
    std::vector<std::string> entries;
    
    try {
        std::string fullPath = Utils::path_join_multiple({base_directory_, path});
        
        if (!fs::exists(fullPath) || !fs::is_directory(fullPath)) {
            return entries;
        }

        for (const auto& entry : fs::directory_iterator(fullPath)) {
            std::string fileName = entry.path().filename().string();
            entries.push_back(fileName);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error listing directory: " << e.what() << std::endl;
    }
    
    // Sort entries for consistent output
    std::sort(entries.begin(), entries.end());
    return entries;
}

std::vector<FileManager::FileInfo> FileManager::listDirectoryDetailed(const std::string& path) {
    std::vector<FileInfo> entries;
    
    try {
        std::string fullPath = Utils::path_join_multiple({base_directory_, path});
        
        if (!fs::exists(fullPath) || !fs::is_directory(fullPath)) {
            return entries;
        }

        for (const auto& entry : fs::directory_iterator(fullPath)) {
            FileInfo info;
            info.name = entry.path().filename().string();
            info.is_directory = entry.is_directory();
            
            if (entry.is_directory()) {
                info.size = 0; // Directories don't have a size in the same way files do
            } else {
                info.size = entry.file_size();
            }
            
            // Get last modified time
            auto last_write_time = entry.last_write_time();
            auto system_time = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                last_write_time - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
            auto time_t = std::chrono::system_clock::to_time_t(system_time);
            
            // Format time as string
            std::ostringstream oss;
            oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
            info.last_modified = oss.str();
            
            entries.push_back(info);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error listing directory: " << e.what() << std::endl;
    }
    
    // Sort entries for consistent output (directories first, then alphabetically)
    std::sort(entries.begin(), entries.end(), [](const FileInfo& a, const FileInfo& b) {
        if (a.is_directory != b.is_directory) {
            return a.is_directory > b.is_directory; // Directories first
        }
        return a.name < b.name;
    });
    
    return entries;
}

bool FileManager::fileExists(const std::string& path) {
    try {
        std::string fullPath = Utils::path_join_multiple({base_directory_, path});
        return fs::exists(fullPath);
    } catch (const std::exception& e) {
        return false;
    }
}

std::string FileManager::getFileContent(const std::string& path) {
    try {
        std::string fullPath = Utils::path_join_multiple({base_directory_, path});
        
        if (!fs::exists(fullPath) || fs::is_directory(fullPath)) {
            return "";
        }

        std::ifstream file(fullPath, std::ios::binary);
        if (!file) {
            return "";
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return content;
    } catch (const std::exception& e) {
        std::cerr << "Error reading file: " << e.what() << std::endl;
        return "";
    }
}

std::vector<std::string> FileManager::getDirectoryTree(const std::string& path) {
    std::vector<std::string> tree;
    
    try {
        std::string fullPath = Utils::path_join_multiple({base_directory_, path});
        
        if (!fs::exists(fullPath) || !fs::is_directory(fullPath)) {
            return tree;
        }

        // Recursive function to build tree
        std::function<void(const fs::path&, int)> buildTree = [&](const fs::path& dirPath, int depth) {
            try {
                for (const auto& entry : fs::directory_iterator(dirPath)) {
                    std::string indent(depth * 2, ' ');
                    std::string name = entry.path().filename().string();
                    
                    if (entry.is_directory()) {
                        tree.push_back(indent + "[" + name + "]");
                        buildTree(entry.path(), depth + 1);
                    } else {
                        tree.push_back(indent + name);
                    }
                }
            } catch (const std::exception& e) {
                // Skip directories we can't access
            }
        };
        
        tree.push_back("[" + fs::path(fullPath).filename().string() + "]");
        buildTree(fullPath, 1);
    } catch (const std::exception& e) {
        std::cerr << "Error building directory tree: " << e.what() << std::endl;
    }
    
    return tree;
}

crow::response FileManager::handleListDirectory(const crow::request& req, const std::string& path) {
    try {
        std::vector<std::string> entries = listDirectory(path);
        
        json11::Json::array entriesJson;
        for (const auto& entry : entries) {
            entriesJson.push_back(entry);
        }
        
        json11::Json response = json11::Json::object{
            {"success", true},
            {"path", path},
            {"entries", entriesJson}
        };
        
        crow::response res(200, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    } catch (const std::exception& e) {
        json11::Json error = json11::Json::object{
            {"success", false},
            {"error", std::string(e.what())}
        };
        crow::response res(500, error.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}

crow::response FileManager::handleListDirectoryDetailed(const crow::request& req, const std::string& path) {
    try {
        if(!std::filesystem::exists(path))
        {
            json11::Json response = json11::Json::object{
                {"success", false},
                {"message", "path does not exists"}
            };
            return crow::response(400, "application/json", response.dump());
        }
        std::vector<FileInfo> entries = listDirectoryDetailed(path);
        
        json11::Json::array entriesJson;
        for (const auto& entry : entries) {
            json11::Json entryJson = json11::Json::object{
                {"name", entry.name},
                {"is_directory", entry.is_directory},
                {"size", static_cast<int>(entry.size)},
                {"last_modified", entry.last_modified}
            };
            entriesJson.push_back(entryJson);
        }
        
        json11::Json response = json11::Json::object{
            {"success", true},
            {"path", path},
            {"entries", entriesJson}
        };
        
        crow::response res(200, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    } catch (const std::exception& e) {
        json11::Json error = json11::Json::object{
            {"success", false},
            {"error", std::string(e.what())}
        };
        crow::response res(500, error.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}

crow::response FileManager::handleGetFile(const crow::request& req, const std::string& path) {
    try {
        // Security check: ensure path doesn't try to escape base directory
        if (path.find("..") != std::string::npos) {
            json11::Json error = json11::Json::object{
                {"success", false},
                {"error", "Invalid path"}
            };
            crow::response res(400, error.dump());
            res.set_header("Content-Type", "application/json");
            return res;
        }
        
        std::string fullPath = Utils::path_join_multiple({base_directory_, path});
        
        if (!fs::exists(fullPath)) {
            json11::Json error = json11::Json::object{
                {"success", false},
                {"error", "File not found"}
            };
            crow::response res(404, error.dump());
            res.set_header("Content-Type", "application/json");
            return res;
        }
        
        if (fs::is_directory(fullPath)) {
            json11::Json error = json11::Json::object{
                {"success", false},
                {"error", "Path is a directory"}
            };
            crow::response res(400, error.dump());
            res.set_header("Content-Type", "application/json");
            return res;
        }
        
        // For binary files, we'll return base64 encoded content
        std::ifstream file(fullPath, std::ios::binary);
        if (!file) {
            json11::Json error = json11::Json::object{
                {"success", false},
                {"error", "Cannot read file"}
            };
            crow::response res(500, error.dump());
            res.set_header("Content-Type", "application/json");
            return res;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
        // Determine MIME type
        std::string mimeType = getMimeType(fullPath);
        
        json11::Json response = json11::Json::object{
            {"success", true},
            {"path", path},
            {"mime_type", mimeType},
            {"size", static_cast<int>(content.size())},
            {"content", content}
        };
        
        crow::response res(200, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    } catch (const std::exception& e) {
        json11::Json error = json11::Json::object{
            {"success", false},
            {"error", std::string(e.what())}
        };
        crow::response res(500, error.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}

crow::response FileManager::handleGetDirectoryTree(const crow::request& req, const std::string& path) {
    try {
        std::vector<std::string> tree = getDirectoryTree(path);
        
        json11::Json::array treeJson;
        for (const auto& line : tree) {
            treeJson.push_back(line);
        }
        
        json11::Json response = json11::Json::object{
            {"success", true},
            {"path", path},
            {"tree", treeJson}
        };
        
        crow::response res(200, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    } catch (const std::exception& e) {
        json11::Json error = json11::Json::object{
            {"success", false},
            {"error", std::string(e.what())}
        };
        crow::response res(500, error.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}

crow::response FileManager::handleFileUpload(const crow::request& req) {
    try {
        // For simplicity, we'll return a not implemented response
        // A full implementation would handle multipart form data
        json11::Json response = json11::Json::object{
            {"success", false},
            {"error", "File upload not implemented in this example"}
        };
        
        crow::response res(501, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    } catch (const std::exception& e) {
        json11::Json error = json11::Json::object{
            {"success", false},
            {"error", std::string(e.what())}
        };
        crow::response res(500, error.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}

crow::response FileManager::handleFileDelete(const crow::request& req, const std::string& path) {
    try {
        // Security check: ensure path doesn't try to escape base directory
        if (path.find("..") != std::string::npos) {
            json11::Json error = json11::Json::object{
                {"success", false},
                {"error", "Invalid path"}
            };
            crow::response res(400, error.dump());
            res.set_header("Content-Type", "application/json");
            return res;
        }
        
        std::string fullPath = Utils::path_join_multiple({base_directory_, path});
        
        if (!fs::exists(fullPath)) {
            json11::Json error = json11::Json::object{
                {"success", false},
                {"error", "File not found"}
            };
            crow::response res(404, error.dump());
            res.set_header("Content-Type", "application/json");
            return res;
        }
        
        // Delete file or directory
        std::error_code ec;
        if (fs::is_directory(fullPath)) {
            fs::remove_all(fullPath, ec);
        } else {
            fs::remove(fullPath, ec);
        }
        
        if (ec) {
            json11::Json error = json11::Json::object{
                {"success", false},
                {"error", "Failed to delete: " + ec.message()}
            };
            crow::response res(500, error.dump());
            res.set_header("Content-Type", "application/json");
            return res;
        }
        
        json11::Json response = json11::Json::object{
            {"success", true},
            {"message", "Deleted successfully"}
        };
        
        crow::response res(200, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    } catch (const std::exception& e) {
        json11::Json error = json11::Json::object{
            {"success", false},
            {"error", std::string(e.what())}
        };
        crow::response res(500, error.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}

std::string FileManager::getMimeType(const std::string& filePath) {
    // Simple MIME type detection based on file extension
    std::string extension = fs::path(filePath).extension().string();
    
    // Convert to lowercase
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension == ".txt") return "text/plain";
    if (extension == ".html" || extension == ".htm") return "text/html";
    if (extension == ".css") return "text/css";
    if (extension == ".js") return "application/javascript";
    if (extension == ".json") return "application/json";
    if (extension == ".png") return "image/png";
    if (extension == ".jpg" || extension == ".jpeg") return "image/jpeg";
    if (extension == ".gif") return "image/gif";
    if (extension == ".svg") return "image/svg+xml";
    if (extension == ".pdf") return "application/pdf";
    if (extension == ".zip") return "application/zip";
    if (extension == ".tar") return "application/x-tar";
    
    return "application/octet-stream";
}

std::string FileManager::getBasePath() const {
    return base_directory_;
}
