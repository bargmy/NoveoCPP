#include "MainWindow.h"
#include "MessageBubble.h"
#include <QHBoxLayout>
#include <QDateTime>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QPixmap>
#include <QIcon>
#include <QSplitter>
#include <QScrollBar>
#include <QApplication>
#include <QSettings>
#include <algorithm>
#include <QPainter>
#include <QPropertyAnimation>
#include <QTimer>
#include <QGraphicsOpacityEffect>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_client(new WebSocketClient(this)), m_nam(new QNetworkAccessManager(this))
{
    // Load Settings FIRST
    QSettings settings("Noveo", "MessengerClient");
    m_isDarkMode = settings.value("darkMode", false).toBool();

    setupUi();
    applyTheme(); 

    // Pre-fill inputs if saved
    if(settings.contains("username")) {
        m_usernameInput->setText(settings.value("username").toString());
        m_passwordInput->setText(settings.value("password").toString());
    }

    connect(m_client, &WebSocketClient::connected, this, &MainWindow::onConnected);
    connect(m_client, &WebSocketClient::loginSuccess, this, &MainWindow::onLoginSuccess);
    connect(m_client, &WebSocketClient::authFailed, this, &MainWindow::onAuthFailed);
    connect(m_client, &WebSocketClient::chatHistoryReceived, this, &MainWindow::onChatHistoryReceived);
    connect(m_client, &WebSocketClient::messageReceived, this, &MainWindow::onMessageReceived);
    connect(m_client, &WebSocketClient::userListUpdated, this, &MainWindow::onUserListUpdated);
    connect(m_client, &WebSocketClient::newChatCreated, this, &MainWindow::onNewChatCreated);
    
    m_statusLabel->setText("Connecting to server...");
    m_client->connectToServer();
}

MainWindow::~MainWindow()
{
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
}

