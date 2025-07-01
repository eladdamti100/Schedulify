#include "main_model.h"

vector<int> Model::lastFilteredScheduleIds;
vector<Course> Model::lastGeneratedCourses;
vector<InformativeSchedule> Model::lastGeneratedSchedules;

// main model menu
void* Model::executeOperation(ModelOperation operation, const void* data, const string& path) {
    switch (operation) {
        case ModelOperation::GENERATE_COURSES: {
            if (!path.empty()) {
                lastGeneratedCourses = generateCourses(path);
                return &lastGeneratedCourses;
            } else {
                Logger::get().logError("File not found, aborting...");
                return nullptr;
            }
        }

        case ModelOperation::LOAD_FROM_HISTORY: {
            if (data) {
                const auto* fileLoadData = static_cast<const FileLoadData*>(data);
                lastGeneratedCourses = loadCoursesFromHistory(fileLoadData->fileIds);
                return &lastGeneratedCourses;
            } else {
                Logger::get().logError("No file IDs provided for history loading");
                return nullptr;
            }
        }

        case ModelOperation::GET_FILE_HISTORY: {
            auto* fileHistory = new vector<FileEntity>(getFileHistory());
            return fileHistory;
        }

        case ModelOperation::DELETE_FILE_FROM_HISTORY: {
            if (data) {
                const int* fileId = static_cast<const int*>(data);
                bool success = deleteFileFromHistory(*fileId);
                bool* result = new bool(success);
                return result;
            } else {
                Logger::get().logError("No file ID provided for deletion");
                return nullptr;
            }
        }

        case ModelOperation::VALIDATE_COURSES: {
            if (data) {
                const auto* courses = static_cast<const vector<Course>*>(data);
                auto* validationResult = new vector<string>(validateCourses(*courses));
                return validationResult;
            } else {
                Logger::get().logError("No courses were found for validation, aborting...");
                return nullptr;
            }
        }

        case ModelOperation::GENERATE_SCHEDULES: {
            if (data) {
                const auto* courses = static_cast<const vector<Course>*>(data);

                lastGeneratedSchedules = generateSchedules(*courses, path);
                return &lastGeneratedSchedules;
            } else {
                Logger::get().logError("unable to generate schedules, aborting...");
                return nullptr;
            }
        }

        case ModelOperation::SAVE_SCHEDULE: {
            if (data && !path.empty()) {
                const auto* schedule = static_cast<const InformativeSchedule*>(data);
                saveSchedule(*schedule, path);
            } else {
                Logger::get().logError("unable to save schedule, aborting...");
            }
            break;
        }

        case ModelOperation::PRINT_SCHEDULE: {
            if (data) {
                const auto* schedule = static_cast<const InformativeSchedule*>(data);
                printSchedule(*schedule);
            } else {
                Logger::get().logError("unable to print schedule, aborting...");
            }
            break;
        }

        case ModelOperation::BOT_QUERY_SCHEDULES: {
            if (data) {
                const auto* queryRequest = static_cast<const BotQueryRequest*>(data);
                auto* response = new BotQueryResponse();

                try {
                    *response = processClaudeQuery(*queryRequest);

                    return response;

                } catch (const std::exception& e) {
                    Logger::get().logError("Exception in demo bot query processing: " + std::string(e.what()));
                    response->hasError = true;
                    response->errorMessage = "An error occurred while processing your demo request";
                    response->isFilterQuery = false;
                    return response;
                }

            } else {
                Logger::get().logError("No bot query request provided");
                return nullptr;
            }
        }

        case ModelOperation::GET_LAST_FILTERED_IDS: {
            auto* result = new std::vector<int>(lastFilteredScheduleIds);
            return result;
        }

        case ModelOperation::CLEAN_SCHEDULES: {
            CleanupManager::performCleanup();
            break;
        }
    }
    return nullptr;
}

// Manage files and courses

