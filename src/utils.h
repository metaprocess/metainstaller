#pragma once

#include <string>
#include <vector>

#define CONST_FILE_ENV_SECRET_PRIVATE ".env.secret"
#define CONST_KEY_SUDO_PSWD "SUDO_PSWD"

class Utils
{
public:
    static std::string path_join_multiple(const std::vector<std::string>& _lst);
    static std::string create_temp_path(const std::string& _basepath);
    static void assert_condition(bool condition, const char *condition_str, const std::string &message, const std::string& _file, const int& _line);
    static void test_7z();
    static std::string extract_7z_to_path(const std::string& _path);
    static std::string get_7z_executable_path();
    static bool test_sudo(const std::string& _sudo_password);
    static void startup();  // Entry point tasks
    static void cleanup();  // Exit point tasks
    static std::string get_metainstaller_home_dir();
    static std::string get_env_secret_file_path();
    static std::string str_to_lower(const std::string& in_data);
};

#define assertm(COND, MSG) Utils::assert_condition((COND), #COND, MSG, __FILE__, __LINE__)
