#ifndef SCHEDULE_MODEL_H
#define SCHEDULE_MODEL_H

#include <QObject>
#include <QVariant>
#include <QVariantList>
#include <QString>
#include <QStringList>
#include <QMap>
#include "model_interfaces.h"

class ScheduleModel : public QObject {
Q_OBJECT
    Q_PROPERTY(int currentScheduleIndex READ currentScheduleIndex
                       WRITE setCurrentScheduleIndex NOTIFY currentScheduleIndexChanged)
    Q_PROPERTY(int scheduleCount READ scheduleCount NOTIFY scheduleCountChanged)
    Q_PROPERTY(bool canGoNext READ canGoNext NOTIFY currentScheduleIndexChanged)
    Q_PROPERTY(bool canGoPrevious READ canGoPrevious NOTIFY currentScheduleIndexChanged)
    Q_PROPERTY(bool isFiltered READ isFiltered NOTIFY filterStateChanged)
    Q_PROPERTY(QVariantList filteredScheduleIds READ filteredScheduleIds NOTIFY filteredScheduleIdsChanged)
    Q_PROPERTY(int totalScheduleCount READ totalScheduleCount NOTIFY totalScheduleCountChanged)

public:
    explicit ScheduleModel(QObject *parent = nullptr);
    ~ScheduleModel() override = default;

    // Schedule management
    void loadSchedules(const vector<InformativeSchedule>& schedules);

    // Properties
    int currentScheduleIndex() const { return m_currentScheduleIndex; }
    Q_INVOKABLE void setCurrentScheduleIndex(int index);
    int scheduleCount() const;
    int totalScheduleCount() const { return static_cast<int>(m_allSchedules.size()); }
    bool isFiltered() const { return m_isFiltered; }
    QVariantList filteredScheduleIds() const;

    // QML accessible methods
    Q_INVOKABLE QVariantList getDayItems(int scheduleIndex, int dayIndex) const;
    Q_INVOKABLE QVariantList getCurrentDayItems(int dayIndex) const;
    Q_INVOKABLE void nextSchedule();
    Q_INVOKABLE void previousSchedule();
    Q_INVOKABLE bool canGoNext() const;
    Q_INVOKABLE bool canGoPrevious() const;
    Q_INVOKABLE bool canJumpToSchedule(int index);
    Q_INVOKABLE void jumpToSchedule(int index);

    // Filtering methods (backward compatibility with indices)
    Q_INVOKABLE void applyScheduleFilter(const QVariantList& scheduleIds);
    Q_INVOKABLE void clearScheduleFilter();
    Q_INVOKABLE QVariantList getAllScheduleIds() const;

    // NEW: Unique ID support methods
    Q_INVOKABLE QVariantList getAllScheduleUniqueIds() const;
    Q_INVOKABLE void applyScheduleFilterByUniqueIds(const QVariantList& uniqueIds);
    Q_INVOKABLE QString getCurrentScheduleUniqueId() const;
    Q_INVOKABLE int getScheduleIndexByUniqueId(const QString& uniqueId) const;
    Q_INVOKABLE QString getUniqueIdByScheduleIndex(int index) const;

    // Get current visible schedules (filtered or all)
    const vector<InformativeSchedule>& getCurrentSchedules() const;

    // Additional utility methods
    Q_INVOKABLE QVariant getCurrentScheduleData() const;
    Q_INVOKABLE QString getDayName(int dayIndex) const;
    Q_INVOKABLE int getCurrentScheduleDisplayNumber() const;

signals:
    void currentScheduleIndexChanged();
    void scheduleCountChanged();
    void scheduleDataChanged();
    void filterStateChanged();
    void filteredScheduleIdsChanged();
    void totalScheduleCountChanged();

private:
    // Schedule data storage
    vector<InformativeSchedule> m_allSchedules;       // All loaded schedules
    vector<InformativeSchedule> m_filteredSchedules;  // Currently visible schedules
    vector<int> m_filteredIds;                        // IDs of filtered schedules (old system)

    // Current state
    int m_currentScheduleIndex;
    bool m_isFiltered;

    // Unique ID mappings
    QStringList m_allUniqueIds;                       // All schedule unique IDs
    QStringList m_filteredUniqueIds;                  // Currently filtered unique IDs
    QMap<QString, int> m_uniqueIdToIndex;             // Maps unique ID to position in m_allSchedules
    QMap<int, QString> m_indexToUniqueId;             // Maps position to unique ID

    // Helper methods
    void updateFilteredSchedules();
    void resetCurrentIndex();
    const vector<InformativeSchedule>& getActiveSchedules() const;

    // Unique ID helper methods
    void updateUniqueIdMappings();
    void rebuildFilteredSchedulesFromUniqueIds(const QStringList& uniqueIds);
    void notifyDataChanged();
};

#endif // SCHEDULE_MODEL_H