vector<Course> Model::generateCourses(const string& path) {
    vector<Course> courses;
    Logger::get().startCollecting();

    try {
        auto& dbIntegration = ModelDatabaseIntegration::getInstance();
        if (!dbIntegration.isInitialized()) {
            if (!dbIntegration.initializeDatabase()) {
                Logger::get().logError("Failed to initialize database - proceeding without persistence");
            } else {
                Logger::get().logInfo("Database initialized successfully");
            }
        }

        std::string extension = getFileExtension(path);

        if (extension == "xlsx") {
            Logger::get().logInfo("Parsing Excel file: " + path);
            ExcelCourseParser excelParser;
            courses = excelParser.parseExcelFile(path);
        }
        else if (extension == "txt") {
            Logger::get().logInfo("Parsing text file: " + path);
            courses = parseCourseDB(path);
        }
        else {
            Logger::get().logError("Unsupported file format: " + extension + ". Supported formats: .txt, .xlsx");
            Logger::get().stopCollecting();
            return courses;
        }

        if (courses.empty()) {
            Logger::get().logError("Error while parsing input data from file: " + path + ". No courses found.");
        } else {
            Logger::get().logInfo("Successfully parsed " + std::to_string(courses.size()) + " courses from " + path);

            size_t lastSlash = path.find_last_of("/\\");
            string fileName = (lastSlash != string::npos) ? path.substr(lastSlash + 1) : path;
            const string& fileType = extension;

            if (dbIntegration.isInitialized()) {
                try {
                    auto& db = DatabaseManager::getInstance();
                    int fileId = db.files()->insertFile(fileName, fileType);

                    if (fileId > 0) {
                        // Set uniqid for each course
                        for (auto& course : courses) {
                            course.generateUniqueId(fileId);
                        }

                        // Now insert courses with uniqid set
                        if (db.courses()->insertCourses(courses, fileId)) {
                            Logger::get().logInfo("SUCCESS: Courses with uniqid saved to database");
                            Logger::get().logInfo("- File: " + fileName + " (type: " + fileType + ")");
                            Logger::get().logInfo("- Courses: " + std::to_string(courses.size()) + " courses with unique IDs");
                        } else {
                            Logger::get().logWarning("Failed to load courses into database, continuing without persistence");
                        }
                    }
                } catch (const std::exception& e) {
                    Logger::get().logWarning("Database error while loading courses: " + string(e.what()));
                    Logger::get().logWarning("Continuing without database persistence");
                }
            } else {
                Logger::get().logWarning("Database not initialized - continuing without persistence");
            }
        }

    } catch (const std::exception& e) {
        Logger::get().logError("Exception during parsing: " + string(e.what()));
    }

    Logger::get().logInfo(std::to_string(courses.size()) + " courses loaded with unique identifiers");
    return courses;
}

vector<Course> Model::loadCoursesFromHistory(const vector<int>& fileIds) {
    vector<Course> courses;
    vector<string> warnings;

    Logger::get().startCollecting();

    try {
        if (fileIds.empty()) {
            Logger::get().logError("No file IDs provided for loading from history");
            Logger::get().stopCollecting();
            return courses;
        }

        auto& dbIntegration = ModelDatabaseIntegration::getInstance();
        if (!dbIntegration.isInitialized()) {
            Logger::get().logInfo("Initializing database for history loading");
            if (!dbIntegration.initializeDatabase()) {
                Logger::get().logError("Failed to initialize database for history loading");
                Logger::get().stopCollecting();
                return courses;
            }
        }

        string fileIdsList;
        for (size_t i = 0; i < fileIds.size(); ++i) {
            if (i > 0) fileIdsList += ", ";
            fileIdsList += std::to_string(fileIds[i]);
        }

        // Use conflict-aware loading
        courses = dbIntegration.getCoursesByFileIds(fileIds, warnings);

        Logger::get().logInfo("File IDs requested: [" + fileIdsList + "]");
        Logger::get().logInfo("Unique courses loaded: " + std::to_string(courses.size()));
        Logger::get().logInfo("Conflicts resolved: " + std::to_string(warnings.size()));

        if (!warnings.empty()) {
            for (const string& warning : warnings) {
                Logger::get().logWarning(warning);
            }
        }

        // Validate that we have unique course keys
        set<string> uniqueKeys;
        for (const Course& course : courses) {
            string key = course.getCourseKey();
            if (uniqueKeys.find(key) != uniqueKeys.end()) {
                Logger::get().logError("DUPLICATE COURSE KEY DETECTED: " + key);
            } else {
                uniqueKeys.insert(key);
            }
        }

        Logger::get().logInfo("Final validation: " + std::to_string(uniqueKeys.size()) +
                              " unique course keys (should match course count)");

    } catch (const std::exception& e) {
        Logger::get().logError("Exception during loading from history: " + string(e.what()));
        courses.clear();
    }

    return courses;
}

