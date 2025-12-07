#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMap>
#include <QTabWidget>
#include <QNetworkAccessManager>
#include <QCheckBox>
#include "WebSocketClient.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onLoginBtnClicked();
    
    void onConnected();
    void onLoginSuccess(const User &user, const QString &token);
    void onAuthFailed(const QString &msg);
    void onChatHistoryReceived(const std::vector<Chat> &chats);
    void onMessageReceived(const Message &msg);
    void onUserListUpdated(const std::vector<User> &users);
    void onNewChatCreated(const Chat &chat);
    
    void onChatSelected(QListWidgetItem *item);
    void onContactSelected(QListWidgetItem *item);
    void onSendBtnClicked();
    
    // Settings Slots
    void onLogoutClicked();
    void onDarkModeToggled(bool checked);

private:
    WebSocketClient *m_client;
    QNetworkAccessManager *m_nam;
    
    QMap<QString, Chat> m_chats; 
    QMap<QString, User> m_users;
    QString m_currentChatId;
    bool m_isDarkMode = false;

    // UI Elements
    QStackedWidget *m_stackedWidget;
    
    // Login Page
    QWidget *m_loginPage;
    QLineEdit *m_usernameInput;
    QLineEdit *m_passwordInput;
    QLabel *m_statusLabel;
    QPushButton *m_loginBtn;

    // Main App Page
    QWidget *m_appPage;
    QTabWidget *m_sidebarTabs;
    QListWidget *m_chatListWidget;
    QListWidget *m_contactListWidget;
    QWidget *m_settingsTab; // New Settings Tab
    
    // Chat Area
    QWidget *m_chatAreaWidget;
    QLabel *m_chatTitle;
    QScrollArea *m_messageScrollArea;
    QWidget *m_messageContainer;
    QVBoxLayout *m_messageLayout;
    
    QLineEdit *m_messageInput;
    QPushButton *m_sendBtn;

    void setupUi();
    void applyTheme(); // Dark/Light mode logic
    void renderMessages(const QString &chatId);
    void addMessageBubble(const Message &msg, bool appendStretch = true, bool animate = false);
    QString resolveChatName(const Chat &chat);
    
    // Avatars
    QIcon getAvatar(const QString &name, const QString &url); 
    QColor getColorForName(const QString &name);
    
    // Scroll Logic
    bool isScrolledToBottom() const;
    void scrollToBottom();
    void smoothScrollToBottom();
};

#endif // MAINWINDOW_H
