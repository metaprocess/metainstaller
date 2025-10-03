#include "EnvConfig.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <cstdlib>

// Static member definitions
std::map<EnvKey, EnvVariable> EnvConfig::env_map;

void EnvConfig::initialize_env_variables() {
    if (!EnvConfig::env_map.empty()) {
        return; // Already initialized
    }
    
    // Define all environment variables with their metadata
    EnvConfig::env_map = {
        {EnvKey::REST_PORT, 
         EnvVariable(EnvKey::REST_PORT, "REST_PORT", "14040", 
                    "Port number for the REST API server")},
        {EnvKey::LOG_LEVEL, 
         EnvVariable(EnvKey::LOG_LEVEL, "LOG_LEVEL", "INFO", 
                    "Logging level (DEBUG, INFO, WARN, ERROR)")},
        {EnvKey::SUDO_PASSWORD,
         EnvVariable(EnvKey::SUDO_PASSWORD, "SUDO_PASSWORD", "",
                    "Sudo password for the current user")}
    };
    return;
}

void EnvConfig::ensure_initialized() {
    if (EnvConfig::env_map.empty()) {
        initialize_env_variables();
    }
}

std::string EnvConfig::get_key_name(EnvKey key) {
    ensure_initialized();
    auto it = EnvConfig::env_map.find(key);
    if (it != EnvConfig::env_map.end()) {
        return it->second.name;
    }
    return "";
}

std::string EnvConfig::get_default_value(EnvKey key) {
    ensure_initialized();
    auto it = EnvConfig::env_map.find(key);
    if (it != EnvConfig::env_map.end()) {
        return it->second.default_value;
    }
    return "";
}

std::string EnvConfig::get_description(EnvKey key) {
    ensure_initialized();
    auto it = EnvConfig::env_map.find(key);
    if (it != EnvConfig::env_map.end()) {
        return it->second.description;
    }
    return "";
}

std::string EnvConfig::get_value(EnvKey key) {
    ensure_initialized();
    std::string key_name = get_key_name(key);
    if (key_name.empty()) {
        return "";
    }
    
    // First check system environment
    const char* env_value = std::getenv(key_name.c_str());
    if (env_value) {
        return std::string(env_value);
    }
    
    // Fall back to default value
    return get_default_value(key);
}

int EnvConfig::get_int_value(EnvKey key) {
    std::string value = get_value(key);
    if (value.empty()) {
        return 0;
    }
    
    try {
        return std::stoi(value);
    } catch (const std::exception&) {
        // If conversion fails, return the integer value of the default
        std::string default_val = get_default_value(key);
        try {
            return std::stoi(default_val);
        } catch (const std::exception&) {
            return 0;
        }
    }
}

bool EnvConfig::create_default_env_file(const std::string& file_path) {
    ensure_initialized();
    
    std::ofstream env_file(file_path);
    if (!env_file.is_open()) {
        std::cerr << "ERROR: Failed to create " << file_path << " file\n";
        return false;
    }
    
    // Write file header
    env_file << "# MetaInstaller Environment Configuration\n";
    env_file << "# This file was automatically generated\n";
    env_file << "# Modify these values as needed for your deployment\n\n";
    
    // Group related variables
    // std::string current_section = "";
    
    for (const auto& [key, var] : EnvConfig::env_map) {
        // Add section headers based on variable prefixes
        // std::string section = "";
        // if (var.name.find("REST_") == 0) {
        //     section = "REST API Configuration";
        // } else if (var.name.find("QUEUE_") == 0) {
        //     section = "Queue Management Configuration";
        // } else if (var.name.find("WS_") == 0) {
        //     section = "WebSocket Configuration";
        // } else if (var.name.find("LOG_") == 0) {
        //     section = "Logging Configuration";
        // } else {
        //     section = "General Configuration";
        // }
        
        // if (section != current_section) {
        //     if (!current_section.empty()) {
        //         env_file << "\n";
        //     }
        //     env_file << "# " << section << "\n";
        //     current_section = section;
        // }
        
        // Write the variable with its description
        env_file << "# " << var.description << "\n";
        env_file << var.name << "=" << var.default_value << "\n\n";
    }
    
    env_file.close();
    std::cerr << "INFO: Created default " << file_path << " file with all configuration variables\n";
    return true;
}

const std::map<EnvKey, EnvVariable>& EnvConfig::get_all_variables() {
    ensure_initialized();
    return EnvConfig::env_map;
}

bool EnvConfig::ensure_env_file_exists(const std::string& file_path) {
    // Check if .env file already exists
    if (std::filesystem::exists(file_path)) {
        std::cerr << "INFO: " << file_path << " file already exists\n";
        return true;
    }
    
    // Create the file with default values
    return create_default_env_file(file_path);
}

bool EnvConfig::set_value(EnvKey key, const std::string& value, const std::string& file_path) {
    ensure_initialized();
    std::string key_name = get_key_name(key);
    if (key_name.empty()) {
        return false;
    }

    std::ifstream file_in(file_path);
    std::stringstream buffer;
    bool key_found = false;

    if (file_in.is_open()) {
        std::string line;
        while (std::getline(file_in, line)) {
            std::string current_key = line.substr(0, line.find('='));
            if (current_key == key_name) {
                buffer << key_name << "=" << value << "\n";
                key_found = true;
            } else {
                buffer << line << "\n";
            }
        }
        file_in.close();
    }

    if (!key_found) {
        buffer << key_name << "=" << value << "\n";
    }

    std::ofstream file_out(file_path);
    if (!file_out.is_open()) {
        return false;
    }

    file_out << buffer.str();
    file_out.close();

    return true;
}
