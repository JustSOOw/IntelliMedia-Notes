/*
 * @Author: Furdow wang22338014@gmail.com
 * @Date: 2025-04-17 12:00:00
 * @LastEditors: Furdow wang22338014@gmail.com
 * @LastEditTime: 2025-05-15 22:02:53
 * @FilePath: \IntelliMedia_Notes\src\sidebarmanager.cpp
 * @Description: 侧边栏管理器实现
 * 
 * Copyright (c) 2025 by Furdow, All Rights Reserved. 
 */
#include "sidebarmanager.h"
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QInputDialog>
#include <QMessageBox>
#include <QTimer>
#include <QSettings>
#include <QApplication>

// 构造函数
SidebarManager::SidebarManager(QQuickWidget *quickWidget, QObject *parent)
    : QObject(parent)
    , m_quickWidget(quickWidget)
    , m_rootObject(nullptr)
    , m_dbManager(nullptr)
    , m_isDarkTheme(false)
    , m_globalFontFamily("Arial") // 默认字体
{
    // 初始化笔记存储
    initializeNoteStorage();
    
    // 初始化数据库管理器
    m_dbManager = new DatabaseManager(this);
    if (!m_dbManager->initialize()) {
        qWarning() << "数据库初始化失败!";
    }
    
    // 读取当前的全局字体设置
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, 
                      QApplication::organizationName(), QApplication::applicationName());
    QString fontFamily = settings.value("Editor/FontFamily", "Arial").toString();
    if (!fontFamily.isEmpty()) {
        m_globalFontFamily = fontFamily;
    }
}

// 析构函数
SidebarManager::~SidebarManager()
{
    // 数据库管理器会随对象树自动清理
}

// 初始化侧边栏
void SidebarManager::initialize()
{
    // 设置QML上下文属性
    QQmlContext *context = m_quickWidget->rootContext();
    context->setContextProperty("sidebarManager", this);
    
    // 设置QML源文件
    m_quickWidget->setSource(QUrl("qrc:/qml/Sidebar.qml"));
    
    // 获取QML根对象
    m_rootObject = m_quickWidget->rootObject();
    
    if (m_rootObject) {
        // 连接QML信号到C++槽
        QObject::connect(m_rootObject, SIGNAL(noteSelected(QString,QString)),
                        this, SLOT(onNoteSelected(QString,QString)));
        
        QObject::connect(m_rootObject, SIGNAL(createNote(QString,QString)),
                        this, SLOT(onCreateNote(QString,QString)));
        
        QObject::connect(m_rootObject, SIGNAL(createFolder(QString,QString)),
                        this, SLOT(onCreateFolder(QString,QString)));
        
        QObject::connect(m_rootObject, SIGNAL(renameItem(QString,QString)),
                        this, SLOT(onRenameItem(QString,QString)));
        
        QObject::connect(m_rootObject, SIGNAL(deleteItem(QString)),
                        this, SLOT(onDeleteItem(QString)));
        
        QObject::connect(m_rootObject, SIGNAL(sendAIMessage(QString)),
                        this, SLOT(onSendAIMessage(QString)));
        
        // 连接搜索按钮点击信号，将QML中的信号转发到C++
        QObject::connect(m_rootObject, SIGNAL(searchButtonClicked()),
                        this, SIGNAL(searchButtonClicked()));
    } else {
        qWarning() << "无法获取QML根对象!";
    }
    
    // 刷新文件列表
    refreshNotesList();
}

// 获取用户名称
QString SidebarManager::getUserName() const
{
    return "150******768"; // 固定用户名
}

// 获取用户状态
QString SidebarManager::getUserStatus() const
{
    return tr("在线"); // 固定状态
}

// 获取根目录路径
QString SidebarManager::getRootPath() const
{
    return m_rootPath;
}

// 获取文件夹结构
QVariantList SidebarManager::getFolderStructure()
{
    QVariantList result;
    qDebug() << "--- [SidebarManager] getFolderStructure() CALLED ---";
    result = getFolderContents(1, -1); 
    return result;
}

