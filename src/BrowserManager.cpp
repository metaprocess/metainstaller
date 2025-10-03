#include <filesystem>
#include <crow.h>
#include "BrowserManager.hpp"
#include "resourceextractor.h"
#include "utils.h"
#include "x_detector.hpp"
#include "MimeTypes.hpp"


void BrowserManager::close()
{
    m_process_browser.killProcess();
}

BrowserManager::BrowserManager()
{
    if((!std::filesystem::exists(_str_tmp_path)))
    {
        assertm(true == std::filesystem::create_directories(_str_tmp_path), "can not create temp directory,,,");
    }
    _7z_executable = Utils::get_7z_executable_path();
    _str_file_path_midori = ResourceExtractor::write_resource_to_path(":/resources/midori-11.5.2.7z", _str_tmp_path);
    _str_file_path_web = ResourceExtractor::write_resource_to_path(":/resources/web.7z", _str_tmp_path);
    _str_file_path_profile_midori = ResourceExtractor::write_resource_to_path(":/resources/profile_midori.7z", _str_tmp_path);
    _path_file_doc = ResourceExtractor::write_resource_to_path(":/resources/comprehensive_docs.html", _str_tmp_path);

    // Extract the 404 fantasy page
    ResourceExtractor::write_resource_to_path(":/resources/404_fantasy.html", _str_tmp_path);

    _path_html = Utils::path_join_multiple({
        _str_tmp_path,
        "dist",
        "index.html"
    });
    _path_web_dist = Utils::path_join_multiple({
        _str_tmp_path,
        "dist"
    });
}

bool BrowserManager::run_browser(const std::string &_url)
{
    {
        ProcessManager _pm;
        auto [_pid, _ret_process] = _pm.startProcessBlocking(
            _7z_executable,
            {
                "-o" + _str_tmp_path,
                "x",
                _str_file_path_midori
            },
            {},
            nullptr
            // [](const std::string& _str){
            //     std::cerr << _str;
            // }
        );
        std::cerr << "ret process extract midori= " << _ret_process << "\n"; 
    }

    {
        ProcessManager _pm;
        auto [_pid, _ret_process] = _pm.startProcessBlocking(
            _7z_executable,
            {
                "-o" + _str_tmp_path,
                "x",
                _str_file_path_web
            },
            {},
            nullptr
            // [](const std::string& _str){
            //     std::cerr << _str;
            // }
        );
        std::cerr << "ret process extract web= " << _ret_process << "\n"; 
    }
    
    {
        ProcessManager _pm;
        auto [_pid, _ret_process] = _pm.startProcessBlocking(
            _7z_executable,
            {
                "-o" + _str_tmp_path,
                "x",
                _str_file_path_profile_midori
            },
            {},
            nullptr
            // [](const std::string& _str){
            //     std::cerr << _str;
            // }
        );
        std::cerr << "ret process extract profile_midori = " << _ret_process << "\n"; 
        if(0 == _ret_process)
        {
            // configure midori to ignore network status when network is disconnected.
            std::ofstream _file_prefs(Utils::path_join_multiple({_str_tmp_path, "profile_midori", "prefs.js"}), std::ios::app);
            _file_prefs << "\n" << "user_pref(\"network.manage-offline-status\", false);" << "\n";
        }
    }

    DisplayDetection::DetectionResult _result = DisplayDetection::StaticDisplayDetector::detectDisplay();
    // std::cerr << "x available:" << _result.x_available << "\nwayland: " << _result.wayland_available << "\nDisplay Info: '" << _result.display_info << "'\n";
    if(
        !_result.socket_test_passed ||
        (0 == _result.display_info.size() && 0 == _result.wayland_display.size())
    )
    {
        crow::logger(crow::LogLevel::Error) << "NO X Environment found...\n";
        return false;
    }
    // if((_result.display_info.size() > 0 || _result.wayland_display.size()) && _result.socket_test_passed)
    // {

    // }
    
    // const std::string _str_tmp_path{"/tmp/metainstaller_browser"};
    

    
    

    // CROW_ROUTE(_app, "/installer").methods("GET"_method)
    // ([&_path_html = std::as_const(_path_html)](const crow::request& req) {
    //     std::ifstream file(_path_html, std::ios::binary);
    //     crow::response res;
    //     if (!file) {
    //         res.code = 404;
    //         res.write("File not found");
    //         // res.end();
    //         return res;
    //     }
    //     std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    //     res.set_header("Content-Type", "text/html");
    //     res.write(content);
    //     // res.end();
    //     return res;
    // });

    
    {
        // ProcessManager _pm;
        m_process_browser.startProcess(
            Utils::path_join_multiple({_str_tmp_path, "midori", "midori"}),
            {
                // "--kiosk",
                // "--offline",
                "--profile",
                Utils::path_join_multiple({_str_tmp_path, "profile_midori"}),
                // "-a",
                _url
            },
            {},
            nullptr,
            [](){
                std::exit(EXIT_SUCCESS);
            }
        );
        // std::exit(0); // Exit the application gracefully after Midori closes
    }

    return false;
}

