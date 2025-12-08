#include "MainWindow.h"
#include <QApplication>
#include <QFont>
#include <QPalette>
#include <QColor>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    // Set a cross-platform default font
    QFont font("Sans Serif", 10);
    font.setStyleStrategy(QFont::PreferAntialias);
    a.setFont(font);

    // Telegram-like Palette
    QPalette p = a.palette();
    p.setColor(QPalette::Window, QColor(245, 245, 245));
    a.setPalette(p);

    MainWindow w;
    w.show();
    return a.exec();
}
