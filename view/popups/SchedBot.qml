import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Basic

Rectangle {
    id: chatBot
    width: 350
    height: parent.height
    color: "#ffffff"
    border.color: "#e5e7eb"
    border.width: 1

    // Expose isOpen property to parent - ensure it starts as false
    property bool isOpen: false
    property alias messagesModel: messagesListModel
    property bool isProcessing: false

    // Reference to the controller for model operations
    property var controller: null

    // Loading animation properties
    property int currentLoadingMessageIndex: 0
    property var loadingMessages: [
        "Analyzing your request...",
        "Checking schedule database...",
        "Building filter criteria...",
        "Searching for matching schedules...",
        "Processing schedule patterns...",
        "Finding optimal matches...",
        "Applying intelligent filters...",
        "Almost done..."
    ]

    // Hide completely when closed - like the original
    visible: isOpen

    // Position for sliding animation
    x: parent.width - width

    // Slide animation using transform instead of x position
    transform: Translate {
        id: slideTransform
        x: isOpen ? 0 : width  // Slide by the chatbot's width

        Behavior on x {
            NumberAnimation {
                duration: 400
                easing.type: Easing.OutCubic
            }
        }
    }

    // Keep fully opaque
    opacity: 1.0

    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: -5
        color: "transparent"
        border.color: isOpen ? "#00000020" : "transparent"
        border.width: isOpen ? 5 : 0
        radius: 8
        z: -1
    }

    Rectangle {
        id: leftBorder
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }
        width: 1
        color: "#e2e8f0"
        visible: isOpen

        gradient: Gradient {
            GradientStop { position: 0.0; color: "#e2e8f0" }
            GradientStop { position: 0.5; color: "#e2e8f0" }
            GradientStop { position: 1.0; color: "#e2e8f0" }
        }
    }

    // Loading message timer
    Timer {
        id: loadingMessageTimer
        interval: 1800  // Change message every 1.8 seconds
        repeat: true
        running: false
        onTriggered: {
            currentLoadingMessageIndex = (currentLoadingMessageIndex + 1) % loadingMessages.length
            updateLoadingMessage()
        }
    }

    // Header
    Rectangle {
        id: header
        width: parent.width
        height: 60
        color: "#f8fafc"
        border.color: "#e2e8f0"
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.margins: 15
            spacing: 12

            // Bot icon with pulse animation when processing
            Rectangle {
                id: botIconRect
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                color: "#3b82f6"
                radius: 16

                // Pulse animation when processing
                SequentialAnimation {
                    loops: Animation.Infinite
                    running: isProcessing

                    ScaleAnimator {
                        target: botIconRect
                        from: 1.0
                        to: 1.1
                        duration: 800
                        easing.type: Easing.InOutQuad
                    }
                    ScaleAnimator {
                        target: botIconRect
                        from: 1.1
                        to: 1.0
                        duration: 800
                        easing.type: Easing.InOutQuad
                    }
                }

                Image {
                    anchors.centerIn: parent
                    width: 20
                    height: 20
                    source: "qrc:/icons/ic-assistant.svg"
                    sourceSize.width: 20
                    sourceSize.height: 20
                }
            }

            // Title with status indicator
            Column {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    text: "SchedBot"
                    font.pixelSize: 18
                    font.bold: true
                    color: "#1f2937"
                    verticalAlignment: Text.AlignVCenter
                }

                Text {
                    id: statusText
                    text: isProcessing ? "Processing..." : "Ready to filter"
                    font.pixelSize: 10
                    color: isProcessing ? "#f59e0b" : "#10b981"
                    visible: true

                    // Fade animation for status
                    OpacityAnimator {
                        target: statusText
                        from: 0.5
                        to: 1.0
                        duration: 1000
                        loops: Animation.Infinite
                        running: isProcessing
                    }
                }
            }

            // Close button with enhanced animation
            Button {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32

                background: Rectangle {
                    color: closeMouseArea.containsMouse ? "#f3f4f6" : "transparent"
                    radius: 6

                    // Smooth color transition
                    Behavior on color {
                        ColorAnimation {
                            duration: 200
                        }
                    }
                }

                contentItem: Text {
                    text: "×"
                    font.pixelSize: 18
                    color: "#6b7280"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter

                    // Scale animation on hover
                    scale: closeMouseArea.containsMouse ? 1.1 : 1.0

                    Behavior on scale {
                        NumberAnimation {
                            duration: 150
                            easing.type: Easing.OutQuad
                        }
                    }
                }

                MouseArea {
                    id: closeMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: chatBot.isOpen = false
                }
            }
        }
    }

    // Messages area
    Rectangle {
        id: messagesArea
        anchors {
            top: header.bottom
            left: parent.left
            right: parent.right
            bottom: inputArea.top
            leftMargin: 4  // Account for the left border
        }
        color: "#ffffff"

        ScrollView {
            id: scrollView
            anchors.fill: parent
            anchors.margins: 1
            clip: true

            ListView {
                id: messagesListView
                model: ListModel {
                    id: messagesListModel

                    Component.onCompleted: {
                        // Add initial bot message with updated functionality
                        append({
                            "isBot": true,
                            "message": "Hello! I'm your schedule assistant. I can help you filter schedules based on your preferences. Try asking me things like:\n\n• 'Show me schedules with no gaps'\n• 'Find schedules that start after 9 AM'\n• 'I want schedules with maximum 4 study days'\n• 'Show schedules ending before 5 PM'\n\nUse the blue 'Filtered' badge in the header to reset filters.",
                            "timestamp": new Date().toLocaleTimeString(Qt.locale(), "hh:mm"),
                            "isTyping": false,
                            "isFilterResponse": false
                        })
                    }
                }

                spacing: 16

                delegate: Item {
                    width: messagesListView.width
                    height: messageContainer.height + 16

                    Rectangle {
                        id: messageContainer
                        width: Math.min(parent.width * 0.85, messageText.implicitWidth + 24)
                        height: messageText.implicitHeight + 40

                        anchors {
                            left: model.isBot ? parent.left : undefined
                            right: model.isBot ? undefined : parent.right
                            leftMargin: model.isBot ? 16 : 0
                            rightMargin: model.isBot ? 0 : 16
                        }

                        color: {
                            if (model.isBot) {
                                return model.isFilterResponse ? "#f0f9ff" : "#f8fafc"
                            }
                            return "#3b82f6"
                        }

                        radius: 16
                        border.color: {
                            if (model.isBot) {
                                return model.isFilterResponse ? "#0ea5e9" : "#e2e8f0"
                            }
                            return "transparent"
                        }
                        border.width: model.isBot && model.isFilterResponse ? 1 : 1

                        // Filter response indicator
                        Rectangle {
                            visible: model.isBot && model.isFilterResponse
                            anchors {
                                top: parent.top
                                right: parent.right
                                margins: 8
                            }
                            width: 20
                            height: 20
                            color: "#0ea5e9"
                            radius: 10

                            Text {
                                anchors.centerIn: parent
                                text: "🔍"
                                font.pixelSize: 10
                                color: "white"
                            }

                            ToolTip {
                                visible: filterIndicatorMouseArea.containsMouse
                                text: "Filter applied to schedules"
                                delay: 500
                            }

                            MouseArea {
                                id: filterIndicatorMouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                            }
                        }

                        // Add subtle shadow effect
                        Rectangle {
                            anchors.fill: parent
                            anchors.topMargin: 2
                            color: model.isBot ? "#00000008" : "#00000015"
                            radius: parent.radius
                            z: -1
                        }

                        // Simple loading text - selectable
                        TextEdit {
                            id: loadingText
                            anchors {
                                left: parent.left
                                right: parent.right
                                top: parent.top
                                bottom: parent.bottom
                                margins: 16
                                bottomMargin: 28
                            }

                            text: model.message
                            font.pixelSize: 14
                            color: "#6b7280"
                            wrapMode: TextEdit.WordWrap
                            verticalAlignment: TextEdit.AlignTop
                            visible: model.isTyping === true
                            font.italic: true
                            readOnly: true
                            selectByMouse: true
                            selectByKeyboard: true
                        }

                        TextEdit {
                            id: messageText
                            anchors {
                                left: parent.left
                                right: parent.right
                                top: parent.top
                                bottom: parent.bottom
                                margins: 16
                                bottomMargin: 28
                                rightMargin: model.isBot && model.isFilterResponse ? 36 : 16
                            }

                            text: model.message
                            font.pixelSize: 14
                            color: model.isBot ? "#1f2937" : "#ffffff"
                            wrapMode: TextEdit.WordWrap
                            verticalAlignment: TextEdit.AlignTop
                            visible: model.isTyping !== true
                            readOnly: true
                            selectByMouse: true
                            selectByKeyboard: true

                            // Enhanced slide in animation for new messages
                            opacity: 0
                            x: model.isBot ? -20 : 20

                            Component.onCompleted: {
                                slideInAnimation.start()
                            }

                            ParallelAnimation {
                                id: slideInAnimation

                                OpacityAnimator {
                                    target: messageText
                                    from: 0
                                    to: 1
                                    duration: 400
                                    easing.type: Easing.OutQuad
                                }

                                NumberAnimation {
                                    target: messageText
                                    property: "x"
                                    from: model.isBot ? -20 : 20
                                    to: 0
                                    duration: 400
                                    easing.type: Easing.OutCubic
                                }
                            }
                        }

                        Text {
                            anchors {
                                right: parent.right
                                bottom: parent.bottom
                                margins: 10
                            }

                            text: model.timestamp
                            font.pixelSize: 10
                            color: model.isBot ? "#6b7280" : "#e0e7ff"
                            visible: model.isTyping !== true
                        }
                    }
                }

                // Auto-scroll to bottom when new message is added
                onCountChanged: {
                    Qt.callLater(function() {
                        if (count > 0) {
                            positionViewAtEnd()
                        }
                    })
                }
            }
        }
    }

    // Input area
    Rectangle {
        id: inputArea
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        width: parent.width
        height: 70
        color: "#f8fafc"
        border.color: "#e2e8f0"
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: 46  // Fixed height for exactly 2 rows
                Layout.maximumHeight: 46    // Prevent expansion
                Layout.minimumHeight: 46    // Prevent shrinking

                TextArea {
                    id: inputField
                    placeholderText: "Ask me to filter schedules..."
                    placeholderTextColor: "#9ca3af"
                    font.pixelSize: 14
                    color: "#1f2937"
                    wrapMode: TextArea.Wrap
                    selectByMouse: true

                    // Force exactly 2 rows
                    height: 46

                    background: Rectangle {
                        color: "#ffffff"
                        border.color: inputField.activeFocus ? "#3b82f6" : "#d1d5db"
                        border.width: 1
                        radius: 8
                    }

                    Keys.onReturnPressed: function(event) {
                        if (event.modifiers & Qt.ControlModifier) {
                            event.accepted = false
                        } else {
                            sendMessage()
                            event.accepted = true
                        }
                    }
                }
            }

            Button {
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40

                enabled: inputField.text.trim().length > 0 && !isProcessing

                background: Rectangle {
                    color: {
                        if (!parent.enabled) return "#e5e7eb"
                        if (isProcessing) return "#f59e0b"
                        return sendMouseArea.containsMouse ? "#2563eb" : "#3b82f6"
                    }
                    radius: 8

                    // Enhanced scale animation on hover
                    scale: sendMouseArea.containsMouse && parent.enabled ? 1.05 : 1.0

                    Behavior on scale {
                        NumberAnimation {
                            duration: 150
                            easing.type: Easing.OutQuad
                        }
                    }
                }

                contentItem: Item {
                    anchors.fill: parent

                    // Regular send arrow
                    Text {
                        anchors.centerIn: parent
                        text: "🔍"  // Filter emoji to indicate filtering capability
                        font.pixelSize: 16
                        color: parent.parent.enabled ? "#ffffff" : "#9ca3af"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        visible: !isProcessing
                    }

                    // Enhanced loading spinner
                    Rectangle {
                        id: loadingSpinner
                        anchors.centerIn: parent
                        width: 20
                        height: 20
                        radius: 10
                        color: "transparent"
                        border.color: "#ffffff"
                        border.width: 2
                        visible: isProcessing

                        Rectangle {
                            anchors.top: parent.top
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: 6
                            height: 6
                            radius: 3
                            color: "#ffffff"
                        }

                        RotationAnimation {
                            target: loadingSpinner
                            property: "rotation"
                            from: 0
                            to: 360
                            duration: 1000
                            loops: Animation.Infinite
                            running: isProcessing
                        }
                    }
                }

                MouseArea {
                    id: sendMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: parent.enabled ? Qt.PointingHandCursor : Qt.ForbiddenCursor
                    onClicked: {
                        if (parent.enabled && !isProcessing) {
                            sendMessage()
                        }
                    }
                }

                ToolTip {
                    visible: sendMouseArea.containsMouse && parent.enabled
                    text: "Send filter request"
                    delay: 500
                }
            }
        }
    }

    // Connect to controller's bot response signal
    Connections {
        target: controller
        function onBotResponseReceived(response) {
            addBotResponse(response, false)  // Most responses are not filter responses
        }

        // REMOVED DUPLICATE: This was causing the duplicate "Filter applied!" message
        // The message is already handled in the controller and sent via onBotResponseReceived
        // function onSchedulesFiltered(filteredCount, totalCount) {
        //     if (filteredCount !== totalCount) {
        //         addBotResponse(`✅ Filter applied! Showing ${filteredCount} of ${totalCount} schedules that match your criteria.`, true)
        //     }
        // }
    }

    function sendMessage() {
        var messageText = inputField.text.trim()
        if (messageText.length === 0 || isProcessing) return

        // Add user message immediately
        messagesModel.append({
            "isBot": false,
            "message": messageText,
            "timestamp": new Date().toLocaleTimeString(Qt.locale(), "hh:mm"),
            "isTyping": false,
            "isFilterResponse": false
        })

        // Clear input field
        inputField.text = ""

        // Set processing state
        isProcessing = true

        // Start animated loading messages
        currentLoadingMessageIndex = 0
        showAnimatedTypingIndicator()

        // Send message to model through controller
        if (controller) {
            controller.processBotMessage(messageText)
        } else {
            // Fallback if controller is not available
            hideTypingIndicator()
            addBotResponse("I'm sorry, but I'm unable to process your request right now. Please try again later.", false)
            isProcessing = false
        }
    }

    function addBotResponse(responseText, isFilterResponse) {
        hideTypingIndicator()
        messagesModel.append({
            "isBot": true,
            "message": responseText,
            "timestamp": new Date().toLocaleTimeString(Qt.locale(), "hh:mm"),
            "isTyping": false,
            "isFilterResponse": isFilterResponse || false
        })
        isProcessing = false
    }

    function showAnimatedTypingIndicator() {
        messagesModel.append({
            "isBot": true,
            "message": loadingMessages[currentLoadingMessageIndex],
            "timestamp": new Date().toLocaleTimeString(Qt.locale(), "hh:mm"),
            "isTyping": true,
            "isFilterResponse": false
        })

        loadingMessageTimer.start()
    }

    function updateLoadingMessage() {
        // Find and update the typing indicator message
        for (var i = messagesModel.count - 1; i >= 0; i--) {
            var item = messagesModel.get(i)
            if (item.isTyping) {
                messagesModel.setProperty(i, "message", loadingMessages[currentLoadingMessageIndex])
                break
            }
        }
    }

    function hideTypingIndicator() {
        loadingMessageTimer.stop()

        for (var i = messagesModel.count - 1; i >= 0; i--) {
            var item = messagesModel.get(i)
            if (item.isTyping) {
                messagesModel.remove(i)
                break
            }
        }
    }
}