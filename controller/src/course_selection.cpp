#include "main_controller.h"
#include "course_selection.h"

CourseSelectionController::CourseSelectionController(QObject *parent)
        : ControllerManager(parent)
        , m_courseModel(new CourseModel(this))
        , m_selectedCoursesModel(new CourseModel(this))
        , m_filteredCourseModel(new CourseModel(this))
        , m_blocksModel(new CourseModel(this))
        , workerThread(nullptr)
        , validatorThread(nullptr)
        , m_validationInProgress(false)
{
    modelConnection = ModelAccess::getModel();

    // Initialize semester-specific vectors
    selectedCoursesA.clear();
    selectedCoursesB.clear();
    selectedCoursesSummer.clear();
    selectedIndicesA.clear();
    selectedIndicesB.clear();
    selectedIndicesSummer.clear();
}

CourseSelectionController::~CourseSelectionController() {
    cleanupValidatorThread();

    if (workerThread) {
        if (workerThread->isRunning()) {
            workerThread->quit();
            if (!workerThread->wait(3000)) {
                workerThread->terminate();
                workerThread->wait(1000);
            }
        }
        workerThread = nullptr;
    }

    if (modelConnection) {
        modelConnection = nullptr;
    }
}

// Get course semester
QString CourseSelectionController::getCourseSemester(int courseIndex) {
    if (courseIndex < 0 || courseIndex >= static_cast<int>(allCourses.size())) {
        return "";
    }

    const Course& course = allCourses[courseIndex];

    switch (course.semester) {
        case 1: return "A";
        case 2: return "B";
        case 3: return "SUMMER";
        case 4: return "A"; // Year-long courses are treated as A for selection purposes
        default: return "A";
    }
}

// Check if course can be added to semester
bool CourseSelectionController::canAddCourseToSemester(int courseIndex) {
    if (courseIndex < 0 || courseIndex >= static_cast<int>(allCourses.size())) {
        return false;
    }

    // If course is already selected, we can always deselect it
    if (isCourseSelected(courseIndex)) {
        return true;
    }

    const Course& course = allCourses[courseIndex];

    // Check limits for each semester this course belongs to
    if (course.semester == 1 || course.semester == 4) { // Semester A
        if (static_cast<int>(selectedCoursesA.size()) >= 7) {
            return false;
        }
    }

    if (course.semester == 2 || course.semester == 4) { // Semester B
        if (static_cast<int>(selectedCoursesB.size()) >= 7) {
            return false;
        }
    }

    if (course.semester == 3) { // Summer
        if (static_cast<int>(selectedCoursesSummer.size()) >= 7) {
            return false;
        }
    }

    return true;
}

// Update selected courses model for display
void CourseSelectionController::updateSelectedCoursesModel() {
    // For display purposes, we'll show the courses for the currently selected semester
    if (currentSemesterFilter == "A") {
        m_selectedCoursesModel->populateCoursesData(selectedCoursesA);
    } else if (currentSemesterFilter == "B") {
        m_selectedCoursesModel->populateCoursesData(selectedCoursesB);
    } else if (currentSemesterFilter == "SUMMER") {
        m_selectedCoursesModel->populateCoursesData(selectedCoursesSummer);
    } else {
        // For "ALL" view, combine all courses for display
        vector<Course> allSelected;
        allSelected.insert(allSelected.end(), selectedCoursesA.begin(), selectedCoursesA.end());
        allSelected.insert(allSelected.end(), selectedCoursesB.begin(), selectedCoursesB.end());
        allSelected.insert(allSelected.end(), selectedCoursesSummer.begin(), selectedCoursesSummer.end());
        m_selectedCoursesModel->populateCoursesData(allSelected);
    }
}

void CourseSelectionController::setValidationInProgress(bool inProgress) {
    if (m_validationInProgress != inProgress) {
        m_validationInProgress = inProgress;
        emit validationStateChanged();
    }
}

void CourseSelectionController::setValidationErrors(const QStringList& errors) {
    if (m_validationErrors != errors) {
        m_validationErrors = errors;
        emit validationStateChanged();
    }
}

// Clear semester-specific vectors
void CourseSelectionController::initiateCoursesData(const vector<Course>& courses) {
    try {
        if (courses.empty()) {
            Logger::get().logError("Empty courses vector provided");
            setValidationErrors(QStringList{"No courses found in file"});
            return;
        }

        cleanupValidatorThread();
        setValidationInProgress(true);

        allCourses = courses;
        m_courseModel->populateCoursesData(courses);

        filteredCourses = courses;
        filteredIndicesMap.clear();
        for (size_t i = 0; i < courses.size(); ++i) {
            filteredIndicesMap.push_back(static_cast<int>(i));
        }
        m_filteredCourseModel->populateCoursesData(filteredCourses, filteredIndicesMap);

        // Clear all semester-specific vectors
        selectedCoursesA.clear();
        selectedCoursesB.clear();
        selectedCoursesSummer.clear();
        selectedIndicesA.clear();
        selectedIndicesB.clear();
        selectedIndicesSummer.clear();

        updateSelectedCoursesModel();

        userBlockTimes.clear();
        blockTimes.clear();
        updateBlockTimesModel();

        setValidationErrors(QStringList());

        int timeoutMs = std::min(VALIDATION_TIMEOUT_MS,
                                 static_cast<int>(courses.size() * 100 + 10000));

        QTimer::singleShot(100, this, [this, courses, timeoutMs]() {
            validateCourses(courses, timeoutMs);
        });

    } catch (const std::exception& e) {
        Logger::get().logError("Exception in initiateCoursesData: " + std::string(e.what()));
        setValidationInProgress(false);
    } catch (...) {
        Logger::get().logError("Unknown exception in initiateCoursesData");
        setValidationInProgress(false);
    }
}

