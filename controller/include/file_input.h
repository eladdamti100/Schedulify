#ifndef FILE_INPUT_H
#define FILE_INPUT_H

#include "controller_manager.h"
#include "model_access.h"
#include "model_interfaces.h"
#include "course_selection.h"
#include "file_model.h"
#include "logger.h"

#include <QFileDialog>
#include <QStandardPaths>
#include <algorithm>

class FileInputController : public ControllerManager {
Q_OBJECT

    Q_PROPERTY(FileHistoryModel* fileHistoryModel READ fileHistoryModel CONSTANT)
    Q_PROPERTY(int selectedFileCount READ selectedFileCount NOTIFY fileSelectionChanged)

public:
    explicit FileInputController(QObject *parent = nullptr);
    ~FileInputController() override;

    FileHistoryModel* fileHistoryModel() const { return m_fileHistoryModel; }
    int selectedFileCount() const;

public slots:
    void handleUploadAndContinue();
    void handleFileSelected(const QString &filePath);

signals:
    void invalidFileFormat();
    void errorMessage(const QString &message);
    void fileSelected(bool hasFile);
    void fileNameChanged(const QString &fileName);
    void fileSelectionChanged();

public:
    Q_INVOKABLE void loadNewFile();
    Q_INVOKABLE void loadFromHistory();
    Q_INVOKABLE void toggleFileSelection(int index);
    Q_INVOKABLE bool isFileSelected(int index);
    Q_INVOKABLE void clearFileSelection();
    Q_INVOKABLE void refreshFileHistory(); // Force refresh file history
    Q_INVOKABLE void deleteFileFromHistory(int fileId); // Delete file from history

private:
    QString selectedFilePath;
    IModel* modelConnection;
    FileHistoryModel* m_fileHistoryModel;
    vector<int> m_selectedFileIds;

    void loadFileHistory();
    void proceedWithCourses(const vector<Course>& courses);

    // Helper methods for debugging
    void validateFileSelection();
};

#endif // FILE_INPUT_H