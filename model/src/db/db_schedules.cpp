#include "db_schedules.h"
#include "sql_validator.h"

DatabaseScheduleManager::DatabaseScheduleManager(QSqlDatabase& database) : db(database) {
}

// insert schedules

bool DatabaseScheduleManager::insertSchedule(const InformativeSchedule& schedule) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for schedule insertion");
        return false;
    }

    QSqlQuery query(db);
    query.prepare(R"(
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
    )");

    string scheduleJson = DatabaseJsonHelpers::scheduleToJson(schedule);

    // Bind all values in correct order
    query.addBindValue(schedule.index);                                     // 1
    query.addBindValue(QString::fromStdString(schedule.unique_id));          // 2 - ADDED UNIQUE_ID
    query.addBindValue(QString::fromStdString(schedule.semester));          // 3 - ADDED SEMESTER
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

        // FIXED: Added semester and unique_id fields to the INSERT query
        const QString insertQuery = R"(
            INSERT INTO schedule
            (schedule_index, unique_id, semester, schedule_data_json,
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
        )";

        for (const auto& schedule : schedules) {
            QVariantList values;
            values << schedule.index                                             // 1
                   << QString::fromStdString(schedule.unique_id)                 // 2 - ADDED UNIQUE_ID
                   << QString::fromStdString(schedule.semester)                  // 3 - ADDED SEMESTER
                   << QString::fromStdString(DatabaseJsonHelpers::scheduleToJson(schedule)) // 4
                   << schedule.amount_days                                       // 5
                   << schedule.amount_gaps                                       // 6
                   << schedule.gaps_time                                         // 7
                   << schedule.avg_start                                         // 8
                   << schedule.avg_end                                           // 9
                   << schedule.earliest_start                                    // 10
                   << schedule.latest_end                                        // 11
                   << schedule.longest_gap                                       // 12
                   << schedule.total_class_time                                  // 13
                   << schedule.consecutive_days                                  // 14
                   << QString::fromStdString(schedule.days_json)                 // 15
                   << schedule.weekend_classes                                   // 16
                   << schedule.has_morning_classes                               // 17
                   << schedule.has_early_morning                                 // 18
                   << schedule.has_evening_classes                               // 19
                   << schedule.has_late_evening                                  // 20
                   << schedule.max_daily_hours                                   // 21
                   << schedule.min_daily_hours                                   // 22
                   << schedule.avg_daily_hours                                   // 23
                   << schedule.has_lunch_break                                   // 24
                   << schedule.max_daily_gaps                                    // 25
                   << schedule.avg_gap_length                                    // 26
                   << schedule.schedule_span                                     // 27
                   << schedule.compactness_ratio                                 // 28
                   << schedule.weekday_only                                      // 29
                   << schedule.has_monday                                        // 30
                   << schedule.has_tuesday                                       // 31
                   << schedule.has_wednesday                                     // 32
                   << schedule.has_thursday                                      // 33
                   << schedule.has_friday                                        // 34
                   << schedule.has_saturday                                      // 35
                   << schedule.has_sunday;                                       // 36

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


// remove schedules

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


// activate query

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

vector<string> DatabaseScheduleManager::executeCustomQueryForUniqueIds(const string& sqlQuery, const vector<string>& parameters) {
    vector<string> uniqueIds;

    if (!db.isOpen()) {
        Logger::get().logError("Database not open for custom query");
        return uniqueIds;
    }

    // Validate the SQL query for security
    SQLValidator::ValidationResult validation = SQLValidator::validateScheduleQuery(sqlQuery);
    if (!validation.isValid) {
        Logger::get().logError("SQL validation failed: " + validation.errorMessage);
        return uniqueIds;
    }

    try {
        QSqlQuery query(db);

        if (!query.prepare(QString::fromStdString(sqlQuery))) {
            Logger::get().logError("Failed to prepare query: " + query.lastError().text().toStdString());
            return uniqueIds;
        }

        for (const auto& param : parameters) {
            query.addBindValue(QString::fromStdString(param));
        }

        if (!query.exec()) {
            Logger::get().logError("Query execution failed: " + query.lastError().text().toStdString());
            Logger::get().logError("Query was: " + sqlQuery);
            return uniqueIds;
        }

        while (query.next()) {
            QVariant idVariant = query.value(0);
            string uniqueId = idVariant.toString().toStdString();

            if (!uniqueId.empty()) {
                uniqueIds.push_back(uniqueId);
            }
        }

        if (uniqueIds.empty()) {
            Logger::get().logWarning("No schedules matched query criteria");
        } else {
            Logger::get().logInfo("Query matched " + std::to_string(uniqueIds.size()) + " schedules");
        }

    } catch (const std::exception& e) {
        Logger::get().logError("Exception executing query: " + std::string(e.what()));
    }

    return uniqueIds;
}

string DatabaseScheduleManager::getUniqueIdByScheduleIndex(int scheduleIndex, const string& semester) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for unique ID lookup");
        return "";
    }

    QSqlQuery query(db);
    query.prepare("SELECT unique_id FROM schedule WHERE schedule_index = ? AND semester = ?");
    query.addBindValue(scheduleIndex);
    query.addBindValue(QString::fromStdString(semester));

    if (query.exec() && query.next()) {
        return query.value(0).toString().toStdString();
    }

    return "";
}

