#pragma once
#include <QObject>
#include <vector>
#include "model_access.h"
#include "model_interfaces.h"

class ScheduleGenerator : public QObject {
Q_OBJECT

public:
    ScheduleGenerator(IModel* modelConn, const vector<Course>& courses, const QString& semester, QObject* parent = nullptr);

public slots:
    void generateSchedules();

signals:
    void schedulesGenerated(vector<InformativeSchedule>* schedules);

private:
    IModel* modelConnection;
    vector<Course> selectedCourses;
    string currentSemester;
};