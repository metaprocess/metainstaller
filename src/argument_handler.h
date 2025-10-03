#ifndef ARGUMENT_HANDLER_H
#define ARGUMENT_HANDLER_H

#include <string>
#include <vector>
#include <variant>

// Define a variant type to hold the argument value
using ArgumentValue = std::variant<bool, std::string, int, float>;

// Define a structure to hold the argument metadata
struct ArgumentMetadata {
    std::string name;
    std::string description;
    ArgumentValue defaultValue;
    bool required = false;
};

class ArgumentHandler {
public:
    // Add a boolean argument
    void addBooleanArgument(const std::string& name, const std::string& description, bool defaultValue = false, bool required = false);

    // Add a string argument
    void addStringArgument(const std::string& name, const std::string& description, const std::string& defaultValue = "", bool required = false);

    // Add an integer argument
    void addIntegerArgument(const std::string& name, const std::string& description, int defaultValue = 0, bool required = false);

    // Add a float argument
    void addFloatArgument(const std::string& name, const std::string& description, float defaultValue = 0.0f, bool required = false);

    // Parse the command line arguments
    void parseArguments(int argc, char* argv[]);

    // Get the argument value
    ArgumentValue getArgumentValue(const std::string& name) const;

    // Check if all required arguments are present
    bool areRequiredArgumentsPresent() const;

private:
    std::vector<ArgumentMetadata> arguments_;
};

/**
 * example using ArgumentHandler class
 * int main(int argc, char* argv[]) {
    ArgumentHandler handler;

    handler.addBooleanArgument("help", "Display help message", false, false);
    handler.addStringArgument("input", "Input file", "", true);
    handler.addIntegerArgument("iterations", "Number of iterations", 10, false);
    handler.addFloatArgument("threshold", "Threshold value", 0.5f, false);

    try {
        handler.parseArguments(argc, argv);

        if (!handler.areRequiredArgumentsPresent()) {
            std::cerr << "Error: Missing required arguments." << std::endl;
            return 1;
        }

        // Get the argument values
        bool help = std::get<bool>(handler.getArgumentValue("help"));
        std::string input = std::get<std::string>(handler.getArgumentValue("input"));
        int iterations = std::get<int>(handler.getArgumentValue("iterations"));
        float threshold = std::get<float>(handler.getArgumentValue("threshold"));

        // Print the argument values
        std::cout << "Help: " << (help ? "yes" : "no") << std::endl;
        std::cout << "Input file: " << input << std::endl;
        std::cout << "Number of iterations: " << iterations << std::endl;
        std::cout << "Threshold value: " << threshold << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
 * 
 * 
 */

#endif  // ARGUMENT_HANDLER_H