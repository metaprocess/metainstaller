#include "x_detector.hpp"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <cctype>

namespace DisplayDetection {

DetectionResult StaticDisplayDetector::detectDisplay() {
    DetectionResult result;
    
    // Perform all detection methods
    checkEnvironment(result);
    checkFileSystem(result);
    checkProcesses(result);
    testSocketConnectivity(result);
    
    // Determine primary system based on evidence
    determinePrimarySystem(result);
    
    return result;
}

bool StaticDisplayDetector::isX11Running() {
    auto result = detectDisplay();
    return result.x_available;
}

bool StaticDisplayDetector::isWaylandRunning() {
    auto result = detectDisplay();
    return result.wayland_available;
}

DisplaySystem StaticDisplayDetector::getPrimaryDisplaySystem() {
    auto result = detectDisplay();
    return result.primary_system;
}

bool StaticDisplayDetector::testX11SocketConnectivity(int display_num) {
    const char* display_env = std::getenv("DISPLAY");
    
    // If display_num not specified, try to parse from DISPLAY
    if (display_num < 0) {
        if (!display_env) return false;
        display_num = parseDisplayNumber(display_env);
        if (display_num < 0) return false;
    }
    
    // Try to connect to X socket
    std::string socket_path = "/tmp/.X11-unix/X" + std::to_string(display_num);
    
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) return false;
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);
    
    bool connected = connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0;
    close(sock);
    
    return connected;
}

std::string StaticDisplayDetector::displaySystemToString(DisplaySystem system) {
    switch (system) {
        case DisplaySystem::NONE: return "None";
        case DisplaySystem::X11: return "X11";
        case DisplaySystem::WAYLAND: return "Wayland";
        case DisplaySystem::UNKNOWN: return "Unknown";
        default: return "Invalid";
    }
}

bool StaticDisplayDetector::stringStartsWith(const std::string& str, const std::string& prefix) {
    if (prefix.length() > str.length()) return false;
    return str.substr(0, prefix.length()) == prefix;
}

bool StaticDisplayDetector::stringContains(const std::string& str, const std::string& substr) {
    return str.find(substr) != std::string::npos;
}

std::string StaticDisplayDetector::stringToLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

void StaticDisplayDetector::checkEnvironment(DetectionResult& result) {
    const char* display = std::getenv("DISPLAY");
    const char* wayland = std::getenv("WAYLAND_DISPLAY");
    const char* xdg_session = std::getenv("XDG_SESSION_TYPE");
    const char* desktop_session = std::getenv("DESKTOP_SESSION");
    const char* gdm_session = std::getenv("GDMSESSION");
    
    if (display) {
        result.display_info = display;
        result.found_evidence.push_back("DISPLAY=" + std::string(display));
        
        // Basic validation of DISPLAY format
        if (display[0] == ':' || (display[0] != '\0' && strchr(display, ':'))) {
            result.x_available = true;
        }
    }
    
    if (wayland) {
        result.wayland_display = wayland;
        result.wayland_available = true;
        result.found_evidence.push_back("WAYLAND_DISPLAY=" + std::string(wayland));
    }
    
    if (xdg_session) {
        result.session_type = xdg_session;
        result.found_evidence.push_back("XDG_SESSION_TYPE=" + std::string(xdg_session));
        
        std::string session_type = stringToLower(xdg_session);
        
        if (session_type == "x11") {
            result.x_available = true;
        } else if (session_type == "wayland") {
            result.wayland_available = true;
        }
    }
    
    // Check additional session indicators
    if (desktop_session) {
        std::string session = stringToLower(desktop_session);
        result.found_evidence.push_back("DESKTOP_SESSION=" + session);
        
        if (stringContains(session, "wayland")) {
            result.wayland_available = true;
        }
    }
    
    if (gdm_session) {
        std::string session = gdm_session;
        result.found_evidence.push_back("GDMSESSION=" + session);
        
        if (stringContains(stringToLower(session), "wayland")) {
            result.wayland_available = true;
        }
    }
}

void StaticDisplayDetector::checkFileSystem(DetectionResult& result) {
    namespace fs = std::filesystem;
    
    // Check X11 sockets
    const std::string x11_dir = "/tmp/.X11-unix";
    if (fs::exists(x11_dir) && fs::is_directory(x11_dir)) {
        result.found_evidence.push_back("X11 socket directory exists");
        
        try {
            for (const auto& entry : fs::directory_iterator(x11_dir)) {
                if (!entry.is_socket() && !entry.is_regular_file()) continue;
                
                std::string filename = entry.path().filename();
                if (stringStartsWith(filename, "X") && filename.length() > 1) {
                    result.x_available = true;
                    result.x_sockets.push_back(filename);
                    result.found_evidence.push_back("X11 socket: " + filename);
                }
            }
        } catch (const std::exception& e) {
            result.found_evidence.push_back("X11 socket directory not accessible: " + std::string(e.what()));
        }
    }
    
    // Check X11 lock files
    for (int i = 0; i < 20; ++i) {
        std::string lock_file = "/tmp/.X" + std::to_string(i) + "-lock";
        if (fs::exists(lock_file)) {
            result.x_available = true;
            result.x_lock_files.push_back(lock_file);
            result.found_evidence.push_back("X11 lock file: " + lock_file);
        }
    }
    
    // Check Wayland socket
    if (!result.wayland_display.empty()) {
        uint32_t uid = getCurrentUserId();
        std::vector<std::string> wayland_paths = {
            "/run/user/" + std::to_string(uid) + "/" + result.wayland_display,
            "/tmp/" + result.wayland_display,
            "/var/run/user/" + std::to_string(uid) + "/" + result.wayland_display
        };
        
        for (const auto& path : wayland_paths) {
            if (fs::exists(path)) {
                result.wayland_available = true;
                result.found_evidence.push_back("Wayland socket: " + path);
                break;
            }
        }
    }
    
    // Check for common Wayland compositor sockets even without WAYLAND_DISPLAY
    uint32_t uid = getCurrentUserId();
    std::string user_runtime = "/run/user/" + std::to_string(uid);
    if (fs::exists(user_runtime) && fs::is_directory(user_runtime)) {
        try {
            for (const auto& entry : fs::directory_iterator(user_runtime)) {
                std::string filename = entry.path().filename();
                if (stringStartsWith(filename, "wayland-")) {
                    result.wayland_available = true;
                    result.found_evidence.push_back("Wayland socket found: " + filename);
                }
            }
        } catch (...) {
            // Directory might not be accessible
        }
    }
}

