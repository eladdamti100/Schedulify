#include "model_db_integration.h"
#include <QQmlApplicationEngine>
#include "main_controller.h"
#include "cleanup_manager.h"
#include <QApplication>
#include "db_manager.h"
#include <QQmlContext>
#include "logger.h"
#include <QUrl>

// !!only add to code when need to reset db!!
void forceCleanDatabaseStart() {
    Logger::get().logInfo("=== FORCING CLEAN DATABASE START ===");

    // Find database file location
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString dbPath = QDir(appDataPath).filePath("schedulify.db");

    // Delete existing database file
    if (QFile::exists(dbPath)) {
        if (QFile::remove(dbPath)) {
            Logger::get().logInfo("Existing database file deleted: " + dbPath.toStdString());
        } else {
            Logger::get().logWarning("Failed to delete existing database file");
        }
    } else {
        Logger::get().logInfo("No existing database file found");
    }

    // Also remove any journal/wal files
    QFile::remove(dbPath + "-wal");
    QFile::remove(dbPath + "-shm");
    QFile::remove(dbPath + "-journal");

    Logger::get().logInfo("Database reset complete - will create fresh v1 schema");
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Logger::get().logInitiate();
    Logger::get().logInfo("Application started - Qt initialized");

    try {
        auto& dbIntegration = ModelDatabaseIntegration::getInstance();
        if (!dbIntegration.initializeDatabase()) {
            Logger::get().logWarning("Database initialization failed - continuing without persistence");
        } else {
            Logger::get().logInfo("Database initialized successfully");

            auto& db = DatabaseManager::getInstance();
            if (db.isConnected() && db.schedules()) {
                Logger::get().logInfo("Schedule database ready for use");
            } else {
                Logger::get().logError("Schedule database not properly initialized");
            }
        }
    } catch (const std::exception& e) {
        Logger::get().logWarning("Database initialization exception: " + std::string(e.what()));
    }

    // Create the QQmlApplicationEngine
    QQmlApplicationEngine engine;

    // Create the ButtonController instance and pass the engine
    MainController controller(&engine);

    // Set the static reference to the main controller for all ControllerManager instances
    ControllerManager::setMainController(&controller);

    // Register the ButtonController with QML
    engine.rootContext()->setContextProperty("controller", &controller);

    engine.rootContext()->setContextProperty("Logger", &Logger::get());

    // Load the QML file from resources
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    // Check for errors
    if (engine.rootObjects().isEmpty()) {
        Logger::get().logError("Error loading main qml (view) file");
        return -1;
    }

    // Setup cleanup on application exit - use aboutToQuit for earlier cleanup
    QObject::connect(&app, &QApplication::aboutToQuit, []() {
        Logger::get().logInfo("Application about to quit - starting cleanup");
        CleanupManager::performCleanup();

        // Process any remaining events to ensure cleanup completes
        QCoreApplication::processEvents();

        Logger::get().logInfo("Cleanup signal processing completed");
    });

    Logger::get().logInfo("Starting application event loop");
    int result = app.exec();
    Logger::get().logInfo("Application event loop finished with code: " + std::to_string(result));

    return result;
}