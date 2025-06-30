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

    // Data loading operations
    bool loadCoursesToDatabase(const vector<Course>& courses, const string& fileName = "", const string& fileType = "");

    // File operations
    bool insertFile(const string& fileName, const string& fileType);
    vector<FileEntity> getAllFiles();

    // Data retrieval operations
    vector<Course> getCoursesFromDatabase();

    // Get courses by file IDs with conflict resolution
    vector<Course> getCoursesByFileIds(const vector<int>& fileIds, vector<string>& warnings);

    // Utility operations
    bool clearAllDatabaseData();

    bool saveSchedulesToDatabase(const vector<InformativeSchedule>& schedules, const string& setName = "",
                                 const vector<int>& sourceFileIds = {});
    vector<InformativeSchedule> getSchedulesFromDatabase(int setId = -1);
    vector<ScheduleSetEntity> getScheduleSets();
    bool deleteScheduleSet(int setId);


private:
    ModelDatabaseIntegration() = default;
    ~ModelDatabaseIntegration() = default;

    // Helper methods
    void updateLastAccessMetadata() const;

    // State
    bool m_initialized = false;
    bool m_autoSaveEnabled = false;
};

#endif // MODEL_DATABASE_INTEGRATION_H