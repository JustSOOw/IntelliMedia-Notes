/*
 * @Author: Furdow wang22338014@gmail.com
 * @Date: 2025-04-17 12:00:00
 * @LastEditors: Furdow wang22338014@gmail.com
 * @LastEditTime: 2025-04-17 12:00:00
 * @FilePath: \IntelliMedia_Notes\src\databasemanager.cpp
 * @Description: 数据库管理类实现
 * 
 * Copyright (c) 2025 by Furdow, All Rights Reserved. 
 */
#include "databasemanager.h"
#include <QCoreApplication>
#include <QFile>
#include <QSqlQuery>
#include <QSqlError>

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
    // 在构造函数中不执行初始化，让调用者决定何时初始化
}

DatabaseManager::~DatabaseManager()
{
    // 关闭数据库连接
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DatabaseManager::initialize()
{
    // 设置数据库文件路径（使用应用程序数据目录）
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataPath);
    
    // 确保目录存在
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // 数据库文件路径
    m_dbPath = appDataPath + "/notes.db";
    qDebug() << "数据库路径:" << m_dbPath;
    
    // 媒体文件夹路径
    m_mediaPath = appDataPath + "/notes_media";
    QDir mediaDir(m_mediaPath);
    if (!mediaDir.exists()) {
        mediaDir.mkpath(".");
    }
    qDebug() << "媒体文件夹路径:" << m_mediaPath;
    
    // 初始化数据库连接
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(m_dbPath);
    
    // 打开数据库
    if (!m_db.open()) {
        qCritical() << "无法打开数据库:" << m_db.lastError().text();
        return false;
    }
    
    // 创建数据库表
    if (!createTables()) {
        qCritical() << "创建数据库表失败";
        return false;
    }
    
    return true;
}

bool DatabaseManager::createTables()
{
    QSqlQuery query;
    
    // 创建Folders表
    if (!tableExists("Folders")) {
        QString sql = 
            "CREATE TABLE Folders ("
            "folder_id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "name TEXT NOT NULL, "
            "parent_id INTEGER DEFAULT 0, "
            "path TEXT NOT NULL, "
            "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
            ")";
        
        if (!executeQuery(query, sql)) {
            return false;
        }
        
        // 创建根文件夹
        sql = "INSERT INTO Folders (folder_id, name, parent_id, path) VALUES (1, '根目录', 0, '/root')";
        if (!executeQuery(query, sql)) {
            return false;
        }
    }
    
    // 创建Notes表
    if (!tableExists("Notes")) {
        QString sql = 
            "CREATE TABLE Notes ("
            "note_id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "title TEXT NOT NULL, "
            "created_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
            "updated_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
            "folder_id INTEGER, "
            "tags TEXT, "
            "is_trashed INTEGER DEFAULT 0, "
            "FOREIGN KEY (folder_id) REFERENCES Folders(folder_id)"
            ")";
        
        if (!executeQuery(query, sql)) {
            return false;
        }
    }
    
    // 创建ContentBlocks表
    if (!tableExists("ContentBlocks")) {
        QString sql = 
            "CREATE TABLE ContentBlocks ("
            "block_id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "note_id INTEGER NOT NULL, "
            "block_type TEXT NOT NULL, "
            "position INTEGER NOT NULL, "
            "content_text TEXT, "
            "media_path TEXT, "
            "properties TEXT, "
            "FOREIGN KEY (note_id) REFERENCES Notes(note_id)"
            ")";
        
        if (!executeQuery(query, sql)) {
            return false;
        }
        
        // 创建索引加速查询
        sql = "CREATE INDEX idx_blocks_note_position ON ContentBlocks(note_id, position)";
        if (!executeQuery(query, sql)) {
            return false;
        }
    }
    
    // 创建Annotations表
    if (!tableExists("Annotations")) {
        QString sql = 
            "CREATE TABLE Annotations ("
            "annotation_id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "block_id INTEGER NOT NULL, "
            "annotation_type TEXT NOT NULL, "
            "data TEXT NOT NULL, "
            "created_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
            "FOREIGN KEY (block_id) REFERENCES ContentBlocks(block_id)"
            ")";
        
        if (!executeQuery(query, sql)) {
            return false;
        }
        
        // 创建索引加速查询
        sql = "CREATE INDEX idx_annotations_block ON Annotations(block_id)";
        if (!executeQuery(query, sql)) {
            return false;
        }
    }
    
    return true;
}

