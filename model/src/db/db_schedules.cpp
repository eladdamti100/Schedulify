#include "db_schedules.h"

DatabaseScheduleManager::DatabaseScheduleManager(QSqlDatabase& database) : db(database) {
}

// Core CRUD Operations

bool DatabaseScheduleManager::insertSchedule(const InformativeSchedule& schedule, const string& name) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for schedule insertion");
        return false;
    }

    if (!schedule.hasValidSemester()) {
        Logger::get().logError("Cannot insert schedule with invalid semester: " + std::to_string(schedule.semester));
        return false;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        INSERT INTO schedule
        (schedule_index, semester, schedule_name, schedule_data_json,
         amount_days, amount_gaps, gaps_time, avg_start, avg_end,
         earliest_start, latest_end, longest_gap, total_class_time,
         consecutive_days, days_json, weekend_classes,
         has_morning_classes, has_early_morning, has_evening_classes, has_late_evening,
         max_daily_hours, min_daily_hours, avg_daily_hours,
         has_lunch_break, max_daily_gaps, avg_gap_length,
         schedule_span, compactness_ratio, weekday_only,
         has_monday, has_tuesday, has_wednesday, has_thursday, has_friday, has_saturday, has_sunday,
         created_at, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,
                CURRENT_TIMESTAMP, CURRENT_TIMESTAMP)
    )");

    string scheduleJson = DatabaseJsonHelpers::scheduleToJson(schedule);

    // Bind all values in correct order
    query.addBindValue(schedule.index);                                     // 1
    query.addBindValue(schedule.semester);                                  // 2
    query.addBindValue(QString::fromStdString(name));                       // 3
    query.addBindValue(QString::fromStdString(scheduleJson));               // 4

    // Basic metrics (5-9)
    query.addBindValue(schedule.amount_days);                               // 5
    query.addBindValue(schedule.amount_gaps);                               // 6
    query.addBindValue(schedule.gaps_time);                                 // 7
    query.addBindValue(schedule.avg_start);                                 // 8
    query.addBindValue(schedule.avg_end);                                   // 9

    // Enhanced time metrics (10-13)
    query.addBindValue(schedule.earliest_start);                            // 10
    query.addBindValue(schedule.latest_end);                                // 11
    query.addBindValue(schedule.longest_gap);                               // 12
    query.addBindValue(schedule.total_class_time);                          // 13

    // Day patterns (14-16)
    query.addBindValue(schedule.consecutive_days);                          // 14
    query.addBindValue(QString::fromStdString(schedule.days_json));         // 15
    query.addBindValue(schedule.weekend_classes);                           // 16

    // Time preference booleans (17-20)
    query.addBindValue(schedule.has_morning_classes);                       // 17
    query.addBindValue(schedule.has_early_morning);                         // 18
    query.addBindValue(schedule.has_evening_classes);                       // 19
    query.addBindValue(schedule.has_late_evening);                          // 20

    // Daily intensity (21-23)
    query.addBindValue(schedule.max_daily_hours);                           // 21
    query.addBindValue(schedule.min_daily_hours);                           // 22
    query.addBindValue(schedule.avg_daily_hours);                           // 23

    // Gap patterns (24-26)
    query.addBindValue(schedule.has_lunch_break);                           // 24
    query.addBindValue(schedule.max_daily_gaps);                            // 25
    query.addBindValue(schedule.avg_gap_length);                            // 26

    // Compactness (27-29)
    query.addBindValue(schedule.schedule_span);                             // 27
    query.addBindValue(schedule.compactness_ratio);                         // 28
    query.addBindValue(schedule.weekday_only);                              // 29

    // Individual weekdays (30-36)
    query.addBindValue(schedule.has_monday);                                // 30
    query.addBindValue(schedule.has_tuesday);                               // 31
    query.addBindValue(schedule.has_wednesday);                             // 32
    query.addBindValue(schedule.has_thursday);                              // 33
    query.addBindValue(schedule.has_friday);                                // 34
    query.addBindValue(schedule.has_saturday);                              // 35
    query.addBindValue(schedule.has_sunday);                                // 36

    if (!query.exec()) {
        Logger::get().logError("Failed to insert schedule: " + query.lastError().text().toStdString());
        return false;
    }

    Logger::get().logInfo("Successfully inserted schedule " + std::to_string(schedule.index) +
                          " for " + schedule.getSemesterName());
    return true;
}

