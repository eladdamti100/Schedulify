#include "ScheduleBuilder.h"

using namespace std;

unordered_map<int, CourseInfo> ScheduleBuilder::courseInfoMap;
mutex ScheduleBuilder::courseInfoMapMutex;
string ScheduleBuilder::currentSemester;

// Generate schedule

vector<InformativeSchedule> ScheduleBuilder::build(const vector<Course>& courses, const string& semester) {
    Logger::get().logInfo("Starting schedule generation for " + to_string(courses.size()) +
                          " courses in semester " + semester);

    currentSemester = semester;
    vector<InformativeSchedule> results;
    totalSchedulesGenerated = 0;

    try {
        buildCourseInfoMap(courses);

        CourseLegalComb generator;
        vector<vector<CourseSelection>> allOptions;

        // Generate combinations for each course
        for (const auto& course : courses) {
            auto combinations = generator.generate(course);
            Logger::get().logInfo("Generated " + to_string(combinations.size()) +
                                  " combinations for course ID " + to_string(course.id));
            allOptions.push_back(std::move(combinations));
        }

        // SAFETY: Estimate and limit total possible schedules to prevent memory exhaustion
        long long estimatedTotal = 1;
        for (const auto& options : allOptions) {
            estimatedTotal *= options.size();
            if (estimatedTotal > 100000) {  // Reduced limit for safety
                Logger::get().logWarning("Estimated schedules (" + to_string(estimatedTotal) +
                                         ") exceeds safe limit. Generation may be limited.");
                break;
            }
        }

        Logger::get().logInfo("Estimated maximum schedules: " + to_string(estimatedTotal));

        // SAFETY: Reserve memory upfront and set hard limit
        try {
            results.reserve(min(estimatedTotal, 50000LL));  // Hard limit of 50k schedules
        } catch (const std::bad_alloc& e) {
            Logger::get().logError("Failed to reserve memory for schedules: " + string(e.what()));
            return results;  // Return empty if can't allocate
        }

        vector<CourseSelection> current;
        backtrack(0, allOptions, current, results);

        Logger::get().logInfo("Finished schedule generation for semester " + semester +
                              ". Total valid schedules: " + to_string(results.size()));

    } catch (const std::bad_alloc& e) {
        Logger::get().logError("Out of memory during schedule generation: " + string(e.what()));
        results.clear();  // Clear partial results to free memory
    } catch (const exception& e) {
        Logger::get().logError("Exception in ScheduleBuilder::build: " + string(e.what()));
    }

    return results;
}

void ScheduleBuilder::backtrack(int currentCourse, const vector<vector<CourseSelection>>& allOptions,
                                vector<CourseSelection>& currentCombination, vector<InformativeSchedule>& results) {
    try {
        // SAFETY: Check if we've generated too many schedules
        if (results.size() >= 50000) {  // Hard limit
            Logger::get().logWarning("Reached maximum schedule limit (50,000). Stopping generation.");
            return;
        }

        if (currentCourse == allOptions.size()) {
            InformativeSchedule schedule = convertToInformativeSchedule(currentCombination, results.size());
            results.push_back(schedule);
            totalSchedulesGenerated++;

            // Log progress for large generations
            if (totalSchedulesGenerated % 1000 == 0) {
                Logger::get().logInfo("Generated " + to_string(totalSchedulesGenerated) + " schedules so far...");
            }

            return;
        }

        for (const auto& option : allOptions[currentCourse]) {
            bool conflict = false;

            for (const auto& selected : currentCombination) {
                if (hasConflict(option, selected)) {
                    conflict = true;
                    break;
                }
            }

            if (!conflict) {
                currentCombination.push_back(option);
                backtrack(currentCourse + 1, allOptions, currentCombination, results);
                currentCombination.pop_back();
            }
        }
    } catch (const std::bad_alloc& e) {
        Logger::get().logError("Out of memory in backtrack: " + string(e.what()));
        return;  // Stop generation
    } catch (const exception& e) {
        Logger::get().logError("Exception in ScheduleBuilder::backtrack: " + string(e.what()));
    }
}

bool ScheduleBuilder::hasConflict(const CourseSelection& a, const CourseSelection& b) {
    vector<const Session*> aSessions = getSessions(a);
    vector<const Session*> bSessions = getSessions(b);

    // Compare each session in a with each session in b
    for (const auto* s1 : aSessions) {
        for (const auto* s2 : bSessions) {
            if (TimeUtils::isOverlap(s1, s2)) return true;
        }
    }
    return false;
}

// Course map helpers

void ScheduleBuilder::buildCourseInfoMap(const vector<Course>& courses) {
    std::lock_guard<std::mutex> lock(courseInfoMapMutex);
    courseInfoMap.clear();
    for (const auto& course : courses) {
        courseInfoMap[course.id] = {course.raw_id, course.name};
    }
}

