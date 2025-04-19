import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt5Compat.GraphicalEffects

// 笔记文件树组件
Item {
    id: noteTree
    
    // 属性
    property string selectedNotePath: ""
    property string selectedNoteType: "" // folder 或 note
    
    // 信号
    signal noteSelected(string path, string type)
    signal createFolder(string parentPath)
    signal createNote(string parentPath)
    signal renameItem(string path, string newName)
    signal deleteItem(string path)
    
    // 右键菜单
    FileContextMenu {
        id: contextMenu
        
        onCreateFolderRequest: function(parentPath) {
            createFolder(parentPath)
        }
        
        onCreateNoteRequest: function(parentPath) {
            createNote(parentPath)
        }
        
        onRenameRequest: function(path, name) {
            console.log("重命名:", path, name)
            // 这里应该显示一个输入框让用户输入新名称
            // 然后调用 renameItem 信号
        }
        
        onDeleteRequest: function(path) {
            deleteItem(path)
        }
    }
    
    // 主容器
    Rectangle {
        id: containerRect
        anchors.fill: parent
        color: "transparent" // 容器背景透明
        
        // 标题部分
        Rectangle {
            id: headerRect
            width: parent.width
            height: 50
            color: "#ffffff"
            radius: 8
            
            // 标题文本
            Text {
                id: headerText
                text: "全部笔记"
                font.pixelSize: 16
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 15
                color: "#333333"
            }
            
            // 展开/折叠按钮
            Rectangle {
                id: expandBtn
                width: 24
                height: 24
                radius: 12
                color: "transparent"
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: 15
                
                Image {
                    id: expandIcon
                    anchors.centerIn: parent
                    source: "qrc:/icons/round_right_fill.svg"
                    width: 14
                    height: 14
                    rotation: 0 // 初始状态 (向右)
                }
                
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    
                    property bool expanded: true
                    
                    onClicked: {
                        expanded = !expanded
                        expandIcon.rotation = expanded ? 0 : 90
                        listView.visible = expanded
                    }
                }
            }
        }
        
        // 文件列表
        ListView {
            id: listView
            anchors.top: headerRect.bottom
            anchors.topMargin: 10
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            
            // 为每个项目添加间距
            spacing: 8
            clip: false // 不裁剪，让阴影能够溢出
            
            // 使用 FolderListModel 或自定义 C++ 模型提供数据
            model: ListModel {
                // 示例数据
                ListElement { 
                    name: "我的资源"; 
                    path: "/root/我的资源"; 
                    type: "folder"; 
                    expanded: true;
                    level: 0;
                }
                ListElement { 
                    name: "CMake"; 
                    path: "/root/我的资源/CMake"; 
                    type: "folder"; 
                    expanded: false;
                    level: 1;
                }
                ListElement { 
                    name: "Git 笔记"; 
                    path: "/root/我的资源/Git笔记"; 
                    type: "folder"; 
                    expanded: false;
                    level: 1;
                }
                ListElement { 
                    name: "Linux系统操作基础"; 
                    path: "/root/我的资源/Linux系统操作基础"; 
                    type: "folder"; 
                    expanded: false;
                    level: 1;
                    starred: true;
                }
                ListElement { 
                    name: "SHELL"; 
                    path: "/root/我的资源/SHELL"; 
                    type: "folder"; 
                    expanded: false;
                    level: 1;
                }
                ListElement { 
                    name: "Thinkings"; 
                    path: "/root/我的资源/Thinkings"; 
                    type: "folder"; 
                    expanded: false;
                    level: 1;
                }
                ListElement { 
                    name: "各游戏笔记记录"; 
                    path: "/root/我的资源/各游戏笔记记录"; 
                    type: "folder"; 
                    expanded: false;
                    level: 1;
                }
                ListElement { 
                    name: "C++ Primer Plus (第六版)"; 
                    path: "/root/我的资源/C++ Primer Plus (第六版)"; 
                    type: "folder"; 
                    expanded: false;
                    level: 1;
                }
            }
            
            delegate: Item {
                id: itemContainer
                width: ListView.view.width - 20 // 给阴影留出空间
                height: 44
                x: 10 // 居中定位
                
                // 阴影 (整个卡片的阴影)
                DropShadow {
                    anchors.fill: itemCard
                    horizontalOffset: 0
                    verticalOffset: 2
                    radius: itemMouseArea.containsMouse || noteTree.selectedNotePath === model.path ? 8.0 : 4.0
                    samples: 17
                    color: "#20000000"
                    source: itemCard
                    visible: true // 始终显示阴影，但强度不同
                    
                    // 平滑动画
                    Behavior on radius { NumberAnimation { duration: 150 } }
                }
                
                // 卡片主体
                Rectangle {
                    id: itemCard
                    anchors.fill: parent
                    radius: 8
                    color: "#ffffff"
                    border.width: 1
                    border.color: itemMouseArea.containsMouse || noteTree.selectedNotePath === model.path ? 
                                 "#4285F4" : "#e0e0e0"
                    
                    // 指示条 (选中时)
                    Rectangle {
                        id: selectionIndicator
                        width: 4
                        height: parent.height - 8
                        radius: 2
                        anchors.left: parent.left
                        anchors.leftMargin: 4
                        anchors.verticalCenter: parent.verticalCenter
                        color: "#4285F4"
                        visible: noteTree.selectedNotePath === model.path
                        
                        // 淡入淡出
                        Behavior on opacity { NumberAnimation { duration: 150 } }
                    }
                    
                    // 内容布局
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16 + model.level * 20 // 根据层级缩进
                        anchors.rightMargin: 16
                        spacing: 12
                        
                        // 展开/折叠图标 (仅文件夹显示)
                        Image {
                            id: folderExpandIcon
                            Layout.preferredWidth: 14
                            Layout.preferredHeight: 14
                            source: "qrc:/icons/round_right_fill.svg"
                            visible: model.type === "folder"
                            opacity: model.type === "folder" ? 1.0 : 0.0
                            rotation: model.expanded ? 90 : 0
                            Layout.alignment: Qt.AlignVCenter
                            
                            // 展开/折叠动画
                            Behavior on rotation { NumberAnimation { duration: 200; easing.type: Easing.OutQuad } }
                            
                            MouseArea {
                                anchors.fill: parent
                                anchors.margins: -5 // 扩大点击区域
                                onClicked: {
                                    model.expanded = !model.expanded
                                    console.log("切换展开状态:", model.path, model.expanded)
                                    // TODO: 实现子项展开/折叠
                                }
                            }
                        }
                        
                        // 文件/文件夹图标
                        Image {
                            id: typeIcon
                            Layout.preferredWidth: 20
                            Layout.preferredHeight: 20
                            source: model.type === "folder" ? 
                                   "qrc:/icons/sidebar/folder.svg" : 
                                   "qrc:/icons/sidebar/note.svg"
                            Layout.alignment: Qt.AlignVCenter
                            
                            ColorOverlay {
                                anchors.fill: parent
                                source: parent
                                color: noteTree.selectedNotePath === model.path ? 
                                      "#4285F4" : "#666666" // 选中时图标变蓝
                                
                                // 平滑过渡
                                Behavior on color { ColorAnimation { duration: 150 } }
                            }
                        }
                        
                        // 名称
                        Label {
                            id: nameLabel
                            text: model.name
                            font.pixelSize: 14
                            font.weight: noteTree.selectedNotePath === model.path ? 
                                       Font.DemiBold : Font.Normal // 选中时加粗
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                            color: noteTree.selectedNotePath === model.path ? 
                                  "#4285F4" : "#333333" // 选中时文字变蓝
                            
                            // 平滑过渡
                            Behavior on color { ColorAnimation { duration: 150 } }
                        }
                        
                        // 星标 (收藏)
                        Image {
                            id: starIcon
                            Layout.preferredWidth: 16
                            Layout.preferredHeight: 16
                            source: "qrc:/icons/month.svg"
                            visible: model.starred === true
                            Layout.alignment: Qt.AlignVCenter
                            
                            ColorOverlay {
                                anchors.fill: parent
                                source: parent
                                color: "#FFD700" // 金色
                            }
                        }
                    }
                }
                
                // 鼠标交互区域
                MouseArea {
                    id: itemMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    cursorShape: Qt.PointingHandCursor
                    
                    // 卡片缩放动画
                    onContainsMouseChanged: {
                        if (containsMouse) {
                            itemContainer.scale = 1.02
                        } else {
                            itemContainer.scale = 1.0
                        }
                    }
                    
                    onClicked: function(mouse) {
                        if (mouse.button === Qt.LeftButton) {
                            // 左键点击：选中项目
                            noteTree.selectedNotePath = model.path
                            noteTree.selectedNoteType = model.type
                            noteSelected(model.path, model.type)
                        } else if (mouse.button === Qt.RightButton) {
                            // 右键点击：显示上下文菜单
                            contextMenu.isFolder = model.type === "folder"
                            contextMenu.itemPath = model.path
                            contextMenu.itemName = model.name
                            contextMenu.popup()
                        }
                    }
                }
                
                // 缩放动画
                Behavior on scale { 
                    NumberAnimation { 
                        duration: 100
                        easing.type: Easing.OutQuad
                    }
                }
            }
            
            // 滚动条
            ScrollBar.vertical: ScrollBar {
                id: scrollBar
                active: true
                interactive: true
                policy: ScrollBar.AsNeeded
                
                // 现代化的滚动条样式
                contentItem: Rectangle {
                    implicitWidth: 6
                    implicitHeight: 100
                    radius: width / 2
                    color: scrollBar.pressed ? "#606060" : "#a0a0a0"
                    opacity: scrollBar.active ? 0.8 : 0.4
                    
                    // 鼠标悬停和按下效果
                    Behavior on color { ColorAnimation { duration: 150 } }
                    Behavior on opacity { NumberAnimation { duration: 150 } }
                }
            }
        }
    }
} 