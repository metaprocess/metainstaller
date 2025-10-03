#ifndef RESOURCEEXTRACTOR_H
#define RESOURCEEXTRACTOR_H

#include RESOURCES_HEADER

class ResourceExtractor
{
    // Q_OBJECT

public:
    static Resource load_resource(const std::string& _name_file_resource);
    static std::string write_resource_to_path(const std::string& _name_file_resource, const std::string& _base_path); 
    explicit ResourceExtractor();

    // bool extractBinary(const QString &binaryName, const QString &targetPath);
    // bool extractAllBinaries(const QStringList &binaries, const QString &targetDir);
    // QString getResourcePath(const QString &binaryName) const;
    // QStringList getAvailableBinaries() const;

// signals:
//     void extractionProgress(const QString &message);
//     void extractionError(const QString &error);

// private:
//     QString getCurrentArchitecture() const;
//     bool makeExecutable(const QString &filePath);
    
//     QString m_currentArch;
};

#endif // RESOURCEEXTRACTOR_H