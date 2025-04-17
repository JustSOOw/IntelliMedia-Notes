import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// 用户信息面板组件
Rectangle {
    id: userPanel
    width: parent.width
    height: 80
    color: "transparent"
    
    property string username: "15039630768"
    property string userStatus: "在线"
    property string userAvatar: ""
    
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
            color: "#f0f0f0"
            Layout.alignment: Qt.AlignVCenter
            border.width: 1
            border.color: "#e0e0e0"
            
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
                border.color: avatarContainer.color
                anchors.right: parent.right
                anchors.bottom: parent.bottom
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
                color: "#333333"
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            
            Text {
                text: userStatus
                font.pixelSize: 12
                color: "#888888"
                Layout.fillWidth: true
            }
        }
        
        // 展开箭头
        Image {
            source: "qrc:/resources/icons/round_right_fill.svg"
            width: 16
            height: 16
            Layout.alignment: Qt.AlignVCenter
            
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    // 用户详情展开功能 (暂未实现)
                    console.log("展开用户详情")
                }
            }
        }
    }
} 