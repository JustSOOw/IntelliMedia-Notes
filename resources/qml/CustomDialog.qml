import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt5Compat.GraphicalEffects

// 自定义对话框组件
Popup {
    id: root
    
    // 可配置属性
    property string title: "标题"
    property string message: "消息内容"
    property string placeholder: ""  // 输入框占位符
    property string inputText: ""    // 输入框内容
    property bool showInput: false   // 是否显示输入框
    property string confirmText: "确定"
    property string cancelText: "取消"
    property bool isWarning: false   // 是否为警告对话框
    property bool cancelVisible: true // 是否显示取消按钮
    
    // 回调信号
    signal confirmed(string inputText)
    signal cancelled()
    
    // 设置基础样式
    width: 220
    height: showInput ? 190 : 160
    padding: 0
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: parent
    
    // 动画效果
    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 150 }
        NumberAnimation { property: "scale"; from: 0.8; to: 1.0; duration: 200; easing.type: Easing.OutBack }
    }
    
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 150 }
        NumberAnimation { property: "scale"; from: 1.0; to: 0.95; duration: 150 }
    }
    
    // 背景样式
    background: Rectangle {
        color: "#ffffff"
        radius: 12
        
        // 阴影效果
        layer.enabled: true
        layer.effect: DropShadow {
            transparentBorder: true
            horizontalOffset: 0
            verticalOffset: 3
            radius: 12.0
            samples: 25
            color: "#30000000"
        }
    }
    
    // 主布局
    contentItem: Item {
        anchors.fill: parent
        
        // 标题栏
        Rectangle {
            id: titleBar
            width: parent.width
            height: 50
            radius: 12
            color: isWarning ? "#FFE0E0" : "#F0F7FF"
            
            // 只设置顶部圆角
            Rectangle {
                width: parent.width
                height: parent.height / 2
                anchors.bottom: parent.bottom
                color: isWarning ? "#FFE0E0" : "#F0F7FF"
            }
            
            Label {
                text: title
                font.pixelSize: 16
                font.bold: true
                color: isWarning ? "#FF4040" : "#4285F4"
                anchors.centerIn: parent
            }
        }
        
        // 内容区域
        ColumnLayout {
            anchors.top: titleBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: buttonBar.top
            anchors.margins: 20
            spacing: 15
            
            // 消息文本
            Label {
                text: message
                font.pixelSize: 14
                color: "#333333"
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }
            
            // 输入框 (可选显示)
            TextField {
                id: inputField
                visible: showInput
                placeholderText: placeholder
                text: root.inputText
                selectByMouse: true
                font.pixelSize: 14
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                horizontalAlignment: Text.AlignLeft
                
                background: Rectangle {
                    radius: 6
                    color: "#f5f5f5"
                    border.width: 1
                    border.color: inputField.activeFocus ? "#4285F4" : "#e0e0e0"
                    
                    // 焦点动画
                    Behavior on border.color {
                        ColorAnimation { duration: 150 }
                    }
                }
                
                // 更新绑定属性
                onTextChanged: {
                    root.inputText = text
                }
                
                // 回车键触发确认
                Keys.onReturnPressed: {
                    confirmButton.clicked()
                }
            }
            
            // 占位空间
            Item {
                Layout.fillHeight: true
            }
        }
        
        // 按钮区域
        Rectangle {
            id: buttonBar
            width: parent.width
            height: 60
            anchors.bottom: parent.bottom
            color: "transparent"
            
            // 按钮布局
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10
                
                // 占位空间
                Item {
                    Layout.fillWidth: true
                }
                
                // 取消按钮
                Button {
                    id: cancelButton
                    text: cancelText
                    visible: cancelVisible
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 36
                    
                    contentItem: Text {
                        text: cancelButton.text
                        font.pixelSize: 14
                        color: "#666666"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    background: Rectangle {
                        radius: 6
                        color: cancelButton.pressed ? "#e0e0e0" : 
                               cancelButton.hovered ? "#f0f0f0" : "#f5f5f5"
                        border.width: 1
                        border.color: "#e0e0e0"
                        
                        // 悬停效果
                        Behavior on color {
                            ColorAnimation { duration: 100 }
                        }
                    }
                    
                    onClicked: {
                        root.cancelled()
                        root.close()
                    }
                }
                
                // 确认按钮
                Button {
                    id: confirmButton
                    text: confirmText
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 36
                    
                    contentItem: Text {
                        text: confirmButton.text
                        font.pixelSize: 14
                        font.bold: true
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    background: Rectangle {
                        radius: 6
                        color: confirmButton.pressed ? (isWarning ? "#D32F2F" : "#1A73E8") : 
                               confirmButton.hovered ? (isWarning ? "#F44336" : "#4285F4") : 
                               isWarning ? "#FF5252" : "#4285F4"
                        
                        // 悬停效果
                        Behavior on color {
                            ColorAnimation { duration: 100 }
                        }
                    }
                    
                    onClicked: {
                        if (showInput) {
                            root.confirmed(inputField.text)
                        } else {
                            root.confirmed("")
                        }
                        root.close()
                    }
                }
            }
        }
    }
    
    // 自动聚焦输入框
    onOpened: {
        console.log("[CustomDialog] Opened. Title:", title, "Visible:", visible)
        if (showInput && inputField.visible) {
            inputField.forceActiveFocus()
            inputField.selectAll()
        }
    }
} 