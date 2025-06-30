#include "db_schema.h"

DatabaseSchema::DatabaseSchema(QSqlDatabase& database) : db(database) {
}

bool DatabaseSchema::createTables() {
    return createMetadataTable() &&
           createFileTable() &&
           createCourseTable() &&
           createScheduleSetTable() &&
           createScheduleTable();
}

bool DatabaseSchema::createIndexes() {
    return createMetadataIndexes() &&
           createFileIndexes() &&
           createCourseIndexes() &&
           createScheduleSetIndexes() &&
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

bool DatabaseSchema::createCourseTable() {
    const QString query = R"(
        CREATE TABLE IF NOT EXISTS course (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            course_file_id INTEGER NOT NULL,
            raw_id TEXT NOT NULL,
            name TEXT NOT NULL,
            teacher TEXT NOT NULL,
            lectures_json TEXT DEFAULT '[]',
            tutorials_json TEXT DEFAULT '[]',
            labs_json TEXT DEFAULT '[]',
            blocks_json TEXT DEFAULT '[]',
            file_id INTEGER NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (file_id) REFERENCES file(id) ON DELETE CASCADE,
            UNIQUE(course_file_id, file_id)
        )
    )";

    if (!executeQuery(query)) {
        Logger::get().logError("Failed to create course table");
        return false;
    }

    Logger::get().logInfo("Course table created successfully");
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

    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_course_composite ON course(course_file_id, file_id)")) {
        Logger::get().logWarning("Failed to create course composite index");
        success = false;
    }

    if (success) {
        Logger::get().logInfo("Course indexes created successfully");
    }
    return success;
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

bool DatabaseSchema::createScheduleSetTable() {
    const QString query = R"(
        CREATE TABLE IF NOT EXISTS schedule_set (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            set_name TEXT NOT NULL,
            source_file_ids_json TEXT DEFAULT '[]',
            schedule_count INTEGER DEFAULT 0,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";

    if (!executeQuery(query)) {
        Logger::get().logError("Failed to create schedule_set table");
        return false;
    }

    Logger::get().logInfo("Schedule set table created successfully");
    return true;
}

bool DatabaseSchema::createScheduleTable() {
    const QString query = R"(
        CREATE TABLE IF NOT EXISTS schedule (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            schedule_set_id INTEGER NOT NULL,
            schedule_index INTEGER NOT NULL,
            schedule_name TEXT DEFAULT '',
            schedule_data_json TEXT NOT NULL,

            -- Basic metrics (original)
            amount_days INTEGER DEFAULT 0,
            amount_gaps INTEGER DEFAULT 0,
            gaps_time INTEGER DEFAULT 0,
            avg_start INTEGER DEFAULT 0,
            avg_end INTEGER DEFAULT 0,

            -- Enhanced time metrics
            earliest_start INTEGER DEFAULT 0,
            latest_end INTEGER DEFAULT 0,
            longest_gap INTEGER DEFAULT 0,
            total_class_time INTEGER DEFAULT 0,

            -- Day pattern metrics
            consecutive_days INTEGER DEFAULT 0,
            days_json TEXT DEFAULT '[]',
            weekend_classes BOOLEAN DEFAULT 0,

            -- Time preference flags
            has_morning_classes BOOLEAN DEFAULT 0,
            has_early_morning BOOLEAN DEFAULT 0,
            has_evening_classes BOOLEAN DEFAULT 0,
            has_late_evening BOOLEAN DEFAULT 0,

            -- Daily intensity metrics
            max_daily_hours INTEGER DEFAULT 0,
            min_daily_hours INTEGER DEFAULT 0,
            avg_daily_hours INTEGER DEFAULT 0,

            -- Gap and break patterns
            has_lunch_break BOOLEAN DEFAULT 0,
            max_daily_gaps INTEGER DEFAULT 0,
            avg_gap_length INTEGER DEFAULT 0,

            -- Efficiency metrics
            schedule_span INTEGER DEFAULT 0,
            compactness_ratio REAL DEFAULT 0.0,

            -- Day pattern flags
            weekday_only BOOLEAN DEFAULT 0,
            has_monday BOOLEAN DEFAULT 0,
            has_tuesday BOOLEAN DEFAULT 0,
            has_wednesday BOOLEAN DEFAULT 0,
            has_thursday BOOLEAN DEFAULT 0,
            has_friday BOOLEAN DEFAULT 0,
            has_saturday BOOLEAN DEFAULT 0,
            has_sunday BOOLEAN DEFAULT 0,

            -- System columns
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,

            FOREIGN KEY (schedule_set_id) REFERENCES schedule_set(id) ON DELETE CASCADE,
            UNIQUE(schedule_set_id, schedule_index)
        )
    )";

    if (!executeQuery(query)) {
        Logger::get().logError("Failed to create enhanced schedule table");
        return false;
    }

    Logger::get().logInfo("Enhanced schedule table (v1) created successfully");
    return true;
}

bool DatabaseSchema::createScheduleSetIndexes() {
    bool success = true;

    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_set_name ON schedule_set(set_name)")) {
        Logger::get().logWarning("Failed to create schedule_set name index");
        success = false;
    }

    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_set_created_at ON schedule_set(created_at)")) {
        Logger::get().logWarning("Failed to create schedule_set created_at index");
        success = false;
    }

    if (success) {
        Logger::get().logInfo("Schedule set indexes created successfully");
    }
    return success;
}

bool DatabaseSchema::createScheduleIndexes() {
    bool success = true;

    // Existing indexes
    if (!executeQuery("CREATE INDEX IF NOT EXISTS idx_schedule_set_id ON schedule(schedule_set_id)")) {
        Logger::get().logWarning("Failed to create schedule set_id index");
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

    // NEW: Comprehensive indexes for common query patterns

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

    if (success) {
        Logger::get().logInfo("Enhanced schedule indexes created successfully");
    }
    return success;
}