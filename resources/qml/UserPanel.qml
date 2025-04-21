import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// 用户信息面板组件
Rectangle {
    id: userPanel
    width: parent.width
    height: 80
    color: "transparent"
    // 添加锚点，以便缩放效果正确
    transformOrigin: Item.Center
    
    property string username: "15039630768"
    property string userStatus: "在线"
    property string userAvatar: ""
    
    // 主题相关颜色属性
    property color textColor: sidebarManager.isDarkTheme ? "#e0e0e0" : "#333333"
    property color subtextColor: sidebarManager.isDarkTheme ? "#aaaaaa" : "#888888"
    property color avatarBgColor: sidebarManager.isDarkTheme ? "#454545" : "#f0f0f0"
    property color avatarBorderColor: sidebarManager.isDarkTheme ? "#555555" : "#e0e0e0"
    
    // 添加悬停效果的 MouseArea
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor // 可选：改变光标形状

        onHoveredChanged: {
            if (containsMouse) {
                avatarContainer.scale = 1.05 // 悬停时放大头像
            } else {
                avatarContainer.scale = 1.0 // 鼠标移开时恢复
            }
        }
    }
    
    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10
        
        // 用户头像
        Rectangle {
            id: avatarContainer
            width: 50
            height: 50
            radius: width / 2
            color: avatarBgColor
            Layout.alignment: Qt.AlignVCenter
            border.width: 1
            border.color: avatarBorderColor
            // 添加锚点和平滑过渡
            transformOrigin: Item.Center
            Behavior on scale { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }
            
            Image {
                id: avatarImage
                anchors.fill: parent
                anchors.margins: 2
                source: userAvatar !== "" ? userAvatar : "../icons/sidebar/user.svg"
                fillMode: Image.PreserveAspectFit
                sourceSize.width: width
                sourceSize.height: height
                visible: status === Image.Ready
            }
            
            Rectangle {
                id: statusIndicator
                width: 12
                height: 12
                radius: width / 2
                color: "#4CAF50" // 在线状态为绿色
                border.width: 2
                border.color: avatarContainer.color // 使用头像背景色作为边框色
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                // 添加锚点以支持缩放
                transformOrigin: Item.Center

                // 呼吸动画
                SequentialAnimation {
                    running: true
                    loops: Animation.Infinite
                    PropertyAnimation { target: statusIndicator; property: "scale"; to: 1.15; duration: 800; easing.type: Easing.InOutQuad }
                    PropertyAnimation { target: statusIndicator; property: "scale"; to: 1.0; duration: 800; easing.type: Easing.InOutQuad }
                    PauseAnimation { duration: 400 } // 暂停一下
                }
            }
        }
        
        // 用户信息
        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 2
            
            Text {
                text: username
                font.pixelSize: 16
                font.bold: true
                color: textColor
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            
            Text {
                text: userStatus
                font.pixelSize: 12
                color: subtextColor
                Layout.fillWidth: true
            }
        }
    }
} 