vector<FileEntity> Model::getFileHistory() {
    try {
        auto& dbIntegration = ModelDatabaseIntegration::getInstance();
        if (!dbIntegration.isInitialized()) {
            Logger::get().logInfo("Initializing database for file history");
            if (!dbIntegration.initializeDatabase()) {
                Logger::get().logError("Failed to initialize database for file history");
                return {};
            }
        }

        auto& db = DatabaseManager::getInstance();
        if (!db.isConnected()) {
            Logger::get().logError("Database connection lost - cannot retrieve file history");
            return {};
        }

        auto files = dbIntegration.getAllFiles();
        Logger::get().logInfo("Retrieved " + std::to_string(files.size()) + " files from history");

        if (files.empty()) {
            Logger::get().logInfo("No files found in database - this is normal for first use");
        }
        return files;

    } catch (const std::exception& e) {
        Logger::get().logError("Exception during file history retrieval: " + string(e.what()));
        return {};
    }
}

bool Model::deleteFileFromHistory(int fileId) {
    try {
        auto& dbIntegration = ModelDatabaseIntegration::getInstance();
        if (!dbIntegration.isInitialized()) {
            Logger::get().logInfo("Initializing database for file deletion");
            if (!dbIntegration.initializeDatabase()) {
                Logger::get().logError("Failed to initialize database for file deletion");
                return false;
            }
        }

        auto& db = DatabaseManager::getInstance();
        if (!db.isConnected()) {
            Logger::get().logError("Database not connected for file deletion");
            return false;
        }

        FileEntity file = db.files()->getFileById(fileId);
        if (file.id != 0) {
            int courseCount = db.courses()->getCourseCountByFileId(fileId);

            DatabaseTransaction transaction(db);

            if (!db.courses()->deleteCoursesByFileId(fileId)) {
                Logger::get().logError("Failed to delete courses for file ID: " + std::to_string(fileId));
                return false;
            }

            if (!db.files()->deleteFile(fileId)) {
                Logger::get().logError("Failed to delete file record for ID: " + std::to_string(fileId));
                return false;
            }

            if (!transaction.commit()) {
                Logger::get().logError("Failed to commit file deletion transaction");
                return false;
            }

            Logger::get().logInfo("Successfully deleted file '" + file.file_name + "' and " +
                                  std::to_string(courseCount) + " associated courses");
            return true;
        } else {
            Logger::get().logError("File with ID " + std::to_string(fileId) + " not found");
            return false;
        }

    } catch (const std::exception& e) {
        Logger::get().logError("Exception during file deletion: " + string(e.what()));
        return false;
    }
}

vector<string> Model::validateCourses(const vector<Course>& courses) {
    if (courses.empty()) {
        Logger::get().logError("No courses were found to validate");
        Logger::get().stopCollecting();
        return {};
    }

    Logger::get().logInfo("Validating " + std::to_string(courses.size()) + " courses");

    vector<string> validationErrors = validate_courses(courses);
    vector<string> allCollectedMessages = Logger::get().getAllCollectedMessages();

    for (const auto& error : validationErrors) {
        allCollectedMessages.push_back("[Validation] " + error);
    }

    Logger::get().stopCollecting();
    Logger::get().clearCollected();

    return allCollectedMessages;
}

