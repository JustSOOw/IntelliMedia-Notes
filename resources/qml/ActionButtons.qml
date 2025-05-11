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
    
    // 主题相关颜色属性
    property color activeIndicatorColor: "#4285F4" // 选中指示器颜色保持不变
    property color activeTextColor: "#f2f4f7" // 选中文本颜色
    property color inactiveTextColor: sidebarManager.isDarkTheme ? "#b0b0b0" : "#666666" // 非选中文本颜色
    property color hoverTextColor: "#4285F4" // 悬停文本颜色
    property color buttonHoverBgColor: sidebarManager.isDarkTheme ? "rgba(255, 255, 255, 0.1)" : "rgba(0, 0, 0, 0.05)" // 按钮悬停背景
    property color activeBgColor: "#4285F4" // 活动按钮背景色
    
    // 信号：按钮被点击
    signal fileButtonClicked()
    signal aiButtonClicked()
    signal searchButtonClicked()
    signal createNewNoteClicked()
    
    // 动画
    Animations { id: animations }
    
    // --- 目标几何属性 --- 
    property real targetX: fileButton ? fileButton.x : 0
    property real targetWidth: fileButton ? fileButton.width : 0
    property real targetY: 0
    property real targetHeight: 0
    
    // --- 计算目标位置和尺寸的函数 ---
    function _calculateTargetX() {
        if (!fileButton || !aiButton || !searchButton) return 0;
        if (activeButton === "file") return fileButton.x;
        else if (activeButton === "ai") return aiButton.x;
        else if (activeButton === "search") return searchButton.x;
        else return fileButton ? fileButton.x : 0;
    }
    function _calculateTargetWidth() {
        if (!fileButton || !aiButton || !searchButton) return 0;
        if (activeButton === "file") return fileButton.width;
        else if (activeButton === "ai") return aiButton.width;
        else if (activeButton === "search") return searchButton.width;
        else return fileButton ? fileButton.width : 0;
    }
    function _calculateTargetY() {
        // 假设所有按钮的Y坐标和高度都一样，取 fileButton 的即可
        return fileButton ? fileButton.y : 0;
    }
    function _calculateTargetHeight() {
        return fileButton ? fileButton.height : 0;
    }
    
    // --- 更新指示器几何属性的函数 ---
    function _updateIndicatorGeometry() {
        var calcX = _calculateTargetX();
        var calcWidth = _calculateTargetWidth();
        if (calcWidth > 0) {
            targetX = calcX;
            targetWidth = calcWidth;
        } else {
            Qt.callLater(_updateIndicatorGeometry);
        }
    }
    
    // --- 响应 activeButton 变化 ---
    onActiveButtonChanged: {
        _updateIndicatorGeometry(); // 更新指示器位置
        _updateButtonColors();    // 更新按钮颜色
    }
    
    // --- 响应宽度变化 (处理侧边栏拉伸) ---
    onWidthChanged: {
        _updateIndicatorGeometry(); // 宽度变化时更新指示器位置
    }
    
    // --- 添加一个函数来更新所有按钮的颜色 ---
    function _updateButtonColors() {
        if (fileText) fileText.color = (activeButton === "file" ? activeTextColor : (fileClickArea.containsMouse ? hoverTextColor : inactiveTextColor));
        if (fileIconOverlay) fileIconOverlay.color = (activeButton === "file" ? "#ffffff" : (fileClickArea.containsMouse ? hoverTextColor : inactiveTextColor));

        if (aiText) aiText.color = (activeButton === "ai" ? activeTextColor : (aiClickArea.containsMouse ? hoverTextColor : inactiveTextColor));
        if (aiIconOverlay) aiIconOverlay.color = (activeButton === "ai" ? "#ffffff" : (aiClickArea.containsMouse ? hoverTextColor : inactiveTextColor));

        if (searchText) searchText.color = (activeButton === "search" ? activeTextColor : (searchClickArea.containsMouse ? hoverTextColor : inactiveTextColor));
        if (searchIconOverlay) searchIconOverlay.color = (activeButton === "search" ? "#ffffff" : (searchClickArea.containsMouse ? hoverTextColor : inactiveTextColor));
    }
    
    // 监听主题变化
    Connections {
        target: sidebarManager
        function onThemeChanged() {
            _updateButtonColors();
        }
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
                GradientStop { 
                    id: gradientStop1
                    position: 0.0
                    color: sidebarManager.isDarkTheme ? "#6abf0d" : "#8ccf0f" // 暗色主题调整为深一点的绿色
                } 
                GradientStop { 
                    id: gradientStop2
                    position: 1.0
                    color: sidebarManager.isDarkTheme ? "#3275e4" : "#4285F4" // 暗色主题调整为深一点的蓝色
                }
            }
            
            // 添加阴影效果
            layer.enabled: true
            layer.effect: DropShadow {
                id: newButtonShadow
                horizontalOffset: 0
                verticalOffset: 2
                radius: 6.0
                samples: 16
                color: sidebarManager.isDarkTheme ? "#40000000" : "#30000000" // 根据主题调整阴影
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
                    text: qsTr("新建")
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
                    PropertyChanges { target: newButtonShadow; color: sidebarManager.isDarkTheme ? "#50000000" : "#40000000"; radius: 8.0 }
                },
                State {
                    name: "PRESSED"
                    when: newButtonMouseArea.pressed
                    // 按下状态下的属性变化
                    PropertyChanges { target: gradientStop1; color: Qt.darker("#8ccf0f", 1.1) }
                    PropertyChanges { target: gradientStop2; color: Qt.darker("#4285F4", 1.1) }
                    PropertyChanges { target: newButton; scale: 0.95 }
                    PropertyChanges { target: newButtonShadow; color: sidebarManager.isDarkTheme ? "#60000000" : "#50000000"; verticalOffset: 1; radius: 4.0 }
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
                // x和width跟随target属性，y和height直接绑定fileButton
                x: targetX
                width: targetWidth
                y: fileButton ? fileButton.y : 0
                height: fileButton ? fileButton.height : 0
                radius: 18
                visible: width > 0 && height > 0
                opacity: 1.0

                // 添加渐变背景 (与指示器协调)
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { 
                        position: 0.0 
                        color: sidebarManager.isDarkTheme ? "#6abf0d" : "#8ccf0f" // 与新建按钮保持一致
                    } 
                    GradientStop { 
                        position: 1.0
                        color: sidebarManager.isDarkTheme ? "#3275e4" : "#4285F4"  // 与新建按钮保持一致
                    } 
                }

                // --- 优化动画 Behavior --- 
                // 动画将自动应用于绑定的 targetX/Width 等属性的变化
                Behavior on x { NumberAnimation { duration: 200; easing.type: Easing.OutQuad } }
                Behavior on width { NumberAnimation { duration: 200; easing.type: Easing.OutQuad } }
                Behavior on y { NumberAnimation { duration: 200; easing.type: Easing.OutQuad } }
                Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.OutQuad } }
                
                // --- 移除 Timer --- 
                // Timer {
                //     id: layoutTimer
                //     interval: 20
                //     repeat: false
                //     onTriggered: { ... }
                // }

                Component.onCompleted: {
                    // layoutTimer.start(); // 移除启动 Timer
                    _updateIndicatorGeometry(); // 初始化指示器位置
                    _updateButtonColors();      // 初始化按钮颜色
                }
            }
            
            // --- 文件按钮 (移除 z 属性) ---
            Rectangle {
                id: fileButton
                Layout.fillWidth: true // 参与布局
                Layout.preferredHeight: 36
                radius: 18
                color: "transparent"
                
                // 布局变化时刷新指示器
                onWidthChanged: actionButtons._updateIndicatorGeometry()
                onXChanged: actionButtons._updateIndicatorGeometry()
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 2

                    RowLayout {
                        Layout.alignment: Qt.AlignCenter
                        height: parent.height - 4
                        spacing: 5
                        
                        Image {
                            id: fileIcon
                            source: "qrc:/icons/sidebar/file.svg"
                            width: 18; height: 18; sourceSize.width: width; sourceSize.height: height
                            ColorOverlay {
                                id: fileIconOverlay
                                anchors.fill: parent
                                source: parent
                                color: activeButton === "file" ? "#ffffff" : (fileClickArea.containsMouse ? hoverTextColor : inactiveTextColor)
                            }
                        }
                        Text {
                            id: fileText
                            text: qsTr("文件")
                            font.pixelSize: 13
                            color: activeButton === "file" ? activeTextColor : (fileClickArea.containsMouse ? hoverTextColor : inactiveTextColor)
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
                
                // 布局变化时刷新指示器
                onWidthChanged: actionButtons._updateIndicatorGeometry()
                onXChanged: actionButtons._updateIndicatorGeometry()
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 2
                    RowLayout {
                        Layout.alignment: Qt.AlignCenter
                        height: parent.height - 4
                        spacing: 5
                        
                        Image {
                            id: aiIcon
                            source: "qrc:/icons/sidebar/ai.svg"
                            width: 18; height: 18; sourceSize.width: width; sourceSize.height: height
                            ColorOverlay {
                                id: aiIconOverlay
                                anchors.fill: parent
                                source: parent
                                color: activeButton === "ai" ? "#ffffff" : (aiClickArea.containsMouse ? hoverTextColor : inactiveTextColor)
                            }
                        }
                        Text {
                            id: aiText
                            text: qsTr("AI")
                            font.pixelSize: 13
                            color: activeButton === "ai" ? activeTextColor : (aiClickArea.containsMouse ? hoverTextColor : inactiveTextColor)
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

                // 布局变化时刷新指示器
                onWidthChanged: actionButtons._updateIndicatorGeometry()
                onXChanged: actionButtons._updateIndicatorGeometry()
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 2
                    RowLayout {
                        Layout.alignment: Qt.AlignCenter
                        height: parent.height - 4
                        spacing: 5
                        
                        Image {
                            id: searchIcon
                            source: "qrc:/icons/sidebar/search.svg"
                            width: 18; height: 18; sourceSize.width: width; sourceSize.height: height
                            ColorOverlay {
                                id: searchIconOverlay
                                anchors.fill: parent
                                source: parent
                                color: activeButton === "search" ? "#ffffff" : (searchClickArea.containsMouse ? hoverTextColor : inactiveTextColor)
                            }
                        }
                        Text {
                            id: searchText
                            text: qsTr("搜索")
                            font.pixelSize: 13
                            color: activeButton === "search" ? activeTextColor : (searchClickArea.containsMouse ? hoverTextColor : inactiveTextColor)
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