void MainWindow::setupUi()
{
    resize(1000, 700);
    setWindowTitle("Noveo Desktop");

    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);

    // --- Login Page ---
    m_loginPage = new QWidget();
    QVBoxLayout *loginLayout = new QVBoxLayout(m_loginPage);
    
    QLabel *title = new QLabel("Noveo");
    title->setStyleSheet("font-size: 32px; font-weight: bold; margin-bottom: 20px;");
    title->setAlignment(Qt::AlignCenter);
    
    m_usernameInput = new QLineEdit();
    m_usernameInput->setPlaceholderText("Username");
    
    m_passwordInput = new QLineEdit();
    m_passwordInput->setPlaceholderText("Password");
    m_passwordInput->setEchoMode(QLineEdit::Password);
    
    m_loginBtn = new QPushButton("Log In");
    m_loginBtn->setCursor(Qt::PointingHandCursor);
    m_loginBtn->setMinimumHeight(40);
    
    m_statusLabel = new QLabel("Waiting for connection...");
    m_statusLabel->setAlignment(Qt::AlignCenter);

    loginLayout->addStretch();
    loginLayout->addWidget(title);
    loginLayout->addWidget(m_usernameInput);
    loginLayout->addWidget(m_passwordInput);
    loginLayout->addWidget(m_loginBtn);
    loginLayout->addWidget(m_statusLabel);
    loginLayout->addStretch();
    loginLayout->setContentsMargins(300, 50, 300, 50);

    m_stackedWidget->addWidget(m_loginPage);

    // --- App Page ---
    m_appPage = new QWidget();
    QHBoxLayout *appLayout = new QHBoxLayout(m_appPage);
    appLayout->setContentsMargins(0,0,0,0);
    appLayout->setSpacing(0);

    // Sidebar
    m_sidebarTabs = new QTabWidget();
    m_sidebarTabs->setFixedWidth(300);
    m_sidebarTabs->setTabPosition(QTabWidget::South); 
    
    m_chatListWidget = new QListWidget();
    m_chatListWidget->setIconSize(QSize(42, 42));
    
    m_contactListWidget = new QListWidget();
    m_contactListWidget->setIconSize(QSize(42, 42));
    
    // Settings Tab
    m_settingsTab = new QWidget();
    QVBoxLayout *settingsLayout = new QVBoxLayout(m_settingsTab);
    
    QCheckBox *darkModeCheck = new QCheckBox("Dark Mode");
    darkModeCheck->setChecked(m_isDarkMode); // Set correct state from start
    connect(darkModeCheck, &QCheckBox::toggled, this, &MainWindow::onDarkModeToggled);
    
    QPushButton *logoutBtn = new QPushButton("Log Out");
    connect(logoutBtn, &QPushButton::clicked, this, &MainWindow::onLogoutClicked);
    
    settingsLayout->addWidget(new QLabel("<h2>Settings</h2>"));
    settingsLayout->addWidget(darkModeCheck);
    settingsLayout->addStretch();
    settingsLayout->addWidget(logoutBtn);
    settingsLayout->addSpacing(20);

    m_sidebarTabs->addTab(m_chatListWidget, "Chats");
    m_sidebarTabs->addTab(m_contactListWidget, "Contacts");
    m_sidebarTabs->addTab(m_settingsTab, "Settings");

    // Chat Area
    m_chatAreaWidget = new QWidget();
    QVBoxLayout *chatAreaLayout = new QVBoxLayout(m_chatAreaWidget);
    chatAreaLayout->setContentsMargins(0,0,0,0);
    chatAreaLayout->setSpacing(0);
    
    // Header
    QWidget *header = new QWidget();
    header->setObjectName("chatHeader");
    header->setFixedHeight(60);
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    m_chatTitle = new QLabel("Select a chat");
    m_chatTitle->setStyleSheet("font-size: 16px; font-weight: bold; margin-left: 10px;");
    headerLayout->addWidget(m_chatTitle);
    
    // Messages (Scroll Area)
    m_messageScrollArea = new QScrollArea();
    m_messageScrollArea->setWidgetResizable(true);
    m_messageScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_messageScrollArea->setObjectName("messageArea");
    
    m_messageContainer = new QWidget();
    m_messageContainer->setObjectName("messageContainer");
    m_messageLayout = new QVBoxLayout(m_messageContainer);
    m_messageLayout->addStretch();
    m_messageScrollArea->setWidget(m_messageContainer);
    
    // Input Area
    QWidget *inputArea = new QWidget();
    inputArea->setObjectName("inputArea");
    inputArea->setFixedHeight(60);
    QHBoxLayout *inputLayout = new QHBoxLayout(inputArea);
    
    m_messageInput = new QLineEdit();
    m_messageInput->setPlaceholderText("Write a message...");
    
    m_sendBtn = new QPushButton("Send");
    m_sendBtn->setCursor(Qt::PointingHandCursor);

    inputLayout->addWidget(m_messageInput);
    inputLayout->addWidget(m_sendBtn);

    chatAreaLayout->addWidget(header);
    chatAreaLayout->addWidget(m_messageScrollArea);
    chatAreaLayout->addWidget(inputArea);

    appLayout->addWidget(m_sidebarTabs);
    appLayout->addWidget(m_chatAreaWidget);

    m_stackedWidget->addWidget(m_appPage);

    // Signals
    connect(m_loginBtn, &QPushButton::clicked, this, &MainWindow::onLoginBtnClicked);
    connect(m_chatListWidget, &QListWidget::itemClicked, this, &MainWindow::onChatSelected);
    connect(m_contactListWidget, &QListWidget::itemClicked, this, &MainWindow::onContactSelected);
    connect(m_sendBtn, &QPushButton::clicked, this, &MainWindow::onSendBtnClicked);
    connect(m_messageInput, &QLineEdit::returnPressed, this, &MainWindow::onSendBtnClicked);
}

