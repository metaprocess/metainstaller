#ifndef DOCKERMANAGER_H
#define DOCKERMANAGER_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <crow.h>
#include <chrono>
#include "ProcessManager.h"
#include "dotenv.hpp"

struct DockerInfo {
    std::string version;
    std::string api_version;
    std::string min_api_version;
    std::string git_commit;
    std::string go_version;
    std::string os;
    std::string arch;
    std::string kernel_version;
    bool experimental;
    std::string build_time;
    bool installed = false;
    std::string error_message;
};

struct DockerComposeInfo {
    std::string version;
    // std::string docker_compose_version;
    bool installed = false;
    std::string error_message;
};

struct DockerComposeProject {
    std::string name;
    std::string compose_file_path;
    std::string working_directory;
    std::vector<std::string> services;
    std::map<std::string, std::string> env_variables;
};

struct ContainerInfo {
    std::string id;
    std::string name;
    std::string image;
    std::string status;
    std::string created;
    std::vector<std::string> ports;
};

enum class InstallationStatus {
    NOT_STARTED,
    IN_PROGRESS,
    COMPLETED,
    FAILED,
    EXTRACTING,
    INSTALLING,
    CONFIGURING
};


struct InstallationProgress {
    InstallationStatus status;
    int percentage;
    std::string message;
    std::string error_details;
};

class DockerManager {
public:
    DockerManager();
    ~DockerManager();

    // Installation methods
    bool installDocker(std::function<void(const InstallationProgress&)> progressCallback = nullptr);
    // bool installDockerCompose(std::function<void(const InstallationProgress&)> progressCallback = nullptr);
    bool uninstallDocker();
    // bool uninstallDockerCompose();

    // Information methods
    DockerInfo getDockerInfo();
    DockerComposeInfo getDockerComposeInfo();
    bool isDockerInstalled();
    bool isDockerComposeInstalled();
    bool isDockerRunning();

    // Service management
    bool startDockerService();
    bool stopDockerService();
    bool restartDockerService();
    std::string getDockerServiceStatus();

    // Container operations
    std::vector<std::string> listContainers(bool all = false);
    std::vector<ContainerInfo> getDetailedContainers(bool all = false);
    std::vector<std::string> listImages();
    bool pullImage(const std::string& imageName, std::function<void(const std::string&)> progressCallback = nullptr);
    bool removeContainer(const std::string& containerId, bool force = false);
    bool removeImage(const std::string& imageId, bool force = false);
    
    // Advanced image operations
    bool loadImageFromFile(const std::string& filePath);
    bool saveImageToFile(const std::string& imageName, const std::string& filePath);
    bool tagImage(const std::string& sourceImage, const std::string& targetTag);
    bool buildImage(const std::string& dockerfilePath, const std::string& imageName, const std::string& buildContext = ".");
    
    // Container lifecycle management
    bool startContainer(const std::string& containerId);
    bool stopContainer(const std::string& containerId, int timeout = 10);
    bool restartContainer(const std::string& containerId, int timeout = 10);
    bool pauseContainer(const std::string& containerId);
    bool unpauseContainer(const std::string& containerId);
    std::string getContainerLogs(const std::string& containerId, int lines = 100);
    
    // Docker Compose operations
    bool loadComposeFile(const std::string& composeFilePath, const std::string& projectName, const std::string& workingDir);
    bool removeComposeProject(const std::string& projectName);
    std::vector<DockerComposeProject> listComposeProjects();
    bool composeUp(const std::string& projectName, bool detached = true);
    bool composeDown(const std::string& projectName, bool removeVolumes = false);
    bool composeRestart(const std::string& projectName);
    std::vector<std::string> composeServices(const std::string& projectName);
    bool composeServiceAction(const std::string& projectName, const std::string& serviceName, const std::string& action);

    // System operations
    std::string getSystemInfo();
    std::string getDiskUsage();
    bool cleanupSystem();

    // REST API registration
    void registerRestEndpoints(crow::SimpleApp& app);

    // Installation progress tracking
    InstallationProgress getCurrentProgress() const { return current_progress_; }
    
