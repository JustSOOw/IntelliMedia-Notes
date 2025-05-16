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
#include <QSqlRecord>      // 添加QSqlRecord头文件
#include <QMainWindow>
#include <QTabBar>
#include <QTabWidget>
#include <QStyle>
#include <QPaintEvent>
#include <QStyleOptionTab>
#include <QPainter>
#include <QPalette>
#include <QFontMetrics>
#include <QFontComboBox>
#include <QDialogButtonBox>
#include <QAbstractItemView> // 添加QAbstractItemView头文件
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>

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
    
    // 应用当前的全局字体设置
    QString fontFamily = m_settings.value("Editor/FontFamily", "Arial").toString();
    if (!fontFamily.isEmpty()) {
        QFont dialogFont(fontFamily);
        this->setFont(dialogFont);
    }
    
    // 初始化UI
    setupUI();
    
    // 加载设置
    loadSettings();
    
    // 加载完成后再次遍历所有子控件，应用字体设置
    if (!fontFamily.isEmpty()) {
        QList<QWidget*> allWidgets = this->findChildren<QWidget*>();
        for (QWidget* widget : allWidgets) {
            if (widget) {
                QFont widgetFont = widget->font();
                widgetFont.setFamily(fontFamily);
                widget->setFont(widgetFont);
            }
        }
    }
    
    // 连接信号和槽 - 常规设置
    connect(m_themeGroup, &QButtonGroup::buttonClicked, this, &SettingsDialog::onThemeChanged);
    connect(m_languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::onLanguageChanged);
    connect(m_autoSaveCheck, &QCheckBox::stateChanged, this, &SettingsDialog::onAutoSaveChanged);
    connect(m_autoSaveIntervalCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::onAutoSaveIntervalChanged);
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
    
    // 应用编辑器字体设置 - 这也将成为应用程序的全局Widget字体
    QString fontFamily = settings.value("Editor/FontFamily", "Arial").toString();
    // 直接使用固定字号10pt
    QFont appWidgetFont(fontFamily);
    appWidgetFont.setPointSize(10);
    
    // 为所有Qt Widgets组件应用字体
    QApplication::setFont(appWidgetFont);
    qDebug() << "应用全局Widget字体设置:" << fontFamily << ", 10pt (固定字号)";
    
    // TextEditorManager会通过editorFontChanged信号独立处理编辑器本身的字体设置

    // 应用制表符宽度设置
    int tabWidth = settings.value("Editor/TabWidth", 4).toInt();
    qDebug() << "应用制表符宽度设置:" << tabWidth << "个空格";
    
    // 应用自动配对括号设置
    bool autoPair = settings.value("Editor/AutoPairEnabled", true).toBool();
    qDebug() << "应用自动配对括号设置:" << (autoPair ? "启用" : "禁用");
    
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
                QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).toString();
            
            // 获取默认备份位置
            QString backupLocation = settings.value("DataStorage/BackupLocation", 
                QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/IntelliMedia_Notes_Backups").toString();
            
            // 输出日志，方便调试
            qDebug() << "准备自动备份，笔记库位置:" << notebookLocation;
            qDebug() << "备份位置:" << backupLocation;
            
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
    // 首先检查设置中是否有用户自定义的笔记库位置
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName());
    QString customLocation = settings.value("DataStorage/NotebookLocation", "").toString();
    
    // 如果有用户自定义位置且目录存在，则使用它
    if (!customLocation.isEmpty() && QDir(customLocation).exists()) {
        qDebug() << "使用用户自定义笔记库路径:" << customLocation;
        return customLocation;
    }
    
    // 如果没有自定义位置或目录不存在，则使用文档位置作为默认路径
    // 这样与设置对话框显示的保持一致
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + QApplication::applicationName();
    QDir dir(defaultPath);
    
    // 确保目录存在
    if (!dir.exists()) {
        qDebug() << "创建默认笔记库目录:" << defaultPath;
        if (!dir.mkpath(".")) {
            qWarning() << "无法创建默认笔记库目录:" << defaultPath;
            
            // 尝试使用备用位置 - 应用数据位置
            defaultPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            dir = QDir(defaultPath);
            if (!dir.exists() && !dir.mkpath(".")) {
                qCritical() << "无法创建备用笔记库目录:" << defaultPath;
            } else {
                qDebug() << "使用备用笔记库目录:" << defaultPath;
            }
        }
    }
    
    // 检查目录是否可写
    QFile testFile(defaultPath + "/test_write.tmp");
    bool isWritable = testFile.open(QIODevice::WriteOnly);
    if (isWritable) {
        testFile.close();
        testFile.remove();
    } else {
        qWarning() << "笔记库目录不可写:" << defaultPath;
        // 如果不可写，尝试使用备用位置
        defaultPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        dir = QDir(defaultPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
        qDebug() << "切换到备用笔记库目录:" << defaultPath;
    }
    
    // 更新设置中的路径
    settings.setValue("DataStorage/NotebookLocation", defaultPath);
    settings.sync();
    
    qDebug() << "使用默认笔记库路径:" << defaultPath;
    return defaultPath;
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

// 自动保存间隔改变时的槽函数
void SettingsDialog::onAutoSaveIntervalChanged(int index)
{
    // 根据选择的间隔值设置自动保存间隔
    int autoSaveInterval = m_autoSaveIntervalCombo->itemData(index).toInt();
    m_settings.setValue("General/AutoSaveInterval", autoSaveInterval);
    m_settings.sync();
    
    // 判断是否自动保存已启用
    bool autoSaveEnabled = m_autoSaveCheck->isChecked();
    if (autoSaveEnabled) {
        // 立即应用新的设置
        // 这里我们使用静态方法中的代码作为发出信号的替代
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName());
        settings.setValue("General/AutoSaveInterval", autoSaveInterval);
        settings.sync();
        
        qDebug() << "自动保存间隔已更改为" << autoSaveInterval << "分钟";
        
        // 发送信号通知外部组件自动保存间隔已更改
        emit autoSaveIntervalChanged(autoSaveInterval);
    }
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
    // 创建自定义字体选择对话框
    QDialog fontDialog(this);
    fontDialog.setWindowTitle(tr("选择字体"));
    fontDialog.setModal(true);
    fontDialog.setMinimumWidth(350);
    
    // 应用与主应用程序相同的样式表，确保对话框遵循当前主题
    fontDialog.setStyleSheet(this->styleSheet());
    
    // 创建布局
    QVBoxLayout *layout = new QVBoxLayout(&fontDialog);
    layout->setSpacing(10);
    layout->setContentsMargins(15, 15, 15, 15);
    
    // 添加说明标签
    QLabel *label = new QLabel(tr("请选择字体家族:"), &fontDialog);
    layout->addWidget(label);
    
    // 添加字体下拉框
    QFontComboBox *fontComboBox = new QFontComboBox(&fontDialog);
    fontComboBox->setObjectName("settingsFontComboBox"); // 设置对象名，便于样式表定位
    fontComboBox->setCurrentFont(m_selectedFont);
    fontComboBox->setEditable(true);
    fontComboBox->lineEdit()->setReadOnly(true);
    
    // 调整字体下拉框的大小
    fontComboBox->setFixedWidth(250);
    // 设置下拉菜单宽度，以保持与按钮宽度不同
    fontComboBox->view()->setMinimumWidth(300);
    fontComboBox->view()->setFixedWidth(300);
    
    layout->addWidget(fontComboBox);
    
    // 添加预览框
    QGroupBox *previewGroup = new QGroupBox(tr("预览"), &fontDialog);
    QVBoxLayout *previewLayout = new QVBoxLayout(previewGroup);
    QLabel *previewLabel = new QLabel("AaBbCcYyZz 1234567890 汉字示例", &fontDialog);
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setMinimumHeight(80);
    previewLayout->addWidget(previewLabel);
    layout->addWidget(previewGroup);
    
    // 连接字体选择变化到预览
    connect(fontComboBox, &QFontComboBox::currentFontChanged, [previewLabel](const QFont &font) {
        QFont previewFont = font;
        previewFont.setPointSize(12); // 预览使用固定字号
        previewLabel->setFont(previewFont);
    });
    
    // 手动触发一次当前字体的预览
    QFont initialPreviewFont = fontComboBox->currentFont();
    initialPreviewFont.setPointSize(12);
    previewLabel->setFont(initialPreviewFont);
    
    // 按钮盒子
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, 
        Qt::Horizontal, 
        &fontDialog);
    layout->addWidget(buttonBox);
    
    // 连接按钮信号
    connect(buttonBox, &QDialogButtonBox::accepted, &fontDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &fontDialog, &QDialog::reject);
    
    // 执行对话框
    if (fontDialog.exec() == QDialog::Accepted) {
        // 获取选定的字体
        QFont selectedFont = fontComboBox->currentFont();
        
        // 只保存字体家族，不保存字号
        m_selectedFont = selectedFont;
        m_selectedFont.setPointSize(10); // 重置为默认值，实际应用时会使用系统默认字号
        m_currentFontLabel->setText(selectedFont.family());
        
        // 保存设置到INI文件，只保存字体家族
        m_settings.setValue("Editor/FontFamily", selectedFont.family());
        
        // 发射信号通知应用字体变更，使用默认字号
        emit editorFontChanged(selectedFont.family(), 0); // 0表示使用系统默认字号
        
        qDebug() << "发出编辑器字体变更信号：" << selectedFont.family() << "(使用系统默认字号)";
        
        // 立即将新字体应用到设置对话框自身
        QFont dialogFont(selectedFont.family());
        this->setFont(dialogFont);
        
        // 遍历设置对话框中的所有子控件，应用新字体
        QList<QWidget*> allWidgets = this->findChildren<QWidget*>();
        for (QWidget* widget : allWidgets) {
            if (widget) {
                QFont widgetFont = widget->font();
                widgetFont.setFamily(selectedFont.family());
                widget->setFont(widgetFont);
            }
        }
        
        // 强制更新
        this->update();
    }
}

// 制表符宽度改变时的槽函数
void SettingsDialog::onTabWidthChanged(int value)
{
    // 保存新的制表符宽度设置
    m_settings.setValue("Editor/TabWidth", value);
    
    // 在日志中记录更改
    qDebug() << "制表符宽度已更改为: " << value;
    
    // 向应用程序其他部分发出信号，通知制表符宽度已更改
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, 
                     QApplication::organizationName(), QApplication::applicationName());
    settings.setValue("Editor/TabWidth", value);
    
    // 发射信号通知应用制表符宽度变更
    emit tabWidthChanged(value);
    qDebug() << "发出制表符宽度变更信号：" << value << "个空格";
}