bool DatabaseScheduleManager::insertSchedules(const vector<InformativeSchedule>& schedules) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for schedule insertion");
        return false;
    }

    if (schedules.empty()) {
        Logger::get().logWarning("No schedules to insert");
        return true;
    }

    // Use bulk insert for better performance
    return insertSchedulesBulk(schedules);
}

bool DatabaseScheduleManager::deleteAllSchedules() {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for schedule deletion");
        return false;
    }

    QSqlQuery query("DELETE FROM schedule", db);
    if (!query.exec()) {
        Logger::get().logError("Failed to delete all schedules: " + query.lastError().text().toStdString());
        return false;
    }

    int rowsAffected = query.numRowsAffected();
    Logger::get().logInfo("Deleted all schedules from database (" + to_string(rowsAffected) + " schedules)");
    return true;
}

// Core Retrieval Operations

vector<InformativeSchedule> DatabaseScheduleManager::getAllSchedules() {
    vector<InformativeSchedule> schedules;
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for schedule retrieval");
        return schedules;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        SELECT id, schedule_index, semester, schedule_data_json,
               amount_days, amount_gaps, gaps_time, avg_start, avg_end,
               earliest_start, latest_end, longest_gap, total_class_time,
               consecutive_days, days_json, weekend_classes,
               has_morning_classes, has_early_morning, has_evening_classes, has_late_evening,
               max_daily_hours, min_daily_hours, avg_daily_hours,
               has_lunch_break, max_daily_gaps, avg_gap_length,
               schedule_span, compactness_ratio, weekday_only,
               has_monday, has_tuesday, has_wednesday, has_thursday, has_friday, has_saturday, has_sunday
        FROM schedule
        ORDER BY semester, schedule_index
    )");

    if (!query.exec()) {
        Logger::get().logError("Failed to retrieve schedules: " + query.lastError().text().toStdString());
        return schedules;
    }

    while (query.next()) {
        schedules.push_back(createScheduleFromQuery(query));
    }

    Logger::get().logInfo("Retrieved " + to_string(schedules.size()) + " schedules from database (ordered by semester)");
    return schedules;
}

vector<InformativeSchedule> DatabaseScheduleManager::getSchedulesBySemester(int semester) {
    vector<InformativeSchedule> schedules;
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for schedule retrieval by semester");
        return schedules;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        SELECT id, schedule_index, semester, schedule_data_json,
               amount_days, amount_gaps, gaps_time, avg_start, avg_end,
               earliest_start, latest_end, longest_gap, total_class_time,
               consecutive_days, days_json, weekend_classes,
               has_morning_classes, has_early_morning, has_evening_classes, has_late_evening,
               max_daily_hours, min_daily_hours, avg_daily_hours,
               has_lunch_break, max_daily_gaps, avg_gap_length,
               schedule_span, compactness_ratio, weekday_only,
               has_monday, has_tuesday, has_wednesday, has_thursday, has_friday, has_saturday, has_sunday
        FROM schedule
        WHERE semester = ?
        ORDER BY schedule_index
    )");
    query.addBindValue(semester);

    if (!query.exec()) {
        Logger::get().logError("Failed to retrieve schedules by semester: " + query.lastError().text().toStdString());
        return schedules;
    }

    while (query.next()) {
        schedules.push_back(createScheduleFromQuery(query));
    }

    string semesterName;
    switch (semester) {
        case 1: semesterName = "Semester A"; break;
        case 2: semesterName = "Semester B"; break;
        case 3: semesterName = "Summer"; break;
        case 4: semesterName = "Year-long"; break;
        default: semesterName = "Semester " + to_string(semester); break;
    }

    Logger::get().logInfo("Retrieved " + to_string(schedules.size()) + " schedules for " + semesterName);
    return schedules;
}

