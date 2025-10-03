#include "MetaDatabase.h"
#include "utils.h"
#include "json11.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <random>
#include <tuple>
#include "node.hpp"
#include <iostream>
#include "sqlite3.h"

namespace fs = std::filesystem;

// SQLite database functions
static int callback(void* data, int argc, char** argv, char** azColName) {
    // Generic callback for SQLite queries
    return 0;
}

MetaDatabase::MetaDatabase() {
    // Constructor
}

MetaDatabase::~MetaDatabase() {
    // Destructor
}

std::string MetaDatabase::getDatabasePath() {
    return Utils::path_join_multiple({Utils::get_metainstaller_home_dir(), "settings.db"});
}

bool MetaDatabase::initDatabase() {
    sqlite3* db;
    char* errMsg = 0;
    int rc;

    rc = sqlite3_open(getDatabasePath().c_str(), &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return false;
    }

    // Create projects table
    const char* sql = "CREATE TABLE IF NOT EXISTS projects ("
                      "name TEXT PRIMARY KEY NOT NULL,"
                      "archive_path TEXT,"
                      "extracted_path TEXT,"
                      "compose_file_path TEXT,"
                      "working_directory TEXT,"
                      "is_loaded INTEGER,"
                      "status_message TEXT,"
                      "created_time TEXT,"
                      "last_modified TEXT);";
    
    rc = sqlite3_exec(db, sql, callback, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }

    // Create project_required_images table
    sql = "CREATE TABLE IF NOT EXISTS project_required_images ("
          "project_name TEXT NOT NULL,"
          "image TEXT NOT NULL,"
          "FOREIGN KEY(project_name) REFERENCES projects(name) ON DELETE CASCADE);";
    
    rc = sqlite3_exec(db, sql, callback, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }

    // Create project_dependent_files table
    sql = "CREATE TABLE IF NOT EXISTS project_dependent_files ("
          "project_name TEXT NOT NULL,"
          "file_path TEXT NOT NULL,"
          "FOREIGN KEY(project_name) REFERENCES projects(name) ON DELETE CASCADE);";
    
    rc = sqlite3_exec(db, sql, callback, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }

    // Create project_services table
    sql = "CREATE TABLE IF NOT EXISTS project_services ("
          "project_name TEXT NOT NULL,"
          "service TEXT NOT NULL,"
          "FOREIGN KEY(project_name) REFERENCES projects(name) ON DELETE CASCADE);";

    rc = sqlite3_exec(db, sql, callback, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }

    // Create settings table
    sql = "CREATE TABLE IF NOT EXISTS settings ("
          "key TEXT PRIMARY KEY NOT NULL,"
          "value TEXT);";

    rc = sqlite3_exec(db, sql, callback, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }

    sqlite3_close(db);
    return true;
}

bool MetaDatabase::saveProjectsToDatabase(const std::map<std::string, ProjectInfo>& projects) {
    sqlite3* db;
    char* errMsg = 0;
    int rc;

    rc = sqlite3_open(getDatabasePath().c_str(), &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return false;
    }

    // Begin transaction
    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }

    // Clear existing data
    const char* clearSql = "DELETE FROM projects;";
    rc = sqlite3_exec(db, clearSql, callback, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
        sqlite3_close(db);
        return false;
    }

    clearSql = "DELETE FROM project_required_images;";
    rc = sqlite3_exec(db, clearSql, callback, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
        sqlite3_close(db);
        return false;
    }

    clearSql = "DELETE FROM project_dependent_files;";
    rc = sqlite3_exec(db, clearSql, callback, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
        sqlite3_close(db);
        return false;
    }

    clearSql = "DELETE FROM project_services;";
    rc = sqlite3_exec(db, clearSql, callback, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
        sqlite3_close(db);
        return false;
    }

    // Insert projects
    const char* insertProjectSql = "INSERT INTO projects (name, archive_path, extracted_path, compose_file_path, working_directory, is_loaded, status_message, created_time, last_modified) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    rc = sqlite3_prepare_v2(db, insertProjectSql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
        sqlite3_close(db);
        return false;
    }

    for (const auto& pair : projects) {
        const ProjectInfo& project = pair.second;
        
        sqlite3_bind_text(stmt, 1, project.name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, project.archive_path.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, project.extracted_path.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, project.compose_file_path.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, project.working_directory.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 6, project.is_loaded ? 1 : 0);
        sqlite3_bind_text(stmt, 7, project.status_message.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 8, project.created_time.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 9, project.last_modified.c_str(), -1, SQLITE_STATIC);
        
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            std::cerr << "Failed to execute statement: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_finalize(stmt);
            sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
            sqlite3_close(db);
            return false;
        }
        
        sqlite3_reset(stmt);
        
        // Insert required images
        const char* insertImageSql = "INSERT INTO project_required_images (project_name, image) VALUES (?, ?);";
        sqlite3_stmt* imageStmt;
        rc = sqlite3_prepare_v2(db, insertImageSql, -1, &imageStmt, NULL);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_finalize(stmt);
            sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
            sqlite3_close(db);
            return false;
        }
        
        for (const std::string& image : project.required_images) {
            sqlite3_bind_text(imageStmt, 1, project.name.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(imageStmt, 2, image.c_str(), -1, SQLITE_STATIC);
            
            rc = sqlite3_step(imageStmt);
            if (rc != SQLITE_DONE) {
                std::cerr << "Failed to execute statement: " << sqlite3_errmsg(db) << std::endl;
                sqlite3_finalize(imageStmt);
                sqlite3_finalize(stmt);
                sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
                sqlite3_close(db);
                return false;
            }
            
            sqlite3_reset(imageStmt);
        }
        sqlite3_finalize(imageStmt);
        
        // Insert dependent files
        const char* insertFileSql = "INSERT INTO project_dependent_files (project_name, file_path) VALUES (?, ?);";
        sqlite3_stmt* fileStmt;
        rc = sqlite3_prepare_v2(db, insertFileSql, -1, &fileStmt, NULL);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_finalize(stmt);
            sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
            sqlite3_close(db);
            return false;
        }
        
        for (const std::string& file : project.dependent_files) {
            sqlite3_bind_text(fileStmt, 1, project.name.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(fileStmt, 2, file.c_str(), -1, SQLITE_STATIC);
            
            rc = sqlite3_step(fileStmt);
            if (rc != SQLITE_DONE) {
                std::cerr << "Failed to execute statement: " << sqlite3_errmsg(db) << std::endl;
                sqlite3_finalize(fileStmt);
                sqlite3_finalize(stmt);
                sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
                sqlite3_close(db);
                return false;
            }
            
            sqlite3_reset(fileStmt);
        }
        sqlite3_finalize(fileStmt);
        
        // Insert services
        const char* insertServiceSql = "INSERT INTO project_services (project_name, service) VALUES (?, ?);";
        sqlite3_stmt* serviceStmt;
        rc = sqlite3_prepare_v2(db, insertServiceSql, -1, &serviceStmt, NULL);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_finalize(stmt);
            sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
            sqlite3_close(db);
            return false;
        }
        
        for (const std::string& service : project.services) {
            sqlite3_bind_text(serviceStmt, 1, project.name.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(serviceStmt, 2, service.c_str(), -1, SQLITE_STATIC);
            
            rc = sqlite3_step(serviceStmt);
            if (rc != SQLITE_DONE) {
                std::cerr << "Failed to execute statement: " << sqlite3_errmsg(db) << std::endl;
                sqlite3_finalize(serviceStmt);
                sqlite3_finalize(stmt);
                sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
                sqlite3_close(db);
                return false;
            }
            
            sqlite3_reset(serviceStmt);
        }
        sqlite3_finalize(serviceStmt);
    }
    
    sqlite3_finalize(stmt);
    
    // Commit transaction
    rc = sqlite3_exec(db, "COMMIT;", NULL, NULL, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }

    sqlite3_close(db);
    return true;
}

