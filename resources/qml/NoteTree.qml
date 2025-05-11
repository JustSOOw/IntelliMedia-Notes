import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt5Compat.GraphicalEffects
import QtQml 2.15

// 笔记文件树组件
Item {
    id: noteTree
    
    // 属性
    property string selectedNotePath: ""
    property string selectedNoteType: "" // folder 或 note
    property string selectedNoteName: "" // 当前选中的笔记或文件夹名称
    property int maxFolderLevel: 4
    property alias folderListModel: folderListModel // 导出列表模型以便外部访问
    
    // 主题相关颜色（扩展更多颜色定义）
    property color bgColor: sidebarManager.isDarkTheme ? "transparent" : "transparent"
    property color textColor: sidebarManager.isDarkTheme ? "#e0e0e0" : "#333333"
    property color folderTextColor: sidebarManager.isDarkTheme ? "#f2f2f2" : "#444444"
    property color noteTextColor: sidebarManager.isDarkTheme ? "#cccccc" : "#666666"
    property color hoverBgColor: sidebarManager.isDarkTheme ? "#454545" : "#f0f0f0"
    property color selectedBgColor: sidebarManager.isDarkTheme ? "#3a5a8c" : "#e3f2fd"
    property color selectedItemTextColor: sidebarManager.isDarkTheme ? "#ffffff" : "#1976D2"
    property color dialogBgColor: sidebarManager.isDarkTheme ? "#3a3a3a" : "#ffffff"
    property color dialogTextColor: sidebarManager.isDarkTheme ? "#e0e0e0" : "#333333"
    property color folderIconColor: sidebarManager.isDarkTheme ? "#80cbc4" : "#4285F4"
    property color noteIconColor: sidebarManager.isDarkTheme ? "#a5d6a7" : "#43a047"
    property color dividerColor: sidebarManager.isDarkTheme ? "#454545" : "#e0e0e0"
    property color expandIconColor: sidebarManager.isDarkTheme ? "#aaaaaa" : "#888888"
    property color menuIconColor: sidebarManager.isDarkTheme ? "#cccccc" : "#888888"
    property color itemBorderColor: sidebarManager.isDarkTheme ? "#505050" : "#e0e0e0"
    property color placeholderTextColor: sidebarManager.isDarkTheme ? "#808080" : "#aaaaaa"
    property color warningColor: sidebarManager.isDarkTheme ? "#ff6060" : "#c75450"
    property color scrollBarColor: sidebarManager.isDarkTheme ? "#606060" : "#bbbbbb"
    property color titleColor: sidebarManager.isDarkTheme ? "#80cbc4" : "#1976D2"
    
    // 信号
    signal noteSelected(string path, string type)
    signal createFolder(string parentPath, string folderName)
    signal createNote(string parentPath, string noteName)
    signal renameItem(string path, string newName)
    signal deleteItem(string path)
    signal createNewRequested(string parentPath) // 添加新信号
    
    // 监控 selectedNotePath 变化
    onSelectedNotePathChanged: {
        // console.log("[NoteTree_Debug] selectedNotePath CHANGED to:", selectedNotePath);
    }
    
    // 右键菜单
    FileContextMenu {
        id: contextMenu
    }
    
    // 添加 Connections 元素来连接 contextMenu 的信号
    Connections {
        target: contextMenu
        
        function onCreateFolderRequested(parentPath) {
            noteTree.handleCreateFolderRequest(parentPath)
        }
        function onCreateNoteRequested(parentPath) {
            noteTree.handleCreateNoteRequest(parentPath)
        }
        function onRenameRequested(path, name) {
            noteTree.handleRenameRequest(path, name)
        }
        function onDeleteRequested(path) {
            noteTree.handleDeleteRequest(path)
        }
    }
    
    // 自定义对话框实例
    CustomDialog {
        id: newFolderDialog
        title: "新建文件夹"
        message: "请输入文件夹名称："
        placeholder: "新建文件夹"
        showInput: true
        parent: noteTree // 明确指定父级
        
        property string parentPath: ""
        property int newFolderId: -1 // 存储新创建的文件夹ID
        
        onConfirmed: function(inputText) {
            if (inputText.trim() !== "") {
                // console.log("创建文件夹:", parentPath, inputText.trim())
                
                // 检查是否超过最大嵌套层级
                if (parentPath !== "/root") {
                    for (var i = 0; i < folderListModel.count; i++) {
                        var item = folderListModel.get(i)
                        if (item.path === parentPath) {
                            if (item.level >= noteTree.maxFolderLevel - 1) {
                                // console.error("文件夹嵌套层级已达上限:", noteTree.maxFolderLevel)
                                errorMessageDialog.message = "文件夹嵌套层级已达上限（" + noteTree.maxFolderLevel + "层）"
                                errorMessageDialog.open()
                                return
                            }
                            break
                        }
                    }
                }
                
                var result = sidebarManager.createFolder(parentPath, inputText.trim())
                newFolderId = result
                
                if (newFolderId > 0) {
                    // console.log("文件夹创建成功:", inputText.trim(), "ID:", newFolderId);
                    
                    var selectRetryCount = 0
                    var maxSelectRetries = 5
                    function selectNewFolder() {
                        if (selectRetryCount >= maxSelectRetries) {
                            // console.warn("选中新文件夹失败，已达最大重试次数")
                            return
                        }
                        selectRetryCount++
                        
                        var folderPath = "/folder_" + newFolderId
                        var found = false
                        var targetIndex = -1
                        for (var i = 0; i < folderListModel.count; i++) {
                            var item = folderListModel.get(i)
                            if (item.path === folderPath) {
                                found = true
                                targetIndex = i
                                // console.log("选中新创建的文件夹 (尝试 "+ selectRetryCount +"):", item.name, "路径:", folderPath)
                                selectedNotePath = folderPath
                                selectedNoteType = "folder"
                                selectedNoteName = item.name
                                listView.positionViewAtIndex(targetIndex, ListView.Center)
                                noteSelected(folderPath, "folder")
                                break
                            }
                        }
                        
                        if (!found) {
                            // console.log("未找到新创建的文件夹，稍后重试 (尝试 " + selectRetryCount + ")")
                            Qt.callLater(selectNewFolder)
                        }
                    }
                    
                    var parentIndex = -1
                    var parentLevel = -1
                    if (parentPath === "/root") {
                        parentIndex = -1
                        parentLevel = -1
                    } else {
                        for (var i = 0; i < folderListModel.count; i++) {
                            if (folderListModel.get(i).path === parentPath) {
                                parentIndex = i
                                parentLevel = folderListModel.get(i).level
                                break
                            }
                        }
                    }
                    
                    if (parentPath !== "/root" && parentIndex === -1) {
                        // console.error("未能找到父文件夹用于插入新项目")
                        return
                    }
                    
                    if (parentIndex !== -1 && !folderListModel.get(parentIndex).expanded) {
                        // console.log("展开父文件夹以插入新项")
                        toggleFolderExpanded(parentIndex)
                    }
                    
                    var newItemLevel = parentLevel + 1;
                    var newItemData = {
                        "id": newFolderId,
                        "name": inputText.trim(),
                        "type": "folder",
                        "level": newItemLevel,
                        "expanded": false,
                        "path": "/folder_" + newFolderId
                    }
                    // console.log("构造新文件夹数据:", JSON.stringify(newItemData));
                    
                    var insertIndex = -1;
                    if (parentIndex === -1) {
                        // 如果是根目录，将文件夹放在所有文件夹后面
                        insertIndex = 0;
                        // 找到所有最上层文件夹的最后一个位置
                        while (insertIndex < folderListModel.count && 
                               (folderListModel.get(insertIndex).type === 'folder' && 
                                folderListModel.get(insertIndex).level === 0)) {
                            insertIndex++;
                        }
                    } else { 
                        insertIndex = parentIndex + 1;
                        while (insertIndex < folderListModel.count && folderListModel.get(insertIndex).level > parentLevel) {
                            insertIndex++;
                        }
                    }
                    
                    // console.log("在索引 " + insertIndex + " 处插入新文件夹")
                    folderListModel.insert(insertIndex, newItemData)
                    
                    Qt.callLater(selectNewFolder)
                    
                } else {
                    // console.error("文件夹创建失败:", inputText.trim());
                    errorMessageDialog.message = "创建文件夹失败：" + inputText.trim()
                    errorMessageDialog.open()
                }
            }
        }
    }
    
    CustomDialog {
        id: newNoteDialog
        title: "新建笔记"
        message: "请输入笔记名称："
        placeholder: "新建笔记"
        showInput: true
        parent: noteTree // 明确指定父级
        
        property string parentPath: ""
        property int newNoteId: -1 // 存储新创建的笔记ID
        
        onConfirmed: function(inputText) {
            if (inputText.trim() !== "") {
                // console.log("创建笔记:", parentPath, inputText.trim())
                
                if (parentPath !== "/root") {
                    for (var i = 0; i < folderListModel.count; i++) {
                        var item = folderListModel.get(i)
                        if (item.path === parentPath) {
                            if (item.level >= noteTree.maxFolderLevel - 1) {
                                // console.error("文件夹嵌套层级已达上限:", noteTree.maxFolderLevel)
                                errorMessageDialog.message = "文件夹嵌套层级已达上限（" + noteTree.maxFolderLevel + "层）"
                                errorMessageDialog.open()
                                return
                            }
                            break
                        }
                    }
                }
                
                var result = sidebarManager.createNote(parentPath, inputText.trim())
                newNoteId = result
                
                if (newNoteId > 0) {
                    // console.log("笔记创建成功:", inputText.trim(), "ID:", newNoteId);
                    
                    var selectRetryCount = 0
                    var maxSelectRetries = 5
                    function selectNewNote() {
                         if (selectRetryCount >= maxSelectRetries) {
                            // console.warn("选中新笔记失败，已达最大重试次数")
                            return
                        }
                        selectRetryCount++
                        
                        var notePath = "/note_" + newNoteId
                        var found = false
                        var targetIndex = -1
                        for (var i = 0; i < folderListModel.count; i++) {
                            var item = folderListModel.get(i)
                            if (item.path === notePath) {
                                found = true
                                targetIndex = i
                                // console.log("选中新创建的笔记 (尝试 "+ selectRetryCount +"):", item.name, "路径:", notePath)
                                selectedNotePath = notePath
                                selectedNoteType = "note"
                                selectedNoteName = item.name
                                listView.positionViewAtIndex(targetIndex, ListView.Center)
                                noteSelected(notePath, "note")
                                break
                            }
                        }
                        
                        if (!found) {
                            // console.log("未找到新创建的笔记，稍后重试 (尝试 " + selectRetryCount + ")")
                            Qt.callLater(selectNewNote)
                        }
                    }
                    
                    var parentIndex = -1
                    var parentLevel = -1
                    if (parentPath === "/root") {
                        parentIndex = -1
                        parentLevel = -1
                    } else {
                        for (var i = 0; i < folderListModel.count; i++) {
                            if (folderListModel.get(i).path === parentPath) {
                                parentIndex = i
                                parentLevel = folderListModel.get(i).level
                                break
                            }
                        }
                    }
                    
                    if (parentPath !== "/root" && parentIndex === -1) {
                        // console.error("未能找到父文件夹用于插入新项目")
                        return
                    }
                    
                     if (parentIndex !== -1 && !folderListModel.get(parentIndex).expanded) {
                        // console.log("展开父文件夹以插入新项")
                        toggleFolderExpanded(parentIndex)
                    }
                    
                    var newItemLevel = parentLevel + 1;
                    var newItemData = {
                        "id": newNoteId,
                        "name": inputText.trim(),
                        "type": "note",
                        "level": newItemLevel,
                        "path": "/note_" + newNoteId,
                        "date": new Date().toISOString() // 模拟日期
                    }
                    // console.log("构造新笔记数据:", JSON.stringify(newItemData));
                    
                    var insertIndex = -1;
                    if (parentIndex === -1) { 
                        // 如果是根目录，将笔记放在所有文件夹后面
                        insertIndex = 0;
                        // 找到所有最上层文件夹的最后一个位置
                        while (insertIndex < folderListModel.count && 
                               (folderListModel.get(insertIndex).type === 'folder' && 
                                folderListModel.get(insertIndex).level === 0)) {
                            insertIndex++;
                        }
                    } else { 
                        insertIndex = parentIndex + 1;
                        while (insertIndex < folderListModel.count && folderListModel.get(insertIndex).level > parentLevel) {
                            insertIndex++;
                        }
                    }
                    
                    // console.log("在索引 " + insertIndex + " 处插入新笔记")
                    folderListModel.insert(insertIndex, newItemData)
                    
                    Qt.callLater(selectNewNote)
                    
                } else {
                    // console.error("笔记创建失败:", inputText.trim());
                    errorMessageDialog.message = "创建笔记失败：" + inputText.trim()
                    errorMessageDialog.open()
                }
            }
        }
    }
    
    CustomDialog {
        id: renameDialog
        title: "重命名"
        message: "请输入新名称："
        showInput: true
        parent: noteTree // 明确指定父级
        
        property string itemPath: ""
        
        onConfirmed: function(inputText) {
            if (inputText.trim() !== "") {
                // console.log("重命名项目:", itemPath, "为", inputText.trim());
                if (sidebarManager.renameItem(itemPath, inputText.trim())) {
                    // console.log("项目重命名成功:", inputText.trim());
                    // 刷新在Connections的onFolderStructureChanged中处理
                } else {
                    // console.error("项目重命名失败:", inputText.trim());
                    errorMessageDialog.message = "重命名失败：" + inputText.trim()
                    errorMessageDialog.open()
                }
            }
        }
    }
    
    CustomDialog {
        id: deleteDialog
        title: "确认删除"
        message: "确定要删除该项目吗？此操作不可恢复。"
        confirmText: "删除"
        cancelText: "取消"
        isWarning: true
        parent: noteTree // 明确指定父级
        
        property string itemPath: ""
        
        onConfirmed: function(inputText) {
            // console.log("删除项目:", itemPath)
            if (sidebarManager.deleteItem(itemPath)) {
                // console.log("项目删除成功:", itemPath);
                // 刷新在Connections的onFolderStructureChanged中处理
            } else {
                // console.error("项目删除失败:", itemPath);
                errorMessageDialog.message = "删除失败：" + itemPath
                errorMessageDialog.open()
            }
        }
    }
    
    // 错误消息对话框
    CustomDialog {
        id: errorMessageDialog
        title: "错误"
        message: ""
        showInput: false
        confirmText: "确定"
        cancelVisible: false
        isWarning: true
        parent: noteTree
    }
    
    // 主容器
    Rectangle {
        id: containerRect
        anchors.fill: parent
        color: bgColor
        
        // 处理顶部空白区域点击的MouseArea
        MouseArea {
            id: topEmptyAreaMouseArea // 给一个明确的ID
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: listAreaContainer.top // 确保只覆盖顶部区域
            acceptedButtons: Qt.RightButton | Qt.LeftButton
            
            onClicked: function(mouse) {
                // console.log("[NoteTree_Debug] 红色区域点击 - Button:", mouse.button);
                if (mouse.button === Qt.RightButton) {
                    // 右键点击空白区域
                    // console.log("[NoteTree_Debug] 红色区域右键点击，打开根目录菜单");
                    contextMenu.isFolder = true
                    contextMenu.itemPath = "/root"
                    contextMenu.itemName = "根目录"
                    contextMenu.popup()
                } else if (mouse.button === Qt.LeftButton) {
                    // 左键点击空白区域，取消选中
                    // console.log("[NoteTree_Debug] 红色区域左键点击，当前选中:", selectedNotePath);
                    if (selectedNotePath !== "") {
                        // console.log("[NoteTree_Debug] 红色区域点击，取消选中:", selectedNotePath);
                        // 先发出信号通知外部，然后再清空本地状态
                        // console.log("[NoteTree_Debug] 发送取消选中信号");
                        noteSelected("", "")
                        // console.log("[NoteTree_Debug] 清除本地选中状态");
                        selectedNotePath = ""
                        selectedNoteType = ""
                        selectedNoteName = ""
                    } else {
                        // console.log("[NoteTree_Debug] 当前无选中项目，无需取消");
                    }
                }
            }
        }
        
        // 标题部分
        Rectangle {
            id: headerRect
            width: parent.width
            height: 50
            color: sidebarManager.isDarkTheme ? "#353535" : "#ffffff"
            radius: 8
            
            // 标题文本
            Text {
                id: headerText
                text: qsTr("全部笔记")
                font.pixelSize: 16
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 15
                color: titleColor
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
                    
                    ColorOverlay {
                        anchors.fill: parent
                        source: parent
                        color: expandIconColor
                    }
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
        
        // 文件列表区域
        Item {
            id: listAreaContainer
            anchors.top: headerRect.bottom
            anchors.topMargin: 10
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            
            // 底层背景 MouseArea，用于捕获空白区域点击
            Rectangle {
                id: listBackgroundRect
                anchors.fill: parent
                color: "transparent" // 恢复透明
                // opacity: 0.3 // 移除透明度
                z: 0  // 基准层
                
                MouseArea {
                    id: emptyAreaMouseArea
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    // z: 100 // 移除高z值
                    
                    enabled: false // 禁用这个MouseArea，因为header/footer已处理空白点击
                    // preventStealing: true // 移除
                    
                    // 移除 isPointInItemCard 函数
                    
                    // 移除 onPressed
                    
                    // 移除 onClicked
                }
            }
            
            // 文件列表
            ListView {
                id: listView
                anchors.fill: parent
                z: 1 // 置于背景之上，但不应该阻挡emptyAreaMouseArea
            
                ScrollBar.vertical: ScrollBar {
                    active: true
                    policy: ScrollBar.AsNeeded
                    contentItem: Rectangle {
                        implicitWidth: 6
                        implicitHeight: 100
                        radius: width / 2
                        color: scrollBarColor
                    }
                }
            
                // 使用C++提供的数据代替静态数据
                model: folderListModel
            
                // 添加空白分隔区域，用于点击取消选中
                header: Rectangle {
                    width: parent.width
                    height: 10
                    color: "transparent" // 恢复透明
                    // opacity: 0.3
                    
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            // console.log("[NoteTree_Debug] 列表头部空白区域点击 - 取消选中");
                            if (selectedNotePath !== "") {
                                noteSelected("", "")
                                selectedNotePath = ""
                                selectedNoteType = ""
                                selectedNoteName = ""
                            }
                        }
                    }
                }
                
                // 列表尾部空白区域
                footer: Rectangle {
                    width: parent.width
                    // 动态计算footer高度，填充剩余可见空间，避免绑定循环
                    property int itemTotalHeight: folderListModel.count * (44 + 8) // 44是项目高度, 8是间距
                    height: Math.max(10, listAreaContainer.height - itemTotalHeight - 10) 
                    color: "transparent" // 恢复透明
                    // opacity: 0.3
                    
                    // 移除调试日志
                    // Component.onCompleted: {
                    //     console.log("[NoteTree_Debug] Footer Initial Height:", height)
                    // }
                    // onHeightChanged: {
                    //     console.log("[NoteTree_Debug] Footer Height Changed:", height)
                    // }
                    
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            // console.log("[NoteTree_Debug] 列表尾部空白区域点击 - 取消选中");
                            if (selectedNotePath !== "") {
                                noteSelected("", "")
                                selectedNotePath = ""
                                selectedNoteType = ""
                                selectedNoteName = ""
                            }
                        }
                    }
                }
                
                // 使列表背景透明，使背景的MouseArea能接收点击
                // 这是关键 - 使用透明delegate背景，但项目本身不透明
                highlight: Item { } // 禁用默认高亮，我们使用自定义高亮
                
                // 为每个项目添加间距
                spacing: 8
                clip: false // 不裁剪，让阴影能够溢出
                
                // 优化ListView性能
                cacheBuffer: 300 // 增加缓存，提高滚动性能
                reuseItems: true // 重用项目
                highlightFollowsCurrentItem: false // 禁用默认高亮跟随，使用我们自己的高亮逻辑
                
                // 视图行为设置
                interactive: true // 允许交互
                flickableDirection: Flickable.VerticalFlick // 只允许垂直滚动
                boundsBehavior: Flickable.StopAtBounds // 到达边界时停止
                
                delegate: Item {
                    id: itemContainer
                    width: ListView.view.width - 20 
                    height: 44
                    x: 10
                    // 确保delegate只在有内容的地方捕获点击，而不是整个区域
                    clip: false
                    
                DropShadow {
                    anchors.fill: itemCard
                    horizontalOffset: 0
                    verticalOffset: 2
                    radius: itemMouseArea.containsMouse || noteTree.selectedNotePath === model.path ? 8.0 : 4.0
                    samples: 17
                    color: sidebarManager.isDarkTheme ? "#40000000" : "#20000000"
                    source: itemCard
                    visible: true // 始终显示阴影，但强度不同
                    
                    // 平滑动画
                    Behavior on radius { NumberAnimation { duration: 150 } }
                }
                
                Rectangle {
                    id: itemCard
                    anchors.fill: parent
                    radius: 8
                    color: {
                        if (noteTree.selectedNotePath === model.path) {
                            return selectedBgColor;
                        } else if (itemMouseArea.containsMouse) {
                            return hoverBgColor;
                        } else {
                            return sidebarManager.isDarkTheme ? "#353535" : "#ffffff";
                        }
                    }
                    border.width: 1
                    border.color: itemMouseArea.containsMouse || noteTree.selectedNotePath === model.path ? 
                                 (sidebarManager.isDarkTheme ? "#607080" : "#4285F4") : 
                                 itemBorderColor
                    
                    // 指示条 (选中时)
                    Rectangle {
                        id: selectionIndicator
                        width: 4
                        height: parent.height - 8
                        radius: 2
                        anchors.left: parent.left
                        anchors.leftMargin: 4
                        anchors.verticalCenter: parent.verticalCenter
                        color: sidebarManager.isDarkTheme ? "#80cbc4" : "#4285F4"
                        visible: noteTree.selectedNotePath === model.path
                        
                        // 淡入淡出
                        Behavior on opacity { NumberAnimation { duration: 150 } }
                    }
                    
                    // 内容布局
                    RowLayout {
                        anchors.fill: parent
                            anchors.leftMargin: 16 + Math.min(model.level, 3) * 20 // 限制最大缩进
                        anchors.rightMargin: 16
                        spacing: 12
                            
                            // 层级指示器
                            Rectangle {
                                id: levelIndicator
                                Layout.preferredWidth: model.level > 3 ? 4 : 0
                                Layout.preferredHeight: 24
                                color: sidebarManager.isDarkTheme ? "#606060" : "#888888"
                                radius: 2
                                visible: model.level > 3
                                
                                // 显示深层级文本提示
                                ToolTip {
                                    text: "深层级文件夹 (Level " + model.level + ")"
                                    visible: levelMouseArea.containsMouse
                                    delay: 500
                                }
                                
                                MouseArea {
                                    id: levelMouseArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                }
                            }
                        
                        // 展开/折叠图标 (仅文件夹显示)
                        Image {
                            id: folderExpandIcon
                                Layout.preferredWidth: 18
                                Layout.preferredHeight: 18
                            source: "qrc:/icons/round_right_fill.svg"
                            visible: model.type === "folder"
                            opacity: model.type === "folder" ? 1.0 : 0.0
                            rotation: model.expanded ? 90 : 0
                            Layout.alignment: Qt.AlignVCenter
                            
                                ColorOverlay {
                                    anchors.fill: parent
                                    source: parent
                                    color: expandIconColor
                                }
                                
                            Behavior on rotation { NumberAnimation { duration: 200; easing.type: Easing.OutQuad } }
                            
                            MouseArea {
                                anchors.fill: parent
                                    anchors.margins: -10 // 增大点击区域
                                    propagateComposedEvents: false // 阻止事件传播
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    
                                onClicked: {
                                        // console.log("[NoteTree] 点击了展开/折叠按钮")
                                    toggleFolderExpanded(index)
                                        mouse.accepted = true 
                                    }
                                    
                                    onContainsMouseChanged: {
                                        parent.opacity = containsMouse ? 0.7 : 1.0
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
                                color: {
                                    if (noteTree.selectedNotePath === model.path) {
                                        // 选中状态，使用高亮颜色
                                        return sidebarManager.isDarkTheme ? "#ffffff" : "#4285F4";
                                    } else {
                                        // 普通状态，使用类型颜色
                                        return model.type === "folder" ? folderIconColor : noteIconColor;
                                    }
                                }
                            }
                        }
                        
                        // 名称
                        Label {
                            id: nameLabel
                            text: model.name
                            font.pixelSize: 14
                            font.weight: noteTree.selectedNotePath === model.path ? 
                                       Font.DemiBold : Font.Normal // 选中时加粗
                                elide: Text.ElideMiddle // 从中间省略，而不是右侧
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                            color: {
                                if (noteTree.selectedNotePath === model.path) {
                                    return selectedItemTextColor;
                                } else {
                                    return model.type === "folder" ? folderTextColor : noteTextColor;
                                }
                            }
                            
                                ToolTip {
                                    text: model.name
                                    visible: nameMouseArea.containsMouse && nameLabel.truncated
                                    delay: 500
                                }
                                
                                MouseArea {
                                    id: nameMouseArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    propagateComposedEvents: true // 允许事件传播
                                    onPressed: function(mouse) { mouse.accepted = false; } // 不处理点击，传递给父级
                                }
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
                                color: "#FFD700" // 金色，不随主题变化
                            }
                        }
                    }
                }
                
                MouseArea {
                    id: itemMouseArea
                        anchors.fill: itemCard // 只覆盖卡片区域而不是整个delegate
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    cursorShape: Qt.PointingHandCursor
                    
                        // Component.onCompleted: {
                        //     // 打印每个项目的位置信息，用于调试
                        //     var pos = mapToItem(listView, 0, 0);
                        //     console.log("[NoteTree_Debug] Item Position - Path:", model.path, 
                        //                "Index:", index, 
                        //                "ListView位置 X:", pos.x, 
                        //                "Y:", pos.y);
                        // }
                    
                    onClicked: function(mouse) {
                            // console.log("[NoteTree_Debug] Item Clicked - Path:", model.path, "Type:", model.type, "Button:", mouse.button); // 添加日志
                        if (mouse.button === Qt.LeftButton) {
                                // console.log("[NoteTree_Debug] Selecting item - Path:", model.path, "Type:", model.type, "Name:", model.name); // 添加日志
                                selectedNotePath = model.path
                                selectedNoteType = model.type
                                selectedNoteName = model.name
                                // console.log("[NoteTree_Debug] Emitting noteSelected signal for item:", model.path); // 添加日志
                            noteSelected(model.path, model.type)
                            
                                if (model.type === "folder") {
                                    // console.log("[NoteTree] 单击文件夹，自动展开/折叠:", model.name)
                                    // console.log("[NoteTree_Debug] Toggling folder expansion for:", model.name); // 添加日志
                                toggleFolderExpanded(index)
                            }
                        } else if (mouse.button === Qt.RightButton) {
                                // console.log("[NoteTree_Debug] Item Right-Clicked. Opening context menu for:", model.path); // 添加日志
                            contextMenu.isFolder = model.type === "folder"
                            contextMenu.itemPath = model.path
                            contextMenu.itemName = model.name
                            contextMenu.popup()
                        }
                    }
                        
                        onDoubleClicked: function(mouse) {
                            if (mouse.button === Qt.LeftButton && model.type === "note") {
                                // console.log("[NoteTree] 双击打开笔记:", model.name)
                                noteSelected(model.path, model.type)
                            }
                        }
                    }
                    
                Behavior on scale { 
                        NumberAnimation { duration: 100; easing.type: Easing.OutQuad }
                }
            }
        }
    }
    } 
    ListModel {
        id: folderListModel
    }
    
    Component.onCompleted: {
        // console.log("[NoteTree] 组件初始化开始")
        refreshNotesList()
    }
    
    function refreshNotesList() {
        // console.log("--- [NoteTree] refreshNotesList() CALLED --- "); 
        var currentSelectedPath = selectedNotePath;
        folderListModel.clear();
        var topLevelItems = sidebarManager.getFolderStructure();
        // console.log("[NoteTree] Fetched top level items count: " + topLevelItems.length);
        for (var i = 0; i < topLevelItems.length; i++) {
            folderListModel.append(topLevelItems[i]);
        }
        // console.log("[NoteTree] refreshNotesList() finished. Total items: " + folderListModel.count);
        if (currentSelectedPath) {
            selectedNotePath = currentSelectedPath;
        }
        listView.forceLayout();
    }
    
    function toggleFolderExpanded(index) {
        if (index < 0 || index >= folderListModel.count) return
        var folder = folderListModel.get(index)
        if (folder.type !== "folder") return
        // console.log("--- [NoteTree] toggleFolderExpanded() CALLED for:", folder.name, "Current expanded:", folder.expanded, "Target state:", !folder.expanded);
        
        folder.expanded = !folder.expanded
        
        if (folder.expanded) {
            var hasChildren = false
            if (index + 1 < folderListModel.count) {
                var nextItem = folderListModel.get(index + 1)
                if (nextItem.level > folder.level) {
                    hasChildren = true
                    // console.log("[NoteTree] Folder already has children in model, skipping add.")
                }
            }
            if (!hasChildren) {
                var contents = sidebarManager.getFolderContents(folder.id, folder.level)
                // console.log("[NoteTree] Fetched children count for expand: " + contents.length)
                var insertPosition = index + 1
                for (var i = 0; i < contents.length; i++) {
                    folderListModel.insert(insertPosition, contents[i])
                    insertPosition++
                }
            }
        } else {
            // console.log("--- [NoteTree] Calling removeChildItems() for index:", index);
            removeChildItems(index)
        }
        listView.forceLayout()
    }
    
    function removeChildItems(parentIndex) {
        if (parentIndex < 0 || parentIndex >= folderListModel.count) return
        var parentItem = folderListModel.get(parentIndex)
        if (parentItem.type !== "folder") return
        var parentLevel = parentItem.level
        // console.log("--- [NoteTree] removeChildItems() CALLED for:", parentItem.name);
        var startIndex = parentIndex + 1
        var endIndex = startIndex
        while (endIndex < folderListModel.count && folderListModel.get(endIndex).level > parentLevel) {
            endIndex++
        }
        if (endIndex > startIndex) {
            var removeCount = endIndex - startIndex
            // console.log("[NoteTree] Removing " + removeCount + " child items from index " + startIndex)
            for (var i = endIndex - 1; i >= startIndex; i--) {
                 folderListModel.remove(i)
            }
        } else {
            // console.log("[NoteTree] No child items found to remove.")
        }
        listView.forceLayout();
    }
    
    Connections {
        target: sidebarManager
        function onFolderStructureChanged() {
            // console.log("--- [NoteTree] onFolderStructureChanged SIGNAL RECEIVED --- Calling refreshNotesList()");
            refreshNotesList();
        }
    }
    
    function handleCreateFolderRequest(parentPath) {
        // console.log("[NoteTree] Handling Create Folder Request for:", parentPath)
        if (parentPath !== "/root") {
            ensureFolderExpanded(parentPath)
        }
        newFolderDialog.parentPath = parentPath
        newFolderDialog.inputText = ""
        // console.log("[NoteTree] Opening New Folder Dialog...")
        newFolderDialog.open()
    }
    
    function handleCreateNoteRequest(parentPath) {
        // console.log("[NoteTree] Handling Create Note Request for:", parentPath)
        
        // 验证 parentPath 是否有效
        var isValidPath = false
        if (parentPath === "/root") {
            isValidPath = true
        } else {
            for (var i = 0; i < folderListModel.count; i++) {
                var item = folderListModel.get(i)
                if (item.path === parentPath && item.type === "folder") {
                    isValidPath = true
                    break
                }
            }
        }
        
        // 如果路径无效（可能是笔记路径），则使用根目录
        if (!isValidPath) {
            console.log("[NoteTree] 无效的父路径，将使用根目录:", parentPath)
            parentPath = "/root"
        }
        
        if (parentPath !== "/root") {
            ensureFolderExpanded(parentPath)
        }
        
        newNoteDialog.parentPath = parentPath
        newNoteDialog.inputText = ""
        // console.log("[NoteTree] Opening New Note Dialog...")
        newNoteDialog.open()
    }
    
    function ensureFolderExpanded(folderPath) {
        for (var i = 0; i < folderListModel.count; i++) {
            var item = folderListModel.get(i)
            if (item.path === folderPath && item.type === "folder") {
                // console.log("[NoteTree] 确保文件夹展开:", item.name)
                if (!item.expanded) {
                    toggleFolderExpanded(i)
                }
                return true
            }
        }
        // console.log("[NoteTree] 未找到要展开的文件夹:", folderPath)
        return false
    }
    
    function handleRenameRequest(path, name) {
        // console.log("[NoteTree] Handling Rename Request for:", path, "Name:", name)
        renameDialog.itemPath = path
        renameDialog.inputText = name
        // console.log("[NoteTree] Opening Rename Dialog...")
        renameDialog.open()
    }
    
    function handleDeleteRequest(path) {
        // console.log("[NoteTree] Handling Delete Request for:", path)
        deleteDialog.itemPath = path
        // console.log("[NoteTree] Opening Delete Dialog...")
        deleteDialog.open()
    }
    
    // 监听主题变化
    Connections {
        target: sidebarManager
        function onThemeChanged() {
            // 属性会自动更新，无需额外操作
            console.log("NoteTree: Theme changed to", sidebarManager.isDarkTheme ? "dark" : "light")
        }
    }
    
    // 编辑控件（仅用于编辑模式）
    TextField {
        id: inlineEditField
        visible: false
        // 其余属性在显示编辑框时动态设置
        font.pixelSize: 14
        selectByMouse: true
        
        onEditingFinished: {
            if (visible) {
                // 处理编辑完成逻辑
                if (text.trim() !== "") {
                    var targetPath = editingItemPath
                    var newName = text.trim()
                    // console.log("提交重命名:", targetPath, "--> 新名称:", newName)
                    noteTree.renameItem(targetPath, newName) // 触发重命名信号
                }
                visible = false // 隐藏编辑框
            }
        }
        
        // 使用Keys而不是Key (QML 2.15+)
        Keys.onPressed: function(event) {
            if (event.key === Qt.Key_Escape) {
                visible = false // 取消编辑
                event.accepted = true
            }
        }
        
        property string editingItemPath: ""
    }
    
    // 函数：根据路径选中项目
    function selectItemByPath(path) {
        console.log("尝试选中项目，路径:", path)
        
        // 遍历模型查找匹配项
        for (var i = 0; i < folderListModel.count; i++) {
            var item = folderListModel.get(i)
            if (item.path === path) {
                // 找到匹配项
                console.log("找到匹配项，索引:", i, "名称:", item.name, "类型:", item.type)
                
                // 设置选中状态
                selectedNotePath = path
                selectedNoteType = item.type
                selectedNoteName = item.name
                
                // 确保文件夹路径展开以显示此项
                ensurePathExpanded(path)
                
                // 滚动到选中项
                Qt.callLater(function() {
                    listView.positionViewAtIndex(i, ListView.Center)
                    
                    // 触发选中信号
                    noteSelected(path, item.type)
                })
                
                return true
            }
        }
        
        console.log("未找到匹配路径:", path)
        return false
    }
    
    // 函数：确保路径展开（用于显示某个深层次的项目）
    function ensurePathExpanded(path) {
        // 对于笔记，需要找到其所在的文件夹
        var folderPath = path
        if (path.indexOf("/note_") !== -1) {
            // 尝试找到此笔记的父文件夹
            for (var i = 0; i < folderListModel.count; i++) {
                var item = folderListModel.get(i)
                if (item.path === path) {
                    // 找到笔记所在的级别
                    var noteLevel = item.level
                    
                    // 向上查找一个级别的文件夹
                    for (var j = i - 1; j >= 0; j--) {
                        var potentialParent = folderListModel.get(j)
                        if (potentialParent.level < noteLevel && potentialParent.type === "folder") {
                            folderPath = potentialParent.path
                            break
                        }
                    }
                    break
                }
            }
        }
        
        // 现在展开从根到所需文件夹的所有文件夹
        if (folderPath.indexOf("/folder_") !== -1) {
            // 获取文件夹ID
            var folderId = parseInt(folderPath.split("_")[1])
            
            // 获取所有父文件夹ID的列表
            var parentIds = []
            var currentFolderId = folderId
            
            while (currentFolderId > 0) {
                // 遍历查找当前文件夹
                for (var i = 0; i < folderListModel.count; i++) {
                    var folder = folderListModel.get(i)
                    if (folder.type === "folder" && folder.id === currentFolderId) {
                        // 确保这个文件夹是展开的
                        if (!folder.expanded) {
                            toggleFolderExpanded(i) // 展开文件夹
                        }
                        
                        // 寻找父文件夹
                        if (folder.level > 0) {
                            // 向上查找一个级别的文件夹
                            for (var j = i - 1; j >= 0; j--) {
                                var potentialParent = folderListModel.get(j)
                                if (potentialParent.level < folder.level && potentialParent.type === "folder") {
                                    currentFolderId = potentialParent.id
                                    break
                                }
                            }
                            // 如果找不到父文件夹，结束循环
                            if (j < 0) {
                                currentFolderId = 0
                            }
                        } else {
                            // 如果是顶级文件夹，结束循环
                            currentFolderId = 0
                        }
                        break
                    }
                }
                // 如果遍历完未找到当前文件夹，结束循环
                if (i >= folderListModel.count) {
                    break
                }
            }
        }
    }
}