int DatabaseScheduleManager::getScheduleIndexByUniqueId(const string& uniqueId) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for schedule index lookup");
        return -1;
    }

    QSqlQuery query(db);
    query.prepare("SELECT schedule_index FROM schedule WHERE unique_id = ?");
    query.addBindValue(QString::fromStdString(uniqueId));

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }

    return -1;
}

vector<int> DatabaseScheduleManager::getScheduleIndicesByUniqueIds(const vector<string>& uniqueIds) {
    vector<int> indices;

    if (!db.isOpen() || uniqueIds.empty()) {
        return indices;
    }

    // Create IN clause for the query
    QString inClause = "(";
    for (size_t i = 0; i < uniqueIds.size(); ++i) {
        if (i > 0) inClause += ",";
        inClause += "?";
    }
    inClause += ")";

    QString queryStr = QString("SELECT schedule_index FROM schedule WHERE unique_id IN %1 ORDER BY schedule_index").arg(inClause);

    QSqlQuery query(db);
    if (!query.prepare(queryStr)) {
        Logger::get().logError("Failed to prepare unique ID lookup query: " + query.lastError().text().toStdString());
        return indices;
    }

    // Bind unique IDs
    for (const string& uniqueId : uniqueIds) {
        query.addBindValue(QString::fromStdString(uniqueId));
    }

    if (!query.exec()) {
        Logger::get().logError("Failed to execute unique ID lookup query: " + query.lastError().text().toStdString());
        return indices;
    }

    while (query.next()) {
        indices.push_back(query.value(0).toInt());
    }

    return indices;
}


// retrieve schedules