// 自动配对括号/引号设置改变时的槽函数
void SettingsDialog::onAutoPairChanged(int state)
{
    bool enabled = (state == Qt::Checked);
    
    // 保存自动配对设置
    m_settings.setValue("Editor/AutoPairEnabled", enabled);
    
    // 在日志中记录更改
    qDebug() << "自动配对括号/引号已" << (enabled ? "启用" : "禁用");
    
    // 将设置保存到全局设置
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, 
                     QApplication::organizationName(), QApplication::applicationName());
    settings.setValue("Editor/AutoPairEnabled", enabled);
    
    // 发射信号通知应用自动配对括号设置变更
    emit autoPairChanged(enabled);
    qDebug() << "发出自动配对括号设置变更信号：" << (enabled ? "启用" : "禁用");
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
    
    // 检查新路径是否可写
    QFile testFile(newPath + "/test_write.tmp");
    bool isWritable = testFile.open(QIODevice::WriteOnly);
    if (!isWritable) {
        QMessageBox::critical(this,
            tr("权限错误"),
            tr("选择的目录不可写。请选择一个您具有写入权限的目录。"));
        return;
    }
    testFile.close();
    testFile.remove();
    
    // 检查是否选择了与当前相同的路径
    if (QDir(currentPath) == QDir(newPath)) {
        QMessageBox::information(this,
            tr("相同路径"),
            tr("您选择的路径与当前笔记库路径相同。"));
        return;
    }
    
    // 确认是否要更改笔记库位置
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        tr("确认更改"),
        tr("更改笔记库位置将会移动所有笔记数据到新位置。\n\n应用程序需要重启才能安全地移动数据库文件。是否继续？"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::No) {
        return;
    }
    
    // 保存新旧路径到设置，以便在重启后执行移动操作
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName());
    settings.setValue("DataStorage/NewNotebookLocation", newPath);
    settings.setValue("DataStorage/OldNotebookLocation", currentPath);
    settings.setValue("DataStorage/PendingNotebookMove", true);
    settings.sync();
    
    // 提示用户重启应用程序
    QMessageBox restartMsgBox(this);
    restartMsgBox.setWindowTitle(tr("需要重启"));
    restartMsgBox.setIcon(QMessageBox::Information);
    restartMsgBox.setText(tr("更改笔记库位置需要重启应用程序。"));
    restartMsgBox.setInformativeText(tr("应用程序将在重启后移动您的数据。要立即重启应用程序吗？"));
    restartMsgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    restartMsgBox.setDefaultButton(QMessageBox::Yes);
        
    if (restartMsgBox.exec() == QMessageBox::Yes) {
        // 使用与MainWindow::restartApplication()相同的方法重启应用程序
        qDebug() << "正在重启应用程序...";
        
        // 保存所有设置
        saveSettings();
            
        // 标记待处理的重启
        settings.setValue("Application/PendingRestart", true);
        settings.sync();
        
        // 获取应用程序路径
        QString appPath = QApplication::applicationFilePath();
        QStringList arguments = QApplication::arguments();
        arguments.removeFirst(); // 移除程序名称
        
        // 添加重启标记参数
        arguments << "--restarted";
        
        qDebug() << "启动新进程:" << appPath << arguments;
            
        // 启动新进程
        bool success = QProcess::startDetached(appPath, arguments);
        
        if (success) {
            qDebug() << "新进程启动成功，即将关闭当前实例";
            // 关闭当前实例
            QApplication::quit();
        } else {
            qCritical() << "启动新进程失败";
            QMessageBox::critical(this, tr("重启失败"), 
                tr("应用程序重启失败。请手动关闭并重新启动应用程序。"));
        }
    } else {
        // 更新UI提示用户稍后需要重启
        m_notebookLocationLabel->setText(newPath);
        m_operationStatusLabel->setText(tr("重启后将移动笔记库到新位置"));
            QMessageBox::information(this,
            tr("操作待处理"),
            tr("笔记库位置更改将在下次启动应用程序时完成。"));
        }
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
    
    // 检查目录是否可写
    QFile testFile(backupPath + "/test_write.tmp");
    bool isWritable = testFile.open(QIODevice::WriteOnly);
    if (!isWritable) {
        QMessageBox::critical(this,
            tr("权限错误"),
            tr("选择的目录不可写。请选择一个您具有写入权限的目录。"));
        return;
    }
    testFile.close();
    testFile.remove();
    
    // 显示进度条
    m_operationProgressBar->setVisible(true);
    m_operationProgressBar->setValue(0);
    m_operationStatusLabel->setText(tr("正在准备备份..."));
    
    // 禁用按钮，防止重复操作
    m_backupNowBtn->setEnabled(false);
    m_restoreFromBackupBtn->setEnabled(false);
    m_exportNotesBtn->setEnabled(false);
    m_importNotesBtn->setEnabled(false);
    
    // 在单独的线程中执行备份操作
    QFuture<bool> future = QtConcurrent::run([=]() {
        return backupData(backupPath);
    });
    
    // 等待备份完成
    QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [=]() {
        bool success = future.result();
            
            // 更新UI
            m_operationProgressBar->setVisible(false);
            m_backupNowBtn->setEnabled(true);
            m_restoreFromBackupBtn->setEnabled(true);
        m_exportNotesBtn->setEnabled(true);
        m_importNotesBtn->setEnabled(true);
            
            if (success) {
                m_operationStatusLabel->setText(tr("备份已完成"));
            
            // 获取备份位置和时间
            QSettings settings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName());
            QString lastBackupLocation = settings.value("DataStorage/LastBackupLocation", "").toString();
            QDateTime lastBackupTime = settings.value("DataStorage/LastBackupTime", QDateTime()).toDateTime();
            
            QString timeStr = lastBackupTime.toString("yyyy-MM-dd hh:mm:ss");
            
                QMessageBox::information(this,
                    tr("备份完成"),
                tr("笔记库已成功备份。\n\n备份位置: %1\n备份时间: %2").arg(lastBackupLocation).arg(timeStr));
            } else {
                m_operationStatusLabel->setText(tr("备份失败"));
                QMessageBox::critical(this,
                    tr("备份失败"),
                tr("无法备份笔记库。请检查目标位置是否可写，以及是否有足够的磁盘空间。"));
            }
        
        watcher->deleteLater();
    });
    
    // 更真实的进度显示
    QTimer *progressTimer = new QTimer(this);
    int progress = 0;
    
    connect(progressTimer, &QTimer::timeout, this, [=]() mutable {
        if (progress < 90) {
            // 在开始阶段快速增加到50%，然后减缓速度
            if (progress < 50) {
                progress += 5;
            } else {
                progress += 2;
            }
            m_operationProgressBar->setValue(progress);
            
            // 更新状态文本
            if (progress < 20) {
                m_operationStatusLabel->setText(tr("正在准备备份..."));
            } else if (progress < 50) {
                m_operationStatusLabel->setText(tr("正在备份数据库..."));
            } else {
                m_operationStatusLabel->setText(tr("正在备份媒体文件..."));
            }
        }
        
        if (watcher->isFinished()) {
            // 备份完成，设置进度为100%
            m_operationProgressBar->setValue(100);
            // 更新状态文本
            bool success = future.result();
            m_operationStatusLabel->setText(success ? tr("备份已完成") : tr("备份失败"));
            // 停止定时器
            progressTimer->stop();
            progressTimer->deleteLater();
        }
    });
    
    watcher->setFuture(future);
    progressTimer->start(100);
}

