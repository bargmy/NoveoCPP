#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QWebEngineView;
class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    void setupUi();
    void loadClient();
    void injectVersionIntoSidebar();

private:
    QWebEngineView* m_webView = nullptr;
    QLabel* m_statusLabel = nullptr;
    QString m_version;
};

#endif // MAINWINDOW_H
