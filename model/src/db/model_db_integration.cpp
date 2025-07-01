#include "model_db_integration.h"

ModelDatabaseIntegration& ModelDatabaseIntegration::getInstance() {
    static ModelDatabaseIntegration instance;
    return instance;
}

// initialize db

bool ModelDatabaseIntegration::initializeDatabase(const string& dbPath) {
    QString qDbPath = dbPath.empty() ? QString() : QString::fromStdString(dbPath);

    // Check if already initialized
    if (m_initialized && DatabaseManager::getInstance().isConnected()) {
        return true;
    }

    // Try to initialize database with comprehensive error checking
    try {
        if (!DatabaseManager::getInstance().initializeDatabase(qDbPath)) {
            Logger::get().logError("CRITICAL: DatabaseManager initialization failed");

            // Try to diagnose the issue
            Logger::get().logInfo("=== DATABASE DIAGNOSTIC ===");

            // Check if SQLite driver is available
            QStringList drivers = QSqlDatabase::drivers();
            Logger::get().logInfo("Available SQL drivers: " + drivers.join(", ").toStdString());

            if (!drivers.contains("QSQLITE")) {
                Logger::get().logError("QSQLITE driver not available - this is a critical issue");
                return false;
            }

            // Check directory permissions
            QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            Logger::get().logInfo("App data path: " + appDataPath.toStdString());

            QDir appDir(appDataPath);
            if (!appDir.exists()) {
                Logger::get().logInfo("Creating app data directory...");
                if (!appDir.mkpath(appDataPath)) {
                    Logger::get().logError("Cannot create app data directory - permission issue");
                    return false;
                }
            }

            return false;
        }

        m_initialized = true;

        // Set up initial metadata
        updateLastAccessMetadata();

        Logger::get().logInfo("Database integration initialized successfully with enhanced course support");

        return true;

    } catch (const exception& e) {
        Logger::get().logError("Exception during database initialization: " + string(e.what()));
        m_initialized = false;
        return false;
    }
}

bool ModelDatabaseIntegration::isInitialized() const {
    return m_initialized && DatabaseManager::getInstance().isConnected();
}

// Main db management

bool ModelDatabaseIntegration::clearAllDatabaseData() {
    if (!isInitialized()) {
        Logger::get().logError("Database not initialized for clearing");
        return false;
    }

    if (!DatabaseManager::getInstance().clearAllData()) {
        Logger::get().logError("Failed to clear database data");
        return false;
    }

    // Re-initialize basic metadata
    auto& db = DatabaseManager::getInstance();
    db.insertMetadata("schema_version", to_string(DatabaseManager::getCurrentSchemaVersion()), "Database schema version");
    db.insertMetadata("created_at", QDateTime::currentDateTime().toString(Qt::ISODate).toStdString(), "Database creation timestamp");
    updateLastAccessMetadata();

    Logger::get().logInfo("Database data cleared successfully");
    return true;
}

void ModelDatabaseIntegration::updateLastAccessMetadata() const {
    if (isInitialized()) {
        auto& db = DatabaseManager::getInstance();
        db.updateMetadata("last_access", QDateTime::currentDateTime().toString(Qt::ISODate).toStdString());
    }
}

// Files management methods

bool ModelDatabaseIntegration::insertFile(const string& fileName, const string& fileType) {
    if (!isInitialized()) {
        Logger::get().logError("Database not initialized for file insertion");
        return false;
    }

    auto& db = DatabaseManager::getInstance();
    int fileId = db.files()->insertFile(fileName, fileType);
    if (fileId <= 0) {
        Logger::get().logError("Failed to insert file into database");
        return false;
    }

    updateLastAccessMetadata();
    Logger::get().logInfo("Successfully inserted file: " + fileName + " with ID: " + to_string(fileId));
    return true;
}

vector<FileEntity> ModelDatabaseIntegration::getAllFiles() {
    if (!isInitialized()) {
        Logger::get().logError("Database not initialized for file retrieval");
        return {};
    }

    try {
        auto& db = DatabaseManager::getInstance();

        // Verify database connection
        if (!db.isConnected()) {
            Logger::get().logError("Database connection lost during file retrieval");
            return {};
        }

        auto files = db.files()->getAllFiles();
        updateLastAccessMetadata();

        Logger::get().logInfo("Retrieved " + to_string(files.size()) + " files from database");

        // Log file details for debugging
        if (files.empty()) {
            Logger::get().logInfo("No files found in database - this is normal for first use");
        }
        return files;

    } catch (const exception& e) {
        Logger::get().logError("Exception during file retrieval: " + string(e.what()));
        return {};
    }
}