// 从备份恢复按钮点击时的槽函数
void SettingsDialog::onRestoreFromBackupClicked()
{
    // 先查找最新的备份位置作为默认路径
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName());
    QString lastBackupLocation = settings.value("DataStorage/LastBackupLocation", "").toString();
    
    // 如果有上次备份位置，则使用其父目录作为默认选择目录
    if (!lastBackupLocation.isEmpty()) {
        QFileInfo info(lastBackupLocation);
        if (info.exists() && info.isDir()) {
            defaultDir = info.absolutePath();
        }
    }
    
    // 选择备份文件夹
    QString backupPath = QFileDialog::getExistingDirectory(this,
        tr("选择备份文件夹"),
        defaultDir,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (backupPath.isEmpty()) {
        return;
    }
    
    // 检查备份文件是否存在
    QDir backupDir(backupPath);
    if (!backupDir.exists("notes.db")) {
        // 如果直接选择了父目录，尝试查找子目录中的备份
        QStringList backupSubDirs = backupDir.entryList(QStringList() << "backup_*", QDir::Dirs, QDir::Time);
        if (!backupSubDirs.isEmpty()) {
            // 找到最新的备份子目录
            QString latestBackup = backupPath + "/" + backupSubDirs.first();
            if (QFile::exists(latestBackup + "/notes.db")) {
                backupPath = latestBackup;
                QMessageBox::information(this,
                    tr("已找到备份"),
                    tr("已自动选择最新的备份:\n%1").arg(backupPath));
            } else {
                // 尝试继续查找可能的备份目录
                bool foundBackup = false;
                for (int i = 1; i < backupSubDirs.size(); i++) {
                    QString checkBackup = backupPath + "/" + backupSubDirs[i];
                    if (QFile::exists(checkBackup + "/notes.db")) {
                        backupPath = checkBackup;
                        foundBackup = true;
                        QMessageBox::information(this,
                            tr("已找到备份"),
                            tr("已自动选择可用的备份:\n%1").arg(backupPath));
                        break;
                    }
                }
                
                // 如果仍未找到有效备份
                if (!foundBackup) {
        QMessageBox::critical(this,
            tr("无效的备份"),
                        tr("所选文件夹及其子目录不包含有效的备份数据。请选择包含notes.db文件的备份目录。"));
        return;
    }
            }
        } else {
            // 尝试查找自动备份目录
            QStringList autoBackupDirs = backupDir.entryList(QStringList() << "auto_backup_*", QDir::Dirs, QDir::Time);
            if (!autoBackupDirs.isEmpty()) {
                QString latestAutoBackup = backupPath + "/" + autoBackupDirs.first();
                if (QFile::exists(latestAutoBackup + "/notes.db")) {
                    backupPath = latestAutoBackup;
                    QMessageBox::information(this,
                        tr("已找到自动备份"),
                        tr("已自动选择最新的自动备份:\n%1").arg(backupPath));
                } else {
                    QMessageBox::critical(this,
                        tr("无效的备份"),
                        tr("所选文件夹不包含有效的备份数据。请选择包含notes.db文件的备份目录。"));
        return;
    }
            } else {
                QMessageBox::critical(this,
                    tr("无效的备份"),
                    tr("所选文件夹不包含有效的备份数据。请选择包含notes.db文件的备份目录。"));
                return;
            }
        }
    }
    
    // 获取备份信息
    QString backupInfo = tr("备份路径: %1\n").arg(backupPath);
    QFile infoFile(backupPath + "/backup_info.txt");
    if (infoFile.exists() && infoFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&infoFile);
        QString infoContent = in.readAll();
        infoFile.close();
        backupInfo += tr("\n备份信息:\n%1").arg(infoContent);
    } else {
        // 如果没有备份信息文件，则尝试推断备份日期
        QFileInfo dbInfo(backupPath + "/notes.db");
        if (dbInfo.exists()) {
            backupInfo += tr("\n备份日期: %1").arg(dbInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss"));
        }
    }
    
    // 确认是否要恢复备份
    QMessageBox confirmDialog(this);
    confirmDialog.setWindowTitle(tr("确认恢复"));
    confirmDialog.setIcon(QMessageBox::Warning);
    confirmDialog.setText(tr("恢复备份将会覆盖当前的所有笔记数据。此操作无法撤销。"));
    confirmDialog.setInformativeText(tr("确定要从以下备份恢复吗？\n\n%1").arg(backupInfo));
    confirmDialog.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmDialog.setDefaultButton(QMessageBox::No);
    
    // 添加"查看备份内容"按钮
    QPushButton *viewButton = confirmDialog.addButton(tr("查看备份内容"), QMessageBox::ActionRole);
    
    int result = confirmDialog.exec();
    
    // 如果用户点击了"查看备份内容"按钮
    if (confirmDialog.clickedButton() == viewButton) {
        // 简单显示备份中的笔记数量和标题
        if (QFile::exists(backupPath + "/notes.db")) {
            QSqlDatabase backupDb = QSqlDatabase::addDatabase("QSQLITE", "backup_preview_connection");
            backupDb.setDatabaseName(backupPath + "/notes.db");
            
            QString previewContent;
            if (backupDb.open()) {
                QSqlQuery query(backupDb);
                if (query.exec("SELECT COUNT(*) FROM notes")) {
                    if (query.next()) {
                        int noteCount = query.value(0).toInt();
                        previewContent += tr("笔记总数: %1\n\n").arg(noteCount);
                    }
                }
                
                if (query.exec("SELECT title, updated_at FROM notes ORDER BY updated_at DESC LIMIT 20")) {
                    previewContent += tr("最近的笔记:\n");
                    int count = 0;
                    while (query.next() && count < 20) {
                        QString title = query.value(0).toString();
                        QDateTime updateTime = query.value(1).toDateTime();
                        previewContent += tr("%1. %2 (最后更新: %3)\n")
                            .arg(count + 1)
                            .arg(title)
                            .arg(updateTime.toString("yyyy-MM-dd hh:mm:ss"));
                        count++;
                    }
                }
                
                backupDb.close();
            } else {
                previewContent = tr("无法打开备份数据库进行预览: %1").arg(backupDb.lastError().text());
            }
            
            QSqlDatabase::removeDatabase("backup_preview_connection");
            
            // 显示预览对话框
                QMessageBox::information(this,
                tr("备份内容预览"),
                previewContent);
                
            // 再次显示确认对话框
            result = QMessageBox::question(this,
        tr("确认恢复"),
                tr("恢复备份将会覆盖当前的所有笔记数据。此操作无法撤销。\n\n确定要从以下备份恢复吗？\n\n%1").arg(backupInfo),
        QMessageBox::Yes | QMessageBox::No);
            } else {
            QMessageBox::warning(this,
                tr("无法预览"),
                tr("无法打开备份数据库进行预览"));
            
            // 再次显示确认对话框
            result = QMessageBox::question(this,
                tr("确认恢复"),
                tr("恢复备份将会覆盖当前的所有笔记数据。此操作无法撤销。\n\n确定要从以下备份恢复吗？\n\n%1").arg(backupInfo),
                QMessageBox::Yes | QMessageBox::No);
        }
    }
    
    if (result != QMessageBox::Yes) {
        return;
    }
    
    // 存储要恢复的备份路径，并设置待恢复标志
    // 使用前面已经创建的settings对象，而不是重新创建
    settings.setValue("DataStorage/PendingRestorePath", backupPath);
    settings.setValue("DataStorage/PendingRestore", true);
    settings.sync();
    
    // 提示用户需要重启应用程序以进行恢复
    QMessageBox restartMsgBox(this);
    restartMsgBox.setWindowTitle(tr("需要重启"));
    restartMsgBox.setIcon(QMessageBox::Information);
    restartMsgBox.setText(tr("恢复备份需要重启应用程序。"));
    restartMsgBox.setInformativeText(tr("应用程序将在重启后从选定的备份恢复您的数据。要立即重启应用程序吗？"));
    restartMsgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    restartMsgBox.setDefaultButton(QMessageBox::Yes);
        
    if (restartMsgBox.exec() == QMessageBox::Yes) {
        // 通知主窗口重启应用程序
        settings.setValue("Application/PendingRestart", true);
        settings.sync();
            
        // 使用与笔记库位置移动相同的重启机制
        qDebug() << "正在重启应用程序...";
        
        // 获取应用程序路径
        QString appPath = QApplication::applicationFilePath();
        QStringList arguments = QApplication::arguments();
        arguments.removeFirst(); // 移除程序名称
        
        // 添加重启标记参数
        arguments << "--restarted";
        
        qDebug() << "启动新进程:" << appPath << arguments;
            
        // 启动新进程
        bool success = QProcess::startDetached(appPath, arguments);
            
            if (success) {
            qDebug() << "新进程启动成功，即将关闭当前实例";
            // 关闭当前实例
            QApplication::quit();
            } else {
            qCritical() << "启动新进程失败";
            QMessageBox::critical(this, tr("重启失败"), 
                tr("应用程序重启失败。请手动关闭并重新启动应用程序。"));
        }
    } else {
        m_operationStatusLabel->setText(tr("重启后将从备份恢复"));
        QMessageBox::information(this,
            tr("操作待处理"),
            tr("下次启动应用程序时将恢复备份数据。"));
            }
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
    // 先检查笔记库是否存在且有效
    QString notebookPath = getNotebookPath();
    QString dbPath = notebookPath + "/notes.db";
    
    // 检查数据库文件是否存在
    if (!QFile::exists(dbPath)) {
        QMessageBox::warning(this, 
            tr("导出失败"), 
            tr("当前笔记库不存在或未创建任何笔记。\n\n路径: %1").arg(dbPath));
        return;
    }
    
    // 检查数据库是否包含笔记
    bool hasNotes = false;
    {
        // 使用临时连接名以避免冲突
        QString tempConnName = "check_notes_" + QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch());
        QSqlDatabase tempDb = QSqlDatabase::addDatabase("QSQLITE", tempConnName);
        tempDb.setDatabaseName(dbPath);
        
        if (tempDb.open()) {
            QSqlQuery checkQuery(tempDb);
            
            // 先检查表结构
            if (checkQuery.exec("PRAGMA table_info(notes)")) {
                if (checkQuery.size() > 0 || checkQuery.next()) { // 表存在
                    // 检查是否有笔记数据
                    if (checkQuery.exec("SELECT COUNT(*) FROM notes")) {
                        if (checkQuery.next() && checkQuery.value(0).toInt() > 0) {
                            hasNotes = true;
                        }
                    }
                }
            }
            tempDb.close();
        }
        QSqlDatabase::removeDatabase(tempConnName);
    }
    
    if (!hasNotes) {
        QMessageBox::warning(this, 
            tr("没有可导出的笔记"), 
            tr("当前笔记库中没有找到任何笔记，无法执行导出操作。"));
        return;
    }
    
    QString exportPath = QFileDialog::getExistingDirectory(this,
        tr("选择导出位置"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (exportPath.isEmpty()) {
        return;
    }
    
    // 检查导出路径是否可写
    QFile testFile(exportPath + "/test_write.tmp");
    bool isWritable = testFile.open(QIODevice::WriteOnly);
    if (!isWritable) {
        QMessageBox::critical(this,
            tr("权限错误"),
            tr("选择的目录不可写。请选择一个您具有写入权限的目录。"));
        return;
    }
    testFile.close();
    testFile.remove();
    
    // 显示进度条
    m_operationProgressBar->setVisible(true);
    m_operationProgressBar->setValue(0);
    m_operationStatusLabel->setText(tr("正在导出..."));
    
    // 禁用按钮，防止重复操作
    m_exportNotesBtn->setEnabled(false);
    m_importNotesBtn->setEnabled(false);
    
    // 参考导入功能的做法，先显示模拟进度条，然后在后台执行实际导出操作
    // 创建一个定时器来模拟进度更新
    QTimer *progressTimer = new QTimer(this);
    int progress = 0;
    QString actualExportPath;
    bool exportSuccess = false;
    
    // 连接定时器到进度更新
    connect(progressTimer, &QTimer::timeout, this, [=]() mutable {
        // 更新进度，确保不超过100%
        if (progress < 95) {
            progress += 3; // 稍微慢一点，让用户能看到进度条变化
            m_operationProgressBar->setValue(progress);
            
            // 更新状态文本
            if (progress < 30) {
                m_operationStatusLabel->setText(tr("正在准备导出..."));
            } else if (progress < 70) {
                m_operationStatusLabel->setText(tr("正在导出笔记..."));
            } else {
                m_operationStatusLabel->setText(tr("正在完成导出..."));
            }
        }
    });
    
    // 启动定时器
    progressTimer->start(100); // 每100毫秒更新一次
    
    // 使用QTimer延迟执行导出操作，确保UI有时间显示进度条
    QTimer::singleShot(500, this, [=]() mutable {
        // 在后台线程执行导出操作
        QFuture<QPair<bool, QString>> future = QtConcurrent::run([=]() -> QPair<bool, QString> {
            // 实际执行导出
            bool success = exportNotes(exportPath, selectedFormat);
            // 返回结果和导出路径
            QString resultPath = success ? exportPath + "/notes_export_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") : "";
            return QPair<bool, QString>(success, resultPath);
        });
        
        // 创建监视器等待完成
        QFutureWatcher<QPair<bool, QString>> *watcher = new QFutureWatcher<QPair<bool, QString>>(this);
        
        // 连接完成信号
        connect(watcher, &QFutureWatcher<QPair<bool, QString>>::finished, this, [=]() mutable {
            // 获取执行结果
            QPair<bool, QString> result = future.result();
            bool success = result.first;
            QString actualExportPath = result.second;
            
            // 设置最终进度为100%
            m_operationProgressBar->setValue(100);
            
            // 停止定时器
            progressTimer->stop();
            progressTimer->deleteLater();
            
            // 延迟一会儿再隐藏进度条和显示结果，让用户能看到100%的进度
            QTimer::singleShot(500, this, [=]() {
                // 更新UI
                m_operationProgressBar->setVisible(false);
                m_exportNotesBtn->setEnabled(true);
                m_importNotesBtn->setEnabled(true);
                
                if (success) {
                    m_operationStatusLabel->setText(tr("导出已完成"));
                    
                    // 显示完成对话框，提供打开导出文件夹的选项
                    QMessageBox msgBox(this);
                    msgBox.setWindowTitle(tr("导出完成"));
                    msgBox.setIcon(QMessageBox::Information);
                    msgBox.setText(tr("笔记已成功导出到:\n%1").arg(actualExportPath));
                    msgBox.setStandardButtons(QMessageBox::Ok);
                    
                    // 添加"打开文件夹"按钮
                    QPushButton *openFolderButton = msgBox.addButton(tr("打开文件夹"), QMessageBox::ActionRole);
                    
                    msgBox.exec();
                    
                    // 如果用户点击了"打开文件夹"按钮
                    if (msgBox.clickedButton() == openFolderButton) {
                        QDesktopServices::openUrl(QUrl::fromLocalFile(actualExportPath));
                    }
                } else {
                    m_operationStatusLabel->setText(tr("导出失败"));
                    QMessageBox::critical(this,
                        tr("导出失败"),
                        tr("无法导出笔记。请检查笔记库和目标位置是否正确，以及是否有足够的权限。"));
                }
            });
            
            // 清理监视器
            watcher->deleteLater();
        });
        
        // 启动监视器
        watcher->setFuture(future);
    });
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
    
    m_startMinimizedCheck = new QCheckBox(tr("启动时最小化到系统托盘"));
    m_checkUpdatesCheck = new QCheckBox(tr("自动检查更新"));
    m_autoStartCheck = new QCheckBox(tr("开机自动启动"));
    
    otherLayout->addWidget(m_startMinimizedCheck);
    otherLayout->addWidget(m_checkUpdatesCheck);
    otherLayout->addWidget(m_autoStartCheck);
    
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
    connect(m_autoSaveIntervalCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::onAutoSaveIntervalChanged);
    connect(m_autoStartCheck, &QCheckBox::stateChanged, this, &SettingsDialog::onAutoStartChanged);
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
    m_currentFontLabel = new QLabel(tr("Arial"));
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
    
    // 注释掉这些连接，因为已经在构造函数中连接过了
    // connect(m_selectNotebookLocationBtn, &QPushButton::clicked, this, &SettingsDialog::onSelectNotebookLocationClicked);
    // connect(m_backupNowBtn, &QPushButton::clicked, this, &SettingsDialog::onBackupNowClicked);
    // connect(m_restoreFromBackupBtn, &QPushButton::clicked, this, &SettingsDialog::onRestoreFromBackupClicked);
    // connect(m_exportNotesBtn, &QPushButton::clicked, this, &SettingsDialog::onExportNotesClicked);
    // connect(m_importNotesBtn, &QPushButton::clicked, this, &SettingsDialog::onImportNotesClicked);
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
    
    // 加载开机自动启动设置
    bool autoStart = checkAutoStart();
    m_autoStartCheck->setChecked(autoStart);
    
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
    
    // 加载编辑器设置 - 只加载字体家族，不再加载字号
    QString fontFamily = m_settings.value("Editor/FontFamily", "Arial").toString();
    m_selectedFont = QFont(fontFamily);
    m_currentFontLabel->setText(fontFamily);
    
    // 应用字体设置到对话框
    QFont dialogFont(fontFamily);
    this->setFont(dialogFont);
    
    // 应用字体到所有子控件
    QList<QWidget*> allWidgets = this->findChildren<QWidget*>();
    for (QWidget* widget : allWidgets) {
        if (widget) {
            QFont widgetFont = widget->font();
            widgetFont.setFamily(fontFamily);
            widget->setFont(widgetFont);
        }
    }
    
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
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).toString();
    m_notebookLocationLabel->setText(notebookLocation);
    
    // 输出日志，显示当前笔记路径
    qDebug() << "当前笔记库路径:" << notebookLocation;
    
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
    
    // 保存编辑器设置 - 只保存字体家族，不再保存字号
    m_settings.setValue("Editor/FontFamily", m_selectedFont.family());
    m_settings.setValue("Editor/TabWidth", m_tabWidthSpin->value());
    m_settings.setValue("Editor/AutoPairEnabled", m_autoPairCheck->isChecked());
    
    // 保存AI服务设置
    QString apiKey = m_apiKeyEdit->text().trimmed();
    if (!apiKey.isEmpty()) {
        QString obfuscatedKey = obfuscateApiKey(apiKey);
        qDebug() << "保存设置：API密钥长度=" << apiKey.length() << "，混淆后长度=" << obfuscatedKey.length();
        m_settings.setValue("AIService/APIKey", obfuscatedKey);
    } else {
        qWarning() << "API密钥为空，未保存到设置中";
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
        qDebug() << "开始备份数据到:" << backupPath;
        
        // 获取当前笔记库位置
        QString notebookLocation = getNotebookPath();
        qDebug() << "当前笔记库位置:" << notebookLocation;
        
        // 检查源路径是否存在
        QDir notebookDir(notebookLocation);
        if (!notebookDir.exists()) {
            qCritical() << "源笔记库目录不存在:" << notebookLocation;
            return false;
        }
        
        // 检查源数据库文件是否存在
        QString sourceDbPath = notebookLocation + "/notes.db";
        bool hasSourceDb = QFile::exists(sourceDbPath);
        if (!hasSourceDb) {
            qWarning() << "源数据库文件不存在:" << sourceDbPath;
        }
        
        // 检查源媒体目录是否存在
        QString sourceMediaPath = notebookLocation + "/notes_media";
        bool hasSourceMedia = QDir(sourceMediaPath).exists();
        if (!hasSourceMedia) {
            qWarning() << "源媒体目录不存在:" << sourceMediaPath;
        }
        
        // 如果既没有数据库也没有媒体目录，则无需备份
        if (!hasSourceDb && !hasSourceMedia) {
            qWarning() << "笔记库为空，没有数据需要备份";
            return false;
        }
        
        // 确保备份根目录存在
        QDir backupDir(backupPath);
        if (!backupDir.exists()) {
            if (!backupDir.mkpath(".")) {
                qCritical() << "无法创建备份根目录:" << backupPath;
                return false;
            }
            qDebug() << "已创建备份根目录:" << backupPath;
        }
        
        // 创建备份子目录，使用时间戳命名
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
        QString backupSubDir = backupPath + "/backup_" + timestamp;
        if (!QDir().mkpath(backupSubDir)) {
            qCritical() << "无法创建备份子目录:" << backupSubDir;
            return false;
        }
        qDebug() << "已创建备份子目录:" << backupSubDir;
        
        // 关闭可能的数据库连接
        QStringList connections = QSqlDatabase::connectionNames();
        for (const QString &connectionName : connections) {
            QSqlDatabase db = QSqlDatabase::database(connectionName);
            if (db.isValid() && db.isOpen()) {
                db.close();
                qDebug() << "已关闭数据库连接:" << connectionName;
            }
        }
        
        // 备份数据库文件
        bool dbBackupSuccess = false;
        if (hasSourceDb) {
            QString destDbPath = backupSubDir + "/notes.db";
            dbBackupSuccess = QFile::copy(sourceDbPath, destDbPath);
            if (dbBackupSuccess) {
                qDebug() << "成功备份数据库文件到:" << destDbPath;
                
                // 验证备份文件大小
                QFileInfo sourceInfo(sourceDbPath);
                QFileInfo destInfo(destDbPath);
                if (destInfo.size() != sourceInfo.size()) {
                    qWarning() << "备份数据库文件大小不匹配: 源=" << sourceInfo.size() << "字节, 备份=" << destInfo.size() << "字节";
                    // 尝试再次复制
                    QFile::remove(destDbPath);
                    dbBackupSuccess = QFile::copy(sourceDbPath, destDbPath);
                    if (dbBackupSuccess) {
                        qDebug() << "第二次尝试备份数据库文件成功";
                    } else {
                        qCritical() << "第二次尝试备份数据库文件失败";
                    }
                }
            } else {
                qCritical() << "无法备份数据库文件到:" << destDbPath;
            }
        } else {
            // 创建一个空的数据库文件
            QSqlDatabase newDb = QSqlDatabase::addDatabase("QSQLITE", "backup_connection");
            newDb.setDatabaseName(backupSubDir + "/notes.db");
            if (newDb.open()) {
                QSqlQuery query(newDb);
                query.exec("CREATE TABLE IF NOT EXISTS notes ("
                           "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                           "title TEXT, "
                           "content TEXT, "
                           "created_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
                           "updated_at DATETIME DEFAULT CURRENT_TIMESTAMP"
                           ")");
                newDb.close();
                dbBackupSuccess = true;
                qDebug() << "已创建空数据库文件在备份目录";
            }
            QSqlDatabase::removeDatabase("backup_connection");
        }
        
        // 备份媒体文件
        bool mediaBackupSuccess = true;
        if (hasSourceMedia) {
            QString destMediaPath = backupSubDir + "/notes_media";
            if (!QDir().mkpath(destMediaPath)) {
                qCritical() << "无法创建备份媒体目录:" << destMediaPath;
                mediaBackupSuccess = false;
            } else {
                qDebug() << "已创建备份媒体目录:" << destMediaPath;
            
                // 复制媒体文件
                QDir mediaDir(sourceMediaPath);
            QStringList mediaFiles = mediaDir.entryList(QDir::Files);
                int totalFiles = mediaFiles.size();
                int successCount = 0;
            
            for (const QString &file : mediaFiles) {
                    QString sourceFilePath = sourceMediaPath + "/" + file;
                    QString destFilePath = destMediaPath + "/" + file;
                    
                    if (QFile::copy(sourceFilePath, destFilePath)) {
                        // 验证文件大小
                        QFileInfo sourceInfo(sourceFilePath);
                        QFileInfo destInfo(destFilePath);
                        if (destInfo.size() == sourceInfo.size()) {
                            successCount++;
                        } else {
                            qWarning() << "备份媒体文件大小不匹配:" << file;
                            // 尝试再次复制
                            QFile::remove(destFilePath);
                            if (QFile::copy(sourceFilePath, destFilePath)) {
                                successCount++;
                                qDebug() << "第二次尝试备份媒体文件成功:" << file;
                            } else {
                                qWarning() << "第二次尝试备份媒体文件失败:" << file;
                            }
                        }
                    } else {
                        qWarning() << "无法备份媒体文件:" << file;
                    }
                }
                
                qDebug() << "成功备份" << successCount << "个媒体文件，共" << totalFiles << "个";
                mediaBackupSuccess = (successCount > 0 || totalFiles == 0);
            }
        } else {
            // 创建空媒体目录
            if (QDir().mkpath(backupSubDir + "/notes_media")) {
                qDebug() << "已创建空的备份媒体目录";
            } else {
                qWarning() << "无法创建空的备份媒体目录";
            }
        }
        
        // 创建备份信息文件
        QFile infoFile(backupSubDir + "/backup_info.txt");
        if (infoFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&infoFile);
            out << "IntelliMedia Notes 备份信息\n";
            out << "备份日期: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n";
            out << "应用版本: 0.1.0\n";
            out << "源路径: " << notebookLocation << "\n";
            out << "数据库备份: " << (dbBackupSuccess ? "成功" : "失败") << "\n";
            out << "媒体文件备份: " << (mediaBackupSuccess ? "成功" : "失败") << "\n";
            infoFile.close();
            qDebug() << "已创建备份信息文件";
        } else {
            qWarning() << "无法创建备份信息文件";
        }
        
        // 更新上次备份时间和位置
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName());
        settings.setValue("DataStorage/LastBackupTime", QDateTime::currentDateTime());
        settings.setValue("DataStorage/LastBackupLocation", backupSubDir);
        settings.sync();
        
        // 清理旧的备份（保留最近10个）
        QStringList backupDirs = backupDir.entryList(QStringList() << "backup_*", QDir::Dirs, QDir::Time);
        if (backupDirs.size() > 10) {
            qDebug() << "清理旧备份，当前共" << backupDirs.size() << "个备份";
            for (int i = 10; i < backupDirs.size(); i++) {
                QDir oldBackup(backupPath + "/" + backupDirs[i]);
                if (oldBackup.removeRecursively()) {
                    qDebug() << "已删除旧备份:" << backupDirs[i];
                } else {
                    qWarning() << "无法删除旧备份:" << backupDirs[i];
                }
            }
        }
        
        qDebug() << "备份完成，备份路径:" << backupSubDir;
        return dbBackupSuccess || mediaBackupSuccess; // 只要有一种内容备份成功就返回true
    } catch (const std::exception &e) {
        qCritical() << "备份过程中发生异常:" << e.what();
        return false;
    } catch (...) {
        qCritical() << "备份过程中发生未知异常";
        return false;
    }
}

