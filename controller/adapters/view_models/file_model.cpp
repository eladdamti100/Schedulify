#include "file_model.h"
#include "logger.h"

FileHistoryModel::FileHistoryModel(QObject* parent)
        : QAbstractListModel(parent) {
}

int FileHistoryModel::rowCount(const QModelIndex& parent) const {
    Q_UNUSED(parent)
    return static_cast<int>(m_files.size());
}

QVariant FileHistoryModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= static_cast<int>(m_files.size())) {
        return QVariant();
    }

    const FileEntity& file = m_files.at(index.row());

    switch (role) {
        case FileIdRole:
            return file.id;
        case FileNameRole:
            return QString::fromStdString(file.file_name);
        case FileTypeRole:
            return QString::fromStdString(file.file_type);
        case UploadTimeRole:
            return file.upload_time;
        case UpdatedAtRole:
            return file.updated_at;
        case FormattedDateRole:
            return file.upload_time.toString("MMM dd, yyyy hh:mm");
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> FileHistoryModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[FileIdRole] = "fileId";
    roles[FileNameRole] = "fileName";
    roles[FileTypeRole] = "fileType";
    roles[UploadTimeRole] = "uploadTime";
    roles[UpdatedAtRole] = "updatedAt";
    roles[FormattedDateRole] = "formattedDate";
    return roles;
}

void FileHistoryModel::populateFiles(const vector<FileEntity>& files) {
    beginResetModel();
    m_files = files;
    endResetModel();
}

void FileHistoryModel::clearFiles() {
    beginResetModel();
    m_files.clear();
    endResetModel();
}

int FileHistoryModel::getFileId(int index) const {
    if (index < 0 || index >= static_cast<int>(m_files.size())) {
        Logger::get().logError("Invalid index for getFileId: " + std::to_string(index));
        return -1;
    }

    int fileId = m_files.at(index).id;
    return fileId;
}

void FileHistoryModel::forceRefresh() {

    if (!m_files.empty()) {
        // Emit dataChanged for all items to force UI refresh
        QModelIndex topLeft = createIndex(0, 0);
        QModelIndex bottomRight = createIndex(static_cast<int>(m_files.size()) - 1, 0);
        emit dataChanged(topLeft, bottomRight);
    }
}