vector<InformativeSchedule> DatabaseScheduleManager::getAllSchedules() {
    vector<InformativeSchedule> schedules;
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for schedule retrieval");
        return schedules;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        SELECT id, schedule_index, schedule_data_json,
               amount_days, amount_gaps, gaps_time, avg_start, avg_end,
               earliest_start, latest_end, longest_gap, total_class_time,
               consecutive_days, days_json, weekend_classes,
               has_morning_classes, has_early_morning, has_evening_classes, has_late_evening,
               max_daily_hours, min_daily_hours, avg_daily_hours,
               has_lunch_break, max_daily_gaps, avg_gap_length,
               schedule_span, compactness_ratio, weekday_only,
               has_monday, has_tuesday, has_wednesday, has_thursday, has_friday, has_saturday, has_sunday
        FROM schedule
        ORDER BY schedule_index
    )");

    if (!query.exec()) {
        Logger::get().logError("Failed to retrieve schedules: " + query.lastError().text().toStdString());
        return schedules;
    }

    while (query.next()) {
        schedules.push_back(createScheduleFromQuery(query));
    }

    Logger::get().logInfo("Retrieved " + to_string(schedules.size()) + " schedules from database");
    return schedules;
}

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
            SELECT id, schedule_index, schedule_data_json,
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
        metadata += "User Identifier: schedule_index (1-based schedule number for filtering)\n\n";

        // Get statistics
        QSqlQuery statsQuery(R"(
            SELECT
                COUNT(*) as total_schedules,
                MIN(amount_days) as min_days, MAX(amount_days) as max_days,
                MIN(amount_gaps) as min_gaps, MAX(amount_gaps) as max_gaps,
                MIN(earliest_start) as min_earliest, MAX(earliest_start) as max_earliest,
                MIN(latest_end) as min_latest, MAX(latest_end) as max_latest
            FROM schedule
        )", db);

        if (statsQuery.exec() && statsQuery.next()) {
            int totalSchedules = statsQuery.value("total_schedules").toInt();
            metadata += "=== CURRENT DATA STATISTICS ===\n";
            metadata += "Total schedules in database: " + std::to_string(totalSchedules) + "\n\n";

            if (totalSchedules > 0) {
                metadata += "VALUE RANGES:\n";
                metadata += "- Study days: " + std::to_string(statsQuery.value("min_days").toInt()) +
                            " to " + std::to_string(statsQuery.value("max_days").toInt()) + "\n";
                metadata += "- Gaps: " + std::to_string(statsQuery.value("min_gaps").toInt()) +
                            " to " + std::to_string(statsQuery.value("max_gaps").toInt()) + "\n";
                metadata += "- Earliest start: " + std::to_string(statsQuery.value("min_earliest").toInt()) +
                            " to " + std::to_string(statsQuery.value("max_earliest").toInt()) + " (minutes from midnight)\n";
                metadata += "- Latest end: " + std::to_string(statsQuery.value("min_latest").toInt()) +
                            " to " + std::to_string(statsQuery.value("max_latest").toInt()) + " (minutes from midnight)\n\n";
            }
        }

        metadata += "=== TIME CONVERSION REFERENCE ===\n";
        metadata += "Minutes from midnight conversions:\n";
        metadata += "- 7:00 AM = 420   - 8:00 AM = 480   - 8:30 AM = 510   - 9:00 AM = 540\n";
        metadata += "- 10:00 AM = 600  - 11:00 AM = 660  - 12:00 PM = 720  - 1:00 PM = 780\n";
        metadata += "- 2:00 PM = 840   - 3:00 PM = 900   - 4:00 PM = 960   - 5:00 PM = 1020\n";
        metadata += "- 6:00 PM = 1080  - 7:00 PM = 1140  - 8:00 PM = 1200  - 9:00 PM = 1260\n\n";

        metadata += "=== AVAILABLE COLUMNS FOR FILTERING ===\n";
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

