#include "db_schedules.h"
#include "sql_validator.h"

DatabaseScheduleManager::DatabaseScheduleManager(QSqlDatabase& database) : db(database) {
}

bool DatabaseScheduleManager::insertSchedule(const InformativeSchedule& schedule, const string& scheduleName,
                                             const vector<int>& sourceFileIds) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for schedule insertion");
        return false;
    }

    // Create a schedule set if we don't have one
    string setName = scheduleName.empty() ? "Generated Schedules " +
                                            QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toStdString()
                                          : scheduleName;

    int setId = createScheduleSet(setName, sourceFileIds);
    if (setId <= 0) {
        Logger::get().logError("Failed to create schedule set");
        return false;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        INSERT INTO schedule 
        (schedule_set_id, schedule_index, schedule_name, schedule_data_json, 
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
    query.addBindValue(setId);                                              // 1
    query.addBindValue(schedule.index);                                     // 2
    query.addBindValue(QString::fromStdString(scheduleName));               // 3
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
        Logger::get().logError("Failed to insert enhanced schedule: " + query.lastError().text().toStdString());
        return false;
    }

    // Update schedule count in set
    updateScheduleSetCount(setId);

    Logger::get().logInfo("Enhanced schedule inserted successfully with set ID: " + to_string(setId));
    return true;
}