void MainWindow::applyTheme()
{
    QString bg = m_isDarkMode ? "#1e1e1e" : "#f5f5f5";
    QString panelBg = m_isDarkMode ? "#2d2d2d" : "#ffffff";
    QString text = m_isDarkMode ? "#ffffff" : "#000000";
    QString border = m_isDarkMode ? "#3d3d3d" : "#e0e0e0";
    QString inputBg = m_isDarkMode ? "#3d3d3d" : "#ffffff";
    QString listHover = m_isDarkMode ? "#3a3a3a" : "#f0f0f0";

    QString style = QString(
        "QMainWindow { background-color: %1; }"
        "QWidget { color: %3; }"
        "QLineEdit { padding: 10px; border: 1px solid %4; border-radius: 5px; background-color: %5; color: %3; }"
        "QPushButton { padding: 8px 15px; border-radius: 5px; background-color: #0088cc; color: white; border: none; }"
        "QPushButton:hover { background-color: #0077b3; }"
        "QListWidget { background-color: %2; border: none; outline: none; }"
        "QListWidget::item { padding: 10px; border-bottom: 1px solid %4; }"
        "QListWidget::item:selected { background-color: #0088cc; color: white; }"
        "QListWidget::item:hover { background-color: %6; }"
        "QTabWidget::pane { border: none; }"
        "QTabBar::tab { background: %2; color: %3; padding: 10px; min-width: 80px; }"
        "QTabBar::tab:selected { border-bottom: 2px solid #0088cc; }"
        "#chatHeader { background-color: %2; border-bottom: 1px solid %4; }"
        "#inputArea { background-color: %2; border-top: 1px solid %4; }"
        "#messageArea { background-color: %1; border: none; }"
        "#messageContainer { background-color: transparent; }"
    ).arg(bg, panelBg, text, border, inputBg, listHover);

    setStyleSheet(style);
}

void MainWindow::onDarkModeToggled(bool checked)
{
    m_isDarkMode = checked;
    
    QSettings settings("Noveo", "MessengerClient");
    settings.setValue("darkMode", m_isDarkMode);
    
    applyTheme();
    
    if (!m_currentChatId.isEmpty()) {
        renderMessages(m_currentChatId);
    }
}

void MainWindow::onLogoutClicked()
{
    QSettings settings("Noveo", "MessengerClient");
    settings.remove("username");
    settings.remove("password");
    // settings.remove("darkMode"); // Optional: keep theme or reset? Usually keep.
    
    m_usernameInput->clear();
    m_passwordInput->clear();
    m_stackedWidget->setCurrentWidget(m_loginPage);
}

void MainWindow::onConnected()
{
    m_statusLabel->setText("Connected.");
    m_loginBtn->setEnabled(true);

    // AUTO LOGIN HERE - Ensures socket is ready
    QSettings settings("Noveo", "MessengerClient");
    if(settings.contains("username") && settings.contains("password")) {
         m_statusLabel->setText("Auto-logging in...");
         onLoginBtnClicked();
    }
}

void MainWindow::onLoginBtnClicked()
{
    QString user = m_usernameInput->text().trimmed();
    QString pass = m_passwordInput->text();
    if (user.isEmpty() || pass.isEmpty()) return;
    
    m_statusLabel->setText("Logging in...");
    m_client->login(user, pass);
}

void MainWindow::onLoginSuccess(const User &user, const QString &token)
{
    QSettings settings("Noveo", "MessengerClient");
    settings.setValue("username", m_usernameInput->text());
    settings.setValue("password", m_passwordInput->text());

    m_stackedWidget->setCurrentWidget(m_appPage);
}

void MainWindow::onAuthFailed(const QString &msg)
{
    m_statusLabel->setText("Login Failed: " + msg);
}

void MainWindow::onUserListUpdated(const std::vector<User> &users)
{
    m_users.clear();
    m_contactListWidget->clear();
    
    for(const auto &u : users) {
        m_users.insert(u.userId, u);
        
        if(u.userId == m_client->currentUserId()) continue;
        
        QListWidgetItem *item = new QListWidgetItem(m_contactListWidget);
        item->setText(u.username);
        item->setData(Qt::UserRole, u.userId);
        item->setIcon(getAvatar(u.username, u.avatarUrl));
    }
    
    for(int i=0; i<m_chatListWidget->count(); i++) {
        QListWidgetItem *item = m_chatListWidget->item(i);
        QString chatId = item->data(Qt::UserRole).toString();
        if(m_chats.contains(chatId)) {
            QString name = resolveChatName(m_chats[chatId]);
            item->setText(name);
            item->setIcon(getAvatar(name, m_chats[chatId].avatarUrl));
        }
    }
}

