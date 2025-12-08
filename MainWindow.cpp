#include "MainWindow.h"
// MessageBubble.h is no longer needed for rendering, but we keep the file if you want
// We now use a Delegate for chat messages.

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QStackedWidget>
#include <QTabWidget>
#include <QSettings>
#include <QTimer>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QRegularExpression>
#include <QCheckBox>
#include <QApplication>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QDateTime>

// ==========================================
// 1. CONTACT LIST DELEGATE (Sidebar)
// ==========================================
class UserListDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        painter->save();
        
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        option.widget->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, option.widget);

        QString fullText = index.data(Qt::DisplayRole).toString();
        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        
        QRect rect = opt.rect;
        const int padding = 10;
        const int iconSize = 40;

        if (!icon.isNull()) {
            int iconY = rect.top() + (rect.height() - iconSize) / 2;
            icon.paint(painter, rect.left() + padding, iconY, iconSize, iconSize);
        }

        // Regex for Nametag
        static QRegularExpression regex("^(.*)\\s\\[#([a-fA-F0-9]{6}),\\s*\"(.*)\"\\]$");
        QRegularExpressionMatch match = regex.match(fullText);

        QString name = fullText;
        QString tagText;
        QColor tagColor;
        bool hasTag = false;

        if (match.hasMatch()) {
            name = match.captured(1).trimmed();
            tagColor = QColor("#" + match.captured(2));
            tagText = match.captured(3);
            hasTag = true;
        }

        // Draw Name
        QColor textColor = (opt.state & QStyle::State_Selected) ? Qt::white : opt.palette.text().color();
        painter->setPen(textColor);
        QFont nameFont = opt.font;
        nameFont.setPixelSize(14);
        painter->setFont(nameFont);

        int textX = rect.left() + padding + iconSize + padding;
        QFontMetrics fm(nameFont);
        int nameWidth = fm.horizontalAdvance(name);
        
        // Vertically center text
        int textY = rect.top() + (rect.height() - fm.height()) / 2 + fm.ascent();
        painter->drawText(textX, textY, name);

        // Draw Tag
        if (hasTag) {
            int tagX = textX + nameWidth + 8;
            int tagH = 18;
            int tagY = rect.center().y() - tagH / 2;
            
            QFont tagFont = nameFont;
            tagFont.setPixelSize(10);
            tagFont.setBold(true);
            QFontMetrics tagFm(tagFont);
            int tagW = tagFm.horizontalAdvance(tagText) + 12;

            painter->setBrush(tagColor);
            painter->setPen(Qt::NoPen);
            painter->drawRoundedRect(tagX, tagY, tagW, tagH, 4, 4);

            painter->setPen(Qt::white); 
            painter->setFont(tagFont);
            painter->drawText(QRect(tagX, tagY, tagW, tagH), Qt::AlignCenter, tagText);
        }

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        Q_UNUSED(index);
        return QSize(option.rect.width(), 60); 
    }
};