InformativeSchedule DatabaseScheduleManager::createScheduleFromQuery(QSqlQuery& query) {
    int scheduleIndex = query.value(1).toInt();
    string scheduleJson = query.value(2).toString().toStdString();

    // Basic metrics
    int amountDays = query.value(3).toInt();
    int amountGaps = query.value(4).toInt();
    int gapsTime = query.value(5).toInt();
    int avgStart = query.value(6).toInt();
    int avgEnd = query.value(7).toInt();

    // Enhanced metrics
    int earliestStart = query.value(8).toInt();
    int latestEnd = query.value(9).toInt();
    int longestGap = query.value(10).toInt();
    int totalClassTime = query.value(11).toInt();
    int consecutiveDays = query.value(12).toInt();
    string daysJson = query.value(13).toString().toStdString();
    bool weekendClasses = query.value(14).toBool();
    bool hasMorningClasses = query.value(15).toBool();
    bool hasEarlyMorning = query.value(16).toBool();
    bool hasEveningClasses = query.value(17).toBool();
    bool hasLateEvening = query.value(18).toBool();
    int maxDailyHours = query.value(19).toInt();
    int minDailyHours = query.value(20).toInt();
    int avgDailyHours = query.value(21).toInt();
    bool hasLunchBreak = query.value(22).toBool();
    int maxDailyGaps = query.value(23).toInt();
    int avgGapLength = query.value(24).toInt();
    int scheduleSpan = query.value(25).toInt();
    double compactnessRatio = query.value(26).toDouble();
    bool weekdayOnly = query.value(27).toBool();
    bool hasMonday = query.value(28).toBool();
    bool hasTuesday = query.value(29).toBool();
    bool hasWednesday = query.value(30).toBool();
    bool hasThursday = query.value(31).toBool();
    bool hasFriday = query.value(32).toBool();
    bool hasSaturday = query.value(33).toBool();
    bool hasSunday = query.value(34).toBool();

    // Create schedule from JSON with basic metrics
    InformativeSchedule schedule = DatabaseJsonHelpers::scheduleFromJson(
            scheduleJson, 0, scheduleIndex, amountDays, amountGaps, gapsTime, avgStart, avgEnd
    );

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


// helper methods

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

vector<string> DatabaseScheduleManager::getWhitelistedTables() {
    return {"schedule"};
}

vector<string> DatabaseScheduleManager::getWhitelistedColumns() {
    return {
            "schedule_index", "amount_days", "amount_gaps", "gaps_time",
            "avg_start", "avg_end", "id", "created_at", "earliest_start",
            "latest_end", "longest_gap", "total_class_time", "consecutive_days",
            "weekend_classes", "has_morning_classes", "has_early_morning",
            "has_evening_classes", "has_late_evening", "max_daily_hours",
            "min_daily_hours", "avg_daily_hours", "has_lunch_break",
            "max_daily_gaps", "avg_gap_length", "schedule_span",
            "compactness_ratio", "weekday_only", "has_monday", "has_tuesday",
            "has_wednesday", "has_thursday", "has_friday", "has_saturday", "has_sunday"
    };
}

void DatabaseScheduleManager::debugScheduleQuery(const string& debugQuery) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for debug query");
        return;
    }

    Logger::get().logInfo("=== DEBUG SCHEDULE QUERY ===");

    // Check what data we actually have
    QSqlQuery dataQuery(R"(
        SELECT
            schedule_index,
            earliest_start,
            avg_start,
            amount_days
        FROM schedule
        ORDER BY earliest_start
        LIMIT 10
    )", db);

    Logger::get().logInfo("Sample schedule data:");
    if (dataQuery.exec()) {
        while (dataQuery.next()) {
            int index = dataQuery.value(0).toInt();
            int earliest = dataQuery.value(1).toInt();
            int avg = dataQuery.value(2).toInt();
            int days = dataQuery.value(3).toInt();

            Logger::get().logInfo("Schedule " + to_string(index) +
                                  ": earliest_start=" + to_string(earliest) +
                                  " (" + to_string(earliest/60) + ":" + to_string(earliest%60) + ")" +
                                  ", avg_start=" + to_string(avg) +
                                  ", days=" + to_string(days));
        }
    }

    // Test a simple query
    Logger::get().logInfo("Testing query: SELECT schedule_index FROM schedule WHERE earliest_start > 600");

    QSqlQuery testQuery(db);
    testQuery.prepare("SELECT schedule_index FROM schedule WHERE earliest_start > ?");
    testQuery.addBindValue(600); // 10:00 AM = 600 minutes

    if (testQuery.exec()) {
        int count = 0;
        Logger::get().logInfo("Schedules starting after 10:00 AM:");
        while (testQuery.next()) {
            int scheduleIndex = testQuery.value(0).toInt();
            count++;
            if (count <= 5) { // Log first 5 results
                Logger::get().logInfo("  - Schedule " + to_string(scheduleIndex));
            }
        }
        Logger::get().logInfo("Total schedules starting after 10:00 AM: " + to_string(count));
    } else {
        Logger::get().logError("Test query failed: " + testQuery.lastError().text().toStdString());
    }

    // Check if earliest_start field is properly populated
    QSqlQuery nullQuery("SELECT COUNT(*) FROM schedule WHERE earliest_start IS NULL OR earliest_start = 0", db);
    if (nullQuery.exec() && nullQuery.next()) {
        int nullCount = nullQuery.value(0).toInt();
        Logger::get().logInfo("Schedules with NULL/0 earliest_start: " + to_string(nullCount));
    }

    // Check the range of earliest_start values
    QSqlQuery rangeQuery("SELECT MIN(earliest_start), MAX(earliest_start), AVG(earliest_start) FROM schedule", db);
    if (rangeQuery.exec() && rangeQuery.next()) {
        int minStart = rangeQuery.value(0).toInt();
        int maxStart = rangeQuery.value(1).toInt();
        double avgStart = rangeQuery.value(2).toDouble();

        Logger::get().logInfo("earliest_start range: " + to_string(minStart) +
                              " to " + to_string(maxStart) +
                              " (avg: " + to_string(avgStart) + ")");
        Logger::get().logInfo("In time format: " + to_string(minStart/60) + ":" + to_string(minStart%60) +
                              " to " + to_string(maxStart/60) + ":" + to_string(maxStart%60));
    }
}