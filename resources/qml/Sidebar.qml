import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt5Compat.GraphicalEffects // 导入图形效果模块

// 侧边栏主组件
Rectangle {
    id: sidebarRoot
    // 设置为白色背景，匹配 light_theme.qss 中的 sidebarContainer
    // 注意：如果需要支持暗色主题，这里需要动态绑定颜色
    color: "#ffffff"
    
    // 添加阴影效果
    layer.enabled: true
    layer.effect: DropShadow {
        horizontalOffset: 1 // 向右偏移一点
        verticalOffset: 0
        radius: 8.0
        samples: 17
        color: "#1a000000" // 非常淡的黑色阴影
        source: sidebarRoot // 指定效果源为 sidebarRoot 本身
    }
    
    // 当前视图模式：file, ai
    property string currentView: "file"
    
    // 信号
    signal noteSelected(string path, string type)
    signal createNote(string parentPath)
    signal createFolder(string parentPath)
    signal renameItem(string path, string newName)
    signal deleteItem(string path)
    signal sendAIMessage(string message)
    
    // 使用 anchors 替换 ColumnLayout
    // ColumnLayout {
    //     anchors.fill: parent
    //     spacing: 0
        
    // 用户信息面板 (使用 anchors)
    UserPanel {
        id: userPanel
        width: parent.width // 宽度填充父级
        // Layout.fillWidth: true // 移除 Layout 属性
        // Layout.preferredHeight: 80 // 使用 height
        height: 80
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
    }
    
    // 操作按钮区域 (使用 anchors)
    ActionButtons {
        id: actionButtons
        width: parent.width // 宽度填充父级
        // Layout.fillWidth: true // 移除 Layout 属性
        // Layout.preferredHeight: 120 // 使用 height
        height: 120
        anchors.top: userPanel.bottom // 锚定在 UserPanel 下方
        anchors.left: parent.left
        anchors.right: parent.right
        activeButton: currentView
        
        onFileButtonClicked: {
            currentView = "file"
            stackLayout.currentIndex = 0
        }
        
        onAiButtonClicked: {
            currentView = "ai"
            stackLayout.currentIndex = 1
        }
        
        onSearchButtonClicked: {
            // 搜索功能暂未实现
            console.log("搜索功能点击")
        }
        
        onCreateNewNoteClicked: {
            // 默认在当前选中文件夹或根目录创建笔记
            if (noteTree.selectedNoteType === "folder") {
                createNote(noteTree.selectedNotePath)
            } else {
                createNote("/root") // 根目录
            }
        }
    }
    
    // 内容区域（使用堆叠布局切换文件树和AI视图） (使用 anchors)
    StackLayout {
        id: stackLayout
        anchors.top: actionButtons.bottom // 锚定在 ActionButtons 下方
        anchors.bottom: parent.bottom // 锚定到底部
        anchors.left: parent.left
        anchors.right: parent.right
        currentIndex: currentView === "file" ? 0 : 1
        
        // 文件树视图
        NoteTree {
            id: noteTree
            
            onNoteSelected: function(path, type) {
                sidebarRoot.noteSelected(path, type)
            }
            
            onCreateFolder: function(parentPath) {
                sidebarRoot.createFolder(parentPath)
            }
            
            onCreateNote: function(parentPath) {
                sidebarRoot.createNote(parentPath)
            }
            
            onRenameItem: function(path, newName) {
                sidebarRoot.renameItem(path, newName)
            }
            
            onDeleteItem: function(path) {
                sidebarRoot.deleteItem(path)
            }
        }
        
        // AI助手视图
        AIView {
            id: aiView
            
            onSendMessage: function(message) {
                sidebarRoot.sendAIMessage(message)
            }
        }
    }
    // }
    
    // 组件初始化完成后的处理
    Component.onCompleted: {
        console.log("侧边栏初始化完成")
        // 打印 StackLayout 的高度 (移除)
        // console.log("[Sidebar] stackLayout height on completed:", stackLayout.height)
    }
}
