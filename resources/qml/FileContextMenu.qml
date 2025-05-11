import QtQuick 2.15
import QtQuick.Controls 2.15
import Qt5Compat.GraphicalEffects

// 文件系统右键菜单
Menu {
    id: contextMenu
    width: 180
    
    // 上下文属性
    property bool isFolder: false
    property string itemPath: ""
    property string itemName: ""
    
    // 主题相关颜色
    property color menuBgColor: sidebarManager.isDarkTheme ? "#353535" : "#ffffff"
    property color menuBorderColor: sidebarManager.isDarkTheme ? "#505050" : "#e0e0e0"
    property color menuShadowColor: sidebarManager.isDarkTheme ? "#40000000" : "#20000000"
    property color dividerColor: sidebarManager.isDarkTheme ? "#454545" : "#e0e0e0"
    property color textColor: sidebarManager.isDarkTheme ? "#e0e0e0" : "#333333"
    property color folderIconColor: sidebarManager.isDarkTheme ? "#80cbc4" : "#4285F4"
    property color noteIconColor: sidebarManager.isDarkTheme ? "#a5d6a7" : "#43a047"
    property color deleteIconColor: sidebarManager.isDarkTheme ? "#ff6060" : "#333333"
    property color hoverColor: sidebarManager.isDarkTheme ? "#454545" : "#f0f0f0"
    
    // 信号 - 修改为请求信号
    signal createFolderRequested(string parentPath)
    signal createNoteRequested(string parentPath)
    signal renameRequested(string path, string name)
    signal deleteRequested(string path)
    
    // 动画
    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 150 }
        NumberAnimation { property: "scale"; from: 0.8; to: 1.0; duration: 150; easing.type: Easing.OutBack }
    }
    
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 100 }
    }
    
    // 菜单项样式
    delegate: MenuItem {
        id: menuItem
        implicitWidth: 180
        implicitHeight: 40
        
        contentItem: Row {
            leftPadding: 10
            spacing: 8
            
            Image {
                source: menuItem.icon.source
                width: 18
                height: 18
                anchors.verticalCenter: parent.verticalCenter
                
                ColorOverlay {
                    anchors.fill: parent
                    source: parent
                    color: {
                        if (menuItem.action === createFolderAction) return folderIconColor;
                        else if (menuItem.action === createNoteAction || menuItem.action === renameAction) return noteIconColor;
                        else if (menuItem.action === deleteAction) return deleteIconColor;
                        else return textColor;
                    }
                }
            }
            
            Text {
                text: menuItem.text
                color: textColor
                font.pixelSize: 14
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        
        background: Rectangle {
            implicitWidth: 180
            implicitHeight: 40
            color: menuItem.highlighted ? hoverColor : "transparent"
            radius: 4
        }
    }
    
    // 创建笔记按钮
    Action {
        id: createNoteAction
        text: qsTr("创建笔记")
        icon.source: "qrc:/icons/sidebar/note.svg"
        enabled: isFolder
        onTriggered: {
            console.log("[ContextMenu] Create Note Requested for:", itemPath)
            // 发射请求信号给 NoteTree
            contextMenu.createNoteRequested(itemPath)
        }
    }
    
    // 创建文件夹按钮
    Action {
        id: createFolderAction
        text: qsTr("创建文件夹")
        icon.source: "qrc:/icons/sidebar/folder.svg"
        enabled: isFolder
        onTriggered: {
            console.log("[ContextMenu] Create Folder Requested for:", itemPath)
            // 发射请求信号给 NoteTree
            contextMenu.createFolderRequested(itemPath)
        }
    }
    
    // 分隔线
    MenuSeparator {
        visible: isFolder
        contentItem: Rectangle {
            implicitWidth: 180
            implicitHeight: 1
            color: dividerColor
        }
    }
    
    // 重命名按钮
    Action {
        id: renameAction
        text: qsTr("重命名")
        icon.source: "qrc:/icons/sidebar/note.svg"
        onTriggered: {
            console.log("[ContextMenu] Rename Requested for:", itemPath, "Name:", itemName)
            // 发射请求信号给 NoteTree
            contextMenu.renameRequested(itemPath, itemName)
        }
    }
    
    // 删除按钮
    Action {
        id: deleteAction
        text: qsTr("删除")
        icon.source: "qrc:/icons/round_close_fill.svg"
        onTriggered: {
            console.log("[ContextMenu] Delete Requested for:", itemPath)
            // 发射请求信号给 NoteTree
            contextMenu.deleteRequested(itemPath)
        }
    }
    
    background: Rectangle {
        implicitWidth: 180
        color: menuBgColor
        border.color: menuBorderColor
        border.width: 1
        radius: 6
        
        // 阴影效果
        layer.enabled: true
        layer.effect: DropShadow {
            transparentBorder: true
            horizontalOffset: 0
            verticalOffset: 2
            radius: 8.0
            samples: 17
            color: menuShadowColor
        }
    }
    
    // 监听主题变化
    Connections {
        target: sidebarManager
        function onThemeChanged() {
            // 属性会自动更新，无需额外操作
            console.log("ContextMenu: Theme changed to", sidebarManager.isDarkTheme ? "dark" : "light")
        }
    }
} 