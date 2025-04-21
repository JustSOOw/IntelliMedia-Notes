/*
 * @Author: Furdow wang22338014@gmail.com
 * @Date: 2025-04-21 18:00:00
 * @LastEditors: Furdow wang22338014@gmail.com
 * @LastEditTime: 2025-04-21 20:30:01
 * @FilePath: \IntelliMedia_Notes\src\searchmanager.h
 * @Description: 搜索管理器类
 * 
 * Copyright (c) 2025 by Furdow, All Rights Reserved. 
 */
#ifndef SEARCHMANAGER_H
#define SEARCHMANAGER_H

#include <QObject>
#include <QtQuickWidgets/QQuickWidget>
#include <QtQml/QQmlContext>
#include <QtQuick/QQuickItem>
#include <QVariant>
#include <QVariantList>
#include <QDialog>
#include <QPoint>
#include "databasemanager.h" // 包含数据库管理器

/**
 * @brief 搜索管理器类，处理搜索功能和界面交互
 */
class SearchManager : public QObject
{
    Q_OBJECT

public:
    explicit SearchManager(DatabaseManager *dbManager, QObject *parent = nullptr);
    ~SearchManager();

    /**
     * @brief 显示搜索对话框
     */
    void showSearchDialog();
    
    /**
     * @brief 搜索笔记
     * @param keyword 搜索关键词
     * @param dateFilter 日期筛选类型: 0-全部时间, 1-今天, 2-最近一周, 3-最近一月
     * @param contentType 内容类型筛选: 0-全部类型, 1-文本, 2-图片, 3-列表
     * @param sortType 排序方式: 0-最近修改, 1-创建时间, 2-按名称排序
     */
    Q_INVOKABLE void searchNotes(
        const QString &keyword, 
        int dateFilter = 0, 
        int contentType = 0, 
        int sortType = 0
    );

public slots:
    /**
     * @brief 处理搜索对话框关闭
     */
    void onDialogClosed();
    
    /**
     * @brief 处理选中笔记
     * @param path 笔记路径
     * @param type 笔记类型
     */
    void onNoteSelected(const QString &path, const QString &type);
    
    // --- 用于窗口拖动的槽函数 ---
    Q_INVOKABLE void handleTitleBarPressed(int globalX, int globalY);
    Q_INVOKABLE void handleTitleBarMoved(int globalX, int globalY);
    Q_INVOKABLE void handleTitleBarReleased();
    // ---------------------------

private slots:
    /**
     * @brief 处理底层QDialog关闭完成的信号
     */
    void handleDialogFinished(int result);

signals:
    /**
     * @brief 选中笔记的信号
     * @param path 笔记路径
     * @param type 笔记类型
     */
    void noteSelected(const QString &path, const QString &type);
    
    /**
     * @brief 搜索结果更新信号
     * @param results 搜索结果列表
     */
    void searchResultsUpdated(const QVariantList &results);

    /**
     * @brief 搜索对话框已关闭信号
     */
    void searchClosed();

private:
    DatabaseManager *m_dbManager; // 数据库管理器
    QDialog *m_searchDialog = nullptr; // 搜索对话框
    QQuickWidget *m_searchWidget = nullptr; // QML搜索界面容器
    QQuickItem *m_rootObject = nullptr; // QML根对象
    
    // --- 用于窗口拖动的成员变量 ---
    bool m_dragging = false;
    QPoint m_dragStartPos;
    // ---------------------------
    
    /**
     * @brief 将SearchResultInfo对象转换为QML可用的QVariantMap
     * @param result 搜索结果
     * @return QVariantMap QML可用的结果数据
     */
    QVariantMap resultToQML(const SearchResultInfo &result);
};

#endif // SEARCHMANAGER_H 