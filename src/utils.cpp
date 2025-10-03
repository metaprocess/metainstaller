#include <stdexcept>
#include <filesystem>
#include <iostream>
#include <bits/stl_algo.h>
#include "utils.h"
#include "resourceextractor.h"
#include "ProcessManager.h"
#include "dotenv.hpp"
#include "EnvConfig.hpp"

namespace fs = std::filesystem;


namespace {
    // Helper class for automatic startup/cleanup
    struct StartupCleanupHandler {
        StartupCleanupHandler() { Utils::startup(); }
        ~StartupCleanupHandler() { Utils::cleanup(); }
    };

    // Static instance ensures constructor/destructor are called
    StartupCleanupHandler globalHandler;
}

std::string Utils::create_temp_path(const std::string &_basepath)
{
    std::string template_path = path_join_multiple({
        _basepath,
        "metainstaller_XXXXXX"
    });
    
    // Create the temporary directory
    char* dir_name = mkdtemp(template_path.data());
    assertm(dir_name != nullptr, "Failed to create temporary directory.");

    // std::cout << "Temporary directory created: " << dir_name << std::endl;

    // Optional: use std::filesystem to work with the directory
    std::filesystem::path temp_dir(dir_name);
    return std::string(dir_name);
}

void Utils::assert_condition(bool condition, const char *condition_str, const std::string &message, const std::string &_file, const int &_line)
{
    if(!condition) {
        throw std::runtime_error("ASSERT FAILED. <COND>(" + std::string(condition_str) + "), <MSG>'" + message + "', <FILE>'" + _file + std::string(":") + std::to_string(_line) + "'");
    }
}

void Utils::test_7z()
{
    const auto _str_tmp_path = create_temp_path("/tmp");
    const auto _str_7z = Utils::get_7z_executable_path();
    
    const auto _str_path_docker_saved =  ResourceExtractor::write_resource_to_path(":/resources/docker-28.3.1.7z", _str_tmp_path);

    ProcessManager _pm;
    auto [_pid, _ret_process] = _pm.startProcessBlocking(
        _str_7z,
        {
            "-o" + _str_tmp_path,
            "x",
            _str_path_docker_saved
        },
        {},
        nullptr
        // [](const std::string& _str){
        //     std::cerr << _str;
        // }
    );

    std::cerr << "ret process = " << _ret_process << "\n"; 

    auto _ret = std::filesystem::remove_all(std::filesystem::path(_str_tmp_path));
    (void)_ret;
    return;
}

std::string Utils::extract_7z_to_path(const std::string &_path)
{
    const auto _str_7z = ResourceExtractor::write_resource_to_path(":/resources/7zzs", _path);
    assertm(!_str_7z.empty() && std::filesystem::exists(_str_7z), "extracting 7zzs to '" + _path + "' failed...");
    fs::permissions(_str_7z, 
        fs::perms::owner_exec |
        fs::perms::group_exec |
        fs::perms::others_exec,
        fs::perm_options::add
    );
    return _str_7z;
}

std::string Utils::get_7z_executable_path()
{
    static std::string _path_extract{"/tmp/metainstaller_7z"};
    static std::string _path_7z;
    if(_path_7z.empty())
    {
        // Ensure the directory exists
        if (!std::filesystem::exists(_path_extract)) {
            std::filesystem::create_directories(_path_extract);
        }
        _path_7z = Utils::extract_7z_to_path(_path_extract);
    }
    return _path_7z;
}

bool Utils::test_sudo(const std::string& _sudo_password)
{
    ProcessManager _pm;
    std::string _str_ret;
    auto [_pid, _ret] = _pm.startProcessBlockingAsRoot(
        "id",
        {},
        {},
        _sudo_password,
        [&_str_ret](const std::string& _str){
            _str_ret += _str;
        }
    );
    // std::cerr << "[test_sudo]::\"" << _str_ret << "\"";
    return (0 == _ret);
}

void Utils::startup()
{
    std::cerr << "GLOBAL STARTUP ROUTINE...\n";
    
    EnvConfig::ensure_env_file_exists(".env");
    
    const std::string _env_file{".env"};
    if(std::filesystem::exists(_env_file))
    {
        EnvParser _parser;
        _parser.load_env_file(_env_file);
        _parser.print_all();
    }
    
    {
        // managing env for testing authoritative methods
        const std::string _env_file{Utils::get_env_secret_file_path()};
        const std::string const_key_sudo_password = CONST_KEY_SUDO_PSWD;
        EnvParser _parser;
        if(!std::filesystem::exists(_env_file))
        {
            _parser.create_env_file(_env_file, {
                {const_key_sudo_password, "CHANGE_PASSWORD"}
            });
        }
        _parser.load_env_file(_env_file);
        if(!_parser.has(const_key_sudo_password))
        {
            _parser.set(const_key_sudo_password, "CHANGE_PASSWORD", _env_file);
        }
    }

}

void Utils::cleanup()
{
}

std::string Utils::get_metainstaller_home_dir()
{
    static std::string _path = Utils::path_join_multiple({
        std::getenv("HOME"),
        ".metainstaller"
    });
    
    if(!std::filesystem::exists(_path))
    {
        std::filesystem::create_directories(_path);
    }
    return _path;
}

std::string Utils::str_to_lower(const std::string &in_data)
{
    std::string buffer = in_data;

    std::transform(
        buffer.begin(),
        buffer.end(),
        buffer.begin(),
        [](unsigned char c)
        {
            return std::tolower(c);
        }
    );

    return buffer;
}

std::string Utils::get_env_secret_file_path()
{
    static std::string _path{
        Utils::path_join_multiple({
            Utils::get_metainstaller_home_dir(),
            CONST_FILE_ENV_SECRET_PRIVATE
        })
    };
    return _path;
}

std::string Utils::path_join_multiple(const std::vector<std::string> &_lst)
{
    std::filesystem::path _path;
    int _ctr = 0;
    for(const std::string& _str: _lst)
    {
        auto _str_copy = _str;
        if(_ctr > 0 && std::filesystem::path::preferred_separator == _str[0])
        {
            _str_copy.erase(_str_copy.begin());
        }
        if(std::filesystem::path::preferred_separator == *(_str.end()-1) && _str.size() > 1)
        {
            _str_copy.pop_back();
        }
        _path.append(_str_copy);
        _ctr++;
    }
    return _path.string();
}