bool DatabaseManager::executeQuery(QSqlQuery &query, const QString &sql)
{
    if (!query.exec(sql)) {
        qCritical() << "SQL执行错误:" << query.lastError().text();
        qCritical() << "SQL语句:" << sql;
        return false;
    }
    return true;
}

bool DatabaseManager::tableExists(const QString &tableName)
{
    QSqlQuery query;
    query.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name=:name");
    query.bindValue(":name", tableName);
    
    if (!query.exec() || !query.next()) {
        return false;
    }
    
    return true;
}

QList<FolderInfo> DatabaseManager::getAllFolders()
{
    QList<FolderInfo> folders;
    QSqlQuery query;
    
    // 查询所有文件夹，按照创建时间排序
    QString sql = "SELECT folder_id, name, parent_id, path, created_at FROM Folders ORDER BY created_at";
    
    if (!executeQuery(query, sql)) {
        return folders;
    }
    
    // 遍历查询结果
    while (query.next()) {
        FolderInfo folder;
        folder.id = query.value(0).toInt();
        folder.name = query.value(1).toString();
        folder.parent_id = query.value(2).toInt();
        folder.path = query.value(3).toString();
        folder.created_at = query.value(4).toString();
        
        folders.append(folder);
    }
    
    return folders;
}

QList<NoteInfo> DatabaseManager::getNotesInFolder(int folder_id)
{
    QList<NoteInfo> notes;
    QSqlQuery query;
    
    // 查询指定文件夹下的所有非回收站笔记
    QString sql = "SELECT note_id, title, created_at, updated_at, folder_id, tags, is_trashed "
                  "FROM Notes WHERE folder_id = :folder_id AND is_trashed = 0 "
                  "ORDER BY updated_at DESC";
    
    query.prepare(sql);
    query.bindValue(":folder_id", folder_id);
    
    if (!query.exec()) {
        qCritical() << "获取文件夹笔记失败:" << query.lastError().text();
        return notes;
    }
    
    // 遍历查询结果
    while (query.next()) {
        NoteInfo note;
        note.id = query.value(0).toInt();
        note.title = query.value(1).toString();
        note.created_at = query.value(2).toString();
        note.updated_at = query.value(3).toString();
        note.folder_id = query.value(4).toInt();
        note.tags = query.value(5).toString();
        note.is_trashed = query.value(6).toBool();
        
        notes.append(note);
    }
    
    return notes;
}

NoteInfo DatabaseManager::getNoteById(int note_id)
{
    NoteInfo note;
    QSqlQuery query;
    
    // 查询指定ID的笔记
    QString sql = "SELECT note_id, title, created_at, updated_at, folder_id, tags, is_trashed "
                 "FROM Notes WHERE note_id = :note_id";
    
    query.prepare(sql);
    query.bindValue(":note_id", note_id);
    
    if (!query.exec()) {
        qCritical() << "获取笔记信息失败:" << query.lastError().text();
        return note;
    }
    
    // 获取查询结果
    if (query.next()) {
        note.id = query.value(0).toInt();
        note.title = query.value(1).toString();
        note.created_at = query.value(2).toString();
        note.updated_at = query.value(3).toString();
        note.folder_id = query.value(4).toInt();
        note.tags = query.value(5).toString();
        note.is_trashed = query.value(6).toBool();
    } else {
        qWarning() << "未找到ID为" << note_id << "的笔记";
    }
    
    return note;
}