// 从指定路径恢复备份
bool SettingsDialog::restoreFromBackup(const QString &backupPath)
{
    try {
        qDebug() << "开始从备份恢复:" << backupPath;
        
        // 验证备份目录是否有效
        if (!QDir(backupPath).exists()) {
            qCritical() << "备份目录不存在:" << backupPath;
            return false;
        }
        
        // 检查备份文件是否存在
        QString backupDbPath = backupPath + "/notes.db";
        bool hasBackupDb = QFile::exists(backupDbPath);
        if (!hasBackupDb) {
            qCritical() << "备份数据库文件不存在:" << backupDbPath;
            return false;
        }
        
        // 检查备份信息文件
        bool hasBackupInfo = QFile::exists(backupPath + "/backup_info.txt");
        if (hasBackupInfo) {
            QFile infoFile(backupPath + "/backup_info.txt");
            if (infoFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&infoFile);
                QString infoContent = in.readAll();
                infoFile.close();
                qDebug() << "备份信息文件内容:\n" << infoContent;
            }
        }
        
        // 获取当前笔记库位置
        QString notebookLocation = getNotebookPath();
        qDebug() << "当前笔记库位置:" << notebookLocation;
        
        // 确保笔记库目录存在
        QDir notebookDir(notebookLocation);
        if (!notebookDir.exists()) {
            if (!notebookDir.mkpath(".")) {
                qCritical() << "无法创建笔记库目录:" << notebookLocation;
                return false;
            }
            qDebug() << "已创建笔记库目录:" << notebookLocation;
        }
        
        // 在恢复之前先创建当前数据的备份
        QString emergencyBackupPath = notebookLocation + "_emergency_backup_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
        bool emergencyBackupCreated = false;
        
        if (QFile::exists(notebookLocation + "/notes.db") || QDir(notebookLocation + "/notes_media").exists()) {
            qDebug() << "创建恢复前的紧急备份:" << emergencyBackupPath;
            if (QDir().mkpath(emergencyBackupPath)) {
                // 备份现有数据库
                if (QFile::exists(notebookLocation + "/notes.db")) {
                    if (QFile::copy(notebookLocation + "/notes.db", emergencyBackupPath + "/notes.db")) {
                        qDebug() << "已备份当前数据库文件";
                        emergencyBackupCreated = true;
                    } else {
                        qWarning() << "无法备份当前数据库文件";
                    }
                }
                
                // 备份现有媒体文件
                if (QDir(notebookLocation + "/notes_media").exists()) {
                    QDir().mkpath(emergencyBackupPath + "/notes_media");
                    QDir mediaDir(notebookLocation + "/notes_media");
                    QStringList mediaFiles = mediaDir.entryList(QDir::Files);
                    
                    for (const QString &file : mediaFiles) {
                        if (QFile::copy(notebookLocation + "/notes_media/" + file, emergencyBackupPath + "/notes_media/" + file)) {
                            qDebug() << "已备份媒体文件:" << file;
                        } else {
                            qWarning() << "无法备份媒体文件:" << file;
                        }
                    }
                }
                
                // 记录备份信息
                QFile emergencyInfoFile(emergencyBackupPath + "/emergency_backup_info.txt");
                if (emergencyInfoFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream out(&emergencyInfoFile);
                    out << "IntelliMedia Notes 紧急备份 (恢复前)\n";
                    out << "备份日期: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n";
                    out << "原因: 在恢复备份前创建的安全备份\n";
                    out << "恢复来源: " << backupPath << "\n";
                    emergencyInfoFile.close();
                }
            }
        }
        
        // 关闭所有数据库连接
        QStringList connections = QSqlDatabase::connectionNames();
        for (const QString &connectionName : connections) {
            QSqlDatabase db = QSqlDatabase::database(connectionName);
            if (db.isValid() && db.isOpen()) {
            db.close();
                qDebug() << "已关闭数据库连接:" << connectionName;
        }
        }
        
        // 恢复数据库文件
        QString currentDbPath = notebookLocation + "/notes.db";
        bool dbRestored = false;
        
        // 如果当前数据库存在，先删除
        if (QFile::exists(currentDbPath)) {
            if (!QFile::remove(currentDbPath)) {
                qCritical() << "无法删除当前数据库文件:" << currentDbPath;
                return false;
            }
        }
        
        // 复制备份数据库到当前位置
        dbRestored = QFile::copy(backupDbPath, currentDbPath);
        if (!dbRestored) {
            qCritical() << "无法恢复数据库:" << backupDbPath << "->" << currentDbPath;
            
            // 尝试从紧急备份恢复（如果存在）
            if (emergencyBackupCreated && QFile::exists(emergencyBackupPath + "/notes.db")) {
                qWarning() << "尝试从紧急备份恢复数据库";
                if (QFile::copy(emergencyBackupPath + "/notes.db", currentDbPath)) {
                    qDebug() << "已从紧急备份恢复数据库";
                } else {
                    qCritical() << "无法从紧急备份恢复数据库";
                }
            }
            
            return false;
        }
        
        // 验证数据库完整性
        bool dbValid = false;
        {
            QSqlDatabase testDb = QSqlDatabase::addDatabase("QSQLITE", "restore_test_connection");
            testDb.setDatabaseName(currentDbPath);
            if (testDb.open()) {
                QSqlQuery query(testDb);
                if (query.exec("PRAGMA integrity_check")) {
                    if (query.next()) {
                        QString result = query.value(0).toString();
                        dbValid = (result == "ok");
                        qDebug() << "数据库完整性检查结果:" << result;
                    }
                }
                testDb.close();
            }
            QSqlDatabase::removeDatabase("restore_test_connection");
        }
        
        if (!dbValid) {
            qCritical() << "恢复的数据库文件完整性检查失败";
            
            // 尝试从紧急备份恢复（如果存在）
            if (emergencyBackupCreated && QFile::exists(emergencyBackupPath + "/notes.db")) {
                qWarning() << "尝试从紧急备份恢复数据库";
            QFile::remove(currentDbPath);
                if (QFile::copy(emergencyBackupPath + "/notes.db", currentDbPath)) {
                    qDebug() << "已从紧急备份恢复数据库";
                } else {
                    qCritical() << "无法从紧急备份恢复数据库";
        }
            }
            
            return false;
        }
        
        // 恢复媒体文件
        QString currentMediaPath = notebookLocation + "/notes_media";
        QString backupMediaPath = backupPath + "/notes_media";
        bool mediaRestored = true;
        
        if (QDir(backupMediaPath).exists()) {
            // 确保媒体目录存在
            if (!QDir(currentMediaPath).exists()) {
                if (!QDir().mkpath(currentMediaPath)) {
                    qCritical() << "无法创建媒体目录:" << currentMediaPath;
                    mediaRestored = false;
                }
            }
            
            if (mediaRestored) {
            // 清空当前媒体目录
            QDir currentMediaDir(currentMediaPath);
            QStringList currentMediaFiles = currentMediaDir.entryList(QDir::Files);
            for (const QString &file : currentMediaFiles) {
                    QString filePath = currentMediaPath + "/" + file;
                    if (!QFile::remove(filePath)) {
                        qWarning() << "无法删除媒体文件:" << filePath;
                    }
            }
            
            // 复制备份媒体文件
            QDir backupMediaDir(backupMediaPath);
            QStringList backupMediaFiles = backupMediaDir.entryList(QDir::Files);
                int totalFiles = backupMediaFiles.size();
                int successCount = 0;
                
            for (const QString &file : backupMediaFiles) {
                    if (QFile::copy(backupMediaPath + "/" + file, currentMediaPath + "/" + file)) {
                        // 验证文件完整性
                        QFileInfo sourceInfo(backupMediaPath + "/" + file);
                        QFileInfo destInfo(currentMediaPath + "/" + file);
                        if (destInfo.size() == sourceInfo.size()) {
                            successCount++;
                        } else {
                            qWarning() << "恢复的媒体文件大小不匹配:" << file;
                            // 尝试再次复制
                            QFile::remove(currentMediaPath + "/" + file);
                            if (QFile::copy(backupMediaPath + "/" + file, currentMediaPath + "/" + file)) {
                                successCount++;
                            }
                        }
                    } else {
                        qWarning() << "无法恢复媒体文件:" << file;
                    }
                }
                
                qDebug() << "成功恢复" << successCount << "个媒体文件，共" << totalFiles << "个";
                mediaRestored = successCount == totalFiles;
            }
        } else {
            qWarning() << "备份中不存在媒体目录:" << backupMediaPath;
            // 创建空媒体目录
            if (!QDir(currentMediaPath).exists()) {
                QDir().mkpath(currentMediaPath);
                }
            }
        
        // 记录恢复操作
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName());
        settings.setValue("DataStorage/LastRestoreTime", QDateTime::currentDateTime());
        settings.setValue("DataStorage/LastRestoreSource", backupPath);
        settings.sync();
        
        // 创建恢复日志文件
        QFile logFile(notebookLocation + "/restore_log.txt");
        if (logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&logFile);
            out << "IntelliMedia Notes 恢复日志\n";
            out << "恢复日期: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n";
            out << "恢复来源: " << backupPath << "\n";
            out << "数据库恢复: " << (dbRestored ? "成功" : "失败") << "\n";
            out << "媒体文件恢复: " << (mediaRestored ? "成功" : "部分或完全失败") << "\n";
            out << "紧急备份路径: " << (emergencyBackupCreated ? emergencyBackupPath : "未创建") << "\n";
            logFile.close();
        }
        
        qDebug() << "从备份恢复完成，数据库:" << (dbRestored ? "成功" : "失败") 
                << "，媒体文件:" << (mediaRestored ? "成功" : "部分或完全失败");
        
        return dbRestored; // 只要数据库恢复成功，我们认为恢复是成功的
    } catch (const std::exception &e) {
        qCritical() << "恢复过程中发生异常:" << e.what();
        return false;
    } catch (...) {
        qCritical() << "恢复过程中发生未知异常";
        return false;
    }
}

