#ifndef ENV_PARSER_HPP
#define ENV_PARSER_HPP

#include <string>
#include <unordered_map>

/**
 * @brief A class for parsing .env files with support for variable substitution and command execution
 * 
 * This class can handle:
 * - Basic key-value pairs (KEY=value)
 * - Variable substitution (${VAR} and $VAR)
 * - Command execution ($(command))
 * - Comments (lines starting with #)
 * - Quoted values
 * - Automatic system environment variable setting
 */
class EnvParser {
private:
    std::unordered_map<std::string, std::string> env_vars;
    
    /**
     * @brief Execute a shell command and return its output
     * @param command The command to execute
     * @return The command output as a string
     * @throws std::runtime_error if command execution fails
     */
    std::string execute_command(const std::string& command);

    
    /**
     * @brief Trim whitespace from both ends of a string
     * @param str The string to trim
     * @return Trimmed string
     */
    std::string trim(const std::string& str);
    
    /**
     * @brief Remove surrounding quotes from a string if present
     * @param value The string to process
     * @return String without surrounding quotes
     */
    std::string remove_quotes(const std::string& value);
    
    /**
     * @brief Resolve variable substitutions and command executions in a value
     * @param value The value to resolve
     * @return Resolved value with all substitutions performed
     */
    std::string resolve_value(const std::string& value);
    
public:

    /**
     * @brief Create a .env file with the given key-value pairs
     * @param _file Path to the .env file to create
     * @param _hash_key_vals Map of key-value pairs to write to the file
     * @throws std::runtime_error if file creation fails
     */
    void create_env_file(const std::string& _file, const std::unordered_map<std::string, std::string>& _hash_key_vals);
    
    /**
     * @brief Default constructor
     */
    EnvParser() = default;

    /**
     * @brief Set a key-value pair in the environment variables and save to file
     * @param _key The key to set
     * @param _value The value to set
     * @param _file Optional path to .env file to save to (default: ".env")
     * @throws std::runtime_error if file writing fails
     */
    void set(const std::string& _key, const std::string& _value, const std::string& _file = ".env");
    
    /**
     * @brief Load and parse a .env file
     * @param filename Path to the .env file (default: ".env")
     * @return true if file was loaded successfully, false otherwise
     * 
     * This method automatically sets all loaded variables in the system environment
     * so they can be accessed via std::getenv()
     */
    bool load_env_file(const std::string& filename = ".env");
    
    /**
     * @brief Get the value of an environment variable
     * @param key The variable name
     * @param default_value Default value if key is not found
     * @return The variable value or default_value if not found
     */
    std::string get(const std::string& key, const std::string& default_value = "") const;
    
    /**
     * @brief Check if a key exists in the loaded environment variables
     * @param key The variable name to check
     * @return true if key exists, false otherwise
     */
    bool has(const std::string& key) const;
    
    /**
     * @brief Get all loaded environment variables
     * @return Reference to the internal map containing all variables
     */
    const std::unordered_map<std::string, std::string>& get_all() const;
    
    /**
     * @brief Set all loaded environment variables in the system environment
     * 
     * This method is automatically called by load_env_file(), but can be
     * called manually if needed
     */
    void set_system_env() const;
    
    /**
     * @brief Print all loaded variables to stdout (for debugging)
     */
    void print_all() const;
};

#endif // ENV_PARSER_HPP
