import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt5Compat.GraphicalEffects // 导入图形效果模块
import "." as Local // 导入当前目录下的组件

// 侧边栏主组件
Rectangle {
    id: sidebarRoot
    // 根据主题动态设置背景色
    color: sidebarManager.isDarkTheme ? "#353535" : "#ffffff"
    
    // 添加阴影效果
    layer.enabled: true
    layer.effect: DropShadow {
        horizontalOffset: 1 // 向右偏移一点
        verticalOffset: 0
        radius: 8.0
        samples: 17
        color: sidebarManager.isDarkTheme ? "#50000000" : "#1a000000" // 根据主题调整阴影颜色
        source: sidebarRoot // 指定效果源为 sidebarRoot 本身
    }
    
    // 当前视图模式：file, ai
    property string currentView: "file"
    
    // 信号
    signal noteSelected(string path, string type)
    signal createNote(string parentPath, string noteName)
    signal createFolder(string parentPath, string folderName)
    signal renameItem(string path, string newName)
    signal deleteItem(string path)
    signal sendAIMessage(string message)
    signal searchButtonClicked() // 添加搜索按钮点击信号
    
    // 使用 anchors 替换 ColumnLayout
    // ColumnLayout {
    //     anchors.fill: parent
    //     spacing: 0
        
    // 用户信息面板 (使用 anchors)
    UserPanel {
        id: userPanel
        width: parent.width // 宽度填充父级
        height: 80
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
    }
    
    // 操作按钮区域 (使用 anchors)
    ActionButtons {
        id: actionButtons
        width: parent.width // 宽度填充父级
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
            // 触发搜索按钮点击信号
            sidebarRoot.searchButtonClicked()
        }
        
        onCreateNewNoteClicked: {
            // 默认在当前选中文件夹或笔记的父文件夹下创建笔记
            if (noteTree.selectedNoteType === "folder") {
                // 如果选中的是文件夹，直接在该文件夹下创建
                noteTree.handleCreateNoteRequest(noteTree.selectedNotePath)
            } else if (noteTree.selectedNoteType === "note") {
                // 如果选中的是笔记，需要获取其父文件夹路径
                // 查找笔记的父文件夹
                for (var i = 0; i < noteTree.folderListModel.count; i++) {
                    var item = noteTree.folderListModel.get(i)
                    if (item.path === noteTree.selectedNotePath) {
                        // 找到笔记所在的父文件夹级别
                        var noteLevel = item.level
                        var parentIndex = -1
                        
                        // 向上查找父文件夹
                        for (var j = i - 1; j >= 0; j--) {
                            var potentialParent = noteTree.folderListModel.get(j)
                            if (potentialParent.level < noteLevel && potentialParent.type === "folder") {
                                parentIndex = j
                                break
                            }
                        }
                        
                        if (parentIndex >= 0) {
                            // 找到父文件夹
                            noteTree.handleCreateNoteRequest(noteTree.folderListModel.get(parentIndex).path)
                        } else {
                            // 未找到父文件夹，使用根目录
                            noteTree.handleCreateNoteRequest("/root")
                        }
                        break
                    }
                }
                
                if (i >= noteTree.folderListModel.count) {
                    // 未找到笔记，使用根目录
                    noteTree.handleCreateNoteRequest("/root")
                }
            } else {
                // 没有选中项，在根目录下创建
                noteTree.handleCreateNoteRequest("/root")
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
        Local.NoteTree {
            id: noteTree
            
            onNoteSelected: function(path, type) {
                // console.log("[Sidebar_Debug] Received noteSelected signal - Path:", path, "Type:", type); // 添加日志
                sidebarRoot.noteSelected(path, type)
            }
            
            onCreateFolder: function(parentPath, folderName) {
                sidebarRoot.createFolder(parentPath, folderName)
            }
            
            onCreateNote: function(parentPath, noteName) {
                sidebarRoot.createNote(parentPath, noteName)
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
    
    // QML函数：恢复默认视图
    function resetView() {
        currentView = "file"
        stackLayout.currentIndex = 0
        actionButtons.activeButton = "file" // 确保ActionButtons也更新
    }
}