// ==========================================
// 2. MESSAGE DELEGATE (Chat Area) - The Fix for Lag
// ==========================================
class MessageDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    // Helper to calculate bubble geometry
    void getBubbleLayout(const QString &text, const QString &sender, bool isMe, int viewWidth, 
                         QRect &bubbleRect, QRect &textRect, QRect &nameRect, int &neededHeight) const {
        
        int maxBubbleWidth = viewWidth * 0.70; // Max 70% of screen width
        int padding = 10;
        int nameHeight = isMe ? 0 : 15; // Space for name if not me
        int timeHeight = 12;
        int bubblePadding = 16; // 8px top + 8px bottom

        // Calculate Text Height using QTextDocument (handles wrapping)
        QTextDocument doc;
        doc.setDefaultFont(QFont("Segoe UI", 10)); // Adjust font size here
        doc.setPlainText(text);
        doc.setTextWidth(maxBubbleWidth - 20); // Inner padding
        
        int textWidth = doc.idealWidth();
        int textHeight = doc.size().height();

        // Calculate total bubble dimensions
        int bubbleW = qMax(textWidth + 20, 100); // Min width
        int bubbleH = textHeight + nameHeight + timeHeight + bubblePadding;

        neededHeight = bubbleH + 10; // +10 for external margin

        // Position
        int x = isMe ? (viewWidth - bubbleW - 20) : 10; // Right or Left aligned
        bubbleRect = QRect(x, 5, bubbleW, bubbleH);
        
        if (!isMe) {
            nameRect = QRect(x + 10, 5 + 5, bubbleW - 20, 15);
            textRect = QRect(x + 10, 5 + 20, bubbleW - 20, textHeight);
        } else {
            textRect = QRect(x + 10, 5 + 8, bubbleW - 20, textHeight);
        }
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        // Retrieve data
        QString text = index.data(Qt::UserRole + 1).toString();
        QString sender = index.data(Qt::UserRole + 2).toString();
        qint64 timestamp = index.data(Qt::UserRole + 3).toLongLong();
        bool isMe = index.data(Qt::UserRole + 4).toBool();

        QRect bubbleRect, textRect, nameRect;
        int neededHeight;
        getBubbleLayout(text, sender, isMe, option.rect.width(), bubbleRect, textRect, nameRect, neededHeight);

        // Adjust rect to current item position
        bubbleRect.translate(0, option.rect.top());
        textRect.translate(0, option.rect.top());
        nameRect.translate(0, option.rect.top());

        // Draw Bubble Background
        QColor bubbleColor = isMe ? QColor("#EEFFDE") : Qt::white;
        painter->setBrush(bubbleColor);
        painter->setPen(QPen(QColor("#d0d0d0"), 1));
        painter->drawRoundedRect(bubbleRect, 10, 10);

        // Draw Name (if not me)
        if (!isMe) {
            // Regex for Nametag in Chat
            static QRegularExpression regex("^(.*)\\s\\[#([a-fA-F0-9]{6}),\\s*\"(.*)\"\\]$");
            QRegularExpressionMatch match = regex.match(sender);
            
            QString displayName = sender;
            QString tagText;
            QColor tagColor;
            bool hasTag = false;

            if (match.hasMatch()) {
                displayName = match.captured(1).trimmed();
                tagColor = QColor("#" + match.captured(2));
                tagText = match.captured(3);
                hasTag = true;
            }

            // Draw Display Name
            painter->setPen(QColor("#E35967"));
            QFont nameFont = option.font;
            nameFont.setPixelSize(11);
            nameFont.setBold(true);
            painter->setFont(nameFont);
            
            QFontMetrics fm(nameFont);
            int nameW = fm.horizontalAdvance(displayName);
            painter->drawText(nameRect.left(), nameRect.top() + fm.ascent(), displayName);

            // Draw Tag
            if (hasTag) {
                int tagX = nameRect.left() + nameW + 5;
                int tagY = nameRect.top(); // Align top
                int tagW = fm.horizontalAdvance(tagText) + 10;
                int tagH = 14;

                painter->setBrush(tagColor);
                painter->setPen(Qt::NoPen);
                painter->drawRoundedRect(tagX, tagY, tagW, tagH, 3, 3);

                painter->setPen(Qt::white);
                QFont tagFont = nameFont;
                tagFont.setPixelSize(9);
                painter->setFont(tagFont);
                painter->drawText(QRect(tagX, tagY, tagW, tagH), Qt::AlignCenter, tagText);
            }
        }

        // Draw Message Text
        painter->setPen(Qt::black);
        QTextDocument doc;
        QFont textFont("Segoe UI", 10); // Standard font size
        doc.setDefaultFont(textFont);
        doc.setPlainText(text);
        doc.setTextWidth(textRect.width());
        
        painter->translate(textRect.topLeft());
        doc.drawContents(painter);
        painter->translate(-textRect.topLeft());

        // Draw Timestamp
        QDateTime dt;
        dt.setSecsSinceEpoch(timestamp);
        QString timeStr = dt.toString("hh:mm AP");
        
        painter->setPen(QColor("#888888"));
        QFont timeFont = option.font;
        timeFont.setPixelSize(9);
        painter->setFont(timeFont);
        
        // Bottom right of bubble
        painter->drawText(bubbleRect.adjusted(0,0,-8,-5), Qt::AlignBottom | Qt::AlignRight, timeStr);

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QString text = index.data(Qt::UserRole + 1).toString();
        QString sender = index.data(Qt::UserRole + 2).toString();
        bool isMe = index.data(Qt::UserRole + 4).toBool();

        QRect b, t, n;
        int h;
        getBubbleLayout(text, sender, isMe, option.rect.width(), b, t, n, h);
        return QSize(option.rect.width(), h);
    }
};

