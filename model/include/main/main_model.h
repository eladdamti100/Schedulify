#ifndef MAIN_MODEL_H
#define MAIN_MODEL_H

#include "model_interfaces.h"
#include "excel_parser.h"
#include "model_access.h"
#include "ScheduleBuilder.h"
#include "parseCoursesToVector.h"
#include "printSchedule.h"
#include "parseToCsv.h"
#include "logger.h"
#include "excel_parser.h"
#include "validate_courses.h"
#include "jsonParser.h"
#include "model_db_integration.h"
#include "db_manager.h"
#include "sql_validator.h"
#include "claude_api_integration.h"
#include "schedule_filter_service.h"
#include "cleanup_manager.h"

#include <algorithm>
#include <cctype>
#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <set>
#include <QDateTime>
#include <QSqlQuery>

using std::string;
using std::cout;
using std::vector;
using std::map;
using std::set;

class Model : public IModel {
public:
    static Model& getInstance() {
        static Model instance;
        return instance;
    }
    void* executeOperation(ModelOperation operation, const void* data, const string& path) override;
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

private:
    Model() {}

    // Courses and files management
    static vector<Course> generateCourses(const string& path);
    static vector<Course> loadCoursesFromHistory(const vector<int>& fileIds);
    static vector<FileEntity> getFileHistory();
    static bool deleteFileFromHistory(int fileId);
    static vector<string> validateCourses(const vector<Course>& courses);

    // Schedule generator
    static vector<InformativeSchedule> generateSchedules(const vector<Course>& userInput, const string& semester);
    static bool saveSchedulesToDB(const vector<InformativeSchedule>& schedules, const string& semester);

    // Schedule export
    static void saveSchedule(const InformativeSchedule& infoSchedule, const string& path);
    static void printSchedule(const InformativeSchedule& infoSchedule);

    // Bot handler
    static BotQueryResponse processClaudeQuery(const BotQueryRequest& request);
    static void setLastFilteredScheduleIds(const vector<int>& ids);
    static vector<int> getLastFilteredScheduleIds();
    static void setLastFilteredUniqueIds(const vector<string>& uniqueIds);
    static vector<string> getLastFilteredUniqueIds();
    static vector<int> convertUniqueIdsToScheduleIndices(const vector<string>& uniqueIds, const string& semester);
    static vector<string> convertScheduleIndicesToUniqueIds(const vector<int>& indices, const string& semester);


    static mutex dataAccessMutex;
    static vector<int> lastFilteredScheduleIds;
    static vector<string> lastFilteredUniqueIds;
    static vector<Course> lastGeneratedCourses;
    static vector<InformativeSchedule> lastGeneratedSchedules;
    static map<string, vector<InformativeSchedule>> semesterSchedules;
};

inline IModel* getModel() {
    return &Model::getInstance();
}

#endif //MAIN_MODEL_H