// Manage schedules

vector<InformativeSchedule> Model::generateSchedules(const vector<Course>& userInput, const string& semester) {
    if (userInput.empty() || userInput.size() > 8) {
        Logger::get().logError("invalid amount of courses (" + std::to_string(userInput.size()) + "), aborting...");
        return {};
    }

    Logger::get().logInfo("Generating schedules for " + std::to_string(userInput.size()) + " courses");

    bool enableProgressiveWriting = true;

    ScheduleBuilder builder;
    vector<InformativeSchedule> schedules;

    try {
        auto &dbIntegration = ModelDatabaseIntegration::getInstance();
        if (!dbIntegration.isInitialized()) {
            if (!dbIntegration.initializeDatabase()) {
                Logger::get().logWarning("Database not available - proceeding without progressive writing");
                enableProgressiveWriting = false;
            }
        }
    } catch (const std::exception &e) {
        Logger::get().logWarning("Database error - proceeding without progressive writing: " + string(e.what()));
        enableProgressiveWriting = false;
    }

    if (enableProgressiveWriting) {
        string setName = "Generated Schedules - " + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toStdString();
        vector<int> sourceFileIds;
        schedules = builder.build(userInput, extractSemester(semester));

        auto& db = DatabaseManager::getInstance();
        if (db.isConnected()) {
            int savedCount = db.schedules()->getScheduleCount();
            Logger::get().logInfo("Schedules in database after generation: " + std::to_string(savedCount));
        }
    } else {
        schedules = builder.build(userInput, false);

        // FALLBACK: Manual save if progressive writing failed
        if (!schedules.empty()) {
            Logger::get().logInfo("Manually saving " + std::to_string(schedules.size()) + " schedules to database");
            saveSchedulesToDB(schedules);
        }
    }

    if (schedules.empty()) {
        Logger::get().logError("unable to generate schedules, aborting process");
        return schedules;
    }

    Logger::get().logInfo("Generated " + std::to_string(schedules.size()) + " possible schedules");

    return schedules;
}