bool DatabaseScheduleManager::insertSchedules(const vector<InformativeSchedule>& schedules, const string& setName,
                                              const vector<int>& sourceFileIds) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for schedule insertion");
        return false;
    }

    if (schedules.empty()) {
        Logger::get().logWarning("No schedules to insert");
        return true;
    }

    // Create schedule set
    string actualSetName = setName.empty() ?
                           "Generated Schedules " + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toStdString()
                                           : setName;

    int setId = createScheduleSet(actualSetName, sourceFileIds);
    if (setId <= 0) {
        Logger::get().logError("Failed to create schedule set");
        return false;
    }

    // Start transaction
    if (!db.transaction()) {
        Logger::get().logError("Failed to begin transaction for schedule insertion");
        return false;
    }

    int successCount = 0;
    for (const auto& schedule : schedules) {
        QSqlQuery query(db);
        query.prepare(R"(
            INSERT INTO schedule 
            (schedule_set_id, schedule_index, schedule_data_json, 
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
        query.addBindValue(setId);                                              // 1
        query.addBindValue(schedule.index);                                     // 2
        query.addBindValue(QString::fromStdString(scheduleJson));               // 3

        // Basic metrics (4-8)
        query.addBindValue(schedule.amount_days);                               // 4
        query.addBindValue(schedule.amount_gaps);                               // 5
        query.addBindValue(schedule.gaps_time);                                 // 6
        query.addBindValue(schedule.avg_start);                                 // 7
        query.addBindValue(schedule.avg_end);                                   // 8

        // Enhanced time metrics (9-12)
        query.addBindValue(schedule.earliest_start);                            // 9
        query.addBindValue(schedule.latest_end);                                // 10
        query.addBindValue(schedule.longest_gap);                               // 11
        query.addBindValue(schedule.total_class_time);                          // 12

        // Day patterns (13-15)
        query.addBindValue(schedule.consecutive_days);                          // 13
        query.addBindValue(QString::fromStdString(schedule.days_json));         // 14
        query.addBindValue(schedule.weekend_classes);                           // 15

        // Time preference booleans (16-19)
        query.addBindValue(schedule.has_morning_classes);                       // 16
        query.addBindValue(schedule.has_early_morning);                         // 17
        query.addBindValue(schedule.has_evening_classes);                       // 18
        query.addBindValue(schedule.has_late_evening);                          // 19

        // Daily intensity (20-22)
        query.addBindValue(schedule.max_daily_hours);                           // 20
        query.addBindValue(schedule.min_daily_hours);                           // 21
        query.addBindValue(schedule.avg_daily_hours);                           // 22

        // Gap patterns (23-25)
        query.addBindValue(schedule.has_lunch_break);                           // 23
        query.addBindValue(schedule.max_daily_gaps);                            // 24
        query.addBindValue(schedule.avg_gap_length);                            // 25

        // Compactness (26-28)
        query.addBindValue(schedule.schedule_span);                             // 26
        query.addBindValue(schedule.compactness_ratio);                         // 27
        query.addBindValue(schedule.weekday_only);                              // 28

        // Individual weekdays (29-35)
        query.addBindValue(schedule.has_monday);                                // 29
        query.addBindValue(schedule.has_tuesday);                               // 30
        query.addBindValue(schedule.has_wednesday);                             // 31
        query.addBindValue(schedule.has_thursday);                              // 32
        query.addBindValue(schedule.has_friday);                                // 33
        query.addBindValue(schedule.has_saturday);                              // 34
        query.addBindValue(schedule.has_sunday);                                // 35

        if (query.exec()) {
            successCount++;
        } else {
            Logger::get().logError("Failed to insert schedule index " + to_string(schedule.index) +
                                   ": " + query.lastError().text().toStdString());
        }
    }

    if (successCount == 0) {
        Logger::get().logError("Failed to insert any schedules, rolling back transaction");
        db.rollback();
        return false;
    }

    if (!db.commit()) {
        Logger::get().logError("Failed to commit schedule insertion transaction");
        db.rollback();
        return false;
    }

    // Update schedule count in set
    updateScheduleSetCount(setId);

    Logger::get().logInfo("Successfully inserted " + to_string(successCount) + "/" +
                          to_string(schedules.size()) + " schedules with set ID: " + to_string(setId));

    return successCount == static_cast<int>(schedules.size());
}

bool DatabaseScheduleManager::updateSchedule(int scheduleId, const InformativeSchedule& schedule) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for schedule update");
        return false;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        UPDATE schedule 
        SET schedule_data_json = ?, amount_days = ?, amount_gaps = ?, gaps_time = ?, 
            avg_start = ?, avg_end = ?, updated_at = CURRENT_TIMESTAMP
        WHERE id = ?
    )");

    string scheduleJson = DatabaseJsonHelpers::scheduleToJson(schedule);

    query.addBindValue(QString::fromStdString(scheduleJson));
    query.addBindValue(schedule.amount_days);
    query.addBindValue(schedule.amount_gaps);
    query.addBindValue(schedule.gaps_time);
    query.addBindValue(schedule.avg_start);
    query.addBindValue(schedule.avg_end);
    query.addBindValue(scheduleId);

    if (!query.exec()) {
        Logger::get().logError("Failed to update schedule: " + query.lastError().text().toStdString());
        return false;
    }

    int rowsAffected = query.numRowsAffected();
    if (rowsAffected == 0) {
        Logger::get().logWarning("No schedule found with ID: " + to_string(scheduleId));
        return false;
    }

    Logger::get().logInfo("Schedule updated successfully (ID: " + to_string(scheduleId) + ")");
    return true;
}

bool DatabaseScheduleManager::deleteSchedule(int scheduleId) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for schedule deletion");
        return false;
    }

    // Get the set ID before deletion to update count
    int setId = -1;
    QSqlQuery getSetQuery(db);
    getSetQuery.prepare("SELECT schedule_set_id FROM schedule WHERE id = ?");
    getSetQuery.addBindValue(scheduleId);
    if (getSetQuery.exec() && getSetQuery.next()) {
        setId = getSetQuery.value(0).toInt();
    }

    QSqlQuery query(db);
    query.prepare("DELETE FROM schedule WHERE id = ?");
    query.addBindValue(scheduleId);

    if (!query.exec()) {
        Logger::get().logError("Failed to delete schedule: " + query.lastError().text().toStdString());
        return false;
    }

    int rowsAffected = query.numRowsAffected();
    if (rowsAffected == 0) {
        Logger::get().logWarning("No schedule found with ID: " + to_string(scheduleId));
        return false;
    }

    // Update schedule count in set
    if (setId > 0) {
        updateScheduleSetCount(setId);
    }

    Logger::get().logInfo("Schedule deleted successfully (ID: " + to_string(scheduleId) + ")");
    return true;
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

