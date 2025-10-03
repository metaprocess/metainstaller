#ifndef ENV_CONFIG_HPP
#define ENV_CONFIG_HPP

#include <string>
#include <map>
#include <vector>

/**
 * @brief Enumeration of all supported environment variables
 */
enum class EnvKey {
    REST_PORT = 1,
    LOG_LEVEL,
    SUDO_PASSWORD
};

// No hash specialization needed for std::map

/**
 * @brief Structure to hold environment variable metadata
 */
struct EnvVariable {
    EnvKey key;
    std::string name;
    std::string default_value;
    std::string description;
    
    EnvVariable(EnvKey k, const std::string& n, const std::string& def_val, const std::string& desc)
        : key(k), name(n), default_value(def_val), description(desc) {}
};

/**
 * @brief Centralized environment configuration management class
 * 
 * This class provides a structured way to manage environment variables,
 * their default values, and automatic .env file generation.
 */
class EnvConfig {
private:
    static std::map<EnvKey, EnvVariable> env_map;
    
    /**
     * @brief Initialize the environment variables registry
     */
    static void initialize_env_variables();
    
    /**
     * @brief Ensure the environment variables are initialized
     */
    static void ensure_initialized();

public:
    /**
     * @brief Get the string name of an environment key
     * @param key The environment key
     * @return The string name of the key
     */
    static std::string get_key_name(EnvKey key);
    
    /**
     * @brief Get the default value of an environment key
     * @param key The environment key
     * @return The default value for the key
     */
    static std::string get_default_value(EnvKey key);
    
    /**
     * @brief Get the description of an environment key
     * @param key The environment key
     * @return The description of the key
     */
    static std::string get_description(EnvKey key);
    
    /**
     * @brief Get the current value of an environment variable
     * @param key The environment key
     * @return The current value (from system env or default)
     */
    static std::string get_value(EnvKey key);
    
    /**
     * @brief Get the current value of an environment variable as integer
     * @param key The environment key
     * @return The current value converted to integer
     */
    static int get_int_value(EnvKey key);
    
    /**
     * @brief Create a default .env file with all configured environment variables
     * @param file_path Path to the .env file (default: ".env")
     * @return true if file was created successfully, false otherwise
     */
    static bool create_default_env_file(const std::string& file_path = ".env");
    
    /**
     * @brief Get all environment variables
     * @return Map of all environment variables
     */
    static const std::map<EnvKey, EnvVariable>& get_all_variables();
    
    /**
     * @brief Check if an .env file exists and create it if it doesn't
     * @param file_path Path to the .env file (default: ".env")
     * @return true if file exists or was created successfully
     */
    static bool ensure_env_file_exists(const std::string& file_path = ".env");

    /**
     * @brief Set or update a value in the .env file
     * @param key The environment key to set
     * @param value The value to set
     * @param file_path Path to the .env file (default: ".env")
     * @return true if the value was set successfully, false otherwise
     */
    static bool set_value(EnvKey key, const std::string& value, const std::string& file_path = ".env");
};

#endif // ENV_CONFIG_HPP