vector<InformativeSchedule> DatabaseScheduleManager::getSchedulesByIds(const vector<int>& scheduleIds) {
    vector<InformativeSchedule> schedules;

    if (!db.isOpen()) {
        Logger::get().logError("Database not open for schedule retrieval by IDs");
        return schedules;
    }

    if (scheduleIds.empty()) {
        Logger::get().logWarning("No schedule IDs provided for retrieval");
        return schedules;
    }

    try {
        // Create IN clause for the query
        QString inClause = "(";
        for (size_t i = 0; i < scheduleIds.size(); ++i) {
            if (i > 0) inClause += ",";
            inClause += "?";
        }
        inClause += ")";

        QString queryStr = QString(R"(
            SELECT id, schedule_index, semester, schedule_data_json,
                   amount_days, amount_gaps, gaps_time, avg_start, avg_end,
                   earliest_start, latest_end, longest_gap, total_class_time,
                   consecutive_days, days_json, weekend_classes,
                   has_morning_classes, has_early_morning, has_evening_classes, has_late_evening,
                   max_daily_hours, min_daily_hours, avg_daily_hours,
                   has_lunch_break, max_daily_gaps, avg_gap_length,
                   schedule_span, compactness_ratio, weekday_only,
                   has_monday, has_tuesday, has_wednesday, has_thursday, has_friday, has_saturday, has_sunday
            FROM schedule
            WHERE schedule_index IN %1
            ORDER BY schedule_index
        )").arg(inClause);

        QSqlQuery query(db);
        if (!query.prepare(queryStr)) {
            Logger::get().logError("Failed to prepare schedule retrieval query: " + query.lastError().text().toStdString());
            return schedules;
        }

        // Bind schedule IDs
        for (int id : scheduleIds) {
            query.addBindValue(id);
        }

        if (!query.exec()) {
            Logger::get().logError("Failed to execute schedule retrieval query: " + query.lastError().text().toStdString());
            return schedules;
        }

        while (query.next()) {
            schedules.push_back(createScheduleFromQuery(query));
        }

        Logger::get().logInfo("Retrieved " + to_string(schedules.size()) + " schedules by IDs");

    } catch (const exception& e) {
        Logger::get().logError("Exception during schedule retrieval by IDs: " + string(e.what()));
    }

    return schedules;
}

// SQL Filtering for Bot Integration

vector<int> DatabaseScheduleManager::executeCustomQuery(const string& sqlQuery, const vector<string>& parameters) {
    vector<int> scheduleIds;

    if (!db.isOpen()) {
        Logger::get().logError("Database not open for custom query");
        return scheduleIds;
    }

    // Validate the SQL query for security
    SQLValidator::ValidationResult validation = SQLValidator::validateScheduleQuery(sqlQuery);
    if (!validation.isValid) {
        Logger::get().logError("SQL validation failed: " + validation.errorMessage);
        return scheduleIds;
    }

    try {
        QSqlQuery query(db);

        if (!query.prepare(QString::fromStdString(sqlQuery))) {
            Logger::get().logError("Failed to prepare query: " + query.lastError().text().toStdString());
            return scheduleIds;
        }

        for (const auto& param : parameters) {
            query.addBindValue(QString::fromStdString(param));
        }

        if (!query.exec()) {
            Logger::get().logError("Query execution failed: " + query.lastError().text().toStdString());
            Logger::get().logError("Query was: " + sqlQuery);
            return scheduleIds;
        }

        while (query.next()) {
            QVariant idVariant = query.value(0);
            bool ok;
            int scheduleIndex = idVariant.toInt(&ok);

            if (ok && scheduleIndex > 0) {
                scheduleIds.push_back(scheduleIndex);
            }
        }

        if (scheduleIds.empty()) {
            Logger::get().logWarning("No schedules matched query criteria");
        } else {
            Logger::get().logInfo("Query matched " + std::to_string(scheduleIds.size()) + " schedules");
        }

    } catch (const std::exception& e) {
        Logger::get().logError("Exception executing query: " + std::string(e.what()));
    }

    return scheduleIds;
}

