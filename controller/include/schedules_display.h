#ifndef SCHEDULES_DISPLAY_H
#define SCHEDULES_DISPLAY_H

#include "controller_manager.h"
#include "model_access.h"
#include "model_interfaces.h"
#include "schedule_model.h"
#include "ChatBot.h"

#include <QObject>
#include <QVariant>
#include <QVariantMap>
#include <QVariantList>
#include <QFileDialog>
#include <QDir>
#include <QQuickItem>
#include <QStandardPaths>
#include <QQuickItemGrabResult>
#include <QtQuick/QQuickItem>
#include <QThread>
#include <algorithm>
#include <vector>
#include <QDebug>

enum class fileType {
    PNG,
    CSV
};

class SchedulesDisplayController : public ControllerManager {
Q_OBJECT
    Q_PROPERTY(ScheduleModel* scheduleModel READ scheduleModel CONSTANT)
    Q_PROPERTY(bool isFiltered READ isFiltered NOTIFY filterStateChanged)

public:
    explicit SchedulesDisplayController(QObject *parent = nullptr);
    ~SchedulesDisplayController() override;

    void loadScheduleData(const std::vector<InformativeSchedule>& schedules);

    // Properties
    ScheduleModel* scheduleModel() const { return m_scheduleModel; }
    bool isFiltered() const { return m_scheduleModel != nullptr && m_scheduleModel->isFiltered(); }

    // QML accessible methods
    Q_INVOKABLE void goBack() override;
    Q_INVOKABLE void saveScheduleAsCSV();
    Q_INVOKABLE void printScheduleDirectly();
    Q_INVOKABLE void captureAndSave(QQuickItem* item, const QString& savePath = QString());

    // Sorting methods
    Q_INVOKABLE void applySorting(const QVariantMap& sortData);
    Q_INVOKABLE void clearSorting();

    // Filter methods
    Q_INVOKABLE void resetFilters();

    // Bot message processing - completely rewritten
    Q_INVOKABLE void processBotMessage(const QString& userMessage);

    static QString generateFilename(const QString& basePath, int index, fileType type);

signals:
    void schedulesSorted(int totalCount);
    void screenshotSaved(const QString& path);
    void screenshotFailed();
    void botResponseReceived(const QString& response);
    void filterStateChanged();
    void schedulesFiltered(int filteredCount, int totalCount);

private slots:
    void onScheduleFilterStateChanged();

private:
    std::vector<InformativeSchedule> m_schedules;
    ScheduleModel* m_scheduleModel;
    IModel* modelConnection;
    QMap<QString, QString> m_sortKeyMap;

    QString m_currentSortField;
    bool m_currentSortAscending = true;

    // Helper methods for bot
    BotQueryRequest createBotQueryRequest(const QString& userMessage);
    void handleBotResponse(const BotQueryResponse& response);
};

#endif // SCHEDULES_DISPLAY_H