void MainWindow::onChatHistoryReceived(const std::vector<Chat> &chats)
{
    m_chats.clear();
    m_chatListWidget->clear();
    
    std::vector<Chat> sortedChats = chats;
    std::sort(sortedChats.begin(), sortedChats.end(), [](const Chat &a, const Chat &b) {
        qint64 timeA = a.messages.empty() ? 0 : a.messages.back().timestamp;
        qint64 timeB = b.messages.empty() ? 0 : b.messages.back().timestamp;
        return timeA > timeB;
    });
    
    for (const auto &chat : sortedChats) {
        m_chats.insert(chat.chatId, chat);
        
        QListWidgetItem *item = new QListWidgetItem(m_chatListWidget);
        QString name = resolveChatName(chat);
        item->setText(name);
        item->setData(Qt::UserRole, chat.chatId);
        item->setIcon(getAvatar(name, chat.avatarUrl));
    }
}

void MainWindow::onNewChatCreated(const Chat &chat)
{
    if(!m_chats.contains(chat.chatId)) {
        m_chats.insert(chat.chatId, chat);
        QListWidgetItem *item = new QListWidgetItem(m_chatListWidget);
        QString name = resolveChatName(chat);
        item->setText(name);
        item->setData(Qt::UserRole, chat.chatId);
        item->setIcon(getAvatar(name, chat.avatarUrl));
        m_chatListWidget->insertItem(0, item);
    }
}

QString MainWindow::resolveChatName(const Chat &chat)
{
    if (!chat.chatName.isEmpty()) return chat.chatName;
    
    if (chat.chatType == "private") {
        for (const auto &memberId : chat.members) {
            if (memberId != m_client->currentUserId()) {
                if (m_users.contains(memberId)) {
                    return m_users[memberId].username;
                }
            }
        }
        return "Unknown User";
    }
    return "Chat";
}

QColor MainWindow::getColorForName(const QString &name)
{
    unsigned int hash = 0;
    QByteArray bytes = name.toUtf8();
    for (char c : bytes) hash = c + (hash << 6) + (hash << 16) - hash;
    
    static const QList<QColor> colors = {
        QColor("#e57373"), QColor("#f06292"), QColor("#ba68c8"), QColor("#9575cd"),
        QColor("#7986cb"), QColor("#64b5f6"), QColor("#4fc3f7"), QColor("#4dd0e1"),
        QColor("#4db6ac"), QColor("#81c784"), QColor("#aed581"), QColor("#ff8a65"),
        QColor("#d4e157"), QColor("#ffd54f"), QColor("#ffb74d"), QColor("#a1887f")
    };
    return colors[hash % colors.size()];
}

QIcon MainWindow::getAvatar(const QString &name, const QString &url)
{
    QPixmap pixmap(42, 42);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    painter.setBrush(getColorForName(name));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, 42, 42);
    
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPixelSize(20);
    font.setBold(true);
    painter.setFont(font);
    QString letter = name.isEmpty() ? "?" : name.left(1).toUpper();
    painter.drawText(QRect(0, 0, 42, 42), Qt::AlignCenter, letter);
    
    return QIcon(pixmap);
}

bool MainWindow::isScrolledToBottom() const
{
    QScrollBar *sb = m_messageScrollArea->verticalScrollBar();
    return sb->value() >= (sb->maximum() - 20); 
}

void MainWindow::scrollToBottom()
{
    QTimer::singleShot(10, this, [this]() {
        QScrollBar *sb = m_messageScrollArea->verticalScrollBar();
        sb->setValue(sb->maximum());
    });
}

void MainWindow::smoothScrollToBottom()
{
    QTimer::singleShot(10, this, [this]() {
        QScrollBar *sb = m_messageScrollArea->verticalScrollBar();
        sb->setValue(sb->maximum());
    });
}

void MainWindow::onChatSelected(QListWidgetItem *item)
{
    QString chatId = item->data(Qt::UserRole).toString();
    m_currentChatId = chatId;
    
    if (m_chats.contains(chatId)) {
        Chat chat = m_chats[chatId];
        m_chatTitle->setText(resolveChatName(chat));
        renderMessages(chatId);
    }
}

