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
    
    // 信号
    signal noteSelected(string path, string type)
    signal createFolder(string parentPath, string folderName)
    signal createNote(string parentPath, string noteName)
    signal renameItem(string path, string newName)
    signal deleteItem(string path)
    
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
                console.log("创建文件夹:", parentPath, inputText.trim())
                
                // 检查是否超过最大嵌套层级
                if (parentPath !== "/root") {
                    // 找到父文件夹
                    for (var i = 0; i < folderListModel.count; i++) {
                        var item = folderListModel.get(i)
                        if (item.path === parentPath) {
                            // 检查层级
                            if (item.level >= noteTree.maxFolderLevel - 1) {
                                console.error("文件夹嵌套层级已达上限:", noteTree.maxFolderLevel)
                                // 显示提示
                                errorMessageDialog.message = "文件夹嵌套层级已达上限（" + noteTree.maxFolderLevel + "层）"
                                errorMessageDialog.open()
                                return
                            }
                            break
                        }
                    }
                }
                
                // 继续创建文件夹
                var result = sidebarManager.createFolder(parentPath, inputText.trim())
                newFolderId = result
                
                if (newFolderId > 0) {
                    console.log("文件夹创建成功:", inputText.trim(), "ID:", newFolderId);
                    
                    // 定义选中函数 (添加重试次数限制)
                    var selectRetryCount = 0
                    var maxSelectRetries = 5 // 最多重试5次
                    function selectNewFolder() {
                        if (selectRetryCount >= maxSelectRetries) {
                            console.warn("选中新文件夹失败，已达最大重试次数")
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
                                console.log("选中新创建的文件夹 (尝试 "+ selectRetryCount +"):", item.name, "路径:", folderPath)
                                selectedNotePath = folderPath
                                selectedNoteType = "folder"
                                selectedNoteName = item.name
                                listView.positionViewAtIndex(targetIndex, ListView.Center)
                                noteSelected(folderPath, "folder")
                                break
                            }
                        }
                        
                        if (!found) {
                            console.log("未找到新创建的文件夹，稍后重试 (尝试 " + selectRetryCount + ")")
                            Qt.callLater(selectNewFolder) // 使用 Qt.callLater 进行下一次尝试
                        }
                    }
                    
                    // 局部插入逻辑
                    var parentIndex = -1
                    var parentLevel = -1
                    if (parentPath === "/root") {
                        parentIndex = -1 // 表示根目录
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
                        console.error("未能找到父文件夹用于插入新项目")
                        return
                    }
                    
                    // 确保父文件夹展开 (如果是子文件夹)
                    if (parentIndex !== -1 && !folderListModel.get(parentIndex).expanded) {
                        console.log("展开父文件夹以插入新项")
                        toggleFolderExpanded(parentIndex) // 展开会加载内容，但不包括我们刚创建的
                    }
                    
                    // *** 获取新创建项的数据 (需要新的C++方法) ***
                    // 假设 sidebarManager.getFolderInfo(newFolderId) 返回 QVariantMap
                    // 需要先实现 getFolderInfo
                    // var newItemData = sidebarManager.getFolderInfo(newFolderId)
                    
                    // *** 临时代替：手动构造数据，level 需要正确计算 ***
                    var newItemLevel = parentLevel + 1;
                    var newItemData = {
                        "id": newFolderId,
                        "name": inputText.trim(),
                        "type": "folder",
                        "level": newItemLevel,
                        "expanded": false,
                        "path": "/folder_" + newFolderId
                    }
                    console.log("构造新文件夹数据:", JSON.stringify(newItemData));
                    
                    // 确定插入位置
                    var insertIndex = -1;
                    if (parentIndex === -1) { // 插入到根目录
                        // 找到第一个非文件夹项或列表末尾
                        insertIndex = 0;
                        while (insertIndex < folderListModel.count && folderListModel.get(insertIndex).type === 'folder') {
                            insertIndex++;
                        }
                    } else { // 插入到子文件夹
                        insertIndex = parentIndex + 1;
                        // 跳过所有子孙节点
                        while (insertIndex < folderListModel.count && folderListModel.get(insertIndex).level > parentLevel) {
                            insertIndex++;
                        }
                    }
                    
                    // 执行插入
                    console.log("在索引 " + insertIndex + " 处插入新文件夹")
                    folderListModel.insert(insertIndex, newItemData)
                    
                    // 延迟执行选中
                    Qt.callLater(selectNewFolder)
                    
                } else {
                    console.error("文件夹创建失败:", inputText.trim());
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
                console.log("创建笔记:", parentPath, inputText.trim())
                
                // 检查是否超过最大嵌套层级
                if (parentPath !== "/root") {
                    // 找到父文件夹
                    for (var i = 0; i < folderListModel.count; i++) {
                        var item = folderListModel.get(i)
                        if (item.path === parentPath) {
                            // 检查层级
                            if (item.level >= noteTree.maxFolderLevel - 1) {
                                console.error("文件夹嵌套层级已达上限:", noteTree.maxFolderLevel)
                                // 显示提示
                                errorMessageDialog.message = "文件夹嵌套层级已达上限（" + noteTree.maxFolderLevel + "层）"
                                errorMessageDialog.open()
                                return
                            }
                            break
                        }
                    }
                }
                
                // 继续创建笔记
                var result = sidebarManager.createNote(parentPath, inputText.trim())
                newNoteId = result
                
                if (newNoteId > 0) {
                    console.log("笔记创建成功:", inputText.trim(), "ID:", newNoteId);
                    
                    // *** 移除全局刷新 ***
                    
                    // 定义选中函数 (添加重试次数限制)
                    var selectRetryCount = 0
                    var maxSelectRetries = 5
                    function selectNewNote() {
                         if (selectRetryCount >= maxSelectRetries) {
                            console.warn("选中新笔记失败，已达最大重试次数")
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
                                console.log("选中新创建的笔记 (尝试 "+ selectRetryCount +"):", item.name, "路径:", notePath)
                                selectedNotePath = notePath
                                selectedNoteType = "note"
                                selectedNoteName = item.name
                                listView.positionViewAtIndex(targetIndex, ListView.Center)
                                noteSelected(notePath, "note")
                                break
                            }
                        }
                        
                        if (!found) {
                            console.log("未找到新创建的笔记，稍后重试 (尝试 " + selectRetryCount + ")")
                            Qt.callLater(selectNewNote)
                        }
                    }
                    
                    // 局部插入逻辑 (与文件夹类似)
                    var parentIndex = -1
                    var parentLevel = -1
                    if (parentPath === "/root") {
                        parentIndex = -1 // 表示根目录
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
                        console.error("未能找到父文件夹用于插入新项目")
                        return
                    }
                    
                    // 确保父文件夹展开 (如果是子文件夹)
                     if (parentIndex !== -1 && !folderListModel.get(parentIndex).expanded) {
                        console.log("展开父文件夹以插入新项")
                        toggleFolderExpanded(parentIndex) // 展开会加载内容，但不包括我们刚创建的
                    }
                    
                    // *** 获取新创建项的数据 (需要新的C++方法) ***
                    // 假设 sidebarManager.getNoteInfo(newNoteId) 返回 QVariantMap
                    // 需要先实现 getNoteInfo
                    // var newItemData = sidebarManager.getNoteInfo(newNoteId)
                    
                    // *** 临时代替：手动构造数据，level 需要正确计算 ***
                    var newItemLevel = parentLevel + 1;
                    var newItemData = {
                        "id": newNoteId,
                        "name": inputText.trim(),
                        "type": "note",
                        "level": newItemLevel,
                        "path": "/note_" + newNoteId,
                        "date": new Date().toISOString() // 模拟日期
                    }
                    console.log("构造新笔记数据:", JSON.stringify(newItemData));
                    
                    // 确定插入位置 (笔记应该放在对应文件夹的最后，但在其他文件夹之前)
                    var insertIndex = -1;
                    if (parentIndex === -1) { // 插入到根目录末尾
                        insertIndex = folderListModel.count
                    } else { // 插入到子文件夹末尾（但在下一个非子项之前）
                        insertIndex = parentIndex + 1;
                        // 跳过所有子孙节点
                        while (insertIndex < folderListModel.count && folderListModel.get(insertIndex).level > parentLevel) {
                            insertIndex++;
                        }
                    }
                    
                    // 执行插入
                    console.log("在索引 " + insertIndex + " 处插入新笔记")
                    folderListModel.insert(insertIndex, newItemData)
                    
                    // 延迟执行选中
                    Qt.callLater(selectNewNote)
                    
                } else {
                    console.error("笔记创建失败:", inputText.trim());
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
                console.log("重命名项目:", itemPath, "为", inputText.trim());
                // 直接调用C++的重命名方法
                if (sidebarManager.renameItem(itemPath, inputText.trim())) {
                    console.log("项目重命名成功:", inputText.trim());
                } else {
                    console.error("项目重命名失败:", inputText.trim());
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
            console.log("删除项目:", itemPath)
            // 直接调用C++的删除方法
            if (sidebarManager.deleteItem(itemPath)) {
                console.log("项目删除成功:", itemPath);
            } else {
                console.error("项目删除失败:", itemPath);
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
        color: "transparent" // 容器背景透明
        
        // 添加鼠标区域用于捕获右键点击空白区域
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton
            z: -1 // 放在底层，只捕获未被其他MouseArea捕获的点击
            
            onClicked: function(mouse) {
                if (mouse.button === Qt.RightButton) {
                    console.log("[NoteTree] 空白区域右键点击")
                    contextMenu.isFolder = true
                    contextMenu.itemPath = "/root" // 根目录ID为1
                    contextMenu.itemName = "根目录"
                    contextMenu.popup()
                }
            }
        }
        
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
            
            // 优化ListView性能
            cacheBuffer: 300 // 增加缓存，提高滚动性能
            reuseItems: true // 重用项目
            highlightFollowsCurrentItem: false // 禁用默认高亮跟随，使用我们自己的高亮逻辑
            
            // 视图行为设置
            interactive: true // 允许交互
            flickableDirection: Flickable.VerticalFlick // 只允许垂直滚动
            boundsBehavior: Flickable.StopAtBounds // 到达边界时停止
            
            // 使用C++提供的数据代替静态数据
            model: folderListModel
            
            // 当模型变化时，强制布局更新
            onModelChanged: {
                console.log("[NoteTree] ListView模型已变化，总项目数: " + (model ? model.count : 0));
                forceLayout();
            }
            
            delegate: Item {
                id: itemContainer
                width: ListView.view.width - 20 
                height: 44
                x: 10
                visible: shouldBeVisible(model)
                
                // 辅助函数：确定项目是否应该可见（基于父文件夹是否展开）
                function shouldBeVisible(item) {
                    // 顶级项目总是可见
                    if (item.level === 0) return true
                    
                    // 查找此项目的父级路径
                    const pathParts = item.path.split("/")
                    if (pathParts.length <= 3) return true  // 顶级项目，可见
                    
                    // 构建父级路径
                    let parentPath = ""
                    for (let i = 0; i < pathParts.length - 1; i++) {
                        parentPath += pathParts[i]
                        if (i < pathParts.length - 2) parentPath += "/"
                    }
                    
                    // 递归检查所有父级是否都展开
                    for (let i = 0; i < folderListModel.count; i++) {
                        let parent = folderListModel.get(i)
                        if (parent.path === parentPath) {
                            // 找到父级，检查是否展开
                            return parent.expanded && shouldBeVisible(parent)
                        }
                    }
                    
                    return false
                }
                
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
                        anchors.leftMargin: 16 + Math.min(model.level, 3) * 20 // 限制最大缩进
                        anchors.rightMargin: 16
                        spacing: 12
                        
                        // 增加层级指示器（针对超过3级的文件夹）
                        Rectangle {
                            id: levelIndicator
                            Layout.preferredWidth: model.level > 3 ? 4 : 0
                            Layout.preferredHeight: 24
                            color: "#888888"
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
                            
                            // 增加红色覆盖，使按钮更明显
                            ColorOverlay {
                                anchors.fill: parent
                                source: parent
                                color: "#333333" // 灰色
                            }
                            
                            // 展开/折叠动画
                            Behavior on rotation { NumberAnimation { duration: 200; easing.type: Easing.OutQuad } }
                            
                            MouseArea {
                                anchors.fill: parent
                                anchors.margins: -10 // 增大点击区域
                                propagateComposedEvents: false // 阻止事件传播
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                
                                onClicked: {
                                    console.log("[NoteTree] 点击了展开/折叠按钮")
                                    // 切换展开状态
                                    toggleFolderExpanded(index)
                                    mouse.accepted = true // 确保事件被接受
                                }
                                
                                // 高亮显示
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
                            elide: Text.ElideMiddle // 从中间省略，而不是右侧
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                            color: noteTree.selectedNotePath === model.path ? 
                                  "#4285F4" : "#333333" // 选中时文字变蓝
                            
                            // 平滑过渡
                            Behavior on color { ColorAnimation { duration: 150 } }
                            
                            // 鼠标悬停时显示完整名称
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
                            noteTree.selectedNoteName = model.name
                            noteSelected(model.path, model.type)
                            
                            // 如果是文件夹，单击时自动展开/折叠
                            if (model.type === "folder") {
                                console.log("[NoteTree] 单击文件夹，自动展开/折叠:", model.name)
                                toggleFolderExpanded(index)
                            }
                        } else if (mouse.button === Qt.RightButton) {
                            // 右键点击：显示上下文菜单
                            contextMenu.isFolder = model.type === "folder"
                            contextMenu.itemPath = model.path
                            contextMenu.itemName = model.name
                            contextMenu.popup()
                        }
                    }
                    
                    // 添加双击处理
                    onDoubleClicked: function(mouse) {
                        if (mouse.button === Qt.LeftButton && model.type === "note") {
                            // 对笔记的双击处理：可以打开笔记内容
                            console.log("[NoteTree] 双击打开笔记:", model.name)
                            noteSelected(model.path, model.type)
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
            }
        }
    }
    
    // 用于连接C++数据的模型
    ListModel {
        id: folderListModel
    }
    
    // 组件初始化
    Component.onCompleted: {
        console.log("[NoteTree] 组件初始化开始")
        // 刷新笔记列表
        refreshNotesList()
        
        // 延迟一点再刷新一次，确保所有内容都加载完毕
        timer.start()
    }
    
    // 用于延迟刷新的定时器
    Timer {
        id: timer
        interval: 500
        repeat: false
        onTriggered: {
            console.log("[NoteTree] 延迟刷新触发，强制更新UI");
            // 强制再次刷新
            refreshNotesList();
            // 强制ListView重新布局
            listView.forceLayout();
            // 发送一个自定义事件给ListView，确保它能察觉到更新
            listView.contentY = listView.contentY + 0.1;
            listView.contentY = listView.contentY - 0.1;
            console.log("[NoteTree] UI刷新完成，当前模型项目数: " + folderListModel.count);
        }
    }
    
    // 刷新文件列表 (主要在初始化时调用)
    function refreshNotesList() {
        console.log("--- [NoteTree] refreshNotesList() CALLED --- "); 
        
        var currentSelectedPath = selectedNotePath;
        folderListModel.clear();
        
        var topLevelItems = sidebarManager.getFolderStructure();
        console.log("[NoteTree] Fetched top level items count: " + topLevelItems.length);
        
        for (var i = 0; i < topLevelItems.length; i++) {
            folderListModel.append(topLevelItems[i]);
            // 初始化时不递归加载子项，依赖用户点击展开
        }
        
        console.log("[NoteTree] refreshNotesList() finished. Total items: " + folderListModel.count);
        
        if (currentSelectedPath) {
            selectedNotePath = currentSelectedPath;
        }
        listView.forceLayout();
    }
    
    // 切换文件夹展开/折叠状态
    function toggleFolderExpanded(index) {
        if (index < 0 || index >= folderListModel.count) return
        
        var folder = folderListModel.get(index)
        if (folder.type !== "folder") return
        
        console.log("--- [NoteTree] toggleFolderExpanded() CALLED for:", folder.name, "Current expanded:", folder.expanded, "Target state:", !folder.expanded);
        
        // 切换状态
        folder.expanded = !folder.expanded
        
        if (folder.expanded) {
            // 展开逻辑 (检查是否已有子项)
            var hasChildren = false
            if (index + 1 < folderListModel.count) {
                var nextItem = folderListModel.get(index + 1)
                if (nextItem.level > folder.level) {
                    hasChildren = true
                    console.log("[NoteTree] Folder already has children in model, skipping add.")
                }
            }
            
            if (!hasChildren) {
                var contents = sidebarManager.getFolderContents(folder.id, folder.level)
                console.log("[NoteTree] Fetched children count for expand: " + contents.length)
                var insertPosition = index + 1
                for (var i = 0; i < contents.length; i++) {
                    folderListModel.insert(insertPosition, contents[i])
                    insertPosition++
                }
            }
        } else {
            // 折叠逻辑
            console.log("--- [NoteTree] Calling removeChildItems() for index:", index);
            removeChildItems(index)
        }
        
        listView.forceLayout()
    }
    
    // 移除子项目
    function removeChildItems(parentIndex) {
        if (parentIndex < 0 || parentIndex >= folderListModel.count) return
        var parentItem = folderListModel.get(parentIndex)
        if (parentItem.type !== "folder") return
        var parentLevel = parentItem.level
        console.log("--- [NoteTree] removeChildItems() CALLED for:", parentItem.name);
        
        var startIndex = parentIndex + 1
        var endIndex = startIndex
        while (endIndex < folderListModel.count && folderListModel.get(endIndex).level > parentLevel) {
            endIndex++
        }
        
        if (endIndex > startIndex) {
            var removeCount = endIndex - startIndex
            console.log("[NoteTree] Removing " + removeCount + " child items from index " + startIndex)
            for (var i = endIndex - 1; i >= startIndex; i--) {
                 folderListModel.remove(i)
            }
        } else {
            console.log("[NoteTree] No child items found to remove.")
        }
        listView.forceLayout();
    }
    
    // 连接C++信号 (现在只处理非创建/删除的全局刷新信号，比如重命名或外部更改)
    Connections {
        target: sidebarManager
        
        function onFolderStructureChanged() {
            console.log("--- [NoteTree] onFolderStructureChanged SIGNAL RECEIVED --- Calling refreshNotesList()");
            // 这个信号现在不应该在创建/删除后立即触发了
            // 它可能在重命名、或未来可能的外部同步时触发
            refreshNotesList();
        }
    }
    
    // 处理创建文件夹请求
    function handleCreateFolderRequest(parentPath) {
        console.log("[NoteTree] Handling Create Folder Request for:", parentPath)
        
        // 自动展开目标文件夹
        if (parentPath !== "/root") {
            ensureFolderExpanded(parentPath)
        }
        
        newFolderDialog.parentPath = parentPath
        newFolderDialog.inputText = ""
        console.log("[NoteTree] Opening New Folder Dialog...")
        newFolderDialog.open()
    }
    
    // 处理创建笔记请求
    function handleCreateNoteRequest(parentPath) {
        console.log("[NoteTree] Handling Create Note Request for:", parentPath)
        
        // 自动展开目标文件夹
        if (parentPath !== "/root") {
            ensureFolderExpanded(parentPath)
        }
        
        newNoteDialog.parentPath = parentPath
        newNoteDialog.inputText = ""
        console.log("[NoteTree] Opening New Note Dialog...")
        newNoteDialog.open()
    }
    
    // 确保文件夹处于展开状态
    function ensureFolderExpanded(folderPath) {
        // 查找匹配的文件夹
        for (var i = 0; i < folderListModel.count; i++) {
            var item = folderListModel.get(i)
            if (item.path === folderPath && item.type === "folder") {
                console.log("[NoteTree] 确保文件夹展开:", item.name)
                
                // 如果未展开，则展开它
                if (!item.expanded) {
                    toggleFolderExpanded(i)
                }
                
                return true
            }
        }
        
        console.log("[NoteTree] 未找到要展开的文件夹:", folderPath)
        return false
    }
    
    // 处理重命名请求
    function handleRenameRequest(path, name) {
        console.log("[NoteTree] Handling Rename Request for:", path, "Name:", name)
        renameDialog.itemPath = path
        renameDialog.inputText = name
        console.log("[NoteTree] Opening Rename Dialog...")
        renameDialog.open()
    }
    
    // 处理删除请求
    function handleDeleteRequest(path) {
        console.log("[NoteTree] Handling Delete Request for:", path)
        deleteDialog.itemPath = path
        console.log("[NoteTree] Opening Delete Dialog...")
        deleteDialog.open()
    }
    
    // 刷新文件夹内容并选中特定项目
    function refreshAndSelectItem(folderId, targetType, targetId) {
        console.log("[NoteTree] 刷新文件夹并选中项目: 文件夹ID=" + folderId + ", 目标类型=" + targetType + ", 目标ID=" + targetId);
        
        // 先获取文件夹内容
        var contents = sidebarManager.getFolderContents(folderId);
        
        // 寻找目标项目的路径
        var targetPath = "";
        for (var i = 0; i < contents.length; i++) {
            if (contents[i].type === targetType && contents[i].id === targetId) {
                targetPath = contents[i].path;
                console.log("[NoteTree] 找到目标项目: " + contents[i].name + ", 路径=" + targetPath);
                break;
            }
        }
        
        // 如果找到目标项目，则选中它
        if (targetPath) {
            // 延迟选中，确保UI已更新
            Qt.callLater(function() {
                console.log("[NoteTree] 选中目标项目: " + targetPath);
                selectedNotePath = targetPath;
                selectedNoteType = targetType;
                
                // 找到项目名称
                for (var i = 0; i < folderListModel.count; i++) {
                    var item = folderListModel.get(i);
                    if (item.path === targetPath) {
                        selectedNoteName = item.name;
                        // 发送选中信号
                        noteSelected(targetPath, targetType);
                        break;
                    }
                }
            });
        } else {
            console.log("[NoteTree] 未找到目标项目");
        }
    }
} 