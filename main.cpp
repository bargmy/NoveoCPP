#include "MainWindow.h"
#include <QApplication>
#include <QFontDatabase>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // Set a nice default font
    QFont font("Segoe UI", 10);
    a.setFont(font);
    
    // Telegram-like Palette
    QPalette p = a.palette();
    p.setColor(QPalette::Window, QColor(245, 245, 245));
    a.setPalette(p);

    MainWindow w;
    w.show();
    return a.exec();
}