// Start semester-based generation
void CourseSelectionController::generateSchedules() {
    // Check if any courses are selected
    if (selectedCoursesA.empty() && selectedCoursesB.empty() && selectedCoursesSummer.empty()) {
        emit errorMessage("Please select at least one course");
        return;
    }

    // Reset navigation flag when starting new generation
    hasNavigatedToSchedules = false;

    // Get schedule controller
    auto* schedule_controller = qobject_cast<SchedulesDisplayController*>(findController("schedulesDisplayController"));
    if (schedule_controller) {
        // Clear all existing schedules before starting new generation
        schedule_controller->clearAllSchedules();

        // Reset to Semester A
        schedule_controller->resetToSemesterA();
    }

    // Start with Semester A
    generateSemesterSchedules("A");
}
void CourseSelectionController::checkAndNavigateToSchedules() {
    auto* schedule_controller = qobject_cast<SchedulesDisplayController*>(findController("schedulesDisplayController"));
    if (!schedule_controller) return;

    // Check if we haven't navigated yet and if any semester has schedules
    if (!hasNavigatedToSchedules) {
        bool hasAnySchedules = false;
        QString firstSemesterWithSchedules;

        // Check each semester in order
        if (schedule_controller->hasSchedulesForSemester("A")) {
            hasAnySchedules = true;
            firstSemesterWithSchedules = "A";
        } else if (schedule_controller->hasSchedulesForSemester("B")) {
            hasAnySchedules = true;
            firstSemesterWithSchedules = "B";
        } else if (schedule_controller->hasSchedulesForSemester("SUMMER")) {
            hasAnySchedules = true;
            firstSemesterWithSchedules = "SUMMER";
        }

        if (hasAnySchedules) {
            // Navigate to schedules display
            goToScreen(QUrl(QStringLiteral("qrc:/schedules_display.qml")));
            hasNavigatedToSchedules = true;

            // Switch to the first semester that has schedules
            schedule_controller->switchToSemester(firstSemesterWithSchedules);
        } else {
            // No schedules for any semester
            emit errorMessage("No valid schedules found for any semester");
        }
    }

    // Notify that all processing is done
    schedule_controller->allSemestersGenerated();
}// Handle semester-specific course selection

