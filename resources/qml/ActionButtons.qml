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
    
    // --- 移动属性、函数、信号处理器到根作用域 --- 
    property real targetX: _calculateTargetX() // 存储目标X
    property real targetWidth: _calculateTargetWidth() // 存储目标宽度
    
    function _calculateTargetX() {
        // 确保按钮已初始化
        if (!fileButton || !aiButton || !searchButton) return 0; 
        if (activeButton === "file") return fileButton.x;
        else if (activeButton === "ai") return aiButton.x;
        else if (activeButton === "search") return searchButton.x;
        else return fileButton.x; // 默认
    }
    function _calculateTargetWidth() {
        // 确保按钮已初始化
        if (!fileButton || !aiButton || !searchButton) return 0;
        if (activeButton === "file") return fileButton.width;
        else if (activeButton === "ai") return aiButton.width;
        else if (activeButton === "search") return searchButton.width;
        else return fileButton.width; // 默认
    }
    
    // 观察 activeButton 变化，更新目标值并触发动画
    onActiveButtonChanged: {
        targetX = _calculateTargetX();
        targetWidth = _calculateTargetWidth();
        var targetY = fileButton ? fileButton.y : -1; // Also check Y
        var targetHeight = fileButton ? fileButton.height : -1; // Also check Height
        if (activeIndicator && targetWidth > 0 && targetHeight > 0) {
             activeIndicator.x = targetX;
             activeIndicator.width = targetWidth;
             activeIndicator.y = targetY; // Ensure Y/Height are also updated if needed
             activeIndicator.height = targetHeight;
            // 强制重新评估按钮文本/图标颜色
            _updateButtonColors();
        }
    }
    // ---------------------------------------------
    
    // --- 添加一个函数来更新所有按钮的颜色 ---
    function _updateButtonColors() {
        if (fileText) fileText.color = (activeButton === "file" ? "#f2f4f7" : (fileClickArea.containsMouse ? "#4285F4" : "#666666"));
        if (fileIconOverlay) fileIconOverlay.color = (activeButton === "file" ? "#ffffff" : (fileClickArea.containsMouse ? "#4285F4" : "#666666"));

        if (aiText) aiText.color = (activeButton === "ai" ? "#f2f4f7" : (aiClickArea.containsMouse ? "#4285F4" : "#666666"));
        if (aiIconOverlay) aiIconOverlay.color = (activeButton === "ai" ? "#ffffff" : (aiClickArea.containsMouse ? "#4285F4" : "#666666"));

        if (searchText) searchText.color = (activeButton === "search" ? "#f2f4f7" : (searchClickArea.containsMouse ? "#4285F4" : "#666666"));
        if (searchIconOverlay) searchIconOverlay.color = (activeButton === "search" ? "#ffffff" : (searchClickArea.containsMouse ? "#4285F4" : "#666666"));
    }
    // --------------------------------------------
    
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
                    source: "qrc:/icons/sidebar/add.svg"
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
                    PropertyChanges { target: gradientStop1; color: Qt.lighter("#8ccf0f", 1.1) }
                    PropertyChanges { target: gradientStop2; color: Qt.lighter("#4285F4", 1.1) }
                    PropertyChanges { target: newButton; scale: 1.03 }
                    PropertyChanges { target: newButtonShadow; color: "#40000000"; radius: 8.0 }
                },
                State {
                    name: "PRESSED"
                    when: newButtonMouseArea.pressed
                    // 按下状态下的属性变化
                    PropertyChanges { target: gradientStop1; color: Qt.darker("#8ccf0f", 1.1) }
                    PropertyChanges { target: gradientStop2; color: Qt.darker("#4285F4", 1.1) }
                    PropertyChanges { target: newButton; scale: 0.95 }
                    PropertyChanges { target: newButtonShadow; color: "#50000000"; verticalOffset: 1; radius: 4.0 }
                }
            ]

            // 定义状态之间的过渡动画
            transitions: [
                Transition { // 应用于所有状态变化
                    // 渐变颜色动画
                    ColorAnimation { target: gradientStop1; property: "color"; duration: 150 }
                    ColorAnimation { target: gradientStop2; property: "color"; duration: 150 }
                    // 缩放动画
                    NumberAnimation { target: newButton; properties: "scale"; duration: 120; easing.type: Easing.OutQuad }
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
            spacing: 0 // 减小间距以尝试增大按钮宽度
            
            // --- 流动的背景指示器 (定义在按钮之前，使其位于下方) ---
            Rectangle {
                id: activeIndicator
                // 初始绑定于 fileButton，Timer 会更新
                y: fileButton ? fileButton.y : 0
                height: fileButton ? fileButton.height : 0
                radius: 18
                visible: width > 0 && height > 0
                opacity: 1.0
                x: fileButton ? fileButton.x : 0      // 初始与文件按钮同位置
                width: fileButton ? fileButton.width : 0 // 初始与文件按钮同宽

                // 添加渐变背景 (与指示器协调)
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: "#8ccf0f" } //与新建按钮的渐变颜色一致
                    GradientStop { position: 1.0; color: "#4285F4" } 
                }

                // --- 优化动画 Behavior ---
                Behavior on x {
                    NumberAnimation {
                        duration: 200 // 调整动画时长
                        easing.type: Easing.OutQuad // 使用更平滑的曲线
                    }
                }
                Behavior on width {
                    NumberAnimation {
                        duration: 200 // 调整动画时长
                        easing.type: Easing.OutQuad // 使用更平滑的曲线
                    }
                }
                
                // --- Timer 逻辑 (移除详细日志) ---
                Timer {
                    id: layoutTimer
                    interval: 20
                    repeat: false
                    onTriggered: {
                        var calcX = actionButtons._calculateTargetX();
                        var calcWidth = actionButtons._calculateTargetWidth();
                        var calcY = fileButton ? fileButton.y : 0;
                        var calcHeight = fileButton ? fileButton.height : 0;
                        if (calcWidth > 0 && calcHeight > 0) {
                            activeIndicator.x = calcX;
                            activeIndicator.width = calcWidth;
                            activeIndicator.y = calcY;
                            activeIndicator.height = calcHeight;
                        }
                    }
                }

                Component.onCompleted: {
                    layoutTimer.start();
                    // 初始化按钮颜色
                    _updateButtonColors();
                }
            }
            
            // --- 文件按钮 (移除 z 属性) ---
            Rectangle {
                id: fileButton
                Layout.fillWidth: true // 参与布局
                Layout.preferredHeight: 36
                radius: 18
                color: "transparent"
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 2

                    RowLayout {
                        anchors.horizontalCenter: parent.horizontalCenter
                        height: parent.height - 4
                        spacing: 5
                        Layout.alignment: Qt.AlignCenter
                        
                        Image {
                            id: fileIcon // 添加 id
                            source: "qrc:/icons/sidebar/file.svg"
                            width: 18; height: 18; sourceSize.width: width; sourceSize.height: height
                            // ColorOverlay 用于控制图标颜色
                            ColorOverlay {
                                id: fileIconOverlay // 添加 id
                                anchors.fill: parent;
                                source: parent;
                                // 颜色由 _updateButtonColors 控制
                                color: "#666666" // 初始默认灰色
                            }
                        }
                        Text {
                            id: fileText // 添加 id
                            text: "文件"
                            font.pixelSize: 12
                            // 颜色由 _updateButtonColors 控制
                            color: "#666666" // 初始默认灰色
                        }
                    }
                }

                MouseArea {
                    id: fileClickArea
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true // 启用悬停检测
                    onClicked: {
                        activeButton = "file";
                        fileButtonClicked();
                        // 点击后也更新颜色状态
                        _updateButtonColors();
                    }

                    // 添加悬停处理
                    onHoveredChanged: _updateButtonColors()
                }
                 // 添加按下缩放效果
                scale: fileClickArea.pressed ? 0.95 : 1.0
                Behavior on scale { NumberAnimation { duration: 100 } }
            }
            
            // --- AI按钮 (移除 z 属性) ---
            Rectangle {
                id: aiButton
                Layout.fillWidth: true // 参与布局
                Layout.preferredHeight: 36
                radius: 18
                color: "transparent" 
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 2
                    RowLayout {
                        anchors.horizontalCenter: parent.horizontalCenter
                        height: parent.height - 4
                        spacing: 5
                        Layout.alignment: Qt.AlignCenter
                        
                        Image {
                            id: aiIcon // 添加 id
                            source: "qrc:/icons/sidebar/ai.svg"
                            width: 18; height: 18; sourceSize.width: width; sourceSize.height: height
                            ColorOverlay {
                                id: aiIconOverlay // 添加 id
                                anchors.fill: parent;
                                source: parent;
                                color: "#666666" // 初始默认灰色
                            }
                        }
                        Text {
                            id: aiText // 添加 id
                            text: "AI"
                            font.pixelSize: 12
                            color: "#666666" // 初始默认灰色
                        }
                    }
                }
                MouseArea {
                    id: aiClickArea
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true
                    onClicked: {
                        activeButton = "ai";
                        aiButtonClicked();
                        _updateButtonColors();
                    }

                    // 添加悬停处理
                    onHoveredChanged: _updateButtonColors()
                }
                // 添加按下缩放效果
                scale: aiClickArea.pressed ? 0.95 : 1.0
                Behavior on scale { NumberAnimation { duration: 100 } }
            }
            
            // --- 搜索按钮 (移除 z 属性) ---
            Rectangle {
                id: searchButton
                Layout.fillWidth: true // 参与布局
                Layout.preferredHeight: 36
                radius: 18
                color: "transparent"

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 2
                    RowLayout {
                        anchors.horizontalCenter: parent.horizontalCenter
                        height: parent.height - 4
                        spacing: 5
                        Layout.alignment: Qt.AlignCenter
                        
                        Image {
                            id: searchIcon // 添加 id
                            source: "qrc:/icons/sidebar/search.svg"
                            width: 18; height: 18; sourceSize.width: width; sourceSize.height: height
                            ColorOverlay {
                                id: searchIconOverlay // 添加 id
                                anchors.fill: parent;
                                source: parent;
                                color: "#666666" // 初始默认灰色
                            }
                        }
                        Text {
                            id: searchText // 添加 id
                            text: "搜索"
                            font.pixelSize: 12
                            color: "#666666" // 初始默认灰色
                        }
                    }
                }
                MouseArea {
                    id: searchClickArea
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true
                    onClicked: {
                        activeButton = "search";
                        searchButtonClicked();
                        _updateButtonColors();
                        // 实际的搜索触发逻辑应在此处或信号处理器中
                        console.log("Search button clicked, state updated");
                    }

                    // 添加悬停处理
                    onHoveredChanged: _updateButtonColors()
                }
                 // 添加按下缩放效果
                scale: searchClickArea.pressed ? 0.95 : 1.0
                Behavior on scale { NumberAnimation { duration: 100 } }
            }
        }
    }
} 