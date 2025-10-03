#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace DisplayDetection {

/**
 * @brief Enumeration of supported display systems
 */
enum class DisplaySystem {
    NONE,           ///< No display system detected
    X11,            ///< X11/Xorg display system
    WAYLAND,        ///< Wayland display system
    UNKNOWN         ///< Display system detected but type unknown
};

/**
 * @brief Detailed result of display system detection
 */
struct DetectionResult {
    bool x_available = false;           ///< X11 is available and accessible
    bool wayland_available = false;     ///< Wayland is available and accessible
    DisplaySystem primary_system = DisplaySystem::NONE;  ///< Primary detected system
    
    std::string display_info;           ///< DISPLAY environment variable content
    std::string wayland_display;        ///< WAYLAND_DISPLAY environment variable content
    std::string session_type;           ///< XDG_SESSION_TYPE environment variable content
    
    std::vector<std::string> found_evidence;  ///< List of detection evidence
    std::vector<std::string> x_processes;     ///< Running X-related processes
    std::vector<std::string> wayland_processes; ///< Running Wayland-related processes
    std::vector<std::string> x_sockets;       ///< Found X11 socket files
    std::vector<std::string> x_lock_files;    ///< Found X11 lock files
    
    bool socket_test_passed = false;    ///< Whether socket connectivity test passed
    std::string socket_test_error;      ///< Error message from socket test if failed
};

/**
 * @brief Static X11 and Wayland display system detector
 * 
 * This class provides comprehensive detection of display systems without
 * requiring any external libraries. It uses multiple detection strategies
 * including environment variables, file system evidence, process detection,
 * and socket connectivity tests.
 */
class StaticDisplayDetector {
public:
    /**
     * @brief Detect available display systems
     * @return Comprehensive detection results
     */
    static DetectionResult detectDisplay();
    
    /**
     * @brief Simple check if X11 is running
     * @return true if X11 is detected and accessible
     */
    static bool isX11Running();
    
    /**
     * @brief Simple check if Wayland is running
     * @return true if Wayland is detected and accessible
     */
    static bool isWaylandRunning();
    
    /**
     * @brief Get the primary display system
     * @return The primary detected display system
     */
    static DisplaySystem getPrimaryDisplaySystem();
    
    /**
     * @brief Test X11 socket connectivity without X11 libraries
     * @param display_num Display number to test (default: auto-detect from DISPLAY)
     * @return true if can connect to X11 socket
     */
    static bool testX11SocketConnectivity(int display_num = -1);
    
    /**
     * @brief Get human-readable string for DisplaySystem enum
     * @param system The display system enum value
     * @return String representation
     */
    static std::string displaySystemToString(DisplaySystem system);

private:
    /**
     * @brief Check if string starts with given prefix (C++17 compatible)
     * @param str String to check
     * @param prefix Prefix to look for
     * @return true if str starts with prefix
     */
    static bool stringStartsWith(const std::string& str, const std::string& prefix); // Check if str begins with prefix
    
    /**
     * @brief Check if string contains given substring
     * @param str String to check
     * @param substr Substring to look for
     * @return true if str contains substr
     */
    static bool stringContains(const std::string& str, const std::string& substr); // Return true if substr is found in str
    
    /**
     * @brief Convert string to lowercase
     * @param str String to convert
     * @return Lowercase version of the string
     */
    static std::string stringToLower(const std::string& str); // Convert str to lowercase
    
    /**
     * @brief Check environment variables for display system evidence
     * @param result Reference to detection result to populate
     */
    static void checkEnvironment(DetectionResult& result); // Populate result with environment variable evidence
    
    /**
     * @brief Check file system for display system evidence
     * @param result Reference to detection result to populate
     */
    static void checkFileSystem(DetectionResult& result); // Check for display system evidence on file system
    
    /**
     * @brief Check running processes for display system evidence
     * @param result Reference to detection result to populate
     */
    static void checkProcesses(DetectionResult& result); // Check running processes for display system clues
    
    /**
     * @brief Test socket connectivity
     * @param result Reference to detection result to populate
     */
    static void testSocketConnectivity(DetectionResult& result); // Test connectivity to display system sockets
    
    /**
     * @brief Check if a specific process is running
     * @param process_name Name of the process to check
     * @return true if process is running
     */
    static bool isProcessRunning(const std::string& process_name); // Check if process_name is currently running
    
    /**
     * @brief Get all running processes with given name
     * @param process_name Name of the process to find
     * @return Vector of PIDs where the process is running
     */
    static std::vector<uint32_t> getProcessPids(const std::string& process_name); // Get PIDs of running processes matching process_name
    
    /**
     * @brief Parse display number from DISPLAY environment variable
     * @param display_str DISPLAY variable content (e.g., ":0", ":1.0")
     * @return Display number, or -1 if parsing failed
     */
    static int parseDisplayNumber(const std::string& display_str); // Extract display number from DISPLAY variable
    
    /**
     * @brief Get current user ID
     * @return User ID
     */
    static uint32_t getCurrentUserId(); // Retrieve the current user's ID
    
    /**
     * @brief Determine primary display system from detection results
     * @param result Detection result to analyze
     */
    static void determinePrimarySystem(DetectionResult& result); // Analyze result to determine primary display system
};

} // namespace DisplayDetection
