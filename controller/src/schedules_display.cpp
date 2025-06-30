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

// initiate loading

void SchedulesDisplayController::loadScheduleData(const std::vector<InformativeSchedule> &schedules) {
    m_schedules = schedules;
    m_scheduleModel->loadSchedules(m_schedules);
    m_scheduleModel->setCurrentScheduleIndex(0);
}

void SchedulesDisplayController::goBack() {
    modelConnection->executeOperation(ModelOperation::CLEAN_SCHEDULES, nullptr, "");
    m_scheduleModel->setCurrentScheduleIndex(0);
    emit navigateBack();
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

    if (m_scheduleModel) {
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
        // Get the filtered schedule IDs from model
        void* result = modelConnection->executeOperation(ModelOperation::GET_LAST_FILTERED_IDS, nullptr, "");

        if (result) {
            auto* filteredIds = static_cast<std::vector<int>*>(result);


            if (filteredIds->empty()) {
                responseMessage += "\n\n❌ No schedules match your criteria.";

            } else {
                // Convert to QVariantList and apply filter
                QVariantList qmlIds;
                for (int id : *filteredIds) {
                    qmlIds.append(id);
                }

                if (m_scheduleModel) {
                    m_scheduleModel->applyScheduleFilter(qmlIds);

                    emit schedulesFiltered(static_cast<int>(filteredIds->size()),
                                           m_scheduleModel->totalScheduleCount());
                }
            }

            delete filteredIds;
        } else {
            responseMessage += "\n\n❌ Failed to apply schedule filter. Please try again.";
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

    // Check if we can make it in O(n)
    if (sortField == m_currentSortField && isAscending != m_currentSortAscending) {
        std::reverse(m_schedules.begin(), m_schedules.end());
    }
    else {
        if (sortField == "amount_days") {
            std::vector<std::vector<InformativeSchedule>> buckets(8); // Constant days
            for (const auto& sched : m_schedules) {
                if (sched.amount_days >= 1 && sched.amount_days <= 7)
                    buckets[sched.amount_days].push_back(sched);
                else
                    qWarning() << "amount_days out of range:" << sched.amount_days;
            }
            m_schedules.clear();
            if (isAscending) {
                for (int i = 1; i <= 7; ++i)
                    m_schedules.insert(m_schedules.end(), buckets[i].begin(), buckets[i].end());
            } else {
                for (int i = 7; i >= 1; --i)
                    m_schedules.insert(m_schedules.end(), buckets[i].begin(), buckets[i].end());
            }
        }
        else if (sortField == "amount_gaps") {
            std::sort(m_schedules.begin(), m_schedules.end(), [isAscending](const InformativeSchedule& a, const InformativeSchedule& b) {
                return isAscending ? a.amount_gaps < b.amount_gaps : a.amount_gaps > b.amount_gaps;
            });
        }
        else if (sortField == "gaps_time") {
            std::sort(m_schedules.begin(), m_schedules.end(), [isAscending](const InformativeSchedule& a, const InformativeSchedule& b) {
                return isAscending ? a.gaps_time < b.gaps_time : a.gaps_time > b.gaps_time;
            });
        }
        else if (sortField == "avg_start") {
            std::sort(m_schedules.begin(), m_schedules.end(), [isAscending](const InformativeSchedule& a, const InformativeSchedule& b) {
                return isAscending ? a.avg_start < b.avg_start : a.avg_start > b.avg_start;
            });
        }
        else if (sortField == "avg_end") {
            std::sort(m_schedules.begin(), m_schedules.end(), [isAscending](const InformativeSchedule& a, const InformativeSchedule& b) {
                return isAscending ? a.avg_end < b.avg_end : a.avg_end > b.avg_end;
            });
        }
        else {
            qWarning() << "Unknown sorting key received:" << sortField;
            clearSorting();
            return;
        }
    }

    m_currentSortField = sortField;
    m_currentSortAscending = isAscending;
    m_scheduleModel->setCurrentScheduleIndex(0);
    m_scheduleModel->loadSchedules(m_schedules);
    emit schedulesSorted(static_cast<int>(m_schedules.size()));
}

void SchedulesDisplayController::clearSorting() {
    // Reset to original order by index
    std::sort(m_schedules.begin(), m_schedules.end(), [](const InformativeSchedule& a, const InformativeSchedule& b) {
        return a.index < b.index;
    });

    m_currentSortField.clear();
    m_currentSortAscending = true;

    m_scheduleModel->loadSchedules(m_schedules);
    emit schedulesSorted(static_cast<int>(m_schedules.size()));
}

// export menu

void SchedulesDisplayController::saveScheduleAsCSV() {
    int currentIndex = m_scheduleModel->currentScheduleIndex();
    const auto& activeSchedules = m_scheduleModel->getCurrentSchedules();

    if (currentIndex >= 0 && currentIndex < static_cast<int>(activeSchedules.size())) {
        QString fileName = QFileDialog::getSaveFileName(nullptr,
                                                        "Save Schedule as CSV",
                                                        QDir::homePath() + "/" + generateFilename("",
                                                                                                  currentIndex + 1, fileType::CSV),
                                                        "CSV Files (*.csv)");
        if (!fileName.isEmpty()) {
            modelConnection->executeOperation(ModelOperation::SAVE_SCHEDULE,
                                              &activeSchedules[currentIndex], fileName.toLocal8Bit().constData());
        }
    }
}

void SchedulesDisplayController::printScheduleDirectly() {
    int currentIndex = m_scheduleModel->currentScheduleIndex();
    const auto& activeSchedules = m_scheduleModel->getCurrentSchedules();

    if (currentIndex >= 0 && currentIndex < static_cast<int>(activeSchedules.size())) {
        modelConnection->executeOperation(ModelOperation::PRINT_SCHEDULE, &activeSchedules[currentIndex], "");
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
                                 currentIndex + 1, fileType::PNG),
                tr("Images (*.png)")
        );

        if (path.isEmpty()) {
            return;
        }
    } else {
        QFileInfo fileInfo(path);
        if (fileInfo.isDir()) {
            int currentIndex = m_scheduleModel->currentScheduleIndex();
            path = QDir(path).filePath(generateFilename("", currentIndex + 1, fileType::PNG));
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

QString SchedulesDisplayController::generateFilename(const QString& basePath, int index, fileType type) {
    QString filename;
    switch (type) {
        case fileType::PNG:
            filename = QString("Schedule-%1.png").arg(index);
            break;
        case fileType::CSV:
            filename = QString("Schedule-%1.csv").arg(index);
            break;
    }

    if (basePath.isEmpty()) {
        return filename;
    }

    return QDir(basePath).filePath(filename);
}