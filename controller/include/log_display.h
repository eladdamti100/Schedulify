#ifndef LOG_CONTROLLER_H
#define LOG_CONTROLLER_H

#include <QObject>
#include <QVariant>
#include <QVariantList>
#include <QColor>
#include <QDateTime>
#include <QJsonObject>
#include <QCoreApplication>
#include <QPointer>
#include "logger.h"
#include "controller_manager.h"

class LogDisplayController : public ControllerManager {
Q_OBJECT
    Q_PROPERTY(QVariantList logEntries READ getLogEntries NOTIFY logEntriesChanged)
    Q_PROPERTY(bool isLogWindowOpen READ isLogWindowOpen WRITE setLogWindowOpen NOTIFY logWindowOpenChanged)

public:
    explicit LogDisplayController(QObject* parent = nullptr);
    virtual ~LogDisplayController() override = default;

    Q_INVOKABLE bool isLogWindowOpen() const { return m_isLogWindowOpen; }
    Q_INVOKABLE void setLogWindowOpen(bool open) {
        if (m_isLogWindowOpen != open) {
            m_isLogWindowOpen = open;
            emit logWindowOpenChanged();
        }
    }

    Q_INVOKABLE void refreshLogs();
    Q_INVOKABLE void forceUpdate() {
        updateLogEntries();
        emit logEntriesChanged();
    }

    // Method to get log entries as QVariantList for QML
    QVariantList getLogEntries() const;


    // Helper method to convert LogLevel to QColor
    static QColor getColorForLogLevel(LogLevel level);

    // Helper method to convert LogLevel to string
    static QString getStringForLogLevel(LogLevel level);

signals:
    void logEntriesChanged();
    void logWindowOpenChanged();

private:
    void updateLogEntries();
    QVariantList m_logEntries;
    bool m_isLogWindowOpen = false;
};


#endif //LOG_CONTROLLER_H
