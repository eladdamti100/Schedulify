#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include "parseCoursesToVector.h"
#include "model_interfaces.h"
#include "logger.h"

#include <string>

class TimeUtils {
public:
    static int toMinutes(const std::string& time);
    static bool isOverlap(const Session* s1, const Session* s2);
};
#endif // TIME_UTILS_H