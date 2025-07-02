#include "schedule_model.h"
#include <set>

ScheduleModel::ScheduleModel(QObject *parent)
        : QObject(parent), m_currentScheduleIndex(0), m_isFiltered(false) {
}

void ScheduleModel::loadSchedules(const std::vector<InformativeSchedule>& schedules) {
    m_allSchedules = schedules;
    m_filteredSchedules = schedules;
    m_isFiltered = false;

    updateUniqueIdMappings();
    m_currentScheduleIndex = 0;
    notifyDataChanged();
    emit currentScheduleIndexChanged();
}

void ScheduleModel::setCurrentScheduleIndex(int index) {
    const auto& activeSchedules = getActiveSchedules();

    int clampedIndex = 0;
    if (!activeSchedules.empty()) {
        clampedIndex = qBound(0, index, static_cast<int>(activeSchedules.size()) - 1);
    }

    if (m_currentScheduleIndex != clampedIndex || activeSchedules.empty()) {
        m_currentScheduleIndex = clampedIndex;
    }
    emit currentScheduleIndexChanged();
}

int ScheduleModel::scheduleCount() const {
    int count = static_cast<int>(getActiveSchedules().size());
    return count;
}

QVariantList ScheduleModel::filteredScheduleIds() const {
    QVariantList ids;
    for (int id : m_filteredIds) {
        ids.append(id);
    }
    return ids;
}

QVariantList ScheduleModel::getDayItems(int scheduleIndex, int dayIndex) const {
    const auto& activeSchedules = getActiveSchedules();

    if (scheduleIndex < 0 || scheduleIndex >= static_cast<int>(activeSchedules.size()))
        return {};

    if (dayIndex < 0 || dayIndex >= static_cast<int>(activeSchedules[scheduleIndex].week.size()))
        return {};

    QVariantList items;
    for (const auto &item : activeSchedules[scheduleIndex].week[dayIndex].day_items) {
        QVariantMap itemMap;
        itemMap["courseName"] = QString::fromStdString(item.courseName);
        itemMap["raw_id"] = QString::fromStdString(item.raw_id);
        itemMap["type"] = QString::fromStdString(item.type);
        itemMap["start"] = QString::fromStdString(item.start);
        itemMap["end"] = QString::fromStdString(item.end);
        itemMap["building"] = QString::fromStdString(item.building);
        itemMap["room"] = QString::fromStdString(item.room);
        items.append(itemMap);
    }
    return items;
}

QVariantList ScheduleModel::getCurrentDayItems(int dayIndex) const {
    return getDayItems(m_currentScheduleIndex, dayIndex);
}

void ScheduleModel::nextSchedule() {
    if (canGoNext()) {
        setCurrentScheduleIndex(m_currentScheduleIndex + 1);
    }
}

void ScheduleModel::previousSchedule() {
    if (canGoPrevious()) {
        setCurrentScheduleIndex(m_currentScheduleIndex - 1);
    }
}

bool ScheduleModel::canGoNext() const {
    const auto& activeSchedules = getActiveSchedules();
    return m_currentScheduleIndex < static_cast<int>(activeSchedules.size()) - 1 && !activeSchedules.empty();
}

bool ScheduleModel::canGoPrevious() const {
    return m_currentScheduleIndex > 0 && !getActiveSchedules().empty();
}

void ScheduleModel::jumpToSchedule(int userScheduleNumber) {
    int index = userScheduleNumber - 1;

    if (canJumpToSchedule(index)) {
        setCurrentScheduleIndex(index);
    }
}

bool ScheduleModel::canJumpToSchedule(int index) {
    const auto& activeSchedules = getActiveSchedules();
    return index >= 0 && index < static_cast<int>(activeSchedules.size());
}

void ScheduleModel::applyScheduleFilter(const QVariantList& scheduleIds) {

    // Convert schedule indices to unique IDs for filtering
    QStringList uniqueIds;
    for (const QVariant& id : scheduleIds) {
        int scheduleIndex = id.toInt();
        QString uniqueId = getUniqueIdByScheduleIndex(scheduleIndex);
        if (!uniqueId.isEmpty()) {
            uniqueIds.append(uniqueId);
        }
    }

    if (uniqueIds.isEmpty()) {
        qWarning() << "No valid unique IDs found for filtering";
        clearScheduleFilter();
        return;
    }

    // FIXED: Delegate to the unique ID method
    applyScheduleFilterByUniqueIds(QVariantList(uniqueIds.begin(), uniqueIds.end()));

    // FIXED: Also update the old m_filteredIds for backward compatibility
    m_filteredIds.clear();
    for (const QVariant& variant : scheduleIds) {
        bool ok;
        int id = variant.toInt(&ok);
        if (ok) {
            m_filteredIds.push_back(id);
        }
    }
}