// Utility Operations

int DatabaseScheduleManager::getScheduleCount() {
    if (!db.isOpen()) {
        return -1;
    }

    QSqlQuery query("SELECT COUNT(*) FROM schedule", db);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }

    return -1;
}

// Performance Operations for Bulk Inserts

bool DatabaseScheduleManager::insertSchedulesBulk(const vector<InformativeSchedule>& schedules) {
    if (!db.isOpen() || schedules.empty()) {
        return false;
    }

    Logger::get().logInfo("Starting bulk insert of " + to_string(schedules.size()) + " schedules");

    // Optimize database for bulk operations
    DatabaseUtils::optimizeForBulkInserts(db);

    try {
        // Prepare batch data
        vector<QVariantList> batchData;
        batchData.reserve(schedules.size());

        const QString insertQuery = R"(
            INSERT INTO schedule
            (schedule_index, semester, schedule_data_json,
             amount_days, amount_gaps, gaps_time, avg_start, avg_end,
             earliest_start, latest_end, longest_gap, total_class_time,
             consecutive_days, days_json, weekend_classes,
             has_morning_classes, has_early_morning, has_evening_classes, has_late_evening,
             max_daily_hours, min_daily_hours, avg_daily_hours,
             has_lunch_break, max_daily_gaps, avg_gap_length,
             schedule_span, compactness_ratio, weekday_only,
             has_monday, has_tuesday, has_wednesday, has_thursday, has_friday, has_saturday, has_sunday,
             created_at, updated_at)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,
                    CURRENT_TIMESTAMP, CURRENT_TIMESTAMP)
        )";

        for (const auto& schedule : schedules) {
            QVariantList values;
            values << schedule.index                                             // 1
                   << schedule.semester                                          // 2
                   << QString::fromStdString(DatabaseJsonHelpers::scheduleToJson(schedule)) // 3
                   << schedule.amount_days                                       // 4
                   << schedule.amount_gaps                                       // 5
                   << schedule.gaps_time                                         // 6
                   << schedule.avg_start                                         // 7
                   << schedule.avg_end                                           // 8
                   << schedule.earliest_start                                    // 9
                   << schedule.latest_end                                        // 10
                   << schedule.longest_gap                                       // 11
                   << schedule.total_class_time                                  // 12
                   << schedule.consecutive_days                                  // 13
                   << QString::fromStdString(schedule.days_json)                 // 14
                   << schedule.weekend_classes                                   // 15
                   << schedule.has_morning_classes                               // 16
                   << schedule.has_early_morning                                 // 17
                   << schedule.has_evening_classes                               // 18
                   << schedule.has_late_evening                                  // 19
                   << schedule.max_daily_hours                                   // 20
                   << schedule.min_daily_hours                                   // 21
                   << schedule.avg_daily_hours                                   // 22
                   << schedule.has_lunch_break                                   // 23
                   << schedule.max_daily_gaps                                    // 24
                   << schedule.avg_gap_length                                    // 25
                   << schedule.schedule_span                                     // 26
                   << schedule.compactness_ratio                                 // 27
                   << schedule.weekday_only                                      // 28
                   << schedule.has_monday                                        // 29
                   << schedule.has_tuesday                                       // 30
                   << schedule.has_wednesday                                     // 31
                   << schedule.has_thursday                                      // 32
                   << schedule.has_friday                                        // 33
                   << schedule.has_saturday                                      // 34
                   << schedule.has_sunday;                                       // 35

            batchData.push_back(values);
        }

        // Execute batch insert
        bool success = DatabaseUtils::executeBatch(db, insertQuery, batchData);

        // Restore normal database settings
        DatabaseUtils::restoreNormalSettings(db);

        if (success) {
            Logger::get().logInfo("Bulk insert completed successfully");
        } else {
            Logger::get().logError("Bulk insert failed");
        }

        return success;

    } catch (const exception& e) {
        Logger::get().logError("Exception during bulk insert: " + string(e.what()));
        DatabaseUtils::restoreNormalSettings(db);
        return false;
    }
}

