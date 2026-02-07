#include "schedules_display.h"

SchedulesDisplayController::SchedulesDisplayController(QObject *parent)
        : ControllerManager(parent),
          m_scheduleModel(new ScheduleModel(this)) {
    modelConnection = ModelAccess::getModel();

    // Initialize sort key mapping for better organization
    m_sortKeyMap["amount_days"] = "Days";
    m_sortKeyMap["amount_gaps"] = "Gaps";
    m_sortKeyMap["gaps_time"] = "Gap Time";
    m_sortKeyMap["avg_start"] = "Average Start";
    m_sortKeyMap["avg_end"] = "Average End";

    // Connect to schedule model filter state changes
    connect(m_scheduleModel, &ScheduleModel::filterStateChanged,
            this, &SchedulesDisplayController::onScheduleFilterStateChanged);

    connect(this, &SchedulesDisplayController::schedulesSorted, this, [this]() {
        emit m_scheduleModel->scheduleDataChanged();
    });
}

SchedulesDisplayController::~SchedulesDisplayController() {
    modelConnection = nullptr;
}

// initiate data and semester management

void SchedulesDisplayController::loadSemesterScheduleData(const QString& semester, const std::vector<InformativeSchedule>& schedules) {
    if (semester == "A") {
        m_schedulesA = schedules;
        // If this is the first semester loaded, set it as current and update display
        if (m_currentSemester == "A") {
            m_scheduleModel->loadSchedules(m_schedulesA);
        }
    } else if (semester == "B") {
        m_schedulesB = schedules;
    } else if (semester == "SUMMER") {
        m_schedulesSummer = schedules;
    }

    // Mark semester as finished loading
    setSemesterFinished(semester, true);
    setSemesterLoading(semester, false);

    emit semesterSchedulesLoaded(semester);
}

void SchedulesDisplayController::switchToSemester(const QString& semester) {
    if (m_currentSemester == semester) {
        return;
    }

    if (!canClickSemester(semester)) {
        qWarning() << "Semester" << semester << "is not ready to be clicked yet";
        return;
    }

    m_currentSemester = semester;

    // Load the appropriate schedules into the model
    if (semester == "A") {
        m_scheduleModel->loadSchedules(m_schedulesA);
    } else if (semester == "B") {
        m_scheduleModel->loadSchedules(m_schedulesB);
    } else if (semester == "SUMMER") {
        m_scheduleModel->loadSchedules(m_schedulesSummer);
    }

    if (m_scheduleModel && m_scheduleModel->isFiltered()) {
        m_scheduleModel->clearScheduleFilter();
    }

    m_scheduleModel->setCurrentScheduleIndex(0);

    emit currentSemesterChanged();

    QTimer::singleShot(50, this, [this]() {
        if (m_scheduleModel) {
            emit m_scheduleModel->scheduleDataChanged();
            emit m_scheduleModel->currentScheduleIndexChanged();
            emit m_scheduleModel->scheduleCountChanged();
        }
    });
}

void SchedulesDisplayController::allSemestersGenerated() {
    m_allSemestersLoaded = true;
    emit allSemestersReady();
}

void SchedulesDisplayController::resetToSemesterA() {
    m_currentSemester = "A";
    // If Semester A has schedules, load them into the model
    if (!m_schedulesA.empty()) {
        m_scheduleModel->loadSchedules(m_schedulesA);
    }
    emit currentSemesterChanged();
}

bool SchedulesDisplayController::hasSchedulesForSemester(const QString& semester) const {
    if (semester == "A") {
        return !m_schedulesA.empty();
    } else if (semester == "B") {
        return !m_schedulesB.empty();
    } else if (semester == "SUMMER") {
        return !m_schedulesSummer.empty();
    }
    return false;
}

bool SchedulesDisplayController::isSemesterLoading(const QString& semester) const {
    return m_semesterLoadingState.value(semester, false);
}

bool SchedulesDisplayController::isSemesterFinished(const QString& semester) const {
    return m_semesterFinishedState.value(semester, false);
}

bool SchedulesDisplayController::canClickSemester(const QString& semester) const {
    return isSemesterFinished(semester) && !isSemesterLoading(semester) && hasSchedulesForSemester(semester);
}

void SchedulesDisplayController::setSemesterLoading(const QString& semester, bool loading) {
    if (m_semesterLoadingState.value(semester, false) != loading) {
        m_semesterLoadingState[semester] = loading;
        emit semesterLoadingStateChanged(semester);
    }
}