int DatabaseManager::createFolder(const QString &name, int parent_id)
{
    if (name.isEmpty()) {
        qWarning() << "文件夹名称不能为空";
        return -1;
    }
    
    QSqlQuery query;
    
    // 首先获取父文件夹路径
    QString parentPath = "/root"; // 默认为根路径
    
    if (parent_id > 0) {
        QString sql = "SELECT path FROM Folders WHERE folder_id = :parent_id";
        query.prepare(sql);
        query.bindValue(":parent_id", parent_id);
        
        if (!query.exec() || !query.next()) {
            qWarning() << "无法找到父文件夹:" << parent_id;
            return -1;
        }
        
        parentPath = query.value(0).toString();
    }
    
    // 构建新文件夹的路径
    QString path = parentPath + "/" + name;
    
    // 插入新文件夹
    query.prepare("INSERT INTO Folders (name, parent_id, path) VALUES (:name, :parent_id, :path)");
    query.bindValue(":name", name);
    query.bindValue(":parent_id", parent_id);
    query.bindValue(":path", path);
    
    if (!query.exec()) {
        qCritical() << "创建文件夹失败:" << query.lastError().text();
        return -1;
    }
    
    // 返回新文件夹的ID
    return query.lastInsertId().toInt();
}

int DatabaseManager::createNote(const QString &title, int folder_id)
{
    if (title.isEmpty()) {
        qWarning() << "笔记标题不能为空";
        return -1;
    }
    
    QSqlQuery query;
    
    // 检查文件夹是否存在
    if (folder_id > 0) {
        query.prepare("SELECT folder_id FROM Folders WHERE folder_id = :folder_id");
        query.bindValue(":folder_id", folder_id);
        
        if (!query.exec() || !query.next()) {
            qWarning() << "无法找到文件夹:" << folder_id;
            return -1;
        }
    }
    
    // 插入新笔记
    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    
    query.prepare("INSERT INTO Notes (title, folder_id, created_at, updated_at) "
                 "VALUES (:title, :folder_id, :created_at, :updated_at)");
    query.bindValue(":title", title);
    query.bindValue(":folder_id", folder_id);
    query.bindValue(":created_at", currentTime);
    query.bindValue(":updated_at", currentTime);
    
    if (!query.exec()) {
        qCritical() << "创建笔记失败:" << query.lastError().text();
        return -1;
    }
    
    return query.lastInsertId().toInt();
}

bool DatabaseManager::deleteFolder(int folder_id)
{
    // 不允许删除根文件夹
    if (folder_id <= 1) {
        qWarning() << "不能删除根文件夹";
        return false;
    }
    
    QSqlQuery query;
    
    // 开始事务
    m_db.transaction();
    
    try {
        // 查询子文件夹
        query.prepare("SELECT folder_id FROM Folders WHERE parent_id = :parent_id");
        query.bindValue(":parent_id", folder_id);
        
        if (!query.exec()) {
            throw std::runtime_error("查询子文件夹失败");
        }
        
        // 递归删除子文件夹
        while (query.next()) {
            int childId = query.value(0).toInt();
            if (!deleteFolder(childId)) {
                throw std::runtime_error("删除子文件夹失败");
            }
        }
        
        // 获取文件夹内的所有笔记
        query.prepare("SELECT note_id FROM Notes WHERE folder_id = :folder_id");
        query.bindValue(":folder_id", folder_id);
        
        if (!query.exec()) {
            throw std::runtime_error("查询文件夹笔记失败");
        }
        
        // 删除所有笔记
        while (query.next()) {
            int noteId = query.value(0).toInt();
            if (!deleteNote(noteId)) {
                throw std::runtime_error("删除笔记失败");
            }
        }
        
        // 删除文件夹本身
        query.prepare("DELETE FROM Folders WHERE folder_id = :folder_id");
        query.bindValue(":folder_id", folder_id);
        
        if (!query.exec()) {
            throw std::runtime_error("删除文件夹失败");
        }
        
        // 提交事务
        if (!m_db.commit()) {
            throw std::runtime_error("提交事务失败");
        }
        
        return true;
    }
    catch (const std::exception &e) {
        // 回滚事务
        m_db.rollback();
        qCritical() << "删除文件夹错误:" << e.what();
        return false;
    }
}

