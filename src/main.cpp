#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QSettings>

#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QSettings::setDefaultFormat(QSettings::IniFormat);

    QSettings::setPath(
        QSettings::IniFormat,
        QSettings::UserScope,
        QCoreApplication::applicationDirPath()
    );

    QApplication::setApplicationName("SillyChat");
    QApplication::setApplicationVersion("0.1.0");

    MainWindow window;
    window.show();

    return app.exec();
}