bool DatabaseScheduleManager::deleteSchedulesBySetId(int setId) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for schedule deletion");
        return false;
    }

    QSqlQuery query(db);
    query.prepare("DELETE FROM schedule WHERE schedule_set_id = ?");
    query.addBindValue(setId);

    if (!query.exec()) {
        Logger::get().logError("Failed to delete schedules for set ID " + to_string(setId) +
                               ": " + query.lastError().text().toStdString());
        return false;
    }

    int deletedCount = query.numRowsAffected();

    // Update schedule count in set
    updateScheduleSetCount(setId);

    Logger::get().logInfo("Deleted " + to_string(deletedCount) + " schedules for set ID: " + to_string(setId));
    return true;
}

int DatabaseScheduleManager::createScheduleSet(const string& setName, const vector<int>& sourceFileIds) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for schedule set creation");
        return -1;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        INSERT INTO schedule_set (set_name, source_file_ids_json, created_at, updated_at)
        VALUES (?, ?, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP)
    )");

    // Convert file IDs to JSON
    QJsonArray jsonArray;
    for (int id : sourceFileIds) {
        jsonArray.append(id);
    }
    QJsonDocument doc(jsonArray);
    string fileIdsJson = doc.toJson(QJsonDocument::Compact).toStdString();

    query.addBindValue(QString::fromStdString(setName));
    query.addBindValue(QString::fromStdString(fileIdsJson));

    if (!query.exec()) {
        Logger::get().logError("Failed to create schedule set: " + query.lastError().text().toStdString());
        return -1;
    }

    QVariant lastId = query.lastInsertId();
    if (!lastId.isValid()) {
        Logger::get().logError("Failed to get schedule set ID after creation");
        return -1;
    }

    int setId = lastId.toInt();
    Logger::get().logInfo("Schedule set created: '" + setName + "' with ID: " + to_string(setId));
    return setId;
}

vector<InformativeSchedule> DatabaseScheduleManager::getSchedulesBySetId(int setId) {
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
        WHERE schedule_set_id = ?
        ORDER BY schedule_index
    )");
    query.addBindValue(setId);

    if (!query.exec()) {
        Logger::get().logError("Failed to retrieve enhanced schedules: " + query.lastError().text().toStdString());
        return schedules;
    }

    while (query.next()) {
        schedules.push_back(createScheduleFromQuery(query));
    }

    Logger::get().logInfo("Retrieved " + to_string(schedules.size()) +
                          " enhanced schedules for set ID: " + to_string(setId));
    return schedules;
}

vector<ScheduleSetEntity> DatabaseScheduleManager::getAllScheduleSets() {
    vector<ScheduleSetEntity> sets;
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for schedule set retrieval");
        return sets;
    }

    QSqlQuery query(R"(
        SELECT id, set_name, source_file_ids_json, schedule_count, created_at, updated_at
        FROM schedule_set 
        ORDER BY created_at DESC
    )", db);

    while (query.next()) {
        sets.push_back(createScheduleSetFromQuery(query));
    }

    Logger::get().logInfo("Retrieved " + to_string(sets.size()) + " schedule sets from database");
    return sets;
}

