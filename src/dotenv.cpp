#include "dotenv.hpp"
#include <fstream>
#include <regex>
#include <iostream>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <array>
#include <sstream>

std::string EnvParser::execute_command(const std::string& command) {
    std::array<char, 128> buffer;
    std::string result;
    
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    
    // Remove trailing newline
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    
    return result;
}

std::string EnvParser::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::string EnvParser::remove_quotes(const std::string& value) {
    std::string trimmed = trim(value);
    if (trimmed.length() >= 2) {
        if ((trimmed.front() == '"' && trimmed.back() == '"') ||
            (trimmed.front() == '\'' && trimmed.back() == '\'')) {
            return trimmed.substr(1, trimmed.length() - 2);
        }
    }
    return trimmed;
}

std::string EnvParser::resolve_value(const std::string& value) {
    std::string result = value;
    
    // Handle command substitution $(command)
    std::regex cmd_regex(R"(\$\(([^)]+)\))");
    std::smatch match;
    
    while (std::regex_search(result, match, cmd_regex)) {
        std::string command = match[1].str();
        try {
            std::string output = execute_command(command);
            result.replace(match.position(), match.length(), output);
        } catch (const std::exception& e) {
            std::cerr << "Error executing command '" << command << "': " << e.what() << std::endl;
            result.replace(match.position(), match.length(), "");
        }
    }
    
    // Handle variable substitution ${VAR}
    std::regex var_regex(R"(\$\{([^}]+)\})");
    
    while (std::regex_search(result, match, var_regex)) {
        std::string var_name = match[1].str();
        std::string replacement = "";
        
        // Check if variable exists in our env_vars
        auto it = env_vars.find(var_name);
        if (it != env_vars.end()) {
            replacement = it->second;
        } else {
            // Check system environment
            const char* env_val = std::getenv(var_name.c_str());
            if (env_val) {
                replacement = env_val;
            }
        }
        
        result.replace(match.position(), match.length(), replacement);
    }
    
    // Handle simple variable substitution $VAR (without braces)
    std::regex simple_var_regex(R"(\$([A-Za-z_][A-Za-z0-9_]*))");
    
    while (std::regex_search(result, match, simple_var_regex)) {
        std::string var_name = match[1].str();
        std::string replacement = "";
        
        auto it = env_vars.find(var_name);
        if (it != env_vars.end()) {
            replacement = it->second;
        } else {
            const char* env_val = std::getenv(var_name.c_str());
            if (env_val) {
                replacement = env_val;
            }
        }
        
        result.replace(match.position(), match.length(), replacement);
    }
    
    return result;
}

bool EnvParser::load_env_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Find the first = sign
        size_t equals_pos = line.find('=');
        if (equals_pos == std::string::npos) {
            continue; // Skip lines without =
        }
        
        std::string key = trim(line.substr(0, equals_pos));
        std::string value = line.substr(equals_pos + 1);
        
        // Remove quotes if present
        value = remove_quotes(value);
        
        // Store the raw value first (needed for variable resolution)
        env_vars[key] = value;
    }
    
    // Second pass: resolve all variables and commands
    for (auto& [key, value] : env_vars) {
        env_vars[key] = resolve_value(value);
    }
    
    // Automatically set all variables in system environment
    set_system_env();
    
    return true;
}

std::string EnvParser::get(const std::string& key, const std::string& default_value) const {
    auto it = env_vars.find(key);
    return (it != env_vars.end()) ? it->second : default_value;
}

bool EnvParser::has(const std::string& key) const {
    return env_vars.find(key) != env_vars.end();
}

const std::unordered_map<std::string, std::string>& EnvParser::get_all() const {
    return env_vars;
}

void EnvParser::set_system_env() const {
    for (const auto& [key, value] : env_vars) {
        setenv(key.c_str(), value.c_str(), 1);
    }
}

void EnvParser::print_all() const {
    std::cout << "Loaded environment variables\n";
    for (const auto& [key, value] : env_vars) {
        std::cout << key << " = " << "*****" << std::endl;
    }
    std::cout << "-------------------------\n\n";
}

void EnvParser::create_env_file(const std::string& _file, const std::unordered_map<std::string, std::string>& _hash_key_vals) {
    std::ofstream file(_file);
    if (!file.is_open()) {
        throw std::runtime_error("Could not create file: " + _file);
    }
    
    for (const auto& [key, value] : _hash_key_vals) {
        file << key << "=" << value << "\n";
    }
    
    file.close();
}

void EnvParser::set(const std::string& _key, const std::string& _value, const std::string& _file) {
    // Update in-memory variables
    env_vars[_key] = _value;
    
    // Read the existing file content
    std::ifstream read_file(_file);
    std::vector<std::string> lines;
    std::string line;
    
    // If file doesn't exist, we'll create it
    if (read_file.is_open()) {
        while (std::getline(read_file, line)) {
            // Check if this line starts with the key we're setting
            size_t equals_pos = line.find('=');
            if (equals_pos != std::string::npos) {
                std::string existing_key = trim(line.substr(0, equals_pos));
                if (existing_key == _key) {
                    continue; // Skip this line as we'll add the updated version
                }
            }
            lines.push_back(line);
        }
        read_file.close();
    }
    
    // Write all lines back, with the new key-value pair
    std::ofstream write_file(_file);
    if (!write_file.is_open()) {
        throw std::runtime_error("Could not open file for writing: " + _file);
    }
    
    // Write existing lines
    for (const auto& l : lines) {
        write_file << l << "\n";
    }
    
    // Write the new/updated key-value pair
    write_file << _key << "=" << _value;
    
    write_file.close();
}