// Generate schedules for a specific semester
void CourseSelectionController::generateSemesterSchedules(const QString& semester) {
    vector<Course> coursesToProcess;

    // Get schedule controller reference
    auto* schedule_controller = qobject_cast<SchedulesDisplayController*>(findController("schedulesDisplayController"));

    // Get courses for the specified semester
    if (semester == "A") {
        if (selectedCoursesA.empty()) {
            // Mark this semester as finished with no schedules
            if (schedule_controller) {
                schedule_controller->setSemesterFinished("A", false);
                schedule_controller->loadSemesterScheduleData("A", std::vector<InformativeSchedule>());
            }
            // Move to B
            generateSemesterSchedules("B");
            return;
        }
        coursesToProcess = selectedCoursesA;
    } else if (semester == "B") {
        if (selectedCoursesB.empty()) {
            // Mark this semester as finished with no schedules
            if (schedule_controller) {
                schedule_controller->setSemesterFinished("B", false);
                schedule_controller->loadSemesterScheduleData("B", std::vector<InformativeSchedule>());
            }
            // Move to Summer
            generateSemesterSchedules("SUMMER");
            return;
        }
        coursesToProcess = selectedCoursesB;
    } else if (semester == "SUMMER") {
        if (selectedCoursesSummer.empty()) {
            // Mark this semester as finished with no schedules
            if (schedule_controller) {
                schedule_controller->setSemesterFinished("SUMMER", false);
                schedule_controller->loadSemesterScheduleData("SUMMER", std::vector<InformativeSchedule>());
            }
            // All semesters complete - check if we need to navigate
            checkAndNavigateToSchedules();
            return;
        }
        coursesToProcess = selectedCoursesSummer;
    }

    // Add block times if they exist (block times apply to all semesters)
    vector<BlockTime> currentSemesterBlockTimes = getBlockTimesForCurrentSemester(semester);
    if (!currentSemesterBlockTimes.empty()) {
        Course blockCourse = createSingleBlockTimeCourseForSemester(currentSemesterBlockTimes, semester);
        coursesToProcess.push_back(blockCourse);
    }

    // Set loading state before starting generation
    if (schedule_controller) {
        schedule_controller->setSemesterLoading(semester, true);
        schedule_controller->setSemesterFinished(semester, false);
    }

    // Create a worker thread for the operation
    workerThread = new QThread();
    auto* worker = new ScheduleGenerator(modelConnection, coursesToProcess);
    worker->moveToThread(workerThread);

    // Connect signals/slots with semester information
    connect(workerThread, &QThread::started, worker, &ScheduleGenerator::generateSchedules);
    connect(worker, &ScheduleGenerator::schedulesGenerated, this,
            [this, semester](vector<InformativeSchedule>* schedules) {
                onSemesterSchedulesGenerated(semester, schedules);
            });
    connect(worker, &ScheduleGenerator::schedulesGenerated, workerThread, &QThread::quit);
    connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

    // Start thread
    workerThread->start();

    // Show loading overlay
    auto* engine = qobject_cast<QQmlApplicationEngine*>(getEngine());
    if (engine && !engine->rootObjects().isEmpty()) {
        QTimer::singleShot(100, this, [this, engine, semester]() {
            if (workerThread && workerThread->isRunning()) {
                QObject* rootObject = engine->rootObjects().first();
                if (rootObject) {
                    // Show loading with semester info
                    QMetaObject::invokeMethod(rootObject, "showLoadingOverlay",
                                              Q_ARG(QVariant, QVariant(true)));
                }
            }
        });
    }
}// Handle completion of semester schedule generation
void CourseSelectionController::onSemesterSchedulesGenerated(const QString& semester, vector<InformativeSchedule>* schedules) {
    // Hide loading overlay
    auto* engine = qobject_cast<QQmlApplicationEngine*>(getEngine());
    if (engine && !engine->rootObjects().isEmpty()) {
        QObject* rootObject = engine->rootObjects().first();
        QMetaObject::invokeMethod(rootObject, "showLoadingOverlay",
                                  Q_ARG(QVariant, QVariant(false)));
    }

    // Get schedule controller reference
    auto* schedule_controller = qobject_cast<SchedulesDisplayController*>(findController("schedulesDisplayController"));

    // Always clear loading state, regardless of success/failure
    if (schedule_controller) {
        schedule_controller->setSemesterLoading(semester, false);
    }

    // Check if schedules were generated successfully
    if (!schedules || schedules->empty()) {
        // Do NOT mark as finished if no schedules were generated
        if (schedule_controller) {
            schedule_controller->setSemesterFinished(semester, false);
            schedule_controller->loadSemesterScheduleData(semester, std::vector<InformativeSchedule>());
        }

        // Only show error message if this is the current semester we're trying to view
        if (semester == "A" || schedule_controller->getCurrentSemester() == semester) {
            emit errorMessage(QString("No valid schedules found for semester %1").arg(semester));
        } else {
            Logger::get().logWarning("No valid schedules found for semester " + semester.toStdString());
        }

        // Continue to next semester even if current one failed
        if (semester == "A") {
            generateSemesterSchedules("B");
        } else if (semester == "B") {
            generateSemesterSchedules("SUMMER");
        } else if (semester == "SUMMER") {
            // All semesters complete - check if we need to navigate
            checkAndNavigateToSchedules();
        }

        workerThread = nullptr;
        return;
    }

    // SUCCESS CASE: Schedules were generated
    emit semesterSchedulesGenerated(semester, schedules);

    // Load the schedule data - this will automatically set finished state to true
    if (schedule_controller) {
        schedule_controller->loadSemesterScheduleData(semester, *schedules);

        // Navigate to schedules display if this is the FIRST semester with schedules
        // Check if we haven't navigated yet
        if (!hasNavigatedToSchedules) {
            goToScreen(QUrl(QStringLiteral("qrc:/schedules_display.qml")));
            hasNavigatedToSchedules = true;

            // Also set this semester as the current one to display
            schedule_controller->switchToSemester(semester);
        }
    }

    // Continue to next semester
    if (semester == "A") {
        generateSemesterSchedules("B");
    } else if (semester == "B") {
        generateSemesterSchedules("SUMMER");
    } else if (semester == "SUMMER") {
        // All semesters complete
        if (schedule_controller) {
            schedule_controller->allSemestersGenerated();
        }
    }

    workerThread = nullptr;
}

// Add this new method:
void CourseSelectionController::toggleCourseSelection(int index) {
    if (index < 0 || index >= static_cast<int>(allCourses.size())) {
        Logger::get().logError("Invalid selected course index");
        return;
    }

    const Course& course = allCourses[index];

    // Determine which semester vectors this course belongs to
    bool belongsToA = (course.semester == 1 || course.semester == 4);
    bool belongsToB = (course.semester == 2 || course.semester == 4);
    bool belongsToSummer = (course.semester == 3);

    // Check if course is already selected in any relevant semester
    bool isSelectedInA = belongsToA && std::find(selectedIndicesA.begin(), selectedIndicesA.end(), index) != selectedIndicesA.end();
    bool isSelectedInB = belongsToB && std::find(selectedIndicesB.begin(), selectedIndicesB.end(), index) != selectedIndicesB.end();
    bool isSelectedInSummer = belongsToSummer && std::find(selectedIndicesSummer.begin(), selectedIndicesSummer.end(), index) != selectedIndicesSummer.end();

    bool isCurrentlySelected = isSelectedInA || isSelectedInB || isSelectedInSummer;

    if (isCurrentlySelected) {
        // Remove from all relevant semesters
        if (isSelectedInA) {
            auto itA = std::find(selectedIndicesA.begin(), selectedIndicesA.end(), index);
            int selectedIndexA = std::distance(selectedIndicesA.begin(), itA);
            selectedIndicesA.erase(itA);
            selectedCoursesA.erase(selectedCoursesA.begin() + selectedIndexA);
        }

        if (isSelectedInB) {
            auto itB = std::find(selectedIndicesB.begin(), selectedIndicesB.end(), index);
            int selectedIndexB = std::distance(selectedIndicesB.begin(), itB);
            selectedIndicesB.erase(itB);
            selectedCoursesB.erase(selectedCoursesB.begin() + selectedIndexB);
        }

        if (isSelectedInSummer) {
            auto itSummer = std::find(selectedIndicesSummer.begin(), selectedIndicesSummer.end(), index);
            int selectedIndexSummer = std::distance(selectedIndicesSummer.begin(), itSummer);
            selectedIndicesSummer.erase(itSummer);
            selectedCoursesSummer.erase(selectedCoursesSummer.begin() + selectedIndexSummer);
        }
    } else {
        // Add to all relevant semesters
        if (belongsToA) {
            selectedIndicesA.push_back(index);
            selectedCoursesA.push_back(course);
        }

        if (belongsToB) {
            selectedIndicesB.push_back(index);
            selectedCoursesB.push_back(course);
        }

        if (belongsToSummer) {
            selectedIndicesSummer.push_back(index);
            selectedCoursesSummer.push_back(course);
        }
    }

    // Update the display model
    updateSelectedCoursesModel();
    emit selectionChanged();
}