// Bot Integration - Metadata Generation

string DatabaseScheduleManager::getSchedulesMetadataForBot() {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for metadata generation");
        return "Database unavailable";
    }

    string metadata;

    try {
        metadata += "SCHEDULE DATABASE SCHEMA:\n";
        metadata += "Table: schedule\n";
        metadata += "Primary Key: id (internal database ID)\n";
        metadata += "User Identifier: schedule_index (1-based schedule number for filtering)\n";
        metadata += "Semester Field: semester (1=Semester A, 2=Semester B, 3=Summer, 4=Year-long)\n\n";

        // Get statistics by semester
        QSqlQuery statsQuery(R"(
            SELECT
                semester,
                COUNT(*) as count,
                MIN(amount_days) as min_days, MAX(amount_days) as max_days,
                MIN(amount_gaps) as min_gaps, MAX(amount_gaps) as max_gaps,
                MIN(earliest_start) as min_earliest, MAX(earliest_start) as max_earliest,
                MIN(latest_end) as min_latest, MAX(latest_end) as max_latest
            FROM schedule
            GROUP BY semester
            ORDER BY semester
        )", db);

        if (statsQuery.exec()) {
            metadata += "=== SCHEDULES BY SEMESTER ===\n";
            int totalSchedules = 0;

            while (statsQuery.next()) {
                int semester = statsQuery.value("semester").toInt();
                int count = statsQuery.value("count").toInt();
                totalSchedules += count;

                string semesterName;
                switch (semester) {
                    case 1: semesterName = "Semester A"; break;
                    case 2: semesterName = "Semester B"; break;
                    case 3: semesterName = "Summer"; break;
                    case 4: semesterName = "Year-long"; break;
                    default: semesterName = "Semester " + to_string(semester); break;
                }

                metadata += semesterName + ": " + to_string(count) + " schedules\n";

                if (count > 0) {
                    metadata += "  - Study days: " + to_string(statsQuery.value("min_days").toInt()) +
                                " to " + to_string(statsQuery.value("max_days").toInt()) + "\n";
                    metadata += "  - Gaps: " + to_string(statsQuery.value("min_gaps").toInt()) +
                                " to " + to_string(statsQuery.value("max_gaps").toInt()) + "\n";
                    metadata += "  - Start times: " + to_string(statsQuery.value("min_earliest").toInt()) +
                                " to " + to_string(statsQuery.value("max_earliest").toInt()) + " minutes\n";
                    metadata += "  - End times: " + to_string(statsQuery.value("min_latest").toInt()) +
                                " to " + to_string(statsQuery.value("max_latest").toInt()) + " minutes\n";
                }
            }

            metadata += "\nTotal schedules: " + to_string(totalSchedules) + "\n\n";
        }

        metadata += "=== TIME CONVERSION REFERENCE ===\n";
        metadata += "Minutes from midnight conversions:\n";
        metadata += "- 7:00 AM = 420   - 8:00 AM = 480   - 8:30 AM = 510   - 9:00 AM = 540\n";
        metadata += "- 10:00 AM = 600  - 11:00 AM = 660  - 12:00 PM = 720  - 1:00 PM = 780\n";
        metadata += "- 2:00 PM = 840   - 3:00 PM = 900   - 4:00 PM = 960   - 5:00 PM = 1020\n";
        metadata += "- 6:00 PM = 1080  - 7:00 PM = 1140  - 8:00 PM = 1200  - 9:00 PM = 1260\n\n";

        metadata += "=== AVAILABLE COLUMNS FOR FILTERING ===\n";
        metadata += "Semester: semester (1=A, 2=B, 3=Summer, 4=Year-long)\n";
        metadata += "Basic metrics: schedule_index, amount_days, amount_gaps, gaps_time, avg_start, avg_end\n";
        metadata += "Time metrics: earliest_start, latest_end, longest_gap, total_class_time\n";
        metadata += "Day patterns: consecutive_days, weekend_classes, weekday_only\n";
        metadata += "Time preferences: has_morning_classes, has_early_morning, has_evening_classes, has_late_evening\n";
        metadata += "Daily intensity: max_daily_hours, min_daily_hours, avg_daily_hours\n";
        metadata += "Gap patterns: has_lunch_break, max_daily_gaps, avg_gap_length\n";
        metadata += "Weekdays: has_monday, has_tuesday, has_wednesday, has_thursday, has_friday, has_saturday, has_sunday\n";

    } catch (const std::exception& e) {
        Logger::get().logError("Exception generating metadata: " + std::string(e.what()));
        metadata += "Error generating metadata: " + std::string(e.what());
    }

    return metadata;
}

