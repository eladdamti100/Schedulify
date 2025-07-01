#include "db_courses.h"

DatabaseCourseManager::DatabaseCourseManager(QSqlDatabase& database) : db(database) {
}

// CRUD Operations

bool DatabaseCourseManager::insertCourse(const Course& course, int fileId) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for course insertion");
        return false;
    }

    if (fileId <= 0) {
        Logger::get().logError("Cannot insert course without valid file ID");
        return false;
    }

    if (!course.hasValidSemester()) {
        Logger::get().logError("Cannot insert course with invalid semester: " + std::to_string(course.semester));
        return false;
    }

    // Create CourseEntity from Course
    CourseEntity entity(course, fileId);

    QSqlQuery query(db);
    query.prepare(R"(
        INSERT OR REPLACE INTO course
        (uniqid, course_id, raw_id, course_key, file_id, name, semester, teacher,
         lectures_json, tutorials_json, labs_json, blocks_json,
         departmental_sessions_json, reinforcements_json, guidance_json,
         optional_colloquium_json, registration_json, thesis_json, project_json,
         created_at, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP)
    )");

    // Bind basic fields
    query.addBindValue(QString::fromStdString(entity.uniqid));
    query.addBindValue(entity.course_id);
    query.addBindValue(QString::fromStdString(entity.raw_id));
    query.addBindValue(QString::fromStdString(entity.course_key));
    query.addBindValue(entity.file_id);
    query.addBindValue(QString::fromStdString(entity.name));
    query.addBindValue(entity.semester);
    query.addBindValue(QString::fromStdString(entity.teacher));

    // Serialize and bind all group types
    query.addBindValue(QString::fromStdString(DatabaseJsonHelpers::groupsToJson(course.Lectures)));
    query.addBindValue(QString::fromStdString(DatabaseJsonHelpers::groupsToJson(course.Tirgulim)));
    query.addBindValue(QString::fromStdString(DatabaseJsonHelpers::groupsToJson(course.labs)));
    query.addBindValue(QString::fromStdString(DatabaseJsonHelpers::groupsToJson(course.blocks)));
    query.addBindValue(QString::fromStdString(DatabaseJsonHelpers::groupsToJson(course.DepartmentalSessions)));
    query.addBindValue(QString::fromStdString(DatabaseJsonHelpers::groupsToJson(course.Reinforcements)));
    query.addBindValue(QString::fromStdString(DatabaseJsonHelpers::groupsToJson(course.Guidance)));
    query.addBindValue(QString::fromStdString(DatabaseJsonHelpers::groupsToJson(course.OptionalColloquium)));
    query.addBindValue(QString::fromStdString(DatabaseJsonHelpers::groupsToJson(course.Registration)));
    query.addBindValue(QString::fromStdString(DatabaseJsonHelpers::groupsToJson(course.Thesis)));
    query.addBindValue(QString::fromStdString(DatabaseJsonHelpers::groupsToJson(course.Project)));

    if (!query.exec()) {
        Logger::get().logError("Failed to insert course: " + query.lastError().text().toStdString());
        return false;
    }

    Logger::get().logInfo("Successfully inserted course: " + entity.uniqid + " (" + entity.name + ")");
    return true;
}

bool DatabaseCourseManager::insertCourses(const vector<Course>& courses, int fileId) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for course insertion");
        return false;
    }

    if (fileId <= 0) {
        Logger::get().logError("Cannot insert courses without valid file ID");
        return false;
    }

    if (courses.empty()) {
        Logger::get().logWarning("No courses to insert");
        return true;
    }

    // Start transaction
    if (!db.transaction()) {
        Logger::get().logError("Failed to begin transaction for course insertion");
        return false;
    }

    int successCount = 0;
    for (const auto& course : courses) {
        if (insertCourse(course, fileId)) {
            successCount++;
        } else {
            Logger::get().logError("Failed to insert course: " + course.name + " (ID: " + std::to_string(course.id) + ", Semester: " + std::to_string(course.semester) + ")");
        }
    }

    if (successCount == 0) {
        Logger::get().logError("Failed to insert any courses, rolling back transaction");
        db.rollback();
        return false;
    }

    if (!db.commit()) {
        Logger::get().logError("Failed to commit course insertion transaction");
        db.rollback();
        return false;
    }

    Logger::get().logInfo("Successfully inserted " + std::to_string(successCount) + "/" +
                          std::to_string(courses.size()) + " courses with file ID: " + std::to_string(fileId));

    return successCount == static_cast<int>(courses.size());
}

