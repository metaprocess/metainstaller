#include <crow.h>
#include <iostream>
#include <signal.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <fstream>  // For manual static file serving
#include <string>
#include "utils.h"
#include "DockerManager.h"
#include "ProjectManager.h"
#include "FileManager.h"
#include "EnvConfig.hpp"
#include "BrowserManager.hpp"
#include "argument_handler.h"
#include "test.h"
#include "help_global.h"
#include "SELinuxManager.h"

// Hardcoded version constant
// const std::string VERSION = "1404.06.11";

// std::atomic<bool> stop_server(false);

void signal_handler(int signum) {
    std::cerr << "Caught signal " << signum << "\n";
    // stop_server = true;
}

int main(int argc, char** argv) {
    signal(SIGHUP, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    ArgumentHandler _handler;
    _handler.addStringArgument("test", "run specified tests as a list separated by comma. use 'all' to run all tests", "", false);
    _handler.addBooleanArgument("help", "shows help string", false, false);
    _handler.addBooleanArgument("version", "shows version string", false, false);
    _handler.parseArguments(argc, argv);
    auto _test_arg = std::get<std::string>(_handler.getArgumentValue("test"));
    auto _help = std::get<bool>(_handler.getArgumentValue("help"));
    auto _version = std::get<bool>(_handler.getArgumentValue("version"));
    if(_version)
    {
        std::cout << "MetaInstaller version: " << APP_VERSION << "\n";
        std::exit(EXIT_SUCCESS);
    }
    if(_help)
    {
        std::cout << HelpGlobal::help_global() << "\n";
        std::exit(EXIT_SUCCESS);
    }

    system("killall midori");
    sleep(1);

    // If invoked with --test, run test suite and exit.
    if(_test_arg.size() > 0)
    {
        std::thread([_test_arg](){
            // wait for REST service to boot.
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            Test _t;
            bool _ret = _t.run_from_test_arg(_test_arg);
            std::exit(_ret ? EXIT_SUCCESS : EXIT_FAILURE);
        }).detach();
    }

    {
        // Utils::test_7z();
        // return EXIT_SUCCESS;
    }

    crow::SimpleApp app;
    app.loglevel(crow::LogLevel::Info);
    
    // Initialize Docker Manager and register REST endpoints
    DockerManager dockerManager;
    dockerManager.registerRestEndpoints(app);

    // Initialize Project Manager and register REST endpoints
    ProjectManager projectManager;
    projectManager.registerRestEndpoints(app);

    // Initialize File Manager and register REST endpoints
    FileManager fileManager;
    fileManager.registerRestEndpoints(app);

    // Initialize SELinux Manager and register REST endpoints
    SELinuxManager selinuxManager;
    selinuxManager.registerRestEndpoints(app);

    // Route for root (/) - Serve React app
    // CROW_ROUTE(app, "/")([] (const crow::request& req, crow::response& res) {
    //     std::ifstream file("./docker-manager-ui/build/index.html", std::ios::binary);
    //     if (!file) {
    //         res.code = 404;
    //         res.write("Docker Manager UI not found. Please build the React app first.");
    //         res.end();
    //         return;
    //     }
    //     std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    //     res.set_header("Content-Type", "text/html");
    //     res.write(content);
    //     res.end();
    // });

    // Serve React static files (JS, CSS, etc.)
    // CROW_ROUTE(app, "/static/<path>")([] (const crow::request& req, crow::response& res, const std::string& subpath) {
    //     std::string path = "./docker-manager-ui/build/static/" + subpath;
    //     std::ifstream file(path, std::ios::binary);
    //     if (!file) {
    //         res.code = 404;
    //         res.write("File not found");
    //         res.end();
    //         return;
    //     }
    //     std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
    //     // Set appropriate content type
    //     std::string mime = "text/plain";
    //     if (subpath.find(".js") != std::string::npos) mime = "application/javascript";
    //     else if (subpath.find(".css") != std::string::npos) mime = "text/css";
    //     else if (subpath.find(".map") != std::string::npos) mime = "application/json";
        
    //     res.set_header("Content-Type", mime);
    //     res.write(content);
    //     res.end();
    // });

    // Route for legacy installer (/) - Serve ./build_web/index.html
    

    // REST endpoint: GET /test
    CROW_ROUTE(app, "/test")([] (const crow::request& req, crow::response& res) {
        crow::logger(crow::LogLevel::Info) << "request from " << req.remote_ip_address << "\n";
        res.set_header("Content-Type", "application/json");
        res.write("{\"message\": \"rest is working\"}");
        res.end();
    });

    // REST endpoint: GET /version
    CROW_ROUTE(app, "/api/version")([] (const crow::request& req, crow::response& res) {
        crow::logger(crow::LogLevel::Info) << "Version request from " << req.remote_ip_address << "\n";
        res.set_header("Content-Type", "application/json");
        res.write("{\"version\": \"" + std::string(APP_VERSION) + "\"}");
        res.end();
    });

    // WebSocket endpoint: /ws (echo server)
    // std::vector<crow::websocket::connection*> connections;
    
    // WebSocket connections for operation logs
    std::vector<crow::websocket::connection*> log_connections;
    
    // WebSocket connections for installation progress
    std::vector<crow::websocket::connection*> progress_connections;

    // Define the WebSocket route at "/ws"
    // CROW_WEBSOCKET_ROUTE(app, "/ws")
    //     // Handle new connections
    //     .onopen([&connections](crow::websocket::connection& conn) {
    //         connections.push_back(&conn);
    //         conn.send_text("Welcome to the WebSocket service!");
    //         crow::logger(crow::LogLevel::Info) << "New connection established\n";
    //     })
    //     // Handle connection closure
    //     .onclose([&connections](crow::websocket::connection& conn, const std::string& reason, short unsigned int _opcode) {
    //         connections.erase(std::remove(connections.begin(), connections.end(), &conn), connections.end());
    //         crow::logger(crow::LogLevel::Info) << "Connection closed: " << reason << "\n";
    //     })
    //     // Handle incoming messages
    //     .onmessage([](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
    //         if (!is_binary) {
    //             crow::logger(crow::LogLevel::Info) << "Received message: " << data << "\n";
    //             conn.send_text("Echo: " + data); // Echo the message back to the client
    //         } else {
    //             crow::logger(crow::LogLevel::Info) << "binary received at size " << data.size() << "\n";
    //             conn.send_text(std::string("Binary data received at size '") + std::to_string(data.size()) + "' bytes." );
    //         }
    //     })
    //     // Handle errors
    //     .onerror([](crow::websocket::connection& conn, const std::string& error_message) {
    //         std::cerr << "WebSocket error: " << error_message << "\n";
    //     });

    // WebSocket endpoint for operation logs: /ws/logs
    CROW_WEBSOCKET_ROUTE(app, "/ws/logs")
        .onopen([&log_connections, &dockerManager](crow::websocket::connection& conn) {
            log_connections.push_back(&conn);
            // conn.send_text("{\"type\":\"connected\",\"message\":\"Connected to operation logs stream\"}");
            dockerManager.broadcastLog("connection", "Connected to operation logs stream", "info");
            crow::logger(crow::LogLevel::Info) << "New log connection established\n";
        })
        .onclose([&log_connections](crow::websocket::connection& conn, const std::string& reason, short unsigned int _opcode) {
            log_connections.erase(std::remove(log_connections.begin(), log_connections.end(), &conn), log_connections.end());
            crow::logger(crow::LogLevel::Info) << "Log connection closed: " << reason << "\n";
        })
        .onmessage([&dockerManager](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
            if (!is_binary) {
                crow::logger(crow::LogLevel::Info) << "Log websocket received: " << data << "\n";
                // Logs are one-way from server to client, so we just acknowledge
                // conn.send_text("{\"type\":\"ack\",\"message\":\"Message received\"}");
                dockerManager.broadcastLog("on_message", data, "info");
            }
        })
        .onerror([](crow::websocket::connection& conn, const std::string& error_message) {
            crow::logger(crow::LogLevel::Error) << "Log WebSocket error: " << error_message << "\n";
        });

    // WebSocket endpoint for installation progress: /ws/progress
    CROW_WEBSOCKET_ROUTE(app, "/ws/progress")
        .onopen([&progress_connections](crow::websocket::connection& conn) {
            progress_connections.push_back(&conn);
            conn.send_text("{\"type\":\"connected\",\"message\":\"Connected to installation progress stream\"}");
            crow::logger(crow::LogLevel::Info) << "New progress connection established\n";
        })
        .onclose([&progress_connections](crow::websocket::connection& conn, const std::string& reason, short unsigned int _opcode) {
            progress_connections.erase(std::remove(progress_connections.begin(), progress_connections.end(), &conn), progress_connections.end());
            crow::logger(crow::LogLevel::Info) << "Progress connection closed: " << reason << "\n";
        })
        .onmessage([](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
            if (!is_binary) {
                crow::logger(crow::LogLevel::Info) << "Progress websocket received: " << data << "\n";
                // Progress is one-way from server to client, so we just acknowledge
                conn.send_text("{\"type\":\"ack\",\"message\":\"Message received\"}");
            }
        })
        .onerror([](crow::websocket::connection& conn, const std::string& error_message) {
            std::cerr << "Progress WebSocket error: " << error_message << "\n";
        });

    // Set websocket connection references in DockerManager and ProjectManager for broadcasting
    dockerManager.setWebSocketConnections(&log_connections, &progress_connections);
    projectManager.setWebSocketConnections(&log_connections, &progress_connections);
    int rest_port = EnvConfig::get_int_value(EnvKey::REST_PORT);

    BrowserManager _bm;
    _bm.register_endpoints(app);
    if(_test_arg.empty())
    {
        std::thread([rest_port, &_bm, &app](){
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::stringstream _ss;
            _ss << "http://127.0.0.1:" << rest_port;
            // _ss << "http://127.0.0.1:3000";
            _bm.run_browser(_ss.str());
        }).detach();
    }
    
    // Custom 404 error handler
    CROW_CATCHALL_ROUTE(app)
    ([](const crow::request& req, crow::response& res) {
        // Path to the 404 page in the temp directory
        const std::string not_found_path = "/tmp/metainstaller_browser/404_fantasy.html";
        CROW_LOG_ERROR << "REQUESTED URL: '" << req.url << "'";
        
        // Check if the file exists
        if (std::filesystem::exists(not_found_path)) {
            res.set_static_file_info_unsafe(not_found_path);
        } else {
            // Fallback if the file doesn't exist
            res.code = 404;
            res.set_header("Content-Type", "text/html");
            res.body = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Page Not Found</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; padding: 50px; background: #f0f0f0; }
        .container { max-width: 500px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 0 20px rgba(0,0,0,0.1); }
        h1 { color: #d35400; }
        p { font-size: 18px; color: #333; }
        a { color: #3498db; text-decoration: none; font-weight: bold; }
        a:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <div class="container">
        <h1>404 - Page Not Found</h1>
        <p>Sorry, the page you're looking for doesn't exist.</p>
        <p><a href="/">Return to Home</a></p>
    </div>
</body>
</html>
            )";
        }
        res.end();
    });
    
    // Start server in a separate thread
    app.port(rest_port).multithreaded().run();
    
    if(_test_arg.empty())
    {
        _bm.close();
    }
    return 0;
}
