import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt5Compat.GraphicalEffects

// 功能按钮组件
Rectangle {
    id: actionButtons
    width: parent.width
    height: 120
    color: "transparent"
    
    // 当前选中的按钮 (file, ai, search)
    property string activeButton: "file"
    
    // 信号：按钮被点击
    signal fileButtonClicked()
    signal aiButtonClicked()
    signal searchButtonClicked()
    signal createNewNoteClicked()
    
    // 动画
    Animations { id: animations }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 15
        
        // 新建按钮
        Rectangle {
            id: newButton
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            radius: 20
            color: "#4285F4"
            
            RowLayout {
                anchors.centerIn: parent
                spacing: 8
                
                Image {
                    source: "qrc:/resources/icons/sidebar/add.svg"
                    width: 18
                    height: 18
                    sourceSize.width: width
                    sourceSize.height: height
                    
                    ColorOverlay {
                        anchors.fill: parent
                        source: parent
                        color: "white"
                    }
                }
                
                Text {
                    text: "新建"
                    font.pixelSize: 14
                    font.bold: true
                    color: "white"
                }
            }
            
            // 点击时的视觉反馈
            MouseArea {
                id: newButtonMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                
                onPressed: {
                    newButton.scale = 0.95
                    newButton.opacity = 0.9
                }
                
                onReleased: {
                    newButton.scale = 1.0
                    newButton.opacity = 1.0
                    createNewNoteClicked()
                }
                
                onEntered: {
                    newButton.color = Qt.darker("#4285F4", 1.1)
                }
                
                onExited: {
                    newButton.color = "#4285F4"
                    newButton.scale = 1.0
                    newButton.opacity = 1.0
                }
            }
            
            // 添加涟漪效果
            Behavior on scale {
                NumberAnimation { duration: 100; easing.type: Easing.OutQuad }
            }
            
            Behavior on opacity {
                NumberAnimation { duration: 100; easing.type: Easing.OutQuad }
            }
            
            Behavior on color {
                ColorAnimation { duration: 150 }
            }
        }
        
        // 功能切换按钮组
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            spacing: 10
            
            // 文件按钮
            Rectangle {
                id: fileButton
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                radius: 18
                color: activeButton === "file" ? "#EBEBEB" : "transparent"
                
                RowLayout {
                    anchors.centerIn: parent
                    spacing: 5
                    
                    Image {
                        source: "qrc:/resources/icons/sidebar/file.svg"
                        width: 18
                        height: 18
                        sourceSize.width: width
                        sourceSize.height: height
                        
                        ColorOverlay {
                            anchors.fill: parent
                            source: parent
                            color: activeButton === "file" ? "#4285F4" : "#666666"
                        }
                    }
                    
                    Text {
                        text: "文件"
                        font.pixelSize: 12
                        color: activeButton === "file" ? "#4285F4" : "#666666"
                    }
                }
                
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        activeButton = "file"
                        fileButtonClicked()
                    }
                    
                    onPressed: {
                        parent.opacity = 0.8
                        parent.scale = 0.98
                    }
                    
                    onReleased: {
                        parent.opacity = 1.0
                        parent.scale = 1.0
                    }
                }
                
                Behavior on color {
                    ColorAnimation { duration: 150 }
                }
                
                Behavior on scale {
                    NumberAnimation { duration: 100; easing.type: Easing.OutQuad }
                }
                
                Behavior on opacity {
                    NumberAnimation { duration: 100; easing.type: Easing.OutQuad }
                }
            }
            
            // AI按钮
            Rectangle {
                id: aiButton
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                radius: 18
                color: activeButton === "ai" ? "#EBEBEB" : "transparent"
                
                RowLayout {
                    anchors.centerIn: parent
                    spacing: 5
                    
                    Image {
                        source: "qrc:/resources/icons/sidebar/ai.svg"
                        width: 18
                        height: 18
                        sourceSize.width: width
                        sourceSize.height: height
                        
                        ColorOverlay {
                            anchors.fill: parent
                            source: parent
                            color: activeButton === "ai" ? "#4285F4" : "#666666"
                        }
                    }
                    
                    Text {
                        text: "AI"
                        font.pixelSize: 12
                        color: activeButton === "ai" ? "#4285F4" : "#666666"
                    }
                }
                
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        activeButton = "ai"
                        aiButtonClicked()
                    }
                    
                    onPressed: {
                        parent.opacity = 0.8
                        parent.scale = 0.98
                    }
                    
                    onReleased: {
                        parent.opacity = 1.0
                        parent.scale = 1.0
                    }
                }
                
                Behavior on color {
                    ColorAnimation { duration: 150 }
                }
                
                Behavior on scale {
                    NumberAnimation { duration: 100; easing.type: Easing.OutQuad }
                }
                
                Behavior on opacity {
                    NumberAnimation { duration: 100; easing.type: Easing.OutQuad }
                }
            }
            
            // 搜索按钮
            Rectangle {
                id: searchButton
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                radius: 18
                color: activeButton === "search" ? "#EBEBEB" : "transparent"
                
                RowLayout {
                    anchors.centerIn: parent
                    spacing: 5
                    
                    Image {
                        source: "qrc:/resources/icons/sidebar/search.svg"
                        width: 18
                        height: 18
                        sourceSize.width: width
                        sourceSize.height: height
                        
                        ColorOverlay {
                            anchors.fill: parent
                            source: parent
                            color: activeButton === "search" ? "#4285F4" : "#666666"
                        }
                    }
                    
                    Text {
                        text: "搜索"
                        font.pixelSize: 12
                        color: activeButton === "search" ? "#4285F4" : "#666666"
                    }
                }
                
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        activeButton = "search"
                        searchButtonClicked()
                    }
                    
                    onPressed: {
                        parent.opacity = 0.8
                        parent.scale = 0.98
                    }
                    
                    onReleased: {
                        parent.opacity = 1.0
                        parent.scale = 1.0
                    }
                }
                
                Behavior on color {
                    ColorAnimation { duration: 150 }
                }
                
                Behavior on scale {
                    NumberAnimation { duration: 100; easing.type: Easing.OutQuad }
                }
                
                Behavior on opacity {
                    NumberAnimation { duration: 100; easing.type: Easing.OutQuad }
                }
            }
        }
    }
} 