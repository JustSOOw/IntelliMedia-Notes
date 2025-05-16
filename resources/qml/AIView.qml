import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt5Compat.GraphicalEffects

// AI聊天视图组件
Rectangle {
    id: aiViewRect
    width: parent.width
    color: "transparent"
    
    // 添加全局字体属性
    property string globalFontFamily: parent.globalFontFamily || "Arial"
    
    // 主题相关颜色
    property color titleTextColor: sidebarManager.isDarkTheme ? "#e0e0e0" : "#333333"
    property color userMessageBgColor: sidebarManager.isDarkTheme ? "#3a5a8c" : "#e3f2fd"
    property color aiMessageBgColor: sidebarManager.isDarkTheme ? "#454545" : "#f5f5f5"
    property color messageTextColor: sidebarManager.isDarkTheme ? "#e0e0e0" : "#333333"
    property color userLabelColor: sidebarManager.isDarkTheme ? "#90caf9" : "#1976D2"
    property color aiLabelColor: sidebarManager.isDarkTheme ? "#a5d6a7" : "#4CAF50"
    property color timestampColor: sidebarManager.isDarkTheme ? "#aaaaaa" : "#888888"
    property color inputBgColor: sidebarManager.isDarkTheme ? "#505050" : "white"
    property color inputBorderColor: sidebarManager.isDarkTheme ? "#606060" : "#e0e0e0"
    property color inputTextColor: sidebarManager.isDarkTheme ? "#e0e0e0" : "black"
    property color inputPlaceholderColor: sidebarManager.isDarkTheme ? "#808080" : "#aaa"
    
    // 信号
    signal sendMessage(string message)
    
    // 标题部分
    Rectangle {
        id: aiHeader
        width: parent.width
        height: 40
        color: "transparent"
        anchors.top: parent.top
        
        Text {
            text: "AI 助手"
            font {
                pixelSize: 16
                bold: true
                family: globalFontFamily
            }
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 15
            color: titleTextColor
        }
    }
    
    // 聊天历史记录
    ListView {
        id: chatHistory
        anchors.top: aiHeader.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: inputArea.top
        anchors.margins: 10
        clip: true
        spacing: 12
        
        // 使用C++模型提供数据
        model: ListModel {
            // 示例数据
            ListElement { role: "user"; message: "你能帮我整理一下笔记吗？"; timestamp: "10:30" }
            ListElement { role: "ai"; message: "当然可以。你想要我怎样帮你整理笔记？我可以提供分类建议、创建摘要或者重组内容结构。"; timestamp: "10:31" }
        }
        
        delegate: Rectangle {
            id: messageItem
            width: ListView.view.width
            implicitHeight: messageLayout.height + 20 // 根据内容调整高度
            color: model.role === "user" ? userMessageBgColor : aiMessageBgColor
            radius: 8
            anchors.right: model.role === "user" ? parent.right : undefined
            
            // 消息布局
            ColumnLayout {
                id: messageLayout
                width: parent.width - 20
                anchors.centerIn: parent
                spacing: 5
                
                // 角色标签
                Text {
                    text: model.role === "user" ? "你" : "AI 助手"
                    font {
                        pixelSize: 12
                        bold: true
                        family: globalFontFamily
                    }
                    color: model.role === "user" ? userLabelColor : aiLabelColor
                    Layout.alignment: model.role === "user" ? Qt.AlignRight : Qt.AlignLeft
                }
                
                // 消息内容
                Text {
                    text: model.message
                    font {
                        pixelSize: 14
                        family: globalFontFamily
                    }
                    wrapMode: Text.Wrap
                    color: messageTextColor
                    Layout.fillWidth: true
                    Layout.alignment: model.role === "user" ? Qt.AlignRight : Qt.AlignLeft
                }
                
                // 时间戳
                Text {
                    text: model.timestamp
                    font {
                        pixelSize: 10
                        family: globalFontFamily
                    }
                    color: timestampColor
                    Layout.alignment: model.role === "user" ? Qt.AlignRight : Qt.AlignLeft
                }
            }
        }
        
        // 滚动条
        ScrollBar.vertical: ScrollBar {
            active: true
            policy: ScrollBar.AsNeeded
        }
    }
    
    // 输入区域
    Rectangle {
        id: inputArea
        width: parent.width
        height: 60
        color: "transparent"
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        
        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10
            
            // 输入框
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                border.width: 1
                border.color: inputBorderColor
                radius: 20
                color: inputBgColor
                
                TextInput {
                    id: messageInput
                    anchors.fill: parent
                    anchors.leftMargin: 15
                    anchors.rightMargin: 15
                    verticalAlignment: TextInput.AlignVCenter
                    font {
                        pixelSize: 14
                        family: globalFontFamily
                    }
                    clip: true
                    selectByMouse: true
                    color: inputTextColor
                    
                    property string placeholderText: "输入消息..."
                    
                    Text {
                        anchors.fill: parent
                        verticalAlignment: TextInput.AlignVCenter
                        font {
                            pixelSize: 14
                            family: globalFontFamily
                        }
                        text: messageInput.placeholderText
                        color: inputPlaceholderColor
                        visible: !messageInput.text && !messageInput.activeFocus
                    }
                    
                    // 回车发送消息
                    Keys.onReturnPressed: {
                        if (messageInput.text.trim() !== "") {
                            sendMessage(messageInput.text.trim())
                            messageInput.text = ""
                        }
                    }
                }
            }
            
            // 发送按钮
            Rectangle {
                id: sendButton
                width: 40
                height: 40
                radius: 20
                color: "#4285F4"
                
                Image {
                    id: sendIcon
                    width: 20
                    height: 20
                    source: "qrc:/icons/round_right_fill.svg"
                    fillMode: Image.PreserveAspectFit
                    
                    ColorOverlay {
                        anchors.fill: parent
                        source: parent
                        color: "white"
                    }
                    
                    rotation: 90 // 向上箭头
                }
                
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    
                    onPressed: {
                        sendButton.opacity = 0.8
                        sendButton.scale = 0.95
                    }
                    
                    onReleased: {
                        sendButton.opacity = 1.0
                        sendButton.scale = 1.0
                        
                        if (messageInput.text.trim() !== "") {
                            sendMessage(messageInput.text.trim())
                            messageInput.text = ""
                        }
                    }
                }
                
                Behavior on opacity {
                    NumberAnimation { duration: 100 }
                }
                
                Behavior on scale {
                    NumberAnimation { duration: 100; easing.type: Easing.OutQuad }
                }
            }
        }
    }
} 