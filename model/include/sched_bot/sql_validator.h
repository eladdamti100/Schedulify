#ifndef SQL_VALIDATOR_H
#define SQL_VALIDATOR_H

#include "logger.h"

#include <string>
#include <vector>
#include <set>
#include <QString>
#include <QRegularExpression>
#include <QString>
#include <algorithm>
#include <cctype>

class SQLValidator {
public:
    struct ValidationResult {
        bool isValid;
        std::string errorMessage;
        std::vector<std::string> warnings;

        ValidationResult(bool valid = true) : isValid(valid) {}
    };

    // Main validation methods
    static ValidationResult validateScheduleQuery(const std::string& sqlQuery);

    // Security checks
    static bool containsForbiddenKeywords(const std::string& query);
    static bool isSelectOnlyQuery(const std::string& query);
    static bool usesWhitelistedTablesOnly(const std::string& query);
    static bool usesWhitelistedColumnsOnly(const std::string& query);

    // Query analysis
    static bool requiresScheduleIdentifier(const std::string& query);
    static int countParameters(const std::string& query);
    static std::vector<std::string> extractTableNames(const std::string& query);
    static std::vector<std::string> extractColumnNames(const std::string& query);

    // Configuration
    static std::vector<std::string> getForbiddenKeywords();
    static std::vector<std::string> getWhitelistedTables();
    static std::vector<std::string> getWhitelistedColumns();

    // Utility methods
    static std::string sanitizeQuery(const std::string& query);
    static std::string normalizeQuery(const std::string& query);

private:
    SQLValidator() = default; // Static class
};

#endif // SQL_VALIDATOR_H