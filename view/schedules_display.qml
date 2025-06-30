import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt.labs.qmlmodels 1.0
import QtQuick.Controls.Basic
import "popups"
import "."

Page {
    id: schedulesDisplayPage

    background: Rectangle { color: "#ffffff" }

    property string errorMessage: ""

    Timer {
        id: errorMessageTimer
        interval: 3000 // 3 seconds
        onTriggered: {
            errorMessage = ""
        }
    }

    property var controller: schedulesDisplayController
    property var scheduleModel: controller ? controller.scheduleModel : null
    property int currentIndex: scheduleModel ? scheduleModel.currentScheduleIndex : 0
    property int totalSchedules: scheduleModel ? scheduleModel.scheduleCount : 0
    property int totalAllSchedules: scheduleModel ? scheduleModel.totalScheduleCount : 0
    property bool isFiltered: controller ? controller.isFiltered : false
    property int numDays: 7

    property int numberOfTimeSlots: 13
    property real headerHeight: 40

    // Updated to account for chatbot width when open with animation
    property real chatBotOffset: chatBot.isOpen ? chatBot.width : 0
    property real availableTableWidth: mainContent.width - 28
    property real availableTableHeight: mainContent.height - 41

    property real timeColumnWidth: availableTableWidth * 0.12
    property real dayColumnWidth: (availableTableWidth - timeColumnWidth) / numDays
    property real uniformRowHeight: (availableTableHeight - headerHeight) / numberOfTimeSlots

    property real baseTextSize: 12

    property var logWindow: null

    // Animate the chatbot offset change
    Behavior on chatBotOffset {
        NumberAnimation {
            duration: 400
            easing.type: Easing.OutCubic
        }
    }

    // Semester navigation properties
    property string currentSemester: controller ? controller.getCurrentSemester() : "A"
    property bool allSemestersLoaded: controller && controller.allSemestersLoaded ? controller.allSemestersLoaded : false

    MouseArea {
        id: outsideClickArea
        anchors.fill: parent
        z: -1
        propagateComposedEvents: true
        onPressed: function(mouse) {
            if (inputField.activeFocus) {
                // Check if click is outside the input field area
                var inputRect = inputField.parent
                var globalInputPos = inputRect.mapToItem(schedulesDisplayPage, 0, 0)

                if (mouse.x < globalInputPos.x ||
                    mouse.x > globalInputPos.x + inputRect.width ||
                    mouse.y < globalInputPos.y ||
                    mouse.y > globalInputPos.y + inputRect.height) {
                    inputField.focus = false
                    inputField.text = ""
                }
            }
            mouse.accepted = false
        }
    }

    Component.onDestruction: {
        if (logWindow) {
            logWindow.destroy();
            logWindow = null;
            if (logDisplayController) {
                logDisplayController.setLogWindowOpen(false);
            }
        }
    }

    Connections {
        target: scheduleModel
        function onCurrentScheduleIndexChanged() {
            if (tableModel) {
                tableModel.updateRows()
            }
        }
        function onScheduleDataChanged() {
            if (tableModel) {
                tableModel.updateRows()
            }
        }
    }

    Connections {
        target: controller
        function onSchedulesFiltered(filteredCount, totalCount) {
        }
    }

    Connections {
        target: controller
        function onCurrentSemesterChanged() {
            currentSemester = controller.getCurrentSemester()
            totalSchedules = scheduleModel ? scheduleModel.scheduleCount : 0
        }
        function onAllSemestersReady() {
            allSemestersLoaded = true
        }
        function onSemesterSchedulesLoaded(semester) {
            // Update button states when new semester data is loaded
        }
    }

    onDayColumnWidthChanged: {
        if (tableModel) {
            tableModel.updateRows()
        }
    }

    onUniformRowHeightChanged: {
        if (tableModel) {
            tableModel.updateRows()
        }
    }

    // Header
    Rectangle {
        id: header
        width: parent.width
        height: 80
        color: "#ffffff"
        border.color: "#e5e7eb"

        Item {
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                bottom: parent.bottom
            }

            // Back button
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
                    onClicked: controller.goBack()
                    cursorShape: Qt.PointingHandCursor
                }

                Component.onCompleted: {
                    forceActiveFocus();
                }
            }

            // Title with filter indicator
            RowLayout {
                id: titleRow
                anchors {
                    left: coursesBackButton.right
                    leftMargin: 16
                    verticalCenter: parent.verticalCenter
                }
                spacing: 12

                Label {
                    id: titleLabel
                    text: "Generated schedules"
                    font.pixelSize: 20
                    color: "#1f2937"
                }

                // Fixed: Remove anchors and use Layout properties instead
                Button {
                    id: filterResetButton
                    visible: isFiltered
                    Layout.preferredWidth: filterResetText.implicitWidth + 16
                    Layout.preferredHeight: 24

                    background: Rectangle {
                        color: filterResetMouseArea.containsMouse ? "#dc2626" : "#3b82f6"
                        radius: 12
                        border.color: filterResetMouseArea.containsMouse ? "#dc2626" : "#3b82f6"
                        border.width: 1

                        Behavior on color {
                            ColorAnimation {
                                duration: 200
                            }
                        }
                    }

                    contentItem: Text {
                        id: filterResetText
                        anchors.centerIn: parent
                        text: filterResetMouseArea.containsMouse ? "Reset Filters" : "Filtered"
                        font.pixelSize: 10
                        font.bold: true
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter

                        Behavior on text {
                            PropertyAnimation {
                                duration: 150
                            }
                        }
                    }

                    MouseArea {
                        id: filterResetMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            controller.resetFilters()
                        }
                    }

                    ToolTip {
                        visible: filterResetMouseArea.containsMouse
                        text: "Click to show all schedules"
                        delay: 500
                    }
                }
            }

            // Schedule navigator row
            Rectangle {
                id: navigationRec
                anchors {
                    horizontalCenter: parent.horizontalCenter  // Center it in the header
                    verticalCenter: parent.verticalCenter
                }
                width: Math.max(navigationRow.implicitWidth + 32, 280)
                height: 56

                color: "#f9fcff"  // Light blue background color
                radius: 12
                border.color: "#70a6ff"  // Blue border
                border.width: 1

                RowLayout {
                    id: navigationRow
                    anchors.centerIn: parent
                    spacing: 12

                    Rectangle {
                        id: prevButton
                        radius: 6
                        width: 36
                        height: 36

                        property bool isPrevEnabled: scheduleModel && totalSchedules > 0 ? scheduleModel.canGoPrevious : false

                        color: {
                            if (!isPrevEnabled) return "#e5e7eb";
                            return prevMouseArea.containsMouse ? "#35455c" : "#1f2937";
                        }

                        opacity: isPrevEnabled ? 1.0 : 0.5

                        Text {
                            text: "←"
                            anchors.centerIn: parent
                            color: parent.isPrevEnabled ? "white" : "#9ca3af"
                            font.pixelSize: 16
                            font.bold: true
                        }

                        MouseArea {
                            id: prevMouseArea
                            anchors.fill: parent
                            hoverEnabled: parent.isPrevEnabled
                            cursorShape: parent.isPrevEnabled ? Qt.PointingHandCursor : Qt.ForbiddenCursor
                            enabled: parent.isPrevEnabled
                            onClicked: {
                                if (scheduleModel) {
                                    scheduleModel.previousSchedule()
                                }
                            }
                        }
                    }

                    Label {
                        text: "Schedule"
                        font.pixelSize: 15
                        font.weight: Font.Medium
                        color: "#1f2937"  // Darker text for better contrast
                    }

                    Rectangle {
                        Layout.preferredWidth: Math.max(inputField.contentWidth + 24, 60)
                        Layout.preferredHeight: 40

                        color: "#ffffff"
                        radius: 8
                        border.color: {
                            if (inputField.activeFocus) return "#3b82f6";
                            if (!inputField.isValidInput && inputField.text !== "") return "#ef4444";
                            return "#cbd5e1";
                        }
                        border.width: inputField.activeFocus ? 2 : 1

                        TextField {
                            id: inputField
                            anchors.fill: parent
                            anchors.margins: inputField.activeFocus ? 2 : 1

                            placeholderText: currentIndex + 1
                            placeholderTextColor: "#94a3b8"

                            background: Rectangle {
                                color: "transparent"
                                radius: 6
                            }

                            color: "#1f2937"
                            font.pixelSize: 14
                            font.weight: Font.Medium
                            horizontalAlignment: TextInput.AlignHCenter

                            leftPadding: 12
                            rightPadding: 12
                            topPadding: 8
                            bottomPadding: 8

                            selectByMouse: true

                            property bool isValidInput: true

                            onTextChanged: {
                                var userInput = parseInt(text)
                                isValidInput = text === "" || (!isNaN(userInput) && userInput > 0 && userInput <= totalSchedules)
                            }

                            onEditingFinished: {
                                var userInput = parseInt(inputField.text)
                                if (!isNaN(userInput) && userInput > 0 && userInput <= totalSchedules) {
                                    scheduleModel.jumpToSchedule(userInput)
                                }
                                inputField.text = ""
                                inputField.focus = false
                            }

                            Keys.onReturnPressed: {
                                var userInput = parseInt(inputField.text)
                                if (!isNaN(userInput) && userInput > 0 && userInput <= totalSchedules) {
                                    scheduleModel.jumpToSchedule(userInput)
                                }
                                inputField.text = ""
                                inputField.focus = false
                            }

                            Keys.onEscapePressed: {
                                text = ""
                                focus = false
                            }
                        }

                        ToolTip {
                            id: errorTooltip
                            text: `Please enter a number between 1 and ${totalSchedules}`
                            visible: !inputField.isValidInput && inputField.text !== "" && inputField.activeFocus
                            delay: 0
                            timeout: 0

                            background: Rectangle {
                                color: "#ef4444"
                                radius: 4
                                border.color: "#dc2626"
                            }

                            contentItem: Text {
                                text: errorTooltip.text
                                color: "white"
                                font.pixelSize: 11
                            }
                        }
                    }

                    Label {
                        text: "of"
                        font.pixelSize: 15
                        font.weight: Font.Medium
                        color: "#1f2937"  // Darker text for better contrast
                    }

                    Rectangle {
                        Layout.preferredWidth: Math.max(inputField.contentWidth + 24, 60)
                        Layout.preferredHeight: 40

                        color: totalSchedules > 0 ? "#ffffff" : "#f1f5f9"  // White background for better contrast
                        radius: 6
                        border.color: "#3b82f6"
                        border.width: 1

                        Label {
                            id: totalLabel
                            anchors.centerIn: parent
                            text: {
                                if (totalSchedules <= 0) return "0"
                                if (isFiltered && totalAllSchedules > 0) {
                                    return `${totalSchedules} (out of ${totalAllSchedules})`
                                }
                                return totalSchedules.toString()
                            }
                            font.pixelSize: 14
                            font.weight: Font.Medium
                            color: totalSchedules > 0 ? "#1f2937" : "#64748b"
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                        }
                    }

                    Rectangle {
                        id: nextButton
                        radius: 6
                        width: 36
                        height: 36

                        property bool isNextEnabled: scheduleModel && totalSchedules > 0 ? scheduleModel.canGoNext : false

                        color: {
                            if (!isNextEnabled) return "#e5e7eb";
                            return nextMouseArea.containsMouse ? "#35455c" : "#1f2937";
                        }

                        opacity: isNextEnabled ? 1.0 : 0.5

                        Text {
                            text: "→"
                            anchors.centerIn: parent
                            color: parent.isNextEnabled ? "white" : "#9ca3af"
                            font.pixelSize: 16
                            font.bold: true
                        }

                        MouseArea {
                            id: nextMouseArea
                            anchors.fill: parent
                            hoverEnabled: parent.isNextEnabled
                            cursorShape: parent.isNextEnabled ? Qt.PointingHandCursor : Qt.ForbiddenCursor
                            enabled: parent.isNextEnabled
                            onClicked: {
                                if (scheduleModel) {
                                    scheduleModel.nextSchedule()
                                }
                            }
                        }
                    }
                }
            }

            // Swap Semester button
            Rectangle {
                id: semesterSwapButton
                anchors {
                    right: chatBotButton.left
                    verticalCenter: parent.verticalCenter
                    rightMargin: 10
                }
                width: 50
                height: 40

                // Use Qt.binding() to ensure properties update when controller changes
                property bool isAvailableA: Qt.binding(function() {
                    return controller ? controller.hasSchedulesForSemester("A") : false
                })
                property bool isFinishedA: Qt.binding(function() {
                    return controller ? controller.isSemesterFinished("A") : false
                })
                property bool isEnabledA: Qt.binding(function() {
                    return controller ? controller.canClickSemester("A") : false
                })

                property bool isAvailableB: Qt.binding(function() {
                    return controller ? controller.hasSchedulesForSemester("B") : false
                })
                property bool isFinishedB: Qt.binding(function() {
                    return controller ? controller.isSemesterFinished("B") : false
                })
                property bool isEnabledB: Qt.binding(function() {
                    return controller ? controller.canClickSemester("B") : false
                })

                property bool isAvailableSummer: Qt.binding(function() {
                    return controller ? controller.hasSchedulesForSemester("SUMMER") : false
                })
                property bool isFinishedSummer: Qt.binding(function() {
                    return controller ? controller.isSemesterFinished("SUMMER") : false
                })
                property bool isEnabledSummer: Qt.binding(function() {
                    return controller ? controller.canClickSemester("SUMMER") : false
                })

                // FIXED: Calculate available semesters based on what can actually be clicked
                property var availableSemesters: {
                    var semesters = []

                    // Only add semesters that can actually be clicked (have schedules AND are ready)
                    if (controller && controller.canClickSemester("A")) {
                        semesters.push("A")
                    }
                    if (controller && controller.canClickSemester("B")) {
                        semesters.push("B")
                    }
                    if (controller && controller.canClickSemester("SUMMER")) {
                        semesters.push("SUMMER")
                    }

                    return semesters
                }

                // Function to get semester colors - Updated color palette
                function getSemesterColors(semester) {
                    switch(semester) {
                        case "A":
                            // Blue color scheme for Semester A
                            return {
                                bg: "#dbeafe",        // Light blue background
                                bgHover: "#bfdbfe",   // Slightly darker blue on hover
                                border: "#3b82f6",    // Blue border
                                borderHover: "#2563eb", // Darker blue border on hover
                                text: "#1e40af"       // Dark blue text
                            }
                        case "B":
                            // Green color scheme for Semester B
                            return {
                                bg: "#dcfce7",        // Light green background
                                bgHover: "#bbf7d0",   // Slightly darker green on hover
                                border: "#22c55e",    // Green border
                                borderHover: "#16a34a", // Darker green border on hover
                                text: "#15803d"       // Dark green text
                            }
                        case "SUMMER":
                            // Yellow color scheme for Summer
                            return {
                                bg: "#fef3c7",        // Light yellow background
                                bgHover: "#fde68a",   // Slightly darker yellow on hover
                                border: "#f59e0b",    // Yellow/orange border
                                borderHover: "#d97706", // Darker yellow/orange border on hover
                                text: "#92400e"       // Dark yellow/brown text
                            }
                        default:
                            // Gray fallback
                            return {
                                bg: "#f1f5f9",
                                bgHover: "#e2e8f0",
                                border: "#94a3b8",
                                borderHover: "#64748b",
                                text: "#475569"
                            }
                    }
                }

                property var colors: getSemesterColors(currentSemester)

                // Visual styling
                color: mouseArea.containsMouse ? colors.bgHover : colors.bg
                radius: 10
                border.color: mouseArea.containsMouse ? colors.borderHover : colors.border
                border.width: 2

                // Smooth color transitions
                Behavior on color {
                    ColorAnimation { duration: 150 }
                }

                // Press effect
                scale: mouseArea.pressed ? 0.95 : 1.0
                Behavior on scale {
                    NumberAnimation { duration: 100 }
                }

                // Main text
                Text {
                    anchors.centerIn: parent
                    text: {
                        switch(currentSemester) {
                            case "A": return "A"
                            case "B": return "B"
                            case "SUMMER": return "S"
                            default: return "?"
                        }
                    }
                    font.pixelSize: 18
                    font.weight: Font.Bold
                    color: semesterSwapButton.colors.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                // Small green indicator when multiple semesters available
                Rectangle {
                    visible: semesterSwapButton? semesterSwapButton.availableSemesters.length > 1 : false
                    anchors {
                        right: parent.right
                        bottom: parent.bottom
                        margins: 4
                    }
                    width: 6
                    height: 6
                    radius: 3
                    color: "#10b981"

                    // Subtle pulse animation
                    SequentialAnimation on scale {
                        running: parent.visible
                        loops: Animation.Infinite
                        NumberAnimation { to: 1.2; duration: 1000 }
                        NumberAnimation { to: 1.0; duration: 1000 }
                    }
                }

                function getNextSemester() {
                    // If only one or no semesters available, return current
                    if (availableSemesters.length <= 1) {
                        return currentSemester
                    }

                    // Find current semester index in available list
                    var currentIndex = availableSemesters.indexOf(currentSemester)

                    // If current semester is not in available list (shouldn't happen), default to first
                    if (currentIndex === -1) {
                        return availableSemesters[0]
                    }

                    // Get next semester in circular order
                    var nextIndex = (currentIndex + 1) % availableSemesters.length
                    var nextSemester = availableSemesters[nextIndex]

                    return nextSemester
                }

                function swapSemester() {
                    if (availableSemesters.length <= 1) {
                        var semesterName = currentSemester === "SUMMER" ? "Summer" : "Semester " + currentSemester
                        schedulesDisplayPage.errorMessage = `You have selected courses only for ${semesterName}`
                        errorMessageTimer.restart()
                        return
                    }

                    // Get the next semester
                    var nextSemester = getNextSemester()
                    if (!availableSemesters.includes(nextSemester)) {
                        return
                    }

                    // Switch to the next semester
                    if (controller) {
                        controller.switchToSemester(nextSemester)
                    }
                }

                // Mouse interaction
                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor

                    onClicked: {
                        semesterSwapButton.swapSemester()
                    }
                }

                ToolTip {
                    id: semesterTooltip
                    text: {

                        if (semesterSwapButton.availableSemesters.length <= 1) {
                            return "No other semesters available"
                        }

                        var next = semesterSwapButton.getNextSemester()
                        var nextDisplay = next === "SUMMER" ? "Summer" : "Semester " + next
                        return `Switch to ${nextDisplay}`
                    }
                    visible: mouseArea.containsMouse
                    delay: 400
                    timeout: 4000

                    background: Rectangle {
                        color: "#374151"
                        radius: 6
                        border.color: "#4b5563"
                        border.width: 1
                    }

                    contentItem: Text {
                        text: semesterTooltip.text
                        color: "white"
                        font.pixelSize: 12
                        padding: 6
                    }
                }

                // FIXED: Add explicit connections to force re-evaluation when semester data changes
                Connections {
                    target: controller
                    function onSemesterSchedulesLoaded(semester) {
                        // Force re-evaluation of available semesters when new data is loaded
                        semesterSwapButton.availableSemesters = Qt.binding(function() {
                            var semesters = []
                            if (controller && controller.canClickSemester("A")) semesters.push("A")
                            if (controller && controller.canClickSemester("B")) semesters.push("B")
                            if (controller && controller.canClickSemester("SUMMER")) semesters.push("SUMMER")
                            return semesters
                        })
                    }

                    function onSemesterFinishedStateChanged(semester) {
                        // Re-evaluate when semester finished state changes
                        semesterSwapButton.availableSemesters = Qt.binding(function() {
                            var semesters = []
                            if (controller && controller.canClickSemester("A")) semesters.push("A")
                            if (controller && controller.canClickSemester("B")) semesters.push("B")
                            if (controller && controller.canClickSemester("SUMMER")) semesters.push("SUMMER")
                            return semesters
                        })
                    }

                    function onCurrentSemesterChanged() {
                        // Update colors when current semester changes
                        semesterSwapButton.colors = semesterSwapButton.getSemesterColors(currentSemester)
                    }
                }
            }

            // ChatBot button
            Button {
                id: chatBotButton
                anchors {
                    right: preferenceButton.left
                    verticalCenter: parent.verticalCenter
                    rightMargin: 10
                }
                width: 40
                height: 40

                background: Rectangle {
                    color: chatBotMouseArea.containsMouse ? "#a8a8a8" : "#f3f4f6"
                    radius: 10
                    border.color: chatBot.isOpen ? "#3b82f6" : "transparent"
                    border.width: chatBot.isOpen ? 2 : 0
                }

                contentItem: Item {
                    anchors.fill: parent

                    Image {
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        source: "qrc:/icons/ic-assistant.svg"
                        sourceSize.width: 24
                        sourceSize.height: 24
                    }

                    ToolTip {
                        id: chatBotTooltip
                        text: chatBot.isOpen ? "Close SchedBot" : "Open SchedBot"
                        visible: chatBotMouseArea.containsMouse
                        delay: 500
                        timeout: 3000

                        background: Rectangle {
                            color: "#374151"
                            radius: 4
                            border.color: "#4b5563"
                        }

                        contentItem: Text {
                            text: chatBotTooltip.text
                            color: "white"
                            font.pixelSize: 12
                        }
                    }
                }

                MouseArea {
                    id: chatBotMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        chatBot.isOpen = !chatBot.isOpen
                    }
                }
            }

            // Sort button
            Button {
                id: preferenceButton
                width: 40
                height: 40
                anchors {
                    right: exportButton.left
                    verticalCenter: parent.verticalCenter
                    rightMargin: 10
                }

                background: Rectangle {
                    color: preferenceMouseArea.containsMouse ? "#a8a8a8" : "#f3f4f6"
                    radius: 10
                }

                contentItem: Item {
                    anchors.fill: parent

                    Image {
                        id: preferenceIcon
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        source: "qrc:/icons/ic-preference.svg"
                        sourceSize.width: 22
                        sourceSize.height: 22
                    }

                    ToolTip {
                        id: preferenceTooltip
                        text: "Set Schedule Preference"
                        visible: preferenceMouseArea.containsMouse
                        delay: 500
                        timeout: 3000

                        background: Rectangle {
                            color: "#374151"
                            radius: 4
                            border.color: "#4b5563"
                        }

                        contentItem: Text {
                            text: preferenceTooltip.text
                            color: "white"
                            font.pixelSize: 12
                        }
                    }
                }

                MouseArea {
                    id: preferenceMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: sortMenu.open()
                }
            }

            // Export button
            Button {
                id: exportButton
                width: 40
                height: 40
                anchors {
                    right: logButtonC.left
                    verticalCenter: parent.verticalCenter
                    rightMargin: 10
                }

                property bool isExportEnabled: scheduleModel && totalSchedules > 0

                background: Rectangle {
                    color: exportMouseArea.containsMouse ? "#a8a8a8" : "#f3f4f6"
                    radius: 10
                }

                contentItem: Item {
                    anchors.fill: parent

                    Image {
                        id: exportIcon
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        source: "qrc:/icons/ic-export.svg"
                        sourceSize.width: 22
                        sourceSize.height: 22
                    }

                    ToolTip {
                        id: exportTooltip
                        text: parent.parent.isExportEnabled ? "Export Schedule" : "No schedule to export"
                        visible: exportMouseArea.containsMouse
                        delay: 500
                        timeout: 3000

                        background: Rectangle {
                            color: "#374151"
                            radius: 4
                            border.color: "#4b5563"
                        }

                        contentItem: Text {
                            text: exportTooltip.text
                            color: "white"
                            font.pixelSize: 12
                        }
                    }
                }

                MouseArea {
                    id: exportMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: parent.isExportEnabled ? Qt.PointingHandCursor : Qt.ForbiddenCursor
                    onClicked: {
                        if (parent.isExportEnabled) {
                            exportMenu.currentIndex = currentIndex
                            exportMenu.open()
                        }
                    }
                }
            }

            // Logs button
            Button {
                id: logButtonC
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

                MouseArea  {
                    id: logMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (logWindow && logWindow.visible) {
                            logWindow.raise();
                            logWindow.requestActivate();
                            return;
                        }

                        if (!logDisplayController.isLogWindowOpen || !logWindow) {
                            var component = Qt.createComponent("qrc:/log_display.qml");
                            if (component.status === Component.Ready) {
                                logDisplayController.setLogWindowOpen(true);
                                logWindow = component.createObject(schedulesDisplayPage);

                                if (logWindow) {
                                    logWindow.closing.connect(function(close) {
                                        logDisplayController.setLogWindowOpen(false);
                                        logWindow = null;
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
        }
    }

    // Error message
    Rectangle {
        id: errorMessageContainer
        anchors {
            top: header.bottom
            left: parent.left
            right: parent.right
            rightMargin: chatBotOffset  // Account for chatbot
        }
        height: errorMessage === "" ? 0 : 40
        visible: errorMessage !== ""
        color: "#fef2f2"
        radius: 4
        border.color: "#fecaca"
        z: 1000  // Ensure it appears above other content

        // Smooth height animation
        Behavior on height {
            NumberAnimation {
                duration: 300
                easing.type: Easing.OutCubic
            }
        }

        // Fade in/out animation
        Behavior on opacity {
            NumberAnimation {
                duration: 200
            }
        }

        Label {
            anchors.centerIn: parent
            text: errorMessage
            color: "#dc2626"
            font.pixelSize: 14
            font.weight: Font.Medium
        }
    }

    // Main content
    Rectangle{
        id: mainContent
        anchors {
            top: header.bottom
            topMargin: errorMessageContainer.height
            left: parent.left
            right: parent.right
            bottom: footer.top
            rightMargin: chatBotOffset
        }

        // Animate width changes when chatbot opens/closes
        Behavior on width {
            NumberAnimation {
                duration: 400
                easing.type: Easing.OutCubic
            }
        }

        // Trim text based on stages (full, mid, min)
        function getFormattedText(courseName, courseId, building, room, type, cellWidth, cellHeight) {
            if (type === "Block") {
                return "<b style='font-size:" + baseTextSize + "px'>" + courseName + "</b>";
            }

            var charWidth = baseTextSize * 0.6;
            var maxCharsPerLine = Math.floor((cellWidth - 12) / charWidth); // Account for padding

            var line1, line2;

            // Full stage
            var fullLine1 = courseName + " (" + courseId + ")";
            var fullLine2 = "Building: " + building + ", Room: " + room;

            if (fullLine1.length <= maxCharsPerLine && fullLine2.length <= maxCharsPerLine) {
                line1 = fullLine1;
                line2 = fullLine2;
            } else {
                // Mid stage
                var courseNameLimit = Math.max(3, maxCharsPerLine - courseId.length - 6); // Minimum 3 chars for "..."
                var trimmedName = courseName.length > courseNameLimit ?
                    courseName.substring(0, courseNameLimit - 3) + "..." :
                    courseName;
                var midLine1 = trimmedName + " (" + courseId + ")";
                var midLine2 = "B: " + building + ", R: " + room;

                if (midLine1.length <= maxCharsPerLine && midLine2.length <= maxCharsPerLine) {
                    line1 = midLine1;
                    line2 = midLine2;
                } else {
                    // Min stage
                    line1 = courseId;

                    // original min building & room
                    var minLine2 = building + ", " + room;

                    if (minLine2.length <= maxCharsPerLine) {
                        line2 = minLine2;
                    } else {
                        // Trim building if needed
                        var roomSpace = room.length + 2;
                        var buildingLimit = maxCharsPerLine - roomSpace;

                        if (buildingLimit > 3) {
                            var trimmedBuilding = building.length > buildingLimit ?
                                building.substring(0, buildingLimit - 3) + "..." :
                                building;
                            line2 = trimmedBuilding + ", " + room;
                        } else {
                            if (building.length <= maxCharsPerLine) {
                                line2 = building;
                            } else {
                                line2 = building.substring(0, maxCharsPerLine - 3) + "...";
                            }
                        }
                    }
                }
            }

            return "<b style='font-size:" + baseTextSize + "px'>" + line1 + "</b><br>" +
                "<span style='font-size:" + baseTextSize + "px'>" + line2 + "</span>";
        }

        Item {
            anchors.fill: parent
            anchors.margins: 10

            Rectangle {
                id: noSchedulesMessage
                anchors.fill: parent
                visible: totalSchedules <= 0
                color: "#f9fafb"
                border.color: "#e5e7eb"
                border.width: 2
                radius: 8

                Column {
                    anchors.centerIn: parent
                    spacing: 16

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: isFiltered ?
                            "No schedules match your current filters." :
                            "No schedules available."
                        font.pixelSize: 24
                        font.bold: true
                        color: "#6b7280"
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        visible: isFiltered
                        text: "Try asking the bot to adjust your criteria,\nor click the filter indicator to reset."
                        font.pixelSize: 16
                        color: "#9ca3af"
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                // Loading animation when schedules are being generated
                Rectangle {
                    visible: !controller || !controller.hasSchedulesForSemester(currentSemester)
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                        top: parent.verticalCenter
                        topMargin: 40
                    }
                    width: 32
                    height: 32
                    radius: 16
                    color: "#3b82f6"

                    SequentialAnimation on opacity {
                        running: parent.visible
                        loops: Animation.Infinite
                        NumberAnimation { to: 0.3; duration: 800 }
                        NumberAnimation { to: 1.0; duration: 800 }
                    }

                    RotationAnimator on rotation {
                        running: parent.visible
                        from: 0
                        to: 360
                        duration: 2000
                        loops: Animation.Infinite
                    }
                }
            }

            Column {
                id: tableContent
                anchors.fill: parent
                visible: totalSchedules > 0
                spacing: 1

                // Animate the entire table content width when chatbot opens/closes
                width: parent.width
                Behavior on width {
                    NumberAnimation {
                        duration: 400
                        easing.type: Easing.OutCubic
                    }
                }

                // Recalculate column widths based on current table content width
                property real contentTimeColumnWidth: (width - 28) * 0.12
                property real contentDayColumnWidth: ((width - 28) - contentTimeColumnWidth) / numDays

                // Header row - uses tableContent's calculated widths
                Row {
                    id: dayHeaderRow
                    height: headerHeight
                    spacing: 1
                    width: parent.width

                    Rectangle {
                        width: tableContent.contentTimeColumnWidth
                        height: headerHeight
                        color: "#e5e7eb"
                        border.color: "#d1d5db"
                        radius: 4

                        Text {
                            anchors.centerIn: parent
                            text: "Hour/Day"
                            font.pixelSize: baseTextSize
                            font.bold: true
                            color: "#4b5563"
                        }
                    }

                    Repeater {
                        model: ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"]

                        Rectangle {
                            width: tableContent.contentDayColumnWidth
                            height: headerHeight
                            color: "#e5e7eb"
                            border.color: "#d1d5db"
                            radius: 4

                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                font.pixelSize: baseTextSize
                                font.bold: true
                                color: "#4b5563"
                                wrapMode: Text.WordWrap
                                elide: Text.ElideRight
                            }
                        }
                    }
                }

                // Schedule table
                TableView {
                    id: scheduleTable
                    width: parent.width
                    height: parent.height - headerHeight - 1
                    clip: true
                    rowSpacing: 1
                    columnSpacing: 1
                    interactive: false

                    property var timeSlots: [
                        "8:00-9:00", "9:00-10:00", "10:00-11:00", "11:00-12:00",
                        "12:00-13:00", "13:00-14:00", "14:00-15:00", "15:00-16:00",
                        "16:00-17:00", "17:00-18:00", "18:00-19:00", "19:00-20:00",
                        "20:00-21:00"
                    ]

                    columnWidthProvider: function(col) {
                        return col === 0 ? tableContent.contentTimeColumnWidth : tableContent.contentDayColumnWidth;
                    }

                    rowHeightProvider: function(row) {
                        return uniformRowHeight;
                    }

                    // Update layout when tableContent width changes
                    Connections {
                        target: tableContent
                        function onContentTimeColumnWidthChanged() {
                            Qt.callLater(scheduleTable.forceLayout)
                        }
                        function onContentDayColumnWidthChanged() {
                            Qt.callLater(scheduleTable.forceLayout)
                        }
                    }

                    model: TableModel {
                        id: tableModel
                        TableModelColumn { display: "timeSlot" }
                        TableModelColumn { display: "sunday" }
                        TableModelColumn { display: "monday" }
                        TableModelColumn { display: "tuesday" }
                        TableModelColumn { display: "wednesday" }
                        TableModelColumn { display: "thursday" }
                        TableModelColumn { display: "friday" }
                        TableModelColumn { display: "saturday" }

                        function updateRows() {
                            let rows = [];
                            const timeSlots = scheduleTable.timeSlots;
                            const days = ["sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday"];

                            for (let i = 0; i < timeSlots.length; i++) {
                                let row = { timeSlot: timeSlots[i] };
                                for (let day of days) {
                                    row[day] = "";
                                    row[day + "_type"] = "";
                                    row[day + "_courseName"] = "";
                                    row[day + "_courseId"] = "";
                                    row[day + "_building"] = "";
                                    row[day + "_room"] = "";
                                }
                                rows.push(row);
                            }

                            if (totalSchedules > 0 && scheduleModel) {
                                for (let day = 0; day < 7; day++) {
                                    let dayName = days[day];
                                    let items = scheduleModel.getCurrentDayItems(day);

                                    for (let itemIndex = 0; itemIndex < items.length; itemIndex++) {
                                        let item = items[itemIndex];
                                        let start = parseInt(item.start.split(":")[0]);
                                        let end = parseInt(item.end.split(":")[0]);

                                        for (let hour = start; hour < end; hour++) {
                                            for (let rowIndex = 0; rowIndex < timeSlots.length; rowIndex++) {
                                                let slot = timeSlots[rowIndex];
                                                let slotStart = parseInt(slot.split("-")[0].split(":")[0]);
                                                let slotEnd = parseInt(slot.split("-")[1].split(":")[0]);

                                                if (hour >= slotStart && hour < slotEnd) {
                                                    if (!rows[rowIndex][dayName + "_type"]) {
                                                        rows[rowIndex][dayName + "_type"] = item.type;
                                                        rows[rowIndex][dayName + "_courseName"] = item.courseName;
                                                        rows[rowIndex][dayName + "_courseId"] = item.raw_id;
                                                        rows[rowIndex][dayName + "_building"] = item.building;
                                                        rows[rowIndex][dayName + "_room"] = item.room;

                                                        rows[rowIndex][dayName] = mainContent.getFormattedText(
                                                            item.courseName,
                                                            item.raw_id,
                                                            item.building,
                                                            item.room,
                                                            item.type,
                                                            dayColumnWidth,
                                                            uniformRowHeight
                                                        );
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            tableModel.rows = rows;
                        }

                        Component.onCompleted: updateRows()
                    }

                    delegate: Rectangle {
                        implicitHeight: uniformRowHeight
                        implicitWidth: model.column === 0 ? tableContent.contentTimeColumnWidth : tableContent.contentDayColumnWidth
                        border.width: 1
                        border.color: "#e0e0e0"
                        radius: 4

                        property string columnName: {
                            switch(model.column) {
                                case 1: return "sunday_type";
                                case 2: return "monday_type";
                                case 3: return "tuesday_type";
                                case 4: return "wednesday_type";
                                case 5: return "thursday_type";
                                case 6: return "friday_type";
                                case 7: return "saturday_type";
                                default: return "";
                            }
                        }

                        property string itemType: {
                            if (columnName && model.row !== undefined) {
                                let rowData = parent.parent.model.rows[model.row];
                                if (rowData && rowData[columnName]) {
                                    return rowData[columnName];
                                }
                            }
                            return "";
                        }

                        color: {
                            if (model.column === 0) {
                                return "#d1d5db";
                            }

                            if (!model.display || String(model.display).trim().length === 0) {
                                return "#ffffff";
                            }

                            switch(itemType) {
                                case "Lecture": return "#b0e8ff";           // Light blue
                                case "Lab": return "#abffc6";               // Light green
                                case "Tutorial": return "#edc8ff";          // Light purple
                                case "Block": return "#7a7a7a";             // Gray
                                case "Departmental": return "#ffd6a5";  // Light orange
                                case "Reinforcement": return "#ffaaa5";     // Light red
                                case "Guidance": return "#a5d6ff";          // Light sky blue
                                case "Colloquium": return "#8b64ff"; //  purple
                                case "Registration": return "#ffa5d4";      // Light pink
                                case "Thesis": return "#a5ffd4";            // Light mint
                                case "Project": return "#d4ffa5";           // Light lime
                                default: return "#ff0000";                  // red
                            }
                        }

                        ToolTip {
                            id: sessionTooltip
                            text: itemType || "No session type"
                            visible: sessionMouseArea.containsMouse && itemType !== ""
                            delay: 500
                            timeout: 3000

                            background: Rectangle {
                                color: "#374151"
                                radius: 4
                                border.color: "#4b5563"
                            }

                            contentItem: Text {
                                text: sessionTooltip.text
                                color: "white"
                                font.pixelSize: 12
                            }
                        }

                        Text {
                            anchors.fill: parent
                            wrapMode: Text.WordWrap
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            padding: 4
                            font.pixelSize: baseTextSize
                            textFormat: Text.RichText
                            text: model.display ? String(model.display) : ""
                            color: itemType === "Block" ? "#ffffff": "#000000"
                            clip: true
                        }

                        MouseArea {
                            id: sessionMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                        }
                    }
                }
            }
        }
    }

    // Footer
    Rectangle {
        id: footer
        width: parent.width
        height: 30
        anchors.bottom: parent.bottom
        color: "#ffffff"
        border.color: "#e5e7eb"

        Label {
            anchors.centerIn: parent
            text: "© 2025 Schedulify. All rights reserved."
            color: "#6b7280"
            font.pixelSize: 12
        }
    }

    // export menu link
    ExportMenu {
        id: exportMenu
        parent: Overlay.overlay

        onPrintRequested: {
            if (controller) {
                controller.printScheduleDirectly()
            }
        }

        onSaveAsPngRequested: {
            if (schedulesDisplayController && tableContent) {
                schedulesDisplayController.captureAndSave(tableContent)
            }
        }

        onSaveAsCsvRequested: {
            if (controller) {
                controller.saveScheduleAsCSV()
            }
        }
    }

    // Sorting menu link
    SortMenu {
        id: sortMenu
        parent: Overlay.overlay
        onSortingApplied: function(sortingData) {
            if (controller) {
                controller.applySorting(sortingData)
            }
        }
    }

    // SchedBot Component link
    SchedBot {
        id: chatBot
        anchors {
            top: header.bottom
            bottom: footer.top
            right: parent.right
        }
        z: 1000
        controller: schedulesDisplayPage.controller
    }
}