void SchedulesDisplayController::setSemesterFinished(const QString& semester, bool finished) {
    if (m_semesterFinishedState.value(semester, false) != finished) {
        m_semesterFinishedState[semester] = finished;
        emit semesterFinishedStateChanged(semester);
    }
}

int SchedulesDisplayController::getScheduleCountForSemester(const QString& semester) const {
    if (semester == "A") {
        return static_cast<int>(m_schedulesA.size());
    } else if (semester == "B") {
        return static_cast<int>(m_schedulesB.size());
    } else if (semester == "SUMMER") {
        return static_cast<int>(m_schedulesSummer.size());
    }
    return 0;
}

std::vector<InformativeSchedule>* SchedulesDisplayController::getCurrentScheduleVector() {
    if (m_currentSemester == "A") {
        return &m_schedulesA;
    } else if (m_currentSemester == "B") {
        return &m_schedulesB;
    } else if (m_currentSemester == "SUMMER") {
        return &m_schedulesSummer;
    }
    return nullptr;
}

void SchedulesDisplayController::clearAllSchedules() {
    // Clear all semester schedule vectors
    m_schedulesA.clear();
    m_schedulesB.clear();
    m_schedulesSummer.clear();

    // Reset all loading and finished states
    m_semesterLoadingState["A"] = false;
    m_semesterLoadingState["B"] = false;
    m_semesterLoadingState["SUMMER"] = false;

    m_semesterFinishedState["A"] = false;
    m_semesterFinishedState["B"] = false;
    m_semesterFinishedState["SUMMER"] = false;

    // Clear the current display
    m_scheduleModel->loadSchedules(std::vector<InformativeSchedule>());

    // Reset to semester A
    m_currentSemester = "A";
    m_allSemestersLoaded = false;

    // Emit signals to update UI
    emit currentSemesterChanged();
    emit semesterLoadingStateChanged("A");
    emit semesterLoadingStateChanged("B");
    emit semesterLoadingStateChanged("SUMMER");
    emit semesterFinishedStateChanged("A");
    emit semesterFinishedStateChanged("B");
    emit semesterFinishedStateChanged("SUMMER");
}

// filter assistant

void SchedulesDisplayController::processBotMessage(const QString& userMessage) {
    if (!modelConnection) {
        emit botResponseReceived("I'm sorry, but I'm unable to process your request right now. Please try again later.");
        return;
    }

    bool hadPreviousFilter = false;
    if (m_scheduleModel && m_scheduleModel->isFiltered()) {
        hadPreviousFilter = true;
    }

    BotQueryRequest queryRequest = createBotQueryRequest(userMessage);

    // Start processing in a separate thread
    auto* workerThread = new QThread;
    auto* worker = new BotWorker(modelConnection, queryRequest);
    worker->moveToThread(workerThread);

    // Connect signals
    connect(workerThread, &QThread::started, worker, &BotWorker::processMessage);

    // Use SINGLE response handler - doesn't matter if demo or real
    connect(worker, QOverload<const BotQueryResponse&>::of(&BotWorker::responseReady),
            this, &SchedulesDisplayController::handleBotResponse);

    connect(worker, &BotWorker::errorOccurred, this, [this](const QString& error) {
        emit botResponseReceived(error);
    });

    connect(worker, &BotWorker::finished, [worker, workerThread]() {
        worker->deleteLater();
        workerThread->quit();
        workerThread->deleteLater();
    });

    workerThread->start();
}