bool DatabaseScheduleManager::deleteScheduleSet(int setId) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for schedule set deletion");
        return false;
    }

    // Get set details before deletion for logging
    ScheduleSetEntity set = getScheduleSetById(setId);
    if (set.id == 0) {
        Logger::get().logError("Schedule set with ID " + to_string(setId) + " not found");
        return false;
    }

    // Due to CASCADE DELETE, deleting the set will automatically delete all schedules
    QSqlQuery query(db);
    query.prepare("DELETE FROM schedule_set WHERE id = ?");
    query.addBindValue(setId);

    if (!query.exec()) {
        Logger::get().logError("Failed to delete schedule set: " + query.lastError().text().toStdString());
        return false;
    }

    int rowsAffected = query.numRowsAffected();
    if (rowsAffected == 0) {
        Logger::get().logWarning("No schedule set found with ID: " + to_string(setId));
        return false;
    }

    Logger::get().logInfo("Schedule set deleted successfully: '" + set.set_name + "' (ID: " + to_string(setId) + ")");
    return true;
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

    // Enhanced metrics (add these after the basic metrics in your SELECT query)
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

ScheduleSetEntity DatabaseScheduleManager::createScheduleSetFromQuery(QSqlQuery& query) {
    ScheduleSetEntity set;
    set.id = query.value(0).toInt();
    set.set_name = query.value(1).toString().toStdString();
    set.source_file_ids_json = query.value(2).toString().toStdString();
    set.schedule_count = query.value(3).toInt();
    set.created_at = query.value(4).toDateTime();
    set.updated_at = query.value(5).toDateTime();
    return set;
}

bool DatabaseScheduleManager::updateScheduleSetCount(int setId) {
    QSqlQuery countQuery(db);
    countQuery.prepare("SELECT COUNT(*) FROM schedule WHERE schedule_set_id = ?");
    countQuery.addBindValue(setId);

    if (!countQuery.exec() || !countQuery.next()) {
        Logger::get().logWarning("Failed to count schedules for set ID: " + to_string(setId));
        return false;
    }

    int count = countQuery.value(0).toInt();

    QSqlQuery updateQuery(db);
    updateQuery.prepare("UPDATE schedule_set SET schedule_count = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?");
    updateQuery.addBindValue(count);
    updateQuery.addBindValue(setId);

    return updateQuery.exec();
}

bool DatabaseScheduleManager::scheduleExists(int scheduleId) {
    if (!db.isOpen()) {
        return false;
    }

    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM schedule WHERE id = ?");
    query.addBindValue(scheduleId);

    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }

    return false;
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

int DatabaseScheduleManager::getScheduleCountBySetId(int setId) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for schedule count by set ID");
        return -1;
    }

    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM schedule WHERE schedule_set_id = ?");
    query.addBindValue(setId);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }

    Logger::get().logError("Failed to get schedule count for set ID: " + to_string(setId));
    return -1;
}

bool DatabaseScheduleManager::scheduleSetExists(int setId) {
    if (!db.isOpen()) {
        return false;
    }

    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM schedule_set WHERE id = ?");
    query.addBindValue(setId);

    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }

    return false;
}

int DatabaseScheduleManager::getScheduleSetCount() {
    if (!db.isOpen()) {
        return -1;
    }

    QSqlQuery query("SELECT COUNT(*) FROM schedule_set", db);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }

    return -1;
}

ScheduleSetEntity DatabaseScheduleManager::getScheduleSetById(int setId) {
    ScheduleSetEntity set;

    if (!db.isOpen()) {
        Logger::get().logError("Database not open for schedule set retrieval");
        return set;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        SELECT id, set_name, source_file_ids_json, schedule_count, created_at, updated_at
        FROM schedule_set
        WHERE id = ?
    )");
    query.addBindValue(setId);

    if (query.exec() && query.next()) {
        set = createScheduleSetFromQuery(query);
    } else {
        Logger::get().logWarning("No schedule set found with ID: " + to_string(setId));
    }

    return set;
}

bool DatabaseScheduleManager::updateScheduleSet(int setId, const string& setName) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for schedule set update");
        return false;
    }

    QSqlQuery query(db);
    query.prepare("UPDATE schedule_set SET set_name = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?");
    query.addBindValue(QString::fromStdString(setName));
    query.addBindValue(setId);

    if (!query.exec()) {
        Logger::get().logError("Failed to update schedule set: " + query.lastError().text().toStdString());
        return false;
    }

    int rowsAffected = query.numRowsAffected();
    if (rowsAffected == 0) {
        Logger::get().logWarning("No schedule set found with ID: " + to_string(setId));
        return false;
    }

    Logger::get().logInfo("Schedule set updated successfully (ID: " + to_string(setId) + ")");
    return true;
}

