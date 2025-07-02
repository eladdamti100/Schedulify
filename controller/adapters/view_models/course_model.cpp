#include "course_model.h"

CourseModel::CourseModel(QObject* parent)
        : QAbstractListModel(parent){}

int CourseModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_courses.size());
}

QVariant CourseModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= static_cast<int>(m_courses.size()))
        return {};

    const CourseM& course = m_courses.at(index.row());

    switch (role) {
        case CourseIdRole:
            return course.id;
        case CourseNameRole:
            return course.name;
        case TeacherNameRole:
            return course.teacherName;
        case SemesterRole:
            return course.semester;
        case SemesterDisplayRole:
            return course.semesterDisplay;
        case OriginalIndexRole:
            // If we have original indices, return them; otherwise, return the current index
            if (!m_originalIndices.empty()) {
                return m_originalIndices.at(index.row());
            } else {
                return index.row();
            }
        default:
            return {};
    }
}

QHash<int, QByteArray> CourseModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[CourseIdRole] = "courseId";
    roles[CourseNameRole] = "courseName";
    roles[TeacherNameRole] = "teacherName";
    roles[SemesterRole] = "semester";
    roles[SemesterDisplayRole] = "semesterDisplay";
    roles[IsSelectedRole] = "isSelected";
    roles[OriginalIndexRole] = "originalIndex";
    return roles;
}

QString CourseModel::getSemesterDisplay(int semesterCode) const
{
    switch (semesterCode) {
        case 1: return "SEM A";
        case 2: return "SEM B";
        case 3: return "SUMMER";
        case 4: return "YEAR";
        default: return "SEM A";
    }
}

void CourseModel::populateCoursesData(const vector<Course>& courses, const vector<int>& originalIndices)
{
    beginResetModel();
    m_courses.clear();

    for (const Course& course : courses) {
        m_courses.emplace_back(
                QString::fromStdString(course.raw_id),
                QString::fromStdString(course.name),
                QString::fromStdString(course.teacher),
                course.semester,
                getSemesterDisplay(course.semester)
        );
    }

    // If originalIndices is provided and has the same size as courses, use it
    if (!originalIndices.empty() && originalIndices.size() == courses.size()) {
        m_originalIndices = originalIndices;
    } else {
        // Otherwise, create a default mapping (index -> index)
        m_originalIndices.clear();
        for (size_t i = 0; i < courses.size(); ++i) {
            m_originalIndices.push_back(static_cast<int>(i));
        }
    }

    endResetModel();
}