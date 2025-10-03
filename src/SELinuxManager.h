#ifndef SELINUXMANAGER_H
#define SELINUXMANAGER_H

#include <string>
#include <memory>
#include <functional>
#include <crow.h>
#include "ProcessManager.h"
#include "dotenv.hpp"

enum class SELinuxStatus {
    DISABLED,
    PERMISSIVE,
    ENFORCING,
    NOT_INSTALLED,
    UNKNOWN
};

enum class SELinuxOperationResult {
    SUCCESS,
    FAILURE,
    NOT_SUPPORTED,
    AUTH_FAILED
};

struct SELinuxInfo {
    SELinuxStatus status;
    std::string mode;
    std::string policy;
    std::string mount_point;
    std::string root_directory;
    std::string config_mode;
    std::string mls_status;
    std::string deny_unknown_status;
    std::string memory_protection;
    std::string max_kernel_policy_version;
    bool configured;
    std::string error_message;
};

class SELinuxManager {
public:
    SELinuxManager();
    ~SELinuxManager();

    // Information methods
    SELinuxInfo getSELinuxInfo();
    SELinuxStatus getStatus();
    bool isSELinuxInstalled();
    bool isSELinuxEnabled();

    // Control methods
    SELinuxOperationResult disableSELinux(bool permanent = false);
    SELinuxOperationResult enableSELinux();
    SELinuxOperationResult setEnforcingMode();
    SELinuxOperationResult setPermissiveMode();

    // Utility methods
    SELinuxOperationResult restoreSELinuxContext(const std::string& path);
    SELinuxOperationResult applyDefaultContext(const std::string& path);

    // Password management
    void setSudoPassword(const std::string& password);
    bool validateSudoPassword();

    // REST API registration
    void registerRestEndpoints(crow::SimpleApp& app);

private:
    // REST endpoint handlers
    crow::response handleGetSELinuxInfo();
    crow::response handleGetSELinuxStatus();
    crow::response handleDisableSELinux(const crow::request& req);
    crow::response handleEnableSELinux();
    crow::response handleSetEnforcingMode();
    crow::response handleSetPermissiveMode();
    crow::response handleRestoreSELinuxContext(const crow::request& req);
    crow::response handleApplyDefaultContext(const crow::request& req);
    // crow::response handleValidateSudoPassword(const crow::request& req);

    // Helper methods for REST
    bool requiresSudoPermission(const std::string& operation);
    crow::response createUnauthorizedResponse(const std::string& operation);

    // Helper methods
    std::string executeSELinuxCommand(const std::string& command, const std::vector<std::string>& args = {});
    std::tuple<int, std::string> executeSELinuxCommandWithOutput(const std::string& command, const std::vector<std::string>& args = {});
    bool requiresSudo(const std::string& command);
    SELinuxStatus parseSELinuxStatus(const std::string& output);
    bool updateSELinuxConfig(const std::string& new_mode);

    // Member variables
    std::unique_ptr<ProcessManager> process_manager_;
    std::string sudo_password_;
    bool sudo_validated_;
    EnvParser m_env_parser;
};

#endif // SELINUXMANAGER_H