map<string, int> DatabaseScheduleManager::getScheduleStatistics() {
    map<string, int> stats;

    if (!db.isOpen()) {
        Logger::get().logError("Database not open for statistics");
        return stats;
    }

    // Get basic counts
    stats["total_sets"] = getScheduleSetCount();
    stats["total_schedules"] = getScheduleCount();

    // Get average schedules per set
    QSqlQuery avgQuery("SELECT AVG(schedule_count) FROM schedule_set", db);
    if (avgQuery.exec() && avgQuery.next()) {
        stats["avg_schedules_per_set"] = avgQuery.value(0).toInt();
    }

    // Get schedules created in last 24 hours
    QSqlQuery recentQuery(R"(
        SELECT COUNT(*) FROM schedule
        WHERE created_at >= datetime('now', '-1 day')
    )", db);
    if (recentQuery.exec() && recentQuery.next()) {
        stats["schedules_last_24h"] = recentQuery.value(0).toInt();
    }

    // Get sets created in last 7 days
    QSqlQuery setsQuery(R"(
        SELECT COUNT(*) FROM schedule_set
        WHERE created_at >= datetime('now', '-7 days')
    )", db);
    if (setsQuery.exec() && setsQuery.next()) {
        stats["sets_last_7_days"] = setsQuery.value(0).toInt();
    }

    return stats;
}

vector<InformativeSchedule> DatabaseScheduleManager::getRecentSchedules(int limit) {
    vector<InformativeSchedule> schedules;

    if (!db.isOpen()) {
        Logger::get().logError("Database not open for recent schedules");
        return schedules;
    }

    QSqlQuery query(db);
    query.prepare(R"(
        SELECT id, schedule_index, schedule_data_json, amount_days, amount_gaps,
               gaps_time, avg_start, avg_end
        FROM schedule
        ORDER BY created_at DESC
        LIMIT ?
    )");
    query.addBindValue(limit);

    if (!query.exec()) {
        Logger::get().logError("Failed to retrieve recent schedules: " + query.lastError().text().toStdString());
        return schedules;
    }

    while (query.next()) {
        schedules.push_back(createScheduleFromQuery(query));
    }

    Logger::get().logInfo("Retrieved " + to_string(schedules.size()) + " recent schedules");
    return schedules;
}

