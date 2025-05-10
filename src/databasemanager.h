/*
 * @Author: Furdow wang22338014@gmail.com
 * @Date: 2025-04-17 12:00:00
 * @LastEditors: Furdow wang22338014@gmail.com
 * @LastEditTime: 2025-04-17 12:00:00
 * @FilePath: \IntelliMedia_Notes\src\databasemanager.h
 * @Description: 数据库管理类
 * 
 * Copyright (c) 2025 by Furdow, All Rights Reserved. 
 */
#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QDir>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QDebug>
#include <QUuid>
#include <QDateTime>
#include <QFileInfo>

// 笔记结构类型声明
struct NoteInfo {
    int id;
    QString title;
    QString created_at;
    QString updated_at;
    int folder_id;
    QString tags;
    bool is_trashed;
};

// 内容块类型声明
struct ContentBlock {
    int id;
    int note_id;
    QString block_type;
    int position;
    QString content_text;
    QString media_path;
    QString properties;
};

// 图片标注类型声明
struct Annotation {
    int id;
    int block_id;
    QString annotation_type;
    QString data; // JSON格式
    QString created_at;
};

// 文件夹结构类型声明
struct FolderInfo {
    int id;
    QString name;
    int parent_id;
    QString path;
    QString created_at;
};

// 搜索结果结构
struct SearchResultInfo {
    int id;
    QString title;
    QString previewText;
    QString path;
    QString created_at;
    QString updated_at;
    int folder_id;
};

/**
 * @brief 数据库管理类，处理SQLite数据库操作
 */
class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    /**
     * @brief 初始化数据库连接和表结构
     * @return bool 是否成功初始化
     */
    bool initialize();

    /**
     * @brief 获取所有文件夹
     * @return QList<FolderInfo> 文件夹列表
     */
    QList<FolderInfo> getAllFolders();

    /**
     * @brief 获取指定文件夹下的所有笔记
     * @param folder_id 文件夹ID
     * @return QList<NoteInfo> 笔记列表
     */
    QList<NoteInfo> getNotesInFolder(int folder_id);

    /**
     * @brief 根据ID获取笔记信息
     * @param note_id 笔记ID
     * @return NoteInfo 笔记信息
     */
    NoteInfo getNoteById(int note_id);

    /**
     * @brief 创建新文件夹
     * @param name 文件夹名称
     * @param parent_id 父文件夹ID，0表示根目录
     * @return int 新创建的文件夹ID，失败返回-1
     */
    int createFolder(const QString &name, int parent_id = 0);

    /**
     * @brief 创建新笔记
     * @param title 笔记标题
     * @param folder_id 所属文件夹ID
     * @return int 新创建的笔记ID，失败返回-1
     */
    int createNote(const QString &title, int folder_id = 0);

    /**
     * @brief 删除文件夹
     * @param folder_id 文件夹ID
     * @return bool 是否成功删除
     */
    bool deleteFolder(int folder_id);

    /**
     * @brief 删除笔记(移到回收站)
     * @param note_id 笔记ID
     * @return bool 是否成功删除
     */
    bool moveNoteToTrash(int note_id);

    /**
     * @brief 永久删除笔记
     * @param note_id 笔记ID
     * @return bool 是否成功删除
     */
    bool deleteNote(int note_id);

    /**
     * @brief 重命名文件夹
     * @param folder_id 文件夹ID
     * @param new_name 新名称
     * @return bool 是否成功重命名
     */
    bool renameFolder(int folder_id, const QString &new_name);

    /**
     * @brief 重命名笔记
     * @param note_id 笔记ID
     * @param new_title 新标题
     * @return bool 是否成功重命名
     */
    bool renameNote(int note_id, const QString &new_title);

    /**
     * @brief 移动笔记到指定文件夹
     * @param note_id 笔记ID
     * @param folder_id 目标文件夹ID
     * @return bool 是否成功移动
     */
    bool moveNote(int note_id, int folder_id);

    /**
     * @brief 获取笔记内容块
     * @param note_id 笔记ID
     * @return QList<ContentBlock> 内容块列表
     */
    QList<ContentBlock> getNoteContent(int note_id);

    /**
     * @brief 获取图片块的所有标注
     * @param block_id 图片块ID
     * @return QList<Annotation> 标注列表
     */
    QList<Annotation> getImageAnnotations(int block_id);

    /**
     * @brief 保存笔记内容
     * @param note_id 笔记ID
     * @param blocks 内容块列表
     * @return bool 是否成功保存
     */
    bool saveNoteContent(int note_id, const QList<ContentBlock> &blocks);

    /**
     * @brief 保存图片标注
     * @param block_id 图片块ID
     * @param annotations 标注列表
     * @return bool 是否成功保存
     */
    bool saveImageAnnotations(int block_id, const QList<Annotation> &annotations);

    /**
     * @brief 导入图片到媒体文件夹
     * @param source_path 源图片路径
     * @return QString 导入后的相对路径
     */
    QString importImageToMedia(const QString &source_path);

    /**
     * @brief 获取媒体文件的绝对路径
     * @param relative_path 相对路径
     * @return QString 绝对路径
     */
    QString getMediaAbsolutePath(const QString &relative_path);

    /**
     * @brief 清理未使用的媒体文件
     * @return int 清理的文件数量
     */
    int cleanUnusedMediaFiles();

    /**
     * @brief 搜索笔记
     * @param keyword 搜索关键词
     * @param dateFilter 日期筛选类型: 0-全部时间, 1-今天, 2-最近一周, 3-最近一月
     * @param contentType 内容类型筛选: 0-全部类型, 1-文本, 2-图片, 3-列表
     * @param sortType 排序方式: 0-最近修改, 1-创建时间, 2-按名称排序
     * @return QList<SearchResultInfo> 搜索结果
     */
    QList<SearchResultInfo> searchNotes(
        const QString &keyword, 
        int dateFilter = 0, 
        int contentType = 0, 
        int sortType = 0
    );

private:
    QSqlDatabase m_db; // 数据库连接
    QString m_dbPath; // 数据库文件路径
    QString m_mediaPath; // 媒体文件夹路径

    /**
     * @brief 创建数据库表
     * @return bool 是否成功创建表
     */
    bool createTables();

    /**
     * @brief 执行SQL语句
     * @param query SQL查询对象
     * @param sql SQL语句
     * @return bool 是否成功执行
     */
    bool executeQuery(QSqlQuery &query, const QString &sql);

    /**
     * @brief 检查表是否存在
     * @param tableName 表名
     * @return bool 表是否存在
     */
    bool tableExists(const QString &tableName);

    /**
     * @brief 更新笔记的最后修改时间
     * @param note_id 笔记ID
     * @return bool 是否成功更新
     */
    bool updateNoteTimestamp(int note_id);

    /**
     * @brief 生成唯一文件名
     * @param original_name 原始文件名
     * @return QString 新的唯一文件名
     */
    QString generateUniqueFilename(const QString &original_name);
};

#endif // DATABASEMANAGER_H 