#include "logger.h"
#include <QApplication>
#include <QMessageBox>

Logger::Logger() : QObject(nullptr) {}

Logger& Logger::get() {
    static Logger instance;
    return instance;
}

void Logger::logInitiate() {
    log(LogLevel::INITIATE, "initiate application");
}

void Logger::log(LogLevel level, const string& message) {
    LogEntry entry;
    entry.timestamp = getTimeStamp();
    entry.level = level;
    entry.message = message;

    {
        std::lock_guard<std::mutex> lock(logMutex);
        logList.push_back(entry);
    }

    // Collect messages if collection is enabled - use separate lock
    {
        std::lock_guard<std::mutex> lock(collectionMutex);
        if (collectingEnabled) {
            try {
                if (level == LogLevel::WARNING) {
                    collectedWarnings.push_back(message);
                } else if (level == LogLevel::ERR) {
                    collectedErrors.push_back(message);
                }
            } catch (const std::exception& e) {
                // Don't let collection errors crash the logger
                collectingEnabled = false;
            }
        }
    }

    emit logAdded();
}

void Logger::logInfo(const string& message) {
    log(LogLevel::INFO, message);
}

void Logger::logError(const string& message) {
    log(LogLevel::ERR, message);
}

void Logger::logWarning(const string &message) {
    log(LogLevel::WARNING, message);
}

const vector<LogEntry>& Logger::getLogs() const {
    return logList;
}

bool Logger::downloadLogs() {
    try {
        // Get default download path
        QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (defaultPath.isEmpty()) {
            defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }

        // Generate default filename with current timestamp
        QString defaultFileName = QString("schedulify_logs_%1.txt")
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss"));

        QString defaultFilePath = defaultPath + "/" + defaultFileName;

        // Show file dialog
        QString filePath = QFileDialog::getSaveFileName(
                nullptr,
                "Save Logs As",
                defaultFilePath,
                "Text Files (*.txt);;All Files (*)"
        );

        if (filePath.isEmpty()) {
            // User cancelled
            return false;
        }

        // Ensure .txt extension
        if (!filePath.endsWith(".txt", Qt::CaseInsensitive)) {
            filePath += ".txt";
        }

        // Create and write to file
        std::ofstream file(filePath.toStdString());
        if (!file.is_open()) {
            emit downloadFailed("Could not create file: " + filePath);
            return false;
        }

        // Write header
        file << "Schedulify Logs Export" << std::endl;
        file << "Generated: " << getTimeStamp() << std::endl;
        file << "Total Entries: ";

        {
            std::lock_guard<std::mutex> lock(logMutex);
            file << logList.size() << std::endl;
            file << "================================" << std::endl << std::endl;

            // Write all log entries
            for (const auto& entry : logList) {
                file << "[" << entry.timestamp << "] "
                     << "[" << logLevelToString(entry.level) << "] "
                     << entry.message << std::endl;
            }
        }

        file.close();

        emit logsDownloaded(filePath);
        return true;

    } catch (const std::exception& e) {
        emit downloadFailed(QString("Error saving logs: %1").arg(e.what()));
        return false;
    }
}

string Logger::getTimeStamp() {
    time_t now = time(nullptr);
    tm* localTime = localtime(&now);

    ostringstream oss;
    oss << put_time(localTime, "%d/%m/%y-%H:%M:%S");
    return oss.str();
}

string Logger::logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::INFO: return "INFO";
        case LogLevel::ERR: return "ERROR";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::INITIATE: return "INITIATE";
        default: return "UNKNOWN";
    }
}

void Logger::startCollecting() {
    std::lock_guard<std::mutex> lock(collectionMutex);
    collectingEnabled = true;
    collectedWarnings.clear();
    collectedErrors.clear();
}

void Logger::stopCollecting() {
    std::lock_guard<std::mutex> lock(collectionMutex);
    collectingEnabled = false;
}

void Logger::clearCollected() {
    std::lock_guard<std::mutex> lock(collectionMutex);
    collectedWarnings.clear();
    collectedErrors.clear();
}

vector<string> Logger::getCollectedWarnings() const {
    std::lock_guard<std::mutex> lock(collectionMutex);
    return collectedWarnings;
}

vector<string> Logger::getCollectedErrors() const {
    std::lock_guard<std::mutex> lock(collectionMutex);
    return collectedErrors;
}

vector<string> Logger::getAllCollectedMessages() const {
    std::lock_guard<std::mutex> lock(collectionMutex);
    vector<string> allMessages;

    // Add warnings with prefix
    for (const auto& warning : collectedWarnings) {
        allMessages.push_back("[Parser Warning] " + warning);
    }

    // Add errors with prefix
    for (const auto& error : collectedErrors) {
        allMessages.push_back("[Parser Error] " + error);
    }

    return allMessages;
}

bool Logger::isCollecting() const {
    std::lock_guard<std::mutex> lock(collectionMutex);
    return collectingEnabled;
}