bool DatabaseManager::renameFolder(int folder_id, const QString &new_name)
{
    // 不允许重命名根文件夹
    if (folder_id <= 1) {
        qWarning() << "不能重命名根文件夹";
        return false;
    }
    
    if (new_name.isEmpty()) {
        qWarning() << "新文件夹名称不能为空";
        return false;
    }
    
    QSqlQuery query;
    
    // 开始事务
    m_db.transaction();
    
    try {
        // 获取当前文件夹信息
        query.prepare("SELECT name, parent_id, path FROM Folders WHERE folder_id = :folder_id");
        query.bindValue(":folder_id", folder_id);
        
        if (!query.exec() || !query.next()) {
            throw std::runtime_error("获取文件夹信息失败");
        }
        
        QString oldName = query.value(0).toString();
        int parentId = query.value(1).toInt();
        QString oldPath = query.value(2).toString();
        
        // 构建新路径
        QString newPath = oldPath.left(oldPath.length() - oldName.length()) + new_name;
        
        // 更新文件夹路径
        query.prepare("UPDATE Folders SET name = :name, path = :path WHERE folder_id = :folder_id");
        query.bindValue(":name", new_name);
        query.bindValue(":path", newPath);
        query.bindValue(":folder_id", folder_id);
        
        if (!query.exec()) {
            throw std::runtime_error("更新文件夹信息失败");
        }
        
        // 递归更新子文件夹路径
        query.prepare("SELECT folder_id, path FROM Folders WHERE path LIKE :path_pattern");
        query.bindValue(":path_pattern", oldPath + "/%");
        
        if (!query.exec()) {
            throw std::runtime_error("查询子文件夹失败");
        }
        
        // 存储需要更新的文件夹
        QList<QPair<int, QString>> foldersToUpdate;
        
        while (query.next()) {
            int childId = query.value(0).toInt();
            QString childPath = query.value(1).toString();
            QString newChildPath = childPath.replace(0, oldPath.length(), newPath);
            
            foldersToUpdate.append(qMakePair(childId, newChildPath));
        }
        
        // 更新所有子文件夹路径
        for (const auto &pair : foldersToUpdate) {
            query.prepare("UPDATE Folders SET path = :path WHERE folder_id = :folder_id");
            query.bindValue(":path", pair.second);
            query.bindValue(":folder_id", pair.first);
            
            if (!query.exec()) {
                throw std::runtime_error("更新子文件夹路径失败");
            }
        }
        
        // 提交事务
        if (!m_db.commit()) {
            throw std::runtime_error("提交事务失败");
        }
        
        return true;
    }
    catch (const std::exception &e) {
        // 回滚事务
        m_db.rollback();
        qCritical() << "重命名文件夹错误:" << e.what();
        return false;
    }
}

