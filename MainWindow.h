#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>
#include <QMap>
#include <QSet>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "WebSocketClient.h"
#include "DataStructures.h"


class UserListDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    ~UserListDelegate() override = default;

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
};

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
    
    // NEW: Slot for scroll detection
    void onScrollValueChanged(int value);

private:
    void setupUi();
    void applyTheme();
    void renderMessages(const QString &chatId);
    void addMessageBubble(const Message &msg, bool appendStretch, bool animate);
    
    // NEW: Prepend function for older messages
    void prependMessageBubble(const Message &msg);
    
    QString resolveChatName(const Chat &chat);
    QColor getColorForName(const QString &name);
    
    // Avatar handling
    QIcon getAvatar(const QString &name, const QString &url);
    QIcon generateGenericAvatar(const QString &name);
    void updateAvatarOnItems(const QString &url, const QPixmap &pixmap);

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

    QListWidget *m_chatListWidget;
    QListWidget *m_contactListWidget;
    QTabWidget *m_sidebarTabs;
    QWidget *m_settingsTab;
    QWidget *m_chatAreaWidget;
    QLabel *m_chatTitle;
    QListWidget *m_chatList; // The main chat area
    QLineEdit *m_messageInput;
    QPushButton *m_sendBtn;

    // Data
    QMap<QString, User> m_users;
    QMap<QString, Chat> m_chats;
    QString m_currentChatId;
    bool m_isDarkMode;
    
    // NEW: Flag to prevent spamming history requests
    bool m_isLoadingHistory = false;

    // Avatar Cache
    QMap<QString, QPixmap> m_avatarCache;
    QSet<QString> m_pendingDownloads;
};

#endif // MAINWINDOW_H
