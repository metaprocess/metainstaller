#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include <iostream>
#include <vector>
#include <string>
#include <array>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <thread>
#include <functional>
#include <map>
#include <atomic>  // Add this include

class ProcessManager {
public:
    ProcessManager();
    ~ProcessManager();

    // Special status code returned by startProcessBlockingAsRoot when sudo authentication fails
    static constexpr int STATUS_SUDO_AUTH_FAILED = 10001;

    // Start process normally
    pid_t startProcess(const std::string& command, const std::vector<std::string>& args = std::vector<std::string>(), const std::map<std::string, std::string>& env = std::map<std::string, std::string>(), std::function<void(const std::string&)> outputCallback = nullptr, std::function<void(void)> callback_termination = nullptr, const std::string& working_directory = "");
    std::tuple<pid_t, int> startProcessBlocking(const std::string& command, const std::vector<std::string>& args = std::vector<std::string>(), const std::map<std::string, std::string>& env = std::map<std::string, std::string>(), std::function<void(const std::string&)> outputCallback = nullptr, const std::string& working_directory = "");

    // Start process with sudo (password read via stdin).
    // In blocking variant, returns STATUS_SUDO_AUTH_FAILED when the sudo password is incorrect.
    pid_t startProcessAsRoot(const std::string& command, const std::vector<std::string>& args , const std::map<std::string, std::string>& env, const std::string& sudoPassword, std::function<void(const std::string&)> outputCallback = nullptr, const std::string& working_directory = "");
    std::tuple<pid_t, int> startProcessBlockingAsRoot(const std::string& command, const std::vector<std::string>& args, const std::map<std::string, std::string>& env, const std::string& sudoPassword, std::function<void(const std::string&)> outputCallback = nullptr, const std::string& working_directory = "");

    void writeToProcess(const std::string& data);
    void killProcess();

private:
    void readFromProcess(std::function<void(const std::string&)> outputCallback, std::function<void(void)> callback_termination = nullptr);

    pid_t pid;
    std::atomic<int> stdin_fd;  // Change to atomic
    std::atomic<int> stdout_fd; // Change to atomic
    std::atomic<int> stderr_fd; // Change to atomic
    std::thread read_thread;    // Uncomment and use
};

#endif // PROCESSMANAGER_H
