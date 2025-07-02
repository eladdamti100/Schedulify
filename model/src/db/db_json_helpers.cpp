#include "db_json_helpers.h"

string DatabaseJsonHelpers::groupsToJson(const vector<Group>& groups) {
    QJsonArray groupsArray;

    for (const auto& group : groups) {
        QJsonObject groupObj;

        // Convert SessionType to string - ENHANCED with all types
        QString typeStr;
        switch (group.type) {
            case SessionType::LECTURE: typeStr = "LECTURE"; break;
            case SessionType::TUTORIAL: typeStr = "TUTORIAL"; break;
            case SessionType::LAB: typeStr = "LAB"; break;
            case SessionType::BLOCK: typeStr = "BLOCK"; break;
            case SessionType::DEPARTMENTAL_SESSION: typeStr = "DEPARTMENTAL_SESSION"; break;
            case SessionType::REINFORCEMENT: typeStr = "REINFORCEMENT"; break;
            case SessionType::GUIDANCE: typeStr = "GUIDANCE"; break;
            case SessionType::OPTIONAL_COLLOQUIUM: typeStr = "OPTIONAL_COLLOQUIUM"; break;
            case SessionType::REGISTRATION: typeStr = "REGISTRATION"; break;
            case SessionType::THESIS: typeStr = "THESIS"; break;
            case SessionType::PROJECT: typeStr = "PROJECT"; break;
            case SessionType::UNSUPPORTED:
            default: typeStr = "UNSUPPORTED"; break;
        }
        groupObj["type"] = typeStr;

        QJsonArray sessionsArray;
        for (const auto& session : group.sessions) {
            QJsonObject sessionObj;
            sessionObj["day_of_week"] = session.day_of_week;
            sessionObj["start_time"] = QString::fromStdString(session.start_time);
            sessionObj["end_time"] = QString::fromStdString(session.end_time);
            sessionObj["building_number"] = QString::fromStdString(session.building_number);
            sessionObj["room_number"] = QString::fromStdString(session.room_number);
            sessionsArray.append(sessionObj);
        }
        groupObj["sessions"] = sessionsArray;

        groupsArray.append(groupObj);
    }

    QJsonDocument doc(groupsArray);
    return doc.toJson(QJsonDocument::Compact).toStdString();
}

vector<Group> DatabaseJsonHelpers::groupsFromJson(const string& json) {
    vector<Group> groups;

    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    if (!doc.isArray()) return groups;

    QJsonArray groupsArray = doc.array();
    for (const auto& value : groupsArray) {
        if (!value.isObject()) continue;

        QJsonObject groupObj = value.toObject();
        Group group;

        // Convert string to SessionType - ENHANCED with all types
        QString typeStr = groupObj["type"].toString();
        if (typeStr == "LECTURE") group.type = SessionType::LECTURE;
        else if (typeStr == "TUTORIAL") group.type = SessionType::TUTORIAL;
        else if (typeStr == "LAB") group.type = SessionType::LAB;
        else if (typeStr == "BLOCK") group.type = SessionType::BLOCK;
        else if (typeStr == "DEPARTMENTAL_SESSION") group.type = SessionType::DEPARTMENTAL_SESSION;
        else if (typeStr == "REINFORCEMENT") group.type = SessionType::REINFORCEMENT;
        else if (typeStr == "GUIDANCE") group.type = SessionType::GUIDANCE;
        else if (typeStr == "OPTIONAL_COLLOQUIUM") group.type = SessionType::OPTIONAL_COLLOQUIUM;
        else if (typeStr == "REGISTRATION") group.type = SessionType::REGISTRATION;
        else if (typeStr == "THESIS") group.type = SessionType::THESIS;
        else if (typeStr == "PROJECT") group.type = SessionType::PROJECT;
        else group.type = SessionType::UNSUPPORTED;

        QJsonArray sessionsArray = groupObj["sessions"].toArray();
        for (const auto& sessionValue : sessionsArray) {
            if (!sessionValue.isObject()) continue;

            QJsonObject sessionObj = sessionValue.toObject();
            Session session;
            session.day_of_week = sessionObj["day_of_week"].toInt();
            session.start_time = sessionObj["start_time"].toString().toStdString();
            session.end_time = sessionObj["end_time"].toString().toStdString();
            session.building_number = sessionObj["building_number"].toString().toStdString();
            session.room_number = sessionObj["room_number"].toString().toStdString();

            group.sessions.push_back(session);
        }

        groups.push_back(group);
    }

    return groups;
}

