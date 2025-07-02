#ifndef DB_FILES_H
#define DB_FILES_H

#include "db_entities.h"
#include "logger.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QVariant>
#include <QSqlDatabase>
#include <vector>
#include <string>

using namespace std;

class DatabaseFileManager {
public:
    explicit DatabaseFileManager(QSqlDatabase& database);

    int insertFile(const string& fileName, const string& fileType);  // Returns file ID
    bool deleteFile(int fileId);
    bool deleteAllFiles();

    vector<FileEntity> getAllFiles();
    FileEntity getFileById(int id);
    FileEntity getFileByName(const string& fileName);
    int getFileIdByName(const string& fileName);

    bool fileExists(int fileId);

private:
    QSqlDatabase& db;

    static FileEntity createFileEntityFromQuery(QSqlQuery& query);
};

#endif // DB_FILES_H