#include "ChatBot.h"
#include <QDebug>
#include <iostream>
#include <utility>

BotWorker::BotWorker(IModel* model, const std::vector<std::string>& messageData, QObject* parent)
        : QObject(parent), m_model(model), m_messageData(messageData), m_useLegacyFormat(true) {
}

BotWorker::BotWorker(IModel* model, BotQueryRequest  queryRequest, QObject* parent)
        : QObject(parent), m_model(model), m_queryRequest(std::move(queryRequest)), m_useLegacyFormat(false) {
}

void BotWorker::processMessage() {
    try {
        if (!m_model) {
            emit errorOccurred("Model connection not available");
            emit finished();
            return;
        }
        processBotQuery();
    } catch (const std::exception& e) {
        qWarning() << "BotWorker: Error processing message:" << e.what();
        emit errorOccurred("An error occurred while processing your request. Please try again.");
        emit finished();
    } catch (...) {
        qWarning() << "BotWorker: Unknown error processing message";
        emit errorOccurred("An unexpected error occurred. Please try again.");
        emit finished();
    }
}

void BotWorker::processBotQuery() {
    // Send to model using new BOT_QUERY_SCHEDULES operation
    void* result = m_model->executeOperation(ModelOperation::BOT_QUERY_SCHEDULES, &m_queryRequest, "");

    if (result) {
        auto* response = static_cast<BotQueryResponse*>(result);

        // Emit the structured response
        emit responseReady(*response);

        // For backward compatibility, also emit the legacy signal
        QString legacyMessage = QString::fromStdString(response->userMessage);
        emit responseReady(legacyMessage, -1);  // No schedule index in new format

        delete response;
    } else {
        BotQueryResponse errorResponse;
        errorResponse.hasError = true;
        errorResponse.errorMessage = "I'm sorry, I couldn't process your request. Please try rephrasing your question.";

        emit responseReady(errorResponse);
        emit errorOccurred(QString::fromStdString(errorResponse.errorMessage));
    }

    emit finished();
}

BotQueryResponse BotWorker::parseLegacyResponse(const std::vector<std::string>& responseVector) {
    BotQueryResponse response;

    if (!responseVector.empty()) {
        response.userMessage = responseVector[0];
        response.isFilterQuery = false;
        response.hasError = false;

        if (responseVector.size() >= 2) {
            const std::string& indexStr = responseVector[1];
            if (!indexStr.empty() && indexStr != "-1") {
                try {
                    int scheduleIndex = std::stoi(indexStr);
                } catch (const std::exception&) {
                    // Invalid index, ignore
                }
            }
        }
    } else {
        response.hasError = true;
        response.errorMessage = "Empty response received";
    }

    return response;
}