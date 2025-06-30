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

    Q_INVOKABLE void loadSemesterScheduleData(const QString& semester, const vector<InformativeSchedule>& schedules);
    Q_INVOKABLE void switchToSemester(const QString& semester);
    Q_INVOKABLE void allSemestersGenerated();

    Q_INVOKABLE void resetToSemesterA();

    // Properties
    ScheduleModel* scheduleModel() const { return m_scheduleModel; }
    bool isFiltered() const { return m_scheduleModel != nullptr && m_scheduleModel->isFiltered(); }

    // Semester query methods
    Q_INVOKABLE QString getCurrentSemester() const { return m_currentSemester; }
    Q_INVOKABLE bool hasSchedulesForSemester(const QString& semester) const;
    Q_INVOKABLE int getScheduleCountForSemester(const QString& semester) const;

    // Loading state management methods
    Q_INVOKABLE bool isSemesterLoading(const QString& semester) const;
    Q_INVOKABLE bool isSemesterFinished(const QString& semester) const;
    Q_INVOKABLE bool canClickSemester(const QString& semester) const;
    void setSemesterLoading(const QString& semester, bool loading);
    void setSemesterFinished(const QString& semester, bool finished);
    void clearAllSchedules();

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

    // Bot message processing
    Q_INVOKABLE void processBotMessage(const QString& userMessage);

    static QString generateFilename(const QString& basePath, int index, fileType type, const QString& semester = "");

signals:
    // sort signals
    void schedulesSorted(int totalCount);

    // screenshot signals
    void screenshotSaved(const QString& path);
    void screenshotFailed();

    // filter bot signals
    void botResponseReceived(const QString& response);
    void filterStateChanged();
    void schedulesFiltered(int filteredCount, int totalCount);

    // Semester-specific signals
    void currentSemesterChanged();
    void semesterSchedulesLoaded(const QString& semester);
    void allSemestersReady();

    // Loading state signals
    void semesterLoadingStateChanged(const QString& semester);
    void semesterFinishedStateChanged(const QString& semester);

private slots:
    void onScheduleFilterStateChanged();

private:
    // schedule model properties
    ScheduleModel* m_scheduleModel;
    IModel* modelConnection;

    // Per-semester schedule storage
    vector<InformativeSchedule> m_schedulesA;
    vector<InformativeSchedule> m_schedulesB;
    vector<InformativeSchedule> m_schedulesSummer;

    // Semester management properties
    QString m_currentSemester = "A";
    bool m_allSemestersLoaded = false;

    // Loading state tracking
    QMap<QString, bool> m_semesterLoadingState;
    QMap<QString, bool> m_semesterFinishedState;

    // sort properties
    QMap<QString, QString> m_sortKeyMap;
    QString m_currentSortField;
    bool m_currentSortAscending = true;

    // Helper methods
    BotQueryRequest createBotQueryRequest(const QString& userMessage);
    void handleBotResponse(const BotQueryResponse& response);
    vector<InformativeSchedule>* getCurrentScheduleVector();
};

#endif // SCHEDULES_DISPLAY_H