// 导出笔记到指定路径和格式
bool SettingsDialog::exportNotes(const QString &exportPath, const QString &format)
{
    try {
        // 获取当前笔记库位置，使用统一的getNotebookPath方法
        QString notebookLocation = getNotebookPath();
        qDebug() << "导出笔记 - 笔记库位置:" << notebookLocation;
        
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
        
        // 检查数据库文件是否存在
        QString dbPath = notebookLocation + "/notes.db";
        if (!QFile::exists(dbPath)) {
            qCritical() << "Database file does not exist at:" << dbPath;
            return false;
        }
        
        // 确保移除任何现有的同名连接
        QString connectionName = "export_connection_" + QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch());
        if (QSqlDatabase::contains(connectionName)) {
            QSqlDatabase::removeDatabase(connectionName);
        }
        
        // 打开数据库连接
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(dbPath);
        
        if (!db.open()) {
            qCritical() << "Failed to open database for export:" << db.lastError().text();
            return false;
        }
        
        qDebug() << "成功打开数据库连接:" << connectionName << "路径:" << dbPath;
        
        // 先检查notes表的结构
        QSqlQuery schemaQuery(db);
        if (!schemaQuery.exec("PRAGMA table_info(notes)")) {
            qCritical() << "Failed to query notes table schema:" << schemaQuery.lastError().text();
            db.close();
            return false;
        }
        
        // 检查表是否存在
        bool hasIdColumn = false;
        bool hasRowidColumn = false;
        QStringList columns;
        
        while (schemaQuery.next()) {
            QString columnName = schemaQuery.value(1).toString();
            columns << columnName;
            if (columnName.toLower() == "id") {
                hasIdColumn = true;
            }
        }
        
        if (columns.isEmpty()) {
            qCritical() << "Notes table is empty or does not exist";
            db.close();
            return false;
        }
        
        // 构建查询SQL，根据实际列结构
        QString querySQL;
        QString idColumn = columns.contains("id") ? "id" : 
                         columns.contains("note_id") ? "note_id" : "rowid";
        
        // 内容列可能存在不同名称
        QString contentColumn = "";
        if (columns.contains("content")) {
            contentColumn = "content";
        } else if (columns.contains("body")) {
            contentColumn = "body";
        } else if (columns.contains("text")) {
            contentColumn = "text";
        } else {
            // 如果没有找到内容列，需要查询相关表获取内容
            qDebug() << "没有找到内容列，尝试查询是否有关联表存储内容";
            
            // 查询是否有其他表可能存储内容
            QSqlQuery tablesQuery(db);
            if (tablesQuery.exec("SELECT name FROM sqlite_master WHERE type='table'")) {
                QStringList tables;
                while (tablesQuery.next()) {
                    tables << tablesQuery.value(0).toString();
                }
                qDebug() << "数据库中的表:" << tables.join(", ");
                
                // 检查是否有note_contents表
                if (tables.contains("note_contents")) {
                    qDebug() << "检测到note_contents表，可能内容存储在此表中";
                    contentColumn = "(SELECT content FROM note_contents WHERE note_contents.note_id=notes.note_id LIMIT 1) AS content";
                }
            }
            
            // 如果还是没找到，使用空字符串代替
            if (contentColumn.isEmpty()) {
                contentColumn = "'' AS content";
                qWarning() << "未能找到笔记内容存储位置，将导出空内容";
            }
        }
        
        // 检查是否有创建和更新时间
        QString createdAtColumn = columns.contains("created_at") ? "created_at" : 
                                "CURRENT_TIMESTAMP AS created_at";
        QString updatedAtColumn = columns.contains("updated_at") ? "updated_at" : 
                                "CURRENT_TIMESTAMP AS updated_at";
        
        // 构建最终查询SQL
        if (columns.contains(idColumn)) {
            querySQL = QString("SELECT %1 AS id, title, %2, %3, %4 FROM notes WHERE is_trashed = 0 OR is_trashed IS NULL")
                    .arg(idColumn)
                    .arg(contentColumn)
                    .arg(createdAtColumn)
                    .arg(updatedAtColumn);
        } else {
            // 检查是否有rowid
            if (schemaQuery.exec("SELECT rowid FROM notes LIMIT 1")) {
                if (schemaQuery.next()) {
                    hasRowidColumn = true;
                }
            }
            
            if (hasRowidColumn) {
                querySQL = QString("SELECT rowid AS id, title, %1, %2, %3 FROM notes WHERE is_trashed = 0 OR is_trashed IS NULL")
                        .arg(contentColumn)
                        .arg(createdAtColumn)
                        .arg(updatedAtColumn);
            } else {
                querySQL = QString("SELECT title, %1, %2, %3 FROM notes WHERE is_trashed = 0 OR is_trashed IS NULL")
                        .arg(contentColumn)
                        .arg(createdAtColumn)
                        .arg(updatedAtColumn);
            }
        }
        
        qDebug() << "Export query SQL:" << querySQL;
        qDebug() << "Available columns:" << columns.join(", ");
        
        // 查询所有笔记
        QSqlQuery query(db);
        if (!query.exec(querySQL)) {
            qCritical() << "Failed to query notes for export:" << query.lastError().text();
            db.close();
            return false;
        }
        
        // 导出每个笔记
        int noteCount = 0;
        int idIndex = query.record().indexOf("id");
        int titleIndex = query.record().indexOf("title");
        int contentIndex = query.record().indexOf("content");
        int createdAtIndex = query.record().indexOf("created_at");
        int updatedAtIndex = query.record().indexOf("updated_at");
        
                        // 查看SQL查询返回的所有列名称
                QStringList returnedColumns;
                for(int i = 0; i < query.record().count(); i++) {
                    returnedColumns << query.record().fieldName(i);
                }
                
                qDebug() << "查询返回的所有列:" << returnedColumns.join(", ");
                qDebug() << "Field indices - id:" << idIndex << "title:" << titleIndex 
                         << "content:" << contentIndex << "created_at:" << createdAtIndex 
                         << "updated_at:" << updatedAtIndex;
                         
                if (contentIndex < 0) {
                    qDebug() << "提示: 未找到content列，尝试查找其他列作为内容列";
                }
                 
        // 如果没有找到内容列，尝试查找其他可能的列名
        if (contentIndex < 0) {
            for (const QString &possibleName : {"body", "text", "note_content", "note_body"}) {
                contentIndex = query.record().indexOf(possibleName);
                if (contentIndex >= 0) {
                    qDebug() << "找到替代内容列:" << possibleName;
                    break;
                }
            }
        }
        
        while (query.next()) {
            int id = idIndex >= 0 ? query.value(idIndex).toInt() : noteCount + 1;
            QString title = titleIndex >= 0 ? query.value(titleIndex).toString() : tr("无标题笔记");
            QString content = contentIndex >= 0 ? query.value(contentIndex).toString() : "";
            QDateTime createdAt = createdAtIndex >= 0 ? query.value(createdAtIndex).toDateTime() : QDateTime::currentDateTime();
            QDateTime updatedAt = updatedAtIndex >= 0 ? query.value(updatedAtIndex).toDateTime() : QDateTime::currentDateTime();
            
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
        QSqlDatabase::removeDatabase(connectionName);
        qDebug() << "已关闭数据库连接:" << connectionName;
        
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
        // 获取当前笔记库位置，使用统一的getNotebookPath方法
        QString notebookLocation = getNotebookPath();
        qDebug() << "导入笔记 - 笔记库位置:" << notebookLocation;
        
        // 检查数据库文件是否存在
        QString dbPath = notebookLocation + "/notes.db";
        if (!QFile::exists(dbPath)) {
            qCritical() << "Database file does not exist at:" << dbPath;
            return false;
        }
        
        // 确保移除任何现有的同名连接
        QString connectionName = "import_connection_" + QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch());
        if (QSqlDatabase::contains(connectionName)) {
            QSqlDatabase::removeDatabase(connectionName);
        }
        
        // 打开数据库连接
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(dbPath);
        
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
        
        // 检查数据库中的所有表
        QSqlQuery tablesQuery(db);
        QString notesTableName = "notes"; // 默认表名
        
        if (tablesQuery.exec("SELECT name FROM sqlite_master WHERE type='table'")) {
            QStringList tables;
            while (tablesQuery.next()) {
                QString tableName = tablesQuery.value(0).toString();
                tables << tableName;
                
                // 查找Notes表，考虑大小写
                if (tableName.toLower() == "notes") {
                    notesTableName = tableName; // 使用实际的表名（可能是Notes而不是notes）
                }
            }
            qDebug() << "数据库中的表:" << tables.join(", ");
        }
        
        // 先检查notes表的结构，确定正确的列名
        QSqlQuery schemaQuery(db);
        if (!schemaQuery.exec(QString("PRAGMA table_info(%1)").arg(notesTableName))) {
            qCritical() << "Failed to query notes table schema for import:" << schemaQuery.lastError().text();
            db.close();
            return false;
        }
        
        QStringList columns;
        while (schemaQuery.next()) {
            QString columnName = schemaQuery.value(1).toString();
            columns << columnName;
        }
        
        qDebug() << "导入笔记表的列:" << columns.join(", ");
        
        // 确定插入语句的格式
        QString insertSQL;
        QString contentColumnName = "content";
        
        // 检查是否有ContentBlocks或Annotations表作为内容存储
        bool useContentBlocks = false;
        QString contentBlocksTable = "";
        QString contentIdField = "";
        
        // 检查数据库中是否有ContentBlocks或其他可能存储内容的表
        if (tablesQuery.exec("SELECT name FROM sqlite_master WHERE type='table'")) {
            while (tablesQuery.next()) {
                QString tableName = tablesQuery.value(0).toString();
                if (tableName.toLower() == "contentblocks") {
                    useContentBlocks = true;
                    contentBlocksTable = tableName;
                    // 假设内容表使用note_id作为外键
                    contentIdField = "note_id";
                    break;
                } else if (tableName.toLower() == "annotations") {
                    useContentBlocks = true;
                    contentBlocksTable = tableName;
                    contentIdField = "note_id";
                    break;
                }
            }
        }
        
        if (useContentBlocks) {
            qDebug() << "检测到" << contentBlocksTable << "表，将使用它存储内容";
            
            // 检查内容表的具体结构
            QSqlQuery contentSchemaQuery(db);
            if (contentSchemaQuery.exec(QString("PRAGMA table_info(%1)").arg(contentBlocksTable))) {
                QStringList contentColumns;
                QString contentColumn = "content";
                
                while (contentSchemaQuery.next()) {
                    QString colName = contentSchemaQuery.value(1).toString();
                    contentColumns << colName;
                    
                    // 尝试识别可能的内容列
                    if (colName.toLower().contains("content") || 
                        colName.toLower().contains("text") || 
                        colName.toLower().contains("body")) {
                        contentColumn = colName;
                    }
                    
                    // 尝试识别可能的关联ID列
                    if (colName.toLower().contains("note_id") || 
                        colName.toLower().contains("noteid") || 
                        colName.toLower() == "id") {
                        contentIdField = colName;
                    }
                }
                qDebug() << contentBlocksTable << "表的列:" << contentColumns.join(", ");
                qDebug() << "将使用" << contentColumn << "列存储内容，关联字段:" << contentIdField;
            }
            
            // 构建不包含内容的插入SQL，稍后会将内容存储到内容表中
            insertSQL = QString("INSERT INTO %1 (title, created_at, updated_at) VALUES (?, ?, ?)")
                      .arg(notesTableName);
        } else {
            // 不使用独立内容表，尝试直接将内容存储在notes表中
            
            // 检查内容存储在哪个列中
            if (columns.contains("content")) {
                contentColumnName = "content";
            } else if (columns.contains("body")) {
                contentColumnName = "body";
            } else if (columns.contains("text")) {
                contentColumnName = "text";
            } else {
                // 没有找到明确的内容存储列，尝试插入基本数据
                qWarning() << "无法确定内容存储位置，将只导入标题和时间戳";
                
                // 构建不包含内容的插入SQL
                insertSQL = QString("INSERT INTO %1 (title, created_at, updated_at) VALUES (?, ?, ?)")
                          .arg(notesTableName);
                
                // 标记为不使用内容列
                contentColumnName = "";
            }
            
            if (!contentColumnName.isEmpty()) {
                // 确定ID列名
                QString idColumnName = columns.contains("id") ? "id" : 
                                    columns.contains("note_id") ? "note_id" : 
                                    "rowid";
                
                // 构建包含内容的插入SQL
                insertSQL = QString("INSERT INTO %1 (title, %2, created_at, updated_at) VALUES (?, ?, ?, ?)")
                          .arg(notesTableName)
                          .arg(contentColumnName);
            }
        }
        
        // 记录最终决定的SQL语句
        bool includesContent = useContentBlocks ? false : !contentColumnName.isEmpty();
                  
        qDebug() << "导入SQL:" << insertSQL;
        
        // 准备插入查询
        QSqlQuery query(db);
        if (!query.prepare(insertSQL)) {
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
            
            if (useContentBlocks) {
                // 使用两张表存储：先在Notes表中插入记录，再在ContentBlocks表中插入内容
                QSqlQuery notesQuery(db);
                if (!notesQuery.prepare(insertSQL)) {
                    qWarning() << "Failed to prepare notes query:" << notesQuery.lastError().text();
                    continue;
                }
                
                // 绑定参数并执行查询
                notesQuery.bindValue(0, title);
                notesQuery.bindValue(1, currentTime);
                notesQuery.bindValue(2, currentTime);
                
                // 执行Notes表插入
                if (!notesQuery.exec()) {
                    qWarning() << "Failed to insert into Notes table:" << notesQuery.lastError().text();
                    continue;
                }
                
                // 获取新插入记录的ID
                int noteId = 0;
                QSqlQuery lastIdQuery(db);
                if (lastIdQuery.exec("SELECT last_insert_rowid()") && lastIdQuery.next()) {
                    noteId = lastIdQuery.value(0).toInt();
                    qDebug() << "插入的笔记ID:" << noteId;
                } else {
                    qWarning() << "Failed to get last insert ID:" << lastIdQuery.lastError().text();
                    continue;
                }
                
                // 向ContentBlocks表中插入内容
                // 找出ContentBlocks表所需的所有必填字段
                QStringList requiredColumns;
                QStringList allColumns;
                QString contentColumn = "content_text"; // 默认内容列
                
                QSqlQuery contentColQuery(db);
                QString checkContentSQL = QString("PRAGMA table_info(%1)").arg(contentBlocksTable);
                if (contentColQuery.exec(checkContentSQL)) {
                    while (contentColQuery.next()) {
                        QString colName = contentColQuery.value(1).toString();
                        int notNull = contentColQuery.value(3).toInt(); // NOT NULL约束
                        int hasDefault = contentColQuery.value(4).isNull() ? 0 : 1; // 是否有默认值
                        
                        // 添加到所有列列表
                        allColumns << colName;
                        
                        // 添加所有必填列（没有默认值的非空列）
                        if (notNull == 1 && hasDefault == 0 && colName != contentIdField) {
                            requiredColumns << colName;
                        }
                        
                        // 找出内容列
                        if (colName.toLower().contains("content") || 
                            colName.toLower().contains("text") || 
                            colName.toLower().contains("body")) {
                            contentColumn = colName;
                            qDebug() << "找到内容列:" << contentColumn;
                        }
                    }
                }
                
                qDebug() << "ContentBlocks表所有列:" << allColumns.join(", ");
                qDebug() << "ContentBlocks表必填列:" << requiredColumns.join(", ");
                
                // 构建插入SQL，包含所有必要的字段
                QStringList insertColumns;
                insertColumns << contentIdField; // 必须包含关联ID字段
                insertColumns << contentColumn;  // 必须包含内容字段
                
                // 添加其他必填字段（如position, block_type等）
                for (const QString &col : requiredColumns) {
                    if (col != contentIdField && col != contentColumn) {
                        insertColumns << col;
                    }
                }
                
                // 构建SQL语句，对应每个字段使用一个?占位符
                QString insertColumnStr = insertColumns.join(", ");
                QStringList placeholders;
                for (int i = 0; i < insertColumns.size(); i++) {
                    placeholders << "?";
                }
                QString placeholderStr = placeholders.join(", ");
                
                QString contentInsertSQL = QString("INSERT INTO %1 (%2) VALUES (%3)")
                                        .arg(contentBlocksTable)
                                        .arg(insertColumnStr)
                                        .arg(placeholderStr);
                                        
                qDebug() << "内容插入SQL:" << contentInsertSQL;
                
                QSqlQuery contentQuery(db);
                if (!contentQuery.prepare(contentInsertSQL)) {
                    qWarning() << "Failed to prepare content query:" << contentQuery.lastError().text();
                    continue;
                }
                
                // 按顺序绑定所有参数
                int paramIndex = 0;
                
                // 绑定noteId和内容
                contentQuery.bindValue(paramIndex++, noteId);
                contentQuery.bindValue(paramIndex++, content);
                
                // 绑定其他必填参数（为其提供合理的默认值）
                for (int i = 2; i < insertColumns.size(); i++) {
                    QString colName = insertColumns[i];
                    
                    // 根据列名设置合理的默认值
                    if (colName.toLower().contains("position")) {
                        // 位置通常是数字
                        contentQuery.bindValue(paramIndex++, 0);
                    } 
                    else if (colName.toLower().contains("type")) {
                        // 类型通常是标识符
                        contentQuery.bindValue(paramIndex++, "text");
                    }
                    else if (colName.toLower().contains("media") || 
                             colName.toLower().contains("path")) {
                        // 媒体路径默认为空
                        contentQuery.bindValue(paramIndex++, "");
                    }
                    else if (colName.toLower().contains("properties")) {
                        // 属性默认为空JSON
                        contentQuery.bindValue(paramIndex++, "{}");
                    }
                    else {
                        // 其他字段默认为空字符串
                        contentQuery.bindValue(paramIndex++, "");
                    }
                }
                
                if (!contentQuery.exec()) {
                    qWarning() << "Failed to insert into content table:" << contentQuery.lastError().text() 
                              << " SQL:" << contentQuery.lastQuery();
                    
                    // 尝试回滚笔记插入
                    QSqlQuery deleteQuery(db);
                    if (!deleteQuery.exec(QString("DELETE FROM %1 WHERE rowid = %2").arg(notesTableName).arg(noteId))) {
                        qWarning() << "Failed to rollback note insert:" << deleteQuery.lastError().text();
                    }
                    continue;
                }
                
                importCount++;
            } else if (!contentColumnName.isEmpty()) {
                // 直接在Notes表中存储内容
                QSqlQuery query(db);
                if (!query.prepare(insertSQL)) {
                    qWarning() << "Failed to prepare query with content:" << query.lastError().text();
                    continue;
                }
                
                // 绑定参数并执行查询
                query.bindValue(0, title);
                query.bindValue(1, content);
                query.bindValue(2, currentTime);
                query.bindValue(3, currentTime);
                
                if (!query.exec()) {
                    qWarning() << "Failed to import note with content:" << query.lastError().text();
                    continue;
                }
                
                importCount++;
            } else {
                // 只存储标题和时间戳
                QSqlQuery query(db);
                if (!query.prepare(insertSQL)) {
                    qWarning() << "Failed to prepare basic query:" << query.lastError().text();
                    continue;
                }
                
                // 绑定参数并执行查询
                query.bindValue(0, title);
                query.bindValue(1, currentTime);
                query.bindValue(2, currentTime);
                
                if (!query.exec()) {
                    qWarning() << "Failed to import basic note:" << query.lastError().text();
                    continue;
                }
                
                importCount++;
            }
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
        QSqlDatabase::removeDatabase(connectionName);
        qDebug() << "已关闭数据库连接:" << connectionName;
        
        return importCount > 0;
    } catch (const std::exception &e) {
        qCritical() << "Exception during import:" << e.what();
        return false;
    }
}

