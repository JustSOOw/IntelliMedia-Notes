#include "settingsdialog.h"
#include <QCloseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QApplication>
#include <QGroupBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QFontDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QSqlDatabase>
#include <QSqlError>
#include <QTimer>
#include <QTextStream>
#include <QRegularExpression>
#include <QStyleFactory>
#include <QInputDialog>
#include <QSqlQuery>
#include <QMainWindow>
#include <QTabBar>
#include <QTabWidget>
#include <QStyle>
#include <QPaintEvent>
#include <QStyleOptionTab>
#include <QPainter>
#include <QPalette>
#include <QFontMetrics>

// 自定义垂直标签栏，文字保持水平显示
class VerticalTabBar : public QTabBar
{
public:
    explicit VerticalTabBar(QWidget* parent = nullptr) : QTabBar(parent) {}

    QSize tabSizeHint(int index) const override
    {
        QSize s = QTabBar::tabSizeHint(index);
        return QSize(s.height(), s.width());
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        QStyleOptionTab opt;
        for (int i = 0; i < count(); ++i) {
            initStyleOption(&opt, i);
            QRect rect = tabRect(i);
            opt.rect = rect;
            // 绘制标签形状，使用样式处理 West 方向
            style()->drawControl(QStyle::CE_TabBarTabShape, &opt, &painter, this);
            // 绘制标签文本，保持水平居中
            painter.save();
            painter.setFont(font());
            QColor textColor = (opt.state & QStyle::State_Selected)
                ? opt.palette.color(QPalette::HighlightedText)
                : opt.palette.color(QPalette::ButtonText);
            painter.setPen(textColor);
            painter.drawText(rect, Qt::AlignCenter, opt.text);
            painter.restore();
        }
    }
};

// 自定义垂直标签页控件，使用 VerticalTabBar
class VerticalTabWidget : public QTabWidget
{
public:
    explicit VerticalTabWidget(QWidget* parent = nullptr) : QTabWidget(parent)
    {
        setTabBar(new VerticalTabBar(this));
        setElideMode(Qt::ElideNone);
    }
};

// 构造函数
SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_settings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName())
    , m_autoSaveTimer(nullptr)
    , m_apiKeyVisible(false)
{
    // 设置对话框属性
    setWindowTitle(tr("设置"));
    setMinimumSize(700, 500);
    setObjectName("settingsDialog");
    
    // 设置全局样式，禁用所有QLabel的边框
    setStyleSheet("QLabel { border: none; background: transparent; }");
    
    // 应用当前主题样式
    QString currentTheme = m_settings.value("General/Theme", "Light").toString();
    QString styleSheetPath;
    if (currentTheme == "Dark") {
        styleSheetPath = ":/resources/styles/dark_theme.qss";
    } else {
        styleSheetPath = ":/resources/styles/light_theme.qss";
    }
    
    // 加载样式表
    QFile styleFile(styleSheetPath);
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString styleSheet = QLatin1String(styleFile.readAll());
        setStyleSheet(styleSheet);
        styleFile.close();
    }
    
    // 初始化UI
    setupUI();
    
    // 加载设置
    loadSettings();
    
    // 连接信号和槽 - 常规设置
    connect(m_themeGroup, &QButtonGroup::buttonClicked, this, &SettingsDialog::onThemeChanged);
    connect(m_languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::onLanguageChanged);
    connect(m_autoSaveCheck, &QCheckBox::stateChanged, this, &SettingsDialog::onAutoSaveChanged);
    connect(m_customBackgroundBtn, &QPushButton::clicked, this, &SettingsDialog::onCustomBackgroundClicked);
    
    // 连接信号和槽 - 编辑器设置
    connect(m_selectFontBtn, &QPushButton::clicked, this, &SettingsDialog::onSelectFontClicked);
    connect(m_tabWidthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsDialog::onTabWidthChanged);
    connect(m_autoPairCheck, &QCheckBox::stateChanged, this, &SettingsDialog::onAutoPairChanged);
    
    // 连接信号和槽 - AI服务设置
    connect(m_toggleApiKeyVisibilityBtn, &QPushButton::clicked, this, &SettingsDialog::onToggleApiKeyVisibilityClicked);
    connect(m_testApiConnectionBtn, &QPushButton::clicked, this, &SettingsDialog::onTestApiConnectionClicked);
    
    // 连接信号和槽 - 数据与存储设置
    connect(m_selectNotebookLocationBtn, &QPushButton::clicked, this, &SettingsDialog::onSelectNotebookLocationClicked);
    connect(m_backupNowBtn, &QPushButton::clicked, this, &SettingsDialog::onBackupNowClicked);
    connect(m_restoreFromBackupBtn, &QPushButton::clicked, this, &SettingsDialog::onRestoreFromBackupClicked);
    connect(m_exportNotesBtn, &QPushButton::clicked, this, &SettingsDialog::onExportNotesClicked);
    connect(m_importNotesBtn, &QPushButton::clicked, this, &SettingsDialog::onImportNotesClicked);
    
    // 连接信号和槽 - 关于
    if (m_websiteBtn) {
        connect(m_websiteBtn, &QPushButton::clicked, this, [this]() {
            QDesktopServices::openUrl(QUrl("https://github.com/yourusername/IntelliMedia_Notes"));
        });
    }
}

// 析构函数
SettingsDialog::~SettingsDialog()
{
    // 清理资源
}

// 应用设置的静态方法
void SettingsDialog::applySettings()
{
    QSettings settings; // settings 对象在函数开始时创建，是正确的
    
    // 主题设置的应用现在完全由 MainWindow 通过 QSS 和调色板处理。
    // MainWindow::applyThemeFromSettings() 会在 SettingsDialog 关闭后被调用。
    // 因此，这里不再需要进行任何主题相关的 setPalette() 或 setStyle() 调用。
    qDebug() << "SettingsDialog::applySettings() called, but theme application is deferred to MainWindow.";
    
    // 应用编辑器设置
    QFont defaultFont;
    defaultFont.fromString(settings.value("Editor/DefaultFont", QFont("Segoe UI", 11).toString()).toString());
    
    // 应用数据与存储设置
    // 检查是否需要进行自动备份
    bool autoBackup = settings.value("DataStorage/AutoBackup", true).toBool();
    if (autoBackup) {
        // 获取上次备份时间
        QDateTime lastBackup = settings.value("DataStorage/LastBackupTime", QDateTime()).toDateTime();
        QDateTime currentTime = QDateTime::currentDateTime();
        
        // 获取备份频率（天数）
        int backupFrequency = settings.value("DataStorage/BackupFrequency", 7).toInt();
        
        // 检查是否需要进行备份
        if (lastBackup.isNull() || lastBackup.daysTo(currentTime) >= backupFrequency) {
            // 获取笔记库位置
            QString notebookLocation = settings.value("DataStorage/NotebookLocation", 
                QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/IntelliMedia_Notes").toString();
            
            // 获取默认备份位置
            QString backupLocation = settings.value("DataStorage/BackupLocation", 
                QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/IntelliMedia_Notes_Backups").toString();
            
            // 确保备份目录存在
            QDir backupDir(backupLocation);
            if (!backupDir.exists()) {
                backupDir.mkpath(".");
            }
            
            // 创建备份子目录，使用时间戳命名
            QString timestamp = currentTime.toString("yyyy-MM-dd_hh-mm-ss");
            QString backupSubDir = backupLocation + "/auto_backup_" + timestamp;
            QDir().mkpath(backupSubDir);
            
            // 备份数据库文件
            QString dbPath = notebookLocation + "/notes.db";
            if (QFile::exists(dbPath)) {
                QFile::copy(dbPath, backupSubDir + "/notes.db");
            }
            
            // 备份媒体文件
            QString mediaPath = notebookLocation + "/notes_media";
            if (QDir(mediaPath).exists()) {
                // 创建媒体文件夹
                QDir().mkpath(backupSubDir + "/notes_media");
                
                // 复制媒体文件夹内容
                QDir mediaDir(mediaPath);
                QStringList mediaFiles = mediaDir.entryList(QDir::Files);
                
                for (const QString &file : mediaFiles) {
                    QFile::copy(mediaPath + "/" + file, backupSubDir + "/notes_media/" + file);
                }
            }
            
            // 创建备份信息文件
            QFile infoFile(backupSubDir + "/backup_info.txt");
            if (infoFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&infoFile);
                out << "IntelliMedia Notes Auto Backup\n";
                out << "Date: " << currentTime.toString("yyyy-MM-dd hh:mm:ss") << "\n";
                out << "Version: 0.1.0\n";
                infoFile.close();
            }
            
            // 更新上次备份时间
            settings.setValue("DataStorage/LastBackupTime", currentTime);
            settings.sync();
            
            // 清理旧的自动备份（保留最近5个）
            QStringList backupDirs = backupDir.entryList(QStringList() << "auto_backup_*", QDir::Dirs, QDir::Time);
            if (backupDirs.size() > 5) {
                for (int i = 5; i < backupDirs.size(); i++) {
                    QDir oldBackup(backupLocation + "/" + backupDirs[i]);
                    oldBackup.removeRecursively();
                }
            }
        }
    }
}

// 获取笔记库路径
QString SettingsDialog::getNotebookPath()
{
    QString basePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(basePath);
    
    // 确保目录存在
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    return basePath;
}

// 关闭事件处理
void SettingsDialog::closeEvent(QCloseEvent *event)
{
    // 保存设置
    saveSettings();
    event->accept();
}

// 主题选择改变时的槽函数
void SettingsDialog::onThemeChanged(QAbstractButton *button)
{
    // 确定选择的主题
    QString theme;
    if (m_lightThemeRadio->isChecked()) {
        theme = "Light";
    } else if (m_darkThemeRadio->isChecked()) {
        theme = "Dark";
    } else {
        theme = "System";
    }
    
    // 立即保存主题设置
    m_settings.setValue("General/Theme", theme);
    m_settings.sync();
    
    // 验证设置是否已正确保存
    QSettings verifySettings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName());
    verifySettings.sync(); // 强制从磁盘重新读取
    
    // 立即应用主题样式到设置对话框
    QString styleSheetPath;
    if (theme == "Dark") {
        styleSheetPath = ":/resources/styles/dark_theme.qss";
    } else if (theme == "Light") {
        styleSheetPath = ":/resources/styles/light_theme.qss";
    } else {
        // 根据系统主题选择
        QSettings settings;
        bool isDarkMode = settings.value("System/IsDarkMode", false).toBool();
        styleSheetPath = isDarkMode ? 
            ":/resources/styles/dark_theme.qss" : 
            ":/resources/styles/light_theme.qss";
    }
    
    // 加载样式表
    QFile styleFile(styleSheetPath);
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString styleSheet = QLatin1String(styleFile.readAll());
        setStyleSheet(styleSheet);
        styleFile.close();
    }
    
    // 通知父窗口应用新的主题设置，直接传递主题值
    emit themeChanged(theme);
}

