#include <sys/prctl.h>
#include <future>
#include <sstream>
#include <poll.h>
#include <algorithm>
#include <cctype>
#include "ProcessManager.h"

ProcessManager::ProcessManager() : pid(-1), stdin_fd(-1), stdout_fd(-1), stderr_fd(-1) {}

ProcessManager::~ProcessManager() {
    killProcess();
}

pid_t ProcessManager::startProcess(const std::string& command, const std::vector<std::string>& args, const std::map<std::string, std::string>& env, std::function<void(const std::string&)> outputCallback, std::function<void(void)> callback_termination, const std::string& working_directory) {

    if (pid > 0) {
        // Terminate process
        killProcess();
    }

    pid_t child_pid;
    int pipe_stdin[2], pipe_stdout[2], pipe_stderr[2];

    if (pipe(pipe_stdin) == -1 || pipe(pipe_stdout) == -1 || pipe(pipe_stderr) == -1) {
        perror("pipe");
        return -1;
    }

    child_pid = fork();

    if (child_pid == -1) {
        perror("fork");
        return -1;
    } else if (child_pid == 0) {
        // Child process
        close(pipe_stdin[1]);
        close(pipe_stdout[0]);
        close(pipe_stderr[0]);
        dup2(pipe_stdin[0], STDIN_FILENO);
        dup2(pipe_stdout[1], STDOUT_FILENO);
        dup2(pipe_stderr[1], STDERR_FILENO);

        // Change to working directory if specified
        if (!working_directory.empty()) {
            chdir(working_directory.c_str());
        }

        // Set environment variables
        for (const auto& pair : env) {
            setenv(pair.first.c_str(), pair.second.c_str(), 1);
        }

        std::vector<char*> c_args;
        c_args.push_back(const_cast<char*>(command.c_str()));
        for (const auto& arg : args) {
            c_args.push_back(const_cast<char*>(arg.c_str()));
        }
        c_args.push_back(nullptr);

        execvp(command.c_str(), c_args.data());
        perror("execvp");
        _exit(1);
    } else {
        // Parent process
        close(pipe_stdin[0]);
        close(pipe_stdout[1]);
        close(pipe_stderr[1]);
        stdin_fd = pipe_stdin[1];
        stdout_fd = pipe_stdout[0];
        stderr_fd = pipe_stderr[0];

        // std::thread read_thread(&ProcessManager::readFromProcess, this, outputCallback);
        read_thread = std::thread(&ProcessManager::readFromProcess, this, outputCallback, callback_termination);
        // read_thread.detach();
        pid = child_pid;
        return pid;
    }

    // pid = child_pid;

    // // Start a thread to read from stdout and call the callback
    // std::thread* read_thread{nullptr};
    // read_thread = new std::thread([&command, this, outputCallback, &read_thread]() {
    //     auto _str = command.substr(0, 16);
    //     Utils::setThreadName(_str.c_str());
    //     readFromProcess(outputCallback);
    //     killProcess();
    //     // delete read_thread;
    //     read_thread = nullptr;
    //     // std::cerr << "exiting Process manager read loop\n";
    // });

    // return child_pid;
}

