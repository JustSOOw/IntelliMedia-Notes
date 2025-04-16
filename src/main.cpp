#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QDebug>

// 加载并应用样式表的辅助函数
void loadStyleSheet(const QString &sheetName)
{
    QFile file(sheetName);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString styleSheet = QLatin1String(file.readAll());
        qApp->setStyleSheet(styleSheet);
        file.close();
    } else {
        qWarning() << "无法打开样式表文件:" << sheetName;
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 启动时加载默认浅色主题
    loadStyleSheet("://styles/light_theme.qss"); // 从资源加载

    MainWindow w;
    w.show();
    return a.exec();
}