// Check if course is selected in any semester
bool CourseSelectionController::isCourseSelected(int index) {
    bool inA = std::find(selectedIndicesA.begin(), selectedIndicesA.end(), index) != selectedIndicesA.end();
    bool inB = std::find(selectedIndicesB.begin(), selectedIndicesB.end(), index) != selectedIndicesB.end();
    bool inSummer = std::find(selectedIndicesSummer.begin(), selectedIndicesSummer.end(), index) != selectedIndicesSummer.end();

    return inA || inB || inSummer;
}

// Get count for specific semester
int CourseSelectionController::getSelectedCoursesCountForSemester(const QString& semester) {
    if (semester == "A") {
        return static_cast<int>(selectedCoursesA.size());
    } else if (semester == "B") {
        return static_cast<int>(selectedCoursesB.size());
    } else if (semester == "SUMMER") {
        return static_cast<int>(selectedCoursesSummer.size());
    }
    return 0;
}

// Get courses for specific semester
QVariantList CourseSelectionController::getSelectedCoursesForSemester(const QString& semester) {
    QVariantList result;
    vector<Course>* coursesVector = nullptr;

    if (semester == "A") {
        coursesVector = &selectedCoursesA;
    } else if (semester == "B") {
        coursesVector = &selectedCoursesB;
    } else if (semester == "SUMMER") {
        coursesVector = &selectedCoursesSummer;
    }

    if (!coursesVector) {
        return result;
    }

    for (int i = 0; i < static_cast<int>(coursesVector->size()); i++) {
        const auto& course = (*coursesVector)[i];
        QVariantMap courseData;
        courseData["courseId"] = QString::fromStdString(course.raw_id);
        courseData["courseName"] = QString::fromStdString(course.name);
        courseData["originalIndex"] = i;
        result.append(courseData);
    }

    return result;
}

// Deselect course from semester-specific vectors
void CourseSelectionController::deselectCourse(int selectedIndex) {
    // Since we're now using semester-specific vectors, we need to find the course
    // in the appropriate semester vector based on the currently selected semester

    vector<Course>* targetVector = nullptr;
    vector<int>* targetIndices = nullptr;

    if (currentSemesterFilter == "A") {
        targetVector = &selectedCoursesA;
        targetIndices = &selectedIndicesA;
    } else if (currentSemesterFilter == "B") {
        targetVector = &selectedCoursesB;
        targetIndices = &selectedIndicesB;
    } else if (currentSemesterFilter == "SUMMER") {
        targetVector = &selectedCoursesSummer;
        targetIndices = &selectedIndicesSummer;
    }

    if (!targetVector || !targetIndices) {
        return;
    }

    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(targetVector->size())) {
        Logger::get().logError("Invalid selected course index for deselection");
        return;
    }

    // Get the original course index
    int originalIndex = (*targetIndices)[selectedIndex];

    // Remove from the target semester
    targetVector->erase(targetVector->begin() + selectedIndex);
    targetIndices->erase(targetIndices->begin() + selectedIndex);

    // For year-long courses (semester 4), also remove from the other semester
    const Course& course = allCourses[originalIndex];
    if (course.semester == 4) {
        // Remove from the other semester as well
        if (currentSemesterFilter == "A") {
            // Also remove from B
            auto itB = std::find(selectedIndicesB.begin(), selectedIndicesB.end(), originalIndex);
            if (itB != selectedIndicesB.end()) {
                int indexB = std::distance(selectedIndicesB.begin(), itB);
                selectedCoursesB.erase(selectedCoursesB.begin() + indexB);
                selectedIndicesB.erase(itB);
            }
        } else if (currentSemesterFilter == "B") {
            // Also remove from A
            auto itA = std::find(selectedIndicesA.begin(), selectedIndicesA.end(), originalIndex);
            if (itA != selectedIndicesA.end()) {
                int indexA = std::distance(selectedIndicesA.begin(), itA);
                selectedCoursesA.erase(selectedCoursesA.begin() + indexA);
                selectedIndicesA.erase(itA);
            }
        }
    }

    updateSelectedCoursesModel();
    emit selectionChanged();
}

