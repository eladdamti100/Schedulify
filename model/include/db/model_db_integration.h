#ifndef MODEL_DATABASE_INTEGRATION_H
#define MODEL_DATABASE_INTEGRATION_H

#include "model_interfaces.h"
#include "db_manager.h"
#include "logger.h"

#include <vector>
#include <string>
#include <functional>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>

using namespace std;

class ModelDatabaseIntegration {
public:
    static ModelDatabaseIntegration& getInstance();

    // Initialization
    bool initializeDatabase(const string& dbPath = "");
    bool isInitialized() const;

    // Data loading operations - updated for new course fields
    bool loadCoursesToDatabase(const vector<Course>& courses, const string& fileName = "", const string& fileType = "");

    // File operations
    bool insertFile(const string& fileName, const string& fileType);
    vector<FileEntity> getAllFiles();

    // Data retrieval operations - updated for conflict resolution using course_key
    vector<Course> getCoursesFromDatabase();

    // Get courses by file IDs with enhanced conflict resolution using course_key
    vector<Course> getCoursesByFileIds(const vector<int>& fileIds, vector<string>& warnings);

    // Semester-specific course retrieval
    vector<Course> getCoursesBySemester(int semester);
    vector<Course> getCoursesByFileIdAndSemester(int fileId, int semester);

    // Utility operations
    bool clearAllDatabaseData();

    // Schedule operations - updated for semester support
    bool saveSchedulesToDatabase(const vector<InformativeSchedule>& schedules);
    vector<InformativeSchedule> getSchedulesFromDatabase();
    vector<InformativeSchedule> getSchedulesBySemester(int semester);

    // Course validation and conflict detection
    vector<string> detectCourseConflicts(const vector<int>& fileIds);
    bool validateCourseConsistency(const vector<Course>& courses);

private:
    ModelDatabaseIntegration() = default;
    ~ModelDatabaseIntegration() = default;

    // Helper methods
    void updateLastAccessMetadata() const;

    // Course processing helpers - updated for new fields
    bool generateAndSetUniqueIds(vector<Course>& courses, int fileId);
    bool validateCourseFields(const Course& course) const;
    void ensureCourseKeysGenerated(vector<Course>& courses);

    // State
    bool m_initialized = false;
    bool m_autoSaveEnabled = false;
};

#endif // MODEL_DATABASE_INTEGRATION_H