bool DatabaseCourseManager::deleteAllCourses() {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for course deletion");
        return false;
    }

    QSqlQuery query("DELETE FROM course", db);
    if (!query.exec()) {
        Logger::get().logError("Failed to delete all courses: " + query.lastError().text().toStdString());
        return false;
    }

    int rowsAffected = query.numRowsAffected();
    Logger::get().logInfo("Deleted all courses from database (" + std::to_string(rowsAffected) + " courses)");
    return true;
}

bool DatabaseCourseManager::deleteCoursesByFileId(int fileId) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for course deletion");
        return false;
    }

    QSqlQuery query(db);
    query.prepare("DELETE FROM course WHERE file_id = ?");
    query.addBindValue(fileId);

    if (!query.exec()) {
        Logger::get().logError("Failed to delete courses for file ID " + std::to_string(fileId) +
                               ": " + query.lastError().text().toStdString());
        return false;
    }

    int deletedCount = query.numRowsAffected();
    Logger::get().logInfo("Deleted " + std::to_string(deletedCount) + " courses for file ID: " + std::to_string(fileId));
    return true;
}

bool DatabaseCourseManager::deleteCourseByUniqid(const string& uniqid) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for course deletion");
        return false;
    }

    QSqlQuery query(db);
    query.prepare("DELETE FROM course WHERE uniqid = ?");
    query.addBindValue(QString::fromStdString(uniqid));

    if (!query.exec()) {
        Logger::get().logError("Failed to delete course with uniqid " + uniqid + ": " + query.lastError().text().toStdString());
        return false;
    }

    int deletedCount = query.numRowsAffected();
    if (deletedCount > 0) {
        Logger::get().logInfo("Deleted course: " + uniqid);
        return true;
    } else {
        Logger::get().logWarning("No course found with uniqid: " + uniqid);
        return false;
    }
}

// Retrieval Operations

vector<Course> DatabaseCourseManager::getAllCourses() {
    vector<Course> courses;
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for course retrieval");
        return courses;
    }

    QSqlQuery query(buildCourseSelectQuery("ORDER BY semester, course_id"), db);

    while (query.next()) {
        courses.push_back(createCourseFromQuery(query));
    }

    Logger::get().logInfo("Retrieved " + std::to_string(courses.size()) + " courses from database");
    return courses;
}

Course DatabaseCourseManager::getCourseByUniqid(const string& uniqid) {
    Course course;
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for course retrieval");
        return course;
    }

    QSqlQuery query(db);
    query.prepare(buildCourseSelectQuery("WHERE uniqid = ?"));
    query.addBindValue(QString::fromStdString(uniqid));

    if (query.exec() && query.next()) {
        course = createCourseFromQuery(query);
        Logger::get().logInfo("Retrieved course: " + uniqid);
    } else {
        Logger::get().logWarning("No course found with uniqid: " + uniqid);
    }

    return course;
}

vector<Course> DatabaseCourseManager::getCoursesByFileId(int fileId) {
    vector<Course> courses;
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for course retrieval");
        return courses;
    }

    QSqlQuery query(db);
    query.prepare(buildCourseSelectQuery("WHERE file_id = ? ORDER BY semester, course_id"));
    query.addBindValue(fileId);

    int courseCount = 0;
    while (query.next()) {
        courses.push_back(createCourseFromQuery(query));
        courseCount++;
    }

    Logger::get().logInfo("Found " + std::to_string(courseCount) + " courses for file ID: " + std::to_string(fileId));
    return courses;
}

vector<Course> DatabaseCourseManager::getCoursesByFileIds(const vector<int>& fileIds, vector<string>& warnings) {
    warnings.clear();
    vector<Course> resultCourses;

    if (!db.isOpen() || fileIds.empty()) {
        Logger::get().logError("Database not open or no file IDs provided");
        return resultCourses;
    }

    // Step 1: Get all courses from selected files
    map<string, vector<CourseConflictInfo>> conflictMap = buildConflictMap(fileIds);

    // Step 2: Resolve conflicts using course_key
    resultCourses = resolveConflictsByLatest(conflictMap, warnings);

    Logger::get().logInfo("Conflict resolution complete: " + std::to_string(resultCourses.size()) +
                          " unique courses selected with " + std::to_string(warnings.size()) + " conflicts resolved");

    return resultCourses;
}