// ==========================================
// 3. MAIN WINDOW IMPL
// ==========================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_client(new WebSocketClient(this)), m_nam(new QNetworkAccessManager(this))
{
    QSettings settings("Noveo", "MessengerClient");
    m_isDarkMode = settings.value("darkMode", false).toBool();

    setupUi();
    applyTheme();

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

MainWindow::~MainWindow() {}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    // Force relayout of chat bubbles on resize
    // m_chatList->scheduleDelayedItemsLayout(); 
}

void MainWindow::setupUi() {
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
    m_chatListWidget->setItemDelegate(new UserListDelegate(this));
    m_chatListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_contactListWidget = new QListWidget();
    m_contactListWidget->setIconSize(QSize(42, 42));
    m_contactListWidget->setItemDelegate(new UserListDelegate(this));
    m_contactListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Settings
    m_settingsTab = new QWidget();
    QVBoxLayout *settingsLayout = new QVBoxLayout(m_settingsTab);
    QCheckBox *darkModeCheck = new QCheckBox("Dark Mode");
    darkModeCheck->setChecked(m_isDarkMode);
    connect(darkModeCheck, &QCheckBox::toggled, this, &MainWindow::onDarkModeToggled);
    QPushButton *logoutBtn = new QPushButton("Log Out");
    connect(logoutBtn, &QPushButton::clicked, this, &MainWindow::onLogoutClicked);
    settingsLayout->addWidget(new QLabel("Settings"));
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

    // --- REPLACED SCROLL AREA WITH QLISTWIDGET (For Virtualization) ---
    m_chatList = new QListWidget();
    m_chatList->setObjectName("chatList");
    m_chatList->setFrameShape(QFrame::NoFrame);
    m_chatList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel); // Smooth scroll
    m_chatList->setUniformItemSizes(false); // Variable bubble heights
    m_chatList->setResizeMode(QListView::Adjust); // Recalculate bubbles on resize
    m_chatList->setSelectionMode(QAbstractItemView::NoSelection); // No highlight on click
    m_chatList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // Requested feature
    m_chatList->setItemDelegate(new MessageDelegate(this));

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
    chatAreaLayout->addWidget(m_chatList); // Add List instead of ScrollArea
    chatAreaLayout->addWidget(inputArea);

    appLayout->addWidget(m_sidebarTabs);
    appLayout->addWidget(m_chatAreaWidget);
    m_stackedWidget->addWidget(m_appPage);

    connect(m_loginBtn, &QPushButton::clicked, this, &MainWindow::onLoginBtnClicked);
    connect(m_chatListWidget, &QListWidget::itemClicked, this, &MainWindow::onChatSelected);
    connect(m_contactListWidget, &QListWidget::itemClicked, this, &MainWindow::onContactSelected);
    connect(m_sendBtn, &QPushButton::clicked, this, &MainWindow::onSendBtnClicked);
    connect(m_messageInput, &QLineEdit::returnPressed, this, &MainWindow::onSendBtnClicked);
}

void MainWindow::applyTheme() {
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
        "#chatList { background-color: %1; border: none; }" // Styled List
    ).arg(bg, panelBg, text, border, inputBg, listHover);

    setStyleSheet(style);
}

void MainWindow::onDarkModeToggled(bool checked) {
    m_isDarkMode = checked;
    QSettings settings("Noveo", "MessengerClient");
    settings.setValue("darkMode", m_isDarkMode);
    applyTheme();
    if (!m_currentChatId.isEmpty()) {
        renderMessages(m_currentChatId);
    }
}

void MainWindow::onLogoutClicked() {
    QSettings settings("Noveo", "MessengerClient");
    settings.remove("username");
    settings.remove("password");
    m_usernameInput->clear();
    m_passwordInput->clear();
    m_stackedWidget->setCurrentWidget(m_loginPage);
}

void MainWindow::onConnected() {
    m_statusLabel->setText("Connected.");
    m_loginBtn->setEnabled(true);
    QSettings settings("Noveo", "MessengerClient");
    if(settings.contains("username") && settings.contains("password")) {
        m_statusLabel->setText("Auto-logging in...");
        onLoginBtnClicked();
    }
}

void MainWindow::onLoginBtnClicked() {
    QString user = m_usernameInput->text().trimmed();
    QString pass = m_passwordInput->text();
    if (user.isEmpty() || pass.isEmpty()) return;
    m_statusLabel->setText("Logging in...");
    m_client->login(user, pass);
}