bool DatabaseManager::renameNote(int note_id, const QString &new_title)
{
    if (new_title.isEmpty()) {
        qWarning() << "新笔记标题不能为空";
        return false;
    }
    
    QSqlQuery query;
    
    // 更新笔记标题和时间戳
    query.prepare("UPDATE Notes SET title = :title, updated_at = CURRENT_TIMESTAMP WHERE note_id = :note_id");
    query.bindValue(":title", new_title);
    query.bindValue(":note_id", note_id);
    
    if (!query.exec()) {
        qCritical() << "重命名笔记失败:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DatabaseManager::moveNoteToTrash(int note_id)
{
    QSqlQuery query;
    
    // 更新笔记状态为已删除
    query.prepare("UPDATE Notes SET is_trashed = 1, updated_at = CURRENT_TIMESTAMP WHERE note_id = :note_id");
    query.bindValue(":note_id", note_id);
    
    if (!query.exec()) {
        qCritical() << "移动笔记到回收站失败:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DatabaseManager::deleteNote(int note_id)
{
    QSqlQuery query;
    
    // 开始事务
    m_db.transaction();
    
    try {
        // 删除笔记的所有标注
        query.prepare("DELETE FROM Annotations WHERE block_id IN "
                     "(SELECT block_id FROM ContentBlocks WHERE note_id = :note_id)");
        query.bindValue(":note_id", note_id);
        
        if (!query.exec()) {
            throw std::runtime_error("删除笔记标注失败");
        }
        
        // 删除笔记的所有内容块
        query.prepare("DELETE FROM ContentBlocks WHERE note_id = :note_id");
        query.bindValue(":note_id", note_id);
        
        if (!query.exec()) {
            throw std::runtime_error("删除笔记内容块失败");
        }
        
        // 删除笔记本身
        query.prepare("DELETE FROM Notes WHERE note_id = :note_id");
        query.bindValue(":note_id", note_id);
        
        if (!query.exec()) {
            throw std::runtime_error("删除笔记失败");
        }
        
        // 提交事务
        if (!m_db.commit()) {
            throw std::runtime_error("提交事务失败");
        }
        
        return true;
    }
    catch (const std::exception &e) {
        // 回滚事务
        m_db.rollback();
        qCritical() << "删除笔记错误:" << e.what();
        return false;
    }
}

bool DatabaseManager::moveNote(int note_id, int folder_id)
{
    QSqlQuery query;
    
    // 检查文件夹是否存在
    if (folder_id > 0) {
        query.prepare("SELECT folder_id FROM Folders WHERE folder_id = :folder_id");
        query.bindValue(":folder_id", folder_id);
        
        if (!query.exec() || !query.next()) {
            qWarning() << "无法找到目标文件夹:" << folder_id;
            return false;
        }
    }
    
    // 移动笔记到指定文件夹
    query.prepare("UPDATE Notes SET folder_id = :folder_id, updated_at = CURRENT_TIMESTAMP WHERE note_id = :note_id");
    query.bindValue(":folder_id", folder_id);
    query.bindValue(":note_id", note_id);
    
    if (!query.exec()) {
        qCritical() << "移动笔记失败:" << query.lastError().text();
        return false;
    }
    
    return true;
}

QList<ContentBlock> DatabaseManager::getNoteContent(int note_id)
{
    QList<ContentBlock> blocks;
    QSqlQuery query;
    
    // 查询笔记的所有内容块，按位置排序
    query.prepare("SELECT block_id, note_id, block_type, position, content_text, media_path, properties "
                 "FROM ContentBlocks WHERE note_id = :note_id ORDER BY position");
    query.bindValue(":note_id", note_id);
    
    if (!query.exec()) {
        qCritical() << "获取笔记内容失败:" << query.lastError().text();
        return blocks;
    }
    
    // 遍历查询结果
    while (query.next()) {
        ContentBlock block;
        block.id = query.value(0).toInt();
        block.note_id = query.value(1).toInt();
        block.block_type = query.value(2).toString();
        block.position = query.value(3).toInt();
        block.content_text = query.value(4).toString();
        block.media_path = query.value(5).toString();
        block.properties = query.value(6).toString();
        
        blocks.append(block);
    }
    
    return blocks;
}

QList<Annotation> DatabaseManager::getImageAnnotations(int block_id)
{
    QList<Annotation> annotations;
    QSqlQuery query;
    
    // 查询图片块的所有标注
    query.prepare("SELECT annotation_id, block_id, annotation_type, data, created_at "
                 "FROM Annotations WHERE block_id = :block_id ORDER BY created_at");
    query.bindValue(":block_id", block_id);
    
    if (!query.exec()) {
        qCritical() << "获取图片标注失败:" << query.lastError().text();
        return annotations;
    }
    
    // 遍历查询结果
    while (query.next()) {
        Annotation annotation;
        annotation.id = query.value(0).toInt();
        annotation.block_id = query.value(1).toInt();
        annotation.annotation_type = query.value(2).toString();
        annotation.data = query.value(3).toString();
        annotation.created_at = query.value(4).toString();
        
        annotations.append(annotation);
    }
    
    return annotations;
}

bool DatabaseManager::saveNoteContent(int note_id, const QList<ContentBlock> &blocks)
{
    QSqlQuery query;
    
    // 开始事务
    m_db.transaction();
    
    try {
        // 删除笔记的所有旧标注
        query.prepare("DELETE FROM Annotations WHERE block_id IN "
                     "(SELECT block_id FROM ContentBlocks WHERE note_id = :note_id)");
        query.bindValue(":note_id", note_id);
        
        if (!query.exec()) {
            throw std::runtime_error("删除旧标注失败");
        }
        
        // 删除笔记的所有旧内容块
        query.prepare("DELETE FROM ContentBlocks WHERE note_id = :note_id");
        query.bindValue(":note_id", note_id);
        
        if (!query.exec()) {
            throw std::runtime_error("删除旧内容块失败");
        }
        
        // 插入新内容块
        for (const ContentBlock &block : blocks) {
            query.prepare("INSERT INTO ContentBlocks (note_id, block_type, position, content_text, media_path, properties) "
                         "VALUES (:note_id, :block_type, :position, :content_text, :media_path, :properties)");
            query.bindValue(":note_id", note_id);
            query.bindValue(":block_type", block.block_type);
            query.bindValue(":position", block.position);
            query.bindValue(":content_text", block.content_text);
            query.bindValue(":media_path", block.media_path);
            query.bindValue(":properties", block.properties);
            
            if (!query.exec()) {
                throw std::runtime_error("插入内容块失败");
            }
        }
        
        // 更新笔记时间戳
        if (!updateNoteTimestamp(note_id)) {
            throw std::runtime_error("更新笔记时间戳失败");
        }
        
        // 提交事务
        if (!m_db.commit()) {
            throw std::runtime_error("提交事务失败");
        }
        
        return true;
    }
    catch (const std::exception &e) {
        // 回滚事务
        m_db.rollback();
        qCritical() << "保存笔记内容错误:" << e.what();
        return false;
    }
}

bool DatabaseManager::saveImageAnnotations(int block_id, const QList<Annotation> &annotations)
{
    QSqlQuery query;
    
    // 开始事务
    m_db.transaction();
    
    try {
        // 删除图片块的所有旧标注
        query.prepare("DELETE FROM Annotations WHERE block_id = :block_id");
        query.bindValue(":block_id", block_id);
        
        if (!query.exec()) {
            throw std::runtime_error("删除旧标注失败");
        }
        
        // 插入新标注
        for (const Annotation &annotation : annotations) {
            query.prepare("INSERT INTO Annotations (block_id, annotation_type, data) "
                         "VALUES (:block_id, :annotation_type, :data)");
            query.bindValue(":block_id", block_id);
            query.bindValue(":annotation_type", annotation.annotation_type);
            query.bindValue(":data", annotation.data);
            
            if (!query.exec()) {
                throw std::runtime_error("插入标注失败");
            }
        }
        
        // 查询该图片块所属的笔记ID
        query.prepare("SELECT note_id FROM ContentBlocks WHERE block_id = :block_id");
        query.bindValue(":block_id", block_id);
        
        if (!query.exec() || !query.next()) {
            throw std::runtime_error("获取笔记ID失败");
        }
        
        int note_id = query.value(0).toInt();
        
        // 更新笔记时间戳
        if (!updateNoteTimestamp(note_id)) {
            throw std::runtime_error("更新笔记时间戳失败");
        }
        
        // 提交事务
        if (!m_db.commit()) {
            throw std::runtime_error("提交事务失败");
        }
        
        return true;
    }
    catch (const std::exception &e) {
        // 回滚事务
        m_db.rollback();
        qCritical() << "保存图片标注错误:" << e.what();
        return false;
    }
}

bool DatabaseManager::updateNoteTimestamp(int note_id)
{
    QSqlQuery query;
    
    // 更新笔记的最后修改时间
    query.prepare("UPDATE Notes SET updated_at = CURRENT_TIMESTAMP WHERE note_id = :note_id");
    query.bindValue(":note_id", note_id);
    
    if (!query.exec()) {
        qCritical() << "更新笔记时间戳失败:" << query.lastError().text();
        return false;
    }
    
    return true;
}

QString DatabaseManager::importImageToMedia(const QString &source_path)
{
    // 检查源文件是否存在
    QFileInfo sourceInfo(source_path);
    if (!sourceInfo.exists() || !sourceInfo.isFile()) {
        qWarning() << "源图片文件不存在:" << source_path;
        return QString();
    }
    
    // 生成唯一文件名
    QString filename = generateUniqueFilename(sourceInfo.fileName());
    QString destPath = m_mediaPath + "/" + filename;
    
    // 复制文件
    if (!QFile::copy(source_path, destPath)) {
        qCritical() << "复制图片文件失败:" << source_path << "->" << destPath;
        return QString();
    }
    
    // 返回相对路径
    return "notes_media/" + filename;
}

QString DatabaseManager::getMediaAbsolutePath(const QString &relative_path)
{
    // 检查是否已是绝对路径
    QFileInfo info(relative_path);
    if (info.isAbsolute()) {
        return relative_path;
    }
    
    // 从相对路径构建绝对路径
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    
    // 如果相对路径以notes_media/开头，去掉这个前缀
    QString path = relative_path;
    if (path.startsWith("notes_media/")) {
        path = path.mid(12);
    }
    
    return appDataPath + "/notes_media/" + path;
}

QString DatabaseManager::generateUniqueFilename(const QString &original_name)
{
    // 获取文件后缀
    QFileInfo info(original_name);
    QString suffix = info.suffix();
    
    // 生成基于UUID和时间戳的唯一文件名
    QString uniqueId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    
    return QString("%1_%2.%3").arg(timestamp).arg(uniqueId).arg(suffix);
}

int DatabaseManager::cleanUnusedMediaFiles()
{
    int count = 0;
    QSqlQuery query;
    
    // 获取数据库中所有已使用的媒体文件路径
    QStringList usedPaths;
    
    if (query.exec("SELECT DISTINCT media_path FROM ContentBlocks WHERE media_path IS NOT NULL")) {
        while (query.next()) {
            QString path = query.value(0).toString();
            if (!path.isEmpty()) {
                // 规范化路径格式
                if (path.startsWith("notes_media/")) {
                    path = path.mid(12);
                }
                usedPaths.append(path);
            }
        }
    }
    
    // 遍历媒体文件夹
    QDir mediaDir(m_mediaPath);
    QStringList files = mediaDir.entryList(QDir::Files);
    
    // 删除未使用的文件
    for (const QString &file : files) {
        if (!usedPaths.contains(file)) {
            if (mediaDir.remove(file)) {
                count++;
                qDebug() << "删除未使用的媒体文件:" << file;
            } else {
                qWarning() << "无法删除未使用的媒体文件:" << file;
            }
        }
    }
    
    return count;
}

QList<SearchResultInfo> DatabaseManager::searchNotes(
    const QString &keyword, 
    int dateFilter, 
    int contentType, 
    int sortType
) {
    QList<SearchResultInfo> results;
    
    // 构建基本查询
    QString sql = "SELECT n.note_id, n.title, n.created_at, n.updated_at, n.folder_id, "
                 "f.path as folder_path, "
                 "(SELECT c.content_text FROM ContentBlocks c WHERE c.note_id = n.note_id "
                 "ORDER BY c.position LIMIT 1) as preview "
                 "FROM Notes n "
                 "LEFT JOIN Folders f ON n.folder_id = f.folder_id "
                 "WHERE n.is_trashed = 0 ";
                 
    // 添加关键词搜索条件 (如果关键词非空)
    if (!keyword.isEmpty()) {
        sql += "AND (n.title LIKE :keyword OR EXISTS (SELECT 1 FROM ContentBlocks c WHERE "
               "c.note_id = n.note_id AND c.content_text LIKE :keyword)) ";
    }
    
    // 添加日期筛选
    QString dateCondition;
    QDateTime now = QDateTime::currentDateTime();
    
    switch (dateFilter) {
        case 1: // 今天
            dateCondition = QString("AND DATE(n.updated_at) = \'%1\' ")
                            .arg(now.toString("yyyy-MM-dd"));
            break;
        case 2: // 最近一周
            dateCondition = QString("AND julianday(n.updated_at) >= julianday(\'%1\') - 7 ")
                            .arg(now.toString("yyyy-MM-dd HH:mm:ss"));
            break;
        case 3: // 最近一月
            dateCondition = QString("AND julianday(n.updated_at) >= julianday(\'%1\') - 30 ")
                            .arg(now.toString("yyyy-MM-dd HH:mm:ss"));
            break;
        default: // 全部时间
            break;
    }
    
    sql += dateCondition;
    
    // 添加内容类型筛选
    if (contentType > 0) {
        QString blockType;
        switch (contentType) {
            case 1: // 文本
                blockType = "text";
                break;
            case 2: // 图片
                blockType = "image";
                break;
            case 3: // 列表
                blockType = "list";
                break;
        }
        
        if (!blockType.isEmpty()) {
            sql += QString("AND EXISTS (SELECT 1 FROM ContentBlocks c WHERE c.note_id = n.note_id "
                           "AND c.block_type = \'%1\') ").arg(blockType);
        }
    }
    
    // 添加排序
    switch (sortType) {
        case 1: // 创建时间
            sql += "ORDER BY n.created_at DESC ";
            break;
        case 2: // 按名称排序
            sql += "ORDER BY n.title COLLATE NOCASE ";
            break;
        default: // 最近修改
            sql += "ORDER BY n.updated_at DESC ";
            break;
    }
    
    QSqlQuery query;
    query.prepare(sql);
    // 只有当关键词非空时才绑定
    if (!keyword.isEmpty()) {
        query.bindValue(":keyword", "%" + keyword + "%");
    }
    
    if (query.exec()) {
        while (query.next()) {
            SearchResultInfo info;
            info.id = query.value("note_id").toInt();
            info.title = query.value("title").toString();
            info.created_at = query.value("created_at").toString();
            info.updated_at = query.value("updated_at").toString();
            info.folder_id = query.value("folder_id").toInt();
            
            // 构建路径
            QString folderPath = query.value("folder_path").toString();
            info.path = QString("/note_%1").arg(info.id);
            if (!folderPath.isEmpty() && folderPath != "/root") {
                info.path = folderPath + info.path;
            }
            
            // 获取预览文本
            QString preview = query.value("preview").toString();
            // 限制预览文本长度
            if (preview.length() > 150) {
                preview = preview.left(150) + "...";
            }
            info.previewText = preview;
            
            results.append(info);
        }
    } else {
        qCritical() << "搜索笔记失败:" << query.lastError().text();
        qCritical() << "SQL:" << query.lastQuery(); // 输出失败的SQL语句
    }
    
    qDebug() << "DatabaseManager::searchNotes - Keyword:" << keyword << "Results count:" << results.size(); // 添加日志
    return results;
} 