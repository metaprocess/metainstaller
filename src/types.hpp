#pragma once

#include <string>
#include <vector>
#include <map>


struct ProjectInfo {
    std::string name;
    std::string archive_path;
    std::string extracted_path;
    std::string compose_file_path;
    std::string working_directory;  // Docker Compose working directory
    std::vector<std::string> required_images;
    std::vector<std::string> dependent_files;
    std::vector<std::string> services;  // Docker Compose services
    bool is_loaded = false;
    bool is_running = false;
    std::string status_message;
    std::string created_time;
    std::string last_modified;
};

struct ProjectArchiveInfo {
    std::string archive_path;
    bool is_encrypted = false;
    bool integrity_verified = false;
    std::vector<std::string> contained_files;
    std::vector<std::string> docker_images;
    std::string compose_file_content;
    std::string error_message;
};

enum class ProjectStatus {
    NOT_LOADED,
    EXTRACTING,
    LOADING_IMAGES,
    VALIDATING,
    READY,
    RUNNING,
    STOPPED,
    ERROR
};

struct ProjectOperationProgress {
    ProjectStatus status;
    int percentage;
    std::string message;
    std::string current_operation;
    std::string error_details;
};

#define CONST_KEY_LAST_BROWSING_DIRECTORY "last_directory"