void BrowserManager::register_endpoints(crow::SimpleApp &_app)
{
    // Capture the necessary variables for each lambda
    // const std::string path_html_copy = _path_html;
    // const std::string str_tmp_path_copy = _str_tmp_path;

    // Handler function to serve index.html for SPA routes
    const auto serve_index_html = [this](const crow::request& req, crow::response& res) {
        const std::string _filename = _path_html;

        if(!std::filesystem::exists(_filename))
        {
            CROW_LOG_CRITICAL << "file path '" << _filename << "' is not valid..";
            res.code = 404;
            res.write("File not found");
            res.end();
            return;
        }

        res.set_static_file_info_unsafe(_filename);
        res.add_header("Content-Type", MimeTypes::getType(_filename));
        res.end();
    };

    // Register SPA routes - all serve the same index.html file to let React Router handle routing
    CROW_ROUTE(_app, "/")([this, serve_index_html] (const crow::request& req, crow::response& res) {
        serve_index_html(req, res);
    });
    
    CROW_ROUTE(_app, "/installation")([this, serve_index_html] (const crow::request& req, crow::response& res) {
        serve_index_html(req, res);
    });
    
    CROW_ROUTE(_app, "/service")([this, serve_index_html] (const crow::request& req, crow::response& res) {
        serve_index_html(req, res);
    });
    
    CROW_ROUTE(_app, "/containers")([this, serve_index_html] (const crow::request& req, crow::response& res) {
        serve_index_html(req, res);
    });
    
    CROW_ROUTE(_app, "/images")([this, serve_index_html] (const crow::request& req, crow::response& res) {
        serve_index_html(req, res);
    });
    
    CROW_ROUTE(_app, "/projects")([this, serve_index_html] (const crow::request& req, crow::response& res) {
        serve_index_html(req, res);
    });
    
    CROW_ROUTE(_app, "/system")([this, serve_index_html] (const crow::request& req, crow::response& res) {
        serve_index_html(req, res);
    });
    
    CROW_ROUTE(_app, "/logs")([this, serve_index_html] (const crow::request& req, crow::response& res) {
        serve_index_html(req, res);
    });

    CROW_ROUTE(_app, "/assets/<string>").methods("GET"_method)([this] (const crow::request& req, const std::string& filename) {
        
        const std::string _path_asset = Utils::path_join_multiple({
            _path_web_dist,
            req.url
        });

        // const std::string _path_asset = Utils::path_join_multiple({
        //     "../frontend/dist/assets",
        //     filename
        // });
        crow::response res;
        if(!std::filesystem::exists(_path_asset))
        {
            CROW_LOG_CRITICAL << "file path '" << _path_asset << "' is not valid..";
            res.code = 404;
            res.write("File not found");
            return res;

        }
        res.set_static_file_info_unsafe(_path_asset);
        res.add_header("Content-Type", MimeTypes::getType(_path_asset));
        return res;
    });

    CROW_ROUTE(_app, "/Metaprocesslogo.jpg")([this] (const crow::request& req, crow::response& res) {
        const std::string _path_asset = Utils::path_join_multiple({
            _path_web_dist,
            req.url
        });
        if(!std::filesystem::exists(_path_asset))
        {
            CROW_LOG_CRITICAL << "file path '" << _path_asset << "' is not valid..";
            res.code = 404;
            res.write("File not found");
            res.end();
            return;

        }
        res.set_static_file_info_unsafe(_path_asset);
        res.add_header("Content-Type", MimeTypes::getType(_path_asset));
        res.end();
    });
    
    CROW_ROUTE(_app, "/favicon.ico")([this] (const crow::request& req, crow::response& res) {
        const std::string _path_asset = Utils::path_join_multiple({
            _path_web_dist,
            "Metaprocesslogo.jpg"
        });
        if(!std::filesystem::exists(_path_asset))
        {
            CROW_LOG_CRITICAL << "file path '" << _path_asset << "' is not valid..";
            res.code = 404;
            res.write("File not found");
            res.end();
            return;

        }
        res.set_static_file_info_unsafe(_path_asset);
        res.add_header("Content-Type", MimeTypes::getType(_path_asset));
        res.end();
    });

        // Documentation endpoint
    CROW_ROUTE(_app, "/api/docs").methods("GET"_method)
        ([this](const crow::request& req) {

            crow::response res;
            if(!std::filesystem::exists(_path_file_doc))
            {
                CROW_LOG_CRITICAL << "file path '" << _path_file_doc << "' is not valid..";
                crow::response res;
                res.code = 404;
                res.write("File not found");
                return res;
            }

            res.set_static_file_info_unsafe(_path_file_doc);
            res.add_header("Content-Type", MimeTypes::getType(_path_file_doc));
            return res;
        }
    );
        


    // // Dynamic route for assets (e.g., /assets/index-ab720b93.js)
    // CROW_ROUTE(_app, "/assets/<string>")([str_tmp_path_copy] (const crow::request& req, crow::response& res, const std::string& filename) {
    //     const std::string _path_asset = Utils::path_join_multiple({
    //         str_tmp_path_copy,
    //         "dist",
    //         "assets",
    //         filename
    //     });
        
    //     std::ifstream file(_path_asset, std::ios::binary);
    //     crow::logger(crow::LogLevel::Info) << "path asset='" << _path_asset << "'\n";
    //     if (!file) {
    //         res.code = 404;
    //         res.write("File not found");
    //         res.end();
    //         return;
    //     }
    //     std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
    //     // Set proper MIME type
    //     std::string mime = "text/plain";
    //     if (filename.find(".js") != std::string::npos) mime = "application/javascript";
    //     else if (filename.find(".css") != std::string::npos) mime = "text/css";
    //     else if (filename.find(".png") != std::string::npos) mime = "image/png";
    //     else if (filename.find(".jpg") != std::string::npos || filename.find(".jpeg") != std::string::npos) mime = "image/jpeg";
    //     else if (filename.find(".svg") != std::string::npos) mime = "image/svg+xml";
    //     else if (filename.find(".ico") != std::string::npos) mime = "image/x-icon";
    //     else if (filename.find(".json") != std::string::npos) mime = "application/json";
    //     else if (filename.find(".woff") != std::string::npos) mime = "font/woff";
    //     else if (filename.find(".woff2") != std::string::npos) mime = "font/woff2";
    //     else if (filename.find(".ttf") != std::string::npos) mime = "font/ttf";
        
    //     res.set_header("Content-Type", mime);
    //     res.set_header("Access-Control-Allow-Origin", "*");
    //     res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    //     res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    //     res.write(content);
    //     res.end();
    // });

    // Handle OPTIONS requests for CORS preflight
    // CROW_ROUTE(_app, "/installer")
    // .methods(crow::HTTPMethod::Options)([](const crow::request& req, crow::response& res) {
    //     res.set_header("Access-Control-Allow-Origin", "*");
    //     res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    //     res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    //     res.code = 204;
    //     res.end();
    // });
}
