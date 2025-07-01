#include "ScheduleGenerator.h"

ScheduleGenerator::ScheduleGenerator(IModel* modelConn, const vector<Course>& courses, const QString& semester, QObject* parent)
        : QObject(parent),
          modelConnection(modelConn),
          selectedCourses(courses) {
    currentSemester = semester.toStdString();
}

void ScheduleGenerator::generateSchedules() {
    auto* schedulePtr = static_cast<std::vector<InformativeSchedule>*>
    (modelConnection->executeOperation(ModelOperation::GENERATE_SCHEDULES, &selectedCourses, currentSemester));
    emit schedulesGenerated(schedulePtr);
}