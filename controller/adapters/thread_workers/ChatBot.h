#ifndef CHAT_BOT_H
#define CHAT_BOT_H

#include "model_interfaces.h"

#include <QObject>
#include <QThread>
#include <QString>
#include <vector>
#include <string>
#include <QDebug>
#include <iostream>
#include <utility>

class BotWorker : public QObject {
Q_OBJECT

public:
    explicit BotWorker(IModel* model, BotQueryRequest queryRequest, QObject* parent = nullptr);

public slots:
    void processMessage();

signals:
    void responseReady(const BotQueryResponse& response);
    void errorOccurred(const QString& errorMessage);
    void finished();

private:
    IModel* m_model;
    BotQueryRequest m_queryRequest;

    void processBotQuery();
};

#endif // CHAT_BOT_H