// Private Helper Methods

InformativeSchedule DatabaseScheduleManager::createScheduleFromQuery(QSqlQuery& query) {
    int scheduleIndex = query.value("schedule_index").toInt();
    int semester = query.value("semester").toInt();
    string scheduleJson = query.value("schedule_data_json").toString().toStdString();

    // Basic metrics
    int amountDays = query.value("amount_days").toInt();
    int amountGaps = query.value("amount_gaps").toInt();
    int gapsTime = query.value("gaps_time").toInt();
    int avgStart = query.value("avg_start").toInt();
    int avgEnd = query.value("avg_end").toInt();

    // Enhanced metrics
    int earliestStart = query.value("earliest_start").toInt();
    int latestEnd = query.value("latest_end").toInt();
    int longestGap = query.value("longest_gap").toInt();
    int totalClassTime = query.value("total_class_time").toInt();
    int consecutiveDays = query.value("consecutive_days").toInt();
    string daysJson = query.value("days_json").toString().toStdString();
    bool weekendClasses = query.value("weekend_classes").toBool();
    bool hasMorningClasses = query.value("has_morning_classes").toBool();
    bool hasEarlyMorning = query.value("has_early_morning").toBool();
    bool hasEveningClasses = query.value("has_evening_classes").toBool();
    bool hasLateEvening = query.value("has_late_evening").toBool();
    int maxDailyHours = query.value("max_daily_hours").toInt();
    int minDailyHours = query.value("min_daily_hours").toInt();
    int avgDailyHours = query.value("avg_daily_hours").toInt();
    bool hasLunchBreak = query.value("has_lunch_break").toBool();
    int maxDailyGaps = query.value("max_daily_gaps").toInt();
    int avgGapLength = query.value("avg_gap_length").toInt();
    int scheduleSpan = query.value("schedule_span").toInt();
    double compactnessRatio = query.value("compactness_ratio").toDouble();
    bool weekdayOnly = query.value("weekday_only").toBool();
    bool hasMonday = query.value("has_monday").toBool();
    bool hasTuesday = query.value("has_tuesday").toBool();
    bool hasWednesday = query.value("has_wednesday").toBool();
    bool hasThursday = query.value("has_thursday").toBool();
    bool hasFriday = query.value("has_friday").toBool();
    bool hasSaturday = query.value("has_saturday").toBool();
    bool hasSunday = query.value("has_sunday").toBool();

    // Create schedule from JSON with basic metrics
    InformativeSchedule schedule = DatabaseJsonHelpers::scheduleFromJson(
            scheduleJson, 0, scheduleIndex, amountDays, amountGaps, gapsTime, avgStart, avgEnd
    );

    // Set semester field
    schedule.semester = semester;

    // Set enhanced metrics
    schedule.earliest_start = earliestStart;
    schedule.latest_end = latestEnd;
    schedule.longest_gap = longestGap;
    schedule.total_class_time = totalClassTime;
    schedule.consecutive_days = consecutiveDays;
    schedule.days_json = daysJson;
    schedule.weekend_classes = weekendClasses;
    schedule.has_morning_classes = hasMorningClasses;
    schedule.has_early_morning = hasEarlyMorning;
    schedule.has_evening_classes = hasEveningClasses;
    schedule.has_late_evening = hasLateEvening;
    schedule.max_daily_hours = maxDailyHours;
    schedule.min_daily_hours = minDailyHours;
    schedule.avg_daily_hours = avgDailyHours;
    schedule.has_lunch_break = hasLunchBreak;
    schedule.max_daily_gaps = maxDailyGaps;
    schedule.avg_gap_length = avgGapLength;
    schedule.schedule_span = scheduleSpan;
    schedule.compactness_ratio = compactnessRatio;
    schedule.weekday_only = weekdayOnly;
    schedule.has_monday = hasMonday;
    schedule.has_tuesday = hasTuesday;
    schedule.has_wednesday = hasWednesday;
    schedule.has_thursday = hasThursday;
    schedule.has_friday = hasFriday;
    schedule.has_saturday = hasSaturday;
    schedule.has_sunday = hasSunday;

    return schedule;
}

