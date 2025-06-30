#include "db_courses.h"

DatabaseCourseManager::DatabaseCourseManager(QSqlDatabase& database) : db(database) {
}


bool DatabaseCourseManager::insertCourse(const Course& course, int fileId) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for course insertion");
        return false;
    }

    if (fileId <= 0) {
        Logger::get().logError("Cannot insert course without valid file ID");
        return false;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        INSERT OR REPLACE INTO course
        (course_file_id, raw_id, name, teacher, semester, lectures_json, tutorials_json, labs_json, blocks_json, file_id, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP)
    )");

    query.addBindValue(course.id);  // This goes to course_file_id
    query.addBindValue(QString::fromStdString(course.raw_id));
    query.addBindValue(QString::fromStdString(course.name));
    query.addBindValue(QString::fromStdString(course.teacher));
    query.addBindValue(course.semester);  // NEW: Add semester field
    query.addBindValue(QString::fromStdString(DatabaseJsonHelpers::groupsToJson(course.Lectures)));
    query.addBindValue(QString::fromStdString(DatabaseJsonHelpers::groupsToJson(course.Tirgulim)));
    query.addBindValue(QString::fromStdString(DatabaseJsonHelpers::groupsToJson(course.labs)));
    query.addBindValue(QString::fromStdString(DatabaseJsonHelpers::groupsToJson(course.blocks)));
    query.addBindValue(fileId);

    if (!query.exec()) {
        Logger::get().logError("Failed to insert course: " + query.lastError().text().toStdString());
        Logger::get().logError("Course details - File ID: " + std::to_string(course.id) +
                               ", Raw ID: " + course.raw_id + ", Name: " + course.name);
        return false;
    }

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
            Logger::get().logError("Failed to insert course: " + course.name + " (File ID: " + std::to_string(course.id) + ")");
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

vector<Course> DatabaseCourseManager::getAllCourses() {
    vector<Course> courses;
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for course retrieval");
        return courses;
    }

    QSqlQuery query(R"(
        SELECT id, course_file_id, raw_id, name, teacher, semester, lectures_json, tutorials_json, labs_json, blocks_json, file_id
        FROM course ORDER BY course_file_id
    )", db);

    while (query.next()) {
        courses.push_back(createCourseFromQuery(query));
    }

    Logger::get().logInfo("Retrieved " + std::to_string(courses.size()) + " courses from database");
    return courses;
}

Course DatabaseCourseManager::getCourseById(int id) {
    Course course;
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for course retrieval");
        return course;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        SELECT id, course_file_id, raw_id, name, teacher, semester, lectures_json, tutorials_json, labs_json, blocks_json, file_id
        FROM course WHERE id = ?
    )");
    query.addBindValue(id);

    if (query.exec() && query.next()) {
        course = createCourseFromQuery(query);
    } else {
        Logger::get().logWarning("No course found with ID: " + std::to_string(id));
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
    query.prepare(R"(
        SELECT id, course_file_id, raw_id, name, teacher, semester, lectures_json, tutorials_json, labs_json, blocks_json, file_id
        FROM course WHERE file_id = ? ORDER BY course_file_id
    )");
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

    if (!db.isOpen() || fileIds.empty()) {
        Logger::get().logError("Database not open or no file IDs provided");
        return {};
    }

    map<string, vector<CourseConflictInfo>> conflictMap;

    for (int fileId : fileIds) {
        QSqlQuery query(db);
        query.prepare(R"(
            SELECT c.id, c.course_file_id, c.raw_id, c.name, c.teacher, c.semester, c.lectures_json, c.tutorials_json,
                   c.labs_json, c.blocks_json, c.file_id, f.file_name, f.upload_time
            FROM course c
            JOIN file f ON c.file_id = f.id
            WHERE c.file_id = ?
            ORDER BY f.upload_time ASC
        )");
        query.addBindValue(fileId);

        if (!query.exec()) {
            Logger::get().logError("Failed to execute query for file ID " + std::to_string(fileId) +
                                   ": " + query.lastError().text().toStdString());
            continue;
        }

        int courseCount = 0;
        while (query.next()) {
            Course course = createCourseFromQuery(query);
            string fileName = query.value(11).toString().toStdString();  // Adjusted index
            QDateTime uploadTime = query.value(12).toDateTime();         // Adjusted index

            CourseConflictInfo conflictInfo;
            conflictInfo.course = course;
            conflictInfo.uploadTime = uploadTime;
            conflictInfo.fileName = fileName;
            conflictInfo.fileId = fileId;

            conflictMap[course.raw_id].push_back(conflictInfo);
            courseCount++;
        }

        Logger::get().logInfo("File ID " + std::to_string(fileId) + " contributed " + std::to_string(courseCount) + " courses");
    }

    Logger::get().logInfo("Total unique course raw_ids found: " + std::to_string(conflictMap.size()));

    return resolveConflicts(conflictMap, warnings);
}

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

Course DatabaseCourseManager::createCourseFromQuery(QSqlQuery& query) {
    Course course;
    course.id = query.value(1).toInt();  // course_file_id
    course.raw_id = query.value(2).toString().toStdString();
    course.name = query.value(3).toString().toStdString();
    course.teacher = query.value(4).toString().toStdString();
    course.semester = query.value(5).toInt();  // NEW: Read semester field
    course.Lectures = DatabaseJsonHelpers::groupsFromJson(query.value(6).toString().toStdString());
    course.Tirgulim = DatabaseJsonHelpers::groupsFromJson(query.value(7).toString().toStdString());
    course.labs = DatabaseJsonHelpers::groupsFromJson(query.value(8).toString().toStdString());
    course.blocks = DatabaseJsonHelpers::groupsFromJson(query.value(9).toString().toStdString());

    return course;
}

vector<Course> DatabaseCourseManager::resolveConflicts(const map<string, vector<CourseConflictInfo>>& conflictMap,
                                                       vector<string>& warnings) {
    vector<Course> courses;

    for (const auto& pair : conflictMap) {
        const string& rawId = pair.first;
        const vector<CourseConflictInfo>& conflicts = pair.second;

        if (conflicts.size() == 1) {
            // No conflict, just add the course
            courses.push_back(conflicts[0].course);
        } else {
            // Multiple courses with same raw_id, resolve by upload time
            auto latest = std::max_element(conflicts.begin(), conflicts.end(),
                                           [](const CourseConflictInfo& a, const CourseConflictInfo& b) {
                                               return a.uploadTime < b.uploadTime;
                                           });

            courses.push_back(latest->course);

            // Generate warning message
            string warningMsg = "Course conflict resolved for " + rawId + " - using version from " + latest->fileName + " (latest upload)";
            warnings.push_back(warningMsg);

            Logger::get().logWarning(warningMsg);
            Logger::get().logInfo("Conflict details for " + rawId + ":");
        }
    }

    Logger::get().logInfo("Resolved " + std::to_string(courses.size()) + " courses with " +
                          std::to_string(warnings.size()) + " conflict(s)");

    return courses;
}