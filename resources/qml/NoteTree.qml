import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt5Compat.GraphicalEffects

// 笔记文件树组件
Rectangle {
    id: noteTreeRect
    width: parent.width
    color: "transparent"
    
    // 当前选中的笔记路径
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
            // 显示重命名输入框
            console.log("重命名:", path, name)
            // 这里应该显示一个输入框让用户输入新名称
            // 然后调用 renameItem 信号
        }
        
        onDeleteRequest: function(path) {
            deleteItem(path)
        }
    }
    
    // 标题部分
    Rectangle {
        id: treeHeader
        width: parent.width
        height: 40
        color: "transparent"
        anchors.top: parent.top
        
        Text {
            text: "全部笔记"
            font.pixelSize: 16
            font.bold: true
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 15
            color: "#333333"
        }
        
        // 展开/折叠按钮
        Image {
            id: expandCollapseIcon
            source: "qrc:/resources/icons/round_right_fill.svg"
            width: 16
            height: 16
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 15
            
            property bool expanded: true
            
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    expandCollapseIcon.expanded = !expandCollapseIcon.expanded
                    expandCollapseIcon.rotation = expandCollapseIcon.expanded ? 0 : 90
                    noteTree.visible = expandCollapseIcon.expanded
                }
            }
            
            Behavior on rotation {
                NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
            }
        }
    }
    
    // 树视图
    ListView {
        id: noteTree
        anchors.top: treeHeader.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        clip: true
        
        // 使用FolderListModel或自定义C++模型提供数据
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
        
        delegate: Rectangle {
            id: noteItem
            width: ListView.view.width
            height: 36
            color: selectedNotePath === model.path ? "#e3f2fd" : "transparent"
            radius: 4
            
            // 条目布局
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10 + model.level * 20 // 根据层级缩进
                anchors.rightMargin: 10
                spacing: 8
                
                // 展开/折叠图标（仅文件夹显示）
                Image {
                    id: expandIcon
                    source: "qrc:/resources/icons/round_right_fill.svg"
                    width: 14
                    height: 14
                    visible: model.type === "folder"
                    rotation: model.expanded ? 90 : 0
                    Layout.alignment: Qt.AlignVCenter
                    
                    MouseArea {
                        anchors.fill: parent
                        anchors.margins: -5 // 扩大点击区域
                        onClicked: {
                            model.expanded = !model.expanded
                            // 这里应该展开或折叠子项
                            console.log("切换展开状态:", model.path, model.expanded)
                        }
                    }
                    
                    Behavior on rotation {
                        NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
                    }
                }
                
                // 图标
                Image {
                    source: model.type === "folder" ? 
                           "qrc:/resources/icons/sidebar/folder.svg" : 
                           "qrc:/resources/icons/sidebar/note.svg"
                    width: 16
                    height: 16
                    Layout.alignment: Qt.AlignVCenter
                }
                
                // 名称
                Text {
                    text: model.name
                    font.pixelSize: 14
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    color: "#333333"
                }
                
                // 收藏星标（如果有）
                Image {
                    source: "qrc:/resources/icons/month.svg"
                    width: 16
                    height: 16
                    visible: model.starred === true
                    Layout.alignment: Qt.AlignVCenter
                    
                    ColorOverlay {
                        anchors.fill: parent
                        source: parent
                        color: "#FFD700" // 金色
                    }
                }
            }
            
            // 点击选择
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                
                onClicked: function(mouse) {
                    if (mouse.button === Qt.LeftButton) {
                        selectedNotePath = model.path
                        selectedNoteType = model.type
                        noteSelected(model.path, model.type)
                    } else if (mouse.button === Qt.RightButton) {
                        // 右键菜单
                        contextMenu.isFolder = model.type === "folder"
                        contextMenu.itemPath = model.path
                        contextMenu.itemName = model.name
                        contextMenu.popup()
                    }
                }
            }
            
            // 悬停效果
            states: [
                State {
                    name: "hovered"
                    when: mouseArea.containsMouse && selectedNotePath !== model.path
                    PropertyChanges { target: noteItem; color: "#f5f5f5" }
                }
            ]
            
            // 鼠标区域用于悬停检测
            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.NoButton // 不处理点击
            }
        }
        
        // 滚动条
        ScrollBar.vertical: ScrollBar {
            active: true
            policy: ScrollBar.AsNeeded
        }
    }
} 