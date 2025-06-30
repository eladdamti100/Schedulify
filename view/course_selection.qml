import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Basic
import "popups"
import "."

Page {
    id: courseListScreen

    property bool validationInProgress: courseSelectionController ? courseSelectionController.validationInProgress : false
    property var validationErrors: courseSelectionController ? courseSelectionController.validationErrors : []
    property bool validationExpanded: false
    property bool validationRowVisible: true

    property var logWindow: null

    // Semester filter properties
    property string selectedSemester: "ALL" // "ALL", "A", "B", "SUMMER"
    property int refreshCounter: 0 // Add this to force updates

    function isCourseSelectedSafe(index) {
        return courseSelectionController ? courseSelectionController.isCourseSelected(index) : false;
    }

    // Function to count selected courses by semester
    function getSelectedCoursesCountBySemester(semester) {
        // Use refreshCounter to make this reactive
        var dummy = refreshCounter;

        if (!courseSelectionController || !courseSelectionController.selectedCoursesModel) {
            return 0;
        }

        // Use the controller's method instead of trying to access model directly
        if (courseSelectionController.getSelectedCoursesCountForSemester) {
            return courseSelectionController.getSelectedCoursesCountForSemester(semester);
        }

        // Fallback: count from the total model
        return courseSelectionController.selectedCoursesModel.rowCount();
    }

    // Function to get selected courses for a specific semester
    function getSelectedCoursesForSemester(semester) {
        // Use refreshCounter to make this reactive
        var dummy = refreshCounter;

        if (!courseSelectionController || !courseSelectionController.selectedCoursesModel) {
            return [];
        }

        // Use controller method if available
        if (courseSelectionController.getSelectedCoursesForSemester) {
            return courseSelectionController.getSelectedCoursesForSemester(semester);
        }

        // Fallback: return all courses
        var allCourses = [];
        var model = courseSelectionController.selectedCoursesModel;
        for (var i = 0; i < model.rowCount(); i++) {
            allCourses.push({
                courseId: model.data(model.index(i, 0), Qt.UserRole + 1) || "",
                courseName: model.data(model.index(i, 0), Qt.UserRole + 2) || "",
                originalIndex: i
            });
        }
        return allCourses;
    }

    // Check if we can add a course based on its semester
    function canAddCourseToSemester(courseIndex) {
        // Use refreshCounter to make this reactive
        var dummy = refreshCounter;

        if (!courseSelectionController) {
            return false;
        }

        // If course is already selected, we can always deselect it
        if (isCourseSelectedSafe(courseIndex)) {
            return true;
        }

        // Get the course's semester from the controller
        var courseSemester = "";
        if (courseSelectionController.getCourseSemester) {
            courseSemester = courseSelectionController.getCourseSemester(courseIndex);
        }

        // Check if this semester has reached the limit
        var semesterCount = getSelectedCoursesCountBySemester(courseSemester);
        return semesterCount < 7;
    }

    // Get total selected courses across all semesters (for display only)
    function getTotalSelectedCoursesCount() {
        // Use refreshCounter to make this reactive
        var dummy = refreshCounter;

        if (!courseSelectionController || !courseSelectionController.selectedCoursesModel) {
            return 0;
        }

        // Sum up all semesters for display purposes
        var totalA = getSelectedCoursesCountBySemester("A");
        var totalB = getSelectedCoursesCountBySemester("B");
        var totalSummer = getSelectedCoursesCountBySemester("SUMMER");

        return totalA + totalB + totalSummer;
    }

    Component.onDestruction: {
        if (logWindow) {
            logWindow.destroy(); // Explicitly destroy
            logWindow = null;
            if (logDisplayController) {
                logDisplayController.setLogWindowOpen(false);
            }
        }
    }

    Connections {
        target: courseSelectionController

        function onErrorMessage(message) {
            showErrorMessage(message);
        }

        function onValidationStateChanged() {
            // Update local properties when validation state changes
            if (courseSelectionController) {
                validationInProgress = courseSelectionController.validationInProgress;
                validationErrors = courseSelectionController.validationErrors || [];

                // Auto-collapse when validation starts
                if (validationInProgress) {
                    validationExpanded = false;
                    validationRowVisible = true
                } else {
                    // Auto-hide when validation finishes
                    validationRowVisible = false
                }
            }
        }

        function onSelectionChanged() {
            // Force refresh of selected courses display
            refreshCounter++;
        }
    }

    function showErrorMessage(msg) {
        errorDialogText = msg;
        errorDialog.open();
    }

    property string errorMessage: ""
    property string searchText: ""

    Timer {
        id: errorMessageTimer
        interval: 3000 // 3 seconds
        onTriggered: {
            errorMessage = ""
        }
    }

    SlotBlockMenu {
        id: timeBlockPopup
        parent: Overlay.overlay

        onBlockTimeAdded: function(day, startTime, endTime, semester) {
            courseSelectionController.addBlockTimeToSemester(day, startTime, endTime, semester);
        }
    }

    AddCoursePopup {
        id: addCoursePopup
        parent: Overlay.overlay

        onCourseCreated: function(courseName, courseId, teacherName, semester, sessionGroups) {
            courseSelectionController.createNewCourse(courseName, courseId, teacherName, semester, sessionGroups);
        }
    }

    Rectangle {
        id: root
        anchors.fill: parent
        color: "#f9fafb"

        // Header
        Rectangle {
            id: header
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
            }
            height: 80
            color: "#ffffff"
            border.color: "#e5e7eb"

            Item {
                id: headerContent
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                }
                height: coursesBackButton.height

                // Back Button
                Button {
                    id: coursesBackButton
                    width: 40
                    height: 40
                    anchors {
                        left: parent.left
                        leftMargin: 15
                        verticalCenter: parent.verticalCenter
                    }
                    background: Rectangle {
                        color: coursesBackMouseArea.containsMouse ? "#a8a8a8" : "#f3f4f6"
                        radius: 4
                    }
                    contentItem: Text {
                        text: "←"
                        font.pixelSize: 18
                        color: "#1f2937"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    MouseArea {
                        id: coursesBackMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            // Clean up log window before navigating back
                            if (logWindow) {
                                logWindow.close();
                                logWindow = null;
                                if (logDisplayController) {
                                    logDisplayController.setLogWindowOpen(false);
                                }
                            }
                            if (courseSelectionController) {
                                courseSelectionController.goBack();
                            }
                        }
                        cursorShape: Qt.PointingHandCursor
                    }
                }

                // Page Title
                Label {
                    id: titleLabel
                    text: "Course Selection"
                    font.pixelSize: 20
                    color: "#1f2937"
                    anchors {
                        left: coursesBackButton.right
                        leftMargin: 16
                        verticalCenter: parent.verticalCenter
                    }
                }

                // log button
                Button {
                    id: logButtonB
                    anchors {
                        right: parent.right
                        verticalCenter: parent.verticalCenter
                        rightMargin: 15
                    }
                    width: 40
                    height: 40

                    background: Rectangle {
                        color: logMouseArea.containsMouse ? "#a8a8a8" : "#f3f4f6"
                        radius: 10
                    }

                    contentItem: Item {
                        anchors.fill: parent

                        Image {
                            id: logIcon
                            anchors.centerIn: parent
                            width: 24
                            height: 24
                            source: "qrc:/icons/ic-logs.svg"
                            sourceSize.width: 22
                            sourceSize.height: 22
                        }

                        ToolTip {
                            id: logsTooltip
                            text: "Open Application Logs"
                            visible: logMouseArea.containsMouse
                            delay: 500
                            timeout: 3000

                            background: Rectangle {
                                color: "#374151"
                                radius: 4
                                border.color: "#4b5563"
                            }

                            contentItem: Text {
                                text: logsTooltip.text
                                color: "white"
                                font.pixelSize: 12
                            }
                        }
                    }

                    MouseArea {
                        id: logMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            // Check if we already have a log window open
                            if (logWindow && logWindow.visible) {
                                // Bring existing window to front
                                logWindow.raise();
                                logWindow.requestActivate();
                                return;
                            }

                            // Create new log window if none exists or if controller says none is open
                            if (!logDisplayController.isLogWindowOpen || !logWindow) {
                                var component = Qt.createComponent("qrc:/log_display.qml");
                                if (component.status === Component.Ready) {
                                    logDisplayController.setLogWindowOpen(true);
                                    logWindow = component.createObject(courseListScreen);

                                    if (logWindow) {
                                        // Connect closing signal properly
                                        logWindow.closing.connect(function(close) {
                                            logDisplayController.setLogWindowOpen(false);
                                            logWindow = null; // Clear our reference
                                        });

                                        logWindow.show();
                                    }
                                } else {
                                    console.error("Error creating log window:", component.errorString());
                                }
                            }
                        }
                    }
                }

                // generate button
                Button {
                    id: generateButton
                    width: 180
                    height: 40
                    anchors {
                        right: logButtonB.left
                        rightMargin: 10
                        verticalCenter: parent.verticalCenter
                    }
                    visible: getTotalSelectedCoursesCount() > 0
                    enabled: getTotalSelectedCoursesCount() > 0

                    background: Rectangle {
                        color: generateMouseArea.containsMouse ? "#35455c" : "#1f2937"
                        radius: 4
                        implicitWidth: 180
                        implicitHeight: 40
                    }
                    font.bold: true
                    contentItem: Text {
                        text: "Generate Schedules →"
                        color: "white"
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    MouseArea {
                        id: generateMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            if (courseSelectionController) {
                                courseSelectionController.generateSchedules();
                            }
                        }
                        cursorShape: Qt.PointingHandCursor
                    }
                }
            }
        }

        // Validation Status Row
        Rectangle {
            id: validationStatusRow
            anchors {
                top: header.bottom
                topMargin: 16
                left: parent.left
                right: parent.right
                leftMargin: 16
                rightMargin: 16
            }
            height: validationRowVisible ? (validationExpanded ? 208 : 60) : 0
            visible: validationRowVisible
            radius: 8
            color: {
                if (validationInProgress) return "#ffffff"
                if (validationErrors.length === 0) return "#e1fff1"
                return "#fff6e7"
            }
            border.color: {
                if (validationInProgress) return "#000000"
                if (validationErrors.length === 0) return "#10b981"
                return "#f59e0b"
            }
            border.width: 1

            Behavior on height {
                NumberAnimation {
                    duration: 200
                    easing.type: Easing.OutCubic
                }
            }

            Column {
                id: validationContent
                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.top
                    margins: 16
                }
                spacing: 12

                Row {
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                    height: 28
                    spacing: 12

                    // Status Icon
                    Image {
                        id: statusIcon
                        anchors.verticalCenter: parent.verticalCenter
                        width: 30
                        height: 30
                        source: {
                            if (validationInProgress) return "qrc:/icons/ic-loading.svg"
                            if (validationErrors.length === 0) return "qrc:/icons/ic-valid.svg"
                            return "qrc:/icons/ic-warning.svg"
                        }
                        sourceSize.width: 30
                        sourceSize.height: 30

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                validationRowVisible = false
                            }
                        }
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: {
                            if (validationInProgress) return "Scanning courses..."
                            if (!validationErrors || validationErrors.length === 0) return "All courses parsed correctly"

                            // Count different types of messages
                            var parserWarnings = 0;
                            var parserErrors = 0;
                            var validationErrorCount = 0;

                            for (var i = 0; i < validationErrors.length; i++) {
                                var msg = validationErrors[i];
                                if (msg.includes("[Parser Warning]")) {
                                    parserWarnings++;
                                } else if (msg.includes("[Parser Error]")) {
                                    parserErrors++;
                                } else if (msg.includes("[Validation]")) {
                                    validationErrorCount++;
                                }
                            }

                            var totalIssues = parserWarnings + parserErrors + validationErrorCount;

                            if (parserErrors > 0 || validationErrorCount > 0) {
                                return "Found " + totalIssues + " issues in your course file"
                            } else {
                                return "Found " + parserWarnings + " warnings in your course file"
                            }
                        }
                        font.pixelSize: 14
                        font.bold: true
                        color: {
                            if (validationInProgress) return "#92400e"
                            if (!validationErrors || validationErrors.length === 0) return "#065f46"
                            return "#b91c1c"
                        }
                    }

                    Button {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: validationErrors && validationErrors.length > 0
                        width: 24
                        height: 24
                        background: Rectangle {
                            color: "transparent"
                        }
                        contentItem: Text {
                            text: validationExpanded ? "▼" : "▶"
                            font.pixelSize: 12
                            color: "#b91c1c"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: validationExpanded = !validationExpanded
                    }
                }

                Rectangle {
                    visible: validationExpanded && validationErrors && validationErrors.length > 0
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                    height: visible ? 150 : 0
                    color: "transparent"
                    radius: 6
                    border.color: "transparent"
                    border.width: 1

                    ListView {
                        id: errorsList
                        anchors {
                            fill: parent
                            margins: 8
                        }
                        clip: true
                        model: validationErrors
                        spacing: 8
                        cacheBuffer: 400 // Cache some items outside visible area

                        ScrollBar.vertical: ScrollBar {
                            active: true
                            policy: ScrollBar.AsNeeded
                        }

                        delegate: Rectangle {
                            width: errorsList.width
                            height: Math.max(40, errorText.implicitHeight + 16)
                            color: {
                                var msg = modelData;
                                if (msg.includes("[Parser Warning]")) return "#fffbeb"
                                if (msg.includes("[Parser Error]")) return "#fef2f2"
                                if (msg.includes("[Validation]")) return "#fffbeb"
                                return "#ffffff"
                            }
                            radius: 4
                            border.color: {
                                var msg = modelData;
                                if (msg.includes("[Parser Warning]")) return "#fbbf24"
                                if (msg.includes("[Parser Error]")) return "#f87171"
                                if (msg.includes("[Validation]")) return "#fbbf24"
                                return "#e5e7eb"
                            }
                            border.width: 1

                            Text {
                                id: errorText
                                anchors {
                                    left: parent.left
                                    right: parent.right
                                    verticalCenter: parent.verticalCenter
                                    leftMargin: 8
                                    rightMargin: 8
                                }
                                text: {
                                    var msg = modelData;
                                    if (msg.includes("[Parser Warning] ")) {
                                        return "⚠️ " + msg.replace("[Parser Warning] ", "");
                                    } else if (msg.includes("[Parser Error] ")) {
                                        return "❌ " + msg.replace("[Parser Error] ", "");
                                    } else if (msg.includes("[Validation] ")) {
                                        return "⚠️ " + msg.replace("[Validation] ", "");
                                    }
                                    return msg;
                                }
                                font.pixelSize: 12
                                color: {
                                    var msg = modelData;
                                    if (msg.includes("[Parser Warning]")) return "#92400e"
                                    return "#991b1b"
                                }
                                wrapMode: Text.WordWrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }

            // Expand/collapse errors list
            MouseArea {
                anchors {
                    fill: parent
                    leftMargin: 58
                }
                enabled: validationErrors && validationErrors.length > 0
                cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: {
                    if (validationErrors && validationErrors.length > 0) {
                        validationExpanded = !validationExpanded
                    }
                }
            }
        }

        // Main content
        Item {
            id: mainContent
            anchors {
                top: validationStatusRow.bottom
                topMargin: 16
                left: parent.left
                right: parent.right
                bottom: footer.top
                margins: 16
            }

            // Left side
            Rectangle {
                id: courseListArea
                anchors {
                    top: parent.top
                    left: parent.left
                    bottom: parent.bottom
                }
                width: (parent.width * 2 / 3) - 8
                color: "#ffffff"
                radius: 8
                border.color: "#e5e7eb"

                Item {
                    id: courseListContent
                    anchors {
                        fill: parent
                        margins: 16
                    }

                    // Error message
                    Rectangle {
                        id: errorMessageContainer
                        anchors {
                            top: parent.top
                            left: parent.left
                            right: parent.right
                        }
                        height: errorMessage === "" ? 0 : 40
                        visible: errorMessage !== ""
                        color: "#fef2f2"
                        radius: 4
                        border.color: "#fecaca"

                        Label {
                            anchors.centerIn: parent
                            text: errorMessage
                            color: "#dc2626"
                            font.pixelSize: 14
                        }
                    }

                    // Header
                    RowLayout {
                        id: courseListTitleRow
                        anchors {
                            top: errorMessageContainer.bottom
                            topMargin: errorMessage === "" ? 0 : 16
                            left: parent.left
                            right: parent.right
                        }
                        height: 30

                        // Validation icon (visible when validation row is hidden)
                        Image {
                            id: compactValidationIcon
                            visible: !validationRowVisible && !validationInProgress
                            Layout.preferredWidth: 24
                            Layout.preferredHeight: 24
                            source: {
                                if (!validationErrors || validationErrors.length === 0) return "qrc:/icons/ic-valid.svg"
                                return "qrc:/icons/ic-warning.svg"
                            }
                            sourceSize.width: 24
                            sourceSize.height: 24

                            MouseArea {
                                id: compactValidationMouseArea
                                anchors.fill: parent
                                cursorShape: (validationErrors && validationErrors.length > 0) ? Qt.PointingHandCursor : Qt.ArrowCursor
                                hoverEnabled: true
                                onClicked: {
                                    // Only show validation row if there are errors/warnings
                                    if (validationErrors && validationErrors.length > 0) {
                                        validationRowVisible = true
                                    }
                                }

                                ToolTip {
                                    visible: parent.containsMouse
                                    delay: 500
                                    timeout: 3000

                                    background: Rectangle {
                                        color: "#374151"
                                        radius: 4
                                        border.color: "#4b5563"
                                    }

                                    contentItem: Text {
                                        text: (validationErrors && validationErrors.length === 0) ? "Parsed successfully" : "View " + validationErrors.length + " issues"
                                        color: "white"
                                        font.pixelSize: 12
                                    }
                                }
                            }
                        }

                        Label {
                            text: "Available Courses"
                            font.pixelSize: 24
                            color: "#1f2937"
                            Layout.fillWidth: true
                        }

                        // // Semester Toggle
                        // Rectangle {
                        //     id: semesterToggle
                        //     Layout.preferredWidth: 200
                        //     Layout.preferredHeight: 32
                        //     radius: 4
                        //     border.color: "#374151"
                        //     border.width: 1
                        //     color: "#1f2937"
                        //
                        //     Row {
                        //         anchors.fill: parent
                        //         spacing: 0
                        //
                        //         Rectangle {
                        //             id: allSemesterButton
                        //             width: parent.width / 4
                        //             height: parent.height
                        //             radius: 4
                        //             color: selectedSemester === "ALL" ? "#3b82f6" : (allMouseArea.containsMouse ? "#374151" : "transparent")
                        //
                        //             Text {
                        //                 anchors.centerIn: parent
                        //                 text: "ALL"
                        //                 font.pixelSize: 10
                        //                 font.bold: true
                        //                 color: selectedSemester === "ALL" ? "#ffffff" : "#d1d5db"
                        //             }
                        //
                        //             MouseArea {
                        //                 id: allMouseArea
                        //                 anchors.fill: parent
                        //                 cursorShape: Qt.PointingHandCursor
                        //                 hoverEnabled: true
                        //                 onClicked: {
                        //                     selectedSemester = "ALL"
                        //                     if (courseSelectionController) {
                        //                         courseSelectionController.filterBySemester("ALL")
                        //                     }
                        //                 }
                        //             }
                        //         }
                        //
                        //         Rectangle {
                        //             id: semesterAButton
                        //             width: parent.width / 4
                        //             height: parent.height
                        //             color: selectedSemester === "A" ? "#3b82f6" : (aMouseArea.containsMouse ? "#374151" : "transparent")
                        //
                        //             Text {
                        //                 anchors.centerIn: parent
                        //                 text: "SEM A"
                        //                 font.pixelSize: 9
                        //                 font.bold: true
                        //                 color: selectedSemester === "A" ? "#ffffff" : "#d1d5db"
                        //             }
                        //
                        //             MouseArea {
                        //                 id: aMouseArea
                        //                 anchors.fill: parent
                        //                 cursorShape: Qt.PointingHandCursor
                        //                 hoverEnabled: true
                        //                 onClicked: {
                        //                     selectedSemester = "A"
                        //                     if (courseSelectionController) {
                        //                         courseSelectionController.filterBySemester("A")
                        //                     }
                        //                 }
                        //             }
                        //         }
                        //
                        //         Rectangle {
                        //             id: semesterBButton
                        //             width: parent.width / 4
                        //             height: parent.height
                        //             color: selectedSemester === "B" ? "#3b82f6" : (bMouseArea.containsMouse ? "#374151" : "transparent")
                        //
                        //             Text {
                        //                 anchors.centerIn: parent
                        //                 text: "SEM B"
                        //                 font.pixelSize: 9
                        //                 font.bold: true
                        //                 color: selectedSemester === "B" ? "#ffffff" : "#d1d5db"
                        //             }
                        //
                        //             MouseArea {
                        //                 id: bMouseArea
                        //                 anchors.fill: parent
                        //                 cursorShape: Qt.PointingHandCursor
                        //                 hoverEnabled: true
                        //                 onClicked: {
                        //                     selectedSemester = "B"
                        //                     if (courseSelectionController) {
                        //                         courseSelectionController.filterBySemester("B")
                        //                     }
                        //                 }
                        //             }
                        //         }
                        //
                        //         Rectangle {
                        //             id: summerSemesterButton
                        //             width: parent.width / 4
                        //             height: parent.height
                        //             radius: 4
                        //             color: selectedSemester === "SUMMER" ? "#3b82f6" : (summerMouseArea.containsMouse ? "#374151" : "transparent")
                        //
                        //             Text {
                        //                 anchors.centerIn: parent
                        //                 text: "SUMMER"
                        //                 font.pixelSize: 8
                        //                 font.bold: true
                        //                 color: selectedSemester === "SUMMER" ? "#ffffff" : "#d1d5db"
                        //             }
                        //
                        //             MouseArea {
                        //                 id: summerMouseArea
                        //                 anchors.fill: parent
                        //                 cursorShape: Qt.PointingHandCursor
                        //                 hoverEnabled: true
                        //                 onClicked: {
                        //                     selectedSemester = "SUMMER"
                        //                     if (courseSelectionController) {
                        //                         courseSelectionController.filterBySemester("SUMMER")
                        //                     }
                        //                 }
                        //             }
                        //         }
                        //     }
                        // }

                        Button {
                            id: createCourseButton
                            Layout.preferredWidth: 140
                            Layout.preferredHeight: 32

                            background: Rectangle {
                                color: createCourseMouseArea.containsMouse ? "#35455c" : "#1f2937"
                                radius: 4
                            }

                            Text {
                                anchors.centerIn: parent
                                text: "+ Create Course"
                                font.pixelSize: 12
                                font.bold: true
                                color: "white"
                            }

                            MouseArea {
                                id: createCourseMouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: {
                                    addCoursePopup.open()
                                }
                                cursorShape: Qt.PointingHandCursor
                            }
                        }
                    }

                    // Search bar
                    Rectangle {
                        id: searchBar
                        anchors {
                            top: courseListTitleRow.bottom
                            topMargin: 16
                            left: parent.left
                            right: parent.right
                        }
                        height: 50
                        radius: 8
                        color: "#f9fafb"
                        border.color: "#e5e7eb"

                        Item {
                            id: searchContent
                            anchors {
                                left: parent.left
                                right: parent.right
                                verticalCenter: parent.verticalCenter
                                leftMargin: 16
                                rightMargin: 16
                            }
                            height: parent.height

                            TextField {
                                id: searchField
                                anchors {
                                    left: parent.left
                                    right: clearSearch.left
                                    rightMargin: 8
                                    verticalCenter: parent.verticalCenter
                                }
                                height: 30
                                placeholderText: "Search by course ID, name, or instructor..."
                                placeholderTextColor: "#9CA3AF"
                                font.pixelSize: 14
                                color: "#1f2937"
                                selectByMouse: true

                                background: Rectangle {
                                    radius: 4
                                    color: "#FFFFFF"
                                    border.color: "#e5e7eb"
                                }

                                onTextChanged: {
                                    searchText = text
                                    if (courseSelectionController) {
                                        courseSelectionController.filterCourses(text)
                                    }
                                }
                            }

                            Button {
                                id: clearSearch
                                visible: searchField.text !== ""
                                width: 30
                                height: 24
                                anchors {
                                    right: parent.right
                                    verticalCenter: parent.verticalCenter
                                }

                                background: Rectangle {
                                    color: "transparent"
                                }

                                contentItem: Text {
                                    text: "✕"
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: clearSearchMouseArea.containsMouse ? "#d81a1a" : "#000000"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }

                                MouseArea {
                                    id: clearSearchMouseArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: {
                                        searchField.text = ""
                                        searchText = ""
                                        if (courseSelectionController) {
                                            courseSelectionController.resetFilter()
                                        }
                                    }
                                    cursorShape: Qt.PointingHandCursor
                                }
                            }
                        }
                    }

                    // Semester Toggle
                    RowLayout {
                        id: selectSemester
                        anchors {
                            top: searchBar.bottom
                            topMargin: 16
                            left: parent.left
                            right: parent.right
                        }
                        height: 40

                        // Spacer to center the toggle
                        Item {
                            Layout.fillWidth: true
                        }

                        // Semester Toggle
                        Rectangle {
                            id: semesterToggle
                            Layout.preferredWidth: 320
                            Layout.preferredHeight: 40
                            radius: 6
                            border.color: "#d1d5db"
                            border.width: 1
                            color: "#f9fafb"

                            Row {
                                anchors.fill: parent
                                spacing: 0

                                Rectangle {
                                    id: allSemesterButton
                                    width: parent.width / 4
                                    height: parent.height
                                    radius: 6
                                    color: selectedSemester === "ALL" ? "#3b82f6" : (allMouseArea.containsMouse ? "#f3f4f6" : "transparent")
                                    border.color: selectedSemester === "ALL" ? "#3b82f6" : "transparent"
                                    border.width: selectedSemester === "ALL" ? 1 : 0

                                    Text {
                                        anchors.centerIn: parent
                                        text: "ALL"
                                        font.pixelSize: 14
                                        font.bold: true
                                        color: selectedSemester === "ALL" ? "#ffffff" : "#374151"
                                    }

                                    MouseArea {
                                        id: allMouseArea
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        hoverEnabled: true
                                        onClicked: {
                                            selectedSemester = "ALL"
                                            if (courseSelectionController) {
                                                courseSelectionController.filterBySemester("ALL")
                                            }
                                        }
                                    }
                                }

                                Rectangle {
                                    id: semesterAButton
                                    width: parent.width / 4
                                    height: parent.height
                                    radius: 6
                                    color: selectedSemester === "A" ? "#3b82f6" : (aMouseArea.containsMouse ? "#f3f4f6" : "transparent")
                                    border.color: selectedSemester === "A" ? "#3b82f6" : "transparent"
                                    border.width: selectedSemester === "A" ? 1 : 0

                                    Text {
                                        anchors.centerIn: parent
                                        text: "SEM A"
                                        font.pixelSize: 13
                                        font.bold: true
                                        color: selectedSemester === "A" ? "#ffffff" : "#374151"
                                    }

                                    MouseArea {
                                        id: aMouseArea
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        hoverEnabled: true
                                        onClicked: {
                                            selectedSemester = "A"
                                            if (courseSelectionController) {
                                                courseSelectionController.filterBySemester("A")
                                            }
                                        }
                                    }
                                }

                                Rectangle {
                                    id: semesterBButton
                                    width: parent.width / 4
                                    height: parent.height
                                    radius: 6
                                    color: selectedSemester === "B" ? "#3b82f6" : (bMouseArea.containsMouse ? "#f3f4f6" : "transparent")
                                    border.color: selectedSemester === "B" ? "#3b82f6" : "transparent"
                                    border.width: selectedSemester === "B" ? 1 : 0

                                    Text {
                                        anchors.centerIn: parent
                                        text: "SEM B"
                                        font.pixelSize: 13
                                        font.bold: true
                                        color: selectedSemester === "B" ? "#ffffff" : "#374151"
                                    }

                                    MouseArea {
                                        id: bMouseArea
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        hoverEnabled: true
                                        onClicked: {
                                            selectedSemester = "B"
                                            if (courseSelectionController) {
                                                courseSelectionController.filterBySemester("B")
                                            }
                                        }
                                    }
                                }

                                Rectangle {
                                    id: summerSemesterButton
                                    width: parent.width / 4
                                    height: parent.height
                                    radius: 6
                                    color: selectedSemester === "SUMMER" ? "#3b82f6" : (summerMouseArea.containsMouse ? "#f3f4f6" : "transparent")
                                    border.color: selectedSemester === "SUMMER" ? "#3b82f6" : "transparent"
                                    border.width: selectedSemester === "SUMMER" ? 1 : 0

                                    Text {
                                        anchors.centerIn: parent
                                        text: "SUMMER"
                                        font.pixelSize: 11
                                        font.bold: true
                                        color: selectedSemester === "SUMMER" ? "#ffffff" : "#374151"
                                    }

                                    MouseArea {
                                        id: summerMouseArea
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        hoverEnabled: true
                                        onClicked: {
                                            selectedSemester = "SUMMER"
                                            if (courseSelectionController) {
                                                courseSelectionController.filterBySemester("SUMMER")
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Spacer to center the toggle
                        Item {
                            Layout.fillWidth: true
                        }
                    }

                    // courses
                    ListView {
                        id: courseListView
                        anchors {
                            top: selectSemester.bottom
                            topMargin: 16
                            left: parent.left
                            right: parent.right
                            bottom: parent.bottom
                        }
                        clip: true
                        model: courseSelectionController ? courseSelectionController.filteredCourseModel : null
                        spacing: 8

                        delegate: Rectangle {
                            id: courseDelegate
                            width: courseListView.width
                            height: 80
                            color: {
                                if (isCourseSelectedSafe(originalIndex)) {
                                    return "#f0f9ff"
                                } else if (!canAddCourseToSemester(originalIndex)) {
                                    return "#e5e7eb"
                                } else {
                                    return "#ffffff"
                                }
                            }
                            radius: 8
                            border.color: {
                                if (isCourseSelectedSafe(originalIndex)) {
                                    return "#3b82f6"
                                } else if (!canAddCourseToSemester(originalIndex)) {
                                    return "#d1d5db"
                                } else {
                                    return "#e5e7eb"
                                }
                            }
                            opacity: !canAddCourseToSemester(originalIndex) && !courseSelectionController.isCourseSelected(originalIndex) ? 0.7 : 1

                            Connections {
                                target: courseSelectionController
                                function onSelectionChanged() {
                                    if (isCourseSelectedSafe(originalIndex)) {
                                        courseDelegate.color = "#f0f9ff"
                                        courseDelegate.border.color = "#3b82f6"
                                        courseDelegate.opacity = 1
                                        courseIdBox.color = "#dbeafe"
                                        courseIdBoxLabel.color = "#2563eb"
                                        semesterBadge.color = "#f3f4f6"
                                        semesterBadgeLabel.color = "#4b5563"
                                    } else if (!canAddCourseToSemester(originalIndex)) {
                                        courseDelegate.color = "#e5e7eb"
                                        courseDelegate.border.color = "#d1d5db"
                                        courseDelegate.opacity = 0.7
                                        courseIdBox.color = "#e5e7eb"
                                        courseIdBoxLabel.color = "#9ca3af"
                                        semesterBadge.color = "#e5e7eb"
                                        semesterBadgeLabel.color = "#9ca3af"
                                    } else {
                                        courseDelegate.color = "#ffffff"
                                        courseDelegate.border.color = "#e5e7eb"
                                        courseDelegate.opacity = 1
                                        courseIdBox.color = "#f3f4f6"
                                        courseIdBoxLabel.color = "#4b5563"
                                        semesterBadge.color = "#f3f4f6"
                                        semesterBadgeLabel.color = "#4b5563"
                                    }
                                }
                            }

                            Item {
                                id: delegateContent
                                anchors {
                                    fill: parent
                                    margins: 12
                                }

                                Rectangle {
                                    id: courseIdBox
                                    anchors {
                                        left: parent.left
                                        verticalCenter: parent.verticalCenter
                                    }
                                    width: 80
                                    height: 56
                                    color: {
                                        if (isCourseSelectedSafe(originalIndex)) {
                                            return "#dbeafe"
                                        } else if (!canAddCourseToSemester(originalIndex)) {
                                            return "#e5e7eb"
                                        } else {
                                            return "#f3f4f6"
                                        }
                                    }
                                    radius: 4

                                    Label {
                                        id: courseIdBoxLabel
                                        anchors.centerIn: parent
                                        text: courseId
                                        font.bold: true
                                        color: {
                                            if (isCourseSelectedSafe(originalIndex)) {
                                                return "#2563eb"
                                            } else if (!canAddCourseToSemester(originalIndex)) {
                                                return "#9ca3af"
                                            } else {
                                                return "#4b5563"
                                            }
                                        }
                                    }
                                }

                                // Semester Badge
                                Rectangle {
                                    id: semesterBadge
                                    anchors {
                                        top: parent.top
                                        right: parent.right
                                    }
                                    width: semesterBadgeLabel.implicitWidth + 12
                                    height: 20
                                    radius: 10
                                    color: {
                                        if (isCourseSelectedSafe(originalIndex)) {
                                            return "#f3f4f6"
                                        } else if (!canAddCourseToSemester(originalIndex)) {
                                            return "#e5e7eb"
                                        } else {
                                            return "#f3f4f6"
                                        }
                                    }

                                    Label {
                                        id: semesterBadgeLabel
                                        anchors.centerIn: parent
                                        text: semesterDisplay || "SEM A"
                                        font.pixelSize: 10
                                        font.bold: true
                                        color: {
                                            if (isCourseSelectedSafe(originalIndex)) {
                                                return "#4b5563"
                                            } else if (!canAddCourseToSemester(originalIndex)) {
                                                return "#9ca3af"
                                            } else {
                                                return "#4b5563"
                                            }
                                        }
                                    }
                                }

                                Item {
                                    id: courseInfoColumn
                                    anchors {
                                        left: courseIdBox.right
                                        leftMargin: 16
                                        right: semesterBadge.left
                                        rightMargin: 8
                                        verticalCenter: parent.verticalCenter
                                    }
                                    height: parent.height

                                    Label {
                                        id: courseTitleLabel
                                        anchors {
                                            top: parent.top
                                            topMargin: 8
                                            left: parent.left
                                            right: parent.right
                                        }
                                        height: 20
                                        text: courseName
                                        font.pixelSize: 16
                                        font.bold: true
                                        color: {
                                            if (!canAddCourseToSemester(originalIndex) && !courseSelectionController.isCourseSelected(originalIndex)) {
                                                return "#9ca3af"
                                            } else {
                                                return "#1f2937"
                                            }
                                        }
                                        elide: Text.ElideRight
                                    }

                                    Label {
                                        id: teacherLabel
                                        anchors {
                                            top: courseTitleLabel.bottom
                                            topMargin: 4
                                            left: parent.left
                                            right: parent.right
                                        }
                                        height: 18
                                        text: "Instructor: " + teacherName
                                        font.pixelSize: 14
                                        color: {
                                            if (!canAddCourseToSemester(originalIndex) && !courseSelectionController.isCourseSelected(originalIndex)) {
                                                return "#9ca3af"
                                            } else {
                                                return "#6b7280"
                                            }
                                        }
                                        elide: Text.ElideRight
                                    }
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if (!courseSelectionController) {
                                        return;
                                    }

                                    // Check if we can add this course to its semester
                                    if (!canAddCourseToSemester(originalIndex)) {
                                        // Get the course's semester for a more specific error message
                                        var courseSemester = "";
                                        if (courseSelectionController.getCourseSemester) {
                                            courseSemester = courseSelectionController.getCourseSemester(originalIndex);
                                        }

                                        var semesterName = courseSemester === "A" ? "Semester A" :
                                                courseSemester === "B" ? "Semester B" :
                                                    courseSemester === "SUMMER" ? "Summer" :
                                                    "this semester";

                                        errorMessage = "You have selected the maximum of 7 courses for " + semesterName;
                                        errorMessageTimer.restart();
                                    } else {
                                        courseSelectionController.toggleCourseSelection(originalIndex);
                                    }
                                }
                            }
                        }

                        Item {
                            anchors.centerIn: parent
                            width: parent.width * 0.8
                            height: 100
                            visible: courseListView.count === 0

                            Item {
                                id: emptyStateContent
                                anchors.centerIn: parent
                                width: 200
                                height: 100

                                Text {
                                    id: emptyIcon
                                    anchors {
                                        top: parent.top
                                        horizontalCenter: parent.horizontalCenter
                                    }
                                    height: 30
                                    text: "🔍"
                                    font.pixelSize: 24
                                }

                                Text {
                                    id: emptyTitle
                                    anchors {
                                        top: emptyIcon.bottom
                                        topMargin: 8
                                        horizontalCenter: parent.horizontalCenter
                                    }
                                    height: 25
                                    text: "No courses found"
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "#4b5563"
                                }

                                Text {
                                    id: emptySubtitle
                                    anchors {
                                        top: emptyTitle.bottom
                                        topMargin: 8
                                        horizontalCenter: parent.horizontalCenter
                                    }
                                    height: 20
                                    text: "Try a different search term"
                                    font.pixelSize: 14
                                    color: "#9ca3af"
                                    horizontalAlignment: Text.AlignHCenter
                                }
                            }
                        }

                        ScrollBar.vertical: ScrollBar {
                            active: true
                        }
                    }
                }
            }

            // Right side
            Item {
                id: rightPanel
                anchors {
                    top: parent.top
                    right: parent.right
                    bottom: parent.bottom
                    left: courseListArea.right
                    leftMargin: 16
                }

                // Selected Courses Section
                Rectangle {
                    id: selectedCoursesSection
                    anchors {
                        top: parent.top
                        left: parent.left
                        right: parent.right
                    }
                    height: (parent.height - 16) / 2
                    color: "#ffffff"
                    radius: 8
                    border.color: "#e5e7eb"

                    Item {
                        id: selectedCoursesContent
                        anchors {
                            fill: parent
                            margins: 16
                        }

                        Item {
                            id: selectedCoursesHeader
                            anchors {
                                top: parent.top
                                left: parent.left
                                right: parent.right
                            }
                            height: 30

                            Label {
                                id: selectedCoursesTitle
                                anchors {
                                    left: parent.left
                                    verticalCenter: parent.verticalCenter
                                }
                                text: "Selected Courses"
                                font.pixelSize: 18
                                font.bold: true
                                color: "#1f2937"
                            }

                            Rectangle {
                                id: courseCounter
                                anchors {
                                    right: parent.right
                                    verticalCenter: parent.verticalCenter
                                }
                                width: 60
                                height: 30
                                radius: 4
                                color: "#f3f4f6"
                                border.color: "#d1d5db"

                                // Show single counter for specific semester
                                Label {
                                    visible: selectedSemester !== "ALL"
                                    anchors.centerIn: parent
                                    text: getSelectedCoursesCountBySemester(selectedSemester) + "/7"
                                    font.pixelSize: 14
                                    font.bold: true
                                    color: "#1f2937"
                                }

                                // Show breakdown for ALL
                                Column {
                                    visible: selectedSemester === "ALL"
                                    anchors.centerIn: parent
                                    spacing: 2

                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: (getSelectedCoursesCountBySemester("A") + getSelectedCoursesCountBySemester("B") + getSelectedCoursesCountBySemester("SUMMER")) + "/21"
                                        font.pixelSize: 14
                                        font.bold: true
                                        color: "#1f2937"
                                    }
                                }
                            }
                        }

                        ScrollView {
                            id: selectedCoursesScrollView
                            anchors {
                                top: selectedCoursesHeader.bottom
                                topMargin: 12
                                left: parent.left
                                right: parent.right
                                bottom: parent.bottom
                            }
                            clip: true
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                            contentWidth: width
                            contentHeight: selectedCoursesColumn.height

                            Item {
                                id: selectedCoursesColumn
                                width: selectedCoursesScrollView.width
                                height: selectedSemester === "ALL" ? allSemesterView.height : (getSelectedCoursesForSemester(selectedSemester).length * 58)

                                // View for ALL - show semester breakdown
                                Column {
                                    id: allSemesterView
                                    visible: selectedSemester === "ALL"
                                    width: parent.width
                                    spacing: 16

                                    // Semester A section
                                    Rectangle {
                                        width: parent.width
                                        height: 50
                                        radius: 6
                                        color: "#f0f9ff"
                                        border.color: "#3b82f6"
                                        border.width: 1

                                        Row {
                                            anchors.centerIn: parent
                                            spacing: 12

                                            Text {
                                                text: "📚"
                                                font.pixelSize: 20
                                            }

                                            Text {
                                                text: "Semester A: " + getSelectedCoursesCountBySemester("A") + "/7 courses"
                                                font.pixelSize: 14
                                                font.bold: true
                                                color: "#1f2937"
                                            }
                                        }
                                    }

                                    // Semester B section
                                    Rectangle {
                                        width: parent.width
                                        height: 50
                                        radius: 6
                                        color: "#f0fdf4"
                                        border.color: "#22c55e"
                                        border.width: 1

                                        Row {
                                            anchors.centerIn: parent
                                            spacing: 12

                                            Text {
                                                text: "📖"
                                                font.pixelSize: 20
                                            }

                                            Text {
                                                text: "Semester B: " + getSelectedCoursesCountBySemester("B") + "/7 courses"
                                                font.pixelSize: 14
                                                font.bold: true
                                                color: "#1f2937"
                                            }
                                        }
                                    }

                                    // Summer section
                                    Rectangle {
                                        width: parent.width
                                        height: 50
                                        radius: 6
                                        color: "#fffbeb"
                                        border.color: "#f59e0b"
                                        border.width: 1

                                        Row {
                                            anchors.centerIn: parent
                                            spacing: 12

                                            Text {
                                                text: "☀️"
                                                font.pixelSize: 20
                                            }

                                            Text {
                                                text: "Summer: " + getSelectedCoursesCountBySemester("SUMMER") + "/7 courses"
                                                font.pixelSize: 14
                                                font.bold: true
                                                color: "#1f2937"
                                            }
                                        }
                                    }
                                }

                                // View for specific semester - show actual courses
                                Repeater {
                                    id: selectedCoursesRepeater
                                    visible: selectedSemester !== "ALL"
                                    model: selectedSemester !== "ALL" ? getSelectedCoursesForSemester(selectedSemester).length : 0

                                    Rectangle {
                                        width: selectedCoursesColumn.width
                                        height: 50
                                        y: index * 58
                                        radius: 6
                                        color: "#f0f9ff"
                                        border.color: "#3b82f6"

                                        property var courseData: {
                                            var courses = getSelectedCoursesForSemester(selectedSemester);
                                            return (index < courses.length) ? courses[index] : {};
                                        }

                                        Item {
                                            id: selectedCourseContent
                                            anchors {
                                                fill: parent
                                                margins: 12
                                            }

                                            Rectangle {
                                                id: selectedCourseIdBox
                                                anchors {
                                                    left: parent.left
                                                    verticalCenter: parent.verticalCenter
                                                }
                                                width: 40
                                                height: 26
                                                radius: 4
                                                color: "#dbeafe"

                                                Label {
                                                    anchors.centerIn: parent
                                                    text: parent.parent.parent.courseData.courseId || ""
                                                    font.bold: true
                                                    font.pixelSize: 12
                                                    color: "#2563eb"
                                                }
                                            }

                                            Item {
                                                id: selectedCourseInfo
                                                anchors {
                                                    left: selectedCourseIdBox.right
                                                    leftMargin: 8
                                                    right: removeButton.left
                                                    rightMargin: 8
                                                    verticalCenter: parent.verticalCenter
                                                }
                                                height: parent.height

                                                Label {
                                                    id: selectedCourseName
                                                    anchors {
                                                        top: parent.top
                                                        topMargin: 4
                                                        left: parent.left
                                                        right: parent.right
                                                    }
                                                    height: 16
                                                    text: parent.parent.parent.courseData.courseName || ""
                                                    font.pixelSize: 14
                                                    font.bold: true
                                                    color: "#1f2937"
                                                    elide: Text.ElideRight
                                                }
                                            }

                                            Button {
                                                id: removeButton
                                                anchors {
                                                    right: parent.right
                                                    verticalCenter: parent.verticalCenter
                                                }
                                                width: 24
                                                height: 24

                                                background: Rectangle {
                                                    color: removeMouseArea.containsMouse ? "#ef4444" : "#f87171"
                                                    radius: 12
                                                }

                                                contentItem: Text {
                                                    text: "×"
                                                    color: "white"
                                                    font.pixelSize: 16
                                                    font.bold: true
                                                    horizontalAlignment: Text.AlignHCenter
                                                    verticalAlignment: Text.AlignVCenter
                                                }

                                                MouseArea {
                                                    id: removeMouseArea
                                                    anchors.fill: parent
                                                    hoverEnabled: true
                                                    onClicked: {
                                                        var originalIndex = parent.parent.parent.courseData.originalIndex;
                                                        if (courseSelectionController && originalIndex !== undefined) {
                                                            courseSelectionController.deselectCourse(originalIndex);
                                                        }
                                                    }
                                                    cursorShape: Qt.PointingHandCursor
                                                }
                                            }
                                        }
                                    }
                                }

                                // Empty state for specific semester
                                Item {
                                    visible: selectedSemester !== "ALL" && getSelectedCoursesForSemester(selectedSemester).length === 0
                                    width: selectedCoursesColumn.width
                                    height: 100

                                    Item {
                                        id: emptySelectedContent
                                        anchors.centerIn: parent
                                        width: 200
                                        height: 80

                                        Text {
                                            id: emptySelectedIcon
                                            anchors {
                                                top: parent.top
                                                horizontalCenter: parent.horizontalCenter
                                            }
                                            height: 25
                                            text: "📖"
                                            font.pixelSize: 20
                                        }

                                        Text {
                                            id: emptySelectedTitle
                                            anchors {
                                                top: emptySelectedIcon.bottom
                                                topMargin: 8
                                                horizontalCenter: parent.horizontalCenter
                                            }
                                            height: 18
                                            text: "No courses selected"
                                            font.pixelSize: 14
                                            color: "#6b7280"
                                            horizontalAlignment: Text.AlignHCenter
                                        }

                                        Text {
                                            id: emptySelectedSubtitle
                                            anchors {
                                                top: emptySelectedTitle.bottom
                                                topMargin: 8
                                                horizontalCenter: parent.horizontalCenter
                                            }
                                            height: 15
                                            text: "for " + selectedSemester + " semester"
                                            font.pixelSize: 12
                                            color: "#9ca3af"
                                            horizontalAlignment: Text.AlignHCenter
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Block Times Section
                Rectangle {
                    id: blockTimesSection
                    anchors {
                        top: selectedCoursesSection.bottom
                        topMargin: 16
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                    }
                    color: "#ffffff"
                    radius: 8
                    border.color: "#e5e7eb"

                    Item {
                        id: blockTimesContent
                        anchors {
                            fill: parent
                            margins: 16
                        }

                        Item {
                            id: blockTimesHeader
                            anchors {
                                top: parent.top
                                left: parent.left
                                right: parent.right
                            }
                            height: 32

                            Label {
                                id: blockTimesTitle
                                anchors {
                                    left: parent.left
                                    verticalCenter: parent.verticalCenter
                                }
                                text: "Block Times"
                                font.pixelSize: 18
                                font.bold: true
                                color: "#1f2937"
                            }

                            Button {
                                id: addBlockButton
                                anchors {
                                    right: parent.right
                                    verticalCenter: parent.verticalCenter
                                }
                                width: 120
                                height: 32

                                background: Rectangle {
                                    color: addBlockMouseArea.containsMouse ? "#35455c" : "#1f2937"
                                    radius: 4
                                }

                                contentItem: Text {
                                    text: "+ Add Block"
                                    color: "white"
                                    font.pixelSize: 12
                                    font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }

                                MouseArea {
                                    id: addBlockMouseArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: timeBlockPopup.open()
                                    cursorShape: Qt.PointingHandCursor
                                }
                            }
                        }

                        ScrollView {
                            id: blockScroll
                            anchors {
                                top: blockTimesHeader.bottom
                                topMargin: 12
                                left: parent.left
                                right: parent.right
                                bottom: parent.bottom
                            }
                            clip: true
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                            ScrollBar.vertical.policy: ScrollBar.AsNeeded

                            Column {
                                id: blockTimesColumn
                                width: blockScroll.width
                                spacing: 8

                                Repeater {
                                    id: blockTimesRepeater
                                    model: courseSelectionController ? courseSelectionController.blocksModel : null

                                    Rectangle {
                                        width: blockTimesColumn.width
                                        height: 60
                                        radius: 4
                                        color: "#fffbeb"
                                        border.color: "#fbbf24"
                                        border.width: 1

                                        Item {
                                            id: blockTimeContent
                                            anchors {
                                                fill: parent
                                                margins: 12
                                            }

                                            Item {
                                                id: blockTimeInfo
                                                anchors {
                                                    left: parent.left
                                                    right: removeBlockButton.left
                                                    rightMargin: 8
                                                    verticalCenter: parent.verticalCenter
                                                }
                                                height: parent.height

                                                Label {
                                                    id: blockDayLabel
                                                    anchors {
                                                        top: parent.top
                                                        topMargin: 4
                                                        left: parent.left
                                                        right: parent.right
                                                    }
                                                    height: 16
                                                    text: teacherName
                                                    font.pixelSize: 14
                                                    font.bold: true
                                                    color: "#92400e"
                                                    elide: Text.ElideRight
                                                }

                                                Label {
                                                    id: blockTimeLabel
                                                    anchors {
                                                        top: blockDayLabel.bottom
                                                        topMargin: 4
                                                        left: parent.left
                                                        right: parent.right
                                                    }
                                                    height: 14
                                                    text: courseId
                                                    font.pixelSize: 12
                                                    color: "#a16207"
                                                    elide: Text.ElideRight
                                                }
                                            }

                                            Button {
                                                id: removeBlockButton
                                                anchors {
                                                    right: parent.right
                                                    verticalCenter: parent.verticalCenter
                                                }
                                                width: 24
                                                height: 24

                                                background: Rectangle {
                                                    color: removeBlockMouseArea.containsMouse ? "#dc2626" : "#ef4444"
                                                    radius: 12
                                                }

                                                contentItem: Text {
                                                    text: "×"
                                                    color: "white"
                                                    font.pixelSize: 16
                                                    font.bold: true
                                                    horizontalAlignment: Text.AlignHCenter
                                                    verticalAlignment: Text.AlignVCenter
                                                }

                                                MouseArea {
                                                    id: removeBlockMouseArea
                                                    anchors.fill: parent
                                                    hoverEnabled: true
                                                    onClicked: courseSelectionController.removeBlockTime(index)
                                                    cursorShape: Qt.PointingHandCursor
                                                }
                                            }
                                        }
                                    }
                                }

                                // Empty block times
                                Item {
                                    width: blockTimesColumn.width
                                    height: blockTimesRepeater.count === 0 ? 80 : 0
                                    visible: blockTimesRepeater.count === 0

                                    Item {
                                        id: emptyBlockContent
                                        anchors.centerIn: parent
                                        width: 200
                                        height: 80

                                        Text {
                                            id: emptyBlockIcon
                                            anchors {
                                                top: parent.top
                                                horizontalCenter: parent.horizontalCenter
                                            }
                                            height: 25
                                            text: "🚫"
                                            font.pixelSize: 20
                                        }

                                        Text {
                                            id: emptyBlockTitle
                                            anchors {
                                                top: emptyBlockIcon.bottom
                                                topMargin: 8
                                                horizontalCenter: parent.horizontalCenter
                                            }
                                            height: 18
                                            text: "No blocked times"
                                            font.pixelSize: 14
                                            color: "#6b7280"
                                            horizontalAlignment: Text.AlignHCenter
                                        }

                                        Text {
                                            id: emptyBlockSubtitle
                                            anchors {
                                                top: emptyBlockTitle.bottom
                                                topMargin: 8
                                                horizontalCenter: parent.horizontalCenter
                                            }
                                            height: 15
                                            text: "Add time slots you want to avoid"
                                            font.pixelSize: 12
                                            color: "#9ca3af"
                                            horizontalAlignment: Text.AlignHCenter
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Footer
        Rectangle {
            id: footer
            anchors {
                bottom: parent.bottom
                left: parent.left
                right: parent.right
            }
            height: 30
            color: "#ffffff"
            border.color: "#e5e7eb"

            Label {
                anchors.centerIn: parent
                text: "© 2025 Schedulify. All rights reserved."
                color: "#6b7280"
                font.pixelSize: 12
            }
        }
    }
}