vector<Course> DatabaseCourseManager::getCoursesBySemester(int semester) {
    vector<Course> courses;
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for semester course retrieval");
        return courses;
    }

    QSqlQuery query(db);
    query.prepare(buildCourseSelectQuery("WHERE semester = ? ORDER BY course_id"));
    query.addBindValue(semester);

    while (query.next()) {
        courses.push_back(createCourseFromQuery(query));
    }

    Logger::get().logInfo("Retrieved " + std::to_string(courses.size()) + " courses for semester " + std::to_string(semester));
    return courses;
}

vector<Course> DatabaseCourseManager::getCoursesByCourseId(int courseId) {
    vector<Course> courses;
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for course ID retrieval");
        return courses;
    }

    QSqlQuery query(db);
    query.prepare(buildCourseSelectQuery("WHERE course_id = ? ORDER BY semester"));
    query.addBindValue(courseId);

    while (query.next()) {
        courses.push_back(createCourseFromQuery(query));
    }

    Logger::get().logInfo("Retrieved " + std::to_string(courses.size()) + " semester variants for course ID " + std::to_string(courseId));
    return courses;
}

vector<Course> DatabaseCourseManager::getCoursesByCourseKey(const string& courseKey) {
    vector<Course> courses;
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for course key retrieval");
        return courses;
    }

    QSqlQuery query(db);
    query.prepare(buildCourseSelectQuery("WHERE course_key = ? ORDER BY file_id"));
    query.addBindValue(QString::fromStdString(courseKey));

    while (query.next()) {
        courses.push_back(createCourseFromQuery(query));
    }

    Logger::get().logInfo("Retrieved " + std::to_string(courses.size()) + " courses for course key: " + courseKey);
    return courses;
}

vector<Course> DatabaseCourseManager::getCoursesByFileIdAndSemester(int fileId, int semester) {
    vector<Course> courses;
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for file+semester course retrieval");
        return courses;
    }

    QSqlQuery query(db);
    query.prepare(buildCourseSelectQuery("WHERE file_id = ? AND semester = ? ORDER BY course_id"));
    query.addBindValue(fileId);
    query.addBindValue(semester);

    while (query.next()) {
        courses.push_back(createCourseFromQuery(query));
    }

    Logger::get().logInfo("Retrieved " + std::to_string(courses.size()) + " courses for file " + std::to_string(fileId) + ", semester " + std::to_string(semester));
    return courses;
}

// Utility Operations

int DatabaseCourseManager::getCourseCount() {
    if (!db.isOpen()) {
        return -1;
    }

    QSqlQuery query("SELECT COUNT(*) FROM course", db);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return -1;
}

int DatabaseCourseManager::getCourseCountByFileId(int fileId) {
    if (!db.isOpen()) {
        return -1;
    }

    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM course WHERE file_id = ?");
    query.addBindValue(fileId);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return -1;
}

int DatabaseCourseManager::getCourseCountBySemester(int semester) {
    if (!db.isOpen()) {
        return -1;
    }

    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM course WHERE semester = ?");
    query.addBindValue(semester);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return -1;
}

bool DatabaseCourseManager::courseExists(const string& uniqid) {
    if (!db.isOpen()) {
        return false;
    }

    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM course WHERE uniqid = ?");
    query.addBindValue(QString::fromStdString(uniqid));

    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
}

bool DatabaseCourseManager::courseKeyExists(const string& courseKey) {
    if (!db.isOpen()) {
        return false;
    }

    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM course WHERE course_key = ?");
    query.addBindValue(QString::fromStdString(courseKey));

    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
}

// Conflict Resolution

