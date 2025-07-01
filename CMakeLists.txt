cmake_minimum_required(VERSION 3.16)

project(SchedGUI VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")
endif()

# Fetch dependencies
include(FetchContent)

# Fetch OpenXLSX for Excel file parsing
FetchContent_Declare(
        OpenXLSX
        GIT_REPOSITORY https://github.com/troldal/OpenXLSX.git
        GIT_TAG        master
        GIT_SHALLOW    TRUE
)

# Fetch jsoncpp for JSON parsing
FetchContent_Declare(
        jsoncpp
        GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
        GIT_TAG        1.9.5
        GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(OpenXLSX jsoncpp)

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Quick Qml QuickLayouts PrintSupport QuickControls2 Sql)

# Find CURL
find_package(CURL REQUIRED)
if(NOT CURL_FOUND)
    message(FATAL_ERROR "CURL library not found. Please install libcurl development package.")
endif()

# Platform-specific CURL handling
if(WIN32)
    # For Windows, you might want to use vcpkg or manually specify CURL paths
    # Example with vcpkg: cmake -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
    if(NOT CURL_FOUND)
        message(STATUS "On Windows, consider using vcpkg to install curl:")
        message(STATUS "  vcpkg install curl")
        message(STATUS "  cmake -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake ..")
    endif()
elseif(APPLE)
    # On macOS, you can install curl via Homebrew
    if(NOT CURL_FOUND)
        message(STATUS "On macOS, install curl via Homebrew:")
        message(STATUS "  brew install curl")
    endif()
else()
    # On Linux, install via package manager
    if(NOT CURL_FOUND)
        message(STATUS "On Linux, install curl development package:")
        message(STATUS "  Ubuntu/Debian: sudo apt-get install libcurl4-openssl-dev")
        message(STATUS "  CentOS/RHEL: sudo yum install libcurl-devel")
        message(STATUS "  Fedora: sudo dnf install libcurl-devel")
    endif()
endif()

set(PROJECT_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/model_interfaces.h

        ${CMAKE_CURRENT_SOURCE_DIR}/controller/include/main_controller.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/include/controller_manager.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/include/file_input.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/include/course_selection.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/include/schedules_display.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/include/log_display.h

        ${CMAKE_CURRENT_SOURCE_DIR}/controller/adapters/thread_workers/ScheduleGenerator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/adapters/thread_workers/CourseValidator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/adapters/thread_workers/ScheduleGenerator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/adapters/thread_workers/CourseValidator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/adapters/thread_workers/ChatBot.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/adapters/thread_workers/ChatBot.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/controller/adapters/filters/schedule_filter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/adapters/filters/schedule_filter.h

        ${CMAKE_CURRENT_SOURCE_DIR}/controller/adapters/view_models/course_model.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/adapters/view_models/schedule_model.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/adapters/view_models/course_model.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/adapters/view_models/schedule_model.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/adapters/view_models/file_model.h
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/adapters/view_models/file_model.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/controller/main_controller.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/src/controller_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/src/file_input.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/src/course_selection.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/src/schedules_display.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/src/log_display.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/logger/logger.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/model/main_model.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/model/src/main/model_access.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/model/src/parsers/parseCoursesToVector.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/model/src/parsers/parseToCsv.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/model/src/parsers/printSchedule.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/model/src/parsers/excel_parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/model/src/parsers/validate_courses.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/model/src/parsers/jsonParser.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/model/src/schedule_algorithm/CourseLegalComb.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/model/src/schedule_algorithm/ScheduleBuilder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/model/src/schedule_algorithm/TimeUtils.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/model/src/db/db_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/model/src/db/model_db_integration.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/model/src/db/db_schema.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/model/src/db/db_files.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/model/src/db/db_courses.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/model/src/db/db_schedules.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/model/src/db/ScheduleDatabaseWriter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/model/src/db/db_json_helpers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/model/src/db/db_json_helpers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/model/src/db/cleanup_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/model/src/db/db_utils.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/model/src/db/schedule_filter_service.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/model/src/sched_bot/claude_api_integration.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/model/src/sched_bot/sql_validator.cpp
)

qt_add_resources(RESOURCES view/qml.qrc)

qt_add_executable(SchedGUI
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${RESOURCES}
)

# Include directories
target_include_directories(SchedGUI PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/model/include/main
        ${CMAKE_CURRENT_SOURCE_DIR}/model/include/parsers
        ${CMAKE_CURRENT_SOURCE_DIR}/model/include/schedule_algorithm
        ${CMAKE_CURRENT_SOURCE_DIR}/model/include/sched_bot
        ${CMAKE_CURRENT_SOURCE_DIR}/model/include/db
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/include
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/adapters/view_models
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/adapters/filters
        ${CMAKE_CURRENT_SOURCE_DIR}/controller/adapters/thread_workers
        ${CMAKE_CURRENT_SOURCE_DIR}/logger
        ${CURL_INCLUDE_DIRS}
)

# Link Qt libraries, OpenXLSX, CURL, and jsoncpp
target_link_libraries(SchedGUI PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Quick
        Qt6::Qml
        Qt6::QuickLayouts
        Qt6::PrintSupport
        Qt6::QuickControls2
        Qt6::Sql
        OpenXLSX::OpenXLSX
        ${CURL_LIBRARIES}
        jsoncpp_lib
)

# Add compiler definitions for CURL
target_compile_definitions(SchedGUI PRIVATE ${CURL_COMPILE_DEFINITIONS})

# Enable UTF-8 support on Windows for Excel file handling
if(WIN32)
    if(MSVC)
        target_compile_options(SchedGUI PRIVATE /utf-8)
    else()
        # For MinGW/GCC
        target_compile_options(SchedGUI PRIVATE -finput-charset=utf-8 -fexec-charset=utf-8)
    endif()
endif()

set_target_properties(SchedGUI PROPERTIES
        ${BUNDLE_ID_OPTION}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE FALSE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(SchedGUI)
endif()