#include "db_schema.h"

DatabaseSchema::DatabaseSchema(QSqlDatabase& database) : db(database) {
}

// Main db methods

bool DatabaseSchema::createTables() {
    return createMetadataTable() &&
           createFileTable() &&
           createCourseTable() &&
           createScheduleTable();
}

bool DatabaseSchema::createIndexes() {
    return createMetadataIndexes() &&
           createFileIndexes() &&
           createCourseIndexes() &&
           createScheduleIndexes();
}

bool DatabaseSchema::createMetadataTable() {
    const QString query = R"(
        CREATE TABLE IF NOT EXISTS metadata (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            key TEXT UNIQUE NOT NULL,
            value TEXT NOT NULL,
            description TEXT,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";

    if (!executeQuery(query)) {
        Logger::get().logError("Failed to create metadata table");
        return false;
    }

    Logger::get().logInfo("Metadata table created successfully");
    return true;
}

bool DatabaseSchema::createMetadataIndexes() {
    Logger::get().logInfo("Creating metadata indexes...");

    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_metadata_key ON metadata(key)")) {
        Logger::get().logWarning("Failed to create metadata key index");
        return false;
    }

    Logger::get().logInfo("Metadata indexes created successfully");
    return true;
}

bool DatabaseSchema::tableExists(const QString& tableName) {
    QSqlQuery query("SELECT name FROM sqlite_master WHERE type='table' AND name=?", db);
    query.addBindValue(tableName);

    if (query.exec() && query.next()) {
        return true;
    }

    return false;
}

bool DatabaseSchema::executeQuery(const QString& query) {
    QSqlQuery sqlQuery(db);
    if (!sqlQuery.exec(query)) {
        Logger::get().logError("Failed to execute query: " + sqlQuery.lastError().text().toStdString());
        return false;
    }
    return true;
}

// File management

bool DatabaseSchema::createFileTable() {
    const QString query = R"(
        CREATE TABLE IF NOT EXISTS file (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_name TEXT NOT NULL,
            file_type TEXT NOT NULL,
            upload_time DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";

    if (!executeQuery(query)) {
        Logger::get().logError("Failed to create file table");
        return false;
    }

    Logger::get().logInfo("File table created successfully");
    return true;
}

bool DatabaseSchema::createFileIndexes() {
    bool success = true;

    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_file_name ON file(file_name)")) {
        Logger::get().logWarning("Failed to create file name index");
        success = false;
    }

    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_file_type ON file(file_type)")) {
        Logger::get().logWarning("Failed to create file type index");
        success = false;
    }

    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_file_upload_time ON file(upload_time)")) {
        Logger::get().logWarning("Failed to create file upload_time index");
        success = false;
    }

    if (success) {
        Logger::get().logInfo("File indexes created successfully");
    }
    return success;
}

// Course management

bool DatabaseSchema::createCourseTable() {
    const QString query = R"(
        CREATE TABLE IF NOT EXISTS course (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            course_file_id INTEGER NOT NULL,
            raw_id TEXT NOT NULL,
            name TEXT NOT NULL,
            teacher TEXT NOT NULL,
            semester INTEGER NOT NULL DEFAULT 1,
            lectures_json TEXT DEFAULT '[]',
            tutorials_json TEXT DEFAULT '[]',
            labs_json TEXT DEFAULT '[]',
            blocks_json TEXT DEFAULT '[]',
            file_id INTEGER NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (file_id) REFERENCES file(id) ON DELETE CASCADE,
            UNIQUE(course_file_id, semester, file_id)
        )
    )";

    if (!executeQuery(query)) {
        Logger::get().logError("Failed to create course table");
        return false;
    }

    Logger::get().logInfo("Course table created successfully");
    return true;
}

bool DatabaseSchema::createCourseIndexes() {
    bool success = true;

    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_course_raw_id ON course(raw_id)")) {
        Logger::get().logWarning("Failed to create course raw_id index");
        success = false;
    }

    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_course_name ON course(name)")) {
        Logger::get().logWarning("Failed to create course name index");
        success = false;
    }

    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_course_file_id ON course(file_id)")) {
        Logger::get().logWarning("Failed to create course file_id index");
        success = false;
    }

    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_course_course_file_id ON course(course_file_id)")) {
        Logger::get().logWarning("Failed to create course course_file_id index");
        success = false;
    }

    // Updated composite index to include semester
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_course_composite ON course(course_file_id, semester, file_id)")) {
        Logger::get().logWarning("Failed to create course composite index");
        success = false;
    }

    // Add semester index for efficient semester-based queries
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_course_semester ON course(semester)")) {
        Logger::get().logWarning("Failed to create course semester index");
        success = false;
    }

    // Add composite index for semester + course_file_id queries
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_course_semester_file_id ON course(semester, course_file_id)")) {
        Logger::get().logWarning("Failed to create course semester+file_id index");
        success = false;
    }

    // NEW: Add index for unique constraint lookup
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_course_unique_lookup ON course(raw_id, semester)")) {
        Logger::get().logWarning("Failed to create course unique lookup index");
        success = false;
    }

    if (success) {
        Logger::get().logInfo("Course indexes created successfully");
    }
    return success;
}

// Schedule management