void CourseSelectionController::filterCourses(const QString& searchText) {
    currentSearchText = searchText;
    applyFilters();
}

void CourseSelectionController::resetFilter() {
    currentSearchText.clear();
    currentSemesterFilter = "ALL";
    applyFilters();
}

void CourseSelectionController::filterBySemester(const QString& semester) {
    currentSemesterFilter = semester;
    applyFilters();
    // Update the selected courses model when semester filter changes
    updateSelectedCoursesModel();
}

void CourseSelectionController::applyFilters() {
    filteredCourses.clear();
    filteredIndicesMap.clear();

    for (size_t i = 0; i < allCourses.size(); ++i) {
        const Course& course = allCourses[i];

        if (matchesSemesterFilter(course) && matchesSearchFilter(course, currentSearchText)) {
            filteredCourses.push_back(course);
            filteredIndicesMap.push_back(static_cast<int>(i));
        }
    }

    m_filteredCourseModel->populateCoursesData(filteredCourses, filteredIndicesMap);
}

bool CourseSelectionController::matchesSemesterFilter(const Course& course) const {
    if (currentSemesterFilter == "ALL") {
        return true;
    }

    if (currentSemesterFilter == "A") {
        return course.semester == 1 || course.semester == 4;
    }

    if (currentSemesterFilter == "B") {
        return course.semester == 2 || course.semester == 4;
    }

    if (currentSemesterFilter == "SUMMER") {
        return course.semester == 3 || course.semester == 4;
    }

    return true;
}

bool CourseSelectionController::matchesSearchFilter(const Course& course, const QString& searchText) const {
    if (searchText.isEmpty()) {
        return true;
    }

    QString searchLower = searchText.toLower();
    QString courseId = QString::fromStdString(course.raw_id).toLower();
    QString courseName = QString::fromStdString(course.name).toLower();
    QString teacherName = QString::fromStdString(course.teacher).toLower();

    return courseId.contains(searchLower) ||
           courseName.contains(searchLower) ||
           teacherName.contains(searchLower);
}

void CourseSelectionController::createNewCourse(const QString& courseName, const QString& courseId,
                                                const QString& teacherName, int semester, const QVariantList& sessionGroups) {

    for (const auto& course : allCourses) {
        if (QString::fromStdString(course.raw_id) == courseId) {
            emit errorMessage("Course ID already exists");
            return;
        }
    }

    Course newCourse = createCourseFromData(courseName, courseId, teacherName, sessionGroups);

    // Set the semester from the parameter
    newCourse.semester = semester;

    Logger::get().logInfo("Created new course with ID: " + to_string(newCourse.id) +
                          ", name: " + newCourse.name + ", raw_id: " + newCourse.raw_id +
                          ", semester: " + to_string(newCourse.semester));

    // Add to allCourses
    allCourses.push_back(newCourse);

    // Update the main course model
    m_courseModel->populateCoursesData(allCourses);

    // Apply filters to include the new course if it matches current filters
    applyFilters();

    Logger::get().logInfo("New course created: " + courseName.toStdString() + ", "  + courseId.toStdString());

    cleanupValidatorThread();

    setValidationInProgress(true);

    setValidationErrors(QStringList());

    vector <Course> coursesToValidate = allCourses;

    int timeoutMs = std::min(VALIDATION_TIMEOUT_MS,
                             static_cast<int>(coursesToValidate.size() * 100 + 10000));

    QTimer::singleShot(100, this, [this, coursesToValidate, timeoutMs]() {
        validateCourses(coursesToValidate, timeoutMs);
    });
}

Course CourseSelectionController::createCourseFromData(const QString& courseName, const QString& courseId,
                                                       const QString& teacherName, const QVariantList& sessionGroups) {
    Course course;
    course.id = courseId.toInt();
    course.raw_id = courseId.toStdString();
    course.name = courseName.toStdString();
    course.teacher = teacherName.toStdString();
    course.semester = 1;

    // Clear all session vectors
    course.Lectures.clear();
    course.Tirgulim.clear();
    course.labs.clear();
    course.blocks.clear();

    // Process session groups
    for (const QVariant& groupVar : sessionGroups) {
        QVariantMap groupMap = groupVar.toMap();
        string groupType = groupMap["type"].toString().toStdString();
        QVariantList sessions = groupMap["sessions"].toList();

        Group group;

        // Set group type
        if (groupType == "Lecture") {
            group.type = SessionType::LECTURE;
        } else if (groupType == "Tutorial") {
            group.type = SessionType::TUTORIAL;
        } else if (groupType == "Lab") {
            group.type = SessionType::LAB;
        } else {
            group.type = SessionType::LECTURE;
        }

        Logger::get().logInfo("parsing group from type: " + groupMap["type"].toString().toStdString());

        // Process sessions for this group
        for (const QVariant& sessionVar : sessions) {
            Logger::get().logInfo("A");
            QVariantMap sessionMap = sessionVar.toMap();
            Logger::get().logInfo("B");

            Session session;
            Logger::get().logInfo("C");

            session.day_of_week = getDayNumber(sessionMap["day"].toString());
            Logger::get().logInfo("D");

            // Ensure proper time format
            QString startTime = sessionMap["startTime"].toString();
            QString endTime = sessionMap["endTime"].toString();

            Logger::get().logInfo("E");

            // Add validation and formatting
            if (!startTime.contains(":")) {
                startTime = startTime + ":00";
            }
            if (!endTime.contains(":")) {
                endTime = endTime + ":00";
            }

            Logger::get().logInfo("F");

            session.start_time = startTime.toStdString();
            session.end_time = endTime.toStdString();
            session.building_number = sessionMap["building"].toString().toStdString();
            session.room_number = sessionMap["room"].toString().toStdString();

            Logger::get().logInfo("Manual course session created: Day=" + std::to_string(session.day_of_week) +
                                  ", Start=" + session.start_time +
                                  ", End=" + session.end_time +
                                  ", Building=" + session.building_number +
                                  ", Room=" + session.room_number);

            group.sessions.push_back(session);
        }

        // Add group to appropriate vector
        if (group.type == SessionType::LECTURE) {
            course.Lectures.push_back(group);
        } else if (group.type == SessionType::TUTORIAL) {
            course.Tirgulim.push_back(group);
        } else if (group.type == SessionType::LAB) {
            course.labs.push_back(group);
        }
    }

    return course;
}

