#include "db_schema.h"

// Main schema methods

DatabaseSchema::DatabaseSchema(QSqlDatabase& database) : db(database) {
}

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

// File entity management

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

// Course entity management

bool DatabaseSchema::createCourseTable() {
    const QString query = R"(
        CREATE TABLE IF NOT EXISTS course (
            uniqid TEXT PRIMARY KEY,
            course_id INTEGER NOT NULL,
            raw_id TEXT NOT NULL,
            course_key TEXT NOT NULL,
            file_id INTEGER NOT NULL,
            name TEXT NOT NULL,
            semester INTEGER NOT NULL DEFAULT 1,
            teacher TEXT NOT NULL,
            lectures_json TEXT DEFAULT '[]',
            tutorials_json TEXT DEFAULT '[]',
            labs_json TEXT DEFAULT '[]',
            blocks_json TEXT DEFAULT '[]',
            departmental_sessions_json TEXT DEFAULT '[]',
            reinforcements_json TEXT DEFAULT '[]',
            guidance_json TEXT DEFAULT '[]',
            optional_colloquium_json TEXT DEFAULT '[]',
            registration_json TEXT DEFAULT '[]',
            thesis_json TEXT DEFAULT '[]',
            project_json TEXT DEFAULT '[]',
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (file_id) REFERENCES file(id) ON DELETE CASCADE
        )
    )";

    if (!executeQuery(query)) {
        Logger::get().logError("Failed to create course table");
        return false;
    }

    Logger::get().logInfo("Course table created with enhanced structure and semester support");
    return true;
}

bool DatabaseSchema::createCourseIndexes() {
    bool success = true;

    // Primary key index on uniqid (automatic)
    Logger::get().logInfo("Primary key index on uniqid created automatically");

    // Index on course_key for conflict detection and resolution
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_course_key ON course(course_key)")) {
        Logger::get().logWarning("Failed to create course_key index");
        success = false;
    }

    // Composite index for conflict resolution (course_key + file_id)
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_course_conflict_resolution ON course(course_key, file_id)")) {
        Logger::get().logWarning("Failed to create conflict resolution index");
        success = false;
    }

    // Index on semester for semester-based queries
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_course_semester ON course(semester)")) {
        Logger::get().logWarning("Failed to create course semester index");
        success = false;
    }

    // Index on course_id for academic course number lookups
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_course_id ON course(course_id)")) {
        Logger::get().logWarning("Failed to create course_id index");
        success = false;
    }

    // Index on file_id for file-based operations
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_course_file_id ON course(file_id)")) {
        Logger::get().logWarning("Failed to create course file_id index");
        success = false;
    }

    // Composite index for semester + course_id (common query pattern)
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_course_semester_id ON course(semester, course_id)")) {
        Logger::get().logWarning("Failed to create semester + course_id index");
        success = false;
    }

    if (success) {
        Logger::get().logInfo("Course indexes created successfully with enhanced conflict resolution");
    }
    return success;
}

// Schedule entity management

bool DatabaseSchema::createScheduleTable() {
    const QString query = R"(
        CREATE TABLE IF NOT EXISTS schedule (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            schedule_index INTEGER NOT NULL,
            semester INTEGER NOT NULL DEFAULT 1,
            schedule_name TEXT,
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

    Logger::get().logInfo("Enhanced schedule table with complete metadata fields created successfully");
    return true;
}

bool DatabaseSchema::createScheduleIndexes() {
    bool success = true;

    // Basic indexes
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_index ON schedule(schedule_index)")) {
        Logger::get().logWarning("Failed to create schedule index index");
        success = false;
    }

    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_created_at ON schedule(created_at)")) {
        Logger::get().logWarning("Failed to create schedule created_at index");
        success = false;
    }

    // Semester-based indexes
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_semester ON schedule(semester)")) {
        Logger::get().logWarning("Failed to create schedule semester index");
        success = false;
    }

    // Composite index for semester + common filters
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_semester_filters ON schedule(semester, amount_days, amount_gaps, has_morning_classes)")) {
        Logger::get().logWarning("Failed to create schedule semester filters index");
        success = false;
    }

    // Time-based queries with semester
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_semester_time ON schedule(semester, earliest_start, latest_end)")) {
        Logger::get().logWarning("Failed to create schedule semester time index");
        success = false;
    }

    // Time range queries
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_time_range ON schedule(earliest_start, latest_end)")) {
        Logger::get().logWarning("Failed to create schedule time range index");
        success = false;
    }

    // Time preference queries
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_time_preferences ON schedule(has_morning_classes, has_early_morning, has_evening_classes, has_late_evening)")) {
        Logger::get().logWarning("Failed to create schedule time preferences index");
        success = false;
    }

    // Basic metrics queries
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_basic_metrics ON schedule(amount_days, amount_gaps, gaps_time)")) {
        Logger::get().logWarning("Failed to create schedule basic metrics index");
        success = false;
    }

    // Intensity and efficiency queries
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_intensity ON schedule(max_daily_hours, total_class_time, compactness_ratio)")) {
        Logger::get().logWarning("Failed to create schedule intensity index");
        success = false;
    }

    // Day pattern queries
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_day_patterns ON schedule(consecutive_days, weekday_only, weekend_classes)")) {
        Logger::get().logWarning("Failed to create schedule day patterns index");
        success = false;
    }

    // Weekday queries (commonly used for filtering)
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_weekdays ON schedule(has_monday, has_tuesday, has_wednesday, has_thursday, has_friday)")) {
        Logger::get().logWarning("Failed to create schedule weekdays index");
        success = false;
    }

    // Gap pattern queries
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_gaps ON schedule(has_lunch_break, max_daily_gaps, avg_gap_length)")) {
        Logger::get().logWarning("Failed to create schedule gap patterns index");
        success = false;
    }

    if (success) {
        Logger::get().logInfo("Enhanced schedule indexes with complete metadata support created successfully");
    }
    return success;
}