BotQueryResponse Model::processClaudeQuery(const BotQueryRequest& request) {
    BotQueryResponse response;

    try {
        auto& dbIntegration = ModelDatabaseIntegration::getInstance();
        if (!dbIntegration.isInitialized()) {
            if (!dbIntegration.initializeDatabase()) {
                Logger::get().logError("Database not available for processing request");
                response.hasError = true;
                response.errorMessage = "Database not available for processing your request";
                return response;
            }
        }

        auto& db = DatabaseManager::getInstance();
        if (!db.isConnected()) {
            Logger::get().logError("Database not connected");
            response.hasError = true;
            response.errorMessage = "Database connection unavailable";
            return response;
        }

        // Create enhanced request (no logging of metadata length)
        BotQueryRequest enhancedRequest = request;
        enhancedRequest.scheduleMetadata = db.schedules()->getSchedulesMetadataForBot();

        // Try Claude API
        ClaudeAPIClient claudeClient;
        response = claudeClient.processScheduleQuery(enhancedRequest);

        cout << response.sqlQuery << endl;
        for (const string& param : response.queryParameters) {
            cout << param << endl;
        }

        // Handle rate limiting with fallback (minimal logging)
        if (response.hasError &&
            (response.errorMessage.find("overloaded") != std::string::npos ||
             response.errorMessage.find("rate limit") != std::string::npos ||
             response.errorMessage.find("429") != std::string::npos ||
             response.errorMessage.find("529") != std::string::npos)) {

            Logger::get().logWarning("Claude API overloaded - using fallback");
            response = ClaudeAPIClient::generateFallbackResponse(enhancedRequest);

            if (!response.hasError) {
                response.userMessage = "⚠️ Claude API is currently busy, using simplified pattern matching.\n\n" + response.userMessage;
            }
        }

        if (response.hasError) {
            Logger::get().logError("Claude processing failed: " + response.errorMessage);
            return response;
        }

        // Execute SQL filter if needed (minimal logging)
        if (response.isFilterQuery && !response.sqlQuery.empty()) {
            SQLValidator::ValidationResult validation = SQLValidator::validateScheduleQuery(response.sqlQuery);
            if (!validation.isValid) {
                Logger::get().logError("Generated query failed validation: " + validation.errorMessage);
                response.hasError = true;
                response.errorMessage = "Generated query failed security validation: " + validation.errorMessage;
                return response;
            }

            vector<int> allMatchingIds = db.schedules()->executeCustomQuery(response.sqlQuery, response.queryParameters);

            // Filter to available IDs
            vector<int> filteredIds;
            std::set<int> availableSet(request.availableScheduleIds.begin(), request.availableScheduleIds.end());

            for (int scheduleId : allMatchingIds) {
                if (availableSet.find(scheduleId) != availableSet.end()) {
                    filteredIds.push_back(scheduleId);
                }
            }

            lastFilteredScheduleIds = filteredIds;

            // Update response message
            if (filteredIds.empty()) {
                response.userMessage += "\n\n❌ No schedules match your criteria in the current set.";
                if (response.sqlQuery.find("earliest_start") != std::string::npos) {
                    response.userMessage += "\n\n💡 Tip: Try 'start after 9 AM' or 'start after 8 AM' for more results.";
                }
            } else {
                response.userMessage += "\n\n✅ Filter applied! Showing " +
                                        std::to_string(filteredIds.size()) + " of " +
                                        std::to_string(request.availableScheduleIds.size()) +
                                        " schedules that match your criteria.";
            }
        } else {
            // Non-filter query - return all available schedules
            lastFilteredScheduleIds = request.availableScheduleIds;
        }

        return response;

    } catch (const std::exception& e) {
        Logger::get().logError("Exception in Claude query processing: " + std::string(e.what()));
        response.hasError = true;
        response.errorMessage = "An error occurred while processing your request: " + std::string(e.what());
        return response;
    }
}

void Model::saveSchedule(const InformativeSchedule& infoSchedule, const string& path) {
    bool status = saveScheduleToCsv(path, infoSchedule);
    string message = status ? "Schedule saved to CSV: " + path : "An error has occurred, unable to save schedule as csv";
    Logger::get().logInfo(message);
}

void Model::printSchedule(const InformativeSchedule& infoSchedule) {
    bool status = printSelectedSchedule(infoSchedule);
    string message = status ? "Schedule sent to printer" : "An error has occurred, unable to print schedule";
    Logger::get().logInfo(message);
}

bool Model::saveSchedulesToDB(const vector<InformativeSchedule>& schedules) {
    try {
        auto& dbIntegration = ModelDatabaseIntegration::getInstance();
        if (!dbIntegration.isInitialized()) {
            if (!dbIntegration.initializeDatabase()) {
                Logger::get().logError("Failed to initialize database for schedule saving");
                return false;
            }
        }
        return dbIntegration.saveSchedulesToDatabase(schedules);
    } catch (const std::exception& e) {
        Logger::get().logError("Exception saving schedules to database: " + string(e.what()));
        return false;
    }
}

// Helpers methods

string Model::getFileExtension(const string& filename) {
    size_t dot = filename.find_last_of('.');
    if (dot == std::string::npos) {
        return "";
    }
    std::string ext = filename.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

int Model::extractSemester(const string& semester) {
    if (semester == "A") {
        return 1;
    } else if (semester == "B") {
        return 2;
    } else if (semester == "SUMMER") {
        return 3;
    } else {
        return 0;
    }
}

