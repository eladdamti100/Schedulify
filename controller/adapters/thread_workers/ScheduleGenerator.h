#pragma once
#include "model_access.h"
#include "model_interfaces.h"

#include <QObject>
#include <vector>
#include <utility>


class ScheduleGenerator : public QObject {
Q_OBJECT

public:
    ScheduleGenerator(IModel* model, const vector<Course>& courses, QString  semester);

public slots:
    void generateSchedules();

signals:
    void schedulesGenerated(std::vector<InformativeSchedule>* schedules);

private:
    IModel* modelConnection;
    vector<Course> coursesToProcess;
    QString semesterName;
};