// Courses management methods

bool ModelDatabaseIntegration::loadCoursesToDatabase(const vector<Course>& courses, const string& fileName,
                                                     const string& fileType) {
    if (!isInitialized()) {
        Logger::get().logError("Database not initialized for course loading");
        return false;
    }

    if (courses.empty()) {
        Logger::get().logWarning("No courses provided to load into database");
        return true;
    }

    if (fileName.empty()) {
        Logger::get().logError("File name is required for course loading");
        return false;
    }

    if (fileType.empty()) {
        Logger::get().logError("File type is required for course loading");
        return false;
    }

    try {
        auto& db = DatabaseManager::getInstance();

        // Verify database connection before proceeding
        if (!db.isConnected()) {
            Logger::get().logError("Database connection lost during course loading");
            return false;
        }

        // Create a new file entry for each upload
        int fileId = db.files()->insertFile(fileName, fileType);

        if (fileId <= 0) {
            Logger::get().logError("Failed to create file entry for: " + fileName);
            return false;
        }

        // Make a copy of courses to modify
        vector<Course> coursesToSave = courses;

        // Generate unique IDs for all courses
        if (!generateAndSetUniqueIds(coursesToSave, fileId)) {
            Logger::get().logError("Failed to generate unique IDs for courses");
            return false;
        }

        // Validate all courses have required fields
        if (!validateCourseConsistency(coursesToSave)) {
            Logger::get().logError("Course validation failed - some courses missing required fields");
            return false;
        }

        // Insert courses into database
        if (!db.courses()->insertCourses(coursesToSave, fileId)) {
            Logger::get().logError("Failed to insert courses into database for file ID: " + to_string(fileId));
            Logger::get().logWarning("File entry created but courses not saved - partial database state");
            return false;
        }

        // Update metadata
        db.updateMetadata("courses_loaded_at", QDateTime::currentDateTime().toString(Qt::ISODate).toStdString());
        db.updateMetadata("courses_count", to_string(coursesToSave.size()));
        db.updateMetadata("last_loaded_file", fileName);
        db.updateMetadata("last_file_type", fileType);

        updateLastAccessMetadata();

        Logger::get().logInfo("SUCCESS: All data saved to database with enhanced course fields");
        Logger::get().logInfo("File ID: " + to_string(fileId) + ", Courses: " + to_string(coursesToSave.size()));

        // Log semester distribution
        map<int, int> semesterCounts;
        for (const auto& course : coursesToSave) {
            semesterCounts[course.semester]++;
        }

        Logger::get().logInfo("Course distribution by semester:");
        for (const auto& [semester, count] : semesterCounts) {
            string semesterName;
            switch (semester) {
                case 1: semesterName = "Semester A"; break;
                case 2: semesterName = "Semester B"; break;
                case 3: semesterName = "Summer"; break;
                case 4: semesterName = "Year-long"; break;
                default: semesterName = "Unknown"; break;
            }
            Logger::get().logInfo("  " + semesterName + ": " + to_string(count) + " courses");
        }

        return true;

    } catch (const exception& e) {
        Logger::get().logError("Exception during course loading: " + string(e.what()));
        return false;
    }
}

vector<Course> ModelDatabaseIntegration::getCoursesByFileIds(const vector<int>& fileIds, vector<string>& warnings) {
    warnings.clear();

    if (!isInitialized()) {
        Logger::get().logError("Database not initialized for course retrieval by file IDs");
        return {};
    }

    if (fileIds.empty()) {
        Logger::get().logWarning("No file IDs provided for course retrieval");
        return {};
    }

    // Use enhanced conflict resolution by course_key
    auto courses = DatabaseManager::getInstance().courses()->getCoursesByFileIds(fileIds, warnings);
    updateLastAccessMetadata();

    // Log conflict warnings that were generated
    if (!warnings.empty()) {
        Logger::get().logWarning("Resolved " + to_string(warnings.size()) + " course conflicts using course_key");
        for (const string& warning : warnings) {
            Logger::get().logWarning("CONFLICT: " + warning);
        }
    }

    // Log semester distribution of resolved courses
    map<int, int> semesterCounts;
    for (const auto& course : courses) {
        semesterCounts[course.semester]++;
    }

    Logger::get().logInfo("=== CONFLICT RESOLUTION COMPLETE ===");
    Logger::get().logInfo("Final course distribution by semester:");
    for (const auto& [semester, count] : semesterCounts) {
        string semesterName;
        switch (semester) {
            case 1: semesterName = "Semester A"; break;
            case 2: semesterName = "Semester B"; break;
            case 3: semesterName = "Summer"; break;
            case 4: semesterName = "Year-long"; break;
            default: semesterName = "Unknown"; break;
        }
        Logger::get().logInfo("  " + semesterName + ": " + to_string(count) + " unique courses");
    }

    return courses;
}

