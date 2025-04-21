/*
 * @Author: Furdow wang22338014@gmail.com
 * @Date: 2025-04-21 18:00:00
 * @LastEditors: Furdow wang22338014@gmail.com
 * @LastEditTime: 2025-04-21 18:39:35
 * @FilePath: \IntelliMedia_Notes\src\searchmanager.cpp
 * @Description: 搜索管理器实现
 * 
 * Copyright (c) 2025 by Furdow, All Rights Reserved. 
 */
#include "searchmanager.h"

#include <QScreen>
#include <QApplication>
#include <QVBoxLayout>
#include <QDebug>

SearchManager::SearchManager(DatabaseManager *dbManager, SidebarManager *sidebarManager, QObject *parent)
    : QObject(parent), m_dbManager(dbManager), m_sidebarManager(sidebarManager)
{
}

SearchManager::~SearchManager()
{
    if (m_searchDialog) {
        m_searchDialog->close();
        delete m_searchDialog;
    }
}

void SearchManager::showSearchDialog()
{
    // 如果已有搜索对话框，则直接显示
    if (m_searchDialog) {
        // 清空搜索结果
        if (m_rootObject) {
            QMetaObject::invokeMethod(m_rootObject, "resetSearch");
        }
        
        m_searchDialog->show();
        m_searchDialog->raise();
        m_searchDialog->activateWindow();
        // 显示时加载所有笔记
        searchNotes(""); 
        return;
    }
    
    // 创建无边框对话框
    m_searchDialog = new QDialog(nullptr, Qt::Dialog | Qt::FramelessWindowHint );
    m_searchDialog->setAttribute(Qt::WA_TranslucentBackground); // 允许透明背景用于圆角
    // 设置固定大小
    m_searchDialog->setFixedSize(800, 600); 
    
    // 居中显示
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        m_searchDialog->move((screenGeometry.width() - m_searchDialog->width()) / 2,
                           (screenGeometry.height() - m_searchDialog->height()) / 2);
    }
    
    // 创建QML搜索界面容器
    m_searchWidget = new QQuickWidget();
    m_searchWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    
    // 设置QML上下文
    QQmlContext *context = m_searchWidget->rootContext();
    context->setContextProperty("searchManager", this);
    context->setContextProperty("sidebarManager", m_sidebarManager);
    
    // 加载QML文件
    m_searchWidget->setSource(QUrl("qrc:///qml/SearchView.qml"));
    
    // 检查是否加载成功
    if (m_searchWidget->status() == QQuickWidget::Error) {
        qCritical() << "加载SearchView.qml失败:" << m_searchWidget->errors();
        delete m_searchWidget;
        delete m_searchDialog;
        m_searchWidget = nullptr;
        m_searchDialog = nullptr;
        return;
    }
    
    // 获取QML根对象
    m_rootObject = m_searchWidget->rootObject();
    qDebug() << "Root object type:" << m_rootObject->metaObject()->className();
    
    // 连接对话框关闭信号
    connect(m_searchDialog, &QDialog::finished, this, &SearchManager::handleDialogFinished);
    
    // 连接QML信号槽
    connect(m_rootObject, SIGNAL(closeSearch()), this, SLOT(onDialogClosed()));
    connect(m_rootObject, SIGNAL(openNote(QString,QString)), this, SLOT(onNoteSelected(QString,QString)));
    
    // 创建布局并设置QML控件
    QVBoxLayout *layout = new QVBoxLayout(m_searchDialog);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_searchWidget);
    m_searchDialog->setLayout(layout);
    
    // 显示对话框并加载初始数据
    m_searchDialog->show();
    searchNotes(""); // 初始加载所有笔记
}

void SearchManager::searchNotes(const QString &keyword, int dateFilter, int contentType, int sortType)
{
    qDebug() << "SearchManager::searchNotes - Calling DB. Keyword:" << keyword;
    if (!m_dbManager) {
        qWarning() << "SearchManager::searchNotes - DatabaseManager is null!";
        return;
    }
    QList<SearchResultInfo> results = m_dbManager->searchNotes(keyword, dateFilter, contentType, sortType);
    qDebug() << "SearchManager::searchNotes - Received" << results.size() << "results from DB. Invoking QML update.";
    QVariantList resultList;
    for (const auto &result : results) {
        resultList.append(resultToQML(result));
    }
    // 每次都获取顶层对象，避免m_rootObject被覆盖
    QQuickItem* root = m_searchWidget ? m_searchWidget->rootObject() : nullptr;
    if (root) {
        qDebug() << "Root object type (searchNotes):" << root->metaObject()->className();
        bool ok = QMetaObject::invokeMethod(root, "updateSearchResults", Q_ARG(QVariant, QVariant::fromValue(resultList)));
        qDebug() << "invokeMethod updateSearchResults result:" << ok;
    } else {
        qWarning() << "SearchManager::searchNotes - QML root object is null!";
    }
}

void SearchManager::onDialogClosed()
{
    qDebug() << "onDialogClosed called";
    if (m_searchDialog) {
        m_searchDialog->deleteLater();
        m_searchDialog = nullptr;
        m_searchWidget = nullptr;
        m_rootObject = nullptr;
    }
}

void SearchManager::onNoteSelected(const QString &path, const QString &type)
{
    emit noteSelected(path, type);
}

QVariantMap SearchManager::resultToQML(const SearchResultInfo &result)
{
    QVariantMap map;
    map["id"] = result.id;
    map["title"] = result.title;
    map["previewText"] = result.previewText;
    map["path"] = result.path;
    map["createdAt"] = result.created_at;
    map["updatedAt"] = result.updated_at;
    map["folderId"] = result.folder_id;
    return map;
}

// 添加处理对话框完成的槽函数
void SearchManager::handleDialogFinished(int result)
{
    Q_UNUSED(result);
    emit searchClosed(); // 发出关闭信号
}

// --- 窗口拖动处理 ---
void SearchManager::handleTitleBarPressed(int globalX, int globalY)
{
    if (m_searchDialog) {
        m_dragging = true;
        // 记录鼠标按下时相对于窗口左上角的位置
        m_dragStartPos = m_searchDialog->mapFromGlobal(QPoint(globalX, globalY));
    }
}

void SearchManager::handleTitleBarMoved(int globalX, int globalY)
{
    if (m_searchDialog && m_dragging) {
        // 移动窗口到新的全局位置，减去初始偏移
        m_searchDialog->move(QPoint(globalX, globalY) - m_dragStartPos);
    }
}

void SearchManager::handleTitleBarReleased()
{
    m_dragging = false;
}
// ------------------ 