void CourseSelectionController::validateCourses(const vector<Course>& courses, int timeoutMs) {
    if (validatorThread && validatorThread->isRunning()) {
        return;
    }

    cleanupValidatorThread();
    validationCompleted = false;
    setValidationInProgress(true);

    Logger::get().logInfo("Starting validation");

    // Create validation thread with safety limits
    validatorThread = new QThread(this);
    auto* worker = new CourseValidator(modelConnection, courses);
    worker->moveToThread(validatorThread);

    // Setup timeout with custom duration
    setupValidationTimeout(timeoutMs);

    // Connect signals
    connect(validatorThread, &QThread::started, worker, &CourseValidator::validateCourses, Qt::QueuedConnection);
    connect(worker, &CourseValidator::coursesValidated, this, &CourseSelectionController::onCoursesValidated, Qt::QueuedConnection);
    connect(worker, &CourseValidator::coursesValidated, worker, &QObject::deleteLater, Qt::QueuedConnection);
    connect(worker, &CourseValidator::coursesValidated, validatorThread, &QThread::quit, Qt::QueuedConnection);
    connect(worker, &CourseValidator::coursesValidated, this, &CourseSelectionController::cleanupValidation, Qt::QueuedConnection);
    connect(validatorThread, &QThread::finished, validatorThread, &QObject::deleteLater, Qt::QueuedConnection);

    connect(validatorThread, &QThread::finished, [this]() {
        validatorThread = nullptr;
    });

    validatorThread->start();
}

void CourseSelectionController::setupValidationTimeout(int timeoutMs) {
    if (validationTimeoutTimer) {
        validationTimeoutTimer->stop();
        validationTimeoutTimer->deleteLater();
    }

    validationTimeoutTimer = new QTimer(this);
    validationTimeoutTimer->setSingleShot(true);
    validationTimeoutTimer->setInterval(timeoutMs);

    connect(validationTimeoutTimer, &QTimer::timeout, this, &CourseSelectionController::onValidationTimeout);
    validationTimeoutTimer->start();
}

void CourseSelectionController::onValidationTimeout() {
    if (validationCompleted) {
        return;
    }

    validationCompleted = true;
    setValidationInProgress(false);

    if (validatorThread && validatorThread->isRunning()) {
        auto workers = validatorThread->findChildren<CourseValidator*>();
        for (auto* worker : workers) {
            worker->cancelValidation();
        }

        QTimer::singleShot(2000, this, [this]() {
            if (validatorThread && validatorThread->isRunning()) {
                validatorThread->quit();

                QTimer::singleShot(3000, this, [this]() {
                    if (validatorThread && validatorThread->isRunning()) {
                        validatorThread->terminate();
                        validatorThread->wait(2000);
                    }
                });
            }
        });
    }

    // Set error state in UI
    setValidationErrors(QStringList{
            "[System] Validation timed out",
            "The course file may be too large or contain complex conflicts",
            "Try using a smaller file or contact support if this continues"
    });

    cleanupValidation();
}

void CourseSelectionController::onCoursesValidated(vector<string>* errors) {
    if (validationTimeoutTimer && validationTimeoutTimer->isActive()) {
        validationTimeoutTimer->stop();
    }

    if (validationCompleted) {
        if (errors) {
            delete errors;
        }
        return;
    }

    validationCompleted = true;
    setValidationInProgress(false);

    QStringList qmlErrors;

    try {
        if (!errors) {
            Logger::get().logError("Received null errors pointer");
            qmlErrors.append("[System] Validation failed - internal error");
        } else {

            for (size_t i = 0; i < errors->size(); ++i) {
                try {
                    const string& message = errors->at(i);
                    qmlErrors.append(QString::fromStdString(message));
                } catch (const std::exception& e) {
                }
            }
        }

    } catch (const std::exception& e) {
        qmlErrors.clear();
    } catch (...) {
        Logger::get().logError("Unknown exception while processing validation results");
        qmlErrors.clear();
    }

    setValidationErrors(qmlErrors);

    Logger::get().logInfo("Validation processing completed safely");
}

