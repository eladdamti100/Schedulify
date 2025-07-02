#include "sql_validator.h"

SQLValidator::ValidationResult SQLValidator::validateScheduleQuery(const std::string& sqlQuery) {
    ValidationResult result;

    if (sqlQuery.empty()) {
        result.isValid = false;
        result.errorMessage = "SQL query cannot be empty";
        return result;
    }

    std::string normalizedQuery = normalizeQuery(sqlQuery);

    if (!isSelectOnlyQuery(normalizedQuery)) {
        result.isValid = false;
        result.errorMessage = "Only SELECT queries are allowed";
        return result;
    }

    if (containsForbiddenKeywords(normalizedQuery)) {
        result.isValid = false;
        result.errorMessage = "Query contains forbidden keywords";
        return result;
    }

    if (!usesWhitelistedTablesOnly(normalizedQuery)) {
        result.isValid = false;
        result.errorMessage = "Query uses non-whitelisted tables";
        return result;
    }

    if (!usesWhitelistedColumnsOnly(normalizedQuery)) {
        result.isValid = false;
        result.errorMessage = "Query uses non-whitelisted columns";
        return result;
    }

    if (!requiresScheduleIdentifier(normalizedQuery)) {
        result.isValid = false;
        result.errorMessage = "Query must SELECT unique_id or schedule_index column";
        return result;
    }

    int paramCount = countParameters(normalizedQuery);
    if (paramCount > 10) {
        result.warnings.push_back("Query has many parameters (" + std::to_string(paramCount) + ")");
    }

    return result;
}

bool SQLValidator::containsForbiddenKeywords(const std::string& query) {
    std::vector<std::string> forbidden = getForbiddenKeywords();
    QString qQuery = QString::fromStdString(query).toLower();

    for (const std::string& keyword : forbidden) {
        QString qKeyword = QString::fromStdString(keyword);
        // Use word boundaries to avoid false positives
        QRegularExpression regex("\\b" + qKeyword + "\\b");
        if (regex.match(qQuery).hasMatch()) {
            return true;
        }
    }

    return false;
}

bool SQLValidator::isSelectOnlyQuery(const std::string& query) {
    QString qQuery = QString::fromStdString(query).trimmed().toLower();

    // Must start with SELECT
    if (!qQuery.startsWith("select")) {
        return false;
    }

    // Should not contain other statement keywords
    std::vector<std::string> statementKeywords = {
            "insert", "update", "delete", "create", "drop", "alter",
            "truncate", "merge", "replace", "call"
    };

    for (const std::string& keyword : statementKeywords) {
        QRegularExpression regex("\\b" + QString::fromStdString(keyword) + "\\b");
        if (regex.match(qQuery).hasMatch()) {
            return false;
        }
    }

    return true;
}

bool SQLValidator::usesWhitelistedTablesOnly(const std::string& query) {
    std::vector<std::string> tables = extractTableNames(query);
    std::vector<std::string> whitelist = getWhitelistedTables();

    for (const std::string& table : tables) {
        if (std::find(whitelist.begin(), whitelist.end(), table) == whitelist.end()) {
            return false;
        }
    }

    return true;
}

bool SQLValidator::usesWhitelistedColumnsOnly(const std::string& query) {
    std::vector<std::string> columns = extractColumnNames(query);
    std::vector<std::string> whitelist = getWhitelistedColumns();

    for (const std::string& column : columns) {
        if (column == "*") {
            // Wildcard not allowed for security
            return false;
        }

        if (std::find(whitelist.begin(), whitelist.end(), column) == whitelist.end()) {
            return false;
        }
    }

    return true;
}

bool SQLValidator::requiresScheduleIdentifier(const std::string& query) {
    QString qQuery = QString::fromStdString(query).toLower();

    // Check if unique_id or schedule_index is in the SELECT clause
    QRegularExpression selectRegex(R"(select\s+(.+?)\s+from)", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = selectRegex.match(qQuery);

    if (!match.hasMatch()) {
        return false;
    }

    QString selectClause = match.captured(1);

    // Prefer unique_id, but accept schedule_index for backward compatibility
    return selectClause.contains("unique_id") || selectClause.contains("schedule_index");
}

int SQLValidator::countParameters(const std::string& query) {
    return std::count(query.begin(), query.end(), '?');
}

std::vector<std::string> SQLValidator::extractTableNames(const std::string& query) {
    std::vector<std::string> tables;
    QString qQuery = QString::fromStdString(query).toLower();

    // Simple extraction using FROM clause
    QRegularExpression fromRegex(R"(from\s+(\w+))");
    QRegularExpressionMatchIterator it = fromRegex.globalMatch(qQuery);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        std::string tableName = match.captured(1).toStdString();
        if (std::find(tables.begin(), tables.end(), tableName) == tables.end()) {
            tables.push_back(tableName);
        }
    }

    // Also check JOIN clauses
    QRegularExpression joinRegex(R"(join\s+(\w+))");
    it = joinRegex.globalMatch(qQuery);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        std::string tableName = match.captured(1).toStdString();
        if (std::find(tables.begin(), tables.end(), tableName) == tables.end()) {
            tables.push_back(tableName);
        }
    }

    return tables;
}