std::tuple<pid_t, int> ProcessManager::startProcessBlocking(const std::string& command, const std::vector<std::string>& args, const std::map<std::string, std::string>& env, std::function<void(const std::string&)> outputCallback, const std::string& working_directory) {
    int pipe_stdout[2], pipe_stderr[2];
    if (pipe(pipe_stdout) == -1 || pipe(pipe_stderr) == -1) {
        perror("pipe");
        return std::make_tuple(-1, -1);
    }

    pid_t child_pid = fork();
    if (child_pid == -1) {
        perror("fork");
        close(pipe_stdout[0]);
        close(pipe_stdout[1]);
        close(pipe_stderr[0]);
        close(pipe_stderr[1]);
        return std::make_tuple(-1, -1);
    } else if (child_pid == 0) {
        // Child process
        close(pipe_stdout[0]); // Close read end
        close(pipe_stderr[0]); // Close read end
        dup2(pipe_stdout[1], STDOUT_FILENO);
        dup2(pipe_stderr[1], STDERR_FILENO);
        close(pipe_stdout[1]);
        close(pipe_stderr[1]);

        // Change to working directory if specified
        if (!working_directory.empty()) {
            chdir(working_directory.c_str());
        }

        // Redirect stdin from /dev/null since we're not writing to it
        int devnull = open("/dev/null", O_RDONLY);
        if (devnull != -1) {
            dup2(devnull, STDIN_FILENO);
            close(devnull);
        }

        // Set environment variables
        for (const auto& pair : env) {
            setenv(pair.first.c_str(), pair.second.c_str(), 1);
        }

        // Prepare arguments
        std::vector<char*> c_args;
        c_args.push_back(const_cast<char*>(command.c_str()));
        for (const auto& arg : args) {
            c_args.push_back(const_cast<char*>(arg.c_str()));
        }
        c_args.push_back(nullptr);

        execvp(command.c_str(), c_args.data());
        perror("execvp");
        _exit(1);
    } else {
        // Parent process
        close(pipe_stdout[1]); // Close write end
        close(pipe_stderr[1]); // Close write end

        int stdout_fd = pipe_stdout[0];
        int stderr_fd = pipe_stderr[0];

        std::array<char, 128> buffer;
        ssize_t bytes_read;
        fd_set read_fds;
        int max_fd = std::max(stdout_fd, stderr_fd);
        bool stdout_open = true;
        bool stderr_open = true;

        while (stdout_open || stderr_open) {
            FD_ZERO(&read_fds);
            if (stdout_open) FD_SET(stdout_fd, &read_fds);
            if (stderr_open) FD_SET(stderr_fd, &read_fds);

            if (select(max_fd + 1, &read_fds, nullptr, nullptr, nullptr) == -1) {
                perror("select");
                break;
            }

            if (FD_ISSET(stdout_fd, &read_fds)) {
                bytes_read = read(stdout_fd, buffer.data(), buffer.size());
                if (bytes_read > 0) {
                    std::string result(buffer.data(), bytes_read);
                    if (outputCallback) {
                        outputCallback(result);
                    }
                } else if (bytes_read == 0) {
                    stdout_open = false;
                }
            }

            if (FD_ISSET(stderr_fd, &read_fds)) {
                bytes_read = read(stderr_fd, buffer.data(), buffer.size());
                if (bytes_read > 0) {
                    std::string result(buffer.data(), bytes_read);
                    if (outputCallback) {
                        outputCallback(result);
                    }
                } else if (bytes_read == 0) {
                    stderr_open = false;
                }
            }
        }

        close(stdout_fd);
        close(stderr_fd);

        int status;
        waitpid(child_pid, &status, 0);


        std::stringstream _ss;
        _ss << "\t\t[RUN]: '" << command ;
        for(const auto& _arg: args)
        {
            _ss << " " << _arg;
        }
        _ss << "'";
        std::cerr << _ss.str() << " --> status: " << status << ", PID: " << child_pid << "\n";

        return std::make_tuple(child_pid, status);
    }
}


void ProcessManager::writeToProcess(const std::string &data)
{
    if (stdin_fd == -1) {
        std::cerr << "Process not started or stdin not available" << std::endl;
        return;
    }
    auto _ret = write(stdin_fd, data.c_str(), data.size());
    (void)_ret;
}

void ProcessManager::killProcess() {
    if (pid > 0) {
        // Terminate process
        kill(pid, SIGTERM);
        waitpid(pid, nullptr, 0);
        pid = -1;
    }

    // Close file descriptors (atomic exchange prevents double-close)
    int stdin_val = stdin_fd.exchange(-1);
    if (stdin_val != -1) close(stdin_val);
    
    int stdout_val = stdout_fd.exchange(-1);
    if (stdout_val != -1) close(stdout_val);
    
    int stderr_val = stderr_fd.exchange(-1);
    if (stderr_val != -1) close(stderr_val);

    // Join thread if running
    if (read_thread.joinable()) {
        read_thread.join();
    }


    // if (pid > 0) {
    //     kill(pid, SIGTERM);
    //     waitpid(pid, nullptr, 0);
    //     if (stdin_fd != -1) close(stdin_fd);
    //     if (stdout_fd != -1) close(stdout_fd);
    //     if (stderr_fd != -1) close(stderr_fd);
    //     pid = -1;
    //     stdin_fd = -1;
    //     stdout_fd = -1;
    //     stderr_fd = -1;
    // }
}

