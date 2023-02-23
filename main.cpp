#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QFileSystemWatcher>

void LoadStyleSheet()
{
#ifdef QT_DEBUG
    QFileSystemWatcher fileWatcher;
    fileWatcher.addPath(":/res/BasicTheme.qss");
    QObject::connect(&fileWatcher, &QFileSystemWatcher::fileChanged, [](const QString& path){
        QFile file(path);
        if(file.open(QIODevice::ReadOnly)){
            qobject_cast<QApplication*>(QApplication::instance())->setStyleSheet(file.readAll());
            file.close();
        }
    });  
#endif

    QFile file(":/res/BasicTheme.qss");
    if (file.open(QIODevice::ReadOnly)) {
        qApp->setStyleSheet(file.readAll());
        file.close();
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/res/recorder.png"));
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "arecorder_" + QLocale(locale).name();
        if (translator.load(":/res/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    LoadStyleSheet();
    MainWindow w;
    w.show();
    return a.exec();
}