// 开机自动启动设置改变时的槽函数
void SettingsDialog::onAutoStartChanged(int state)
{
    bool enable = (state == Qt::Checked);
    
    // 设置开机自动启动
    bool success = setAutoStart(enable);
    
    if (!success) {
        // 如果设置失败，恢复复选框状态
        QSignalBlocker blocker(m_autoStartCheck);
        m_autoStartCheck->setChecked(!enable);
        
        // 显示错误消息
        QString errorMsg = enable ? 
            tr("无法设置开机自动启动，可能需要管理员权限。") : 
            tr("无法取消开机自动启动，可能需要管理员权限。");
        QMessageBox::warning(this, tr("设置失败"), errorMsg);
    }
}

// 设置开机自动启动
bool SettingsDialog::setAutoStart(bool enable)
{
    try {
        #ifdef Q_OS_WIN
            // Windows平台使用注册表设置开机自动启动
            QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
                              QSettings::NativeFormat);
            
            QString appName = QApplication::applicationName();
            
            if (enable) {
                // 获取应用程序路径
                QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
                // 设置注册表键值
                settings.setValue(appName, appPath);
            } else {
                // 删除注册表键值
                settings.remove(appName);
            }
            
            return true;
        #else
            // 其他平台暂不实现
            // TODO: 实现其他平台的开机自动启动设置
            QMessageBox::information(this, tr("功能未实现"), 
                tr("当前平台暂不支持开机自动启动功能。"));
            return false;
        #endif
    } catch (const std::exception &e) {
        qCritical() << "设置开机自动启动异常:" << e.what();
        return false;
    }
}