// 获取文件夹内容（供QML模型使用）
QVariantList SidebarManager::getFolderContents(int folder_id, int parentLevel)
{
    QVariantList result;
    int currentLevel = parentLevel + 1;
    qDebug() << "--- [SidebarManager] getFolderContents() CALLED for ID:" << folder_id << " ParentLevel:" << parentLevel;
    
    QList<FolderInfo> allFolders = m_dbManager->getAllFolders();
    for (const FolderInfo &childFolder : allFolders) {
        if (childFolder.parent_id == folder_id) {
            result.append(folderToQML(childFolder, currentLevel, false)); 
        }
    }
    
    QList<NoteInfo> notes = m_dbManager->getNotesInFolder(folder_id);
    for (const NoteInfo &note : notes) {
        result.append(noteToQML(note, currentLevel));
    }
    
    return result;
}

// 获取文件夹下所有笔记（包括子文件夹）
QVariantList SidebarManager::getAllNotes(int folder_id)
{
    QVariantList result;
    qDebug() << "--- [SidebarManager] getAllNotes() CALLED for ID:" << folder_id;
    
    // 获取指定文件夹下的笔记
    QList<NoteInfo> notes = m_dbManager->getNotesInFolder(folder_id);
    for (const NoteInfo &note : notes) {
        result.append(noteToQML(note, 0)); // 使用0作为级别，因为我们只关心笔记本身
    }
    
    // 递归获取所有子文件夹中的笔记
    QList<FolderInfo> subFolders = m_dbManager->getAllFolders();
    for (const FolderInfo &folder : subFolders) {
        if (folder.parent_id == folder_id) {
            // 递归调用获取子文件夹的笔记
            QVariantList subNotes = getAllNotes(folder.id);
            for (const QVariant &subNote : subNotes) {
                result.append(subNote);
            }
        }
    }
    
    return result;
}