void StaticDisplayDetector::checkProcesses(DetectionResult& result) {
    std::vector<std::string> x_processes = {
        "Xorg", "X", "Xwayland", "xinit", "startx"
    };
    
    std::vector<std::string> wayland_processes = {
        "weston", "sway", "gnome-shell", "kwin_wayland", "river", 
        "hyprland", "wayfire", "mutter", "cosmic-comp"
    };
    
    // Check X processes
    for (const auto& proc : x_processes) {
        if (isProcessRunning(proc)) {
            result.x_available = true;
            result.x_processes.push_back(proc);
            result.found_evidence.push_back("X11 process running: " + proc);
        }
    }
    
    // Check Wayland processes
    for (const auto& proc : wayland_processes) {
        if (isProcessRunning(proc)) {
            result.wayland_available = true;
            result.wayland_processes.push_back(proc);
            result.found_evidence.push_back("Wayland process running: " + proc);
        }
    }
}

void StaticDisplayDetector::testSocketConnectivity(DetectionResult& result) {
    if (!result.display_info.empty()) {
        int display_num = parseDisplayNumber(result.display_info);
        if (display_num >= 0) {
            bool connected = testX11SocketConnectivity(display_num);
            result.socket_test_passed = connected;
            
            if (connected) {
                result.x_available = true;
                result.found_evidence.push_back("X11 socket connectivity test passed");
            } else {
                result.socket_test_error = "Failed to connect to X11 socket";
                result.found_evidence.push_back("X11 socket connectivity test failed");
            }
        }
    }
}

bool StaticDisplayDetector::isProcessRunning(const std::string& process_name) {
    return !getProcessPids(process_name).empty();
}

std::vector<uint32_t> StaticDisplayDetector::getProcessPids(const std::string& process_name) {
    std::vector<uint32_t> pids;
    namespace fs = std::filesystem;
    
    try {
        for (const auto& entry : fs::directory_iterator("/proc")) {
            if (!entry.is_directory()) continue;
            
            std::string dirname = entry.path().filename();
            if (dirname.find_first_not_of("0123456789") != std::string::npos) {
                continue; // Not a PID directory
            }
            
            std::string comm_path = entry.path() / "comm";
            std::ifstream comm_file(comm_path);
            if (comm_file.is_open()) {
                std::string comm;
                std::getline(comm_file, comm);
                if (comm == process_name) {
                    pids.push_back(static_cast<uint32_t>(std::stoul(dirname)));
                }
            }
        }
    } catch (const std::exception&) {
        // /proc might not be accessible or other filesystem error
    }
    
    return pids;
}

int StaticDisplayDetector::parseDisplayNumber(const std::string& display_str) {
    if (display_str.empty()) return -1;
    
    // Find the colon
    size_t colon_pos = display_str.find(':');
    if (colon_pos == std::string::npos) return -1;
    
    // Extract the number after colon
    std::string number_part = display_str.substr(colon_pos + 1);
    
    // Remove screen number if present (e.g., ":0.0" -> "0")
    size_t dot_pos = number_part.find('.');
    if (dot_pos != std::string::npos) {
        number_part = number_part.substr(0, dot_pos);
    }
    
    try {
        return std::stoi(number_part);
    } catch (const std::exception&) {
        return -1;
    }
}

uint32_t StaticDisplayDetector::getCurrentUserId() {
    return static_cast<uint32_t>(getuid());
}

void StaticDisplayDetector::determinePrimarySystem(DetectionResult& result) {
    // Priority: Wayland > X11 > None
    // This reflects modern preference for Wayland
    
    if (result.wayland_available && result.x_available) {
        // Both available, check which seems more active
        if (!result.wayland_processes.empty() && result.wayland_processes.size() >= result.x_processes.size()) {
            result.primary_system = DisplaySystem::WAYLAND;
        } else {
            result.primary_system = DisplaySystem::X11;
        }
    } else if (result.wayland_available) {
        result.primary_system = DisplaySystem::WAYLAND;
    } else if (result.x_available) {
        result.primary_system = DisplaySystem::X11;
    } else if (!result.found_evidence.empty()) {
        result.primary_system = DisplaySystem::UNKNOWN;
    } else {
        result.primary_system = DisplaySystem::NONE;
    }
}

} // namespace DisplayDetection