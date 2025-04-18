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
            
            // 添加渐变背景
            gradient: Gradient {
                id: newButtonGradient
                orientation: Gradient.Horizontal
                GradientStop { id: gradientStop1; position: 0.0; color: "#8ccf0f" } // 左侧浅绿
                GradientStop { id: gradientStop2; position: 1.0; color: "#4285F4" } // 右侧原蓝
            }
            
            // 添加阴影效果
            layer.enabled: true
            layer.effect: DropShadow {
                id: newButtonShadow
                anchors.fill: newButton
                horizontalOffset: 0
                verticalOffset: 2
                radius: 6.0
                samples: 16
                color: "#30000000" // 半透明黑色阴影
            }
            
            // 按钮内容 (图标和文字)
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
            
            // 鼠标交互区域
            MouseArea {
                id: newButtonMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                
                // 点击信号
                onClicked: createNewNoteClicked()
            }
            
            // 定义状态
            states: [
                State {
                    name: "HOVERED"
                    when: newButtonMouseArea.containsMouse && !newButtonMouseArea.pressed
                    // 悬停状态下的属性变化
                    PropertyChanges { target: newButtonGradient; stop1.color: Qt.lighter("#5c9dff", 1.1); stop2.color: Qt.lighter("#4285F4", 1.1) }
                    PropertyChanges { target: newButton; scale: 1.03 }
                    PropertyChanges { target: newButtonShadow; color: "#40000000"; radius: 8.0 }
                },
                State {
                    name: "PRESSED"
                    when: newButtonMouseArea.pressed
                    // 按下状态下的属性变化
                    PropertyChanges { target: newButtonGradient; stop1.color: Qt.darker("#5c9dff", 1.1); stop2.color: Qt.darker("#4285F4", 1.1) }
                    PropertyChanges { target: newButton; scale: 0.95 }
                    PropertyChanges { target: newButtonShadow; color: "#50000000"; verticalOffset: 1; radius: 4.0 }
                }
            ]

            // 定义状态之间的过渡动画
            transitions: [
                Transition { // 应用于所有状态变化
                    // 渐变颜色动画
                    ColorAnimation { target: newButtonGradient.stop1; property: "color"; duration: 150 }
                    ColorAnimation { target: newButtonGradient.stop2; property: "color"; duration: 150 }
                    // 缩放动画
                    NumberAnimation { properties: "scale"; duration: 120; easing.type: Easing.OutQuad }
                    // 阴影属性动画
                    NumberAnimation { target: newButtonShadow; properties: "radius, verticalOffset"; duration: 150; easing.type: Easing.OutQuad }
                    ColorAnimation { target: newButtonShadow; property: "color"; duration: 150 }
                }
            ]
        }
        
        // 功能切换按钮组
        RowLayout {
            id: buttonRowLayout
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            spacing: 10
            
            // 流动的背景指示器 (放在按钮之前，或使用 z 属性)
            Rectangle {
                id: activeIndicator
                y: fileButton.y // Restore binding
                height: fileButton.height // Restore binding
                radius: 18
                color: "#EBEBEB" // Restore original background color
                // z: 2 // Remove test Z value
                // opacity: 1 // Remove test opacity
                
                // Restore fixed geometry for testing
                // x: 10 
                // y: 10 
                // width: 60
                // height: 30
                
                // Restore bindings and behaviors
                x: {
                    if (activeButton === "file") return fileButton.x;
                    else if (activeButton === "ai") return aiButton.x;
                    else if (activeButton === "search") return searchButton.x;
                    else return fileButton.x;
                }
                width: {
                    if (activeButton === "file") return fileButton.width;
                    else if (activeButton === "ai") return aiButton.width;
                    else if (activeButton === "search") return searchButton.width;
                    else return fileButton.width;
                }
                
                Behavior on x {
                    NumberAnimation {
                        duration: 350
                        easing.type: Easing.OutElastic
                        easing.amplitude: 1.5
                        easing.period: 0.6
                    }
                }
                Behavior on width {
                    NumberAnimation {
                        duration: 350
                        easing.type: Easing.OutElastic
                        easing.amplitude: 1.5
                        easing.period: 0.6
                    }
                }
                
            }
            
            // 文件按钮
            Rectangle {
                id: fileButton
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                radius: 18
                color: "transparent"
                // z: 1 // Remove Z value, indicator is drawn first
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 2

                    RowLayout {
                        anchors.horizontalCenter: parent.horizontalCenter
                        height: parent.height - 4
                        spacing: 5
                        Layout.alignment: Qt.AlignCenter
                        
                        Image {
                            source: "qrc:/icons/sidebar/file.svg"
                            width: 18; height: 18; sourceSize.width: width; sourceSize.height: height
                            ColorOverlay { anchors.fill: parent; source: parent; color: activeButton === "file" ? "#4285F4" : "#666666" }
                        }
                        Text {
                            text: "文件"
                            font.pixelSize: 12
                            color: activeButton === "file" ? "#4285F4" : "#666666"
                        }
                    }
                }

                MouseArea {
                    id: fileClickArea
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { activeButton = "file"; fileButtonClicked(); }
                    onPressed: { fileButton.opacity = 0.8; fileButton.scale = 0.98; }
                    onReleased: { fileButton.opacity = 1.0; fileButton.scale = 1.0; }
                }
            }
            
            // AI按钮
            Rectangle {
                id: aiButton
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                radius: 18
                color: "transparent"
                // z: 1 // Remove Z value

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 2
                    RowLayout {
                        anchors.horizontalCenter: parent.horizontalCenter
                        height: parent.height - 4
                        spacing: 5
                        Layout.alignment: Qt.AlignCenter
                        
                        Image {
                            source: "qrc:/icons/sidebar/ai.svg"
                            width: 18; height: 18; sourceSize.width: width; sourceSize.height: height
                            ColorOverlay { anchors.fill: parent; source: parent; color: activeButton === "ai" ? "#4285F4" : "#666666" }
                        }
                        Text {
                            text: "AI"
                            font.pixelSize: 12
                            color: activeButton === "ai" ? "#4285F4" : "#666666"
                        }
                    }
                }
                MouseArea {
                    id: aiClickArea
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { activeButton = "ai"; aiButtonClicked(); }
                    onPressed: { aiButton.opacity = 0.8; aiButton.scale = 0.98; }
                    onReleased: { aiButton.opacity = 1.0; aiButton.scale = 1.0; }
                }
            }
            
            // 搜索按钮
            Rectangle {
                id: searchButton
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                radius: 18
                color: "transparent"
                // z: 1 // Remove Z value

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 2
                    RowLayout {
                        anchors.horizontalCenter: parent.horizontalCenter
                        height: parent.height - 4
                        spacing: 5
                        Layout.alignment: Qt.AlignCenter
                        
                        Image {
                            source: "qrc:/icons/sidebar/search.svg"
                            width: 18; height: 18; sourceSize.width: width; sourceSize.height: height
                            ColorOverlay { anchors.fill: parent; source: parent; color: activeButton === "search" ? "#4285F4" : "#666666" }
                        }
                        Text {
                            text: "搜索"
                            font.pixelSize: 12
                            color: activeButton === "search" ? "#4285F4" : "#666666"
                        }
                    }
                }
                MouseArea {
                    id: searchClickArea
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { activeButton = "search"; searchButtonClicked(); }
                    onPressed: { searchButton.opacity = 0.8; searchButton.scale = 0.98; }
                    onReleased: { searchButton.opacity = 1.0; searchButton.scale = 1.0; }
                }
            }
        }
    }
} 