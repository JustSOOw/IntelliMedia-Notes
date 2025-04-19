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
    
    // 信号
    signal createFolderRequest(string parentPath)
    signal createNoteRequest(string parentPath)
    signal renameRequest(string path, string name)
    signal deleteRequest(string path)
    
    // 动画
    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 150 }
        NumberAnimation { property: "scale"; from: 0.8; to: 1.0; duration: 150; easing.type: Easing.OutBack }
    }
    
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 100 }
    }
    
    // 创建笔记按钮
    Action {
        id: createNoteAction
        text: "创建笔记"
        icon.source: "qrc:/icons/sidebar/note.svg"
        enabled: isFolder
        onTriggered: createNoteRequest(itemPath)
    }
    
    // 创建文件夹按钮
    Action {
        id: createFolderAction
        text: "创建文件夹"
        icon.source: "qrc:/icons/sidebar/folder.svg"
        enabled: isFolder
        onTriggered: createFolderRequest(itemPath)
    }
    
    // 分隔线
    MenuSeparator {
        visible: isFolder
        contentItem: Rectangle {
            implicitWidth: 180
            implicitHeight: 1
            color: "#e0e0e0"
        }
    }
    
    // 重命名按钮
    Action {
        id: renameAction
        text: "重命名"
        icon.source: "qrc:/icons/sidebar/note.svg"
        onTriggered: renameRequest(itemPath, itemName)
    }
    
    // 删除按钮
    Action {
        id: deleteAction
        text: "删除"
        icon.source: "qrc:/icons/round_close_fill.svg"
        icon.color: "#333333"
        onTriggered: deleteRequest(itemPath)
    }
    
    // 菜单项 (移除显式 MenuItem，让 Menu 自动创建)
    // MenuItem { action: createNoteAction; visible: isFolder }
    // MenuItem { action: createFolderAction; visible: isFolder }
    // MenuSeparator { visible: isFolder }
    // MenuItem { action: renameAction }
    // MenuItem { action: deleteAction }
    
    // 注意：如果需要控制特定项的可见性（如只在文件夹上显示创建），
    // 可以在 Action 的 visible 属性上绑定，例如：
    // Action { id: createNoteAction; ... ; visible: contextMenu.isFolder }
    // 确认 Action 的 enabled 属性已正确绑定 (enabled: isFolder)

    background: Rectangle {
        implicitWidth: 180
        color: "#ffffff"
        border.color: "#e0e0e0"
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
            color: "#20000000"
        }
    }
} 