BotQueryRequest SchedulesDisplayController::createBotQueryRequest(const QString& userMessage) {
    BotQueryRequest request;
    request.userMessage = userMessage.toStdString();
    request.scheduleMetadata = "";
    request.semester = m_currentSemester.toStdString();

    std::vector<InformativeSchedule>* currentSchedules = getCurrentScheduleVector();
    if (currentSchedules && !currentSchedules->empty()) {
        for (const InformativeSchedule& s : *currentSchedules) {
            request.availableUniqueIds.push_back(s.unique_id);
            request.availableScheduleIds.push_back(s.index);
            ScheduleFilterMetrics m;
            m.unique_id = s.unique_id;
            m.semester = s.semester;
            m.amount_days = s.amount_days;
            m.amount_gaps = s.amount_gaps;
            m.gaps_time = s.gaps_time;
            m.avg_start = s.avg_start;
            m.avg_end = s.avg_end;
            m.earliest_start = s.earliest_start;
            m.latest_end = s.latest_end;
            m.longest_gap = s.longest_gap;
            m.total_class_time = s.total_class_time;
            m.consecutive_days = s.consecutive_days;
            m.weekend_classes = s.weekend_classes;
            m.has_morning_classes = s.has_morning_classes;
            m.has_early_morning = s.has_early_morning;
            m.has_evening_classes = s.has_evening_classes;
            m.has_late_evening = s.has_late_evening;
            m.max_daily_hours = s.max_daily_hours;
            m.min_daily_hours = s.min_daily_hours;
            m.avg_daily_hours = s.avg_daily_hours;
            m.has_lunch_break = s.has_lunch_break;
            m.max_daily_gaps = s.max_daily_gaps;
            m.avg_gap_length = s.avg_gap_length;
            m.schedule_span = s.schedule_span;
            m.compactness_ratio = s.compactness_ratio;
            m.weekday_only = s.weekday_only;
            m.has_monday = s.has_monday;
            m.has_tuesday = s.has_tuesday;
            m.has_wednesday = s.has_wednesday;
            m.has_thursday = s.has_thursday;
            m.has_friday = s.has_friday;
            m.has_saturday = s.has_saturday;
            m.has_sunday = s.has_sunday;
            request.viewScheduleMetrics.push_back(m);
        }
    } else if (m_scheduleModel) {
        QVariantList allUniqueIds = m_scheduleModel->getAllScheduleUniqueIds();
        for (const QVariant& uniqueId : allUniqueIds) {
            request.availableUniqueIds.push_back(uniqueId.toString().toStdString());
        }
        QVariantList allIds = m_scheduleModel->getAllScheduleIds();
        for (const QVariant& id : allIds) {
            request.availableScheduleIds.push_back(id.toInt());
        }
    }

    return request;
}

void SchedulesDisplayController::handleBotResponse(const BotQueryResponse& response) {
    if (response.hasError) {
        emit botResponseReceived(response.errorMessage.empty()
                                 ? QString("An error occurred while processing your request.")
                                 : QString::fromStdString(response.errorMessage));
        m_scheduleModel->clearScheduleFilter();
        m_scheduleModel->setCurrentScheduleIndex(0);
        return;
    }

    // Display the response message to user
    QString responseMessage = QString::fromStdString(response.userMessage);

    // If this was a filter query, apply the filter
    if (response.isFilterQuery) {
        m_scheduleModel->clearScheduleFilter();
        m_scheduleModel->setCurrentScheduleIndex(0);

        // NEW: Primary path - use unique IDs if available
        if (!response.filteredUniqueIds.empty()) {
            QVariantList uniqueIdsForFilter;
            for (const string& uniqueId : response.filteredUniqueIds) {
                uniqueIdsForFilter.append(QString::fromStdString(uniqueId));
            }

            if (m_scheduleModel) {
                m_scheduleModel->applyScheduleFilterByUniqueIds(uniqueIdsForFilter);
                emit schedulesFiltered(uniqueIdsForFilter.size(),
                                       m_scheduleModel->totalScheduleCount());
            }
        }
            // FALLBACK: Use old index-based system if unique IDs not available
        else if (!response.filteredScheduleIds.empty()) {
            // Get the filtered schedule IDs from model (legacy path)
            void* result = modelConnection->executeOperation(ModelOperation::GET_LAST_FILTERED_IDS, nullptr, "");

            if (result) {
                auto* filteredIds = static_cast<std::vector<int>*>(result);

                if (filteredIds->empty()) {
                    responseMessage += "\n\n❌ No schedules match your criteria.";
                } else {
                    // Convert schedule indices to unique IDs for filtering
                    QVariantList uniqueIdsForFilter;

                    for (int scheduleIndex : *filteredIds) {
                        QString uniqueId = m_scheduleModel->getUniqueIdByScheduleIndex(scheduleIndex);
                        if (!uniqueId.isEmpty()) {
                            uniqueIdsForFilter.append(uniqueId);
                        }
                    }

                    if (m_scheduleModel && !uniqueIdsForFilter.isEmpty()) {
                        m_scheduleModel->applyScheduleFilterByUniqueIds(uniqueIdsForFilter);
                        emit schedulesFiltered(uniqueIdsForFilter.size(),
                                               m_scheduleModel->totalScheduleCount());
                    } else {
                        responseMessage += "\n\n❌ Failed to apply schedule filter. Please try again.";
                    }
                }

                delete filteredIds;
            } else {
                responseMessage += "\n\n❌ Failed to apply schedule filter. Please try again.";
            }
        }
        else {
            responseMessage += "\n\n❌ No filtering results received.";
        }
    }

    emit botResponseReceived(responseMessage);
}

