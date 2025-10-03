#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <crow.h>
#include <filesystem>

class FileManager {
public:
    FileManager();
    ~FileManager();

    // REST API registration
    void registerRestEndpoints(crow::SimpleApp& app);

    // File operations
    struct FileInfo {
        std::string name;
        bool is_directory;
        size_t size;
        std::string last_modified;
    };
    
    std::vector<FileInfo> listDirectoryDetailed(const std::string& path);
    std::vector<std::string> listDirectory(const std::string& path); // Keep for backward compatibility
    bool fileExists(const std::string& path);
    std::string getFileContent(const std::string& path);
    std::vector<std::string> getDirectoryTree(const std::string& path);

private:
    // REST endpoint handlers
    crow::response handleListDirectory(const crow::request& req, const std::string& path);
    crow::response handleListDirectoryDetailed(const crow::request& req, const std::string& path);
    crow::response handleGetFile(const crow::request& req, const std::string& path);
    crow::response handleGetDirectoryTree(const crow::request& req, const std::string& path);
    crow::response handleFileUpload(const crow::request& req);
    crow::response handleFileDelete(const crow::request& req, const std::string& path);
    
    // Utility methods
    std::string getMimeType(const std::string& filePath);
    std::string getBasePath() const;
    
    // Member variables
    std::string base_directory_;
};

#endif // FILEMANAGER_H