// 笔记被选中
void SidebarManager::onNoteSelected(const QString &path, const QString &type)
{
    qDebug() << "笔记选中:" << path << "类型:" << type;
    
    // 如果是普通笔记文件，读取内容并发送信号
    if (type == "note") {
        int noteId = extractIdFromPath(path);
        
        // 查找笔记信息 - 需要实现一个辅助方法从所有笔记中筛选，因为没有getNoteById方法
        NoteInfo note;
        bool found = false;
        
        // 获取所有文件夹
        QList<FolderInfo> folders = m_dbManager->getAllFolders();
        
        // 遍历所有文件夹查找笔记
        for (const FolderInfo &folder : folders) {
            QList<NoteInfo> notes = m_dbManager->getNotesInFolder(folder.id);
            for (const NoteInfo &n : notes) {
                if (n.id == noteId) {
                    note = n;
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
        
        if (found) {
            // 获取笔记内容块
            QList<ContentBlock> blocks = m_dbManager->getNoteContent(noteId);
            
            // 简单处理：将所有内容块转换为文本内容
            QString content;
            for (const ContentBlock &block : blocks) {
                if (block.block_type == "text") {
                    content += block.content_text + "\n\n";
                } else if (block.block_type == "image") {
                    content += "[图片: " + block.media_path + "]\n\n";
                }
            }
            
            emit noteOpened(path, content);
        } else {
            qWarning() << "找不到对应的笔记:" << noteId;
        }
    }
}

// 创建新笔记
void SidebarManager::onCreateNote(const QString &parentPath, const QString &noteName)
{
    qDebug() << "创建笔记在:" << parentPath << "名称:" << noteName;
    
    // 获取父文件夹ID
    int parentId = extractIdFromPath(parentPath);
    
    // 使用传入的笔记名称
    if (noteName.isEmpty()) {
        QMessageBox::warning(nullptr, tr("错误"), tr("笔记名称不能为空!"));
        return;
    }
    
    // 创建笔记
    int noteId = m_dbManager->createNote(noteName, parentId);
    
    if (noteId > 0) {
        // 创建初始内容块
        QList<ContentBlock> blocks;
        
        ContentBlock block;
        block.id = 0; // 新块ID由数据库分配
        block.note_id = noteId;
        block.block_type = "text";
        block.content_text = tr("开始编写你的笔记...");
        block.position = 0;
        
        blocks.append(block);
        
        // 保存内容块
        m_dbManager->saveNoteContent(noteId, blocks);
        
        // 刷新列表
        refreshNotesList();
    } else {
        QMessageBox::warning(nullptr, tr("错误"), tr("创建笔记失败!"));
    }
}

// 创建新文件夹
void SidebarManager::onCreateFolder(const QString &parentPath, const QString &folderName)
{
    qDebug() << "创建文件夹在:" << parentPath << "名称:" << folderName;
    
    // 获取父文件夹ID
    int parentId = extractIdFromPath(parentPath);
    
    // 使用传入的文件夹名称
    if (folderName.isEmpty()) {
        QMessageBox::warning(nullptr, tr("错误"), tr("文件夹名称不能为空!"));
        return;
    }
    
    // 创建文件夹
    if (m_dbManager->createFolder(folderName, parentId) > 0) {
        // 刷新列表
        refreshNotesList();
    } else {
        QMessageBox::warning(nullptr, tr("错误"), tr("创建文件夹失败!"));
    }
}

// 重命名项目
void SidebarManager::onRenameItem(const QString &path, const QString &newName)
{
    qDebug() << "重命名:" << path << "为:" << newName;
    
    if (path.isEmpty() || newName.isEmpty()) {
        return;
    }
    
    int itemId = extractIdFromPath(path);
    
    // 判断类型
    if (path.contains("folder_")) {
        // 重命名文件夹
        if (m_dbManager->renameFolder(itemId, newName)) {
            refreshNotesList();
        } else {
            QMessageBox::warning(nullptr, tr("错误"), tr("重命名文件夹失败!"));
        }
    } else if (path.contains("note_")) {
        // 重命名笔记
        if (m_dbManager->renameNote(itemId, newName)) {
            refreshNotesList();
        } else {
            QMessageBox::warning(nullptr, tr("错误"), tr("重命名笔记失败!"));
        }
    }
}

// 删除项目
void SidebarManager::onDeleteItem(const QString &path)
{
    qDebug() << "删除:" << path;
    
    // 不再显示确认对话框，直接执行删除操作
    // QML中的对话框已经处理了确认
    int itemId = extractIdFromPath(path);
    bool success = false;
    
    // 判断类型
    if (path.contains("folder_")) {
        // 删除文件夹
        success = m_dbManager->deleteFolder(itemId);
    } else if (path.contains("note_")) {
        // 删除笔记
        success = m_dbManager->deleteNote(itemId);
    }
    
    if (success) {
        refreshNotesList();
    } else {
        QMessageBox::warning(nullptr, tr("错误"), tr("删除操作失败!"));
    }
}

// 发送AI消息
void SidebarManager::onSendAIMessage(const QString &message)
{
    qDebug() << "发送AI消息:" << message;
    
    // 简单模拟AI响应
    QString response = getTestAIResponse(message);
    
    // 返回AI回复
    emit aiMessageReceived(response);
}

// 刷新文件列表
void SidebarManager::refreshNotesList()
{
    qDebug() << "--- [SidebarManager] refreshNotesList() CALLED ---";
    // 这个函数现在主要由QML的folderStructureChanged信号触发
    // 或者在初始化时调用
    // 不再需要复杂的延迟和多次触发
    
    // 简单地发送信号给QML，让QML的refreshNotesList处理
    emit folderStructureChanged();
}

// 初始化笔记存储目录
void SidebarManager::initializeNoteStorage()
{
    // 使用应用程序数据目录作为笔记的根目录
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_rootPath = appDataPath + "/notes";
    
    // 确保目录存在
    QDir dir;
    if (!dir.exists(m_rootPath)) {
        dir.mkpath(m_rootPath);
    }
    
    qDebug() << "笔记存储目录:" << m_rootPath;
}

// 将数据库FolderInfo转换为QML可用的格式
QVariantMap SidebarManager::folderToQML(const FolderInfo &folder, int level, bool expanded)
{
    QVariantMap result;
    result["id"] = folder.id;
    result["name"] = folder.name;
    result["type"] = "folder";
    result["level"] = level;
    result["expanded"] = expanded;
    result["path"] = QString("/folder_%1").arg(folder.id);
    return result;
}

// 将数据库NoteInfo转换为QML可用的格式
QVariantMap SidebarManager::noteToQML(const NoteInfo &note, int level)
{
    QVariantMap result;
    result["id"] = note.id;
    result["name"] = note.title;
    result["type"] = "note";
    result["level"] = level;
    result["path"] = QString("/note_%1").arg(note.id);
    result["date"] = note.updated_at;
    return result;
}

// 从路径中提取ID
int SidebarManager::extractIdFromPath(const QString &path)
{
    // 处理特殊路径
    if (path.isEmpty()) return 0;
    if (path == "/root") return 1; // 根目录ID为1
    
    // 处理路径格式，例如"/folder_1"或"/note_2"
    QStringList parts = path.split("_");
    if (parts.size() < 2) return 0;
    
    QString idStr = parts.last();
    return idStr.toInt();
}

// 测试用的AI回复消息
QString SidebarManager::getTestAIResponse(const QString &userMessage)
{
    // 简单的回复逻辑，实际应用中可以接入真正的AI服务
    if (userMessage.contains("你好") || userMessage.contains("您好")) {
        return "你好！我是您的AI助手，有什么可以帮助您的？";
    } else if (userMessage.contains("天气")) {
        return "抱歉，我没有联网功能，无法查询天气信息。";
    } else if (userMessage.contains("帮助") || userMessage.contains("能做什么")) {
        return "我可以帮助您管理笔记、回答简单问题，或者提供一些建议。";
    } else if (userMessage.length() < 5) {
        return "请提供更详细的信息，这样我才能更好地帮助您。";
    } else {
        return "我收到了您的消息：\"" + userMessage + "\"。我正在学习中，希望能更好地为您服务。";
    }
}

// 供QML直接调用的创建笔记方法
int SidebarManager::createNote(const QString &parentPath, const QString &noteName)
{
    qDebug() << "--- [SidebarManager] createNote() CALLED for Parent:" << parentPath << " Name:" << noteName;
    // 获取父文件夹ID
    int parentId = extractIdFromPath(parentPath);
    qDebug() << "父文件夹ID:" << parentId << "从路径:" << parentPath;
    
    // 验证笔记名称
    if (noteName.isEmpty()) {
        qWarning() << "笔记名称不能为空!";
        return -1;
    }
    
    // 创建笔记
    int noteId = m_dbManager->createNote(noteName, parentId);
    
    if (noteId > 0) {
        qDebug() << "成功创建笔记:" << noteName << "ID:" << noteId << "父ID:" << parentId;
        // 创建初始内容块
        QList<ContentBlock> blocks;
        
        ContentBlock block;
        block.id = 0; // 新块ID由数据库分配
        block.note_id = noteId;
        block.block_type = "text";
        block.content_text = tr("开始编写你的笔记...");
        block.position = 0;
        
        blocks.append(block);
        
        // 保存内容块
        if (m_dbManager->saveNoteContent(noteId, blocks)) {
            qDebug() << "成功保存笔记内容块";
        } else {
            qWarning() << "保存笔记内容块失败";
        }
        
        return noteId; // 返回新创建的笔记ID
    } else {
        qWarning() << "创建笔记失败! 名称:" << noteName << "父ID:" << parentId;
        return -1;
    }
}

// 供QML直接调用的创建文件夹方法
int SidebarManager::createFolder(const QString &parentPath, const QString &folderName)
{
    qDebug() << "--- [SidebarManager] createFolder() CALLED for Parent:" << parentPath << " Name:" << folderName;
    // 获取父文件夹ID
    int parentId = extractIdFromPath(parentPath);
    qDebug() << "父文件夹ID:" << parentId << "从路径:" << parentPath;
    
    // 验证文件夹名称
    if (folderName.isEmpty()) {
        qWarning() << "文件夹名称不能为空!";
        return -1;
    }
    
    // 创建文件夹
    int folderId = m_dbManager->createFolder(folderName, parentId);
    if (folderId > 0) {
        qDebug() << "成功创建文件夹:" << folderName << "ID:" << folderId << "父ID:" << parentId;
        
        return folderId; // 返回新创建的文件夹ID
    } else {
        qWarning() << "创建文件夹失败! 名称:" << folderName << "父ID:" << parentId;
        return -1;
    }
}

// 供QML直接调用的删除方法
bool SidebarManager::deleteItem(const QString &path)
{
    qDebug() << "--- [SidebarManager] deleteItem() CALLED for Path:" << path;
    if (path.isEmpty()) {
        qWarning() << "路径不能为空!";
        return false;
    }
    
    int itemId = extractIdFromPath(path);
    bool success = false;
    QString errorMsg;
    
    // 判断类型
    if (path.contains("folder_")) {
        // 删除文件夹
        // 先检查文件夹是否存在
        bool folderExists = false;
        QList<FolderInfo> allFolders = m_dbManager->getAllFolders();
        for (const FolderInfo &folder : allFolders) {
            if (folder.id == itemId) {
                folderExists = true;
                break;
            }
        }
        
        if (!folderExists) {
            qWarning() << "要删除的文件夹不存在:" << itemId;
            return false;
        }
        
        // 先检查文件夹下是否有子项目
        QList<FolderInfo> subFolders;
        for (const FolderInfo &folder : allFolders) {
            if (folder.parent_id == itemId) {
                subFolders.append(folder);
            }
        }
        
        QList<NoteInfo> notes = m_dbManager->getNotesInFolder(itemId);
        
        // 如果有子项，先删除子项
        if (!subFolders.isEmpty() || !notes.isEmpty()) {
            qDebug() << "文件夹" << itemId << "下有子项，先递归删除子项";
            
            // 先删除所有笔记
            for (const NoteInfo &note : notes) {
                if (!m_dbManager->deleteNote(note.id)) {
                    qWarning() << "删除笔记失败:" << note.id;
                }
            }
            
            // 再递归删除子文件夹
            for (const FolderInfo &folder : subFolders) {
                QString subPath = QString("/folder_%1").arg(folder.id);
                if (!deleteItem(subPath)) {
                    qWarning() << "删除子文件夹失败:" << folder.id;
                }
            }
        }
        
        // 尝试删除文件夹
        success = m_dbManager->deleteFolder(itemId);
        if (!success) {
            qWarning() << "删除文件夹错误: 提交事务失败";
            // 重试一次
            QTimer::singleShot(100, this, [this, itemId]() {
                qDebug() << "重试删除文件夹:" << itemId;
                if (m_dbManager->deleteFolder(itemId)) {
                    qDebug() << "重试删除文件夹成功:" << itemId;
                    refreshNotesList();
                }
            });
        }
    } else if (path.contains("note_")) {
        // 删除笔记
        success = m_dbManager->deleteNote(itemId);
        if (!success) {
            qWarning() << "删除笔记错误: 提交事务失败";
            // 重试一次
            QTimer::singleShot(100, this, [this, itemId]() {
                qDebug() << "重试删除笔记:" << itemId;
                if (m_dbManager->deleteNote(itemId)) {
                    qDebug() << "重试删除笔记成功:" << itemId;
                    refreshNotesList();
                }
            });
        }
    }
    
    if (success) {
        qDebug() << "[SidebarManager] Delete Success for:" << path;
        // *** 保留信号发射，因为删除操作确实需要全局刷新 ***
        emit folderStructureChanged(); 
        // refreshNotesList(); // refreshNotesList 现在只是发射信号，所以直接发射即可
        return true;
    } else {
        qWarning() << "[SidebarManager] Delete Failed for:" << path;
        return false;
    }
}

// 供QML直接调用的重命名方法
bool SidebarManager::renameItem(const QString &path, const QString &newName)
{
    qDebug() << "--- [SidebarManager] renameItem() CALLED for Path:" << path << " NewName:" << newName;
    if (path.isEmpty() || newName.isEmpty()) {
        qWarning() << "路径或新名称不能为空!";
        return false;
    }
    
    int itemId = extractIdFromPath(path);
    bool success = false;
    
    // 判断类型
    if (path.contains("folder_")) {
        // 重命名文件夹
        success = m_dbManager->renameFolder(itemId, newName);
    } else if (path.contains("note_")) {
        // 重命名笔记
        success = m_dbManager->renameNote(itemId, newName);
    }
    
    if (success) {
        qDebug() << "[SidebarManager] Rename Success for:" << path;
         // *** 保留信号发射，因为重命名也需要全局刷新 ***
        emit folderStructureChanged();
        // refreshNotesList(); // refreshNotesList 现在只是发射信号，所以直接发射即可
        return true;
    } else {
        qWarning() << "[SidebarManager] Rename Failed for:" << path;
        return false;
    }
}

// 恢复侧边栏到默认视图
void SidebarManager::resetToDefaultView()
{
    if (m_rootObject) {
        // 调用QML的resetView方法
        QMetaObject::invokeMethod(m_rootObject, "resetView");
    }
}

// 重置ActionButtons组件的布局
bool SidebarManager::resetActionButtons()
{
    if (!m_rootObject) {
        qWarning() << "无法重置ActionButtons：根对象为空";
        return false;
    }
    
    // 使用QML对象的objectName找到ActionButtons组件
    QList<QObject*> children = m_rootObject->findChildren<QObject*>("actionButtons", Qt::FindChildrenRecursively);
    if (!children.isEmpty()) {
        QObject* actionButtons = children.first();
        qDebug() << "找到ActionButtons组件，调用reset方法";
        return QMetaObject::invokeMethod(actionButtons, "reset");
    } else {
        // 如果找不到通过objectName，尝试直接找
        QObject* actionButtons = m_rootObject->findChild<QObject*>("actionButtons");
        if (actionButtons) {
            qDebug() << "通过直接查找找到ActionButtons组件";
            return QMetaObject::invokeMethod(actionButtons, "reset");
        }
        
        qWarning() << "在侧边栏中找不到ActionButtons组件";
        
        // 最后尝试用QML对象名称直接访问
        QObject* rootItem = qobject_cast<QObject*>(m_rootObject);
        if (rootItem) {
            QVariant returnedValue;
            QVariant nameValue = "actionButtons";
            // 尝试调用QML的findChild方法
            bool success = QMetaObject::invokeMethod(
                rootItem,
                "findChild",
                Q_RETURN_ARG(QVariant, returnedValue),
                Q_ARG(QVariant, nameValue)
            );
            
            if (success && !returnedValue.isNull()) {
                QObject* actionButtonsObj = qvariant_cast<QObject*>(returnedValue);
                if (actionButtonsObj) {
                    qDebug() << "通过QML findChild找到ActionButtons组件";
                    return QMetaObject::invokeMethod(actionButtonsObj, "reset");
                }
            }
        }
        
        return false;
    }
}

// 通过路径打开笔记（用于重启后恢复打开的笔记）
void SidebarManager::openNoteByPath(const QString &path)
{
    qDebug() << "--- [SidebarManager] openNoteByPath() CALLED for Path:" << path;
    
    if (path.isEmpty()) {
        qWarning() << "路径为空，无法打开笔记";
        return;
    }
    
    // 判断路径类型和提取ID
    if (path.contains("note_")) {
        int noteId = extractIdFromPath(path);
        if (noteId > 0) {
            // 通过ID获取笔记信息
            NoteInfo noteInfo = m_dbManager->getNoteById(noteId);
            if (noteInfo.id > 0) {
                // 发送笔记打开信号
                emit noteOpened(path, "note");
                
                // 如果需要在QML中显示选中状态，可以调用QML方法
                if (m_rootObject) {
                    QMetaObject::invokeMethod(m_rootObject, "selectNoteByPath", 
                                            Q_ARG(QVariant, path));
                }
            } else {
                qWarning() << "找不到ID为" << noteId << "的笔记";
            }
        } else {
            qWarning() << "无效的笔记ID:" << noteId;
        }
    } else {
        qWarning() << "不支持的路径类型:" << path;
    }
}

// 更新主题设置
void SidebarManager::updateTheme(bool isDarkTheme)
{
    if (m_isDarkTheme != isDarkTheme) {
        m_isDarkTheme = isDarkTheme;
        emit themeChanged();
        qDebug() << "侧边栏主题已更新为:" << (m_isDarkTheme ? "暗色主题" : "亮色主题");
    }
}

// 更新全局字体设置
void SidebarManager::updateGlobalFont(const QString &fontFamily)
{
    if (m_globalFontFamily != fontFamily) {
        m_globalFontFamily = fontFamily;
        emit fontChanged();
        qDebug() << "侧边栏全局字体已更新为:" << fontFamily;
    }
} 