bool DatabaseScheduleManager::insertSchedulesBulk(const vector<InformativeSchedule>& schedules, int setId) {
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
            (schedule_set_id, schedule_index, schedule_data_json,
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
            values << setId                                                      // 1
                   << schedule.index                                             // 2
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
            // Update schedule count in set
            updateScheduleSetCount(setId);
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

vector<InformativeSchedule> DatabaseScheduleManager::getAllSchedules() {
    return {};
}

vector<InformativeSchedule> DatabaseScheduleManager::getSchedulesByMetrics(int maxDays, int maxGaps, int maxGapTime,
                                                                           int minAvgStart, int maxAvgStart,
                                                                           int minAvgEnd, int maxAvgEnd) {
    return {};
}

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

        // Only log result count, not details
        if (scheduleIds.empty()) {
            Logger::get().logWarning("No schedules matched query criteria");

            Logger::get().logError("Query execution failed: " + query.lastError().text().toStdString());

            // Debug: Check if column exists
            QSqlQuery debugQuery("PRAGMA table_info(schedule)", db);
            Logger::get().logInfo("=== SCHEDULE TABLE COLUMNS ===");
            while (debugQuery.next()) {
                QString columnName = debugQuery.value(1).toString();
                Logger::get().logInfo("Column: " + columnName.toStdString());
            }

            // Debug: Check if we have any schedule data
            QSqlQuery countQuery("SELECT COUNT(*), MIN(earliest_start), MAX(earliest_start) FROM schedule", db);
            if (countQuery.exec() && countQuery.next()) {
                int count = countQuery.value(0).toInt();
                int minStart = countQuery.value(1).toInt();
                int maxStart = countQuery.value(2).toInt();
                Logger::get().logInfo("Schedules: " + std::to_string(count) +
                                      ", earliest_start range: " + std::to_string(minStart) +
                                      " to " + std::to_string(maxStart));
            }
        } else {
            Logger::get().logInfo("Query matched " + std::to_string(scheduleIds.size()) + " schedules");
        }

    } catch (const std::exception& e) {
        Logger::get().logError("Exception executing query: " + std::string(e.what()));
    }

    return scheduleIds;
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
            SELECT id, schedule_index, schedule_data_json, amount_days, amount_gaps,
                   gaps_time, avg_start, avg_end
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
        // Just build the metadata without excessive logging
        metadata += "ENHANCED SCHEDULE DATABASE SCHEMA:\n";
        metadata += "Table: schedule\n";
        metadata += "Primary Key: id (internal database ID)\n";
        metadata += "User Identifier: schedule_index (1-based schedule number for filtering)\n\n";

        // ... rest of metadata construction without Logger::get().logInfo calls ...

        // Get statistics but don't log every step
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

            // Add ranges without logging each one
            if (totalSchedules > 0) {
                metadata += "VALUE RANGES:\n";
                metadata += "- Study days: " + std::to_string(statsQuery.value("min_days").toInt()) +
                            " to " + std::to_string(statsQuery.value("max_days").toInt()) + "\n";
                // ... etc without individual logs
            }
        }

        // Add time conversion and examples without logging
        metadata += "=== TIME CONVERSION REFERENCE ===\n";
        metadata += "Minutes from midnight conversions:\n";
        metadata += "- 7:00 AM = 420   - 8:00 AM = 480   - 8:30 AM = 510   - 9:00 AM = 540\n";
        // ... rest of metadata

    } catch (const std::exception& e) {
        Logger::get().logError("Exception generating metadata: " + std::string(e.what()));
        metadata += "Error generating metadata: " + std::string(e.what());
    }

    return metadata;
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

    // Check for valid columns only
    vector<string> validColumns = getWhitelistedColumns();

    // Basic validation passed
    Logger::get().logInfo("SQL query validation passed: " + sqlQuery);
    return true;
}

vector<string> DatabaseScheduleManager::getWhitelistedTables() {
    return {"schedule", "schedule_set"};
}

vector<string> DatabaseScheduleManager::getWhitelistedColumns() {
    return {
            "schedule_index", "amount_days", "amount_gaps", "gaps_time",
            "avg_start", "avg_end", "id", "schedule_set_id", "created_at"
    };
}