string ScheduleBuilder::getCourseNameById(int courseId) {
    lock_guard<mutex> lock(courseInfoMapMutex);
    auto it = courseInfoMap.find(courseId);
    if (it != courseInfoMap.end()) {
        return it->second.name;
    }
    return "Unknown Course";
}

string ScheduleBuilder::getCourseRawIdById(int courseId) {
    lock_guard<mutex> lock(courseInfoMapMutex);
    auto it = courseInfoMap.find(courseId);
    if (it != courseInfoMap.end()) {
        return it->second.raw_id;
    }
    return "Unknown ID";
}

// Convert to informative schedule and calculate metadata

InformativeSchedule ScheduleBuilder::convertToInformativeSchedule(const vector<CourseSelection>& selections, int index) {
    InformativeSchedule schedule;
    schedule.index = index;
    schedule.semester = currentSemester;

    // NEW: Generate unique ID for this schedule
    schedule.unique_id = generateUniqueScheduleId(currentSemester, index);

    try {
        map<int, vector<ScheduleItem>> daySchedules;
        const vector<string> dayNames = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

        for (const auto& selection : selections) {
            if (selection.lectureGroup) {
                processGroupSessions(selection, selection.lectureGroup, "Lecture", daySchedules);
            }

            if (selection.tutorialGroup) {
                processGroupSessions(selection, selection.tutorialGroup, "Tutorial", daySchedules);
            }

            if (selection.labGroup) {
                processGroupSessions(selection, selection.labGroup, "Lab", daySchedules);
            }

            if (selection.blockGroup) {
                processGroupSessions(selection, selection.blockGroup, "Block", daySchedules);
            }

            if (selection.departmentalGroup) {
                processGroupSessions(selection, selection.departmentalGroup, "Departmental", daySchedules);
            }

            if (selection.reinforcementGroup) {
                processGroupSessions(selection, selection.reinforcementGroup, "Reinforcement", daySchedules);
            }

            if (selection.guidanceGroup) {
                processGroupSessions(selection, selection.guidanceGroup, "Guidance", daySchedules);
            }

            if (selection.colloquiumGroup) {
                processGroupSessions(selection, selection.colloquiumGroup, "Colloquium", daySchedules);
            }

            if (selection.registrationGroup) {
                processGroupSessions(selection, selection.registrationGroup, "Registration", daySchedules);
            }

            if (selection.thesisGroup) {
                processGroupSessions(selection, selection.thesisGroup, "Thesis", daySchedules);
            }

            if (selection.projectGroup) {
                processGroupSessions(selection, selection.projectGroup, "Project", daySchedules);
            }
        }

        // Build schedule days
        for (int day = 0; day < 7; day++) {
            ScheduleDay scheduleDay;
            scheduleDay.day = dayNames[day];

            auto it = daySchedules.find(day +1);
            if (it != daySchedules.end()) {
                scheduleDay.day_items = it->second;
                sort(scheduleDay.day_items.begin(), scheduleDay.day_items.end(),
                     [](const ScheduleItem& a, const ScheduleItem& b) {
                         return a.start < b.start;
                     });
            }

            schedule.week.push_back(scheduleDay);
        }

        calculateScheduleMetrics(schedule);

    } catch (const exception& e) {
        Logger::get().logError("Exception in convertToInformativeSchedule: " + string(e.what()));

        // Create empty schedule on error
        schedule.week.clear();
        const vector<string> dayNames = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
        for (int day = 0; day < 7; day++) {
            ScheduleDay scheduleDay;
            scheduleDay.day = dayNames[day];
            schedule.week.push_back(scheduleDay);
        }

        calculateScheduleMetrics(schedule);
    }

    return schedule;
}

void ScheduleBuilder::processGroupSessions(const CourseSelection& selection, const Group* group,
                                           const string& sessionType, map<int, vector<ScheduleItem>>& daySchedules) {
    if (!group) return;

    try {
        string courseName = getCourseNameById(selection.courseId);
        string courseRawId = getCourseRawIdById(selection.courseId);

        for (const auto& session : group->sessions) {

            ScheduleItem item;
            item.courseName = courseName;
            item.raw_id = courseRawId;
            item.type = sessionType;
            item.start = session.start_time;
            item.end = session.end_time;
            item.building = session.building_number;
            item.room = session.room_number;

            daySchedules[session.day_of_week].push_back(item);
        }

    } catch (const exception& e) {
        Logger::get().logError("Exception in processGroupSessions: " + string(e.what()));
    }
}

