#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <vector>
#include <mutex>
#include <iomanip>
#include <fstream>
#include <QObject>
#include <sstream>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDateTime>

using std::string;
using std::vector;
using std::ostringstream;
using std::put_time;
using std::endl;

enum class LogLevel {
    INFO,
    ERR,
    WARNING,
    INITIATE
};

struct LogEntry {
    string timestamp;
    LogLevel level;
    string message;
};

class Logger : public QObject {
Q_OBJECT
public:
    static Logger& get();
    void logInitiate();

    void logInfo(const string& message);
    void logError(const string& message);
    void logWarning(const string& message);

    const vector<LogEntry>& getLogs() const;

    // New method for downloading logs
    Q_INVOKABLE bool downloadLogs();

    // message collection (for course validator)
    void startCollecting();
    void stopCollecting();
    void clearCollected();
    vector<string> getCollectedWarnings() const;
    vector<string> getCollectedErrors() const;
    vector<string> getAllCollectedMessages() const;
    bool isCollecting() const;

signals:
    void logAdded();
    void logsDownloaded(const QString& filePath);
    void downloadFailed(const QString& error);  // ADDED: Missing signal

private:
    Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    void log(LogLevel level, const string& message);
    static string getTimeStamp();
    static string logLevelToString(LogLevel level);

    vector<LogEntry> logList;
    mutable std::mutex logMutex;

    // message collection (for course validator)
    vector<string> collectedWarnings;
    vector<string> collectedErrors;
    bool collectingEnabled;
    mutable std::mutex collectionMutex;
};

#endif // LOGGER_H