#include "argument_handler.h"
#include <iostream>
#include <stdexcept>
#include <optional>
#include <algorithm>

void ArgumentHandler::addBooleanArgument(const std::string& name, const std::string& description, bool defaultValue, bool required) {
    arguments_.emplace_back(ArgumentMetadata{name, description, defaultValue, required});
}

void ArgumentHandler::addStringArgument(const std::string& name, const std::string& description, const std::string& defaultValue, bool required) {
    arguments_.emplace_back(ArgumentMetadata{name, description, defaultValue, required});
}

void ArgumentHandler::addIntegerArgument(const std::string& name, const std::string& description, int defaultValue, bool required) {
    arguments_.emplace_back(ArgumentMetadata{name, description, defaultValue, required});
}

void ArgumentHandler::addFloatArgument(const std::string& name, const std::string& description, float defaultValue, bool required) {
    arguments_.emplace_back(ArgumentMetadata{name, description, defaultValue, required});
}

void ArgumentHandler::parseArguments(int argc, char* argv[]) {
    std::vector<std::string> parsedArguments;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if(arg[0] == '-') {
            // Find the argument metadata
            auto it = std::find_if(arguments_.begin(), arguments_.end(), [&](const ArgumentMetadata& metadata) {
                auto _substr = arg.substr(2);
                return metadata.name == _substr;
            });

            if(it != arguments_.end()) {
                // Check if the argument has a value
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    // Parse the argument value
                    if(std::holds_alternative<bool>(it->defaultValue)) 
                    {
                        auto _val = std::stoi(argv[++i]);
                        it->defaultValue = (_val >0 ? true : false);
                    }
                    else if(std::holds_alternative<std::string>(it->defaultValue))
                    {
                        it->defaultValue = std::string(argv[++i]);
                    } else if (std::holds_alternative<int>(it->defaultValue)) {
                        it->defaultValue = std::stoi(argv[++i]);
                    } else if (std::holds_alternative<float>(it->defaultValue)) {
                        it->defaultValue = std::stof(argv[++i]);
                    }
                } else {
                    // If the argument doesn't have a value, set it to true
                    if (std::holds_alternative<bool>(it->defaultValue)) {
                        it->defaultValue = true;
                    } else {
                        throw std::invalid_argument("Missing value for argument '" + it->name + "'");
                    }
                }
                // Add the argument to the parsed arguments vector
                parsedArguments.push_back(it->name);
            } else {
                throw std::invalid_argument("Unknown argument '" + arg + "'");
            }
        }
    }

    for (const auto& metadata : arguments_) {
        if (metadata.required) {
            if (std::find(parsedArguments.begin(), parsedArguments.end(), metadata.name) == parsedArguments.end()) {
                throw std::invalid_argument("Missing required argument '" + metadata.name + "'");
            }
        }
    }

    // if (!areRequiredArgumentsPresent()) {
    //     std::cerr << "Error: Missing required arguments." << std::endl;
    //     throw std::runtime_error("Error: Missing required arguments.");
    // }
}
ArgumentValue ArgumentHandler::getArgumentValue(const std::string& name) const {
    auto it = std::find_if(arguments_.begin(), arguments_.end(), [&](const ArgumentMetadata& metadata) {
        return metadata.name == name;
    });

    if (it != arguments_.end()) {
        return it->defaultValue;
    } else {
        throw std::invalid_argument("Unknown argument " + name);
    }
}

bool ArgumentHandler::areRequiredArgumentsPresent() const {
    for (const auto& metadata : arguments_) {
        if (metadata.required) {
            if (std::holds_alternative<bool>(metadata.defaultValue) && !std::get<bool>(metadata.defaultValue)) {
                return false;
            } else if (std::holds_alternative<std::string>(metadata.defaultValue) && std::get<std::string>(metadata.defaultValue).empty()) {
                return false;
            } else if (std::holds_alternative<int>(metadata.defaultValue) && std::get<int>(metadata.defaultValue) == 0) {
                return false;
            } else if (std::holds_alternative<float>(metadata.defaultValue) && std::get<float>(metadata.defaultValue) == 0.0f) {
                return false;
            }
        }
    }
    return true;
}