    // WebSocket support
    void setWebSocketConnections(std::vector<crow::websocket::connection*>* log_connections, 
                                std::vector<crow::websocket::connection*>* progress_connections);
    void broadcastLog(const std::string& operation, const std::string& message, const std::string& level = "info");
    void broadcastProgress(const InstallationProgress& progress);

private:
    // Helper methods
    std::string extractDockerBinary();
    std::string extractDockerComposeBinary();
    bool installDockerBinaries(const std::string& _path);
    // bool installSystemdServices();
    bool setupDockerEnvironment();
    bool addToSystemPath(const std::string& path);
    bool createDockerGroup();
    bool addUserToDockerGroup(const std::string& username = "");
    std::string executeCommand(const std::string& command, const std::vector<std::string>& args = {});
    std::string executeCommandWithOutput(const std::string& command, const std::vector<std::string>& args = {});
    bool isCommandAvailable(const std::string& command);
    std::string getTemporaryPath();
    void updateProgress(InstallationStatus status, int percentage, const std::string& message, const std::string& error = "");
    static std::string get_installation_status(const InstallationStatus& _status);

    // REST endpoint handlers
    crow::response handleGetDockerInfo();
    crow::response handleGetDockerComposeInfo();
    // crow::response handleInstallDocker();
    crow::response handleUninstallDocker();
    // crow::response handleInstallDockerCompose();
    // crow::response handleUninstallDockerCompose();
    crow::response handleStartDockerService();
    crow::response handleStopDockerService();
    crow::response handleRestartDockerService();
    crow::response handleGetServiceStatus();
    crow::response handleListContainers(const crow::request& req);
    crow::response handleListImages();
    crow::response handlePullImage(const crow::request& req);
    crow::response handleRemoveContainer(const std::string& containerId, const crow::request& req);
    crow::response handleRemoveImage(const std::string& imageId, const crow::request& req);
    crow::response handleGetSystemInfo();
    crow::response handleGetDiskUsage();
    crow::response handleCleanupSystem();
    crow::response handleGetInstallationProgress();
    
    // Advanced image management handlers
    crow::response handleLoadImageFromFile(const crow::request& req);
    crow::response handleSaveImageToFile(const crow::request& req);
    crow::response handleTagImage(const crow::request& req);
    crow::response handleBuildImage(const crow::request& req);
    
    // Container lifecycle handlers
    crow::response handleStartContainer(const std::string& containerId);
    crow::response handleStopContainer(const std::string& containerId, const crow::request& req);
    crow::response handleRestartContainer(const std::string& containerId, const crow::request& req);
    crow::response handlePauseContainer(const std::string& containerId);
    crow::response handleUnpauseContainer(const std::string& containerId);
    crow::response handleGetContainerLogs(const std::string& containerId, const crow::request& req);
    crow::response handleGetDetailedContainers(const crow::request& req);
    
    // Docker Compose handlers
    crow::response handleLoadComposeFile(const crow::request& req);
    crow::response handleRemoveComposeProject(const std::string& projectName);
    crow::response handleListComposeProjects();
    crow::response handleComposeUp(const std::string& projectName, const crow::request& req);
    crow::response handleComposeDown(const std::string& projectName, const crow::request& req);
    crow::response handleComposeRestart(const std::string& projectName);
    crow::response handleComposeServices(const std::string& projectName);
    crow::response handleComposeServiceAction(const std::string& projectName, const std::string& serviceName, const crow::request& req);
    
    // Complete installation and service management handlers
    crow::response handleInstallDocker();
    crow::response handleEnableDockerService();
    crow::response handleDisableDockerService();
    crow::response handleGetSystemIntegrationStatus();
    
    // Documentation endpoint
    crow::response handleGetDocumentation();

    // Test endpoint
    crow::response handleTestSudo(const crow::request& _req);

    // Member variables
    std::unique_ptr<ProcessManager> process_manager_;
    InstallationProgress current_progress_;
    std::string docker_install_path_;
    // std::string docker_compose_install_path_;
    std::string temp_dir_;
    bool installation_in_progress_{false};
    std::function<void(const InstallationProgress&)> progress_callback_;
    std::map<std::string, DockerComposeProject> compose_projects_;
    
    // WebSocket connections
    std::vector<crow::websocket::connection*>* log_connections_{nullptr};
    std::vector<crow::websocket::connection*>* progress_connections_{nullptr};

    std::string sudo_password_;
    bool sudo_validated_{false};
    std::chrono::steady_clock::time_point last_sudo_validation_;
    static constexpr std::chrono::minutes SUDO_TIMEOUT{15}; // Sudo session timeout
    EnvParser m_env_parser;

    // Add these helper methods to the private section
    bool validateSudoPassword();
    bool requiresSudoPermission(const std::string& operation);
    crow::response createUnauthorizedResponse(const std::string& operation);
};

#endif // DOCKERMANAGER_H
