#include "help_global.h"
#include "EnvConfig.hpp"
#include "argument_handler.h"
#include "test.h"

std::string HelpGlobal::help_global() {
    std::string help_str = "Container Installer REST - Comprehensive Docker Management System\n";
    help_str += "==================================================================\n\n";
    
    // Application Purpose
    help_str += "PURPOSE:\n";
    help_str += "This application provides a complete solution for Docker installation, configuration, and management\n";
    help_str += "through a REST API interface. It automates the entire Docker setup process and provides comprehensive\n";
    help_str += "management capabilities for containers, images, and services.\n\n";
    
    // Command Line Arguments
    help_str += "COMMAND LINE ARGUMENTS:\n";
    help_str += "  --test <test_name>     : Run specific test(s)\n";
    help_str += "    - Use 'all' to run all available tests\n";
    help_str += "    - Use a single test name to run one test (e.g., 'sudo')\n";
    help_str += "    - Use comma-separated names to run multiple tests (e.g., 'sudo,docker_install')\n";
    help_str += "  --help                 : Show this help message\n\n";
    
    // Available Tests
    help_str += "AVAILABLE TESTS:\n";
    Test test_instance;
    std::vector<std::string> test_names = test_instance.get_test_names();
    for (const auto& name : test_names) {
        help_str += "  " + name + "\n";
    }
    help_str += "\n";
    
    // .env File Configuration
    help_str += ".ENV FILE CONFIGURATION:\n";
    help_str += "The application uses environment variables defined in .env files for configuration.\n";
    help_str += "If no .env file exists, the application will create one with default values.\n";
    help_str += "The .env file supports variable substitution and command execution:\n";
    help_str += "  - Basic key-value pairs (KEY=value)\n";
    help_str += "  - Variable substitution (${VAR} and $VAR)\n";
    help_str += "  - Command execution ($(command))\n";
    help_str += "  - Comments (lines starting with #)\n";
    help_str += "  - Quoted values\n\n";
    
    // List all environment variables from EnvConfig
    for (const auto& [key, var] : EnvConfig::get_all_variables()) {
        help_str += "  " + var.name + " = " + var.default_value + "\n";
        help_str += "    " + var.description + "\n\n";
    }
    
    // .env.secret File
    help_str += ".ENV.SECRET FILE:\n";
    help_str += "This file contains sensitive information and should not be committed to version control.\n";
    help_str += "It is used to store the sudo password for Docker installation and management operations.\n";
    help_str += "The file is automatically created if it doesn't exist, with a placeholder password.\n";
    help_str += "The application supports the same features as .env files, including variable substitution.\n\n";
    help_str += "  SUDO_PSWD = CHANGE_PASSWORD\n";
    help_str += "    Sudo password for the current user (required for Docker installation)\n\n";
    
    // REST API Mechanisms
    help_str += "REST API MECHANISMS:\n";
    help_str += "The application exposes a comprehensive REST API for Docker management:\n\n";
    help_str += "  - All endpoints are accessible via standard HTTP methods (GET, POST, PUT, DELETE)\n";
    help_str += "  - Endpoints follow the pattern: /api/<resource>/<operation>\n";
    help_str += "  - All responses are in JSON format with standard HTTP status codes\n";
    help_str += "  - Error responses follow a consistent format with error messages and codes\n";
    help_str += "  - WebSocket endpoints provide real-time updates for long-running operations\n";
    help_str += "  - Sudo-protected operations require authentication via the SUDO_PSWD environment variable\n\n";
    
    help_str += "REST API RESPONSE FORMAT:\n";
    help_str += "  Success Response:\n";
    help_str += "    {\n";
    help_str += "      \"success\": true,\n";
    help_str += "      \"message\": \"Operation completed successfully\",\n";
    help_str += "      \"data\": { ... }\n";
    help_str += "    }\n\n";
    help_str += "  Error Response:\n";
    help_str += "    {\n";
    help_str += "      \"error\": \"Error description\",\n";
    help_str += "      \"code\": 400\n";
    help_str += "    }\n\n";
    
    // Key REST Endpoints
    help_str += "KEY REST ENDPOINTS:\n";
    help_str += "  GET    /api/docker/info                    : Get Docker installation information\n";
    help_str += "  POST   /api/docker/install                 : Install Docker\n";
    help_str += "  DELETE /api/docker/uninstall               : Uninstall Docker\n";
    help_str += "  POST   /api/docker/service/start           : Start Docker service\n";
    help_str += "  POST   /api/docker/service/stop            : Stop Docker service\n";
    help_str += "  POST   /api/docker/service/restart         : Restart Docker service\n";
    help_str += "  GET    /api/docker/containers              : List containers\n";
    help_str += "  GET    /api/docker/containers/detailed     : List containers with detailed information\n";
    help_str += "  GET    /api/docker/images                  : List images\n";
    help_str += "  POST   /api/docker/images/pull             : Pull image from registry\n";
    help_str += "  GET    /api/docker/system/info             : Get system information\n";
    help_str += "  GET    /api/docker/system/df               : Get disk usage\n";
    help_str += "  POST   /api/docker/system/prune            : Clean up system\n";
    help_str += "  GET    /api/docker/system/integration-status : Check system integration status\n";
    help_str += "  GET    /api/docs                           : Get API documentation in HTML format\n\n";
    
    // WebSocket Endpoints
    help_str += "WEBSOCKET ENDPOINTS:\n";
    help_str += "  ws://localhost:<port>/ws/logs            : Real-time operation logs\n";
    help_str += "  ws://localhost:<port>/ws/progress        : Real-time installation progress\n\n";
    
    // Usage Examples
    help_str += "USAGE EXAMPLES:\n";
    help_str += "  # Start the application\n";
    help_str += "  ./MetaInstaller\n\n";
    help_str += "  # Run all tests\n";
    help_str += "  ./MetaInstaller --test all\n\n";
    help_str += "  # Run specific test\n";
    help_str += "  ./MetaInstaller --test sudo,docker_install\n\n";
    help_str += "  # Show help\n";
    help_str += "  ./MetaInstaller --help\n\n";
    
    // API Usage Examples
    help_str += "API USAGE EXAMPLES:\n";
    help_str += "  # Check Docker installation status\n";
    help_str += "  curl http://localhost:14040/api/docker/info\n\n";
    help_str += "  # Install Docker (requires sudo password)\n";
    help_str += "  curl -X POST http://localhost:14040/api/docker/install\n\n";
    help_str += "  # List containers\n";
    help_str += "  curl http://localhost:14040/api/docker/containers\n\n";
    help_str += "  # Get API documentation\n";
    help_str += "  curl http://localhost:14040/api/docs\n\n";
    
    // Additional Information
    help_str += "ADDITIONAL INFORMATION:\n";
    help_str += "  - The application automatically creates .env files if they don't exist\n";
    help_str += "  - Docker binaries are extracted from embedded resources and installed to /usr/local/bin\n";
    help_str += "  - Systemd services are configured for automatic Docker startup\n";
    help_str += "  - The docker group is created and the current user is added to it\n";
    help_str += "  - A web-based UI is available at http://localhost:<REST_PORT> after startup\n";
    help_str += "  - Full API documentation is available at /api/docs endpoint\n";
    help_str += "  - Sudo password is required for installation and system operations\n";
    
    return help_str;
}