std::map<std::string, ProjectInfo> MetaDatabase::loadProjectsFromDatabase() {
    std::map<std::string, ProjectInfo> projects;
    sqlite3* db;
    char* errMsg = 0;
    int rc;

    rc = sqlite3_open(getDatabasePath().c_str(), &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return projects;
    }

    // Load projects
    const char* selectProjectsSql = "SELECT * FROM projects;";
    sqlite3_stmt* stmt;
    rc = sqlite3_prepare_v2(db, selectProjectsSql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return projects;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        ProjectInfo project;
        project.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        project.archive_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        project.extracted_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        project.compose_file_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        project.working_directory = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        project.is_loaded = sqlite3_column_int(stmt, 5) != 0;
        project.status_message = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        project.created_time = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        project.last_modified = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        
        projects[project.name] = project;
    }
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(db) << std::endl;
    }
    
    sqlite3_finalize(stmt);

    // Load required images
    const char* selectImagesSql = "SELECT project_name, image FROM project_required_images;";
    rc = sqlite3_prepare_v2(db, selectImagesSql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return projects;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::string projectName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string image = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        
        auto it = projects.find(projectName);
        if (it != projects.end()) {
            it->second.required_images.push_back(image);
        }
    }
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(db) << std::endl;
    }
    
    sqlite3_finalize(stmt);

    // Load dependent files
    const char* selectFilesSql = "SELECT project_name, file_path FROM project_dependent_files;";
    rc = sqlite3_prepare_v2(db, selectFilesSql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return projects;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::string projectName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string file = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        
        auto it = projects.find(projectName);
        if (it != projects.end()) {
            it->second.dependent_files.push_back(file);
        }
    }
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(db) << std::endl;
    }
    
    sqlite3_finalize(stmt);

    // Load services
    const char* selectServicesSql = "SELECT project_name, service FROM project_services;";
    rc = sqlite3_prepare_v2(db, selectServicesSql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return projects;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::string projectName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string service = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        
        auto it = projects.find(projectName);
        if (it != projects.end()) {
            it->second.services.push_back(service);
        }
    }
    
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(db) << std::endl;
    }
    
    sqlite3_finalize(stmt);

    sqlite3_close(db);
    return projects;
}

bool MetaDatabase::saveSetting(const std::string& key, const std::string& value) {
    sqlite3* db;
    int rc;

    rc = sqlite3_open(getDatabasePath().c_str(), &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return false;
    }

    const char* insertSettingSql = "INSERT OR REPLACE INTO settings (key, value) VALUES (?, ?);";
    sqlite3_stmt* stmt;
    rc = sqlite3_prepare_v2(db, insertSettingSql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return false;
    }

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, value.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return false;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return true;
}

std::string MetaDatabase::getSetting(const std::string& key) {
    sqlite3* db;
    int rc;

    rc = sqlite3_open(getDatabasePath().c_str(), &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return "";
    }

    const char* selectSettingSql = "SELECT value FROM settings WHERE key = ?;";
    sqlite3_stmt* stmt;
    rc = sqlite3_prepare_v2(db, selectSettingSql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return "";
    }

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);

    std::string value = "";
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const char* val = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (val) {
            value = val;
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return value;
}
