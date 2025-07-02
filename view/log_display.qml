import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Basic

Window {
    id: logWindow
    width: 700
    height: 500
    title: "Schedulify Logs"
    color: "#f9fafb"

    flags: Qt.Window

    onVisibleChanged: {
        if (visible && logDisplayController) {
            logDisplayController.refreshLogs();
        }
    }

    Connections {
        target: logDisplayController

        function onLogEntriesChanged() {
            // Force the ListView to update by reassigning the model
            logListView.model = null;
            logListView.model = logDisplayController.logEntries;
        }
    }

    // FIXED: Connections to Logger for download feedback
    Connections {
        target: Logger

        function onLogsDownloaded(filePath) {
            downloadSuccessPopup.filePath = filePath;
            downloadSuccessPopup.open();
        }

        function onDownloadFailed(error) {
            downloadErrorPopup.errorMessage = error;
            downloadErrorPopup.open();
        }
    }

    Item {
        focus: true
        Keys.onEscapePressed: logWindow.close()
    }

    Rectangle {
        anchors.fill: parent
        color: "#f9fafb"

        // Header with title and buttons
        Rectangle {
            id: logHeader
            width: parent.width
            height: 60
            color: "#ffffff"
            border.color: "#e5e7eb"

            Text {
                id: headerText
                anchors {
                    left: parent.left
                    verticalCenter: parent.verticalCenter
                    leftMargin: 20
                }
                text: "Schedulify Logs"
                font.pixelSize: 18
                font.bold: true
                color: "#1f2937"
            }

            Row {
                anchors {
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                    rightMargin: 10
                }
                spacing: 8

                // Download button
                Button {
                    width: 32
                    height: 32
                    Text {
                        text: "⬇️"
                        anchors.centerIn: parent
                        font.pixelSize: 16
                        color: "#1f2937"
                    }

                    background: Rectangle {
                        color: parent.hovered ? "#f3f4f6" : "#ffffff"
                        radius: 16
                        border.color: "#e5e7eb"
                        border.width: 1
                    }

                    MouseArea {
                        id: downloadMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            // FIXED: Direct call to Logger.downloadLogs()
                            Logger.downloadLogs();
                        }
                    }

                    ToolTip {
                        visible: downloadMouseArea.containsMouse
                        text: "Download logs as TXT file"
                        delay: 500
                    }
                }

                // Refresh button
                Button {
                    width: 32
                    height: 32
                    Text {
                        text: "🔄"
                        anchors.centerIn: parent
                        font.pixelSize: 16
                        color: "#1f2937"
                    }

                    background: Rectangle {
                        color: parent.hovered ? "#f3f4f6" : "#ffffff"
                        radius: 16
                        border.color: "#e5e7eb"
                        border.width: 1
                    }

                    MouseArea {
                        id: refreshMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (logDisplayController) {
                                logDisplayController.refreshLogs();
                            }
                        }
                    }

                    ToolTip {
                        visible: refreshMouseArea.containsMouse
                        text: "Refresh logs"
                        delay: 500
                    }
                }
            }
        }

        // Logs ListView
        ListView {
            id: logListView
            anchors {
                top: logHeader.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                margins: 1
            }
            clip: true

            // Set both model binding and initial value
            model: logDisplayController ? logDisplayController.logEntries : []
            spacing: 1

            // Auto-scroll to bottom when content changes
            onCountChanged: {
                if (count > 0) {
                    positionViewAtEnd();
                }
            }

            ScrollBar.vertical: ScrollBar {
                active: true

                // Keep it at bottom when new items are added
                onPositionChanged: {
                    if (position + size < 1.0) {
                        // User has manually scrolled up, don't auto-scroll
                        logListView.autoScroll = false;
                    } else {
                        // At bottom, enable auto-scroll
                        logListView.autoScroll = true;
                    }
                }
            }

            property bool autoScroll: true

            // The rest of your delegate code remains the same
            delegate: Rectangle {
                width: logListView.width
                height: contentLayout.implicitHeight + 16
                color: "#ffffff"

                // Separator line
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    color: "#e5e7eb"
                }

                RowLayout {
                    id: contentLayout
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12

                    // Log level indicator
                    Rectangle {
                        Layout.preferredWidth: 6
                        Layout.fillHeight: true
                        color: modelData.color
                        radius: 3
                    }

                    // Log level badge
                    Rectangle {
                        Layout.preferredWidth: 80
                        Layout.preferredHeight: 26
                        color: Qt.alpha(modelData.color, 0.15)
                        radius: 4

                        Text {
                            anchors.centerIn: parent
                            text: modelData.level
                            color: modelData.color
                            font.pixelSize: 12
                            font.bold: true
                        }
                    }

                    // Timestamp
                    Text {
                        Layout.preferredWidth: 140
                        text: modelData.timestamp
                        color: "#64748b"
                        font.pixelSize: 12
                        font.family: "Consolas, monospace"
                    }

                    // Message
                    Text {
                        Layout.fillWidth: true
                        text: modelData.message
                        color: "#1e293b"
                        font.pixelSize: 14
                        wrapMode: Text.Wrap
                    }
                }
            }

            // Empty state
            Rectangle {
                anchors.centerIn: parent
                width: 300
                height: 100
                color: "transparent"
                visible: logDisplayController ? logDisplayController.logEntries.length === 0 : true

                Column {
                    anchors.centerIn: parent
                    spacing: 10

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "📋"
                        font.pixelSize: 32
                        color: "#9ca3af"
                    }

                    Text {
                        text: "No logs available"
                        font.pixelSize: 16
                        color: "#6b7280"
                    }
                }
            }
        }
    }

    // Success popup
    Popup {
        id: downloadSuccessPopup
        anchors.centerIn: parent
        width: 450
        height: 200
        modal: true

        property string filePath: ""

        background: Rectangle {
            color: "#ffffff"
            radius: 8
            border.color: "#e5e7eb"
            border.width: 1

            // Drop shadow effect
            Rectangle {
                anchors.fill: parent
                anchors.margins: -2
                color: "transparent"
                radius: 10
                border.color: "#00000010"
                border.width: 2
                z: -1
            }
        }

        Column {
            anchors.centerIn: parent
            spacing: 20
            width: parent.width - 40  // Add proper margins

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "✅"
                font.pixelSize: 32
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Logs downloaded successfully!"
                font.pixelSize: 16
                font.bold: true
                color: "#059669"
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Saved to: " + downloadSuccessPopup.filePath
                font.pixelSize: 12
                color: "#64748b"
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                width: parent.width - 20  // Ensure text fits within bounds
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "OK"
                width: 80
                height: 36
                onClicked: downloadSuccessPopup.close()

                background: Rectangle {
                    color: "#059669"
                    radius: 4
                }

                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }

    // Error popup
    Popup {
        id: downloadErrorPopup
        anchors.centerIn: parent
        width: 400
        height: 150
        modal: true

        property string errorMessage: ""

        background: Rectangle {
            color: "#ffffff"
            radius: 8
            border.color: "#e5e7eb"
            border.width: 1

            // Drop shadow effect
            Rectangle {
                anchors.fill: parent
                anchors.margins: -2
                color: "transparent"
                radius: 10
                border.color: "#00000010"
                border.width: 2
                z: -1
            }
        }

        Column {
            anchors.centerIn: parent
            spacing: 15

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "❌"
                font.pixelSize: 32
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Download failed"
                font.pixelSize: 16
                font.bold: true
                color: "#dc2626"
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: downloadErrorPopup.errorMessage
                font.pixelSize: 12
                color: "#64748b"
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "OK"
                onClicked: downloadErrorPopup.close()

                background: Rectangle {
                    color: "#dc2626"
                    radius: 4
                }

                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }

    onClosing: {
        if (logDisplayController) {
            logDisplayController.setLogWindowOpen(false);
        }
    }
}