void ScheduleModel::clearScheduleFilter() {

    if (!m_isFiltered) {
        return; // Already not filtered
    }

    m_filteredSchedules = m_allSchedules;
    m_filteredUniqueIds = m_allUniqueIds;
    m_filteredIds.clear();
    m_isFiltered = false;

    emit filterStateChanged();
    emit filteredScheduleIdsChanged();
    emit scheduleCountChanged();
    emit scheduleDataChanged();
    emit totalScheduleCountChanged();
}

QVariantList ScheduleModel::getAllScheduleIds() const {
    // Keep this method for backward compatibility, but now it maps to unique IDs
    QVariantList ids;
    for (const auto& schedule : m_allSchedules) {
        ids.append(schedule.index);  // Still return schedule index for compatibility
    }
    return ids;
}

const vector<InformativeSchedule>& ScheduleModel::getCurrentSchedules() const {
    return getActiveSchedules();
}

void ScheduleModel::updateFilteredSchedules() {
    m_filteredSchedules.clear();

    // Create a set for faster lookup
    std::set<int> filterIdSet(m_filteredIds.begin(), m_filteredIds.end());

    for (const auto& schedule : m_allSchedules) {
        if (filterIdSet.find(schedule.index) != filterIdSet.end()) {
            m_filteredSchedules.push_back(schedule);
        }
    }
}

void ScheduleModel::resetCurrentIndex() {
    const auto& activeSchedules = getActiveSchedules();

    if (activeSchedules.empty()) {
        m_currentScheduleIndex = 0;
    } else if (m_currentScheduleIndex >= static_cast<int>(activeSchedules.size())) {
        m_currentScheduleIndex = static_cast<int>(activeSchedules.size()) - 1;
    }

    emit currentScheduleIndexChanged();
}

const vector<InformativeSchedule>& ScheduleModel::getActiveSchedules() const {
    return m_isFiltered ? m_filteredSchedules : m_allSchedules;
}

void ScheduleModel::updateUniqueIdMappings() {
    m_allUniqueIds.clear();
    m_filteredUniqueIds.clear();
    m_uniqueIdToIndex.clear();
    m_indexToUniqueId.clear();

    // Build mappings for all schedules
    for (size_t i = 0; i < m_allSchedules.size(); ++i) {
        QString uniqueId = QString::fromStdString(m_allSchedules[i].unique_id);
        m_allUniqueIds.append(uniqueId);
        m_uniqueIdToIndex[uniqueId] = static_cast<int>(i);
        m_indexToUniqueId[static_cast<int>(i)] = uniqueId;
    }

    // If not filtered, filtered list matches all list
    if (!m_isFiltered) {
        m_filteredUniqueIds = m_allUniqueIds;
    }
}

QVariantList ScheduleModel::getAllScheduleUniqueIds() const {
    QVariantList uniqueIds;
    for (const QString& uniqueId : m_allUniqueIds) {
        uniqueIds.append(uniqueId);
    }
    return uniqueIds;
}

void ScheduleModel::applyScheduleFilterByUniqueIds(const QVariantList& uniqueIds) {

    QStringList filterUniqueIds;
    for (const QVariant& id : uniqueIds) {
        QString uniqueId = id.toString();
        filterUniqueIds.append(uniqueId);
    }

    if (filterUniqueIds.isEmpty()) {
        clearScheduleFilter();
        return;
    }

    rebuildFilteredSchedulesFromUniqueIds(filterUniqueIds);

    m_filteredUniqueIds = filterUniqueIds;
    m_isFiltered = true;
    setCurrentScheduleIndex(0);

    emit filterStateChanged();
    emit filteredScheduleIdsChanged();
    emit scheduleCountChanged();
    emit scheduleDataChanged();
    emit totalScheduleCountChanged();  // Make sure this is emitted too
}

void ScheduleModel::rebuildFilteredSchedulesFromUniqueIds(const QStringList& uniqueIds) {

    m_filteredSchedules.clear();

    // Find schedules by unique ID
    for (const QString& uniqueId : uniqueIds) {
        bool found = false;
        for (const auto& schedule : m_allSchedules) {
            if (QString::fromStdString(schedule.unique_id) == uniqueId) {
                m_filteredSchedules.push_back(schedule);
                found = true;
                break;
            }
        }
        if (!found) {
            qWarning() << "Could not find schedule for unique ID:" << uniqueId;
        }
    }
}

