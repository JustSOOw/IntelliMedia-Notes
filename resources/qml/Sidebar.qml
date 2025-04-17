import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// 侧边栏主组件
Rectangle {
    id: sidebarRoot
    color: "transparent"
    
    // 当前视图模式：file, ai
    property string currentView: "file"
    
    // 信号
    signal noteSelected(string path, string type)
    signal createNote(string parentPath)
    signal createFolder(string parentPath)
    signal renameItem(string path, string newName)
    signal deleteItem(string path)
    signal sendAIMessage(string message)
    
    // 布局
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // 用户信息面板
        UserPanel {
            id: userPanel
            Layout.fillWidth: true
            Layout.preferredHeight: 80
        }
        
        // 操作按钮区域
        ActionButtons {
            id: actionButtons
            Layout.fillWidth: true
            Layout.preferredHeight: 120
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
        
        // 内容区域（使用堆叠布局切换文件树和AI视图）
        StackLayout {
            id: stackLayout
            Layout.fillWidth: true
            Layout.fillHeight: true
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
    }
    
    // 组件初始化完成后的处理
    Component.onCompleted: {
        console.log("侧边栏初始化完成")
    }
}
