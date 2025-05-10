#pragma once

#include <QDialog>
#include <QSettings>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRadioButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDebug>
#include <QFontDialog>
#include <QLineEdit>
#include <QGroupBox>
#include <QTextBrowser>
#include <QProgressBar>

/**
 * 设置对话框类，负责管理和应用应用程序设置
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * 构造函数
     * @param parent 父窗口
     */
    explicit SettingsDialog(QWidget *parent = nullptr);
    
    /**
     * 析构函数
     */
    ~SettingsDialog();

    /**
     * 静态方法，应用设置
     */
    static void applySettings();
    
    /**
     * 加载设置
     */
    void loadSettings();
    
    /**
     * 获取笔记库路径的静态方法
     * @return 笔记库路径
     */
    static QString getNotebookPath();

signals:
    /**
     * 主题更改信号
     * @param theme 主题名称
     */
    void themeChanged(const QString &theme);
    
    /**
     * 语言更改信号
     * @param language 语言代码
     */
    void languageChanged(const QString &language);

protected:
    /**
     * 关闭事件处理
     * @param event 关闭事件
     */
    void closeEvent(QCloseEvent *event) override;

private slots:
    /**
     * 主题更改处理函数
     * @param button 被选中的按钮
     */
    void onThemeChanged(QAbstractButton *button);
    
    /**
     * 语言更改处理函数
     * @param index 当前选中的索引
     */
    void onLanguageChanged(int index);
    
    /**
     * 自动保存设置更改处理函数
     * @param state 复选框状态
     */
    void onAutoSaveChanged(int state);
    
    /**
     * 自定义背景按钮点击处理函数
     */
    void onCustomBackgroundClicked();
    
    /**
     * 选择字体按钮点击处理函数
     */
    void onSelectFontClicked();
    
    /**
     * 制表符宽度改变处理函数
     * @param value 新的制表符宽度
     */
    void onTabWidthChanged(int value);
    
    /**
     * 自动配对括号/引号设置更改处理函数
     * @param state 复选框状态
     */
    void onAutoPairChanged(int state);
    
    /**
     * 显示/隐藏API密钥按钮点击处理函数
     */
    void onToggleApiKeyVisibilityClicked();
    
    /**
     * 测试API连接按钮点击处理函数
     */
    void onTestApiConnectionClicked();
    
    /**
     * 选择笔记库位置按钮点击处理函数
     */
    void onSelectNotebookLocationClicked();
    
    /**
     * 立即备份按钮点击处理函数
     */
    void onBackupNowClicked();
    
    /**
     * 从备份恢复按钮点击处理函数
     */
    void onRestoreFromBackupClicked();
    
    /**
     * 导出笔记按钮点击处理函数
     */
    void onExportNotesClicked();
    
    /**
     * 导入笔记按钮点击处理函数
     */
    void onImportNotesClicked();

private:
    /**
     * 初始化UI
     */
    void setupUI();
    
    /**
     * 初始化常规设置标签页
     */
    void setupGeneralTab();
    
    /**
     * 初始化编辑器设置标签页
     */
    void setupEditorTab();
    
    /**
     * 初始化AI服务设置标签页
     */
    void setupAiServiceTab();
    
    /**
     * 初始化数据与存储设置标签页
     */
    void setupDataStorageTab();
    
    /**
     * 初始化关于标签页
     */
    void setupAboutTab();
    
    /**
     * 混淆API密钥
     * @param apiKey API密钥
     * @return 混淆后的API密钥
     */
    QString obfuscateApiKey(const QString &apiKey);
    
    /**
     * 解混淆API密钥
     * @param obfuscatedApiKey 混淆后的API密钥
     * @return 原始API密钥
     */
    QString deobfuscateApiKey(const QString &obfuscatedApiKey);
    
    /**
     * 保存设置
     */
    void saveSettings();
    
    /**
     * 备份数据到指定路径
     * @param backupPath 备份路径
     * @return 备份是否成功
     */
    bool backupData(const QString &backupPath);
    
    /**
     * 从指定路径恢复备份
     * @param backupPath 备份路径
     * @return 恢复是否成功
     */
    bool restoreFromBackup(const QString &backupPath);
    
    /**
     * 导出笔记到指定路径和格式
     * @param exportPath 导出路径
     * @param format 导出格式
     * @return 导出是否成功
     */
    bool exportNotes(const QString &exportPath, const QString &format);
    
    /**
     * 从指定路径导入笔记
     * @param importPath 导入路径
     * @return 导入是否成功
     */
    bool importNotes(const QString &importPath);
    
    // UI元素
    QTabWidget *m_tabWidget;
    
    // 常规设置
    QWidget *m_generalTab;
    QButtonGroup *m_themeGroup;
    QRadioButton *m_lightThemeRadio;
    QRadioButton *m_darkThemeRadio;
    QRadioButton *m_systemThemeRadio;
    QPushButton *m_customBackgroundBtn;
    QComboBox *m_languageCombo;
    QCheckBox *m_autoSaveCheck;
    QComboBox *m_autoSaveIntervalCombo;
    QCheckBox *m_startMinimizedCheck;
    QCheckBox *m_checkUpdatesCheck;
    
    // 编辑器设置
    QWidget *m_editorTab;
    QLabel *m_currentFontLabel;
    QPushButton *m_selectFontBtn;
    QSpinBox *m_tabWidthSpin;
    QCheckBox *m_autoPairCheck;
    QFont m_selectedFont;
    
    // AI服务设置
    QWidget *m_aiServiceTab;
    QLineEdit *m_apiKeyEdit;
    QPushButton *m_toggleApiKeyVisibilityBtn;
    QLineEdit *m_apiEndpointEdit;
    QPushButton *m_testApiConnectionBtn;
    QLabel *m_apiStatusLabel;
    bool m_apiKeyVisible;
    
    // 数据与存储设置
    QWidget *m_dataStorageTab;
    QLabel *m_notebookLocationLabel;
    QPushButton *m_selectNotebookLocationBtn;
    QPushButton *m_backupNowBtn;
    QPushButton *m_restoreFromBackupBtn;
    QPushButton *m_exportNotesBtn;
    QPushButton *m_importNotesBtn;
    QProgressBar *m_operationProgressBar;
    QLabel *m_operationStatusLabel;
    
    // 关于
    QWidget *m_aboutTab;
    QTextBrowser *m_aboutTextBrowser;
    QLabel *m_versionLabel;
    QLabel *m_copyrightLabel;
    QPushButton *m_websiteBtn;
    
    QSettings m_settings;
    
    QTimer *m_autoSaveTimer;
};