void DatabaseScheduleManager::debugScheduleQuery(const string& debugQuery) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for debug query");
        return;
    }

    Logger::get().logInfo("=== DEBUG SCHEDULE QUERY ===");

    // First, let's check what data we actually have
    QSqlQuery dataQuery(R"(
        SELECT
            schedule_index,
            earliest_start,
            avg_start,
            amount_days,
            schedule_data_json
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

    // Check the specific query that should work for "start after 10 AM"
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

void DatabaseScheduleManager::debugPrintScheduleData(int limit) {
    if (!db.isOpen()) {
        Logger::get().logError("Database not open for debug inspection");
        return;
    }

    Logger::get().logInfo("=== DEBUG: SCHEDULE DATABASE INSPECTION ===");

    QSqlQuery query(db);
    query.prepare(R"(
        SELECT
            schedule_index,
            amount_days, amount_gaps, gaps_time, avg_start, avg_end,
            earliest_start, latest_end, longest_gap, total_class_time,
            consecutive_days, weekend_classes,
            has_morning_classes, has_early_morning, has_evening_classes, has_late_evening,
            max_daily_hours, min_daily_hours, avg_daily_hours,
            has_lunch_break, max_daily_gaps, avg_gap_length,
            schedule_span, compactness_ratio, weekday_only,
            has_monday, has_tuesday, has_wednesday, has_thursday, has_friday, has_saturday, has_sunday
        FROM schedule
        ORDER BY schedule_index
        LIMIT ?
    )");

    query.addBindValue(limit);

    if (!query.exec()) {
        Logger::get().logError("Failed to execute debug query: " + query.lastError().text().toStdString());
        return;
    }

    int count = 0;
    Logger::get().logInfo("Showing first " + std::to_string(limit) + " schedules:");
    Logger::get().logInfo("Format: [Index] BasicMetrics | TimeMetrics | BooleanFlags | DayFlags");

    while (query.next() && count < limit) {
        count++;

        // Basic metrics
        int scheduleIndex = query.value(0).toInt();
        int amountDays = query.value(1).toInt();
        int amountGaps = query.value(2).toInt();
        int gapsTime = query.value(3).toInt();
        int avgStart = query.value(4).toInt();
        int avgEnd = query.value(5).toInt();

        // Enhanced time metrics
        int earliestStart = query.value(6).toInt();
        int latestEnd = query.value(7).toInt();
        int longestGap = query.value(8).toInt();
        int totalClassTime = query.value(9).toInt();
        int consecutiveDays = query.value(10).toInt();
        bool weekendClasses = query.value(11).toBool();

        // Time preference flags
        bool hasMorning = query.value(12).toBool();
        bool hasEarlyMorning = query.value(13).toBool();
        bool hasEvening = query.value(14).toBool();
        bool hasLateEvening = query.value(15).toBool();

        // Daily intensity
        int maxDailyHours = query.value(16).toInt();
        int minDailyHours = query.value(17).toInt();
        int avgDailyHours = query.value(18).toInt();

        // Gap patterns
        bool hasLunchBreak = query.value(19).toBool();
        int maxDailyGaps = query.value(20).toInt();
        int avgGapLength = query.value(21).toInt();

        // Efficiency
        int scheduleSpan = query.value(22).toInt();
        double compactnessRatio = query.value(23).toDouble();
        bool weekdayOnly = query.value(24).toBool();

        // Individual days
        bool hasMonday = query.value(25).toBool();
        bool hasTuesday = query.value(26).toBool();
        bool hasWednesday = query.value(27).toBool();
        bool hasThursday = query.value(28).toBool();
        bool hasFriday = query.value(29).toBool();
        bool hasSaturday = query.value(30).toBool();
        bool hasSunday = query.value(31).toBool();

        // Format the output for easy reading
        Logger::get().logInfo("--- Schedule " + std::to_string(scheduleIndex) + " ---");

        // Basic metrics (should be populated)
        Logger::get().logInfo("  Basic: days=" + std::to_string(amountDays) +
                              ", gaps=" + std::to_string(amountGaps) +
                              ", gap_time=" + std::to_string(gapsTime) +
                              ", avg_start=" + std::to_string(avgStart) +
                              ", avg_end=" + std::to_string(avgEnd));

        // Enhanced time metrics (these might be 0 if not calculated)
        Logger::get().logInfo("  Time: earliest=" + std::to_string(earliestStart) +
                              " (" + std::to_string(earliestStart/60) + ":" + std::to_string(earliestStart%60) + ")" +
                              ", latest=" + std::to_string(latestEnd) +
                              " (" + std::to_string(latestEnd/60) + ":" + std::to_string(latestEnd%60) + ")" +
                              ", longest_gap=" + std::to_string(longestGap) +
                              ", total_class=" + std::to_string(totalClassTime));

        // Boolean flags
        Logger::get().logInfo("  Flags: morning=" + std::to_string(hasMorning) +
                              ", early=" + std::to_string(hasEarlyMorning) +
                              ", evening=" + std::to_string(hasEvening) +
                              ", late=" + std::to_string(hasLateEvening) +
                              ", weekend=" + std::to_string(weekendClasses) +
                              ", weekday_only=" + std::to_string(weekdayOnly));

        // Daily patterns
        Logger::get().logInfo("  Daily: max_hours=" + std::to_string(maxDailyHours) +
                              ", min_hours=" + std::to_string(minDailyHours) +
                              ", avg_hours=" + std::to_string(avgDailyHours) +
                              ", consecutive=" + std::to_string(consecutiveDays) +
                              ", span=" + std::to_string(scheduleSpan) +
                              ", compactness=" + std::to_string(compactnessRatio));

        // Day flags
        std::string dayFlags = "  Days: ";
        if (hasMonday) dayFlags += "Mon ";
        if (hasTuesday) dayFlags += "Tue ";
        if (hasWednesday) dayFlags += "Wed ";
        if (hasThursday) dayFlags += "Thu ";
        if (hasFriday) dayFlags += "Fri ";
        if (hasSaturday) dayFlags += "Sat ";
        if (hasSunday) dayFlags += "Sun ";
        Logger::get().logInfo(dayFlags);

        Logger::get().logInfo("");
    }

    Logger::get().logInfo("=== END DEBUG INSPECTION ===");
    Logger::get().logInfo("Total schedules inspected: " + std::to_string(count));

    // Additional summary
    QSqlQuery summaryQuery(R"(
        SELECT
            COUNT(*) as total_schedules,
            COUNT(CASE WHEN earliest_start > 0 THEN 1 END) as non_zero_earliest,
            COUNT(CASE WHEN latest_end > 0 THEN 1 END) as non_zero_latest,
            COUNT(CASE WHEN has_morning_classes = 1 THEN 1 END) as with_morning,
            COUNT(CASE WHEN has_early_morning = 1 THEN 1 END) as with_early_morning,
            MIN(earliest_start) as min_earliest,
            MAX(earliest_start) as max_earliest,
            MIN(latest_end) as min_latest,
            MAX(latest_end) as max_latest
        FROM schedule
    )", db);

    if (summaryQuery.exec() && summaryQuery.next()) {
        int totalSchedules = summaryQuery.value(0).toInt();
        int nonZeroEarliest = summaryQuery.value(1).toInt();
        int nonZeroLatest = summaryQuery.value(2).toInt();
        int withMorning = summaryQuery.value(3).toInt();
        int withEarlyMorning = summaryQuery.value(4).toInt();
        int minEarliest = summaryQuery.value(5).toInt();
        int maxEarliest = summaryQuery.value(6).toInt();
        int minLatest = summaryQuery.value(7).toInt();
        int maxLatest = summaryQuery.value(8).toInt();

        Logger::get().logInfo("=== SUMMARY STATISTICS ===");
        Logger::get().logInfo("Total schedules: " + std::to_string(totalSchedules));
        Logger::get().logInfo("Schedules with earliest_start > 0: " + std::to_string(nonZeroEarliest) + "/" + std::to_string(totalSchedules));
        Logger::get().logInfo("Schedules with latest_end > 0: " + std::to_string(nonZeroLatest) + "/" + std::to_string(totalSchedules));
        Logger::get().logInfo("Schedules with morning classes: " + std::to_string(withMorning));
        Logger::get().logInfo("Schedules with early morning classes: " + std::to_string(withEarlyMorning));
        Logger::get().logInfo("Earliest start range: " + std::to_string(minEarliest) + " to " + std::to_string(maxEarliest));
        Logger::get().logInfo("Latest end range: " + std::to_string(minLatest) + " to " + std::to_string(maxLatest));

        if (nonZeroEarliest == 0) {
            Logger::get().logError("PROBLEM FOUND: All earliest_start values are 0!");
        }
        if (nonZeroLatest == 0) {
            Logger::get().logError("PROBLEM FOUND: All latest_end values are 0!");
        }
    }
}