vector<Course> ModelDatabaseIntegration::getCoursesFromDatabase() {
    if (!isInitialized()) {
        Logger::get().logError("Database not initialized for course retrieval");
        return {};
    }

    auto courses = DatabaseManager::getInstance().courses()->getAllCourses();
    updateLastAccessMetadata();

    Logger::get().logInfo("Retrieved " + to_string(courses.size()) + " courses from database");

    return courses;
}

vector<Course> ModelDatabaseIntegration::getCoursesBySemester(int semester) {
    if (!isInitialized()) {
        Logger::get().logError("Database not initialized for semester course retrieval");
        return {};
    }

    auto courses = DatabaseManager::getInstance().courses()->getCoursesBySemester(semester);
    updateLastAccessMetadata();

    string semesterName;
    switch (semester) {
        case 1: semesterName = "Semester A"; break;
        case 2: semesterName = "Semester B"; break;
        case 3: semesterName = "Summer"; break;
        case 4: semesterName = "Year-long"; break;
        default: semesterName = "Semester " + to_string(semester); break;
    }

    Logger::get().logInfo("Retrieved " + to_string(courses.size()) + " courses for " + semesterName);
    return courses;
}

vector<Course> ModelDatabaseIntegration::getCoursesByFileIdAndSemester(int fileId, int semester) {
    if (!isInitialized()) {
        Logger::get().logError("Database not initialized for file+semester course retrieval");
        return {};
    }

    auto courses = DatabaseManager::getInstance().courses()->getCoursesByFileIdAndSemester(fileId, semester);
    updateLastAccessMetadata();

    string semesterName;
    switch (semester) {
        case 1: semesterName = "Semester A"; break;
        case 2: semesterName = "Semester B"; break;
        case 3: semesterName = "Summer"; break;
        case 4: semesterName = "Year-long"; break;
        default: semesterName = "Semester " + to_string(semester); break;
    }

    Logger::get().logInfo("Retrieved " + to_string(courses.size()) +
                          " courses for file " + to_string(fileId) + ", " + semesterName);
    return courses;
}

vector<string> ModelDatabaseIntegration::detectCourseConflicts(const vector<int>& fileIds) {
    if (!isInitialized()) {
        Logger::get().logError("Database not initialized for conflict detection");
        return {};
    }

    return DatabaseManager::getInstance().courses()->detectConflicts(fileIds);
}

bool ModelDatabaseIntegration::validateCourseConsistency(const vector<Course>& courses) {
    for (const auto& course : courses) {
        if (!validateCourseFields(course)) {
            return false;
        }
    }
    return true;
}

bool ModelDatabaseIntegration::generateAndSetUniqueIds(vector<Course>& courses, int fileId) {
    try {
        for (auto& course : courses) {
            // Generate unique ID: courseId_f{fileId}_s{semester}
            course.generateUniqueId(fileId);

            // Ensure course_key is also set: courseId_s{semester}
            if (course.course_key.empty()) {
                course.generateCourseKey();
            }

            Logger::get().logInfo("Generated IDs for course " + to_string(course.id) +
                                  ": uniqid=" + course.uniqid + ", course_key=" + course.course_key);
        }
        return true;
    } catch (const exception& e) {
        Logger::get().logError("Exception generating unique IDs: " + string(e.what()));
        return false;
    }
}

bool ModelDatabaseIntegration::validateCourseFields(const Course& course) const {
    if (course.uniqid.empty()) {
        Logger::get().logError("Course " + to_string(course.id) + " missing uniqid");
        return false;
    }

    if (course.course_key.empty()) {
        Logger::get().logError("Course " + to_string(course.id) + " missing course_key");
        return false;
    }

    if (!course.hasValidSemester()) {
        Logger::get().logError("Course " + to_string(course.id) + " has invalid semester: " + to_string(course.semester));
        return false;
    }

    // Validate course_key format: should be courseId_s{semester}
    string expectedKey = to_string(course.id) + "_s" + to_string(course.semester);
    if (course.course_key != expectedKey) {
        Logger::get().logError("Course " + to_string(course.id) + " has invalid course_key format. Expected: " +
                               expectedKey + ", Got: " + course.course_key);
        return false;
    }

    return true;
}