// 检查是否设置了开机自动启动
bool SettingsDialog::checkAutoStart()
{
    try {
        #ifdef Q_OS_WIN
            // Windows平台使用注册表检查开机自动启动
            QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
                              QSettings::NativeFormat);
            
            QString appName = QApplication::applicationName();
            
            // 检查注册表键值是否存在
            return settings.contains(appName);
        #else
            // 其他平台暂不实现
            return false;
        #endif
    } catch (const std::exception &e) {
        qCritical() << "检查开机自动启动异常:" << e.what();
        return false;
    }
} 

// 在应用启动时执行待处理的笔记库位置移动
bool SettingsDialog::executePendingNotebookMove()
{
    // 检查是否有待处理的笔记库位置移动
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName());
    bool pendingMove = settings.value("DataStorage/PendingNotebookMove", false).toBool();
    
    if (!pendingMove) {
        return false; // 没有待处理的移动操作
    }
    
    // 获取新旧路径
    QString oldPath = settings.value("DataStorage/OldNotebookLocation", "").toString();
    QString newPath = settings.value("DataStorage/NewNotebookLocation", "").toString();
    
    qDebug() << "执行待处理的笔记库移动: " << oldPath << " -> " << newPath;
    
    if (oldPath.isEmpty() || newPath.isEmpty() || oldPath == newPath) {
        // 清除待处理标记
        settings.setValue("DataStorage/PendingNotebookMove", false);
        settings.sync();
        return false;
    }
    
    // 确保新路径存在
    QDir newDir(newPath);
    if (!newDir.exists()) {
        if (!newDir.mkpath(".")) {
            qCritical() << "无法创建目标笔记库目录:" << newPath;
            return false;
        }
        qDebug() << "已创建目标笔记库目录:" << newPath;
    }
    
    // 创建notes_media目录
    if (!newDir.exists("notes_media") && !newDir.mkpath("notes_media")) {
        qCritical() << "无法创建目标媒体目录:" << newPath + "/notes_media";
        return false;
    }
    
    // 首先创建一个备份，以防移动失败
    QString backupPath = oldPath + "_backup_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    bool backupCreated = false;
    
    // 只有当原始目录包含有效数据时才创建备份
    if (QDir(oldPath).exists() && (QFile::exists(oldPath + "/notes.db") || QDir(oldPath + "/notes_media").exists())) {
        qDebug() << "创建源数据备份到:" << backupPath;
        QDir().mkpath(backupPath);
        
        // 复制数据库文件到备份
        if (QFile::exists(oldPath + "/notes.db")) {
            if (QFile::copy(oldPath + "/notes.db", backupPath + "/notes.db")) {
                qDebug() << "已备份数据库文件";
                backupCreated = true;
            } else {
                qWarning() << "无法备份数据库文件";
            }
        }
        
        // 复制媒体文件到备份
        QDir mediaDir(oldPath + "/notes_media");
        if (mediaDir.exists()) {
            QDir().mkpath(backupPath + "/notes_media");
            QStringList mediaFiles = mediaDir.entryList(QDir::Files);
            for (const QString &file : mediaFiles) {
                if (QFile::copy(oldPath + "/notes_media/" + file, backupPath + "/notes_media/" + file)) {
                    qDebug() << "已备份媒体文件:" << file;
                } else {
                    qWarning() << "无法备份媒体文件:" << file;
                }
            }
        }
    }
    
    // 移动数据库文件
    QString oldDbPath = oldPath + "/notes.db";
    QString newDbPath = newPath + "/notes.db";
    
    bool dbMoved = false;
    if (QFile::exists(oldDbPath)) {
        qDebug() << "移动数据库文件:" << oldDbPath << "->" << newDbPath;
        
        // 确保目标位置不存在同名文件
        if (QFile::exists(newDbPath)) {
            QFile::remove(newDbPath);
        }
        
        // 复制文件
        if (QFile::copy(oldDbPath, newDbPath)) {
            // 验证文件大小
            QFileInfo sourceInfo(oldDbPath);
            QFileInfo destInfo(newDbPath);
            
            if (destInfo.size() == sourceInfo.size()) {
                // 删除源文件
                QFile file(oldDbPath);
                if (file.remove()) {
                    dbMoved = true;
                    qDebug() << "成功移动数据库文件";
                } else {
                    qWarning() << "无法删除源数据库文件，但已复制到新位置:" << file.errorString();
                    // 仍然认为是成功的，因为数据已经复制
                    dbMoved = true;
                }
            } else {
                qCritical() << "目标数据库文件大小不匹配，复制可能不完整";
                return false;
            }
        } else {
            qCritical() << "无法复制数据库文件到新位置";
            return false;
        }
    } else {
        // 如果旧路径中不存在数据库，创建新的空数据库
        qDebug() << "创建新的空数据库:" << newDbPath;
        QSqlDatabase newDb = QSqlDatabase::addDatabase("QSQLITE", "temp_connection");
        newDb.setDatabaseName(newDbPath);
        if (newDb.open()) {
            QSqlQuery query(newDb);
            // 创建基本的笔记表
            query.exec("CREATE TABLE IF NOT EXISTS notes ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                       "title TEXT, "
                       "content TEXT, "
                       "created_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
                       "updated_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
                       "is_deleted INTEGER DEFAULT 0, "
                       "category_id INTEGER DEFAULT 0, "
                       "tags TEXT, "
                       "priority INTEGER DEFAULT 0, "
                       "color TEXT"
                       ")");
            
            // 创建分类表
            query.exec("CREATE TABLE IF NOT EXISTS categories ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                       "name TEXT, "
                       "color TEXT, "
                       "icon TEXT, "
                       "parent_id INTEGER DEFAULT 0, "
                       "sort_order INTEGER DEFAULT 0"
                       ")");
            
            newDb.close();
            dbMoved = true;
        } else {
            qCritical() << "无法创建新数据库:" << newDb.lastError().text();
        }
        QSqlDatabase::removeDatabase("temp_connection");
    }
    
    if (!dbMoved) {
        qCritical() << "数据库移动失败";
        return false;
    }
    
    // 移动媒体文件
    QString oldMediaPath = oldPath + "/notes_media";
    QString newMediaPath = newPath + "/notes_media";
    
    QDir oldMediaDir(oldMediaPath);
    bool mediaMoved = true;
    
    if (oldMediaDir.exists()) {
        qDebug() << "移动媒体文件从:" << oldMediaPath << "到:" << newMediaPath;
        QStringList mediaFiles = oldMediaDir.entryList(QDir::Files);
        int totalFiles = mediaFiles.size();
        int successfulMoves = 0;
        
        for (const QString &file : mediaFiles) {
            QString oldFilePath = oldMediaPath + "/" + file;
            QString newFilePath = newMediaPath + "/" + file;
            
            if (QFile::exists(newFilePath)) {
                // 如果目标文件已存在，先删除
                if (!QFile::remove(newFilePath)) {
                    qWarning() << "无法删除目标媒体文件:" << newFilePath;
                    continue;
                }
            }
            
            if (QFile::copy(oldFilePath, newFilePath)) {
                if (QFile::remove(oldFilePath)) {
                    successfulMoves++;
                    qDebug() << "成功移动媒体文件:" << file;
                } else {
                    qWarning() << "无法删除源媒体文件，但已复制:" << oldFilePath;
                    successfulMoves++;
                }
            } else {
                qWarning() << "无法移动媒体文件:" << file;
            }
        }
        
        qDebug() << "成功移动" << successfulMoves << "个媒体文件，共" << totalFiles << "个";
        
        // 如果至少有一个文件移动成功，或者没有文件需要移动，则认为媒体移动成功
        mediaMoved = (successfulMoves > 0 || totalFiles == 0);
        
        // 尝试删除旧媒体目录（如果已经空了）
        if (oldMediaDir.entryList(QDir::Files).isEmpty()) {
            if (!oldMediaDir.rmdir(".")) {
                qWarning() << "无法删除旧的媒体目录，但它已经是空的";
            }
        }
    }
    
    // 更新设置
    settings.setValue("DataStorage/NotebookLocation", newPath);
    settings.setValue("DataStorage/PendingNotebookMove", false);
    settings.remove("DataStorage/NewNotebookLocation");
    settings.remove("DataStorage/OldNotebookLocation");
    settings.sync();
    
    qDebug() << "笔记库位置已更新为:" << newPath;
    
    return true;
} 

