#include <filesystem>
#include <fstream>
#include "utils.h"
#include "resourceextractor.h"

// #define STRING(s) #s

// static void assert_condition(bool condition, const char* condition_str, const std::string& message)
// {
//     if(!condition) {
//         // std::cerr << "Assertion failed: " << condition_str << " | Message: " << message << std::endl;
//         throw std::runtime_error("Assertion failed: " + std::string(condition_str) + " | " + message);
//     }
// }
// #define assertm(COND, MSG) assert_condition((COND), #COND, MSG)


Resource ResourceExtractor::load_resource(const std::string& _name_file_resource)
{
    assertm(!_name_file_resource.empty(), "filename is empty");
    if(':' == _name_file_resource[0])
    {
        auto _res = get_resource(std::string(_name_file_resource.begin() + 2, _name_file_resource.end()));
        return _res;
    }
    else
    {
        return Resource();
    }
}

std::string ResourceExtractor::write_resource_to_path(const std::string &_name_file_resource, const std::string &_base_path)
{
    assertm(std::filesystem::exists(_base_path), "base path does not exists: '" + _base_path +  "', filename: '" + _name_file_resource + "'");
    auto _res = load_resource(_name_file_resource);
    assertm(_res.size > 0, std::string("load resource '") + _name_file_resource + std::string("' failed.."));
    std::string _str_resource_relative = std::string(_name_file_resource.begin() + 2, _name_file_resource.end());
    std::filesystem::path _path_resource(_str_resource_relative);
    std::string _str_path_output = Utils::path_join_multiple({_base_path, _path_resource.filename()});
    std::ofstream _file_out(_str_path_output, std::ios::out | std::ios::binary);
    _file_out.write(reinterpret_cast<const char*>(_res.start), _res.size);
    assertm(_file_out.good(), "failed to write file: " + _str_path_output);
    return _str_path_output;
}

ResourceExtractor::ResourceExtractor() //    : QObject(parent)
{
    // m_currentArch = getCurrentArchitecture();
}

// bool ResourceExtractor::extractBinary(const QString &binaryName, const QString &targetPath)
// {
//     QString resourcePath = getResourcePath(binaryName);
//     if (resourcePath.isEmpty()) {
//         emit extractionError(QString("Binary %1 not found in resources for architecture %2")
//                            .arg(binaryName, m_currentArch));
//         return false;
//     }
    
//     QFile resourceFile(resourcePath);
//     if (!resourceFile.open(QIODevice::ReadOnly)) {
//         emit extractionError(QString("Failed to open resource file: %1").arg(resourcePath));
//         return false;
//     }
    
//     // Remove existing file if it exists
//     if (QFile::exists(targetPath)) {
//         QFile::remove(targetPath);
//     }
    
//     // Create target directory if it doesn't exist
//     QFileInfo targetInfo(targetPath);
//     QDir targetDir = targetInfo.dir();
//     if (!targetDir.exists()) {
//         targetDir.mkpath(".");
//     }
    
//     // Copy the binary
//     QFile targetFile(targetPath);
//     if (!targetFile.open(QIODevice::WriteOnly)) {
//         emit extractionError(QString("Failed to create target file: %1").arg(targetPath));
//         return false;
//     }
    
//     targetFile.write(resourceFile.readAll());
//     targetFile.close();
//     resourceFile.close();
    
//     // Make executable
//     if (!makeExecutable(targetPath)) {
//         emit extractionError(QString("Failed to make binary executable: %1").arg(targetPath));
//         return false;
//     }
    
//     emit extractionProgress(QString("Extracted %1 to %2").arg(binaryName, targetPath));
//     return true;
// }

// bool ResourceExtractor::extractAllBinaries(const QStringList &binaries, const QString &targetDir)
// {
//     for (const QString &binary : binaries) {
//         QString targetPath = QString("%1/%2").arg(targetDir, binary);
//         if (!extractBinary(binary, targetPath)) {
//             return false;
//         }
//     }
//     return true;
// }

// QString ResourceExtractor::getResourcePath(const QString &binaryName) const
// {
//     QString resourcePath = QString(":/resources/%1-%2").arg(binaryName, m_currentArch);
    
//     // Check if the resource exists
//     QFile resourceFile(resourcePath);
//     if (resourceFile.exists()) {
//         return resourcePath;
//     }
    
//     return QString(); // Not found
// }

// QStringList ResourceExtractor::getAvailableBinaries() const
// {
//     QStringList binaries;
//     QStringList candidates = {"docker", "dockerd", "docker-compose", "podman"};
    
//     for (const QString &binary : candidates) {
//         if (!getResourcePath(binary).isEmpty()) {
//             binaries << binary;
//         }
//     }
    
//     return binaries;
// }

// QString ResourceExtractor::getCurrentArchitecture() const
// {
//     QString arch = QSysInfo::currentCpuArchitecture();
//     if (arch == "x86_64" || arch == "amd64") {
//         return "x86_64";
//     } else if (arch == "arm64" || arch == "aarch64") {
//         return "aarch64";
//     }
//     return arch; // Return as-is for unsupported architectures
// }

// bool ResourceExtractor::makeExecutable(const QString &filePath)
// {
//     QProcess chmod;
//     chmod.start("chmod", QStringList() << "+x" << filePath);
//     chmod.waitForFinished();
//     return chmod.exitCode() == 0;
// }