std::vector<std::string> SQLValidator::extractColumnNames(const std::string& query) {
    std::vector<std::string> columns;
    QString qQuery = QString::fromStdString(query).toLower();

    // Extract from SELECT clause
    QRegularExpression selectRegex(R"(select\s+(.+?)\s+from)", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = selectRegex.match(qQuery);

    if (match.hasMatch()) {
        QString selectClause = match.captured(1);
        QStringList columnList = selectClause.split(',');

        for (QString column : columnList) {
            column = column.trimmed();
            // Remove table prefixes (e.g., "table.column" -> "column")
            if (column.contains('.')) {
                column = column.split('.').last();
            }
            // Remove alias (e.g., "column AS alias" -> "column")
            if (column.contains(" as ")) {
                column = column.split(" as ").first().trimmed();
            }

            std::string columnName = column.toStdString();
            if (!columnName.empty() && std::find(columns.begin(), columns.end(), columnName) == columns.end()) {
                columns.push_back(columnName);
            }
        }
    }

    // Extract from WHERE clause
    QRegularExpression whereRegex(R"(where\s+(.+?)(?:\s+order|\s+group|\s+limit|$))", QRegularExpression::DotMatchesEverythingOption);
    match = whereRegex.match(qQuery);

    if (match.hasMatch()) {
        QString whereClause = match.captured(1);
        // Extract column names from conditions (simplified)
        QRegularExpression columnRegex(R"(\b(\w+)\s*[=<>!])");
        QRegularExpressionMatchIterator it = columnRegex.globalMatch(whereClause);

        while (it.hasNext()) {
            QRegularExpressionMatch columnMatch = it.next();
            std::string columnName = columnMatch.captured(1).toStdString();
            if (std::find(columns.begin(), columns.end(), columnName) == columns.end()) {
                columns.push_back(columnName);
            }
        }
    }

    return columns;
}

std::vector<std::string> SQLValidator::getForbiddenKeywords() {
    return {
            // Write operations
            "insert", "update", "delete", "drop", "create", "alter",
            "truncate", "grant", "revoke", "merge", "replace",

            // Execution and procedural
            "exec", "execute", "call", "do", "handler",
            "declare", "prepare", "deallocate",

            // Data manipulation that could be dangerous
            "union", "into", "outfile", "dumpfile", "load",

            // System and schema operations
            "show", "describe", "explain", "analyze", "check",
            "checksum", "optimize", "repair", "backup", "restore",

            // Security-sensitive operations
            "user", "password", "privilege", "role",

            // File and system operations
            "file", "directory", "path", "system"
    };
}

std::vector<std::string> SQLValidator::getWhitelistedTables() {
    return {
            "schedule"
    };
}

std::vector<std::string> SQLValidator::getWhitelistedColumns() {
    return {
            // Primary identifiers (unique_id is preferred)
            "unique_id",        // NEW: Primary filtering identifier
            "schedule_index",   // Keep for backward compatibility

            // System columns
            "id", "semester", "created_at", "updated_at",

            // Basic existing metrics
            "amount_days", "amount_gaps", "gaps_time", "avg_start", "avg_end",

            // Enhanced time-based metrics
            "earliest_start", "latest_end", "longest_gap", "total_class_time",

            // Day pattern metrics
            "consecutive_days", "days_json", "weekend_classes",

            // Time preference flags (most commonly queried)
            "has_morning_classes", "has_early_morning",
            "has_evening_classes", "has_late_evening",

            // Daily intensity metrics
            "max_daily_hours", "min_daily_hours", "avg_daily_hours",

            // Gap and break patterns
            "has_lunch_break", "max_daily_gaps", "avg_gap_length",

            // Schedule efficiency metrics
            "schedule_span", "compactness_ratio", "weekday_only",

            // Individual weekday flags
            "has_monday", "has_tuesday", "has_wednesday",
            "has_thursday", "has_friday", "has_saturday", "has_sunday"
    };
}

std::string SQLValidator::sanitizeQuery(const std::string& query) {
    std::string sanitized = query;

    // Remove comments
    size_t pos = 0;
    while ((pos = sanitized.find("--", pos)) != std::string::npos) {
        size_t endPos = sanitized.find('\n', pos);
        if (endPos == std::string::npos) {
            sanitized.erase(pos);
            break;
        } else {
            sanitized.erase(pos, endPos - pos);
        }
    }

    // Remove block comments
    pos = 0;
    while ((pos = sanitized.find("/*", pos)) != std::string::npos) {
        size_t endPos = sanitized.find("*/", pos);
        if (endPos != std::string::npos) {
            sanitized.erase(pos, endPos - pos + 2);
        } else {
            sanitized.erase(pos);
            break;
        }
    }

    return sanitized;
}

std::string SQLValidator::normalizeQuery(const std::string& query) {
    std::string normalized = sanitizeQuery(query);

    // Convert to lowercase for analysis
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);

    // Replace multiple whitespace with single space
    QString qNormalized = QString::fromStdString(normalized);
    qNormalized = qNormalized.simplified();

    return qNormalized.toStdString();
}