// 在应用启动时执行待处理的备份恢复操作
bool SettingsDialog::executePendingRestore()
{
    // 检查是否有待处理的恢复操作
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName());
    bool pendingRestore = settings.value("DataStorage/PendingRestore", false).toBool();
    
    if (!pendingRestore) {
        return false; // 没有待处理的恢复操作
    }
    
    // 获取要恢复的备份路径
    QString backupPath = settings.value("DataStorage/PendingRestorePath", "").toString();
    if (backupPath.isEmpty() || !QDir(backupPath).exists()) {
        qWarning() << "待恢复的备份路径无效:" << backupPath;
        settings.setValue("DataStorage/PendingRestore", false);
        settings.sync();
        return false;
    }
    
    qDebug() << "执行待处理的备份恢复操作，备份路径:" << backupPath;
    
    // 创建临时SettingsDialog实例并执行恢复操作
    SettingsDialog tempDialog;
    bool success = tempDialog.restoreFromBackup(backupPath);
    
    // 清除待处理标志
    settings.setValue("DataStorage/PendingRestore", false);
    settings.remove("DataStorage/PendingRestorePath");
    settings.sync();
    
    if (success) {
        qDebug() << "备份恢复成功完成";
        
        // 在成功恢复后更新笔记库位置设置（以防备份来自不同位置）
        QString notebookPath = getNotebookPath();
        settings.setValue("DataStorage/NotebookLocation", notebookPath);
        settings.sync();
    } else {
        qCritical() << "备份恢复失败";
    }
    
    return success;
} 