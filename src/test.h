#pragma once 
#include "httplib.h"
#include "json11.hpp"
#include "dotenv.hpp"
#include "EnvConfig.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <functional>

class Test {
public:
    Test();
    /**
     * @brief runs test methods based on input argument. if input arg is 'all', it should call  'run_all()'. if _test_arg is 'test1,test2,test3', it should run tests consequently
     * @param _test_arg can be 'all' or list of test names separated by comma like 'test1,test2,test3'
     * @return true if all tests ran successfullt
     */

    bool run_from_test_arg(const std::string& _test_arg);

    /**
     * @brief returns help string about using this class and list of tests defined in this class
     * @return help string 
     */
    std::string help();
    
    /**
     * @brief returns list of test names defined in this class
     * @return vector of test names
     */
    std::vector<std::string> get_test_names() const;
    
private:
    std::vector<std::pair<std::string, std::function<bool()>>> tests;
    std::string base_url;
    
private:
    bool REST_test_sudo(const std::string& _password);
    bool REST_test_docker_install(const std::string& _password);
    bool REST_test_docker_uninstall(const std::string& _password);
    bool REST_test_project_load();
    bool REST_test_project_remove();
    bool REST_test_project_start();
    bool REST_test_project_stop();
    bool REST_test_project_restart();
    bool REST_test_project_services();
    bool run_test(const std::string& _test_name);
    bool run_all();
};

void run_all_tests();
