#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QMap>
#include "WebSocketClient.h"

// Forward declarations to reduce compile times
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
    // --- WebSocket Logic ---
    void onConnected();
    void onLoginSuccess(const User &user, const QString &token);
    void onAuthFailed(const QString &msg);
    void onChatHistoryReceived(const std::vector<Chat> &chats);
    void onMessageReceived(const Message &msg);
    void onUserListUpdated(const std::vector<User> &users);
    void onNewChatCreated(const Chat &chat);

    // --- New Feature Slots ---
    void onUserTyping(const QString &chatId, const QString &username);
    void onUserPresenceChanged(const QString &userId, bool online);
    void onMessageDeleted(const QString &chatId, const QString &messageId);
    
    // --- UI Interactions ---
    void onLoginBtnClicked();
    void onChatSelected(QListWidgetItem *item);
    void onContactSelected(QListWidgetItem *item);
    void onSendBtnClicked();
    void onLogoutClicked();
    void onDarkModeToggled(bool checked);
    void onInputTextChanged(const QString &text);
    void onMessageContextMenu(const QPoint &pos);

    // --- Network (PFP) ---
    void onAvatarDownloaded(QNetworkReply *reply);

private:
    WebSocketClient *m_client;
    QNetworkAccessManager *m_nam;

    // Data Models
    QMap<QString, Chat> m_chats;
    QMap<QString, User> m_users;
    QMap<QString, QIcon> m_avatarCache; // Stores loaded PFPs

    // State
    QString m_currentChatId;
    QString m_replyToId; // Stores the ID of the message being replied to
    bool m_isDarkMode = false;
    
    // Timers
    QTimer *m_typingTimer;      // Limits how often we send "I am typing"
    QTimer *m_typingClearTimer; // Clears the "X is typing..." label

    // UI Elements
    QStackedWidget *m_stackedWidget;
    
    // Login Page
    QWidget *m_loginPage;
    QLineEdit *m_usernameInput;
    QLineEdit *m_passwordInput;
    QLabel *m_statusLabel;
    QPushButton *m_loginBtn;

    // App Page
    QWidget *m_appPage;
    QTabWidget *m_sidebarTabs;
    QListWidget *m_chatListWidget;    // Left sidebar (Chats)
    QListWidget *m_contactListWidget; // Left sidebar (Contacts)
    QWidget *m_settingsTab;

    // Chat Area
    QWidget *m_chatAreaWidget;
    QLabel *m_chatTitle;
    QLabel *m_typingLabel;  // Shows "User is typing..."
    QListWidget *m_chatList; // The main message area (bubbles)
    
    // Reply & Input
    QWidget *m_replyContainer; 
    QLabel *m_replyLabel;
    QPushButton *m_cancelReplyBtn;

    QLineEdit *m_messageInput;
    QPushButton *m_sendBtn;

    // Helper Functions
    void setupUi();
    void applyTheme();
    void renderMessages(const QString &chatId);
    void addMessageBubble(const Message &msg);
    QString resolveChatName(const Chat &chat);
    
    // PFP Helpers
    void loadAvatar(const QString &url, const QString &targetId, bool isChat);
    QIcon getAvatar(const QString &id);

    void smoothScrollToBottom();
};

#endif // MAINWINDOW_H
