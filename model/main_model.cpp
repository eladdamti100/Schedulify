#include "main_model.h"

mutex Model::dataAccessMutex;
vector<int> Model::lastFilteredScheduleIds;
vector<string> Model::lastFilteredUniqueIds;
vector<Course> Model::lastGeneratedCourses;
vector<InformativeSchedule> Model::lastGeneratedSchedules;
map<string, vector<InformativeSchedule>> Model::semesterSchedules;


// main model menu
void* Model::executeOperation(ModelOperation operation, const void* data, const string& path) {
    try {
        switch (operation) {
            case ModelOperation::GENERATE_COURSES: {
                if (!path.empty()) {
                    auto* courses = new vector<Course>(generateCourses(path));
                    if (!courses->empty()) {
                        lock_guard<mutex> lock(dataAccessMutex);
                        lastGeneratedCourses = *courses;
                    }
                    return courses;
                } else {
                    Logger::get().logError("File not found, aborting...");
                    return nullptr;
                }
            }

            case ModelOperation::LOAD_FROM_HISTORY: {
                if (data) {
                    // FIX: Correct cast to FileLoadData
                    const auto* loadData = static_cast<const FileLoadData*>(data);
                    auto* courses = new vector<Course>(loadCoursesFromHistory(loadData->fileIds));
                    if (!courses->empty()) {
                        lock_guard<mutex> lock(dataAccessMutex);
                        lastGeneratedCourses = *courses;
                    }
                    return courses;
                } else {
                    Logger::get().logError("No file IDs provided for history loading");
                    return nullptr;
                }
            }

            case ModelOperation::GET_FILE_HISTORY: {
                // FIX: Return actual file history, not validate courses
                try {
                    auto* fileHistory = new vector<FileEntity>(getFileHistory());
                    return fileHistory;
                } catch (const std::exception& e) {
                    Logger::get().logError("Exception getting file history: " + std::string(e.what()));
                    return nullptr;
                }
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
                    auto* schedules = new vector<InformativeSchedule>(generateSchedules(*courses, path));

                    if (!schedules->empty()) {
                        lock_guard<mutex> lock(dataAccessMutex);
                        lastGeneratedSchedules = *schedules;
                        semesterSchedules[path] = *schedules;
                    }
                    return schedules;
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
                        Logger::get().logError("Exception in bot query processing: " + std::string(e.what()));
                        response->hasError = true;
                        response->errorMessage = "An error occurred while processing your request";
                        response->isFilterQuery = false;
                        return response;
                    }

                } else {
                    Logger::get().logError("No bot query request provided");
                    return nullptr;
                }
            }

            case ModelOperation::GET_LAST_FILTERED_IDS: {
                lock_guard<mutex> lock(dataAccessMutex);
                auto* result = new std::vector<int>(lastFilteredScheduleIds);
                return result;
            }

            case ModelOperation::CLEAN_SCHEDULES: {
                try {
                    CleanupManager::performCleanup();
                } catch (const std::exception& e) {
                    Logger::get().logError("Exception during cleanup: " + std::string(e.what()));
                }
                break;
            }

            case ModelOperation::GET_LAST_FILTERED_UNIQUE_IDS: {
                lock_guard<mutex> lock(dataAccessMutex);
                auto* result = new std::vector<string>(lastFilteredUniqueIds);
                return result;
            }

            case ModelOperation::CONVERT_UNIQUE_IDS_TO_INDICES: {
                if (data) {
                    const auto* request = static_cast<const UniqueIdConversionRequest*>(data);
                    auto* result = new vector<int>(convertUniqueIdsToScheduleIndices(request->uniqueIds, request->semester));
                    return result;
                } else {
                    Logger::get().logError("No conversion request provided");
                    return nullptr;
                }
            }

            case ModelOperation::CONVERT_INDICES_TO_UNIQUE_IDS: {
                if (data) {
                    const auto* request = static_cast<const IndexConversionRequest*>(data);
                    auto* result = new vector<string>(convertScheduleIndicesToUniqueIds(request->indices, request->semester));
                    return result;
                } else {
                    Logger::get().logError("No conversion request provided");
                    return nullptr;
                }
            }
        }
    } catch (const std::exception& e) {
        Logger::get().logError("Exception in executeOperation: " + std::string(e.what()));
        return nullptr;
    }

    return nullptr;
}

// Manage files and courses

