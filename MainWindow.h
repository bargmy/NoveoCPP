#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QMap>
#include "WebSocketClient.h"

// Forward declarations
class QListWidget;
class QLineEdit;
class QLabel;
class QPushButton;
class QStackedWidget;
class QTabWidget;
class QCheckBox;

class MainWindow : public QMainWindow {
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
    void onLogoutClicked();
    void onDarkModeToggled(bool checked);
    
    // New Slots
    void onUserTyping(const QString &chatId, const QString &username);
    void onUserPresenceChanged(const QString &userId, bool online);
    void onMessageDeleted(const QString &chatId, const QString &messageId);
    void onInputTextChanged(const QString &text);
    void onMessageContextMenu(const QPoint &pos);
    void onAvatarDownloaded(QNetworkReply *reply);

private:
    WebSocketClient *m_client;
    QNetworkAccessManager *m_nam;
    
    QMap<QString, Chat> m_chats;
    QMap<QString, User> m_users;
    QMap<QString, QIcon> m_avatarCache; // Cache for loaded PFPs

    QString m_currentChatId;
    bool m_isDarkMode = false;
    
    // Typing Logic
    QTimer *m_typingTimer;
    QTimer *m_typingClearTimer; // Clears "User is typing..." label
    QString m_replyToId; // Store ID if replying

    // UI Elements
    QStackedWidget *m_stackedWidget;
    QWidget *m_loginPage;
    QLineEdit *m_usernameInput;
    QLineEdit *m_passwordInput;
    QLabel *m_statusLabel;
    QPushButton *m_loginBtn;

    QWidget *m_appPage;
    QTabWidget *m_sidebarTabs;
    QListWidget *m_chatListWidget; // Left sidebar chats
    QListWidget *m_contactListWidget; // Left sidebar contacts
    QWidget *m_settingsTab;
    
    QWidget *m_chatAreaWidget;
    QLabel *m_chatTitle;
    QLabel *m_typingLabel; // Shows "X is typing..."
    QListWidget *m_chatList; // The messages area
    
    QWidget *m_replyContainer; // Shows "Replying to..."
    QLabel *m_replyLabel;
    QPushButton *m_cancelReplyBtn;

    QLineEdit *m_messageInput;
    QPushButton *m_sendBtn;

    void setupUi();
    void applyTheme();
    void renderMessages(const QString &chatId);
    void addMessageBubble(const Message &msg);
    QString resolveChatName(const Chat &chat);
    
    // PFP Helper
    void loadAvatar(const QString &url, const QString &targetId, bool isChat);
    QIcon getAvatar(const QString &id);

    void smoothScrollToBottom();
};

#endif // MAINWINDOW_H
