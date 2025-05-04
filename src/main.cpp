#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QDebug>
#include <QQuickStyle>

// 加载并应用样式表的辅助函数
QString loadStyleSheet(const QString &sheetName)
{
    QFile file(sheetName);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString styleSheet = QLatin1String(file.readAll());
        file.close();
        return styleSheet;
    } else {
        qWarning() << "无法打开样式表文件:" << sheetName;
        return QString();
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 设置 QML 控件样式为 Fusion (必须在 MainWindow 创建之前)
    QQuickStyle::setStyle("Fusion");

    // 加载全局样式和浅色主题样式
    QString globalStyle = loadStyleSheet(":/styles/style.qss");
    QString lightThemeStyle = loadStyleSheet(":/styles/light_theme.qss");
    
    // 应用组合样式
    a.setStyleSheet(globalStyle + lightThemeStyle);

    MainWindow w;
    w.show();
    return a.exec();
}