vector<string> DatabaseCourseManager::detectConflicts(const vector<int>& fileIds) {
    vector<string> conflicts;
    if (!db.isOpen() || fileIds.empty()) {
        return conflicts;
    }

    // Build IN clause for file IDs
    QString inClause = "(";
    for (size_t i = 0; i < fileIds.size(); ++i) {
        if (i > 0) inClause += ",";
        inClause += "?";
    }
    inClause += ")";

    QSqlQuery query(db);
    query.prepare(QString(R"(
        SELECT course_key, COUNT(*) as conflict_count
        FROM course
        WHERE file_id IN %1
        GROUP BY course_key
        HAVING COUNT(*) > 1
        ORDER BY course_key
    )").arg(inClause));

    for (int fileId : fileIds) {
        query.addBindValue(fileId);
    }

    if (query.exec()) {
        while (query.next()) {
            string courseKey = query.value(0).toString().toStdString();
            int conflictCount = query.value(1).toInt();
            conflicts.push_back(courseKey + " (found in " + std::to_string(conflictCount) + " files)");
        }
    }

    return conflicts;
}

map<string, vector<Course>> DatabaseCourseManager::groupCoursesByKey(const vector<Course>& courses) {
    map<string, vector<Course>> grouped;
    for (const Course& course : courses) {
        grouped[course.course_key].push_back(course);
    }
    return grouped;
}

Course DatabaseCourseManager::selectLatestCourse(const vector<Course>& conflictingCourses) {
    if (conflictingCourses.empty()) {
        return Course();
    }

    if (conflictingCourses.size() == 1) {
        return conflictingCourses[0];
    }

    // Find course from file with latest upload time
    Course latest = conflictingCourses[0];
    QDateTime latestTime;

    for (const Course& course : conflictingCourses) {
        // Get file upload time
        QSqlQuery query(db);
        query.prepare("SELECT upload_time FROM file WHERE id = (SELECT file_id FROM course WHERE uniqid = ?)");
        query.addBindValue(QString::fromStdString(course.uniqid));

        if (query.exec() && query.next()) {
            QDateTime uploadTime = query.value(0).toDateTime();
            if (!latestTime.isValid() || uploadTime > latestTime) {
                latest = course;
                latestTime = uploadTime;
            }
        }
    }

    return latest;
}

vector<string> DatabaseCourseManager::getDistinctCourseKeys() {
    vector<string> keys;
    if (!db.isOpen()) {
        return keys;
    }

    QSqlQuery query("SELECT DISTINCT course_key FROM course ORDER BY course_key", db);
    while (query.next()) {
        keys.push_back(query.value(0).toString().toStdString());
    }

    return keys;
}

vector<int> DatabaseCourseManager::getDistinctSemesters() {
    vector<int> semesters;
    if (!db.isOpen()) {
        return semesters;
    }

    QSqlQuery query("SELECT DISTINCT semester FROM course ORDER BY semester", db);
    while (query.next()) {
        semesters.push_back(query.value(0).toInt());
    }

    return semesters;
}

// Private Helper Methods

Course DatabaseCourseManager::createCourseFromQuery(QSqlQuery& query) {
    Course course;

    // Basic fields
    course.uniqid = query.value("uniqid").toString().toStdString();
    course.id = query.value("course_id").toInt();
    course.raw_id = query.value("raw_id").toString().toStdString();
    course.course_key = query.value("course_key").toString().toStdString();
    course.name = query.value("name").toString().toStdString();
    course.semester = query.value("semester").toInt();
    course.teacher = query.value("teacher").toString().toStdString();

    // Deserialize all group types
    course.Lectures = DatabaseJsonHelpers::groupsFromJson(query.value("lectures_json").toString().toStdString());
    course.Tirgulim = DatabaseJsonHelpers::groupsFromJson(query.value("tutorials_json").toString().toStdString());
    course.labs = DatabaseJsonHelpers::groupsFromJson(query.value("labs_json").toString().toStdString());
    course.blocks = DatabaseJsonHelpers::groupsFromJson(query.value("blocks_json").toString().toStdString());
    course.DepartmentalSessions = DatabaseJsonHelpers::groupsFromJson(query.value("departmental_sessions_json").toString().toStdString());
    course.Reinforcements = DatabaseJsonHelpers::groupsFromJson(query.value("reinforcements_json").toString().toStdString());
    course.Guidance = DatabaseJsonHelpers::groupsFromJson(query.value("guidance_json").toString().toStdString());
    course.OptionalColloquium = DatabaseJsonHelpers::groupsFromJson(query.value("optional_colloquium_json").toString().toStdString());
    course.Registration = DatabaseJsonHelpers::groupsFromJson(query.value("registration_json").toString().toStdString());
    course.Thesis = DatabaseJsonHelpers::groupsFromJson(query.value("thesis_json").toString().toStdString());
    course.Project = DatabaseJsonHelpers::groupsFromJson(query.value("project_json").toString().toStdString());

    return course;
}

map<string, vector<DatabaseCourseManager::CourseConflictInfo>> DatabaseCourseManager::buildConflictMap(const vector<int>& fileIds) {
    map<string, vector<CourseConflictInfo>> conflictMap;

    for (int fileId : fileIds) {
        // Get file upload time
        QSqlQuery fileQuery(db);
        fileQuery.prepare("SELECT file_name, upload_time FROM file WHERE id = ?");
        fileQuery.addBindValue(fileId);

        string fileName = "Unknown";
        QDateTime uploadTime = QDateTime::currentDateTime();

        if (fileQuery.exec() && fileQuery.next()) {
            fileName = fileQuery.value(0).toString().toStdString();
            uploadTime = fileQuery.value(1).toDateTime();
        }

        // Get courses from this file
        QSqlQuery courseQuery(db);
        courseQuery.prepare(buildCourseSelectQuery("WHERE file_id = ?"));
        courseQuery.addBindValue(fileId);

        if (courseQuery.exec()) {
            while (courseQuery.next()) {
                Course course = createCourseFromQuery(courseQuery);
                CourseConflictInfo info(course, uploadTime, fileName, fileId);
                conflictMap[course.course_key].push_back(info);
            }
        }
    }

    return conflictMap;
}

vector<Course> DatabaseCourseManager::resolveConflictsByLatest(const map<string, vector<CourseConflictInfo>>& conflictMap, vector<string>& warnings) {
    vector<Course> resultCourses;

    for (const auto& [courseKey, conflicts] : conflictMap) {
        if (conflicts.size() == 1) {
            // No conflict - just add the course
            resultCourses.push_back(conflicts[0].course);
            Logger::get().logInfo("Course " + courseKey + " - no conflict (single source)");
        } else {
            // Multiple versions found - resolve by file upload time
            const CourseConflictInfo* mostRecent = nullptr;
            QDateTime mostRecentTime;

            for (const CourseConflictInfo& conflict : conflicts) {
                if (!mostRecent || conflict.uploadTime > mostRecentTime) {
                    mostRecent = &conflict;
                    mostRecentTime = conflict.uploadTime;
                }
            }

            if (mostRecent) {
                resultCourses.push_back(mostRecent->course);

                // Generate detailed warning
                string warningMsg = "CONFLICT RESOLVED for course " + courseKey + " (" +
                                    mostRecent->course.name + "):\n";
                warningMsg += "  Found in " + std::to_string(conflicts.size()) + " files: ";
                for (size_t i = 0; i < conflicts.size(); ++i) {
                    if (i > 0) warningMsg += ", ";
                    warningMsg += conflicts[i].fileName + " (" + conflicts[i].uploadTime.toString().toStdString() + ")";
                }
                warningMsg += "\n  Using most recent version from " + mostRecentTime.toString().toStdString();

                warnings.push_back(warningMsg);
                Logger::get().logWarning(warningMsg);
            }
        }
    }

    return resultCourses;
}

QString DatabaseCourseManager::buildCourseSelectQuery(const string& whereClause) const {
    QString query = R"(
        SELECT uniqid, course_id, raw_id, course_key, file_id, name, semester, teacher,
               lectures_json, tutorials_json, labs_json, blocks_json,
               departmental_sessions_json, reinforcements_json, guidance_json,
               optional_colloquium_json, registration_json, thesis_json, project_json,
               created_at, updated_at
        FROM course
    )";

    if (!whereClause.empty()) {
        query += " " + QString::fromStdString(whereClause);
    }

    return query;
}

QString DatabaseCourseManager::buildCourseCountQuery(const string& whereClause) const {
    QString query = "SELECT COUNT(*) FROM course";

    if (!whereClause.empty()) {
        query += " " + QString::fromStdString(whereClause);
    }

    return query;
}