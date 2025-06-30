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
Q_OBJECT
    Q_PROPERTY(ScheduleModel* scheduleModel READ scheduleModel CONSTANT)

public:
    explicit SchedulesDisplayController(QObject *parent = nullptr);
    ~SchedulesDisplayController() override;

    void loadScheduleData(const std::vector<InformativeSchedule>& schedules);

    // Semester-specific loading and switching
    Q_INVOKABLE void loadSemesterScheduleData(const QString& semester, const std::vector<InformativeSchedule>& schedules);
    Q_INVOKABLE void switchToSemester(const QString& semester);
    Q_INVOKABLE void allSemestersGenerated();

    // Reset to default semester
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
    // Include semester in filename
    static QString generateFilename(const QString& basePath, int index, fileType type, const QString& semester = "");

    // Methods to be called from CourseSelectionController
    void setSemesterLoading(const QString& semester, bool loading);
    void setSemesterFinished(const QString& semester, bool finished);
    void clearAllSchedules();


signals:
    void schedulesSorted(int totalCount);
    void screenshotSaved(const QString& path);
    void screenshotFailed();
    void botResponseReceived(const QString& response);
    void filterStateChanged();
    void schedulesFiltered(int filteredCount, int totalCount);

private slots:
    void onScheduleFilterStateChanged();
signals:
    void schedulesSorted(int totalCount);
    void screenshotSaved(const QString& path);
    void screenshotFailed();

    // Semester-specific signals
    void currentSemesterChanged();
    void semesterSchedulesLoaded(const QString& semester);
    void allSemestersReady();

    // Loading state signals
    void semesterLoadingStateChanged(const QString& semester);
    void semesterFinishedStateChanged(const QString& semester);

private:
    // Per-semester schedule storage
    std::vector<InformativeSchedule> m_schedulesA;
    std::vector<InformativeSchedule> m_schedulesB;
    std::vector<InformativeSchedule> m_schedulesSummer;

    // Semester management properties
    QString m_currentSemester = "A"; // Track which semester is currently being displayed
    bool m_allSemestersLoaded = false;

    // Loading state tracking
    QMap<QString, bool> m_semesterLoadingState;  // Track if semester is currently loading
    QMap<QString, bool> m_semesterFinishedState; // Track if semester has finished loading

    ScheduleModel* m_scheduleModel;
    IModel* modelConnection;
    QMap<QString, QString> m_sortKeyMap;

    // Track current sort state for optimization
    QString m_currentSortField;
    bool m_currentSortAscending = true;

    // Helper methods for bot
    BotQueryRequest createBotQueryRequest(const QString& userMessage);
    void handleBotResponse(const BotQueryResponse& response);

    // Helper methods
    std::vector<InformativeSchedule>* getCurrentScheduleVector();
};

#endif // SCHEDULES_DISPLAY_H