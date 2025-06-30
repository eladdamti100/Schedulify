#include "db_files.h"


DatabaseFileManager::DatabaseFileManager(QSqlDatabase& database) : db(database) {
}

int DatabaseFileManager::insertFile(const string& fileName, const string& fileType) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for file insertion");
        return -1;
    }

    if (fileName.empty()) {
        Logger::get().logError("Cannot insert file with empty name");
        return -1;
    }

    if (fileType.empty()) {
        Logger::get().logError("Cannot insert file with empty type");
        return -1;
    }

    // Test database connection before proceeding
    QSqlQuery connectionTest(db);
    if (!connectionTest.exec("SELECT 1")) {
        Logger::get().logError("Database connection test failed: " + connectionTest.lastError().text().toStdString());
        return -1;
    }

    // Check if file table exists
    QSqlQuery tableCheck(db);
    if (!tableCheck.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='file'")) {
        Logger::get().logError("Cannot check for file table existence: " + tableCheck.lastError().text().toStdString());
        return -1;
    }

    if (!tableCheck.next()) {
        Logger::get().logError("File table does not exist in database!");
        return -1;
    }

    // Prepare and execute the insertion query
    QSqlQuery query(db);
    query.prepare(R"(
        INSERT INTO file (file_name, file_type, upload_time, updated_at)
        VALUES (?, ?, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP)
    )");

    query.addBindValue(QString::fromStdString(fileName));
    query.addBindValue(QString::fromStdString(fileType));

    if (!query.exec()) {
        QString errorText = query.lastError().text();

        Logger::get().logError("File insertion query failed!");
        Logger::get().logError("Error text: " + errorText.toStdString());

        // Check if it's a constraint violation
        if (errorText.contains("constraint", Qt::CaseInsensitive) ||
            errorText.contains("unique", Qt::CaseInsensitive)) {
            Logger::get().logError("Constraint violation - possible duplicate or invalid data");
        }

        // Check if it's a permission issue
        if (errorText.contains("readonly", Qt::CaseInsensitive) ||
            errorText.contains("permission", Qt::CaseInsensitive)) {
            Logger::get().logError("Database permission issue - file may be read-only");
        }

        return -1;
    }

    // Get the ID of the inserted file
    QVariant lastId = query.lastInsertId();
    if (!lastId.isValid()) {
        Logger::get().logError("Failed to get file ID after insertion - lastInsertId() returned invalid");

        // Try alternative method to get the ID
        QSqlQuery idQuery(db);
        idQuery.prepare("SELECT id FROM file WHERE file_name = ? AND file_type = ? ORDER BY upload_time DESC LIMIT 1");
        idQuery.addBindValue(QString::fromStdString(fileName));
        idQuery.addBindValue(QString::fromStdString(fileType));

        if (idQuery.exec() && idQuery.next()) {
            int altId = idQuery.value(0).toInt();
            Logger::get().logInfo("Retrieved file ID using alternative method: " + std::to_string(altId));
            return altId;
        }

        Logger::get().logError("Alternative ID retrieval also failed");
        return -1;
    }

    int fileId = lastId.toInt();
    if (fileId <= 0) {
        Logger::get().logError("Invalid file ID generated: " + std::to_string(fileId));
        return -1;
    }

    Logger::get().logInfo("File: '" + fileName + "' inserted with ID: " + std::to_string(fileId));
    return fileId;
}

bool DatabaseFileManager::deleteFile(int fileId) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for file deletion");
        return false;
    }

    if (fileId <= 0) {
        Logger::get().logError("Invalid file ID for deletion: " + std::to_string(fileId));
        return false;
    }

    // First check if file exists and get its name for logging
    FileEntity existingFile = getFileById(fileId);
    if (existingFile.id == 0) {
        Logger::get().logWarning("Attempted to delete non-existent file with ID: " + std::to_string(fileId));
        return false;
    }

    QSqlQuery query(db);
    query.prepare("DELETE FROM file WHERE id = ?");
    query.addBindValue(fileId);

    if (!query.exec()) {
        Logger::get().logError("Failed to delete file ID " + std::to_string(fileId) + ": " + query.lastError().text().toStdString());
        return false;
    }

    int rowsAffected = query.numRowsAffected();
    if (rowsAffected == 0) {
        Logger::get().logWarning("No file was actually deleted for ID: " + std::to_string(fileId));
        return false;
    }

    Logger::get().logInfo("File deleted successfully: " + existingFile.file_name + " (ID: " + std::to_string(fileId) + ")");
    return true;
}

bool DatabaseFileManager::deleteAllFiles() {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for file deletion");
        return false;
    }

    QSqlQuery query("DELETE FROM file", db);
    if (!query.exec()) {
        Logger::get().logError("Failed to delete all files: " + query.lastError().text().toStdString());
        return false;
    }

    int rowsAffected = query.numRowsAffected();
    Logger::get().logInfo("Successfully deleted " + std::to_string(rowsAffected) + " files from database");

    return true;
}

vector<FileEntity> DatabaseFileManager::getAllFiles() {
    vector<FileEntity> files;

    if (!db.isOpen()) {
        Logger::get().logError("Database not open for file retrieval");
        return files;
    }

    QSqlQuery query(R"(
        SELECT id, file_name, file_type, upload_time, updated_at
        FROM file
        ORDER BY upload_time DESC
    )", db);

    if (!query.exec()) {
        Logger::get().logError("Failed to retrieve files: " + query.lastError().text().toStdString());
        return files;
    }

    while (query.next()) {
        try {
            FileEntity file = createFileEntityFromQuery(query);
            files.push_back(file);
        } catch (const std::exception& e) {
            Logger::get().logError("Error parsing file data: " + string(e.what()));
            continue; // Skip this file but continue with others
        }
    }

    Logger::get().logInfo("Successfully retrieved " + std::to_string(files.size()) + " files from database");
    return files;
}