void SchedulesDisplayController::resetFilters() {
    if (m_scheduleModel && m_scheduleModel->isFiltered()) {
        m_scheduleModel->clearScheduleFilter();
        m_scheduleModel->setCurrentScheduleIndex(0);
    }
}

void SchedulesDisplayController::onScheduleFilterStateChanged() {
    emit filterStateChanged();

    if (m_scheduleModel) {
        int filteredCount = m_scheduleModel->scheduleCount();
        int totalCount = m_scheduleModel->totalScheduleCount();

        if (m_scheduleModel->isFiltered()) {
            emit schedulesFiltered(filteredCount, totalCount);
        }
    }
}

// sorting assistant

void SchedulesDisplayController::applySorting(const QVariantMap& sortData) {
    QString sortField;
    bool isAscending = true;
    for (auto it = sortData.constBegin(); it != sortData.constEnd(); ++it) {
        const QVariantMap criterion = it.value().toMap();
        if (criterion.value("enabled").toBool()) {
            sortField = it.key();
            isAscending = criterion["ascending"].toBool();
            break;
        }
    }

    if (sortField.isEmpty()) {
        qWarning() << "No sorting field enabled!";
        clearSorting();
        return;
    }

    // Get reference to current semester's schedules
    std::vector<InformativeSchedule>* currentSchedules = getCurrentScheduleVector();
    if (!currentSchedules) {
        return;
    }

    // Apply sorting logic (same as before, but ensure unique IDs are preserved)
    if (sortField == m_currentSortField && isAscending != m_currentSortAscending) {
        std::reverse(currentSchedules->begin(), currentSchedules->end());
    } else {
        if (sortField == "amount_days") {
            std::vector<std::vector<InformativeSchedule>> buckets(8);
            for (const auto& sched : *currentSchedules) {
                if (sched.amount_days >= 1 && sched.amount_days <= 7)
                    buckets[sched.amount_days].push_back(sched);
                else
                    qWarning() << "amount_days out of range:" << sched.amount_days;
            }
            currentSchedules->clear();
            if (isAscending) {
                for (int i = 1; i <= 7; ++i)
                    currentSchedules->insert(currentSchedules->end(), buckets[i].begin(), buckets[i].end());
            } else {
                for (int i = 7; i >= 1; --i)
                    currentSchedules->insert(currentSchedules->end(), buckets[i].begin(), buckets[i].end());
            }
        } else if (sortField == "amount_gaps") {
            std::sort(currentSchedules->begin(), currentSchedules->end(), [isAscending](const InformativeSchedule& a, const InformativeSchedule& b) {
                return isAscending ? a.amount_gaps < b.amount_gaps : a.amount_gaps > b.amount_gaps;
            });
        } else if (sortField == "gaps_time") {
            std::sort(currentSchedules->begin(), currentSchedules->end(), [isAscending](const InformativeSchedule& a, const InformativeSchedule& b) {
                return isAscending ? a.gaps_time < b.gaps_time : a.gaps_time > b.gaps_time;
            });
        } else if (sortField == "avg_start") {
            std::sort(currentSchedules->begin(), currentSchedules->end(), [isAscending](const InformativeSchedule& a, const InformativeSchedule& b) {
                return isAscending ? a.avg_start < b.avg_start : a.avg_start > b.avg_start;
            });
        } else if (sortField == "avg_end") {
            std::sort(currentSchedules->begin(), currentSchedules->end(), [isAscending](const InformativeSchedule& a, const InformativeSchedule& b) {
                return isAscending ? a.avg_end < b.avg_end : a.avg_end > b.avg_end;
            });
        } else {
            qWarning() << "Unknown sorting key received:" << sortField;
            clearSorting();
            return;
        }
    }

    m_currentSortField = sortField;
    m_currentSortAscending = isAscending;
    m_scheduleModel->setCurrentScheduleIndex(0);

    // UPDATED: Reload schedules while preserving unique ID mappings
    m_scheduleModel->loadSchedules(*currentSchedules);

    emit schedulesSorted(static_cast<int>(currentSchedules->size()));
}

