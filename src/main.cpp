#include <QApplication>

#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QApplication::setApplicationName("SillyChat");
    QApplication::setApplicationVersion("0.1.0");

    MainWindow window;
    window.show();

    return app.exec();
}