FileEntity DatabaseFileManager::getFileById(int id) {
    FileEntity file; // Default constructor creates empty file with id = 0

    if (!db.isOpen()) {
        Logger::get().logError("Database not open for file retrieval");
        return file;
    }

    if (id <= 0) {
        Logger::get().logError("Invalid file ID for retrieval: " + std::to_string(id));
        return file;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        SELECT id, file_name, file_type, upload_time, updated_at
        FROM file WHERE id = ?
    )");
    query.addBindValue(id);

    if (!query.exec()) {
        Logger::get().logError("Failed to retrieve file by ID " + std::to_string(id) + ": " + query.lastError().text().toStdString());
        return file;
    }

    if (query.next()) {
        try {
            file = createFileEntityFromQuery(query);
            Logger::get().logInfo("Retrieved file: " + file.file_name + " (ID: " + std::to_string(id) + ")");
        } catch (const std::exception& e) {
            Logger::get().logError("Error parsing file data for ID " + std::to_string(id) + ": " + string(e.what()));
        }
    } else {
        Logger::get().logWarning("No file found with ID: " + std::to_string(id));
    }

    return file;
}

FileEntity DatabaseFileManager::getFileByName(const string& fileName) {
    FileEntity file; // Default constructor creates empty file with id = 0

    if (!db.isOpen()) {
        Logger::get().logError("Database not open for file retrieval");
        return file;
    }

    if (fileName.empty()) {
        Logger::get().logError("Cannot retrieve file with empty name");
        return file;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        SELECT id, file_name, file_type, upload_time, updated_at
        FROM file
        WHERE file_name = ?
        ORDER BY upload_time DESC
        LIMIT 1
    )");
    query.addBindValue(QString::fromStdString(fileName));

    if (!query.exec()) {
        Logger::get().logError("Failed to retrieve file by name '" + fileName + "': " + query.lastError().text().toStdString());
        return file;
    }

    if (query.next()) {
        try {
            file = createFileEntityFromQuery(query);
            Logger::get().logInfo("Retrieved file: " + fileName + " (ID: " + std::to_string(file.id) + ")");
        } catch (const std::exception& e) {
            Logger::get().logError("Error parsing file data for name '" + fileName + "': " + string(e.what()));
        }
    } else {
        Logger::get().logWarning("No file found with name: " + fileName);
    }

    return file;
}

int DatabaseFileManager::getFileIdByName(const string& fileName) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for file ID retrieval");
        return -1;
    }

    if (fileName.empty()) {
        Logger::get().logError("Cannot get file ID with empty name");
        return -1;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        SELECT id FROM file
        WHERE file_name = ?
        ORDER BY upload_time DESC
        LIMIT 1
    )");
    query.addBindValue(QString::fromStdString(fileName));

    if (!query.exec()) {
        Logger::get().logError("Failed to get file ID by name '" + fileName + "': " + query.lastError().text().toStdString());
        return -1;
    }

    if (query.next()) {
        int fileId = query.value(0).toInt();
        Logger::get().logInfo("Found file ID " + std::to_string(fileId) + " for name: " + fileName);
        return fileId;
    }

    Logger::get().logWarning("No file ID found for name: " + fileName);
    return -1;
}

bool DatabaseFileManager::fileExists(int fileId) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for file existence check");
        return false;
    }

    if (fileId <= 0) {
        return false; // Invalid IDs don't exist
    }

    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM file WHERE id = ?");
    query.addBindValue(fileId);

    if (!query.exec()) {
        Logger::get().logError("Failed to check file existence for ID " + std::to_string(fileId) + ": " + query.lastError().text().toStdString());
        return false;
    }

    if (query.next()) {
        return query.value(0).toInt() > 0;
    }

    return false;
}

FileEntity DatabaseFileManager::createFileEntityFromQuery(QSqlQuery& query) {
    FileEntity file;

    // Extract values with validation
    bool ok = false;

    // ID (required, must be positive)
    file.id = query.value(0).toInt(&ok);
    if (!ok || file.id <= 0) {
        throw std::runtime_error("Invalid file ID in query result");
    }

    // File name (required, cannot be empty)
    file.file_name = query.value(1).toString().toStdString();
    if (file.file_name.empty()) {
        throw std::runtime_error("Empty file name in query result");
    }

    // File type (required, cannot be empty)
    file.file_type = query.value(2).toString().toStdString();
    if (file.file_type.empty()) {
        throw std::runtime_error("Empty file type in query result");
    }

    // Upload time (required, must be valid)
    file.upload_time = query.value(3).toDateTime();
    if (!file.upload_time.isValid()) {
        Logger::get().logWarning("Invalid upload time for file: " + file.file_name);
        file.upload_time = QDateTime::currentDateTime(); // Fallback to current time
    }

    // Updated at (required, must be valid)
    file.updated_at = query.value(4).toDateTime();
    if (!file.updated_at.isValid()) {
        Logger::get().logWarning("Invalid updated_at time for file: " + file.file_name);
        file.updated_at = file.upload_time; // Fallback to upload time
    }

    return file;
}