void SchedulesDisplayController::clearSorting() {
    std::vector<InformativeSchedule>* currentSchedules = getCurrentScheduleVector();
    if (!currentSchedules) {
        return;
    }

    // Reset to original order by index (preserve unique IDs)
    std::sort(currentSchedules->begin(), currentSchedules->end(), [](const InformativeSchedule& a, const InformativeSchedule& b) {
        return a.index < b.index;
    });

    m_currentSortField.clear();
    m_currentSortAscending = true;

    // UPDATED: Reload schedules while preserving unique ID mappings
    m_scheduleModel->loadSchedules(*currentSchedules);

    emit schedulesSorted(static_cast<int>(currentSchedules->size()));
}

// export menu

void SchedulesDisplayController::saveScheduleAsCSV() {
    std::vector<InformativeSchedule>* currentSchedules = getCurrentScheduleVector();
    if (!currentSchedules || currentSchedules->empty()) {
        return;
    }

    int currentIndex = m_scheduleModel->currentScheduleIndex();
    if (currentIndex >= 0 && currentIndex < static_cast<int>(currentSchedules->size())) {
        // Get the current schedule's unique ID for better file naming
        QString currentUniqueId = m_scheduleModel->getCurrentScheduleUniqueId();

        QString fileName = QFileDialog::getSaveFileName(nullptr,
                                                        "Save Schedule as CSV",
                                                        QDir::homePath() + "/" + generateFilename("",
                                                                                                  currentIndex + 1, fileType::CSV, m_currentSemester),
                                                        "CSV Files (*.csv)");
        if (!fileName.isEmpty()) {
            modelConnection->executeOperation(ModelOperation::SAVE_SCHEDULE,
                                              &(*currentSchedules)[currentIndex], fileName.toLocal8Bit().constData());
        }
    }
}

void SchedulesDisplayController::printScheduleDirectly() {
    std::vector<InformativeSchedule>* currentSchedules = getCurrentScheduleVector();
    if (!currentSchedules || currentSchedules->empty()) {
        return;
    }

    int currentIndex = m_scheduleModel->currentScheduleIndex();
    if (currentIndex >= 0 && currentIndex < static_cast<int>(currentSchedules->size())) {
        modelConnection->executeOperation(ModelOperation::PRINT_SCHEDULE, &(*currentSchedules)[currentIndex], "");
    }
}

void SchedulesDisplayController::captureAndSave(QQuickItem* item, const QString& savePath) {
    if (!item) {
        emit screenshotFailed();
        return;
    }

    QString path = savePath;
    if (path.isEmpty()) {
        int currentIndex = m_scheduleModel->currentScheduleIndex();
        path = QFileDialog::getSaveFileName(
                nullptr,
                tr("Save Screenshot"),
                generateFilename(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
                                 currentIndex + 1, fileType::PNG, m_currentSemester),
                tr("Images (*.png)")
        );

        if (path.isEmpty()) {
            return;
        }
    } else {
        QFileInfo fileInfo(path);
        if (fileInfo.isDir()) {
            int currentIndex = m_scheduleModel->currentScheduleIndex();
            path = QDir(path).filePath(generateFilename("", currentIndex + 1, fileType::PNG, m_currentSemester));
        }
    }

    QSharedPointer<QQuickItemGrabResult> result = item->grabToImage();
    connect(result.data(), &QQuickItemGrabResult::ready, [this, result, path]() {
        if (result->saveToFile(path)) {
            emit screenshotSaved(path);
        } else {
            emit screenshotFailed();
        }
    });
}

QString SchedulesDisplayController::generateFilename(const QString& basePath, int index, fileType type, const QString& semester) {
    QString filename;
    QString semesterSuffix = semester.isEmpty() ? "" : QString("_%1").arg(semester);

    switch (type) {
        case fileType::PNG:
            filename = QString("Schedule%1-%2.png").arg(semesterSuffix).arg(index);
            break;
        case fileType::CSV:
            filename = QString("Schedule%1-%2.csv").arg(semesterSuffix).arg(index);
            break;
    }

    if (basePath.isEmpty()) {
        return filename;
    }

    return QDir(basePath).filePath(filename);
}

// navigate back

void SchedulesDisplayController::goBack() {
    modelConnection->executeOperation(ModelOperation::CLEAN_SCHEDULES, nullptr, "");
    m_scheduleModel->setCurrentScheduleIndex(0);
    emit navigateBack();
}