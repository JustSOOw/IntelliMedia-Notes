import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt5Compat.GraphicalEffects

// 自定义对话框组件
Popup {
    id: root
    
    // 全局字体属性
    property string globalFontFamily: parent ? (parent.globalFontFamily || "Arial") : "Arial"
    
    // 可配置属性
    property string title: qsTr("标题")
    property string message: qsTr("消息内容")
    property string placeholder: ""  // 输入框占位符
    property string inputText: ""    // 输入框内容
    property bool showInput: false   // 是否显示输入框
    property string confirmText: qsTr("确定")
    property string cancelText: qsTr("取消")
    property bool isWarning: false   // 是否为警告对话框
    property bool cancelVisible: true // 是否显示取消按钮
    
    // 主题相关颜色
    property color dialogBgColor: sidebarManager.isDarkTheme ? "#353535" : "#ffffff"
    property color titleBarColor: sidebarManager.isDarkTheme ? 
        (isWarning ? "#503030" : "#304050") : 
        (isWarning ? "#FFE0E0" : "#F0F7FF")
    property color titleTextColor: isWarning ? 
        (sidebarManager.isDarkTheme ? "#ff7070" : "#FF4040") :
        (sidebarManager.isDarkTheme ? "#90caf9" : "#4285F4")
    property color messageTextColor: sidebarManager.isDarkTheme ? "#e0e0e0" : "#333333"
    property color inputBgColor: sidebarManager.isDarkTheme ? "#454545" : "#f5f5f5"
    property color inputBorderColor: sidebarManager.isDarkTheme ? 
        (inputField.activeFocus ? "#90caf9" : "#606060") : 
        (inputField.activeFocus ? "#4285F4" : "#e0e0e0")
    property color inputTextColor: sidebarManager.isDarkTheme ? "#e0e0e0" : "#333333"
    property color buttonBgColor: sidebarManager.isDarkTheme ? "#454545" : "#f5f5f5"
    property color buttonHoverColor: sidebarManager.isDarkTheme ? "#555555" : "#f0f0f0" 
    property color buttonPressedColor: sidebarManager.isDarkTheme ? "#656565" : "#e0e0e0"
    property color buttonBorderColor: sidebarManager.isDarkTheme ? "#606060" : "#e0e0e0"
    property color cancelTextColor: sidebarManager.isDarkTheme ? "#cccccc" : "#666666"
    property color confirmBgColor: "#4285F4" // 确认按钮保持蓝色
    property color confirmHoverColor: "#5295F5"
    property color confirmPressedColor: "#3275E5"
    property color confirmTextColor: "white"
    property color shadowColor: sidebarManager.isDarkTheme ? "#60000000" : "#30000000"
    
    // 回调信号
    signal confirmed(string inputText)
    signal cancelled()
    
    // 设置基础样式
    width: 220 // 调整宽度
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
        color: dialogBgColor
        radius: 12
        
        // 阴影效果
        layer.enabled: true
        layer.effect: DropShadow {
            transparentBorder: true
            horizontalOffset: 0
            verticalOffset: 3
            radius: 12.0
            samples: 25
            color: shadowColor
        }
    }
    
    // 主布局 - 改为ColumnLayout
    contentItem: ColumnLayout { 
        id: mainContentLayout
        width: parent.width // 宽度绑定到父级
        // implicitHeight 由子元素决定

        // 标题栏
        Rectangle {
            id: titleBar
            // width: parent.width // 由Layout控制
            // height: 50 // 由Layout控制
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            radius: 12
            color: titleBarColor
            
            // 只设置顶部圆角
            Rectangle {
                width: parent.width
                height: parent.height / 2
                anchors.bottom: parent.bottom
                color: titleBarColor
            }
            
            Label {
                text: title
                font {
                    pixelSize: 16
                    bold: true
                    family: globalFontFamily
                }
                color: titleTextColor
                anchors.centerIn: parent
            }
        }
        
        // 内容区域 - 使用嵌套ColumnLayout并设置边距
        ColumnLayout {
            id: contentAreaLayout
            Layout.fillWidth: true
            // anchors 和 margins 改为 Layout margins
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            Layout.topMargin: 20 
            Layout.bottomMargin: 15
            spacing: 15
            // anchors.top: titleBar.bottom
            // anchors.left: parent.left
            // anchors.right: parent.right
            // anchors.bottom: buttonBar.top
            // anchors.margins: 20
            // spacing: 15
            
            // 消息文本
            Label {
                text: message
                font {
                    pixelSize: 14
                    family: globalFontFamily
                }
                color: messageTextColor
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
                font {
                    pixelSize: 14
                    family: globalFontFamily
                }
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                horizontalAlignment: Text.AlignLeft
                color: inputTextColor
                
                background: Rectangle {
                    radius: 6
                    color: inputBgColor
                    border.width: 1
                    border.color: inputBorderColor
                    
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
            
            /* // 移除固定占位空间
            Item {
                Layout.fillHeight: true
            }*/
        }
        
        // 按钮区域
        Rectangle {
            id: buttonBar
            // width: parent.width // 由Layout控制
            // height: 60 // 由Layout控制
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            // anchors.bottom: parent.bottom // 由Layout控制
            color: "transparent"
            
            // 按钮布局
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10
                
                // 占位空间
                Item { Layout.fillWidth: true }
                
                // 取消按钮
                Button {
                    id: cancelButton
                    text: cancelText
                    visible: cancelVisible
                    Layout.preferredWidth: Math.max(80, contentItem.implicitWidth + 20) // 自适应宽度，最小80
                    Layout.preferredHeight: 36
                    
                    contentItem: Text {
                        text: cancelButton.text
                        font {
                            pixelSize: 14
                            family: globalFontFamily
                        }
                        color: cancelTextColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    background: Rectangle {
                        radius: 6
                        color: cancelButton.pressed ? buttonPressedColor : 
                               cancelButton.hovered ? buttonHoverColor : buttonBgColor
                        border.width: 1
                        border.color: buttonBorderColor
                        
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
                    Layout.preferredWidth: Math.max(80, contentItem.implicitWidth + 20) // 自适应宽度，最小80
                    Layout.preferredHeight: 36
                    
                    contentItem: Text {
                        text: confirmButton.text
                        font {
                            pixelSize: 14
                            family: globalFontFamily
                        }
                        color: confirmTextColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    background: Rectangle {
                        radius: 6
                        color: confirmButton.pressed ? confirmPressedColor : 
                               confirmButton.hovered ? confirmHoverColor : confirmBgColor
                        
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
    
    // 监听主题变化
    Connections {
        target: sidebarManager
        function onThemeChanged() {
            // 属性会自动更新，无需额外操作
        }
    }
} 