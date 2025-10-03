#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <crow.h>
#include "ProcessManager.h"
#include "MetaDatabase.h"
#include "types.hpp"



class MetaDatabase;

class ProjectManager {
public:
    ProjectManager();
    ~ProjectManager();

    // Archive operations
    ProjectArchiveInfo analyzeArchive(const std::string& archivePath, const std::string& password = "");
    // bool validateArchiveIntegrity(const std::string& archivePath, const std::string& password = "");
    bool extractArchive(const std::string& archivePath, const std::string& extractPath, 
                       const std::string& password = "", 
                       std::function<void(const ProjectOperationProgress&)> progressCallback = nullptr);

    // Project management
    bool loadProject(const std::string& archivePath, const std::string& projectName, 
                    const std::string& password = "",
                    std::function<void(const ProjectOperationProgress&)> progressCallback = nullptr);
    bool unloadProject(const std::string& projectName);
    bool removeProject(const std::string& projectName, bool removeFiles = false);
    
    // Project operations
    bool startProject(const std::string& projectName);
    bool stopProject(const std::string& projectName);
    bool restartProject(const std::string& projectName);
    ProjectStatus getProjectStatus(const std::string& projectName);
    
    // Information methods
    std::vector<ProjectInfo> listProjects();
    ProjectInfo getProjectInfo(const std::string& projectName);
    std::vector<std::string> getProjectServices(const std::string& projectName);
    std::string getProjectLogs(const std::string& projectName, const std::string& serviceName = "");

    // Docker image management
    bool loadDockerImagesFromProject(const std::string& projectPath);
    std::vector<std::string> findDockerImageFiles(const std::string& projectPath);
    bool validateRequiredImages(const std::string& projectName);

    // Archive creation (for creating project archives)
    bool createProjectArchive(const std::string& projectPath, const std::string& archivePath, 
                             const std::string& password = "",
                             std::function<void(const ProjectOperationProgress&)> progressCallback = nullptr);

    // REST API registration
    void registerRestEndpoints(crow::SimpleApp& app);

    // WebSocket support
    void setWebSocketConnections(std::vector<crow::websocket::connection*>* log_connections, 
                                std::vector<crow::websocket::connection*>* progress_connections);
    void broadcastLog(const std::string& operation, const std::string& message, const std::string& level = "info");
    void broadcastProgress(const ProjectOperationProgress& progress);

    // Configuration
    void setProjectsDirectory(const std::string& directory);
    std::string getProjectsDirectory() const { return projects_directory_; }

private:
    // Helper methods
    bool extract7zArchive(const std::string& archivePath, const std::string& extractPath, 
                         const std::string& password = "");
    bool create7zArchive(const std::string& sourcePath, const std::string& archivePath, 
                        const std::string& password = "");
    bool test7zArchive(const std::string& archivePath, const std::string& password = "");
    std::vector<std::string> list7zContents(const std::string& archivePath, const std::string& password = "");
    
    // Docker Compose analysis
    // std::vector<std::string> parseDockerComposeImages(const std::string& composeContent);
    std::vector<std::string> parseDockerComposeDependentFiles(const std::string& composeContent, 
                                                              const std::string& basePath);
    std::tuple<bool, std::vector<std::string>> validateDockerComposeFile(const std::string& composeFilePath, std::vector<std::string>& _list_images);
    
    // Project directory management
    std::string createProjectDirectory(const std::string& projectName);
    bool cleanupProjectDirectory(const std::string& projectName);
    std::string getProjectPath(const std::string& projectName);
    
    // Progress tracking
    void updateProgress(const std::string& projectName, ProjectStatus status, int percentage, 
                       const std::string& message, const std::string& operation = "", 
                       const std::string& error = "");
    
    // REST endpoint handlers
    crow::response handleAnalyzeArchive(const crow::request& req);
    crow::response handleLoadProject(const crow::request& req);
    crow::response handleUnloadProject(const std::string& projectName);
    crow::response handleRemoveProject(const std::string& projectName, const crow::request& req);
    crow::response handleStartProject(const std::string& projectName);
    crow::response handleStopProject(const std::string& projectName);
    crow::response handleRestartProject(const std::string& projectName);
    crow::response handleListProjects();
    crow::response handleGetProjectInfo(const std::string& projectName);
    crow::response handleGetProjectServices(const std::string& projectName);
    crow::response handleGetProjectLogs(const std::string& projectName, const crow::request& req);
    crow::response handleCreateProjectArchive(const crow::request& req);
    crow::response handleGetProjectStatus(const std::string& projectName);
    crow::response handleSaveBrowsingDirectory(const crow::request& req);
    crow::response handleGetBrowsingDirectory();
    
    std::unique_ptr<ProcessManager> process_manager_;
    std::unique_ptr<MetaDatabase> database_;
    
    // Member variables
    std::map<std::string, ProjectInfo> projects_;
    std::map<std::string, ProjectOperationProgress> project_progress_;
    std::string projects_directory_;
    // std::string temp_directory_;
    
    // WebSocket connections
    std::vector<crow::websocket::connection*>* log_connections_{nullptr};
    std::vector<crow::websocket::connection*>* progress_connections_{nullptr};
    
    // Docker methods
    bool loadImageFromFile(const std::string& filePath);
    // bool loadComposeFile(const std::string& composeFilePath, const std::string& projectName, const std::string& workingDir);
    // bool removeComposeProject(const std::string& projectName);
    /**
     * @brief starts project
     * @param projectName name of project
     * @param detached indicated to use '-d' in commandline
     * @return a tuple of bool and std::string containing success of running project and output string of docker compose command
     */
    std::tuple<bool, std::string> composeUp(const std::string& projectName);
    std::tuple<bool, std::string> composeDown(const std::string& projectName, bool removeVolumes = false);
    std::tuple<bool, std::string> composeRestart(const std::string& projectName);
    std::tuple<bool, std::string> composeSatus(const std::string& projectName);
    std::tuple<bool, std::string, std::vector<std::string>> composeServices(const std::string& projectName);
    std::vector<std::string> listImages();
    
    // Utility methods
    static std::string projectStatusToString(ProjectStatus status);
    static ProjectStatus stringToProjectStatus(const std::string& status);
    std::string generateProjectId();
    bool isValidProjectName(const std::string& name);
    
    // Command execution helper
    std::string executeCommandWithOutput(const std::string& command, const std::vector<std::string>& args);
};

#endif // PROJECTMANAGER_H