void ProcessManager::readFromProcess(std::function<void(const std::string&)> outputCallback, std::function<void(void)> callback_termination) {
    std::array<char, 128> buffer;
    ssize_t bytes_read;

    static int64_t _counter = 1;
    std::stringstream _ss;
    _ss << "PMGR.RD_" << _counter;
    _counter++;
    prctl(PR_SET_NAME, _ss.str().c_str(), 0, 0, 0);

    while (true) {
        int local_stdout = stdout_fd.load();
        int local_stderr = stderr_fd.load();
        // std::cerr << "Thread: stdout_fd=" << local_stdout << ", stderr_fd=" << local_stderr << "\n";

        if (local_stdout == -1 && local_stderr == -1) {
            // std::cerr << "Thread: Both FDs closed, exiting\n";
            break;
        }

        std::vector<pollfd> pfds;
        if (local_stdout != -1) pfds.push_back({local_stdout, POLLIN, 0});
        if (local_stderr != -1) pfds.push_back({local_stderr, POLLIN, 0});
        if (pfds.empty()) {
            // std::cerr << "Thread: No FDs to poll, exiting\n";
            break;
        }

        // std::cerr << "Thread: Polling " << pfds.size() << " FDs\n";
        int ready = poll(pfds.data(), pfds.size(), -1);
        if (ready == -1) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }

        for (auto& pfd : pfds) {
            // std::cerr << "Thread: FD " << pfd.fd << " events: " << pfd.revents << "\n";
            if (pfd.revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)) {
                if (pfd.revents & POLLIN) {
                    bytes_read = read(pfd.fd, buffer.data(), buffer.size());
                    // std::cerr << "Thread: Read " << bytes_read << " bytes from FD " << pfd.fd << "\n";
                    if (bytes_read > 0 && outputCallback) {
                        outputCallback(std::string(buffer.data(), bytes_read));
                    }
                }
                if (!(pfd.revents & POLLIN) || bytes_read <= 0) {
                    // std::cerr << "Thread: Closing FD " << pfd.fd << "\n";
                    if (pfd.fd == local_stdout) {
                        int old = stdout_fd.exchange(-1);
                        if (old != -1) close(old);
                    } else if (pfd.fd == local_stderr) {
                        int old = stderr_fd.exchange(-1);
                        if (old != -1) close(old);
                    }
                }
            }
        }
    }
    std::cerr << "Thread: Exiting readFromProcess\n";
    if(callback_termination)
    {
        callback_termination();
    }
}



// Start process via sudo (non-blocking). Writes the password to sudo's stdin right after spawn.
pid_t ProcessManager::startProcessAsRoot(const std::string& command,
                                         const std::vector<std::string>& args,
                                         const std::map<std::string, std::string>& env,
                                         const std::string& sudoPassword,
                                         std::function<void(const std::string&)> outputCallback,
                                         const std::string& working_directory) {
    // Build sudo args: -S reads password from stdin, -k prevents cached credentials being used, -p "" suppresses prompt text
    std::vector<std::string> sudo_args = {"-S", "-k", "-p", "", "--", command};
    for (const auto& a : args) sudo_args.push_back(a);

    pid_t child = startProcess("sudo", sudo_args, env, outputCallback, nullptr, working_directory);
    if (child > 0 && !sudoPassword.empty()) {
        std::string pw = sudoPassword;
        pw.push_back('\n');
        writeToProcess(pw);
    }
    return child;
}

