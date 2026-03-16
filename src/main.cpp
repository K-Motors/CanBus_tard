#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>
#include <QMessageBox>
#include <QStyle>

int main(int argc, char* argv[])
{
    qSetMessagePattern("%{time yyyy-MM-dd hh:mm:ss} %{type} %{file}:%{line} --- %{message}");

    QApplication app(argc, argv);

    QStyle* style = QStyleFactory::create("Fusion");
    if (style)
    {
        app.setStyle(style);
        QPalette palette = style->standardPalette();
        app.setPalette(palette);
    }
    else
    {
        QMessageBox(QMessageBox::Information, "Style error", "Fusion style not available...").exec();
    }

    QSettings  settings("KMotors", "CanBus-tard");
    MainWindow w(&settings);
    w.show();

    return app.exec();
}