string DatabaseJsonHelpers::scheduleToJson(const InformativeSchedule& schedule) {
    QJsonObject scheduleObj;

    QJsonArray weekArray;
    for (const auto& day : schedule.week) {
        QJsonObject dayObj;
        dayObj["day"] = QString::fromStdString(day.day);

        QJsonArray itemsArray;
        for (const auto& item : day.day_items) {
            QJsonObject itemObj;
            itemObj["courseName"] = QString::fromStdString(item.courseName);
            itemObj["raw_id"] = QString::fromStdString(item.raw_id);
            itemObj["type"] = QString::fromStdString(item.type);
            itemObj["start"] = QString::fromStdString(item.start);
            itemObj["end"] = QString::fromStdString(item.end);
            itemObj["building"] = QString::fromStdString(item.building);
            itemObj["room"] = QString::fromStdString(item.room);
            itemsArray.append(itemObj);
        }
        dayObj["day_items"] = itemsArray;

        weekArray.append(dayObj);
    }
    scheduleObj["week"] = weekArray;

    QJsonDocument doc(scheduleObj);
    return doc.toJson(QJsonDocument::Compact).toStdString();
}

InformativeSchedule DatabaseJsonHelpers::scheduleFromJson(const string& json, int id, int schedule_index,
                                                          int amount_days, int amount_gaps, int gaps_time,
                                                          int avg_start, int avg_end) {
    InformativeSchedule schedule;

    // Set basic metrics
    schedule.index = schedule_index;
    schedule.amount_days = amount_days;
    schedule.amount_gaps = amount_gaps;
    schedule.gaps_time = gaps_time;
    schedule.avg_start = avg_start;
    schedule.avg_end = avg_end;

    // Parse the JSON schedule data
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    if (!doc.isObject()) return schedule;

    QJsonObject scheduleObj = doc.object();
    QJsonArray weekArray = scheduleObj["week"].toArray();

    for (const auto& value : weekArray) {
        if (!value.isObject()) continue;

        QJsonObject dayObj = value.toObject();
        ScheduleDay scheduleDay;
        scheduleDay.day = dayObj["day"].toString().toStdString();

        QJsonArray itemsArray = dayObj["day_items"].toArray();
        for (const auto& itemValue : itemsArray) {
            if (!itemValue.isObject()) continue;

            QJsonObject itemObj = itemValue.toObject();
            ScheduleItem item;
            item.courseName = itemObj["courseName"].toString().toStdString();
            item.raw_id = itemObj["raw_id"].toString().toStdString();
            item.type = itemObj["type"].toString().toStdString();
            item.start = itemObj["start"].toString().toStdString();
            item.end = itemObj["end"].toString().toStdString();
            item.building = itemObj["building"].toString().toStdString();
            item.room = itemObj["room"].toString().toStdString();

            scheduleDay.day_items.push_back(item);
        }

        schedule.week.push_back(scheduleDay);
    }

    return schedule;
}

string DatabaseJsonHelpers::courseToJson(const Course& course) {
    QJsonObject courseObj;
    courseObj["id"] = course.id;
    courseObj["raw_id"] = QString::fromStdString(course.raw_id);
    courseObj["name"] = QString::fromStdString(course.name);
    courseObj["teacher"] = QString::fromStdString(course.teacher);
    courseObj["semester"] = course.semester;  // Add semester field
    courseObj["lectures"] = QJsonDocument::fromJson(QByteArray::fromStdString(groupsToJson(course.Lectures))).array();
    courseObj["tutorials"] = QJsonDocument::fromJson(QByteArray::fromStdString(groupsToJson(course.Tirgulim))).array();
    courseObj["labs"] = QJsonDocument::fromJson(QByteArray::fromStdString(groupsToJson(course.labs))).array();
    courseObj["blocks"] = QJsonDocument::fromJson(QByteArray::fromStdString(groupsToJson(course.blocks))).array();

    QJsonDocument doc(courseObj);
    return doc.toJson(QJsonDocument::Compact).toStdString();
}

Course DatabaseJsonHelpers::courseFromJson(const string& json, int id, const string& raw_id,
                                           const string& name, const string& teacher, int semester) {
    Course course;
    course.id = id;
    course.raw_id = raw_id;
    course.name = name;
    course.teacher = teacher;
    course.semester = semester;  // Add semester field

    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    if (!doc.isObject()) return course;

    QJsonObject courseObj = doc.object();

    // Parse groups from JSON arrays
    if (courseObj.contains("lectures")) {
        QJsonDocument lecturesDoc(courseObj["lectures"].toArray());
        course.Lectures = groupsFromJson(lecturesDoc.toJson(QJsonDocument::Compact).toStdString());
    }

    if (courseObj.contains("tutorials")) {
        QJsonDocument tutorialsDoc(courseObj["tutorials"].toArray());
        course.Tirgulim = groupsFromJson(tutorialsDoc.toJson(QJsonDocument::Compact).toStdString());
    }

    if (courseObj.contains("labs")) {
        QJsonDocument labsDoc(courseObj["labs"].toArray());
        course.labs = groupsFromJson(labsDoc.toJson(QJsonDocument::Compact).toStdString());
    }

    if (courseObj.contains("blocks")) {
        QJsonDocument blocksDoc(courseObj["blocks"].toArray());
        course.blocks = groupsFromJson(blocksDoc.toJson(QJsonDocument::Compact).toStdString());
    }

    return course;
}