void CourseSelectionController::cleanupValidation() {
    if (validationTimeoutTimer && validationTimeoutTimer->isActive()) {
        validationTimeoutTimer->stop();
    }

    if (validationTimeoutTimer) {
        validationTimeoutTimer->deleteLater();
        validationTimeoutTimer = nullptr;
    }

    validationCompleted = false;

    setValidationInProgress(false);
}

void CourseSelectionController::cleanupValidatorThread() {
    if (!validatorThread) {
        return;
    }

    try {
        auto workers = validatorThread->findChildren<CourseValidator*>();
        for (auto* worker : workers) {
            worker->cancelValidation();
        }

        if (!validatorThread->isRunning()) {
            validatorThread->deleteLater();
            validatorThread = nullptr;
            return;
        }

        validatorThread->quit();

        if (validatorThread->wait(5000)) {
            Logger::get().logInfo("Thread quit gracefully");
        } else {
            Logger::get().logWarning("Thread didn't quit gracefully, forcing termination");
            validatorThread->terminate();
            if (validatorThread->wait(3000)) {
                Logger::get().logInfo("Thread terminated successfully");
            } else {
                Logger::get().logError("Thread termination failed, emergency cleanup");
            }
        }

        validatorThread->deleteLater();
        validatorThread = nullptr;

    } catch (const std::exception& e) {
        validatorThread = nullptr;
    } catch (...) {
        validatorThread = nullptr;
    }

    if (validationTimeoutTimer) {
        validationTimeoutTimer->stop();
        validationTimeoutTimer->deleteLater();
        validationTimeoutTimer = nullptr;
    }

    setValidationInProgress(false);
    validationCompleted = false;
}

void CourseSelectionController::addBlockTime(const QString& day, const QString& startTime, const QString& endTime) {
    // Validate time format and logic first
    if (startTime >= endTime) {
        emit errorMessage("Start time must be before end time");
        return;
    }

    // Helper function to convert time string to minutes for easier comparison
    auto timeToMinutes = [](const QString& time) -> int {
        QStringList parts = time.split(":");
        if (parts.size() != 2) return -1;
        int hours = parts[0].toInt();
        int minutes = parts[1].toInt();
        return hours * 60 + minutes;
    };

    int newStartMinutes = timeToMinutes(startTime);
    int newEndMinutes = timeToMinutes(endTime);

    if (newStartMinutes == -1 || newEndMinutes == -1) {
        emit errorMessage("Invalid time format");
        return;
    }

    // Check for overlaps with existing block times on the same day
    for (const auto& blockTime : userBlockTimes) {
        if (blockTime.day == day) {
            int existingStartMinutes = timeToMinutes(blockTime.startTime);
            int existingEndMinutes = timeToMinutes(blockTime.endTime);

            // Check if the new time block overlaps with the existing one
            // Two time blocks overlap if:
            // 1. new start time is before existing end time AND
            // 2. new end time is after existing start time
            if (newStartMinutes < existingEndMinutes && newEndMinutes > existingStartMinutes) {
                emit errorMessage(QString("Time block overlaps with existing block on %1 (%2 - %3)")
                                          .arg(day)
                                          .arg(blockTime.startTime)
                                          .arg(blockTime.endTime));
                return;
            }
        }
    }

    // If we get here, there's no overlap, so add the new block time
    userBlockTimes.emplace_back(day, startTime, endTime);
    updateBlockTimesModel();

    emit blockTimesChanged();
}

void CourseSelectionController::removeBlockTime(int index) {
    if (index < 0 || index >= static_cast<int>(userBlockTimes.size())) {
        Logger::get().logError("Invalid block time index for removal");
        return;
    }

    userBlockTimes.erase(userBlockTimes.begin() + index);
    updateBlockTimesModel();

    emit blockTimesChanged();
}

void CourseSelectionController::clearAllBlockTimes() {
    userBlockTimes.clear();
    updateBlockTimesModel();
    emit blockTimesChanged();
}

Course CourseSelectionController::createSingleBlockTimeCourse() {
    Course blockCourse;
    blockCourse.id = 90000; // Fixed ID for the single block course
    blockCourse.raw_id = "TIME_BLOCKS";
    blockCourse.name = "Time Block";
    blockCourse.teacher = "System Generated";

    // Clear all session type vectors
    blockCourse.Lectures.clear();
    blockCourse.Tirgulim.clear();
    blockCourse.labs.clear();
    blockCourse.blocks.clear();

    // Create a single group containing all block times
    Group blockGroup;
    blockGroup.type = SessionType::BLOCK;

    // Add all user block times as sessions to this single group
    for (const auto& blockTime : userBlockTimes) {
        Session blockSession;
        blockSession.day_of_week = getDayNumber(blockTime.day);
        blockSession.start_time = blockTime.startTime.toStdString();
        blockSession.end_time = blockTime.endTime.toStdString();
        blockSession.building_number = "BLOCKED";
        blockSession.room_number = "BLOCK";

        blockGroup.sessions.push_back(blockSession);
    }

    // Add the group to the course's blocks
    blockCourse.blocks.push_back(blockGroup);

    return blockCourse;
}

