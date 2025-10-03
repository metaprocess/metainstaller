#ifndef METADATABASE_H
#define METADATABASE_H

#include <map>
#include <string>
#include "types.hpp"

class MetaDatabase {
public:
    MetaDatabase();
    ~MetaDatabase();

    bool initDatabase();
    bool saveProjectsToDatabase(const std::map<std::string, ProjectInfo>& projects);
    std::map<std::string, ProjectInfo> loadProjectsFromDatabase();

    bool saveSetting(const std::string& key, const std::string& value);
    std::string getSetting(const std::string& key);

private:
    std::string getDatabasePath();
};

#endif // METADATABASE_H