std::string getFileExtension(const std::string& filename) {
    size_t dot = filename.find_last_of('.');
    if (dot == std::string::npos) {
        return "";
    }
    std::string ext = filename.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

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

            // Log unique IDs for debugging
            Logger::get().logInfo("Course unique IDs:");
            for (const auto& course : courses) {
                Logger::get().logInfo("  - " + course.getUniqueId() + ": " + course.getDisplayName());
            }

            size_t lastSlash = path.find_last_of("/\\");
            string fileName = (lastSlash != string::npos) ? path.substr(lastSlash + 1) : path;
            const string& fileType = extension;

            if (dbIntegration.isInitialized()) {
                try {
                    if (dbIntegration.loadCoursesToDatabase(courses, fileName, fileType)) {
                        Logger::get().logInfo("SUCCESS: Courses and file metadata saved to database");
                        Logger::get().logInfo("- File: " + fileName + " (type: " + fileType + ")");
                        Logger::get().logInfo("- Courses: " + std::to_string(courses.size()) + " courses linked to file");
                    } else {
                        Logger::get().logWarning("Failed to load courses into database, continuing without persistence");
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

    Logger::get().logInfo(std::to_string(courses.size()) + " courses loaded");
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
        courses = dbIntegration.getCoursesByFileIds(fileIds, warnings);

        Logger::get().logInfo("=== HISTORY LOADING RESULTS ===");
        Logger::get().logInfo("File IDs requested: [" + fileIdsList + "]");
        Logger::get().logInfo("Courses loaded: " + std::to_string(courses.size()));
        Logger::get().logInfo("Conflicts resolved: " + std::to_string(warnings.size()));

        // Log loaded course unique IDs
        Logger::get().logInfo("Loaded course unique IDs:");
        for (const auto& course : courses) {
            Logger::get().logInfo("  - " + course.getUniqueId() + ": " + course.getDisplayName());
        }

        if (!warnings.empty()) {
            for (const string& warning : warnings) {
                Logger::get().logWarning(warning);
            }
        }

        if (courses.empty()) {
            auto& db = DatabaseManager::getInstance();
            if (!db.isConnected()) {
                Logger::get().logError("Database is not connected!");
                return courses;
            }

            for (int fileId : fileIds) {
                FileEntity file = db.files()->getFileById(fileId);
                if (file.id != 0) {
                    vector<Course> fileCourses = db.courses()->getCoursesByFileId(fileId);
                    Logger::get().logInfo("File " + std::to_string(fileId) + " contains " +
                                          std::to_string(fileCourses.size()) + " courses");
                }
            }
        }

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

    Logger::get().logInfo("Generating schedules for " + std::to_string(userInput.size()) +
                          " courses in semester " + semester);

    ScheduleBuilder builder;
    vector<InformativeSchedule> schedules;

    try {
        schedules = builder.build(userInput, semester);

        if (!schedules.empty()) {
            Logger::get().logInfo("Generated " + std::to_string(schedules.size()) +
                                  " schedules for semester " + semester);

            saveSchedulesToDB(schedules, semester);
        }

    } catch (const std::exception& e) {
        Logger::get().logError("Exception during schedule generation: " + string(e.what()));
    }

    return schedules;
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

bool Model::saveSchedulesToDB(const vector<InformativeSchedule>& schedules, const string& semester) {
    try {
        auto& dbIntegration = ModelDatabaseIntegration::getInstance();
        if (!dbIntegration.isInitialized()) {
            if (!dbIntegration.initializeDatabase()) {
                Logger::get().logWarning("Database not available for saving schedules");
                return false;
            }
        }

        auto& db = DatabaseManager::getInstance();
        if (!db.isConnected()) {
            Logger::get().logWarning("Database not connected for saving schedules");
            return false;
        }

        // Use database manager's thread-safe batch method
        bool success = dbIntegration.saveSchedulesToDatabase(schedules);

        if (success) {
            Logger::get().logInfo("Successfully saved " + std::to_string(schedules.size()) +
                                  " schedules for semester " + semester);
        } else {
            Logger::get().logWarning("Failed to save schedules for semester " + semester);
        }

        return success;

    } catch (const std::exception& e) {
        Logger::get().logError("Exception saving schedules to database: " + std::string(e.what()));
        return false;
    }
}

BotQueryResponse Model::processClaudeQuery(const BotQueryRequest& request) {
    try {
        Logger::get().logInfo("Model::processClaudeQuery - Processing request for semester: " + request.semester);

        BotQueryResponse response = ClaudeAPIClient::ActivateBot(request);

        // UPDATED: Handle both unique IDs and schedule indices
        if (response.isFilterQuery) {
            if (!response.filteredUniqueIds.empty()) {
                // NEW: Primary path - use unique IDs
                setLastFilteredUniqueIds(response.filteredUniqueIds);

                // Convert unique IDs to schedule indices for backward compatibility
                vector<int> scheduleIndices = convertUniqueIdsToScheduleIndices(response.filteredUniqueIds, request.semester);
                setLastFilteredScheduleIds(scheduleIndices);
                response.filteredScheduleIds = scheduleIndices;

            } else if (!response.filteredScheduleIds.empty()) {
                // FALLBACK: Support old index-based filtering
                setLastFilteredScheduleIds(response.filteredScheduleIds);

                // Convert indices to unique IDs for consistency
                vector<string> uniqueIds = convertScheduleIndicesToUniqueIds(response.filteredScheduleIds, request.semester);
                setLastFilteredUniqueIds(uniqueIds);
                response.filteredUniqueIds = uniqueIds;
            }
        }

        return response;

    } catch (const std::exception &e) {
        Logger::get().logError("Exception in Model::processClaudeQuery: " + std::string(e.what()));

        BotQueryResponse errorResponse;
        errorResponse.hasError = true;
        errorResponse.errorMessage = "An error occurred while processing your query: " + std::string(e.what());
        errorResponse.isFilterQuery = false;
        return errorResponse;
    }
}

void Model::setLastFilteredScheduleIds(const vector<int>& ids) {
    try {
        lock_guard<mutex> lock(dataAccessMutex);
        lastFilteredScheduleIds = ids;
    } catch (const std::exception& e) {
        Logger::get().logError("Exception in setLastFilteredScheduleIds: " + std::string(e.what()));
    }
}

vector<int> Model::getLastFilteredScheduleIds() {
    try {
        lock_guard<mutex> lock(dataAccessMutex);
        return lastFilteredScheduleIds;
    } catch (const std::exception& e) {
        Logger::get().logError("Exception in getLastFilteredScheduleIds: " + std::string(e.what()));
        return vector<int>(); // Return empty vector on error
    }
}

void Model::setLastFilteredUniqueIds(const vector<string>& uniqueIds) {
    try {
        lock_guard<mutex> lock(dataAccessMutex);
        lastFilteredUniqueIds = uniqueIds;
    } catch (const std::exception& e) {
        Logger::get().logError("Exception in setLastFilteredUniqueIds: " + std::string(e.what()));
    }
}

vector<string> Model::getLastFilteredUniqueIds() {
    try {
        lock_guard<mutex> lock(dataAccessMutex);
        return lastFilteredUniqueIds;
    } catch (const std::exception& e) {
        Logger::get().logError("Exception in getLastFilteredUniqueIds: " + std::string(e.what()));
        return vector<string>();
    }
}

vector<int> Model::convertUniqueIdsToScheduleIndices(const vector<string>& uniqueIds, const string& semester) {
    vector<int> scheduleIndices;

    try {
        auto& dbIntegration = ModelDatabaseIntegration::getInstance();
        if (!dbIntegration.isInitialized()) {
            Logger::get().logError("Database not initialized for unique ID conversion");
            return scheduleIndices;
        }

        auto& db = DatabaseManager::getInstance();
        if (!db.isConnected()) {
            Logger::get().logError("Database not connected for unique ID conversion");
            return scheduleIndices;
        }

        scheduleIndices = db.schedules()->getScheduleIndicesByUniqueIds(uniqueIds);

        Logger::get().logInfo("Converted " + std::to_string(uniqueIds.size()) + " unique IDs to " +
                              std::to_string(scheduleIndices.size()) + " schedule indices");

    } catch (const std::exception& e) {
        Logger::get().logError("Exception converting unique IDs to schedule indices: " + std::string(e.what()));
    }

    return scheduleIndices;
}

vector<string> Model::convertScheduleIndicesToUniqueIds(const vector<int>& indices, const string& semester) {
    vector<string> uniqueIds;

    try {
        auto& dbIntegration = ModelDatabaseIntegration::getInstance();
        if (!dbIntegration.isInitialized()) {
            Logger::get().logError("Database not initialized for index conversion");
            return uniqueIds;
        }

        auto& db = DatabaseManager::getInstance();
        if (!db.isConnected()) {
            Logger::get().logError("Database not connected for index conversion");
            return uniqueIds;
        }

        for (int index : indices) {
            string uniqueId = db.schedules()->getUniqueIdByScheduleIndex(index, semester);
            if (!uniqueId.empty()) {
                uniqueIds.push_back(uniqueId);
            }
        }

        Logger::get().logInfo("Converted " + std::to_string(indices.size()) + " schedule indices to " +
                              std::to_string(uniqueIds.size()) + " unique IDs");

    } catch (const std::exception& e) {
        Logger::get().logError("Exception converting schedule indices to unique IDs: " + std::string(e.what()));
    }

    return uniqueIds;
}
