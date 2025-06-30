#include "schedule_model.h"
#include <QDebug>
#include <set>

ScheduleModel::ScheduleModel(QObject *parent)
        : QObject(parent), m_currentScheduleIndex(0), m_isFiltered(false) {
}

void ScheduleModel::loadSchedules(const std::vector<InformativeSchedule>& schedules) {
    m_allSchedules = schedules;

    // Reset filter state when loading new schedules
    clearScheduleFilter();

    emit totalScheduleCountChanged();
    emit scheduleCountChanged();
    emit scheduleDataChanged();
}

void ScheduleModel::setCurrentScheduleIndex(int index) {
    const auto& activeSchedules = getActiveSchedules();

    if (index >= 0 && index < static_cast<int>(activeSchedules.size()) && m_currentScheduleIndex != index) {
        m_currentScheduleIndex = index;
        emit currentScheduleIndexChanged();
    }
}

int ScheduleModel::scheduleCount() const {
    return static_cast<int>(getActiveSchedules().size());
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
    m_filteredIds.clear();

    // Convert QVariantList to vector<int>
    for (const QVariant& variant : scheduleIds) {
        bool ok;
        int id = variant.toInt(&ok);
        if (ok) {
            m_filteredIds.push_back(id);
        } else {
            qWarning() << "Invalid schedule ID in filter list:" << variant.toString();
        }
    }

    if (m_filteredIds.empty()) {
        qWarning() << "No valid schedule IDs provided for filtering";
        clearScheduleFilter();
        return;
    }

    updateFilteredSchedules();

    m_isFiltered = true;
    resetCurrentIndex();

    emit filterStateChanged();
    emit filteredScheduleIdsChanged();
    emit scheduleCountChanged();
    emit scheduleDataChanged();
}

void ScheduleModel::clearScheduleFilter() {
    bool wasFiltered = m_isFiltered;

    m_isFiltered = false;
    m_filteredIds.clear();
    m_filteredSchedules.clear();

    resetCurrentIndex();

    if (wasFiltered) {
        emit filterStateChanged();
        emit filteredScheduleIdsChanged();
        emit scheduleCountChanged();
        emit scheduleDataChanged();

    }
}

QVariantList ScheduleModel::getAllScheduleIds() const {
    QVariantList ids;
    for (const auto& schedule : m_allSchedules) {
        ids.append(schedule.index);
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