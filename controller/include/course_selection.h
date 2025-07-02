#ifndef COURSE_SELECTION_H
#define COURSE_SELECTION_H

#include "course_model.h"
#include "ScheduleGenerator.h"
#include "CourseValidator.h"
#include "model_access.h"
#include "controller_manager.h"
#include "schedules_display.h"
#include "logger.h"

#include <algorithm>
#include <QUrl>
#include <QStringList>
#include <QThread>
#include <vector>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QTimer>

// Time block for blocking schedule slots
struct BlockTime {
    QString day;
    QString startTime;
    QString endTime;
    QString semester; // semester assignment

    BlockTime(const QString& d, const QString& st, const QString& et, const QString& sem = "A")
            : day(d), startTime(st), endTime(et), semester(sem) {} // default to semester A
};

class CourseSelectionController final : public ControllerManager {
Q_OBJECT

    Q_PROPERTY(CourseModel* courseModel READ courseModel CONSTANT)
    Q_PROPERTY(CourseModel* selectedCoursesModel READ selectedCoursesModel CONSTANT)
    Q_PROPERTY(CourseModel* filteredCourseModel READ filteredCourseModel CONSTANT)
    Q_PROPERTY(CourseModel* blocksModel READ blocksModel CONSTANT)
    Q_PROPERTY(bool validationInProgress READ validationInProgress NOTIFY validationStateChanged)
    Q_PROPERTY(QStringList validationErrors READ validationErrors NOTIFY validationStateChanged)

public:
    explicit CourseSelectionController(QObject *parent = nullptr);
    ~CourseSelectionController() override;

    [[nodiscard]] CourseModel* courseModel() const { return m_courseModel; }
    [[nodiscard]] CourseModel* selectedCoursesModel() const { return m_selectedCoursesModel; }
    [[nodiscard]] CourseModel* filteredCourseModel() const { return m_filteredCourseModel; }
    [[nodiscard]] CourseModel* blocksModel() const { return m_blocksModel; }
    [[nodiscard]] bool validationInProgress() const { return m_validationInProgress; }
    [[nodiscard]] QStringList validationErrors() const { return m_validationErrors; }

    void initiateCoursesData(const vector<Course>& courses);

    Q_INVOKABLE bool isCourseSelected(int index);
    Q_INVOKABLE void addBlockTimeToSemester(const QString& day, const QString& startTime,
                                            const QString& endTime, const QString& semester);
    Q_INVOKABLE void toggleCourseSelection(int index);
    Q_INVOKABLE void filterCourses(const QString &text);
    Q_INVOKABLE void resetFilter();
    Q_INVOKABLE void generateSchedules();
    Q_INVOKABLE void deselectCourse(int index);
    Q_INVOKABLE void createNewCourse(const QString& courseName, const QString& courseId,
                                     const QString& teacherName, int semester, const QVariantList& sessionGroups);

    // Semester filtering functionality
    Q_INVOKABLE void filterBySemester(const QString& semester);

    // Semester-specific utilities

    // Get course's semester
    Q_INVOKABLE QString getCourseSemester(int courseIndex);

    // Check if a course can be added to semester
    Q_INVOKABLE bool canAddCourseToSemester(int courseIndex);

    Q_INVOKABLE void addBlockTime(const QString& day, const QString& startTime, const QString& endTime);
    Q_INVOKABLE void removeBlockTime(int index);
    Q_INVOKABLE void clearAllBlockTimes();

    Q_INVOKABLE void setupValidationTimeout(int timeoutMs);
    Q_INVOKABLE int getSelectedCoursesCountForSemester(const QString& semester);
    Q_INVOKABLE QVariantList getSelectedCoursesForSemester(const QString& semester);

private slots:
    void onValidationTimeout();

    // Handle semester schedule generation completion
    void onSemesterSchedulesGenerated(const QString& semester, vector<InformativeSchedule>* schedules);

signals:
    void selectionChanged();
    void blockTimesChanged();
    void errorMessage(const QString &message);
    void validationStateChanged();

    // Emitted when semester schedules are ready
    void semesterSchedulesGenerated(const QString& semester, vector<InformativeSchedule>* schedules);

private:
    CourseModel* m_courseModel;
    CourseModel* m_selectedCoursesModel;
    CourseModel* m_filteredCourseModel;
    CourseModel* m_blocksModel;
    QTimer* validationTimeoutTimer = nullptr;
    bool validationCompleted = false;
    bool m_validationInProgress = false;
    QStringList m_validationErrors;
    bool hasNavigatedToSchedules = false;

    vector<Course> allCourses;
    vector<Course> filteredCourses;
    vector<Course> blockTimes;
    vector<BlockTime> userBlockTimes;

    // Per-semester course selections
    vector<Course> selectedCoursesA;
    vector<Course> selectedCoursesB;
    vector<Course> selectedCoursesSummer;

    vector<int> selectedIndicesA;
    vector<int> selectedIndicesB;
    vector<int> selectedIndicesSummer;

    vector<int> filteredIndicesMap;
    QString currentSearchText;

    // Semester filtering state: "ALL", "A", "B", "SUMMER"
    QString currentSemesterFilter = "ALL";

    IModel* modelConnection;
    QThread* validatorThread = nullptr;
    QThread* workerThread = nullptr;

    // Semester-specific helper methods
    void updateSelectedCoursesModel();
    void generateSemesterSchedules(const QString& semester);
    void checkAndNavigateToSchedules();

    void updateBlockTimesModel();
    Course createSingleBlockTimeCourse();
    static int getDayNumber(const QString& dayName);
    Course createCourseFromData(const QString& courseName, const QString& courseId,
                                const QString& teacherName, const QVariantList& sessionGroups);
    void validateCourses(const vector<Course>& courses, int timeoutMs);
    void onCoursesValidated(vector<string>* errors);
    void cleanupValidation();
    void cleanupValidatorThread();
    void setValidationInProgress(bool inProgress);
    void setValidationErrors(const QStringList& errors);

    // Helper methods for semester filtering
    void applyFilters();
    bool matchesSemesterFilter(const Course& course) const;
    bool matchesSearchFilter(const Course& course, const QString& searchText) const;

    // Per-semester block time handling
    vector<BlockTime> getBlockTimesForCurrentSemester(const QString& semester);
    Course createSingleBlockTimeCourseForSemester(const vector<BlockTime>& semesterBlockTimes, const QString& semester);

    inline static const int VALIDATION_TIMEOUT_MS = 60000;
    inline static const int THREAD_CLEANUP_TIMEOUT_MS = 10000;
    inline static const int MAX_COURSES_LIMIT = 1000;
};

#endif //COURSE_SELECTION_H