vector<string> DatabaseScheduleManager::getWhitelistedTables() {
    return {"schedule"};
}

vector<string> DatabaseScheduleManager::getWhitelistedColumns() {
    return {
            // Primary identifier
            "schedule_index", "id", "semester",

            // Basic existing metrics
            "amount_days", "amount_gaps", "gaps_time", "avg_start", "avg_end",

            // Enhanced time-based metrics
            "earliest_start", "latest_end", "longest_gap", "total_class_time",

            // Day pattern metrics
            "consecutive_days", "days_json", "weekend_classes",

            // Time preference flags (most commonly queried)
            "has_morning_classes", "has_early_morning",
            "has_evening_classes", "has_late_evening",

            // Daily intensity metrics
            "max_daily_hours", "min_daily_hours", "avg_daily_hours",

            // Gap and break patterns
            "has_lunch_break", "max_daily_gaps", "avg_gap_length",

            // Schedule efficiency metrics
            "schedule_span", "compactness_ratio", "weekday_only",

            // Individual weekday flags
            "has_monday", "has_tuesday", "has_wednesday",
            "has_thursday", "has_friday", "has_saturday", "has_sunday",

            // System columns
            "created_at", "updated_at"
    };
}

bool DatabaseScheduleManager::isValidScheduleQuery(const string& sqlQuery) {
    QString query = QString::fromStdString(sqlQuery).trimmed().toLower();

    // Must start with SELECT
    if (!query.startsWith("select")) {
        Logger::get().logWarning("Query rejected: must start with SELECT");
        return false;
    }

    // Must not contain dangerous keywords
    vector<string> forbiddenKeywords = {
            "insert", "update", "delete", "drop", "create", "alter",
            "truncate", "grant", "revoke", "exec", "execute",
            "declare", "cast", "convert", "union", "into"
    };

    for (const string& keyword : forbiddenKeywords) {
        if (query.contains(QString::fromStdString(keyword))) {
            Logger::get().logWarning("Query rejected: contains forbidden keyword: " + keyword);
            return false;
        }
    }

    // Must reference schedule table
    if (!query.contains("schedule")) {
        Logger::get().logWarning("Query rejected: must reference 'schedule' table");
        return false;
    }

    // Must select schedule_index
    if (!query.contains("schedule_index")) {
        Logger::get().logWarning("Query rejected: must select 'schedule_index' column");
        return false;
    }

    return true;
}