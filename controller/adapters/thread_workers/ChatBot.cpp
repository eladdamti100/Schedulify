#include "ChatBot.h"


BotWorker::BotWorker(IModel* model, BotQueryRequest queryRequest, QObject* parent)
        : QObject(parent), m_model(model), m_queryRequest(std::move(queryRequest)) {}

void BotWorker::processMessage() {
    try {
        if (!m_model) {
            emit errorOccurred("Model connection not available");
            emit finished();
            return;
        }

        processBotQuery();

    } catch (const std::exception& e) {
        emit errorOccurred("An error occurred while processing your request. Please try again.");
        emit finished();
    } catch (...) {
        emit errorOccurred("An unexpected error occurred. Please try again.");
        emit finished();
    }
}

void BotWorker::processBotQuery() {
    try {
        // UPDATED: Use BOT_QUERY_SCHEDULES operation with BotQueryRequest
        void* result = m_model->executeOperation(ModelOperation::BOT_QUERY_SCHEDULES, &m_queryRequest, "");

        if (result) {
            auto* response = static_cast<BotQueryResponse*>(result);

            // Emit the structured response
            emit responseReady(*response);

            // Clean up
            delete response;

        } else {

            // Create error response
            BotQueryResponse errorResponse;
            errorResponse.hasError = true;
            errorResponse.errorMessage = "I'm sorry, I couldn't process your request. Please try rephrasing your question.";
            errorResponse.isFilterQuery = false;

            emit responseReady(errorResponse);
            emit errorOccurred(QString::fromStdString(errorResponse.errorMessage));
        }

    } catch (const std::exception& e) {

        BotQueryResponse errorResponse;
        errorResponse.hasError = true;
        errorResponse.errorMessage = "An error occurred while processing your query: " + std::string(e.what());
        errorResponse.isFilterQuery = false;

        emit responseReady(errorResponse);
        emit errorOccurred(QString::fromStdString(errorResponse.errorMessage));

    } catch (...) {

        BotQueryResponse errorResponse;
        errorResponse.hasError = true;
        errorResponse.errorMessage = "An unexpected error occurred while processing your query.";
        errorResponse.isFilterQuery = false;

        emit responseReady(errorResponse);
        emit errorOccurred(QString::fromStdString(errorResponse.errorMessage));
    }

    emit finished();
}