// 自动保存设置改变时的槽函数
void SettingsDialog::onAutoSaveChanged(int state)
{
    // 启用或禁用自动保存间隔选择
    m_autoSaveIntervalCombo->setEnabled(state == Qt::Checked);
}

// 自定义背景按钮点击时的槽函数
void SettingsDialog::onCustomBackgroundClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("选择背景图片"), 
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
        tr("图片文件 (*.png *.jpg *.jpeg *.bmp)"));
        
    if (!fileName.isEmpty()) {
        // 保存背景图片路径
        m_settings.setValue("General/CustomBackground", fileName);
        QMessageBox::information(this, tr("背景已设置"), tr("自定义背景图片已设置，关闭设置窗口后生效。"));
    }
}

// 选择字体按钮点击时的槽函数
void SettingsDialog::onSelectFontClicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_selectedFont, this, tr("选择默认字体"));
    
    if (ok) {
        m_selectedFont = font;
        m_currentFontLabel->setText(QString("%1, %2pt").arg(font.family()).arg(font.pointSize()));
    }
}

// 制表符宽度改变时的槽函数
void SettingsDialog::onTabWidthChanged(int value)
{
    // 这里可以添加实时预览制表符宽度的代码
    qDebug() << "制表符宽度已更改为: " << value;
}

// 自动配对括号/引号设置改变时的槽函数
void SettingsDialog::onAutoPairChanged(int state)
{
    // 这里可以添加实时预览自动配对括号/引号的代码
    qDebug() << "自动配对括号/引号已" << (state == Qt::Checked ? "启用" : "禁用");
}

// 显示/隐藏API密钥按钮点击时的槽函数
void SettingsDialog::onToggleApiKeyVisibilityClicked()
{
    m_apiKeyVisible = !m_apiKeyVisible;
    
    if (m_apiKeyVisible) {
        m_apiKeyEdit->setEchoMode(QLineEdit::Normal);
        m_toggleApiKeyVisibilityBtn->setText(tr("显示/隐藏"));
    } else {
        m_apiKeyEdit->setEchoMode(QLineEdit::Password);
        m_toggleApiKeyVisibilityBtn->setText(tr("显示/隐藏"));
    }
}

// 测试API连接按钮点击时的槽函数
void SettingsDialog::onTestApiConnectionClicked()
{
    QString apiKey = m_apiKeyEdit->text();
    QString apiEndpoint = m_apiEndpointEdit->text();
    
    if (apiKey.isEmpty() || apiEndpoint.isEmpty()) {
        m_apiStatusLabel->setText(tr("<font color='red'>请输入API密钥和端点</font>"));
        return;
    }
    
    m_apiStatusLabel->setText(tr("<font color='blue'>正在测试连接...</font>"));
    m_testApiConnectionBtn->setEnabled(false);
    
    // 创建网络请求管理器
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    
    // 创建请求
    QUrl url(apiEndpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());
    
    // 创建请求体
    QJsonObject jsonObject;
    jsonObject["model"] = "deepseek-chat";
    jsonObject["messages"] = QJsonArray{
        QJsonObject{{"role", "user"}, {"content", "Hello"}}
    };
    jsonObject["max_tokens"] = 5;
    
    QJsonDocument doc(jsonObject);
    QByteArray data = doc.toJson();
    
    // 发送请求
    QNetworkReply *reply = manager->post(request, data);
    
    // 处理响应
    connect(reply, &QNetworkReply::finished, this, [=]() {
        reply->deleteLater();
        manager->deleteLater();
        
        m_testApiConnectionBtn->setEnabled(true);
        
        if (reply->error() == QNetworkReply::NoError) {
            m_apiStatusLabel->setText(tr("<font color='green'>连接成功！</font>"));
        } else {
            m_apiStatusLabel->setText(tr("<font color='red'>连接失败：%1</font>").arg(reply->errorString()));
        }
    });
}