void MainWindow::onLoginSuccess(const User &user, const QString &token) {
    QSettings settings("Noveo", "MessengerClient");
    settings.setValue("username", m_usernameInput->text());
    settings.setValue("password", m_passwordInput->text());
    m_stackedWidget->setCurrentWidget(m_appPage);
}

void MainWindow::onAuthFailed(const QString &msg) {
    m_statusLabel->setText("Login Failed: " + msg);
}

void MainWindow::onUserListUpdated(const std::vector<User> &users) {
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
    // Refresh chat names
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

void MainWindow::onChatHistoryReceived(const std::vector<Chat> &chats) {
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
        m_chatListWidget->addItem(item);
    }
}

void MainWindow::onNewChatCreated(const Chat &chat) {
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

QString MainWindow::resolveChatName(const Chat &chat) {
    if (!chat.chatName.isEmpty()) return chat.chatName;
    if (chat.chatType == "private") {
        for (const auto &memberId : chat.members) {
            if (memberId != m_client->currentUserId()) {
                if (m_users.contains(memberId)) return m_users[memberId].username;
            }
        }
        return "Unknown User";
    }
    return "Chat";
}

QColor MainWindow::getColorForName(const QString &name) {
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

QIcon MainWindow::getAvatar(const QString &name, const QString &url) {
    Q_UNUSED(url);
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

void MainWindow::scrollToBottom() {
    if (m_chatList->count() > 0)
        m_chatList->scrollToBottom();
}

void MainWindow::smoothScrollToBottom() {
    if (m_chatList->count() > 0)
        m_chatList->scrollToBottom();
}

bool MainWindow::isScrolledToBottom() const {
    QScrollBar *sb = m_chatList->verticalScrollBar();
    return sb->value() >= (sb->maximum() - 50);
}

void MainWindow::onChatSelected(QListWidgetItem *item) {
    QString chatId = item->data(Qt::UserRole).toString();
    m_currentChatId = chatId;
    if (m_chats.contains(chatId)) {
        Chat chat = m_chats[chatId];
        m_chatTitle->setText(resolveChatName(chat));
        renderMessages(chatId);
    }
}

void MainWindow::onContactSelected(QListWidgetItem *item) {
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
        m_chatList->clear();
    }
}

void MainWindow::renderMessages(const QString &chatId) {
    m_chatList->clear();

    if (m_chats.contains(chatId)) {
        const auto &msgs = m_chats[chatId].messages;
        
        // With QListWidget + Delegate, we can render thousands of items smoothly.
        // No need to limit to 50 anymore. The viewport handles it.
        for (const auto &msg : msgs) {
            addMessageBubble(msg, false, false);
        }
    }
    scrollToBottom();
}

void MainWindow::addMessageBubble(const Message &msg, bool appendStretch, bool animate) {
    Q_UNUSED(appendStretch); // Not needed for QListWidget
    Q_UNUSED(animate);       // Animations in ListWidget are harder, skipping for performance

    bool isMe = (msg.senderId == m_client->currentUserId());
    QString senderName = m_users.contains(msg.senderId) ? m_users[msg.senderId].username : "Unknown";

    QListWidgetItem *item = new QListWidgetItem(m_chatList);
    item->setData(Qt::UserRole + 1, msg.text);
    item->setData(Qt::UserRole + 2, senderName);
    item->setData(Qt::UserRole + 3, msg.timestamp);
    item->setData(Qt::UserRole + 4, isMe);
    
    // Size hint is calculated by Delegate
}

void MainWindow::onMessageReceived(const Message &msg) {
    if (m_chats.contains(msg.chatId)) {
        m_chats[msg.chatId].messages.push_back(msg);
        for(int i=0; i<m_chatListWidget->count(); i++) {
            if (m_chatListWidget->item(i)->data(Qt::UserRole).toString() == msg.chatId) {
                QListWidgetItem *item = m_chatListWidget->takeItem(i);
                m_chatListWidget->insertItem(0, item);
                break;
            }
        }
    }

    if (m_currentChatId == msg.chatId) {
        bool wasAtBottom = isScrolledToBottom();
        addMessageBubble(msg, false, false);
        if (wasAtBottom || msg.senderId == m_client->currentUserId()) {
            smoothScrollToBottom();
        }
    }
}

void MainWindow::onSendBtnClicked() {
    QString text = m_messageInput->text().trimmed();
    if (text.isEmpty() || m_currentChatId.isEmpty()) return;
    m_client->sendMessage(m_currentChatId, text);
    m_messageInput->clear();
}
