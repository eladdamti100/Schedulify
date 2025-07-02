#ifndef FILE_MODEL_H
#define FILE_MODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include "db_entities.h"

class FileHistoryModel : public QAbstractListModel {
Q_OBJECT

public:
    enum FileRoles {
        FileIdRole = Qt::UserRole + 1,
        FileNameRole,
        FileTypeRole,
        UploadTimeRole,
        UpdatedAtRole,
        FormattedDateRole
    };

    explicit FileHistoryModel(QObject* parent = nullptr);

    // QAbstractListModel interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Custom methods
    void populateFiles(const vector<FileEntity>& files);
    void clearFiles();
    int getFileId(int index) const;

    // Method to force refresh of UI elements
    Q_INVOKABLE void forceRefresh();

private:
    vector<FileEntity> m_files;
};

#endif // FILE_MODEL_H