void ModelDatabaseIntegration::ensureCourseKeysGenerated(vector<Course>& courses) {
    for (auto& course : courses) {
        if (course.course_key.empty()) {
            course.generateCourseKey();
            Logger::get().logInfo("Generated missing course_key for course " + to_string(course.id) +
                                  ": " + course.course_key);
        }
    }
}

// Schedules management methods

bool ModelDatabaseIntegration::saveSchedulesToDatabase(const vector<InformativeSchedule>& schedules) {
    if (!isInitialized()) {
        Logger::get().logError("Database not initialized for schedule saving");
        return false;
    }

    if (schedules.empty()) {
        Logger::get().logWarning("No schedules provided to save to database");
        return true;
    }

    try {
        auto& db = DatabaseManager::getInstance();

        if (!db.isConnected()) {
            Logger::get().logError("Database connection lost during schedule saving");
            return false;
        }

        // Validate all schedules have valid semesters
        for (const auto& schedule : schedules) {
            if (!schedule.hasValidSemester()) {
                Logger::get().logError("Schedule " + to_string(schedule.index) +
                                       " has invalid semester: " + to_string(schedule.semester));
                return false;
            }
        }

        if (!db.schedules()->insertSchedules(schedules)) {
            Logger::get().logError("Failed to insert schedules into database");
            return false;
        }

        // Update metadata with semester information
        db.updateMetadata("schedules_saved_at", QDateTime::currentDateTime().toString(Qt::ISODate).toStdString());
        db.updateMetadata("last_saved_schedule_count", std::to_string(schedules.size()));

        // Log semester distribution
        map<int, int> semesterCounts;
        for (const auto& schedule : schedules) {
            semesterCounts[schedule.semester]++;
        }

        string semesterInfo = "Semester distribution: ";
        for (const auto& [semester, count] : semesterCounts) {
            semesterInfo += "S" + to_string(semester) + ":" + to_string(count) + " ";
        }
        db.updateMetadata("last_saved_semester_info", semesterInfo);

        updateLastAccessMetadata();

        Logger::get().logInfo("SUCCESS: " + std::to_string(schedules.size()) + " schedules saved to database");
        Logger::get().logInfo(semesterInfo);

        return true;

    } catch (const exception& e) {
        Logger::get().logError("Exception during schedule saving: " + string(e.what()));
        return false;
    }
}

vector<InformativeSchedule> ModelDatabaseIntegration::getSchedulesFromDatabase() {
    if (!isInitialized()) {
        Logger::get().logError("Database not initialized for schedule retrieval");
        return {};
    }

    try {
        auto& db = DatabaseManager::getInstance();

        if (!db.isConnected()) {
            Logger::get().logError("Database connection lost during schedule retrieval");
            return {};
        }

        vector<InformativeSchedule> schedules = db.schedules()->getAllSchedules();

        updateLastAccessMetadata();

        Logger::get().logInfo("Retrieved " + std::to_string(schedules.size()) + " schedules from database");

        return schedules;

    } catch (const exception& e) {
        Logger::get().logError("Exception during schedule retrieval: " + string(e.what()));
        return {};
    }
}

vector<InformativeSchedule> ModelDatabaseIntegration::getSchedulesBySemester(int semester) {
    if (!isInitialized()) {
        Logger::get().logError("Database not initialized for semester schedule retrieval");
        return {};
    }

    try {
        auto& db = DatabaseManager::getInstance();

        if (!db.isConnected()) {
            Logger::get().logError("Database connection lost during semester schedule retrieval");
            return {};
        }

        vector<InformativeSchedule> schedules = db.schedules()->getSchedulesBySemester(semester);

        updateLastAccessMetadata();

        string semesterName;
        switch (semester) {
            case 1: semesterName = "Semester A"; break;
            case 2: semesterName = "Semester B"; break;
            case 3: semesterName = "Summer"; break;
            case 4: semesterName = "Year-long"; break;
            default: semesterName = "Semester " + to_string(semester); break;
        }

        Logger::get().logInfo("Retrieved " + std::to_string(schedules.size()) +
                              " schedules for " + semesterName);

        return schedules;

    } catch (const exception& e) {
        Logger::get().logError("Exception during semester schedule retrieval: " + string(e.what()));
        return {};
    }
}
