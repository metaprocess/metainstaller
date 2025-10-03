#pragma once

#include <string>
#include "ProcessManager.h"
#include <crow.h>
class BrowserManager{
public:
    BrowserManager();
    bool run_browser(const std::string& _url);
    void register_endpoints(crow::SimpleApp& _app);
    void close();
private:
    ProcessManager m_process_browser;
    const std::string _str_tmp_path{"/tmp/metainstaller_browser"};
    std::string _7z_executable;
    std::string _str_file_path_midori;
    std::string _str_file_path_web;
    std::string _str_file_path_profile_midori;
    std::string _path_html;
    std::string _path_file_doc;
    std::string _path_web_dist;
};