QString ScheduleModel::getCurrentScheduleUniqueId() const {
    const auto& activeSchedules = getActiveSchedules();
    if (m_currentScheduleIndex >= 0 && m_currentScheduleIndex < static_cast<int>(activeSchedules.size())) {
        return QString::fromStdString(activeSchedules[m_currentScheduleIndex].unique_id);
    }
    return QString();
}

int ScheduleModel::getScheduleIndexByUniqueId(const QString& uniqueId) const {
    for (size_t i = 0; i < m_allSchedules.size(); ++i) {
        if (QString::fromStdString(m_allSchedules[i].unique_id) == uniqueId) {
            return m_allSchedules[i].index;  // Return the display index
        }
    }
    return -1;
}

QString ScheduleModel::getUniqueIdByScheduleIndex(int scheduleIndex) const {
    for (const auto& schedule : m_allSchedules) {
        if (schedule.index == scheduleIndex) {
            return QString::fromStdString(schedule.unique_id);
        }
    }
    return QString();
}

QVariant ScheduleModel::getCurrentScheduleData() const {
    const auto& activeSchedules = getActiveSchedules();
    if (m_currentScheduleIndex >= 0 && m_currentScheduleIndex < static_cast<int>(activeSchedules.size())) {
        const auto& schedule = activeSchedules[m_currentScheduleIndex];

        QVariantMap scheduleMap;
        scheduleMap["index"] = schedule.index;
        scheduleMap["unique_id"] = QString::fromStdString(schedule.unique_id);
        scheduleMap["semester"] = QString::fromStdString(schedule.semester);
        scheduleMap["amount_days"] = schedule.amount_days;
        scheduleMap["amount_gaps"] = schedule.amount_gaps;
        scheduleMap["gaps_time"] = schedule.gaps_time;
        scheduleMap["avg_start"] = schedule.avg_start;
        scheduleMap["avg_end"] = schedule.avg_end;

        // Enhanced metrics
        scheduleMap["earliest_start"] = schedule.earliest_start;
        scheduleMap["latest_end"] = schedule.latest_end;
        scheduleMap["longest_gap"] = schedule.longest_gap;
        scheduleMap["total_class_time"] = schedule.total_class_time;
        scheduleMap["consecutive_days"] = schedule.consecutive_days;
        scheduleMap["weekend_classes"] = schedule.weekend_classes;
        scheduleMap["has_morning_classes"] = schedule.has_morning_classes;
        scheduleMap["has_early_morning"] = schedule.has_early_morning;
        scheduleMap["has_evening_classes"] = schedule.has_evening_classes;
        scheduleMap["has_late_evening"] = schedule.has_late_evening;
        scheduleMap["max_daily_hours"] = schedule.max_daily_hours;
        scheduleMap["min_daily_hours"] = schedule.min_daily_hours;
        scheduleMap["avg_daily_hours"] = schedule.avg_daily_hours;
        scheduleMap["has_lunch_break"] = schedule.has_lunch_break;
        scheduleMap["max_daily_gaps"] = schedule.max_daily_gaps;
        scheduleMap["avg_gap_length"] = schedule.avg_gap_length;
        scheduleMap["schedule_span"] = schedule.schedule_span;
        scheduleMap["compactness_ratio"] = schedule.compactness_ratio;
        scheduleMap["weekday_only"] = schedule.weekday_only;

        // Individual weekdays
        scheduleMap["has_monday"] = schedule.has_monday;
        scheduleMap["has_tuesday"] = schedule.has_tuesday;
        scheduleMap["has_wednesday"] = schedule.has_wednesday;
        scheduleMap["has_thursday"] = schedule.has_thursday;
        scheduleMap["has_friday"] = schedule.has_friday;
        scheduleMap["has_saturday"] = schedule.has_saturday;
        scheduleMap["has_sunday"] = schedule.has_sunday;

        return scheduleMap;
    }
    return QVariant();
}

QString ScheduleModel::getDayName(int dayIndex) const {
    const QStringList dayNames = {
            "Sunday", "Monday", "Tuesday", "Wednesday",
            "Thursday", "Friday", "Saturday"
    };

    if (dayIndex >= 0 && dayIndex < dayNames.size()) {
        return dayNames[dayIndex];
    }
    return QString();
}

int ScheduleModel::getCurrentScheduleDisplayNumber() const {
    return m_currentScheduleIndex + 1; // Convert from 0-based to 1-based for display
}

void ScheduleModel::notifyDataChanged() {
    emit scheduleDataChanged();
    emit filterStateChanged();
    emit scheduleCountChanged();
    emit totalScheduleCountChanged();
    emit filteredScheduleIdsChanged();
}