#include "ScheduleGenerator.h"

ScheduleGenerator::ScheduleGenerator(IModel* modelConn, const std::vector<Course>& courses, QString semester)
        : modelConnection(modelConn),
          coursesToProcess(courses),
          semesterName(std::move(semester)) {}

void ScheduleGenerator::generateSchedules() {

    std::vector<InformativeSchedule>* schedulePtr = nullptr;

    try {
        if (!modelConnection) {
            qDebug() << "ScheduleGenerator: ERROR - No model connection";
            emit schedulesGenerated(nullptr);
            return;
        }

        if (coursesToProcess.empty()) {
            qDebug() << "ScheduleGenerator: ERROR - No courses to process";
            emit schedulesGenerated(nullptr);
            return;
        }

        // Call model to generate schedules
        schedulePtr = static_cast<std::vector<InformativeSchedule>*>(
                modelConnection->executeOperation(
                        ModelOperation::GENERATE_SCHEDULES,
                        &coursesToProcess,
                        semesterName.toStdString()
                )
        );

        // Emit result (receiver takes ownership)
        emit schedulesGenerated(schedulePtr);

    } catch (const std::exception& e) {

        // Clean up on exception
        if (schedulePtr) {
            delete schedulePtr;
            schedulePtr = nullptr;
        }

        emit schedulesGenerated(nullptr);

    } catch (...) {

        // Clean up on exception
        if (schedulePtr) {
            delete schedulePtr;
            schedulePtr = nullptr;
        }

        emit schedulesGenerated(nullptr);
    }
}