// Start process via sudo (blocking). Returns STATUS_SUDO_AUTH_FAILED if password is wrong.
std::tuple<pid_t, int> ProcessManager::startProcessBlockingAsRoot(const std::string& command,
                                                                  const std::vector<std::string>& args,
                                                                  const std::map<std::string, std::string>& env,
                                                                  const std::string& sudoPassword,
                                                                  std::function<void(const std::string&)> outputCallback,
                                                                  const std::string& working_directory) {
    int pipe_stdin[2], pipe_stdout[2], pipe_stderr[2];
    if (pipe(pipe_stdin) == -1 || pipe(pipe_stdout) == -1 || pipe(pipe_stderr) == -1) {
        perror("pipe");
        return std::make_tuple(-1, -1);
    }

    pid_t child_pid = fork();
    if (child_pid == -1) {
        perror("fork");
        // Cleanup on failure
        close(pipe_stdin[0]); close(pipe_stdin[1]);
        close(pipe_stdout[0]); close(pipe_stdout[1]);
        close(pipe_stderr[0]); close(pipe_stderr[1]);
        return std::make_tuple(-1, -1);
    } else if (child_pid == 0) {
        // Child
        close(pipe_stdin[1]);
        close(pipe_stdout[0]);
        close(pipe_stderr[0]);

        dup2(pipe_stdin[0], STDIN_FILENO);
        dup2(pipe_stdout[1], STDOUT_FILENO);
        dup2(pipe_stderr[1], STDERR_FILENO);

        close(pipe_stdin[0]);
        close(pipe_stdout[1]);
        close(pipe_stderr[1]);

        // Change to working directory if specified
        if (!working_directory.empty()) {
            chdir(working_directory.c_str());
        }

        // Set env
        for (const auto& pair : env) {
            setenv(pair.first.c_str(), pair.second.c_str(), 1);
        }

        // Construct sudo argv: sudo -S -k -p "" -- command args...
        std::vector<std::string> sudo_args = {"-S", "-k", "-p", "", "--", command};
        for (const auto& a : args) sudo_args.push_back(a);

        std::vector<char*> c_args;
        c_args.push_back(const_cast<char*>("sudo"));
        for (auto& s : sudo_args) {
            c_args.push_back(const_cast<char*>(s.c_str()));
        }
        c_args.push_back(nullptr);

        execvp("sudo", c_args.data());
        perror("execvp");
        _exit(1);
    } else {
        // Parent
        close(pipe_stdin[0]);
        close(pipe_stdout[1]);
        close(pipe_stderr[1]);

        int stdin_w = pipe_stdin[1];
        int stdout_r = pipe_stdout[0];
        int stderr_r = pipe_stderr[0];

        // Write password then close stdin (sudo reads until newline)
        if (!sudoPassword.empty()) {
            std::string pw = sudoPassword;
            pw.push_back('\n');
            ssize_t _ = write(stdin_w, pw.c_str(), pw.size());
            (void)_; // ignore write size
        }
        close(stdin_w);

        std::array<char, 128> buffer;
        ssize_t bytes_read;
        fd_set read_fds;
        int max_fd = std::max(stdout_r, stderr_r);
        bool stdout_open = true;
        bool stderr_open = true;

        std::string stderr_accum;

        while (stdout_open || stderr_open) {
            FD_ZERO(&read_fds);
            if (stdout_open) FD_SET(stdout_r, &read_fds);
            if (stderr_open) FD_SET(stderr_r, &read_fds);

            if (select(max_fd + 1, &read_fds, nullptr, nullptr, nullptr) == -1) {
                perror("select");
                break;
            }

            if (FD_ISSET(stdout_r, &read_fds)) {
                bytes_read = read(stdout_r, buffer.data(), buffer.size());
                if (bytes_read > 0) {
                    std::string out(buffer.data(), bytes_read);
                    if (outputCallback) outputCallback(out);
                } else if (bytes_read == 0) {
                    stdout_open = false;
                }
            }

            if (FD_ISSET(stderr_r, &read_fds)) {
                bytes_read = read(stderr_r, buffer.data(), buffer.size());
                if (bytes_read > 0) {
                    std::string err(buffer.data(), bytes_read);
                    stderr_accum.append(err);
                    if (outputCallback) outputCallback(err);
                } else if (bytes_read == 0) {
                    stderr_open = false;
                }
            }
        }

        close(stdout_r);
        close(stderr_r);

        int status;
        waitpid(child_pid, &status, 0);

        std::stringstream _ss;
        _ss << "\t\t[SUDO_RUN]: '" << command ;
        for(const auto& _arg: args)
        {
            _ss << " " << _arg;
        }
        _ss << "'";
        std::cerr << _ss.str() << " --> status: " << status << ", PID: " << child_pid << "\n";

        // Detect sudo auth failure by inspecting stderr text
        auto to_lower = [](std::string s) {
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
            return s;
        };
        std::string err_l = to_lower(stderr_accum);
        // bool auth_failed = (err_l.find("incorrect password") != std::string::npos) ||
        //                    (err_l.find("sorry, try again") != std::string::npos) ||
        //                    (err_l.find("incorrect password attempts") != std::string::npos);

        // if (auth_failed) {
        //     return std::make_tuple(child_pid, STATUS_SUDO_AUTH_FAILED);
        // }

        return std::make_tuple(child_pid, status);
    }
}