bool DatabaseSchema::createScheduleTable() {
    const QString query = R"(
        CREATE TABLE IF NOT EXISTS schedule (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            schedule_index INTEGER NOT NULL,
            unique_id TEXT NOT NULL UNIQUE,
            semester TEXT NOT NULL DEFAULT 'A',
            schedule_data_json TEXT NOT NULL,
            amount_days INTEGER NOT NULL,
            amount_gaps INTEGER NOT NULL,
            gaps_time INTEGER NOT NULL,
            avg_start INTEGER NOT NULL,
            avg_end INTEGER NOT NULL,
            earliest_start INTEGER NOT NULL,
            latest_end INTEGER NOT NULL,
            longest_gap INTEGER NOT NULL,
            total_class_time INTEGER NOT NULL,
            consecutive_days INTEGER NOT NULL,
            days_json TEXT NOT NULL,
            weekend_classes BOOLEAN NOT NULL,
            has_morning_classes BOOLEAN NOT NULL,
            has_early_morning BOOLEAN NOT NULL,
            has_evening_classes BOOLEAN NOT NULL,
            has_late_evening BOOLEAN NOT NULL,
            max_daily_hours INTEGER NOT NULL,
            min_daily_hours INTEGER NOT NULL,
            avg_daily_hours INTEGER NOT NULL,
            has_lunch_break BOOLEAN NOT NULL,
            max_daily_gaps INTEGER NOT NULL,
            avg_gap_length INTEGER NOT NULL,
            schedule_span INTEGER NOT NULL,
            compactness_ratio REAL NOT NULL,
            weekday_only BOOLEAN NOT NULL,
            has_monday BOOLEAN NOT NULL,
            has_tuesday BOOLEAN NOT NULL,
            has_wednesday BOOLEAN NOT NULL,
            has_thursday BOOLEAN NOT NULL,
            has_friday BOOLEAN NOT NULL,
            has_saturday BOOLEAN NOT NULL,
            has_sunday BOOLEAN NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";

    if (!executeQuery(query)) {
        Logger::get().logError("Failed to create schedule table");
        return false;
    }

    return true;
}

bool DatabaseSchema::createScheduleIndexes() {
    bool success = true;

    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_index ON schedule(schedule_index)")) {
        Logger::get().logWarning("Failed to create schedule index index");
        success = false;
    }

    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_semester ON schedule(semester)")) {
        Logger::get().logWarning("Failed to create schedule semester index");
        success = false;
    }

    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_semester_index ON schedule(semester, schedule_index)")) {
        Logger::get().logWarning("Failed to create schedule semester+index compound index");
        success = false;
    }

    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_index ON schedule(schedule_index)")) {
        Logger::get().logWarning("Failed to create schedule index index");
        success = false;
    }

    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_unique_id ON schedule(unique_id)")) {
        Logger::get().logWarning("Failed to create schedule unique_id index");
        success = false;
    }

    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_semester ON schedule(semester)")) {
        Logger::get().logWarning("Failed to create schedule semester index");
        success = false;
    }

    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_semester_unique ON schedule(semester, unique_id)")) {
        Logger::get().logWarning("Failed to create schedule semester+unique_id compound index");
        success = false;
    }

    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_index ON schedule(schedule_index)")) {
        Logger::get().logWarning("Failed to create schedule index index");
        success = false;
    }

    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_created_at ON schedule(created_at)")) {
        Logger::get().logWarning("Failed to create schedule created_at index");
        success = false;
    }

    // Time-based queries
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_time_range ON schedule(earliest_start, latest_end)")) {
        Logger::get().logWarning("Failed to create schedule time range index");
        success = false;
    }

    // Morning/evening preferences
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_time_preferences ON schedule(has_morning_classes, has_early_morning, has_evening_classes, has_late_evening)")) {
        Logger::get().logWarning("Failed to create schedule time preferences index");
        success = false;
    }

    // Basic metrics (most common queries)
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_basic_metrics ON schedule(amount_days, amount_gaps, gaps_time)")) {
        Logger::get().logWarning("Failed to create schedule basic metrics index");
        success = false;
    }

    // Intensity and compactness
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_intensity ON schedule(max_daily_hours, total_class_time, compactness_ratio)")) {
        Logger::get().logWarning("Failed to create schedule intensity index");
        success = false;
    }

    // Day patterns
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_day_patterns ON schedule(consecutive_days, weekday_only, weekend_classes)")) {
        Logger::get().logWarning("Failed to create schedule day patterns index");
        success = false;
    }

    // Individual weekdays for specific day queries
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_weekdays ON schedule(has_monday, has_tuesday, has_wednesday, has_thursday, has_friday)")) {
        Logger::get().logWarning("Failed to create schedule weekdays index");
        success = false;
    }

    // Gap patterns
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_gaps ON schedule(longest_gap, avg_gap_length, has_lunch_break, max_daily_gaps)")) {
        Logger::get().logWarning("Failed to create schedule gaps index");
        success = false;
    }

    // Composite index for the most common "ideal schedule" queries
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_ideal_combo ON schedule(amount_days, amount_gaps, has_morning_classes, has_evening_classes, weekday_only)")) {
        Logger::get().logWarning("Failed to create schedule ideal combo index");
        success = false;
    }

    return success;
}