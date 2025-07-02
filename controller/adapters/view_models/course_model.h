#ifndef COURSE_MODEL_H
#define COURSE_MODEL_H

#include "model_interfaces.h"

#include <QAbstractListModel>
#include <utility>
#include <vector>
#include <string>

// Course structure definition
struct CourseM {
    QString id;
    QString name;
    QString teacherName;
    int semester;
    QString semesterDisplay;

    CourseM(QString id, QString name, QString teacherName, int semester, QString semesterDisplay)
            : id(std::move(id))
            , name(std::move(name))
            , teacherName(std::move(teacherName))
            , semester(semester)
            , semesterDisplay(std::move(semesterDisplay)) {}
};

// Qt list model for courses
class CourseModel : public QAbstractListModel {
Q_OBJECT

public:
    // Define roles for the model
    enum CourseRoles {
        CourseIdRole = Qt::UserRole + 1,
        CourseNameRole,
        TeacherNameRole,
        SemesterRole,
        SemesterDisplayRole,
        IsSelectedRole,
        OriginalIndexRole
    };

    explicit CourseModel(QObject* parent = nullptr);

    // Override methods from QAbstractListModel
    [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    // Method to populate the model with sample data
    void populateCoursesData(const vector<Course>& courses, const vector<int>& originalIndices = {});

private:
    vector<CourseM> m_courses;
    vector<int> m_originalIndices;

    // Helper method to convert semester code to display string
    QString getSemesterDisplay(int semesterCode) const;
};

#endif // COURSE_MODEL_H