void ScheduleBuilder::calculateScheduleMetrics(InformativeSchedule& schedule) {
    // Initialize all metrics
    int totalDaysWithItems = 0;
    int totalGaps = 0;
    int totalGapTime = 0;
    int totalStartTime = 0;
    int totalEndTime = 0;

    // NEW: Enhanced metrics
    int earliestStart = INT_MAX;
    int latestEnd = 0;
    int longestGap = 0;
    int totalClassTime = 0;
    int consecutiveDays = 0;
    int maxDailyHours = 0;
    int minDailyHours = INT_MAX;
    int totalDailyHours = 0;
    int maxDailyGaps = 0;
    int totalGapsForAvg = 0;
    int gapCount = 0;
    int scheduleSpan = 0;

    // Boolean flags
    bool hasEarlyMorning = false;
    bool hasMorning = false;
    bool hasEvening = false;
    bool hasLateEvening = false;
    bool hasLunchBreak = false;
    bool weekendClasses = false;

    // Day tracking
    std::vector<int> daysWithClasses;
    std::vector<bool> dayHasClasses(8, false);

    try {
        for (int dayIndex = 0; dayIndex < static_cast<int>(schedule.week.size()); dayIndex++) {
            const auto& scheduleDay = schedule.week[dayIndex];

            if (scheduleDay.day_items.empty()) {
                continue;
            }

            totalDaysWithItems++;
            int algorithmDay = dayIndex + 1;
            daysWithClasses.push_back(algorithmDay);
            dayHasClasses[algorithmDay] = true;

            // Check for weekend classes (Saturday=7, Sunday=1)
            if (algorithmDay == 1 || algorithmDay == 7) {
                weekendClasses = true;
            }

            // Daily calculations
            int dayStartMinutes = TimeUtils::toMinutes(scheduleDay.day_items.front().start);
            int dayEndMinutes = TimeUtils::toMinutes(scheduleDay.day_items.back().end);
            int dailyClassTime = 0;
            int dailyGaps = 0;

            // Update global earliest/latest
            earliestStart = std::min(earliestStart, dayStartMinutes);
            latestEnd = std::max(latestEnd, dayEndMinutes);

            totalStartTime += dayStartMinutes;
            totalEndTime += dayEndMinutes;

            // Check time preferences
            if (dayStartMinutes < 510) hasEarlyMorning = true;  // Before 8:30 AM
            if (dayStartMinutes < 600) hasMorning = true;       // Before 10:00 AM
            if (dayEndMinutes > 1080) hasEvening = true;        // After 6:00 PM
            if (dayEndMinutes > 1200) hasLateEvening = true;    // After 8:00 PM

            // Calculate daily class time and gaps
            for (size_t i = 0; i < scheduleDay.day_items.size(); i++) {
                const auto& item = scheduleDay.day_items[i];
                int itemStart = TimeUtils::toMinutes(item.start);
                int itemEnd = TimeUtils::toMinutes(item.end);
                dailyClassTime += (itemEnd - itemStart);

                // Check for gaps
                if (i < scheduleDay.day_items.size() - 1) {
                    int currentEndMinutes = itemEnd;
                    int nextStartMinutes = TimeUtils::toMinutes(scheduleDay.day_items[i + 1].start);
                    int gapDuration = nextStartMinutes - currentEndMinutes;

                    if (gapDuration >= 30) {  // 30+ minute gap
                        totalGaps++;
                        dailyGaps++;
                        totalGapTime += gapDuration;
                        totalGapsForAvg += gapDuration;
                        gapCount++;
                        longestGap = std::max(longestGap, gapDuration);

                        // Check for lunch break (gap overlapping 12:00-14:00)
                        if (currentEndMinutes <= 840 && nextStartMinutes >= 720) { // 12:00-14:00 range
                            hasLunchBreak = true;
                        }
                    }
                }
            }

            totalClassTime += dailyClassTime;
            maxDailyGaps = std::max(maxDailyGaps, dailyGaps);

            // Daily hours calculation (convert minutes to hours, rounded)
            int dailyHours = (dailyClassTime + 30) / 60; // Round to nearest hour
            maxDailyHours = std::max(maxDailyHours, dailyHours);
            if (totalDaysWithItems == 1) {
                minDailyHours = dailyHours;
            } else {
                minDailyHours = std::min(minDailyHours, dailyHours);
            }
            totalDailyHours += dailyHours;
        }

        // Calculate consecutive days
        if (!daysWithClasses.empty()) {
            std::sort(daysWithClasses.begin(), daysWithClasses.end());
            int currentStreak = 1;
            int maxStreak = 1;

            for (size_t i = 1; i < daysWithClasses.size(); i++) {
                if (daysWithClasses[i] == daysWithClasses[i-1] + 1) {
                    currentStreak++;
                    maxStreak = std::max(maxStreak, currentStreak);
                } else {
                    currentStreak = 1;
                }
            }
            consecutiveDays = maxStreak;
        }

        // Calculate schedule span and compactness
        if (earliestStart != INT_MAX && latestEnd > 0) {
            scheduleSpan = latestEnd - earliestStart;
        }

        // Set basic metrics
        schedule.amount_days = totalDaysWithItems;
        schedule.amount_gaps = totalGaps;
        schedule.gaps_time = totalGapTime;

        // Calculate averages
        if (totalDaysWithItems > 0) {
            schedule.avg_start = totalStartTime / totalDaysWithItems;
            schedule.avg_end = totalEndTime / totalDaysWithItems;
        } else {
            schedule.avg_start = 0;
            schedule.avg_end = 0;
        }

        // NEW: Set enhanced metrics
        schedule.earliest_start = (earliestStart == INT_MAX) ? 0 : earliestStart;
        schedule.latest_end = latestEnd;
        schedule.longest_gap = longestGap;
        schedule.total_class_time = totalClassTime;
        schedule.consecutive_days = consecutiveDays;
        schedule.max_daily_hours = maxDailyHours;
        schedule.min_daily_hours = (minDailyHours == INT_MAX) ? 0 : minDailyHours;
        schedule.avg_daily_hours = totalDaysWithItems > 0 ? totalDailyHours / totalDaysWithItems : 0;
        schedule.max_daily_gaps = maxDailyGaps;
        schedule.avg_gap_length = gapCount > 0 ? totalGapsForAvg / gapCount : 0;
        schedule.schedule_span = scheduleSpan;
        schedule.compactness_ratio = scheduleSpan > 0 ? static_cast<double>(totalClassTime) / scheduleSpan : 0.0;

        // Boolean flags
        schedule.has_early_morning = hasEarlyMorning;
        schedule.has_morning_classes = hasMorning;
        schedule.has_evening_classes = hasEvening;
        schedule.has_late_evening = hasLateEvening;
        schedule.has_lunch_break = hasLunchBreak;
        schedule.weekend_classes = weekendClasses;
        schedule.weekday_only = !weekendClasses && totalDaysWithItems > 0;

        // Individual day flags
        schedule.has_sunday = dayHasClasses[1];
        schedule.has_monday = dayHasClasses[2];
        schedule.has_tuesday = dayHasClasses[3];
        schedule.has_wednesday = dayHasClasses[4];
        schedule.has_thursday = dayHasClasses[5];
        schedule.has_friday = dayHasClasses[6];
        schedule.has_saturday = dayHasClasses[7];

        // Days JSON array
        schedule.days_json = "[";
        for (size_t i = 0; i < daysWithClasses.size(); i++) {
            if (i > 0) schedule.days_json += ",";
            schedule.days_json += std::to_string(daysWithClasses[i]);
        }
        schedule.days_json += "]";

    } catch (const exception& e) {
        Logger::get().logError("Exception in enhanced calculateScheduleMetrics: " + string(e.what()));
        // Set safe defaults on error
        schedule.amount_days = 0;
        schedule.amount_gaps = 0;
        schedule.gaps_time = 0;
        schedule.avg_start = 0;
        schedule.avg_end = 0;
        // Set all new fields to safe defaults
        schedule.earliest_start = 0;
        schedule.latest_end = 0;
        schedule.longest_gap = 0;
        schedule.total_class_time = 0;
        schedule.consecutive_days = 0;
        schedule.max_daily_hours = 0;
        schedule.min_daily_hours = 0;
        schedule.avg_daily_hours = 0;
        schedule.max_daily_gaps = 0;
        schedule.avg_gap_length = 0;
        schedule.schedule_span = 0;
        schedule.compactness_ratio = 0.0;
        schedule.has_early_morning = false;
        schedule.has_morning_classes = false;
        schedule.has_evening_classes = false;
        schedule.has_late_evening = false;
        schedule.has_lunch_break = false;
        schedule.weekend_classes = false;
        schedule.weekday_only = false;
        schedule.has_monday = false;
        schedule.has_tuesday = false;
        schedule.has_wednesday = false;
        schedule.has_thursday = false;
        schedule.has_friday = false;
        schedule.has_saturday = false;
        schedule.has_sunday = false;
        schedule.days_json = "[]";
    }
}

string ScheduleBuilder::generateUniqueScheduleId(const string& semester, int index) {
    // Simple approach: semester + timestamp + index + random component
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    int randomComponent = dis(gen);

    std::stringstream ss;
    ss << semester << "_" << timestamp << "_" << index << "_" << randomComponent;
    return ss.str();
}
