#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QStackedWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSettings>
#include <QTimer>
#include <QMap>
#include <QSet>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QScrollBar>

#include "WebSocketClient.h"
#include "DataStructures.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onConnected();
    void onLoginBtnClicked();
    void onLoginSuccess(const User &user, const QString &token);
    void onAuthFailed(const QString &msg);
    void onChatHistoryReceived(const std::vector<Chat> &chats);
    void onMessageReceived(const Message &msg);
    void onUserListUpdated(const std::vector<User> &users);
    void onSendBtnClicked();
    void onNewChatCreated(const Chat &chat);
    void onChatSelected(QListWidgetItem *item);
    void onContactSelected(QListWidgetItem *item);
    void onDarkModeToggled(bool checked);
    void onLogoutClicked();
    void onScrollValueChanged(int value);

private:
    void setupUi();
    void applyTheme();
    void renderMessages(const QString &chatId);
    void addMessageBubble(const Message &msg, bool appendStretch, bool animate);
    void prependMessageBubble(const Message &msg);
    QString resolveChatName(const Chat &chat);
    QColor getColorForName(const QString &name);

    // --- Avatar Handling ---
    QIcon getAvatar(const QString &name, const QString &url);
    QIcon generateGenericAvatar(const QString &name);
    void updateAvatarOnItems(const QString &url, const QPixmap &pixmap);
    
    // Cache Helpers
    QString getCachePath(const QString &url) const;
    void saveToCache(const QString &url, const QByteArray &data);
    void downloadAvatar(const QString &url);

    // Scroll Helpers
    void scrollToBottom();
    void smoothScrollToBottom();
    bool isScrolledToBottom() const;

    WebSocketClient *m_client;
    QNetworkAccessManager *m_nam;

    // UI Elements
    QStackedWidget *m_stackedWidget;
    QWidget *m_loginPage;
    QWidget *m_appPage;

    QLineEdit *m_usernameInput;
    QLineEdit *m_passwordInput;
    QPushButton *m_loginBtn;
    QLabel *m_statusLabel;

    QListWidget *m_chatListWidget;    // Sidebar Chats
    QListWidget *m_contactListWidget; // Sidebar Contacts
    QTabWidget *m_sidebarTabs;
    QWidget *m_settingsTab;

    QWidget *m_chatAreaWidget;
    QLabel *m_chatTitle;
    QListWidget *m_chatList;          // Message Area
    QLineEdit *m_messageInput;
    QPushButton *m_sendBtn;

    // Data
    QMap<QString, User> m_users;
    QMap<QString, Chat> m_chats;
    QString m_currentChatId;
    bool m_isDarkMode;
    bool m_isLoadingHistory = false;

    // Avatar Cache System
    QMap<QString, QPixmap> m_avatarCache; // Memory Cache
    QSet<QString> m_pendingDownloads;     // URLs currently downloading
    QSet<QString> m_checkedUrls;          // URLs checked for updates this session
};

#endif // MAINWINDOW_H
