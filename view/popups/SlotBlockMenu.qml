import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Basic

Popup {
    id: root

    // Modified signal to include semester
    signal blockTimeAdded(string day, string startTime, string endTime, string semester)

    // Properties
    property int startHour: 8
    property int endHour: 9
    property string errorMessage: ""
    property bool entireDay: false

    // Semester selection property
    property string selectedSemester: "A" // Default to Semester A

    width: 400
    height: 480  // Increased height for semester selection
    modal: true
    focus: true
    clip: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

    background: Rectangle {
        color: "#ffffff"
        border.color: "#d1d5db"
        border.width: 1
        radius: 8

        // Drop shadow effect
        Rectangle {
            anchors.fill: parent
            anchors.margins: -2
            color: "transparent"
            border.color: "#00000020"
            border.width: 1
            radius: 10
            z: -1
        }
    }

    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    Timer {
        id: errorMessageTimer
        interval: 3000
        onTriggered: {
            errorMessage = ""
        }
    }

    function getFormattedTime(hour) {
        return String(hour).padStart(2, '0') + ":00";
    }

    function validateTimes() {
        if (entireDay) {
            startHour = 8;
            endHour = 21;
            return;
        }

        if (endHour <= startHour) {
            endHour = startHour + 1;
            if (endHour > 23) {
                endHour = 23;
                startHour = 22;
            }
        }
    }

    function addBlockTime() {
        if (daySelector.currentIndex < 0) {
            errorMessage = "Please select a day";
            errorMessageTimer.restart();
            return;
        }

        validateTimes();

        var day = daySelector.currentText;
        var startTime = getFormattedTime(startHour);
        var endTime = getFormattedTime(endHour);

        // Include semester in the signal
        root.blockTimeAdded(day, startTime, endTime, selectedSemester);

        daySelector.currentIndex = 0;
        startHour = 8;
        endHour = 9;
        entireDay = false;
        selectedSemester = "A"; // Reset to default
        errorMessage = "";

        root.close();
    }

    onEntireDayChanged: {
        if (entireDay) {
            startHour = 8;
            endHour = 21;
        }
    }

    Item {
        id: contentContainer
        anchors {
            fill: parent
            margins: 20
        }

        // Header
        Item {
            id: headerSection
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
            }
            height: 40

            Text {
                id: headerTitle
                anchors {
                    left: parent.left
                    verticalCenter: parent.verticalCenter
                }
                text: "Add Block Time"
                font.pixelSize: 20
                font.bold: true
                color: "#1f2937"
            }

            Button {
                id: closeButton
                anchors {
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                }
                width: 30
                height: 30

                background: Rectangle {
                    color: closeMouseArea.containsMouse ? "#f3f4f6" : "transparent"
                    radius: 15
                }

                contentItem: Text {
                    text: "×"
                    font.pixelSize: 20
                    color: "#6b7280"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                MouseArea {
                    id: closeMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.close()
                    cursorShape: Qt.PointingHandCursor
                }
            }
        }

        // Error message
        Rectangle {
            id: errorMessageContainer
            anchors {
                top: headerSection.bottom
                topMargin: 20
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

        // Day selection
        Item {
            id: daySelectionSection
            anchors {
                top: errorMessageContainer.bottom
                topMargin: errorMessage === "" ? 0 : 20
                left: parent.left
                right: parent.right
            }
            height: 70

            Label {
                id: dayLabel
                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                }
                height: 22
                text: "Day of Week"
                font.pixelSize: 14
                font.bold: true
                color: "#374151"
            }

            Rectangle {
                id: dayComboRect
                anchors {
                    top: dayLabel.bottom
                    topMargin: 8
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }
                color: "#f9fafb"
                radius: 6
                border.width: 1
                border.color: "#d1d5db"

                ComboBox {
                    id: daySelector
                    anchors.fill: parent
                    model: ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"]
                    currentIndex: 0

                    background: Rectangle {
                        color: "transparent"
                        radius: 6
                    }

                    contentItem: Text {
                        leftPadding: 12
                        rightPadding: daySelector.indicator.width + daySelector.spacing
                        text: daySelector.displayText
                        font.pixelSize: 14
                        color: "#374151"
                        verticalAlignment: Text.AlignVCenter
                    }

                    indicator: Text {
                        x: daySelector.width - width - 10
                        y: daySelector.topPadding + (daySelector.availableHeight - height) / 2
                        text: "▼"
                        font.pixelSize: 10
                        color: "#6b7280"
                    }
                }
            }
        }

        // Semester selection
        Item {
            id: semesterSelectionSection
            anchors {
                top: daySelectionSection.bottom
                topMargin: 20
                left: parent.left
                right: parent.right
            }
            height: 70

            Label {
                id: semesterLabel
                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                }
                height: 22
                text: "Semester"
                font.pixelSize: 14
                font.bold: true
                color: "#374151"
            }

            Rectangle {
                id: semesterContainer
                anchors {
                    top: semesterLabel.bottom
                    topMargin: 8
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }
                color: "#f9fafb"
                radius: 6
                border.width: 1
                border.color: "#d1d5db"

                Row {
                    anchors.centerIn: parent
                    spacing: 15

                    ButtonGroup { id: semesterGroup }

                    // Semester A
                    Rectangle {
                        width: 80
                        height: 32
                        radius: 5
                        color: selectedSemester === "A" ? "#3b82f6" : "#ffffff"
                        border.width: 2
                        border.color: selectedSemester === "A" ? "#3b82f6" : "#d1d5db"

                        Text {
                            anchors.centerIn: parent
                            text: "Semester A"
                            font.pixelSize: 12
                            font.bold: selectedSemester === "A"
                            color: selectedSemester === "A" ? "#ffffff" : "#374151"
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: selectedSemester = "A"
                            cursorShape: Qt.PointingHandCursor
                        }

                        // Hover effect
                        Rectangle {
                            anchors.fill: parent
                            radius: 5
                            color: "#3b82f6"
                            opacity: selectedSemester !== "A" && parent.children[1].containsMouse ? 0.1 : 0
                            Behavior on opacity {
                                NumberAnimation { duration: 150 }
                            }
                        }
                    }

                    // Semester B
                    Rectangle {
                        width: 80
                        height: 32
                        radius: 5
                        color: selectedSemester === "B" ? "#3b82f6" : "#ffffff"
                        border.width: 2
                        border.color: selectedSemester === "B" ? "#3b82f6" : "#d1d5db"

                        Text {
                            anchors.centerIn: parent
                            text: "Semester B"
                            font.pixelSize: 12
                            font.bold: selectedSemester === "B"
                            color: selectedSemester === "B" ? "#ffffff" : "#374151"
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: selectedSemester = "B"
                            cursorShape: Qt.PointingHandCursor
                        }

                        // Hover effect
                        Rectangle {
                            anchors.fill: parent
                            radius: 5
                            color: "#3b82f6"
                            opacity: selectedSemester !== "B" && parent.children[1].containsMouse ? 0.1 : 0
                            Behavior on opacity {
                                NumberAnimation { duration: 150 }
                            }
                        }
                    }

                    // Summer
                    Rectangle {
                        width: 70
                        height: 32
                        radius: 5
                        color: selectedSemester === "SUMMER" ? "#3b82f6" : "#ffffff"
                        border.width: 2
                        border.color: selectedSemester === "SUMMER" ? "#3b82f6" : "#d1d5db"

                        Text {
                            anchors.centerIn: parent
                            text: "Summer"
                            font.pixelSize: 12
                            font.bold: selectedSemester === "SUMMER"
                            color: selectedSemester === "SUMMER" ? "#ffffff" : "#374151"
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: selectedSemester = "SUMMER"
                            cursorShape: Qt.PointingHandCursor
                        }

                        // Hover effect
                        Rectangle {
                            anchors.fill: parent
                            radius: 5
                            color: "#3b82f6"
                            opacity: selectedSemester !== "SUMMER" && parent.children[1].containsMouse ? 0.1 : 0
                            Behavior on opacity {
                                NumberAnimation { duration: 150 }
                            }
                        }
                    }
                }
            }
        }

        // Time selection section
        Item {
            id: timeSelectionSection
            anchors {
                top: semesterSelectionSection.bottom // Changed from daySelectionSection.bottom
                topMargin: 20
                left: parent.left
                right: parent.right
            }
            height: 100

            Label {
                id: timeLabel
                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                }
                height: 22
                text: "Time Range"
                font.pixelSize: 14
                font.bold: true
                color: "#374151"
            }

            Item {
                id: timeControlsContainer
                anchors {
                    top: timeLabel.bottom
                    topMargin: 8
                    left: parent.left
                    right: entireDayToggleContainer.left
                    rightMargin: 12
                    bottom: parent.bottom
                }

                // Start time
                Item {
                    id: startTimeContainer
                    anchors {
                        top: parent.top
                        left: parent.left
                        bottom: parent.bottom
                    }
                    width: (parent.width - 10) / 2

                    Label {
                        id: startTimeLabel
                        anchors {
                            top: parent.top
                            left: parent.left
                            right: parent.right
                        }
                        height: 18
                        text: "Start Time"
                        font.pixelSize: 12
                        color: "#6b7280"
                    }

                    Rectangle {
                        id: startTimeRect
                        anchors {
                            top: startTimeLabel.bottom
                            topMargin: 4
                            left: parent.left
                            right: parent.right
                            bottom: parent.bottom
                        }
                        color: entireDay ? "#f3f4f6" : "#f9fafb"
                        radius: 6
                        border.width: 1
                        border.color: "#d1d5db"
                        opacity: entireDay ? 0.5 : 1.0

                        Text {
                            id: startTimeDisplay
                            anchors.centerIn: parent
                            text: getFormattedTime(startHour)
                            font.pixelSize: 16
                            font.bold: true
                            color: entireDay ? "#9ca3af" : "#374151"
                        }

                        Item {
                            id: startTimeControls
                            anchors {
                                right: parent.right
                                rightMargin: 8
                                verticalCenter: parent.verticalCenter
                            }
                            width: 20
                            height: parent.height - 8

                            Rectangle {
                                id: startUpButton
                                anchors {
                                    top: parent.top
                                    left: parent.left
                                    right: parent.right
                                }
                                height: 18
                                color: (!entireDay && upStartHourMouseArea.containsMouse) ? "#e5e7eb" : "#f3f4f6"
                                radius: 3
                                border.width: 1
                                border.color: "#d1d5db"
                                opacity: entireDay ? 0.5 : 1.0

                                Text {
                                    anchors.centerIn: parent
                                    text: "▲"
                                    font.pixelSize: 10
                                    color: entireDay ? "#9ca3af" : "#374151"
                                }

                                MouseArea {
                                    id: upStartHourMouseArea
                                    anchors.fill: parent
                                    hoverEnabled: !entireDay
                                    enabled: !entireDay
                                    onClicked: {
                                        if (startHour < 23) {
                                            startHour = startHour + 1;
                                            validateTimes();
                                        }
                                    }
                                    cursorShape: entireDay ? Qt.ArrowCursor : Qt.PointingHandCursor
                                }
                            }

                            Rectangle {
                                id: startDownButton
                                anchors {
                                    bottom: parent.bottom
                                    left: parent.left
                                    right: parent.right
                                }
                                height: 18
                                color: (!entireDay && downStartHourMouseArea.containsMouse) ? "#e5e7eb" : "#f3f4f6"
                                radius: 3
                                border.width: 1
                                border.color: "#d1d5db"
                                opacity: entireDay ? 0.5 : 1.0

                                Text {
                                    anchors.centerIn: parent
                                    text: "▼"
                                    font.pixelSize: 10
                                    color: entireDay ? "#9ca3af" : "#374151"
                                }

                                MouseArea {
                                    id: downStartHourMouseArea
                                    anchors.fill: parent
                                    hoverEnabled: !entireDay
                                    enabled: !entireDay
                                    onClicked: {
                                        if (startHour > 0) {
                                            startHour = startHour - 1;
                                            validateTimes();
                                        }
                                    }
                                    cursorShape: entireDay ? Qt.ArrowCursor : Qt.PointingHandCursor
                                }
                            }
                        }
                    }
                }

                // End time
                Item {
                    id: endTimeContainer
                    anchors {
                        top: parent.top
                        right: parent.right
                        bottom: parent.bottom
                    }
                    width: (parent.width - 10) / 2

                    Label {
                        id: endTimeLabel
                        anchors {
                            top: parent.top
                            left: parent.left
                            right: parent.right
                        }
                        height: 18
                        text: "End Time"
                        font.pixelSize: 12
                        color: "#6b7280"
                    }

                    Rectangle {
                        id: endTimeRect
                        anchors {
                            top: endTimeLabel.bottom
                            topMargin: 4
                            left: parent.left
                            right: parent.right
                            bottom: parent.bottom
                        }
                        color: entireDay ? "#f3f4f6" : "#f9fafb"
                        radius: 6
                        border.width: 1
                        border.color: "#d1d5db"
                        opacity: entireDay ? 0.5 : 1.0

                        Text {
                            id: endTimeDisplay
                            anchors.centerIn: parent
                            text: getFormattedTime(endHour)
                            font.pixelSize: 16
                            font.bold: true
                            color: entireDay ? "#9ca3af" : "#374151"
                        }

                        Item {
                            id: endTimeControls
                            anchors {
                                right: parent.right
                                rightMargin: 8
                                verticalCenter: parent.verticalCenter
                            }
                            width: 20
                            height: parent.height - 8

                            Rectangle {
                                id: endUpButton
                                anchors {
                                    top: parent.top
                                    left: parent.left
                                    right: parent.right
                                }
                                height: 18
                                color: (!entireDay && upEndHourMouseArea.containsMouse) ? "#e5e7eb" : "#f3f4f6"
                                radius: 3
                                border.width: 1
                                border.color: "#d1d5db"
                                opacity: entireDay ? 0.5 : 1.0

                                Text {
                                    anchors.centerIn: parent
                                    text: "▲"
                                    font.pixelSize: 10
                                    color: entireDay ? "#9ca3af" : "#374151"
                                }

                                MouseArea {
                                    id: upEndHourMouseArea
                                    anchors.fill: parent
                                    hoverEnabled: !entireDay
                                    enabled: !entireDay
                                    onClicked: {
                                        if (endHour < 23) {
                                            endHour = endHour + 1;
                                        }
                                    }
                                    cursorShape: entireDay ? Qt.ArrowCursor : Qt.PointingHandCursor
                                }
                            }

                            Rectangle {
                                id: endDownButton
                                anchors {
                                    bottom: parent.bottom
                                    left: parent.left
                                    right: parent.right
                                }
                                height: 18
                                color: (!entireDay && downEndHourMouseArea.containsMouse) ? "#e5e7eb" : "#f3f4f6"
                                radius: 3
                                border.width: 1
                                border.color: "#d1d5db"
                                opacity: entireDay ? 0.5 : 1.0

                                Text {
                                    anchors.centerIn: parent
                                    text: "▼"
                                    font.pixelSize: 10
                                    color: entireDay ? "#9ca3af" : "#374151"
                                }

                                MouseArea {
                                    id: downEndHourMouseArea
                                    anchors.fill: parent
                                    hoverEnabled: !entireDay
                                    enabled: !entireDay
                                    onClicked: {
                                        if (endHour > 0) {
                                            endHour = endHour - 1;
                                            validateTimes();
                                        }
                                    }
                                    cursorShape: entireDay ? Qt.ArrowCursor : Qt.PointingHandCursor
                                }
                            }
                        }
                    }
                }
            }

            Item {
                id: entireDayToggleContainer
                anchors {
                    top: timeLabel.bottom
                    topMargin: 8
                    right: parent.right
                    bottom: parent.bottom
                }
                width: 120

                Label {
                    id: toggleSectionLabel
                    anchors {
                        top: parent.top
                        left: parent.left
                        right: parent.right
                    }
                    height: 18
                    text: "Entire day"
                    font.pixelSize: 12
                    color: "#6b7280"
                }

                Rectangle {
                    id: toggleBackground
                    anchors {
                        top: toggleSectionLabel.bottom
                        topMargin: 4
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                    }
                    color: "#f9fafb"
                    radius: 6
                    border.width: 1
                    border.color: "#d1d5db"

                    Item {
                        id: toggleContent
                        anchors {
                            fill: parent
                            margins: 12
                        }

                        Rectangle {
                            id: toggleSwitch
                            anchors {
                                top: parent.top
                                horizontalCenter: parent.horizontalCenter
                            }
                            width: 50
                            height: 26
                            radius: 13
                            color: entireDay ? "#2563eb" : "#d1d5db"
                            border.width: 1
                            border.color: entireDay ? "#1d4ed8" : "#9ca3af"

                            Rectangle {
                                id: toggleCircle
                                width: 22
                                height: 22
                                radius: 11
                                color: "#ffffff"
                                anchors.verticalCenter: parent.verticalCenter
                                x: entireDay ? parent.width - width - 2 : 2

                                Behavior on x {
                                    NumberAnimation {
                                        duration: 200
                                        easing.type: Easing.OutCubic
                                    }
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    entireDay = !entireDay;
                                    validateTimes();
                                }
                                cursorShape: Qt.PointingHandCursor
                            }
                        }

                    }
                }
            }
        }

        // Action buttons
        Item {
            id: actionButtonsSection
            anchors {
                bottom: parent.bottom
                left: parent.left
                right: parent.right
            }
            height: 45

            Button {
                id: cancelButton
                anchors {
                    left: parent.left
                    top: parent.top
                    bottom: parent.bottom
                }
                width: (parent.width - 12) / 2

                background: Rectangle {
                    color: cancelMouseArea.containsMouse ? "#f3f4f6" : "#ffffff"
                    radius: 6
                    border.width: 1
                    border.color: "#d1d5db"
                }

                contentItem: Text {
                    text: "Cancel"
                    font.pixelSize: 14
                    color: "#374151"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                MouseArea {
                    id: cancelMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.close()
                    cursorShape: Qt.PointingHandCursor
                }
            }

            Button {
                id: addButton
                anchors {
                    right: parent.right
                    top: parent.top
                    bottom: parent.bottom
                }
                width: (parent.width - 12) / 2

                background: Rectangle {
                    color: addMouseArea.containsMouse ? "#2563eb" : "#3b82f6"
                    radius: 6
                }

                contentItem: Text {
                    text: "Add"
                    font.pixelSize: 14
                    font.bold: true
                    color: "#ffffff"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                MouseArea {
                    id: addMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: addBlockTime()
                    cursorShape: Qt.PointingHandCursor
                }
            }
        }
    }
}