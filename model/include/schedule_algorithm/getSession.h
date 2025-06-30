#ifndef GET_SESSION_H
#define GET_SESSION_H

#pragma once

#include "model_interfaces.h"
#include "inner_structs.h"

#include <vector>

inline vector<const Session*> getSessions(const CourseSelection& selection) {
    vector<const Session*> sessions;

    try {
        // Add sessions from lecture group
        if (selection.lectureGroup) {
            for (const auto& session : selection.lectureGroup->sessions) {
                sessions.push_back(&session);
            }
        }

        // Add sessions from tutorial group
        if (selection.tutorialGroup) {
            for (const auto& session : selection.tutorialGroup->sessions) {
                sessions.push_back(&session);
            }
        }

        // Add sessions from lab group
        if (selection.labGroup) {
            for (const auto& session : selection.labGroup->sessions) {
                sessions.push_back(&session);
            }
        }

        // Add sessions from block group
        if (selection.blockGroup) {
            for (const auto& session : selection.blockGroup->sessions) {
                sessions.push_back(&session);
            }
        }

        // Add sessions from departmental group
        if (selection.departmentalGroup) {
            for (const auto& session : selection.departmentalGroup->sessions) {
                sessions.push_back(&session);
            }
        }

        // Add sessions from reinforcement group
        if (selection.reinforcementGroup) {
            for (const auto& session : selection.reinforcementGroup->sessions) {
                sessions.push_back(&session);
            }
        }

        // Add sessions from guidance group
        if (selection.guidanceGroup) {
            for (const auto& session : selection.guidanceGroup->sessions) {
                sessions.push_back(&session);
            }
        }

        // Add sessions from colloquium group
        if (selection.colloquiumGroup) {
            for (const auto& session : selection.colloquiumGroup->sessions) {
                sessions.push_back(&session);
            }
        }

        // Add sessions from registration group
        if (selection.registrationGroup) {
            for (const auto& session : selection.registrationGroup->sessions) {
                sessions.push_back(&session);
            }
        }

        // Add sessions from thesis group
        if (selection.thesisGroup) {
            for (const auto& session : selection.thesisGroup->sessions) {
                sessions.push_back(&session);
            }
        }

        // Add sessions from project group
        if (selection.projectGroup) {
            for (const auto& session : selection.projectGroup->sessions) {
                sessions.push_back(&session);
            }
        }

    } catch (const exception& e) {
        Logger::get().logError("Exception in getSessions: " + string(e.what()));
    }

    return sessions;
}


#endif //GET_SESSION_H