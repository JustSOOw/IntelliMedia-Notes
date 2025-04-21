import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt5Compat.GraphicalEffects

// 搜索视图组件
Rectangle {
    id: searchRoot
    color: "#f5f5f5" // 浅灰色背景
    radius: 8 // 添加圆角以匹配无边框窗口
    clip: true // 确保内容在圆角内
    
    // 信号定义
    signal closeSearch()
    signal openNote(string path, string type)
    
    // 属性
    property var searchResults: [] // 搜索结果数组
    property bool isLoading: true // 添加加载状态
    
    // 自定义标题栏
    Rectangle {
        id: customTitleBar
        width: parent.width
        height: 40 // 标题栏高度
        color: "#ffffff" // 修改背景为白色
        anchors.top: parent.top
        
        // 拖动区域
        MouseArea {
            id: dragArea
            anchors.fill: parent
            // 排除按钮区域
            anchors.right: closeButton.left
            anchors.rightMargin: 5
            
            property point clickPos: "0,0"
            
            onPressed: (mouse) => {
                var globalPos = mapToGlobal(mouse.x, mouse.y)
                searchManager.handleTitleBarPressed(globalPos.x, globalPos.y)
            }
            onPositionChanged: (mouse) => {
                var globalPos = mapToGlobal(mouse.x, mouse.y)
                searchManager.handleTitleBarMoved(globalPos.x, globalPos.y)
            }
            onReleased: searchManager.handleTitleBarReleased()
            onCanceled: searchManager.handleTitleBarReleased()
        }
        
        // 关闭按钮 (放置在标题栏右侧)
        Rectangle {
            id: closeButton
            width: 32
            height: 32
            radius: 16
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            color: closeMouseArea.containsMouse ? "#c75450" : "transparent" // 关闭按钮悬停颜色
            
            Image {
                anchors.centerIn: parent
                width: 14
                height: 14
                source: "qrc:///icons/round_close_fill.svg"
                fillMode: Image.PreserveAspectFit
                ColorOverlay {
                    anchors.fill: parent
                    source: parent
                    color: closeMouseArea.containsMouse ? "#ffffff" : "#555555"
                }
            }
            
            MouseArea {
                id: closeMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    console.log("QML Close button clicked")
                    searchManager.onDialogClosed()
                    searchManager.searchClosed() // 主动发射信号，恢复侧边栏
                }
            }
        }
    }
    
    // 搜索栏区域
    Rectangle {
        id: searchHeader
        width: parent.width
        height: 70
        color: "#ffffff"
        anchors.top: customTitleBar.bottom // 锚定到自定义标题栏下方
        
        // 添加阴影效果
        layer.enabled: true
        layer.effect: DropShadow {
            horizontalOffset: 0
            verticalOffset: 2
            radius: 8.0
            samples: 17
            color: "#1a000000"
        }
        
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20 // 增加左边距
            anchors.rightMargin: 20 // 增加右边距
            anchors.verticalCenter: parent.verticalCenter
            spacing: 15
            
            // 搜索框 (调整布局以包含清除按钮)
            Rectangle {
                id: searchInputRect
                Layout.fillWidth: true
                height: 40
                radius: 20
                color: "#f0f0f0"
                border.color: searchField.activeFocus ? "#3a7afe" : "transparent"
                border.width: 1
                
                Image {
                    id: searchIconInBox // 给图标一个ID
                    width: 20
                    height: 20
                    source: "qrc:///icons/sidebar/search.svg"
                    fillMode: Image.PreserveAspectFit
                    anchors.left: parent.left
                    anchors.leftMargin: 15
                    anchors.verticalCenter: parent.verticalCenter
                }
                
                TextField {
                    id: searchField
                    // 使用 anchors 定位
                    anchors.left: searchIconInBox.right
                    anchors.leftMargin: 8
                    anchors.right: clearButton.left // 锚定到清除按钮左侧
                    anchors.rightMargin: 5
                    anchors.verticalCenter: parent.verticalCenter
                    height: parent.height // 填充高度
                    
                    placeholderText: "搜索笔记..."
                    background: Item {} // 移除默认背景
                    selectByMouse: true
                    font.pixelSize: 14
                    verticalAlignment: TextInput.AlignVCenter // 垂直居中文本
                    
                    onAccepted: {
                        searchManager.searchNotes(text.trim(), 0, 0, sortOrder.currentIndex)
                    }
                }
                
                // 清除按钮 (放置在搜索框内部右侧)
                Rectangle {
                    id: clearButton
                    width: 24
                    height: 24
                    radius: 12
                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    color: clearMouseArea.containsMouse ? "#e0e0e0" : "transparent"
                    visible: searchField.text.length > 0
                    z: 1 // 确保在 TextField 上方
                    
                    Text {
                        anchors.centerIn: parent
                        text: "×"
                        font.pixelSize: 16
                        color: "#606060" // 恢复为灰色
                    }
                    
                    MouseArea {
                        id: clearMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            searchField.text = ""
                            searchField.focus = true
                            searchManager.searchNotes("", 0, 0, sortOrder.currentIndex)
                        }
                    }
                }
            }
            
            // 搜索按钮
            Rectangle {
                id: searchButton
                width: 100
                height: 40
                radius: 20
                color: searchBtnMouseArea.containsMouse ? "#2a6ade" : "#3a7afe"
                
                Text {
                    anchors.centerIn: parent
                    text: "搜索"
                    font.pixelSize: 14
                    color: "#ffffff"
                }
                
                MouseArea {
                    id: searchBtnMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        searchManager.searchNotes(searchField.text.trim(), 0, 0, sortOrder.currentIndex)
                    }
                }
            }
        }
    }
    
    // 筛选/排序区域
    Rectangle {
        id: filterBar
        width: parent.width
        height: 50
        color: "#ffffff"
        anchors.top: searchHeader.bottom
        
        RowLayout {
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 20
            spacing: 10
            
            Text {
                text: "排序:"
                font.pixelSize: 13
                color: "#606060"
            }
            
            // 排序方式 (自定义样式)
            ComboBox {
                id: sortOrder
                model: ["最近修改", "创建时间", "按名称排序"]
                currentIndex: 0
                font.pixelSize: 13
                implicitWidth: 130
                implicitHeight: 32
                
                background: Rectangle {
                    color: sortOrder.hovered ? "#e8e8e8" : "#f0f0f0"
                    radius: 16
                    border.color: sortOrder.activeFocus ? "#3a7afe" : "transparent"
                    border.width: 1
                }
                
                contentItem: Text {
                    text: sortOrder.displayText
                    font: sortOrder.font
                    color: "#333333"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                    leftPadding: 10
                    rightPadding: sortOrder.indicator.width + sortOrder.spacing
                }
                
                indicator: Rectangle {
                    x: sortOrder.width - width - sortOrder.rightPadding + 5
                    y: sortOrder.height / 2 - height / 2
                    width: 12
                    height: 8
                    color: "transparent"
                    
                    Canvas {
                        anchors.fill: parent
                        onPaint: {
                            var ctx = getContext("2d");
                            ctx.fillStyle = "#606060";
                            ctx.beginPath();
                            ctx.moveTo(0, 0);
                            ctx.lineTo(width, 0);
                            ctx.lineTo(width / 2, height);
                            ctx.closePath();
                            ctx.fill();
                        }
                    }
                }
                
                popup: Popup {
                    y: sortOrder.height
                    width: sortOrder.width
                    padding: 1
                    
                    contentItem: ListView {
                        clip: true
                        implicitHeight: contentHeight
                        model: sortOrder.popup.visible ? sortOrder.delegateModel : null
                        currentIndex: sortOrder.currentIndex
                        
                        ScrollIndicator.vertical: ScrollIndicator { }
                    }
                    
                    background: Rectangle {
                        color: "#ffffff"
                        border.color: "#c0c0c0"
                        radius: 5
                    }
                }
                
                delegate: ItemDelegate {
                    width: parent.width
                    contentItem: Text {
                        text: modelData
                        font: sortOrder.font
                        color: highlighted ? "#ffffff" : "#333333"
                        elide: Text.ElideRight
                    }
                    highlighted: sortOrder.currentIndex === index
                    
                    background: Rectangle {
                        color: highlighted ? "#3a7afe" : (hovered ? "#e8e8e8" : "transparent")
                        radius: 3
                    }
                }
                
                onActivated: {
                    searchManager.searchNotes(searchField.text.trim(), 0, 0, currentIndex)
                }
            }
        }
    }
    
    // 结果区域
    Rectangle {
        id: resultsArea
        anchors.top: filterBar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        color: "#f5f5f5"
        
        // 加载指示器
        BusyIndicator {
            anchors.centerIn: parent
            running: isLoading && searchResults.length === 0
        }
        
        // 空状态提示 (区分加载中和无结果)
        Item {
            anchors.centerIn: parent
            width: 300
            height: 200
            visible: !isLoading && resultGrid.count === 0
            
            Column {
                anchors.centerIn: parent
                spacing: 20
                
                Image {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 80
                    height: 80
                    source: "qrc:///icons/sidebar/search.svg"
                    fillMode: Image.PreserveAspectFit
                    opacity: 0.5
                }
                
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: searchField.text.trim().length > 0 ? "未找到匹配的笔记" : "没有笔记，快去创建吧！" // 修改空状态文本
                    font.pixelSize: 16
                    color: "#808080"
                }
            }
        }
        
        // 搜索结果网格
        ScrollView {
            id: resultScrollView
            anchors.fill: parent
            anchors.margins: 20
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            visible: !isLoading // 加载完成再显示
            
            GridView {
                id: resultGrid
                anchors.fill: parent
                cellWidth: Math.floor(parent.width / Math.max(2, Math.floor(parent.width / 300))) // 调整卡片宽度计算
                cellHeight: 200
                model: searchResults
                
                delegate: Rectangle {
                    id: noteCard
                    // 调整 delegate 大小以创建间距
                    width: GridView.view.cellWidth - 20 
                    height: GridView.view.cellHeight - 20
                    radius: 8
                    color: "#ffffff"
                    border.color: noteMouseArea.containsMouse ? Qt.lighter("#1a000000", 1.5) : "transparent"
                    border.width: 1
                    
                    // 添加阴影效果
                    layer.enabled: true
                    layer.effect: DropShadow {
                        horizontalOffset: 0
                        verticalOffset: 2
                        radius: noteMouseArea.containsMouse ? 10.0 : 6.0
                        samples: 17
                        color: noteMouseArea.containsMouse ? "#2a000000" : "#1a000000"
                        Behavior on radius { NumberAnimation { duration: 150 } }
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }
                    
                    // 卡片内容
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 15
                        spacing: 10
                        
                        // 标题区域
                        RowLayout {
                            width: parent.width
                            // 笔记图标
                            Image {
                                id: noteIcon
                                Layout.preferredWidth: 20
                                Layout.preferredHeight: 20
                                source: "qrc:///icons/sidebar/note.svg"
                                fillMode: Image.PreserveAspectFit
                            }
                            
                            // 笔记标题
                            Text {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                text: modelData.title
                                font.pixelSize: 15
                                font.bold: true
                                elide: Text.ElideRight
                                color: "#333333"
                            }
                        }
                        
                        // 分隔线
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: "#e0e0e0"
                        }
                        
                        // 内容预览
                        Text {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 70
                            text: modelData.previewText || "无内容预览"
                            font.pixelSize: 13
                            color: "#606060"
                            wrapMode: Text.WordWrap
                            elide: Text.ElideRight
                            maximumLineCount: 4
                        }
                        
                        // 底部信息 (使用Spacer优化布局)
                        RowLayout {
                            width: parent.width
                            spacing: 5
                            
                            Text {
                                text: modelData.updatedAt ? "修改: " + modelData.updatedAt : ""
                                font.pixelSize: 11
                                color: "#909090"
                            }
                            
                            // 添加弹性空间将创建时间推到右边
                            Item {
                                Layout.fillWidth: true
                            }
                            
                            Text {
                                text: modelData.createdAt ? "创建: " + modelData.createdAt : ""
                                font.pixelSize: 11
                                color: "#909090"
                            }
                        }
                    }
                    
                    // 卡片点击区域
                    MouseArea {
                        id: noteMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        
                        onClicked: {
                            searchRoot.openNote(modelData.path, "note")
                            searchRoot.closeSearch()
                        }
                    }
                }
            }
        }
    }
    
    // 打开动画
    ParallelAnimation {
        id: openAnimation
        running: true
        
        NumberAnimation {
            target: searchRoot
            property: "opacity"
            from: 0
            to: 1
            duration: 200
            easing.type: Easing.OutQuad
        }
        
        NumberAnimation {
            target: searchRoot
            property: "scale"
            from: 0.95
            to: 1
            duration: 200
            easing.type: Easing.OutQuad
        }
    }
    
    // 组件初始化完成后的处理
    Component.onCompleted: {
        console.log("SearchView.qml: Component.onCompleted")
        searchField.forceActiveFocus()
        // 可以在这里触发一次初始搜索，如果 C++ 的初始调用有问题
        // searchManager.searchNotes("", 0, 0, 0) 
    }
    
    // 处理搜索结果更新
    function updateSearchResults(results) {
        console.log("SearchView.qml: updateSearchResults called with results count:", results.length) // 添加日志
        isLoading = false // 收到结果，停止加载状态
        searchResults = results
    }
    
    // 重置搜索状态 (用于再次打开时)
    function resetSearch() {
        console.log("SearchView.qml: resetSearch called") // 添加日志
        searchField.text = ""
        searchResults = []
        sortOrder.currentIndex = 0 // 重置排序
        isLoading = true // 重置时标记为加载中，直到首次数据返回
    }
} 