void MainWindow::onContactSelected(QListWidgetItem *item)
{
    QString userId = item->data(Qt::UserRole).toString();
    QString selfId = m_client->currentUserId();
    
    QStringList ids = {userId, selfId};
    ids.sort();
    QString potentialChatId = ids.join("_");
    
    if(m_chats.contains(potentialChatId)) {
        m_currentChatId = potentialChatId;
        m_chatTitle->setText(resolveChatName(m_chats[potentialChatId]));
        renderMessages(potentialChatId);
    } else {
        m_currentChatId = potentialChatId;
        m_chatTitle->setText(item->text());
        
        QLayoutItem *child;
        while ((child = m_messageLayout->takeAt(0)) != nullptr) {
            if(child->widget()) delete child->widget();
            delete child;
        }
        m_messageLayout->addStretch();
    }
}

void MainWindow::renderMessages(const QString &chatId)
{
    QLayoutItem *child;
    while ((child = m_messageLayout->takeAt(0)) != nullptr) {
        if(child->widget()) delete child->widget();
        delete child;
    }
    
    QLabel *loadingLabel = new QLabel("Loading chat...");
    loadingLabel->setStyleSheet("color: gray; font-size: 12px; margin: 20px;");
    loadingLabel->setAlignment(Qt::AlignCenter);
    m_messageLayout->addWidget(loadingLabel);
    QApplication::processEvents();

    if (m_chats.contains(chatId)) {
        const auto &msgs = m_chats[chatId].messages;
        int count = 0;
        
        for (const auto &msg : msgs) {
            addMessageBubble(msg, false, false);
            count++;
            if (count % 20 == 0) QApplication::processEvents();
        }
    }
    
    loadingLabel->hide();
    delete loadingLabel;
    m_messageLayout->addStretch();
    
    m_messageContainer->adjustSize(); 
    QApplication::processEvents();
    scrollToBottom();
}

void MainWindow::addMessageBubble(const Message &msg, bool appendStretch, bool animate)
{
    if (appendStretch && m_messageLayout->count() > 0) {
        QLayoutItem *item = m_messageLayout->takeAt(m_messageLayout->count() - 1);
        delete item;
    }

    bool isMe = (msg.senderId == m_client->currentUserId());
    QString senderName = m_users.contains(msg.senderId) ? m_users[msg.senderId].username : "Unknown";
    
    MessageBubble *bubble = new MessageBubble(msg.text, senderName, msg.timestamp, isMe);
    
    m_messageLayout->addWidget(bubble);
    
    if (animate) {
        QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(bubble);
        bubble->setGraphicsEffect(eff);
        QPropertyAnimation *a = new QPropertyAnimation(eff, "opacity");
        a->setDuration(300);
        a->setStartValue(0);
        a->setEndValue(1);
        a->start(QAbstractAnimation::DeleteWhenStopped);
    }
    
    if (appendStretch) {
        m_messageLayout->addStretch();
    }
}

void MainWindow::onMessageReceived(const Message &msg)
{
    if (m_chats.contains(msg.chatId)) {
        m_chats[msg.chatId].messages.push_back(msg);
        
        for(int i=0; i<m_chatListWidget->count(); i++) {
            if (m_chatListWidget->item(i)->data(Qt::UserRole).toString() == msg.chatId) {
                QListWidgetItem *item = m_chatListWidget->takeItem(i);
                m_chatListWidget->insertItem(0, item);
                break;
            }
        }
        
        if (m_currentChatId == msg.chatId) {
            bool wasAtBottom = isScrolledToBottom();
            bool isMyMessage = (msg.senderId == m_client->currentUserId());

            addMessageBubble(msg, true, true);
            
            if (wasAtBottom || isMyMessage) {
                QApplication::processEvents();
                smoothScrollToBottom();
            }
        }
    }
}

void MainWindow::onSendBtnClicked()
{
    QString text = m_messageInput->text().trimmed();
    if (text.isEmpty() || m_currentChatId.isEmpty()) return;
    
    m_client->sendMessage(m_currentChatId, text);
    m_messageInput->clear();
}