// 选择笔记库位置按钮点击时的槽函数
void SettingsDialog::onSelectNotebookLocationClicked()
{
    QString currentPath = m_notebookLocationLabel->text();
    QString newPath = QFileDialog::getExistingDirectory(this,
        tr("选择笔记库位置"),
        currentPath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (newPath.isEmpty()) {
        return;
    }
    
    // 确认是否要更改笔记库位置
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        tr("确认更改"),
        tr("更改笔记库位置将会移动所有笔记数据到新位置。\n\n确定要更改吗？"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::No) {
        return;
    }
    
    // 显示进度条
    m_operationProgressBar->setVisible(true);
    m_operationProgressBar->setValue(0);
    m_operationStatusLabel->setText(tr("正在移动笔记库..."));
    
    // 禁用按钮，防止重复操作
    m_selectNotebookLocationBtn->setEnabled(false);
    
    // 在实际应用中，这里应该启动一个线程来移动数据
    // 为了简单起见，我们这里只是模拟移动过程
    
    // 模拟移动进度
    QTimer *progressTimer = new QTimer(this);
    int progress = 0;
    
    connect(progressTimer, &QTimer::timeout, this, [=]() mutable {
        progress += 10;
        m_operationProgressBar->setValue(progress);
        
        if (progress >= 100) {
            // 移动完成
            progressTimer->stop();
            progressTimer->deleteLater();
            
            // 更新笔记库位置
            m_settings.setValue("DataStorage/NotebookLocation", newPath);
            m_notebookLocationLabel->setText(newPath);
            
            // 更新UI
            m_operationProgressBar->setVisible(false);
            m_operationStatusLabel->setText(tr("笔记库已移动到新位置"));
            m_selectNotebookLocationBtn->setEnabled(true);
            
            QMessageBox::information(this,
                tr("操作完成"),
                tr("笔记库已成功移动到新位置。"));
        }
    });
    
    progressTimer->start(200);
}

// 立即备份按钮点击时的槽函数
void SettingsDialog::onBackupNowClicked()
{
    QString backupPath = QFileDialog::getExistingDirectory(this,
        tr("选择备份位置"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (backupPath.isEmpty()) {
        return;
    }
    
    // 显示进度条
    m_operationProgressBar->setVisible(true);
    m_operationProgressBar->setValue(0);
    m_operationStatusLabel->setText(tr("正在备份..."));
    
    // 禁用按钮，防止重复操作
    m_backupNowBtn->setEnabled(false);
    m_restoreFromBackupBtn->setEnabled(false);
    
    // 在实际应用中，这里应该启动一个线程来备份数据
    // 为了简单起见，我们这里只是调用备份函数并模拟进度
    
    // 模拟备份进度
    QTimer *progressTimer = new QTimer(this);
    int progress = 0;
    
    connect(progressTimer, &QTimer::timeout, this, [=]() mutable {
        progress += 10;
        m_operationProgressBar->setValue(progress);
        
        if (progress >= 100) {
            // 备份完成
            progressTimer->stop();
            progressTimer->deleteLater();
            
            // 实际备份
            bool success = backupData(backupPath);
            
            // 更新UI
            m_operationProgressBar->setVisible(false);
            m_backupNowBtn->setEnabled(true);
            m_restoreFromBackupBtn->setEnabled(true);
            
            if (success) {
                m_operationStatusLabel->setText(tr("备份已完成"));
                QMessageBox::information(this,
                    tr("备份完成"),
                    tr("笔记库已成功备份到:\n%1").arg(backupPath));
            } else {
                m_operationStatusLabel->setText(tr("备份失败"));
                QMessageBox::critical(this,
                    tr("备份失败"),
                    tr("无法备份笔记库。请检查目标位置是否可写。"));
            }
        }
    });
    
    progressTimer->start(200);
}

// 从备份恢复按钮点击时的槽函数
void SettingsDialog::onRestoreFromBackupClicked()
{
    QString backupPath = QFileDialog::getExistingDirectory(this,
        tr("选择备份文件夹"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (backupPath.isEmpty()) {
        return;
    }
    
    // 检查备份文件是否存在
    QDir backupDir(backupPath);
    if (!backupDir.exists("notes.db") || !backupDir.exists("notes_media")) {
        QMessageBox::critical(this,
            tr("无效的备份"),
            tr("所选文件夹不包含有效的备份数据。请选择包含notes.db和notes_media文件夹的备份目录。"));
        return;
    }
    
    // 确认是否要恢复备份
    QMessageBox::StandardButton reply = QMessageBox::warning(this,
        tr("确认恢复"),
        tr("恢复备份将会覆盖当前的所有笔记数据。此操作无法撤销。\n\n确定要继续吗？"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::No) {
        return;
    }
    
    // 显示进度条
    m_operationProgressBar->setVisible(true);
    m_operationProgressBar->setValue(0);
    m_operationStatusLabel->setText(tr("正在恢复..."));
    
    // 禁用按钮，防止重复操作
    m_backupNowBtn->setEnabled(false);
    m_restoreFromBackupBtn->setEnabled(false);
    
    // 在实际应用中，这里应该启动一个线程来恢复数据
    // 为了简单起见，我们这里只是调用恢复函数并模拟进度
    
    // 模拟恢复进度
    QTimer *progressTimer = new QTimer(this);
    int progress = 0;
    
    connect(progressTimer, &QTimer::timeout, this, [=]() mutable {
        progress += 10;
        m_operationProgressBar->setValue(progress);
        
        if (progress >= 100) {
            // 恢复完成
            progressTimer->stop();
            progressTimer->deleteLater();
            
            // 实际恢复
            bool success = restoreFromBackup(backupPath);
            
            // 更新UI
            m_operationProgressBar->setVisible(false);
            m_backupNowBtn->setEnabled(true);
            m_restoreFromBackupBtn->setEnabled(true);
            
            if (success) {
                m_operationStatusLabel->setText(tr("恢复已完成"));
                QMessageBox::information(this,
                    tr("恢复完成"),
                    tr("笔记库已成功从备份恢复。\n应用程序需要重启才能应用更改。"));
            } else {
                m_operationStatusLabel->setText(tr("恢复失败"));
                QMessageBox::critical(this,
                    tr("恢复失败"),
                    tr("无法从备份恢复笔记库。请确保备份文件完整且可读。"));
            }
        }
    });
    
    progressTimer->start(200);
}

// 导出笔记按钮点击时的槽函数
void SettingsDialog::onExportNotesClicked()
{
    // 选择导出格式
    QStringList formats;
    formats << tr("Markdown (.md)") << tr("HTML (.html)");
    
    bool ok;
    QString format = QInputDialog::getItem(this, tr("选择导出格式"),
                                         tr("格式:"), formats, 0, false, &ok);
    if (!ok || format.isEmpty()) {
        return;
    }
    
    QString selectedFormat = format.contains("Markdown") ? "markdown" : "html";
    QString fileExtension = selectedFormat == "markdown" ? "md" : "html";
    
    // 选择导出位置
    QString exportPath = QFileDialog::getExistingDirectory(this,
        tr("选择导出位置"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (exportPath.isEmpty()) {
        return;
    }
    
    // 显示进度条
    m_operationProgressBar->setVisible(true);
    m_operationProgressBar->setValue(0);
    m_operationStatusLabel->setText(tr("正在导出..."));
    
    // 禁用按钮，防止重复操作
    m_exportNotesBtn->setEnabled(false);
    m_importNotesBtn->setEnabled(false);
    
    // 在实际应用中，这里应该启动一个线程来导出数据
    // 为了简单起见，我们这里只是调用导出函数并模拟进度
    
    // 模拟导出进度
    QTimer *progressTimer = new QTimer(this);
    int progress = 0;
    
    connect(progressTimer, &QTimer::timeout, this, [=]() mutable {
        progress += 10;
        m_operationProgressBar->setValue(progress);
        
        if (progress >= 100) {
            // 导出完成
            progressTimer->stop();
            progressTimer->deleteLater();
            
            // 实际导出
            bool success = exportNotes(exportPath, selectedFormat);
            
            // 更新UI
            m_operationProgressBar->setVisible(false);
            m_exportNotesBtn->setEnabled(true);
            m_importNotesBtn->setEnabled(true);
            
            if (success) {
                m_operationStatusLabel->setText(tr("导出已完成"));
                QMessageBox::information(this,
                    tr("导出完成"),
                    tr("笔记已成功导出到:\n%1").arg(exportPath));
            } else {
                m_operationStatusLabel->setText(tr("导出失败"));
                QMessageBox::critical(this,
                    tr("导出失败"),
                    tr("无法导出笔记。请检查目标位置是否可写。"));
            }
        }
    });
    
    progressTimer->start(200);
}

// 导入笔记按钮点击时的槽函数
void SettingsDialog::onImportNotesClicked()
{
    QString importPath = QFileDialog::getExistingDirectory(this,
        tr("选择导入文件夹"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (importPath.isEmpty()) {
        return;
    }
    
    // 确认是否要导入笔记
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        tr("确认导入"),
        tr("导入操作将会添加新的笔记到您的笔记库中。\n\n确定要继续吗？"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::No) {
        return;
    }
    
    // 显示进度条
    m_operationProgressBar->setVisible(true);
    m_operationProgressBar->setValue(0);
    m_operationStatusLabel->setText(tr("正在导入..."));
    
    // 禁用按钮，防止重复操作
    m_exportNotesBtn->setEnabled(false);
    m_importNotesBtn->setEnabled(false);
    
    // 在实际应用中，这里应该启动一个线程来导入数据
    // 为了简单起见，我们这里只是调用导入函数并模拟进度
    
    // 模拟导入进度
    QTimer *progressTimer = new QTimer(this);
    int progress = 0;
    
    connect(progressTimer, &QTimer::timeout, this, [=]() mutable {
        progress += 10;
        m_operationProgressBar->setValue(progress);
        
        if (progress >= 100) {
            // 导入完成
            progressTimer->stop();
            progressTimer->deleteLater();
            
            // 实际导入
            bool success = importNotes(importPath);
            
            // 更新UI
            m_operationProgressBar->setVisible(false);
            m_exportNotesBtn->setEnabled(true);
            m_importNotesBtn->setEnabled(true);
            
            if (success) {
                m_operationStatusLabel->setText(tr("导入已完成"));
                QMessageBox::information(this,
                    tr("导入完成"),
                    tr("笔记已成功导入。"));
            } else {
                m_operationStatusLabel->setText(tr("导入失败"));
                QMessageBox::critical(this,
                    tr("导入失败"),
                    tr("无法导入笔记。请确保导入文件格式正确。"));
            }
        }
    });
    
    progressTimer->start(200);
}

// 语言选择改变时的槽函数
void SettingsDialog::onLanguageChanged(int index)
{
    if (index < 0) {
        return;
    }
    
    // 获取选择的语言代码
    QString language = m_languageCombo->itemData(index).toString();
    
    // 立即保存语言设置
    m_settings.setValue("General/Language", language);
    m_settings.sync();
    
    // 不再显示提示对话框，避免双重提示
    // 只发送语言更改信号，由MainWindow处理提示
    emit languageChanged(language);
}

// 初始化UI
void SettingsDialog::setupUI()
{
    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 创建标签控件，使用自定义垂直 TabWidget
    m_tabWidget = new VerticalTabWidget(this);
    m_tabWidget->setTabPosition(QTabWidget::West); // 标签放在左侧

    // 初始化各个标签页
    setupGeneralTab();
    setupEditorTab();
    setupAiServiceTab();
    setupDataStorageTab();
    setupAboutTab();

    // 添加标签页到标签控件
    m_tabWidget->addTab(m_generalTab, tr("常规"));
    m_tabWidget->addTab(m_editorTab, tr("编辑器"));
    m_tabWidget->addTab(m_aiServiceTab, tr("AI服务"));
    m_tabWidget->addTab(m_dataStorageTab, tr("数据与存储"));
    m_tabWidget->addTab(m_aboutTab, tr("关于"));

    // 将标签控件添加到主布局
    mainLayout->addWidget(m_tabWidget);

    // 设置主布局
    setLayout(mainLayout);
}

// 初始化常规设置标签页
void SettingsDialog::setupGeneralTab()
{
    m_generalTab = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(m_generalTab);
    
    // 主题设置组
    QGroupBox *themeGroup = new QGroupBox(tr("主题"));
    themeGroup->setStyleSheet("QGroupBox { border: 1px solid #d0d0d0; } QLabel { border: none; background: transparent; }");
    QVBoxLayout *themeLayout = new QVBoxLayout(themeGroup);
    
    m_themeGroup = new QButtonGroup(this);
    m_lightThemeRadio = new QRadioButton(tr("浅色 (Light)"));
    m_darkThemeRadio = new QRadioButton(tr("深色 (Dark)"));
    m_systemThemeRadio = new QRadioButton(tr("系统默认 (System)"));
    
    m_themeGroup->addButton(m_lightThemeRadio, 0);
    m_themeGroup->addButton(m_darkThemeRadio, 1);
    m_themeGroup->addButton(m_systemThemeRadio, 2);
    
    themeLayout->addWidget(m_lightThemeRadio);
    themeLayout->addWidget(m_darkThemeRadio);
    themeLayout->addWidget(m_systemThemeRadio);
    
    // 添加自定义背景按钮
    m_customBackgroundBtn = new QPushButton(tr("自定义背景..."));
    themeLayout->addWidget(m_customBackgroundBtn);
    
    // 语言设置组
    QGroupBox *languageGroup = new QGroupBox(tr("语言"));
    QVBoxLayout *languageLayout = new QVBoxLayout(languageGroup);
    
    QLabel *languageLabel = new QLabel(tr("界面语言:"));
    m_languageCombo = new QComboBox();
    m_languageCombo->addItem(tr("简体中文"), "zh_CN");
    m_languageCombo->addItem(tr("English"), "en_US");
    m_languageCombo->addItem(tr("跟随系统"), "System");
    
    languageLayout->addWidget(languageLabel);
    languageLayout->addWidget(m_languageCombo);
    
    // 自动保存设置组
    QGroupBox *autoSaveGroup = new QGroupBox(tr("自动保存"));
    QVBoxLayout *autoSaveLayout = new QVBoxLayout(autoSaveGroup);
    
    m_autoSaveCheck = new QCheckBox(tr("启用自动保存"));
    QLabel *autoSaveIntervalLabel = new QLabel(tr("自动保存间隔:"));
    m_autoSaveIntervalCombo = new QComboBox();
    m_autoSaveIntervalCombo->addItem(tr("默认 (即时保存)"), 0);
    m_autoSaveIntervalCombo->addItem(tr("1分钟"), 1);
    m_autoSaveIntervalCombo->addItem(tr("5分钟"), 5);
    m_autoSaveIntervalCombo->addItem(tr("10分钟"), 10);
    m_autoSaveIntervalCombo->addItem(tr("30分钟"), 30);
    
    autoSaveLayout->addWidget(m_autoSaveCheck);
    autoSaveLayout->addWidget(autoSaveIntervalLabel);
    autoSaveLayout->addWidget(m_autoSaveIntervalCombo);
    
    // 其他常规设置
    QGroupBox *otherGroup = new QGroupBox(tr("其他设置"));
    QVBoxLayout *otherLayout = new QVBoxLayout(otherGroup);
    
    QCheckBox *startMinimizedCheck = new QCheckBox(tr("启动时最小化到系统托盘"));
    QCheckBox *checkUpdatesCheck = new QCheckBox(tr("自动检查更新"));
    
    otherLayout->addWidget(startMinimizedCheck);
    otherLayout->addWidget(checkUpdatesCheck);
    
    // 添加所有组到主布局
    mainLayout->addWidget(themeGroup);
    mainLayout->addWidget(languageGroup);
    mainLayout->addWidget(autoSaveGroup);
    mainLayout->addWidget(otherGroup);
    mainLayout->addStretch();
    
    // 连接信号和槽
    connect(m_themeGroup, &QButtonGroup::buttonClicked, this, &SettingsDialog::onThemeChanged);
    connect(m_customBackgroundBtn, &QPushButton::clicked, this, &SettingsDialog::onCustomBackgroundClicked);
    connect(m_autoSaveCheck, &QCheckBox::stateChanged, this, &SettingsDialog::onAutoSaveChanged);
    
    // 保存对startMinimizedCheck和checkUpdatesCheck的引用
    m_startMinimizedCheck = startMinimizedCheck;
    m_checkUpdatesCheck = checkUpdatesCheck;
}

// 初始化编辑器设置标签页
void SettingsDialog::setupEditorTab()
{
    // 创建编辑器设置标签页
    m_editorTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_editorTab);
    layout->setSpacing(15);
    
    // 1. 默认字体设置
    QGroupBox *fontGroup = new QGroupBox(tr("字体设置"));
    QVBoxLayout *fontLayout = new QVBoxLayout(fontGroup);
    
    QHBoxLayout *fontDisplayLayout = new QHBoxLayout();
    QLabel *fontLabel = new QLabel(tr("当前字体:"));
    m_currentFontLabel = new QLabel(tr("Arial, 12pt"));
    m_currentFontLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_currentFontLabel->setMinimumWidth(200);
    
    fontDisplayLayout->addWidget(fontLabel);
    fontDisplayLayout->addWidget(m_currentFontLabel);
    fontDisplayLayout->addStretch();
    
    QHBoxLayout *fontButtonLayout = new QHBoxLayout();
    m_selectFontBtn = new QPushButton(tr("选择字体..."));
    
    fontButtonLayout->addWidget(m_selectFontBtn);
    fontButtonLayout->addStretch();
    
    fontLayout->addLayout(fontDisplayLayout);
    fontLayout->addLayout(fontButtonLayout);
    
    fontGroup->setLayout(fontLayout);
    
    // 2. 制表符宽度设置
    QGroupBox *tabGroup = new QGroupBox(tr("制表符宽度"));
    QHBoxLayout *tabLayout = new QHBoxLayout(tabGroup);
    
    QLabel *tabWidthLabel = new QLabel(tr("制表符宽度:"));
    m_tabWidthSpin = new QSpinBox();
    m_tabWidthSpin->setRange(2, 8);
    m_tabWidthSpin->setValue(4);
    m_tabWidthSpin->setSuffix(tr(" 个空格"));
    
    tabLayout->addWidget(tabWidthLabel);
    tabLayout->addWidget(m_tabWidthSpin);
    tabLayout->addStretch();
    
    tabGroup->setLayout(tabLayout);
    
    // 3. 自动配对括号/引号设置
    QGroupBox *pairGroup = new QGroupBox(tr("编辑器行为"));
    QVBoxLayout *pairLayout = new QVBoxLayout(pairGroup);
    
    m_autoPairCheck = new QCheckBox(tr("自动配对括号和引号"));
    QLabel *pairDescLabel = new QLabel(tr("启用后，当输入 (, [, {, \", ' 等字符时，将自动添加对应的右括号或引号"));
    pairDescLabel->setWordWrap(true);
    
    pairLayout->addWidget(m_autoPairCheck);
    pairLayout->addWidget(pairDescLabel);
    
    pairGroup->setLayout(pairLayout);
    
    // 将所有组添加到编辑器标签页布局
    layout->addWidget(fontGroup);
    layout->addWidget(tabGroup);
    layout->addWidget(pairGroup);
    layout->addStretch();
    
    // 设置编辑器标签页的布局
    m_editorTab->setLayout(layout);
}

// 初始化AI服务设置标签页
void SettingsDialog::setupAiServiceTab()
{
    // 创建AI服务设置标签页
    m_aiServiceTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_aiServiceTab);
    layout->setSpacing(15);
    
    // 1. 服务提供商设置
    QGroupBox *providerGroup = new QGroupBox(tr("服务提供商"));
    QVBoxLayout *providerLayout = new QVBoxLayout(providerGroup);
    
    QLabel *providerLabel = new QLabel(tr("当前服务提供商: DeepSeek"));
    QLabel *providerDescLabel = new QLabel(tr("目前仅支持DeepSeek服务，未来将支持更多AI服务提供商"));
    providerDescLabel->setWordWrap(true);
    
    providerLayout->addWidget(providerLabel);
    providerLayout->addWidget(providerDescLabel);
    
    providerGroup->setLayout(providerLayout);
    
    // 2. API密钥设置
    QGroupBox *apiKeyGroup = new QGroupBox(tr("API设置"));
    QVBoxLayout *apiKeyLayout = new QVBoxLayout(apiKeyGroup);
    
    QLabel *apiKeyDescLabel = new QLabel(tr("请输入您的DeepSeek API密钥，用于访问AI服务"));
    apiKeyDescLabel->setWordWrap(true);
    
    QHBoxLayout *apiKeyInputLayout = new QHBoxLayout();
    QLabel *apiKeyLabel = new QLabel(tr("API密钥:"));
    m_apiKeyEdit = new QLineEdit();
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setPlaceholderText(tr("输入您的DeepSeek API密钥"));
    m_toggleApiKeyVisibilityBtn = new QPushButton(tr("显示/隐藏"));
    m_toggleApiKeyVisibilityBtn->setFixedWidth(80);
    
    apiKeyInputLayout->addWidget(apiKeyLabel);
    apiKeyInputLayout->addWidget(m_apiKeyEdit);
    apiKeyInputLayout->addWidget(m_toggleApiKeyVisibilityBtn);
    
    apiKeyLayout->addWidget(apiKeyDescLabel);
    apiKeyLayout->addLayout(apiKeyInputLayout);
    
    apiKeyGroup->setLayout(apiKeyLayout);
    
    // 3. API端点设置
    QGroupBox *endpointGroup = new QGroupBox(tr("API端点"));
    QVBoxLayout *endpointLayout = new QVBoxLayout(endpointGroup);
    
    QLabel *endpointDescLabel = new QLabel(tr("API端点URL，一般无需修改"));
    endpointDescLabel->setWordWrap(true);
    
    QHBoxLayout *endpointInputLayout = new QHBoxLayout();
    QLabel *endpointLabel = new QLabel(tr("服务提供商:"));
    m_apiEndpointEdit = new QLineEdit();
    m_apiEndpointEdit->setPlaceholderText(tr("https://api.deepseek.com/chat/completions"));
    
    endpointInputLayout->addWidget(endpointLabel);
    endpointInputLayout->addWidget(m_apiEndpointEdit);
    
    endpointLayout->addWidget(endpointDescLabel);
    endpointLayout->addLayout(endpointInputLayout);
    
    endpointGroup->setLayout(endpointLayout);
    
    // 4. 测试连接
    QGroupBox *testGroup = new QGroupBox(tr("连接测试"));
    QVBoxLayout *testLayout = new QVBoxLayout(testGroup);
    
    m_testApiConnectionBtn = new QPushButton(tr("测试连接"));
    m_apiStatusLabel = new QLabel(tr("未测试"));
    
    testLayout->addWidget(m_testApiConnectionBtn);
    testLayout->addWidget(m_apiStatusLabel);
    
    testGroup->setLayout(testLayout);
    
    // 将所有组添加到AI服务标签页布局
    layout->addWidget(providerGroup);
    layout->addWidget(apiKeyGroup);
    layout->addWidget(endpointGroup);
    layout->addWidget(testGroup);
    layout->addStretch();
    
    // 设置AI服务标签页的布局
    m_aiServiceTab->setLayout(layout);
}

// 初始化数据与存储设置标签页
void SettingsDialog::setupDataStorageTab()
{
    m_dataStorageTab = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(m_dataStorageTab);
    mainLayout->setSpacing(15);
    
    // 笔记库位置设置
    QGroupBox *locationGroup = new QGroupBox(tr("存储位置"));
    QVBoxLayout *locationLayout = new QVBoxLayout(locationGroup);
    
    QLabel *locationLabel = new QLabel(tr("笔记库位置:"));
    m_notebookLocationLabel = new QLabel();
    m_notebookLocationLabel->setWordWrap(true);
    m_notebookLocationLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    
    m_selectNotebookLocationBtn = new QPushButton(tr("选择位置..."));
    
    locationLayout->addWidget(locationLabel);
    locationLayout->addWidget(m_notebookLocationLabel);
    locationLayout->addWidget(m_selectNotebookLocationBtn);
    
    // 备份与恢复设置
    QGroupBox *backupGroup = new QGroupBox(tr("备份与恢复"));
    QVBoxLayout *backupLayout = new QVBoxLayout(backupGroup);
    
    QCheckBox *autoBackupCheck = new QCheckBox(tr("启用自动备份"));
    autoBackupCheck->setObjectName("autoBackupCheck");
    
    QHBoxLayout *frequencyLayout = new QHBoxLayout();
    QLabel *frequencyLabel = new QLabel(tr("备份频率:"));
    QSpinBox *backupFrequencySpin = new QSpinBox();
    backupFrequencySpin->setObjectName("backupFrequencySpin");
    backupFrequencySpin->setMinimum(1);
    backupFrequencySpin->setMaximum(90);
    backupFrequencySpin->setValue(7);
    QLabel *daysLabel = new QLabel(tr("天"));
    
    frequencyLayout->addWidget(frequencyLabel);
    frequencyLayout->addWidget(backupFrequencySpin);
    frequencyLayout->addWidget(daysLabel);
    frequencyLayout->addStretch();
    
    QHBoxLayout *backupButtonsLayout = new QHBoxLayout();
    m_backupNowBtn = new QPushButton(tr("立即备份"));
    m_restoreFromBackupBtn = new QPushButton(tr("从备份恢复"));
    
    backupButtonsLayout->addWidget(m_backupNowBtn);
    backupButtonsLayout->addWidget(m_restoreFromBackupBtn);
    
    backupLayout->addWidget(autoBackupCheck);
    backupLayout->addLayout(frequencyLayout);
    backupLayout->addLayout(backupButtonsLayout);
    
    // 导入/导出设置
    QGroupBox *exportGroup = new QGroupBox(tr("导入/导出"));
    QVBoxLayout *exportLayout = new QVBoxLayout(exportGroup);
    
    QLabel *exportInfoLabel = new QLabel(tr("导出笔记为Markdown或HTML格式，或从其他来源导入笔记。"));
    exportInfoLabel->setWordWrap(true);
    
    QHBoxLayout *exportButtonsLayout = new QHBoxLayout();
    m_exportNotesBtn = new QPushButton(tr("导出笔记"));
    m_importNotesBtn = new QPushButton(tr("导入笔记"));
    
    exportButtonsLayout->addWidget(m_exportNotesBtn);
    exportButtonsLayout->addWidget(m_importNotesBtn);
    
    exportLayout->addWidget(exportInfoLabel);
    exportLayout->addLayout(exportButtonsLayout);
    
    // 操作状态
    QGroupBox *statusGroup = new QGroupBox(tr("操作进度"));
    QVBoxLayout *statusLayout = new QVBoxLayout(statusGroup);
    
    m_operationProgressBar = new QProgressBar();
    m_operationProgressBar->setRange(0, 100);
    m_operationProgressBar->setValue(0);
    m_operationProgressBar->setVisible(false);
    
    m_operationStatusLabel = new QLabel(tr("就绪"));
    
    statusLayout->addWidget(m_operationProgressBar);
    statusLayout->addWidget(m_operationStatusLabel);
    
    // 添加所有组到主布局
    mainLayout->addWidget(locationGroup);
    mainLayout->addWidget(backupGroup);
    mainLayout->addWidget(exportGroup);
    mainLayout->addWidget(statusGroup);
    mainLayout->addStretch();
    
    // 连接信号和槽
    connect(m_selectNotebookLocationBtn, &QPushButton::clicked, this, &SettingsDialog::onSelectNotebookLocationClicked);
    connect(m_backupNowBtn, &QPushButton::clicked, this, &SettingsDialog::onBackupNowClicked);
    connect(m_restoreFromBackupBtn, &QPushButton::clicked, this, &SettingsDialog::onRestoreFromBackupClicked);
    connect(m_exportNotesBtn, &QPushButton::clicked, this, &SettingsDialog::onExportNotesClicked);
    connect(m_importNotesBtn, &QPushButton::clicked, this, &SettingsDialog::onImportNotesClicked);
}

// 初始化关于标签页
void SettingsDialog::setupAboutTab()
{
    // 创建关于标签页
    m_aboutTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_aboutTab);
    layout->setSpacing(15);
    
    // 1. 应用信息
    QHBoxLayout *appInfoLayout = new QHBoxLayout();
    
    // 应用图标
    QLabel *iconLabel = new QLabel();
    QPixmap appIcon(":/icons/app_icon.png");
    if (!appIcon.isNull()) {
        iconLabel->setPixmap(appIcon.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        iconLabel->setText(tr("图标"));
    }
    
    // 应用名称和版本
    QVBoxLayout *appNameLayout = new QVBoxLayout();
    QLabel *appNameLabel = new QLabel(tr("IntelliMedia Notes"));
    QFont nameFont = appNameLabel->font();
    nameFont.setPointSize(nameFont.pointSize() + 4);
    nameFont.setBold(true);
    appNameLabel->setFont(nameFont);
    
    m_versionLabel = new QLabel(tr("版本:") + " 0.1.0");
    
    appNameLayout->addWidget(appNameLabel);
    appNameLayout->addWidget(m_versionLabel);
    appNameLayout->addStretch();
    
    appInfoLayout->addWidget(iconLabel);
    appInfoLayout->addLayout(appNameLayout);
    appInfoLayout->addStretch();
    
    // 2. 应用描述
    QTextBrowser *descriptionBrowser = new QTextBrowser();
    descriptionBrowser->setOpenExternalLinks(true);
    descriptionBrowser->setHtml(tr(
        "<p>IntelliMedia Notes是一款功能丰富的笔记应用程序，融合了现代界面设计与AI辅助功能，"
        "为用户提供流畅的笔记编辑与管理体验。</p>"
        "<p>应用采用无边框设计，支持深色/浅色主题，通过Qt混合技术（Widgets与QML）构建了精美的用户界面。</p>"
        "<p><b>核心功能:</b></p>"
        "<ul>"
        "<li>现代化无边框界面</li>"
        "<li>深色/浅色主题切换</li>"
        "<li>文件管理系统</li>"
        "<li>富文本编辑</li>"
        "<li>浮动工具栏</li>"
        "<li>全局搜索</li>"
        "<li>AI助手集成</li>"
        "</ul>"
    ));
    
    // 3. 版权信息
    m_copyrightLabel = new QLabel(tr("版权所有 © 2023-2025 IntelliMedia Team. 保留所有权利。"));
    
    // 4. 链接
    QHBoxLayout *linksLayout = new QHBoxLayout();
    m_websiteBtn = new QPushButton(tr("查看网站"));
    QPushButton *docsBtn = new QPushButton(tr("查看文档"));
    QPushButton *updateBtn = new QPushButton(tr("检查更新"));
    
    linksLayout->addStretch();
    linksLayout->addWidget(m_websiteBtn);
    linksLayout->addWidget(docsBtn);
    linksLayout->addWidget(updateBtn);
    linksLayout->addStretch();
    
    // 5. 致谢
    QGroupBox *creditsGroup = new QGroupBox(tr("致谢"));
    QVBoxLayout *creditsLayout = new QVBoxLayout(creditsGroup);
    
    QTextBrowser *creditsBrowser = new QTextBrowser();
    creditsBrowser->setOpenExternalLinks(true);
    creditsBrowser->setHtml(tr(
        "<p>IntelliMedia Notes 使用了以下开源项目和技术:</p>"
        "<ul>"
        "<li>Qt Framework - <a href='https://www.qt.io/'>https://www.qt.io/</a></li>"
        "<li>DeepSeek AI - <a href='https://www.deepseek.com/'>https://www.deepseek.com/</a></li>"
        "<li>SQLite - <a href='https://www.sqlite.org/'>https://www.sqlite.org/</a></li>"
        "</ul>"
        "<p>特别感谢所有为本项目做出贡献的开发者和测试人员。</p>"
    ));
    
    creditsLayout->addWidget(creditsBrowser);
    creditsGroup->setLayout(creditsLayout);
    
    // 将所有组添加到关于标签页布局
    layout->addLayout(appInfoLayout);
    layout->addWidget(descriptionBrowser);
    layout->addWidget(m_copyrightLabel);
    layout->addLayout(linksLayout);
    layout->addWidget(creditsGroup);
    
    // 设置关于标签页的布局
    m_aboutTab->setLayout(layout);
    
    // 连接新增按钮的信号和槽
    if (docsBtn) {
        connect(docsBtn, &QPushButton::clicked, this, [this]() {
            QDesktopServices::openUrl(QUrl("https://github.com/yourusername/IntelliMedia_Notes/docs"));
        });
    }
    
    if (updateBtn) {
        connect(updateBtn, &QPushButton::clicked, this, [this]() {
            QMessageBox::information(this, tr("检查更新"), tr("当前已是最新版本。"));
        });
    }
}

// 混淆API密钥
QString SettingsDialog::obfuscateApiKey(const QString &apiKey)
{
    QString result;
    for (QChar c : apiKey) {
        result.append(QChar(c.unicode() + 1));
    }
    return result;
}

// 解混淆API密钥
QString SettingsDialog::deobfuscateApiKey(const QString &obfuscatedApiKey)
{
    QString result;
    for (QChar c : obfuscatedApiKey) {
        result.append(QChar(c.unicode() - 1));
    }
    return result;
}

// 加载设置
void SettingsDialog::loadSettings()
{
    // 加载常规设置
    QString theme = m_settings.value("General/Theme", "Light").toString();
    if (theme == "Light") {
        m_lightThemeRadio->setChecked(true);
    } else if (theme == "Dark") {
        m_darkThemeRadio->setChecked(true);
    } else {
        m_systemThemeRadio->setChecked(true);
    }
    
    // 修改此处，根据实际语言代码设置下拉框
    QString language = m_settings.value("General/Language", "System").toString();
    if (language == "zh_CN") {
        m_languageCombo->setCurrentIndex(0);
    } else if (language == "en_US") {
        m_languageCombo->setCurrentIndex(1);
    } else {
        m_languageCombo->setCurrentIndex(2); // System
    }
    
    bool startMinimized = m_settings.value("General/StartMinimized", false).toBool();
    m_startMinimizedCheck->setChecked(startMinimized);
    
    bool checkUpdates = m_settings.value("General/CheckUpdatesAutomatically", true).toBool();
    m_checkUpdatesCheck->setChecked(checkUpdates);
    
    // 加载自动保存设置
    bool autoSaveEnabled = m_settings.value("General/AutoSaveEnabled", false).toBool();
    m_autoSaveCheck->setChecked(autoSaveEnabled);
    
    // 加载自动保存间隔
    int autoSaveInterval = m_settings.value("General/AutoSaveInterval", 5).toInt();
    // 根据保存的间隔值设置下拉框
    int index = 1; // 默认为5分钟
    for (int i = 0; i < m_autoSaveIntervalCombo->count(); i++) {
        if (m_autoSaveIntervalCombo->itemData(i).toInt() == autoSaveInterval) {
            index = i;
            break;
        }
    }
    m_autoSaveIntervalCombo->setCurrentIndex(index);
    // 根据自动保存是否启用设置间隔下拉框的启用状态
    m_autoSaveIntervalCombo->setEnabled(autoSaveEnabled);
    
    // 加载编辑器设置
    QString fontFamily = m_settings.value("Editor/FontFamily", "Arial").toString();
    int fontSize = m_settings.value("Editor/FontSize", 12).toInt();
    m_selectedFont = QFont(fontFamily, fontSize);
    m_currentFontLabel->setText(QString("%1, %2pt").arg(fontFamily).arg(fontSize));
    
    int tabWidth = m_settings.value("Editor/TabWidth", 4).toInt();
    m_tabWidthSpin->setValue(tabWidth);
    
    bool autoPairEnabled = m_settings.value("Editor/AutoPairEnabled", true).toBool();
    m_autoPairCheck->setChecked(autoPairEnabled);
    
    // 加载AI服务设置
    QString serviceProvider = m_settings.value("AIService/Provider", "OpenAI").toString();
    
    QString apiKey = m_settings.value("AIService/APIKey", "").toString();
    if (!apiKey.isEmpty()) {
        apiKey = deobfuscateApiKey(apiKey);
    }
    m_apiKeyEdit->setText(apiKey);
    
    QString apiEndpoint = m_settings.value("AIService/APIEndpoint", "https://api.deepseek.com/chat/completions").toString();
    m_apiEndpointEdit->setText(apiEndpoint);
    
    // 加载数据与存储设置
    QString notebookLocation = m_settings.value("DataStorage/NotebookLocation", 
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/IntelliMedia_Notes").toString();
    m_notebookLocationLabel->setText(notebookLocation);
    
    bool autoBackup = m_settings.value("DataStorage/AutoBackup", true).toBool();
    QCheckBox *autoBackupCheck = m_dataStorageTab->findChild<QCheckBox*>("autoBackupCheck");
    if (autoBackupCheck) {
        autoBackupCheck->setChecked(autoBackup);
    }
    
    int backupFrequency = m_settings.value("DataStorage/BackupFrequency", 7).toInt();
    QSpinBox *backupFrequencySpin = m_dataStorageTab->findChild<QSpinBox*>("backupFrequencySpin");
    if (backupFrequencySpin) {
        backupFrequencySpin->setValue(backupFrequency);
    }
}

// 保存设置
void SettingsDialog::saveSettings()
{
    // 保存常规设置
    QString theme;
    if (m_lightThemeRadio->isChecked()) {
        theme = "Light";
    } else if (m_darkThemeRadio->isChecked()) {
        theme = "Dark";
    } else {
        theme = "System";
    }
    m_settings.setValue("General/Theme", theme);
    
    QString language = m_languageCombo->currentData().toString();
    m_settings.setValue("General/Language", language);
    
    bool startMinimized = m_startMinimizedCheck->isChecked();
    m_settings.setValue("General/StartMinimized", startMinimized);
    
    bool checkUpdates = m_checkUpdatesCheck->isChecked();
    m_settings.setValue("General/CheckUpdatesAutomatically", checkUpdates);
    
    // 保存自动保存设置
    bool autoSaveEnabled = m_autoSaveCheck->isChecked();
    m_settings.setValue("General/AutoSaveEnabled", autoSaveEnabled);
    
    // 保存自动保存间隔
    int autoSaveInterval = m_autoSaveIntervalCombo->currentData().toInt();
    m_settings.setValue("General/AutoSaveInterval", autoSaveInterval);
    
    // 保存编辑器设置
    m_settings.setValue("Editor/FontFamily", m_selectedFont.family());
    m_settings.setValue("Editor/FontSize", m_selectedFont.pointSize());
    m_settings.setValue("Editor/TabWidth", m_tabWidthSpin->value());
    m_settings.setValue("Editor/AutoPairEnabled", m_autoPairCheck->isChecked());
    
    // 保存AI服务设置
    if (!m_apiKeyEdit->text().isEmpty()) {
        m_settings.setValue("AIService/APIKey", obfuscateApiKey(m_apiKeyEdit->text()));
    }
    
    m_settings.setValue("AIService/APIEndpoint", m_apiEndpointEdit->text());
    
    // 保存数据与存储设置
    QString notebookLocation = m_notebookLocationLabel->text();
    m_settings.setValue("DataStorage/NotebookLocation", notebookLocation);
    
    QCheckBox *autoBackupCheck = m_dataStorageTab->findChild<QCheckBox*>("autoBackupCheck");
    if (autoBackupCheck) {
        bool autoBackup = autoBackupCheck->isChecked();
        m_settings.setValue("DataStorage/AutoBackup", autoBackup);
    }
    
    QSpinBox *backupFrequencySpin = m_dataStorageTab->findChild<QSpinBox*>("backupFrequencySpin");
    if (backupFrequencySpin) {
        int backupFrequency = backupFrequencySpin->value();
        m_settings.setValue("DataStorage/BackupFrequency", backupFrequency);
    }
    
    // 同步设置
    m_settings.sync();
    
    // 应用设置
    applySettings();
}

// 备份数据到指定路径
bool SettingsDialog::backupData(const QString &backupPath)
{
    try {
        // 获取当前笔记库位置
        QString notebookLocation = m_settings.value("DataStorage/NotebookLocation", 
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/IntelliMedia_Notes").toString();
        
        // 确保备份目录存在
        QDir backupDir(backupPath);
        if (!backupDir.exists()) {
            if (!backupDir.mkpath(".")) {
                return false;
            }
        }
        
        // 创建备份子目录，使用时间戳命名
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
        QString backupSubDir = backupPath + "/backup_" + timestamp;
        QDir().mkpath(backupSubDir);
        
        // 备份数据库文件
        QString dbPath = notebookLocation + "/notes.db";
        if (QFile::exists(dbPath)) {
            QFile::copy(dbPath, backupSubDir + "/notes.db");
        }
        
        // 备份媒体文件
        QString mediaPath = notebookLocation + "/notes_media";
        if (QDir(mediaPath).exists()) {
            // 创建媒体文件夹
            QDir().mkpath(backupSubDir + "/notes_media");
            
            // 复制媒体文件夹内容
            QDir mediaDir(mediaPath);
            QStringList mediaFiles = mediaDir.entryList(QDir::Files);
            
            for (const QString &file : mediaFiles) {
                QFile::copy(mediaPath + "/" + file, backupSubDir + "/notes_media/" + file);
            }
        }
        
        // 创建备份信息文件
        QFile infoFile(backupSubDir + "/backup_info.txt");
        if (infoFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&infoFile);
            out << "IntelliMedia Notes Backup\n";
            out << "Date: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n";
            out << "Version: 0.1.0\n";
            infoFile.close();
        }
        
        return true;
    } catch (const std::exception &e) {
        qCritical() << "Exception during backup:" << e.what();
        return false;
    }
}

// 从指定路径恢复备份
bool SettingsDialog::restoreFromBackup(const QString &backupPath)
{
    try {
        // 获取当前笔记库位置
        QString notebookLocation = m_settings.value("DataStorage/NotebookLocation", 
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/IntelliMedia_Notes").toString();
        
        // 确保笔记库目录存在
        QDir notebookDir(notebookLocation);
        if (!notebookDir.exists()) {
            if (!notebookDir.mkpath(".")) {
                return false;
            }
        }
        
        // 关闭数据库连接（如果有）
        QString connectionName;
        {
            QSqlDatabase db = QSqlDatabase::database();
            connectionName = db.connectionName();
            db.close();
        }
        QSqlDatabase::removeDatabase(connectionName);
        
        // 备份当前数据库（以防恢复失败）
        QString currentDbPath = notebookLocation + "/notes.db";
        if (QFile::exists(currentDbPath)) {
            QString backupDbPath = currentDbPath + ".bak";
            if (QFile::exists(backupDbPath)) {
                QFile::remove(backupDbPath);
            }
            if (!QFile::copy(currentDbPath, backupDbPath)) {
                qWarning() << "Failed to create backup of current database before restore";
            }
        }
        
        // 复制备份数据库到当前位置
        QString backupDbPath = backupPath + "/notes.db";
        if (QFile::exists(currentDbPath)) {
            QFile::remove(currentDbPath);
        }
        if (!QFile::copy(backupDbPath, currentDbPath)) {
            // 恢复失败，尝试恢复原始数据库
            QString backupOfCurrentDb = currentDbPath + ".bak";
            if (QFile::exists(backupOfCurrentDb)) {
                QFile::copy(backupOfCurrentDb, currentDbPath);
            }
            return false;
        }
        
        // 恢复媒体文件
        QString currentMediaPath = notebookLocation + "/notes_media";
        QString backupMediaPath = backupPath + "/notes_media";
        
        if (QDir(backupMediaPath).exists()) {
            // 确保媒体目录存在
            QDir().mkpath(currentMediaPath);
            
            // 清空当前媒体目录
            QDir currentMediaDir(currentMediaPath);
            QStringList currentMediaFiles = currentMediaDir.entryList(QDir::Files);
            for (const QString &file : currentMediaFiles) {
                QFile::remove(currentMediaPath + "/" + file);
            }
            
            // 复制备份媒体文件
            QDir backupMediaDir(backupMediaPath);
            QStringList backupMediaFiles = backupMediaDir.entryList(QDir::Files);
            for (const QString &file : backupMediaFiles) {
                if (!QFile::copy(backupMediaPath + "/" + file, currentMediaPath + "/" + file)) {
                    qWarning() << "Failed to restore media file:" << file;
                }
            }
        }
        
        return true;
    } catch (const std::exception &e) {
        qCritical() << "Exception during restore:" << e.what();
        return false;
    }
}

// 导出笔记到指定路径和格式
bool SettingsDialog::exportNotes(const QString &exportPath, const QString &format)
{
    try {
        // 获取当前笔记库位置
        QString notebookLocation = m_settings.value("DataStorage/NotebookLocation", 
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/IntelliMedia_Notes").toString();
        
        // 确保导出目录存在
        QDir exportDir(exportPath);
        if (!exportDir.exists()) {
            if (!exportDir.mkpath(".")) {
                return false;
            }
        }
        
        // 创建导出子目录
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
        QString exportSubDir = exportPath + "/notes_export_" + timestamp;
        QDir().mkpath(exportSubDir);
        
        // 打开数据库连接
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "export_connection");
        db.setDatabaseName(notebookLocation + "/notes.db");
        
        if (!db.open()) {
            qCritical() << "Failed to open database for export:" << db.lastError().text();
            return false;
        }
        
        // 查询所有笔记
        QSqlQuery query(db);
        if (!query.exec("SELECT id, title, content, created_at, updated_at FROM notes")) {
            qCritical() << "Failed to query notes for export:" << query.lastError().text();
            db.close();
            return false;
        }
        
        // 导出每个笔记
        int noteCount = 0;
        while (query.next()) {
            int id = query.value(0).toInt();
            QString title = query.value(1).toString();
            QString content = query.value(2).toString();
            QDateTime createdAt = query.value(3).toDateTime();
            QDateTime updatedAt = query.value(4).toDateTime();
            
            // 创建安全的文件名
            QString safeTitle = title;
            safeTitle.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
            if (safeTitle.isEmpty()) {
                safeTitle = "note_" + QString::number(id);
            }
            
            // 根据格式导出
            QString fileExtension = format == "markdown" ? "md" : "html";
            QString filePath = exportSubDir + "/" + safeTitle + "." + fileExtension;
            
            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                
                if (format == "markdown") {
                    // Markdown格式导出
                    out << "# " << title << "\n\n";
                    out << content << "\n\n";
                    out << "---\n";
                    out << "创建时间: " << createdAt.toString("yyyy-MM-dd hh:mm:ss") << "\n";
                    out << "更新时间: " << updatedAt.toString("yyyy-MM-dd hh:mm:ss") << "\n";
                } else {
                    // HTML格式导出
                    out << "<!DOCTYPE html>\n";
                    out << "<html>\n<head>\n";
                    out << "<meta charset=\"utf-8\">\n";
                    out << "<title>" << title << "</title>\n";
                    out << "<style>\n";
                    out << "body { font-family: Arial, sans-serif; margin: 40px; line-height: 1.6; }\n";
                    out << "h1 { color: #333; }\n";
                    out << ".meta { color: #777; font-size: 0.9em; border-top: 1px solid #eee; padding-top: 10px; margin-top: 30px; }\n";
                    out << "</style>\n";
                    out << "</head>\n<body>\n";
                    out << "<h1>" << title << "</h1>\n";
                    out << "<div class=\"content\">\n" << content << "\n</div>\n";
                    out << "<div class=\"meta\">\n";
                    out << "创建时间: " << createdAt.toString("yyyy-MM-dd hh:mm:ss") << "<br>\n";
                    out << "更新时间: " << updatedAt.toString("yyyy-MM-dd hh:mm:ss") << "\n";
                    out << "</div>\n";
                    out << "</body>\n</html>";
                }
                
                file.close();
                noteCount++;
            }
        }
        
        // 关闭数据库连接
        db.close();
        QSqlDatabase::removeDatabase("export_connection");
        
        // 创建导出信息文件
        QFile infoFile(exportSubDir + "/export_info.txt");
        if (infoFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&infoFile);
            out << "IntelliMedia Notes Export\n";
            out << "Date: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n";
            out << "Format: " << format << "\n";
            out << "Notes exported: " << noteCount << "\n";
            infoFile.close();
        }
        
        return noteCount > 0;
    } catch (const std::exception &e) {
        qCritical() << "Exception during export:" << e.what();
        return false;
    }
}

// 从指定路径导入笔记
bool SettingsDialog::importNotes(const QString &importPath)
{
    try {
        // 获取当前笔记库位置
        QString notebookLocation = m_settings.value("DataStorage/NotebookLocation", 
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/IntelliMedia_Notes").toString();
        
        // 打开数据库连接
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "import_connection");
        db.setDatabaseName(notebookLocation + "/notes.db");
        
        if (!db.open()) {
            qCritical() << "Failed to open database for import:" << db.lastError().text();
            return false;
        }
        
        // 扫描导入目录中的文件
        QDir importDir(importPath);
        QStringList filters;
        filters << "*.md" << "*.html" << "*.txt";
        QStringList files = importDir.entryList(filters, QDir::Files);
        
        if (files.isEmpty()) {
            qWarning() << "No importable files found in" << importPath;
            db.close();
            return false;
        }
        
        // 准备插入查询
        QSqlQuery query(db);
        if (!query.prepare("INSERT INTO notes (title, content, created_at, updated_at) VALUES (?, ?, ?, ?)")) {
            qCritical() << "Failed to prepare import query:" << query.lastError().text();
            db.close();
            return false;
        }
        
        // 开始事务
        db.transaction();
        
        int importCount = 0;
        QDateTime currentTime = QDateTime::currentDateTime();
        
        // 处理每个文件
        for (const QString &fileName : files) {
            QFile file(importPath + "/" + fileName);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                qWarning() << "Failed to open file for import:" << fileName;
                continue;
            }
            
            QTextStream in(&file);
            QString content = in.readAll();
            file.close();
            
            // 从文件名提取标题
            QString title = fileName;
            int lastDot = title.lastIndexOf('.');
            if (lastDot > 0) {
                title = title.left(lastDot);
            }
            
            // 处理Markdown文件
            if (fileName.endsWith(".md")) {
                // 如果内容以#开头，尝试提取标题
                QStringList lines = content.split("\n");
                if (!lines.isEmpty() && lines[0].startsWith("# ")) {
                    title = lines[0].mid(2).trimmed();
                    // 移除标题行
                    lines.removeAt(0);
                    content = lines.join("\n").trimmed();
                }
            }
            // 处理HTML文件
            else if (fileName.endsWith(".html")) {
                // 简单提取标题和内容（实际应用中应使用HTML解析库）
                QRegularExpression titleRegex("<title>(.*?)</title>", QRegularExpression::CaseInsensitiveOption);
                QRegularExpressionMatch titleMatch = titleRegex.match(content);
                if (titleMatch.hasMatch()) {
                    title = titleMatch.captured(1);
                }
                
                QRegularExpression bodyRegex("<body>(.*?)</body>", QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
                QRegularExpressionMatch bodyMatch = bodyRegex.match(content);
                if (bodyMatch.hasMatch()) {
                    content = bodyMatch.captured(1);
                    // 简单清理HTML标签（实际应用中应使用HTML解析库）
                    content.replace(QRegularExpression("<h1>.*?</h1>"), "");
                    content.replace(QRegularExpression("<div class=\"meta\">.*?</div>", QRegularExpression::DotMatchesEverythingOption), "");
                }
            }
            
            // 绑定参数并执行查询
            query.bindValue(0, title);
            query.bindValue(1, content);
            query.bindValue(2, currentTime);
            query.bindValue(3, currentTime);
            
            if (!query.exec()) {
                qWarning() << "Failed to import note from file:" << fileName << "Error:" << query.lastError().text();
                continue;
            }
            
            importCount++;
        }
        
        // 提交事务
        if (importCount > 0) {
            if (!db.commit()) {
                qCritical() << "Failed to commit import transaction:" << db.lastError().text();
                db.rollback();
                db.close();
                return false;
            }
        } else {
            db.rollback();
        }
        
        // 关闭数据库连接
        db.close();
        QSqlDatabase::removeDatabase("import_connection");
        
        return importCount > 0;
    } catch (const std::exception &e) {
        qCritical() << "Exception during import:" << e.what();
        return false;
    }
} 