void CourseSelectionController::updateBlockTimesModel() {
    blockTimes.clear();

    for (size_t i = 0; i < userBlockTimes.size(); ++i) {
        const BlockTime& blockTime = userBlockTimes[i];
        Course blockCourse;
        blockCourse.id = static_cast<int>(i) + 90000;
        // Include semester in the display
        blockCourse.raw_id = (blockTime.startTime + " - " + blockTime.endTime + " (" + blockTime.semester + ")").toStdString();
        blockCourse.name = "Blocked Time (" + blockTime.semester.toStdString() + ")";
        blockCourse.teacher = blockTime.day.toStdString(); // Store day in teacher field for display

        blockCourse.Lectures.clear();
        blockCourse.Tirgulim.clear();
        blockCourse.labs.clear();
        blockCourse.blocks.clear();

        // Create a group for this block time
        Group blockGroup;
        blockGroup.type = SessionType::BLOCK;

        Session blockSession;
        blockSession.day_of_week = getDayNumber(blockTime.day);
        blockSession.start_time = blockTime.startTime.toStdString();
        blockSession.end_time = blockTime.endTime.toStdString();
        blockSession.building_number = "BLOCKED";
        blockSession.room_number = "BLOCK";

        blockGroup.sessions.push_back(blockSession);
        blockCourse.blocks.push_back(blockGroup);

        blockTimes.push_back(blockCourse);
    }
    m_blocksModel->populateCoursesData(blockTimes);
}

int CourseSelectionController::getDayNumber(const QString& dayName) {
    if (dayName == "Sunday") return 1;
    if (dayName == "Monday") return 2;
    if (dayName == "Tuesday") return 3;
    if (dayName == "Wednesday") return 4;
    if (dayName == "Thursday") return 5;
    if (dayName == "Friday") return 6;
    if (dayName == "Saturday") return 7;
    return 1;
}
void CourseSelectionController::addBlockTimeToSemester(const QString& day, const QString& startTime,
                                                       const QString& endTime, const QString& semester) {
    // Validate time format and logic first
    if (startTime >= endTime) {
        emit errorMessage("Start time must be before end time");
        return;
    }

    // Helper function to convert time string to minutes for easier comparison
    auto timeToMinutes = [](const QString& time) -> int {
        QStringList parts = time.split(":");
        if (parts.size() != 2) return -1;
        int hours = parts[0].toInt();
        int minutes = parts[1].toInt();
        return hours * 60 + minutes;
    };

    int newStartMinutes = timeToMinutes(startTime);
    int newEndMinutes = timeToMinutes(endTime);

    if (newStartMinutes == -1 || newEndMinutes == -1) {
        emit errorMessage("Invalid time format");
        return;
    }

    // Check for overlaps with existing block times on the same day IN THE SAME SEMESTER
    for (const auto& blockTime : userBlockTimes) {
        // Only check against block times in the same semester
        if (blockTime.day == day && blockTime.semester == semester) {
            int existingStartMinutes = timeToMinutes(blockTime.startTime);
            int existingEndMinutes = timeToMinutes(blockTime.endTime);

            // Check if the new time block overlaps with the existing one
            if (newStartMinutes < existingEndMinutes && newEndMinutes > existingStartMinutes) {
                emit errorMessage(QString("Time block overlaps with existing block on %1 in semester %2 (%3 - %4)")
                                          .arg(day)
                                          .arg(semester)
                                          .arg(blockTime.startTime)
                                          .arg(blockTime.endTime));
                return;
            }
        }
    }

    // If we get here, there's no overlap, so add the new block time with semester info
    userBlockTimes.emplace_back(day, startTime, endTime, semester);
    updateBlockTimesModel();

    emit blockTimesChanged();
}
vector<BlockTime> CourseSelectionController::getBlockTimesForCurrentSemester(const QString& semester) {
    vector<BlockTime> result;

    // Filter userBlockTimes by semester
    for (const auto& blockTime : userBlockTimes) {
        if (blockTime.semester == semester) {
            result.push_back(blockTime);
        }
    }

    return result;
}

// Method to create block course for specific semester only
Course CourseSelectionController::createSingleBlockTimeCourseForSemester(
        const vector<BlockTime>& semesterBlockTimes, const QString& semester) {

    Course blockCourse;
    blockCourse.id = 90000;
    blockCourse.raw_id = "TIME_BLOCKS_" + semester.toStdString();
    blockCourse.name = "Time Block (" + semester.toStdString() + ")";
    blockCourse.teacher = "System Generated";

    blockCourse.Lectures.clear();
    blockCourse.Tirgulim.clear();
    blockCourse.labs.clear();
    blockCourse.blocks.clear();

    Group blockGroup;
    blockGroup.type = SessionType::BLOCK;

    for (const auto& blockTime : semesterBlockTimes) {
        Session blockSession;
        blockSession.day_of_week = getDayNumber(blockTime.day);
        blockSession.start_time = blockTime.startTime.toStdString();
        blockSession.end_time = blockTime.endTime.toStdString();
        blockSession.building_number = "BLOCKED";
        blockSession.room_number = "BLOCK";

        blockGroup.sessions.push_back(blockSession);
    }

    if (!blockGroup.sessions.empty()) {
        blockCourse.blocks.push_back(blockGroup);
    }

    return blockCourse;
}