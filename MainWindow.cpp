#include "MainWindow.h"
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
#include <QScrollBar> 
#include <QSet>
#include <QNetworkRequest>
#include <QPainterPath>
#include <QStandardPaths>
#include <QDir>
#include <QCryptographicHash>
#include <QUuid>
#include <QMenu>
#include <QDebug>
#include <algorithm>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QAction>
#include <QCloseEvent>

const int AvatarUrlRole = Qt::UserRole + 10;
const QString API_BASE_URL = "https://api.pcpapc172.ir:8443";

void UserListDelegate::paint(QPainter* painter,
                             const QStyleOptionViewItem& option,
                             const QModelIndex& index) const
{
    painter->save();
    
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    
    option.widget->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, option.widget);
    
    QString fullText = index.data(Qt::DisplayRole).toString();
    
    // FIX: Added <QIcon> to value() call
    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    
    QRect rect = opt.rect;
    const int padding = 10;
    const int iconSize = 40;
    
    if (!icon.isNull()) {
        int iconY = rect.top() + (rect.height() - iconSize) / 2;
        icon.paint(painter, rect.left() + padding, iconY, iconSize, iconSize);
    }
    
    static QRegularExpression regex("^(.*)\\s\\[#([a-fA-F0-9]{6}),\\s*\\\"(.*)\\\"\\]$");
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
    
    QColor textColor = (opt.state & QStyle::State_Selected) ? Qt::white : opt.palette.text().color();
    painter->setPen(textColor);
    
    QFont nameFont = opt.font;
    nameFont.setPixelSize(14);
    painter->setFont(nameFont);
    
    int textX = rect.left() + padding + iconSize + padding;
    QFontMetrics fm(nameFont);
    int nameWidth = fm.horizontalAdvance(name);
    int textY = rect.top() + (rect.height() - fm.height()) / 2 + fm.ascent();
    
    painter->drawText(textX, textY, name);
    
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


QSize UserListDelegate::sizeHint(const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QSize(option.rect.width(), 60);
}

class MessageDelegate : public QStyledItemDelegate {
    bool m_isDarkMode = false;
    MainWindow* m_mainWindow = nullptr;
    QString m_highlightedMessageId;

public:
    explicit MessageDelegate(MainWindow* mainWindow, QObject* parent = nullptr)
        : QStyledItemDelegate(parent), m_mainWindow(mainWindow) {}

    void setTheme(bool isDarkMode) {
        m_isDarkMode = isDarkMode;
    }

    void setHighlightedMessage(const QString& messageId) {
        m_highlightedMessageId = messageId;
    }

    void getBubbleLayout(const QString& text, const QString& sender, bool isMe, int viewWidth,
                         QRect& bubbleRect, QRect& textRect, QRect& nameRect, QRect& avatarRect, 
                         int& neededHeight, bool hasReply, const QString& replyText, QRect& replyQuoteRect) const 
    {
        int maxBubbleWidth = viewWidth * 0.75;
        int nameHeight = isMe ? 0 : 18;
        int replyQuoteHeight = hasReply ? 48 : 0;
        int timeHeight = 12;
        int bubblePadding = 16;
        int avatarSize = 38;
        int avatarGap = 10;
        int sideMargin = 10;

        static QRegularExpression regex("^(.*)\\s\\[#([a-fA-F0-9]{6}),\\s*\\\"(.*)\\\"\\]$");
        QRegularExpressionMatch match = regex.match(sender);

        QString displayName = sender;
        QString tagText;
        bool hasTag = false;

        if (match.hasMatch()) {
            displayName = match.captured(1).trimmed();
            tagText = match.captured(3);
            hasTag = true;
        }

        int totalNameWidth = 0;
        if (!isMe) {
            QFont nameFont("Segoe UI", 11);
            nameFont.setBold(true);
            QFontMetrics fm(nameFont);
            int nameTextW = fm.horizontalAdvance(displayName);
            int tagW = 0;
            if (hasTag) {
                QFont tagFont("Segoe UI", 9);
                tagFont.setBold(true);
                QFontMetrics tagFm(tagFont);
                tagW = tagFm.horizontalAdvance(tagText) + 12;
            }
            totalNameWidth = nameTextW + (hasTag ? (tagW + 8) : 0);
        }

        int replyQuoteWidth = 0;
        if (hasReply && !replyText.isEmpty()) {
            QFont replyFont("Segoe UI", 9);
            replyFont.setItalic(true);
            QFontMetrics replyFm(replyFont);
            QString replyPreview = replyText.length() > 40 ? replyText.left(40) + "..." : replyText;
            replyQuoteWidth = replyFm.horizontalAdvance(replyPreview) + 30;
        }

        QTextDocument doc;
        doc.setDefaultFont(QFont("Segoe UI", 10));
        doc.setPlainText(text);
        doc.setTextWidth(maxBubbleWidth - 20);

        int textWidth = doc.idealWidth();
        int textHeight = doc.size().height();

        int contentWidth = isMe ? qMax(textWidth + 20, replyQuoteWidth) 
                                : qMax(textWidth + 20, qMax(totalNameWidth + 30, replyQuoteWidth));
        
        int bubbleW = qMax(contentWidth, 100);
        int bubbleH = textHeight + nameHeight + replyQuoteHeight + timeHeight + bubblePadding;

        neededHeight = bubbleH + 10;

        int x;
        if (isMe) {
            x = viewWidth - bubbleW - sideMargin;
            avatarRect = QRect();
        } else {
            x = sideMargin + avatarSize + avatarGap;
            avatarRect = QRect(sideMargin, 5 + bubbleH - avatarSize, avatarSize, avatarSize);
        }

        bubbleRect = QRect(x, 5, bubbleW, bubbleH);

        if (!isMe) {
            nameRect = QRect(x + 10, 5 + 5, bubbleW - 20, 15);
            if (hasReply) {
                replyQuoteRect = QRect(x + 10, 5 + 23, bubbleW - 20, replyQuoteHeight - 8);
                textRect = QRect(x + 10, 5 + 23 + replyQuoteHeight, bubbleW - 20, textHeight);
            } else {
                textRect = QRect(x + 10, 5 + 23, bubbleW - 20, textHeight);
            }
        } else {
            if (hasReply) {
                replyQuoteRect = QRect(x + 10, 5 + 8, bubbleW - 20, replyQuoteHeight - 8);
                textRect = QRect(x + 10, 5 + 8 + replyQuoteHeight, bubbleW - 20, textHeight);
            } else {
                textRect = QRect(x + 10, 5 + 8, bubbleW - 20, textHeight);
            }
        }
    }

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
		painter->save();
		painter->setRenderHint(QPainter::Antialiasing);

		QString text = index.data(Qt::UserRole + 1).toString();
		QString sender = index.data(Qt::UserRole + 2).toString();
		qint64 timestamp = index.data(Qt::UserRole + 3).toLongLong();
		bool isMe = index.data(Qt::UserRole + 4).toBool();

		// FIX: Added <QIcon> to value() call
		QIcon avatar = index.data(Qt::DecorationRole).value<QIcon>();

		QString replyToId = index.data(Qt::UserRole + 9).toString();
		QString messageId = index.data(Qt::UserRole + 6).toString();
		bool hasReply = !replyToId.isEmpty();

		QRect bubbleRect, textRect, nameRect, avatarRect, replyQuoteRect;
		int neededHeight;
		QString replyText;

		if (hasReply) {
			replyText = index.data(Qt::UserRole + 10).toString();
		}

		// Pass 'replyQuoteRect' which is a local QRect variable
		getBubbleLayout(text, sender, isMe, option.rect.width(),
						bubbleRect, textRect, nameRect, avatarRect, neededHeight,
						hasReply, replyText, replyQuoteRect);

		bubbleRect.translate(0, option.rect.top());
		textRect.translate(0, option.rect.top());
		nameRect.translate(0, option.rect.top());
		avatarRect.translate(0, option.rect.top());
		replyQuoteRect.translate(0, option.rect.top());

		if (!isMe && !avatar.isNull()) {
			avatar.paint(painter, avatarRect);
		}

		QColor bubbleColor;
		QColor borderColor;
		QColor textColor;
		QColor timeColor;

		bool isHighlighted = (messageId == m_highlightedMessageId);

		if (m_isDarkMode) {
			bubbleColor = isMe ? QColor("#2b5278") : QColor("#2d2d2d");
			borderColor = Qt::transparent;
			textColor = Qt::white;
			timeColor = QColor("#a0a0a0");
		} else {
			bubbleColor = isMe ? QColor("#EEFFDE") : Qt::white;
			borderColor = QColor("#d0d0d0");
			textColor = Qt::black;
			timeColor = QColor("#888888");
		}

		if (isHighlighted) {
			borderColor = QColor("#FFD700");
			painter->setBrush(bubbleColor);
			painter->setPen(QPen(borderColor, 3));
		} else {
			painter->setBrush(bubbleColor);
			painter->setPen(borderColor == Qt::transparent ? Qt::NoPen : QPen(borderColor, 1));
		}

		painter->drawRoundedRect(bubbleRect, 12, 12);

		if (!isMe) {
			static QRegularExpression regex("^(.*)\\s\\[#([a-fA-F0-9]{6}),\\s*\\\"(.*)\\\"\\]$");
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

			painter->setPen(QColor("#E35967"));
			QFont nameFont = option.font;
			nameFont.setPixelSize(11);
			nameFont.setBold(true);
			painter->setFont(nameFont);

			QFontMetrics fm(nameFont);
			int nameW = fm.horizontalAdvance(displayName);
			painter->drawText(nameRect.left(), nameRect.top() + fm.ascent(), displayName);

			if (hasTag) {
				int tagX = nameRect.left() + nameW + 6;
				int tagY = nameRect.top();
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

		// Draw Reply Quote
		if (!replyToId.isEmpty()) {
			QString replySenderName = index.data(Qt::UserRole + 11).toString();
			int replyX = replyQuoteRect.left();
			int replyY = replyQuoteRect.top();

			if (!replySenderName.isEmpty()) {
				QFont senderFont("Segoe UI", 9);
				senderFont.setBold(true);
				painter->setFont(senderFont);
				painter->setPen(m_isDarkMode ? QColor("#60A5FA") : QColor("#2563EB"));
				QFontMetrics senderFm(senderFont);
				painter->drawText(replyX + 6, replyY + senderFm.ascent(), replySenderName);
				replyY += senderFm.height() + 2;
			}

			// Vertical line
			painter->setPen(m_isDarkMode ? QColor("#666") : QColor("#ccc"));
			painter->drawLine(replyX, replyY, replyX, replyY + 20);

			QFont replyFont("Segoe UI", 9);
			replyFont.setItalic(true);
			painter->setFont(replyFont);
			painter->setPen(m_isDarkMode ? QColor("#aaa") : QColor("#666"));

			if (replyText.isEmpty()) {
				replyText = "[Original message]";
			}
			QString replyPreview = replyText.length() > 40 ? replyText.left(40) + "..." : replyText;
			QFontMetrics replyFm(replyFont);
			painter->drawText(replyX + 6, replyY + replyFm.ascent() + 2, replyPreview);
		}

		// Draw Message Text
		painter->setPen(textColor);
		QTextDocument doc;
		QFont textFont("Segoe UI", 10);
		doc.setDefaultFont(textFont);

		QString html = QString("<font color='%1'>%2</font>")
						   .arg(textColor.name())
						   .arg(text.toHtmlEscaped().replace("\n", "<br>"));
		doc.setHtml(html);
		doc.setTextWidth(textRect.width());

		painter->translate(textRect.topLeft());
		doc.drawContents(painter);
		painter->translate(-textRect.topLeft());

		// Draw Time & Status
		QDateTime dt;
		dt.setSecsSinceEpoch(timestamp);
		QString timeStr = dt.toString("hh:mm AP");

		painter->setPen(timeColor);
		QFont timeFont = option.font;
		timeFont.setPixelSize(9);
		painter->setFont(timeFont);
		QFontMetrics timeFm(timeFont);
		int timeWidth = timeFm.horizontalAdvance(timeStr);

		if (isMe) {
			int statusInt = index.data(Qt::UserRole + 5).toInt();
			MessageStatus status = static_cast<MessageStatus>(statusInt);

			QFont statusFont("Segoe UI", 9);
			painter->setFont(statusFont);
			QFontMetrics fm(statusFont);

			QString checkmark = QString(QChar(0x2713));
			QString circle = QString(QChar(0x25CB));
			bool hasCheckmark = fm.horizontalAdvance(checkmark) > 0;

			QString statusIcon;
			QColor statusColor = timeColor;
			int iconX = 0;

			if (status == MessageStatus::Pending) {
				statusIcon = circle;
				statusColor = QColor("#999999");
			} else if (status == MessageStatus::Sent) {
				statusIcon = hasCheckmark ? checkmark : "v";
				statusColor = QColor("#999999");
			} else if (status == MessageStatus::Seen) {
				if (hasCheckmark) {
					painter->setPen(m_isDarkMode ? QColor("#93C5FD") : QColor("#60A5FA"));
					int singleWidth = fm.horizontalAdvance(checkmark);
					int overlapOffset = singleWidth / 2;
					iconX = bubbleRect.right() - 8 - timeWidth - 4 - singleWidth - overlapOffset;
					int iconY = bubbleRect.bottom() - 5 - timeFm.height();
					painter->drawText(iconX, iconY + fm.ascent(), checkmark);
					painter->drawText(iconX + overlapOffset, iconY + fm.ascent(), checkmark);
					statusIcon.clear();
				} else {
					statusIcon = "vv";
					statusColor = m_isDarkMode ? QColor("#93C5FD") : QColor("#60A5FA");
				}
			}

			if (!statusIcon.isEmpty()) {
				painter->setPen(statusColor);
				int iconWidth = fm.horizontalAdvance(statusIcon);
				iconX = bubbleRect.right() - 8 - timeWidth - 4 - iconWidth;
				int iconY = bubbleRect.bottom() - 5 - timeFm.height();
				painter->drawText(iconX, iconY + fm.ascent(), statusIcon);
			}
		}

		painter->setPen(timeColor);
		painter->setFont(timeFont);
		painter->drawText(bubbleRect.adjusted(0, 0, -8, -5), Qt::AlignBottom | Qt::AlignRight, timeStr);

		qint64 editedAt = index.data(Qt::UserRole + 7).toLongLong();
		if (editedAt > 0) {
			QFont editedFont = timeFont;
			editedFont.setItalic(true);
			editedFont.setPixelSize(8);
			painter->setFont(editedFont);
			painter->setPen(QColor("#888888"));
			QFontMetrics editedFm(editedFont);
			QString editedText = "edited";
			int editedWidth = editedFm.horizontalAdvance(editedText);
			int editedX = bubbleRect.right() - 8 - editedWidth;
			int editedY = bubbleRect.bottom() - 5 - timeFm.height() - editedFm.height() - 2;
			painter->drawText(editedX, editedY + editedFm.ascent(), editedText);
		}

		painter->restore();
	}

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QString text = index.data(Qt::UserRole + 1).toString();
        QString sender = index.data(Qt::UserRole + 2).toString();
        bool isMe = index.data(Qt::UserRole + 4).toBool();
        QString replyToId = index.data(Qt::UserRole + 9).toString();
        bool hasReply = !replyToId.isEmpty();

        QString replyText;
        if (hasReply) {
            replyText = index.data(Qt::UserRole + 10).toString();
        }

        QRect b, t, n, a, r;
        int h;
        // Pass 'r' which is local
        getBubbleLayout(text, sender, isMe, option.rect.width(), b, t, n, a, h, hasReply, replyText, r);
        return QSize(option.rect.width(), h);
    }

    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                QString replyToId = index.data(Qt::UserRole + 9).toString();
                if (!replyToId.isEmpty()) {
                    QString text = index.data(Qt::UserRole + 1).toString();
                    QString sender = index.data(Qt::UserRole + 2).toString();
                    bool isMe = index.data(Qt::UserRole + 4).toBool();
                    
                    QRect bubbleRect, textRect, nameRect, avatarRect, replyQuoteRect;
                    int neededHeight;
                    QString replyText = index.data(Qt::UserRole + 10).toString();
                    bool hasReply = true;
                    
                    // Pass 'replyQuoteRect' which is local
                    getBubbleLayout(text, sender, isMe, option.rect.width(), 
                                    bubbleRect, textRect, nameRect, avatarRect, neededHeight, 
                                    hasReply, replyText, replyQuoteRect);
                    
                    replyQuoteRect.translate(0, option.rect.top());
                    if (replyQuoteRect.contains(mouseEvent->pos())) {
                        if (m_mainWindow) {
                            m_mainWindow->focusOnMessage(replyToId);
                        }
                        return true;
                    }
                }
            }
        }
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }
};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_client(new WebSocketClient(this)), m_nam(new QNetworkAccessManager(this))
{
    QSettings settings("Noveo", "MessengerClient");
    m_isDarkMode = settings.value("darkMode", false).toBool();
    m_notificationsEnabled = settings.value("notificationsEnabled", true).toBool();

    setupUi();
    applyTheme();
    setupTrayIcon();

    if (settings.contains("username")) {
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
    connect(m_client, &WebSocketClient::messageSeenUpdate, this, &MainWindow::onMessageSeenUpdate);
    connect(m_client, &WebSocketClient::messageUpdated, this, &MainWindow::onMessageUpdated);
    connect(m_client, &WebSocketClient::messageDeleted, this, &MainWindow::onMessageDeleted);

    m_statusLabel->setText("Connecting to server...");
    m_client->connectToServer();
}

MainWindow::~MainWindow() {}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
}

void MainWindow::setupUi() {
    resize(1000, 700);
    setWindowTitle("Noveo Desktop");

    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);

    // Login Page
    m_loginPage = new QWidget();
    QVBoxLayout* loginLayout = new QVBoxLayout(m_loginPage);
    QLabel* title = new QLabel("Noveo");
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

    // App Page
    m_appPage = new QWidget();
    QHBoxLayout* appLayout = new QHBoxLayout(m_appPage);
    appLayout->setContentsMargins(0, 0, 0, 0);
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

    m_settingsTab = new QWidget();
    QVBoxLayout* settingsLayout = new QVBoxLayout(m_settingsTab);

    QCheckBox* darkModeCheck = new QCheckBox("Dark Mode");
    darkModeCheck->setChecked(m_isDarkMode);
    connect(darkModeCheck, &QCheckBox::toggled, this, &MainWindow::onDarkModeToggled);

    m_notificationsCheck = new QCheckBox("Notifications");
    m_notificationsCheck->setChecked(m_notificationsEnabled);
    connect(m_notificationsCheck, &QCheckBox::toggled, this, &MainWindow::onNotificationsToggled);

    QPushButton* logoutBtn = new QPushButton("Log Out");
    connect(logoutBtn, &QPushButton::clicked, this, &MainWindow::onLogoutClicked);

    settingsLayout->addWidget(new QLabel("Settings"));
    settingsLayout->addWidget(darkModeCheck);
    settingsLayout->addWidget(m_notificationsCheck);
    settingsLayout->addStretch();
    settingsLayout->addWidget(logoutBtn);
    settingsLayout->addSpacing(20);

    m_sidebarTabs->addTab(m_chatListWidget, "Chats");
    m_sidebarTabs->addTab(m_contactListWidget, "Contacts");
    m_sidebarTabs->addTab(m_settingsTab, "Settings");

    // Chat Area
    m_chatAreaWidget = new QWidget();
    QVBoxLayout* chatAreaLayout = new QVBoxLayout(m_chatAreaWidget);
    chatAreaLayout->setContentsMargins(0, 0, 0, 0);
    chatAreaLayout->setSpacing(0);

    // Header
    QWidget* header = new QWidget();
    header->setObjectName("chatHeader");
    header->setFixedHeight(60);
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    m_chatTitle = new QLabel("Select a chat");
    m_chatTitle->setStyleSheet("font-size: 16px; font-weight: bold; margin-left: 10px;");
    headerLayout->addWidget(m_chatTitle);

    // Messages
    m_chatList = new QListWidget();
    m_chatList->setObjectName("chatList");
    m_chatList->setFrameShape(QFrame::NoFrame);
    m_chatList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_chatList->setUniformItemSizes(false);
    m_chatList->setResizeMode(QListView::Adjust);
    m_chatList->setSelectionMode(QAbstractItemView::NoSelection);
    m_chatList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    MessageDelegate* messageDelegate = new MessageDelegate(this);
    m_chatList->setItemDelegate(messageDelegate);
    m_chatList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_chatList, &QListWidget::customContextMenuRequested, this, &MainWindow::onChatListContextMenu);
    connect(m_chatList, &QListWidget::clicked, this, &MainWindow::onChatListItemClicked);
    connect(m_chatList->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::onScrollValueChanged);

    // Reply Bar
    m_replyBar = new QWidget();
    m_replyBar->setObjectName("replyBar");
    m_replyBar->setFixedHeight(40);
    m_replyBar->hide();

    QHBoxLayout* replyBarLayout = new QHBoxLayout(m_replyBar);
    replyBarLayout->setContentsMargins(10, 5, 10, 5);

    QVBoxLayout* replyTextLayout = new QVBoxLayout();
    QLabel* replyTitle = new QLabel("Replying to:");
    replyTitle->setStyleSheet("color: #888; font-size: 10px;");
    m_replyLabel = new QLabel();
    m_replyLabel->setStyleSheet("color: #333; font-weight: 500;");
    replyTextLayout->addWidget(replyTitle);
    replyTextLayout->addWidget(m_replyLabel);

    m_cancelReplyBtn = new QPushButton("X");
    m_cancelReplyBtn->setFixedSize(25, 25);
    m_cancelReplyBtn->setCursor(Qt::PointingHandCursor);
    m_cancelReplyBtn->setStyleSheet("border: none; background: transparent; color: #888; font-size: 16px; font-weight: bold;");

    replyBarLayout->addLayout(replyTextLayout);
    replyBarLayout->addStretch();
    replyBarLayout->addWidget(m_cancelReplyBtn);

    connect(m_cancelReplyBtn, &QPushButton::clicked, this, &MainWindow::onCancelReply);

    // Edit Bar
    m_editBar = new QWidget();
    m_editBar->setObjectName("editBar");
    m_editBar->setFixedHeight(35);
    m_editBar->hide();

    QHBoxLayout* editBarLayout = new QHBoxLayout(m_editBar);
    editBarLayout->setContentsMargins(10, 5, 10, 5);
    
    m_editLabel = new QLabel("Editing: ");
    m_editLabel->setStyleSheet("color: #888; font-style: italic;");
    
    m_cancelEditBtn = new QPushButton("X");
    m_cancelEditBtn->setFixedSize(25, 25);
    m_cancelEditBtn->setCursor(Qt::PointingHandCursor);
    m_cancelEditBtn->setStyleSheet("border: none; background: transparent; color: #888; font-size: 16px; font-weight: bold;");

    editBarLayout->addWidget(m_editLabel);
    editBarLayout->addStretch();
    editBarLayout->addWidget(m_cancelEditBtn);

    connect(m_cancelEditBtn, &QPushButton::clicked, this, &MainWindow::onCancelEdit);

    // Input Area
    QWidget* inputArea = new QWidget();
    inputArea->setObjectName("inputArea");
    inputArea->setFixedHeight(60);
    QHBoxLayout* inputLayout = new QHBoxLayout(inputArea);

    m_messageInput = new QLineEdit();
    m_messageInput->setPlaceholderText("Write a message...");
    m_sendBtn = new QPushButton("Send");
    m_sendBtn->setCursor(Qt::PointingHandCursor);

    inputLayout->addWidget(m_messageInput);
    inputLayout->addWidget(m_sendBtn);

    chatAreaLayout->addWidget(header);
    chatAreaLayout->addWidget(m_chatList);
    chatAreaLayout->addWidget(m_replyBar);
    chatAreaLayout->addWidget(m_editBar);
    chatAreaLayout->addWidget(inputArea);

    appLayout->addWidget(m_sidebarTabs);
    appLayout->addWidget(m_chatAreaWidget);

    m_stackedWidget->addWidget(m_appPage);

    // Connections
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
    QString scrollHandle = m_isDarkMode ? "#505050" : "#c0c0c0";
    QString scrollHandleHover = m_isDarkMode ? "#606060" : "#a0a0a0";

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
        "#chatList { background-color: %1; border: none; }"
        "QScrollBar:vertical { border: none; background: %2; width: 10px; margin: 0px; }"
        "QScrollBar::handle:vertical { background: %7; min-height: 20px; border-radius: 5px; }"
        "QScrollBar::handle:vertical:hover { background: %8; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"
        "QCheckBox { color: %3; spacing: 5px; }"
        "QCheckBox::indicator { width: 18px; height: 18px; border: 1px solid %4; border-radius: 4px; background: %5; }"
        "QCheckBox::indicator:checked { background-color: #0088cc; border: 1px solid #0088cc; }"
    ).arg(bg, panelBg, text, border, inputBg, listHover, scrollHandle, scrollHandleHover);

    setStyleSheet(style);

    if (m_chatList->itemDelegate()) {
        MessageDelegate* delegate = dynamic_cast<MessageDelegate*>(m_chatList->itemDelegate());
        if (delegate) {
            delegate->setTheme(m_isDarkMode);
            m_chatList->viewport()->update();
        }
    }
}

void MainWindow::onScrollValueChanged(int value) {
    if (value == 0 && !m_isLoadingHistory && !m_currentChatId.isEmpty()) {
        if (m_chats.contains(m_currentChatId)) {
            const auto& msgs = m_chats[m_currentChatId].messages;
            if (!msgs.empty()) {
                qint64 oldestTime = msgs.front().timestamp;
                m_isLoadingHistory = true;
                m_client->fetchHistory(m_currentChatId, oldestTime);
            }
        }
    }
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
    m_chats.clear();
    m_users.clear();
    m_currentChatId.clear();
    m_isLoadingHistory = false;
    m_avatarCache.clear();
    m_pendingDownloads.clear();
    
    m_chatListWidget->clear();
    m_contactListWidget->clear();
    m_chatList->clear();
    m_chatTitle->setText("Select a chat");
    
    m_loginBtn->setEnabled(false);
    m_statusLabel->setText("Resetting connection...");

    m_client->logout();
    m_stackedWidget->setCurrentWidget(m_loginPage);
    QTimer::singleShot(500, m_client, &WebSocketClient::connectToServer);
}

void MainWindow::onConnected() {
    m_statusLabel->setText("Connected.");
    m_loginBtn->setEnabled(true);

    QSettings settings("Noveo", "MessengerClient");
    if (settings.contains("username") && settings.contains("password")) {
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

void MainWindow::onLoginSuccess(const User& user, const QString& token) {
    QSettings settings("Noveo", "MessengerClient");
    settings.setValue("username", m_usernameInput->text());
    settings.setValue("password", m_passwordInput->text());
    
    m_stackedWidget->setCurrentWidget(m_appPage);
}

void MainWindow::onAuthFailed(const QString& msg) {
    m_statusLabel->setText("Login Failed: " + msg);
}

void MainWindow::onUserListUpdated(const std::vector<User>& users) {
    m_users.clear();
    m_contactListWidget->clear();

    for (const auto& u : users) {
        m_users.insert(u.userId, u);
        if (u.userId == m_client->currentUserId()) continue;

        QListWidgetItem* item = new QListWidgetItem(m_contactListWidget);
        item->setText(u.username);
        item->setData(Qt::UserRole, u.userId);

        QString fullUrl = u.avatarUrl;
        if (!fullUrl.startsWith("http")) {
            fullUrl = API_BASE_URL + fullUrl;
        }
        item->setData(AvatarUrlRole, fullUrl);
        item->setIcon(getAvatar(u.username, fullUrl));
    }

    for (int i = 0; i < m_chatListWidget->count(); i++) {
        QListWidgetItem* item = m_chatListWidget->item(i);
        QString chatId = item->data(Qt::UserRole).toString();
        if (m_chats.contains(chatId)) {
            QString name = resolveChatName(m_chats[chatId]);
            QString fullUrl = m_chats[chatId].avatarUrl;
            if (!fullUrl.startsWith("http")) {
                 fullUrl = API_BASE_URL + fullUrl;
            }
            item->setText(name);
            item->setData(AvatarUrlRole, fullUrl);
            item->setIcon(getAvatar(name, fullUrl));
        }
    }
}

void MainWindow::onChatHistoryReceived(const std::vector<Chat>& incomingChats) {
    bool initialLoad = m_chats.isEmpty();

    for (const auto& inChat : incomingChats) {
        if (m_chats.contains(inChat.chatId)) {
            Chat& existingChat = m_chats[inChat.chatId];
            QSet<QString> existingIds;
            for (const auto& m : existingChat.messages) existingIds.insert(m.messageId);

            std::vector<Message> newMessages;
            for (const auto& m : inChat.messages) {
                if (!existingIds.contains(m.messageId)) {
                    newMessages.push_back(m);
                }
            }

            if (!newMessages.empty()) {
                existingChat.messages.insert(existingChat.messages.end(), newMessages.begin(), newMessages.end());
                std::sort(existingChat.messages.begin(), existingChat.messages.end(), [](const Message& a, const Message& b) {
                    return a.timestamp < b.timestamp;
                });

                if (m_currentChatId == inChat.chatId && m_isLoadingHistory) {
                    m_isLoadingHistory = false;
                    
                    std::sort(newMessages.begin(), newMessages.end(), [](const Message& a, const Message& b) {
                        return a.timestamp < b.timestamp;
                    });

                    int oldMax = m_chatList->verticalScrollBar()->maximum();
                    for (auto it = newMessages.rbegin(); it != newMessages.rend(); ++it) {
                        prependMessageBubble(*it);
                    }
                    int newMax = m_chatList->verticalScrollBar()->maximum();
                    m_chatList->verticalScrollBar()->setValue(newMax - oldMax);
                }
            }
        } else {
            m_chats.insert(inChat.chatId, inChat);
        }
    }

    if (initialLoad) {
        m_chatListWidget->clear();
        std::vector<Chat> sortedChats;
        for (auto k : m_chats.keys()) sortedChats.push_back(m_chats[k]);
        
        std::sort(sortedChats.begin(), sortedChats.end(), [](const Chat& a, const Chat& b) {
            qint64 timeA = a.messages.empty() ? 0 : a.messages.back().timestamp;
            qint64 timeB = b.messages.empty() ? 0 : b.messages.back().timestamp;
            return timeA > timeB;
        });

        for (const auto& chat : sortedChats) {
            QListWidgetItem* item = new QListWidgetItem(m_chatListWidget);
            QString name = resolveChatName(chat);
            QString url = chat.avatarUrl;
            
            if (chat.chatType == "private" && url.isEmpty()) {
                for (const auto& memberId : chat.members) {
                    if (memberId != m_client->currentUserId() && m_users.contains(memberId)) {
                        url = m_users[memberId].avatarUrl;
                        break;
                    }
                }
            }

            if (url.startsWith("/")) url = API_BASE_URL + url;
            
            item->setText(name);
            item->setData(Qt::UserRole, chat.chatId);
            item->setData(AvatarUrlRole, url);
            item->setIcon(getAvatar(name, url));
            m_chatListWidget->addItem(item);
        }
    }
}

void MainWindow::onNewChatCreated(const Chat& chat) {
    if (!m_chats.contains(chat.chatId)) {
        m_chats.insert(chat.chatId, chat);
        QListWidgetItem* item = new QListWidgetItem(m_chatListWidget);
        
        QString name = resolveChatName(chat);
        QString url = chat.avatarUrl;
        if (chat.chatType == "private" && url.isEmpty()) {
            for (const auto& memberId : chat.members) {
                if (memberId != m_client->currentUserId() && m_users.contains(memberId)) {
                    url = m_users[memberId].avatarUrl;
                    break;
                }
            }
        }
        if (url.startsWith("/")) url = API_BASE_URL + url;

        item->setText(name);
        item->setData(Qt::UserRole, chat.chatId);
        item->setData(AvatarUrlRole, url);
        item->setIcon(getAvatar(name, url));
        m_chatListWidget->insertItem(0, item);
    }
}

QString MainWindow::resolveChatName(const Chat& chat) {
    if (!chat.chatName.isEmpty()) return chat.chatName;
    if (chat.chatType == "private") {
        for (const auto& memberId : chat.members) {
            if (memberId != m_client->currentUserId()) {
                if (m_users.contains(memberId)) return m_users[memberId].username;
            }
        }
        return "Unknown User";
    }
    return "Chat";
}

QColor MainWindow::getColorForName(const QString& name) {
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

QIcon MainWindow::generateGenericAvatar(const QString& name) {
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

QIcon MainWindow::getAvatar(const QString& name, const QString& urlIn) {
    if (urlIn.isEmpty() || urlIn == "default.png" || urlIn == "/default.png" || urlIn.endsWith("/default.png")) {
        return generateGenericAvatar(name);
    }

    QString fullUrl = urlIn;
    if (fullUrl.startsWith("/")) {
        fullUrl = API_BASE_URL + fullUrl;
    } else if (!fullUrl.startsWith("http://") && !fullUrl.startsWith("https://")) {
        fullUrl = API_BASE_URL + "/" + fullUrl;
    }

    if (m_avatarCache.contains(fullUrl)) {
        return QIcon(m_avatarCache[fullUrl]);
    }

    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/avatars";
    QDir().mkpath(cacheDir);
    QString urlHash = QString(QCryptographicHash::hash(fullUrl.toUtf8(), QCryptographicHash::Md5).toHex());
    QString cachedFilePath = cacheDir + "/" + urlHash + ".png";

    if (QFile::exists(cachedFilePath)) {
        QPixmap pixmap;
        if (pixmap.load(cachedFilePath)) {
            m_avatarCache.insert(fullUrl, pixmap);
            return QIcon(pixmap);
        }
    }

    if (!m_pendingDownloads.contains(fullUrl)) {
        m_pendingDownloads.insert(fullUrl);
        QUrl qurl(fullUrl);
        QNetworkRequest request(qurl);
        request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
        request.setTransferTimeout(60000);

        QNetworkReply* reply = m_nam->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, fullUrl, name]() {
            reply->deleteLater();
            m_pendingDownloads.remove(fullUrl);

            if (reply->error() == QNetworkReply::NoError) {
                QByteArray data = reply->readAll();
                QPixmap pixmap;
                if (pixmap.loadFromData(data)) {
                    QPixmap circular(42, 42);
                    circular.fill(Qt::transparent);
                    QPainter p(&circular);
                    p.setRenderHint(QPainter::Antialiasing);
                    QPainterPath path;
                    path.addEllipse(0, 0, 42, 42);
                    p.setClipPath(path);
                    
                    int sourceSize = qMin(pixmap.width(), pixmap.height());
                    int x = (pixmap.width() - sourceSize) / 2;
                    int y = (pixmap.height() - sourceSize) / 2;
                    QPixmap cropped = pixmap.copy(x, y, sourceSize, sourceSize);
                    
                    p.drawPixmap(0, 0, 42, 42, cropped.scaled(42, 42, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    m_avatarCache.insert(fullUrl, circular);

                    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/avatars";
                    QString urlHash = QString(QCryptographicHash::hash(fullUrl.toUtf8(), QCryptographicHash::Md5).toHex());
                    QString cachedFilePath = cacheDir + "/" + urlHash + ".png";
                    circular.save(cachedFilePath, "PNG");

                    updateAvatarOnItems(fullUrl, circular);
                }
            } else {
                // Failed download, fallback
            }
        });
    }

    return generateGenericAvatar(name);
}

void MainWindow::updateAvatarOnItems(const QString& url, const QPixmap& pixmap) {
    QIcon icon(pixmap);
    auto updateList = [&](QListWidget* list) {
        bool updated = false;
        for (int i = 0; i < list->count(); i++) {
            QListWidgetItem* item = list->item(i);
            if (item && item->data(AvatarUrlRole).toString() == url) {
                item->setIcon(icon);
                updated = true;
            }
        }
        if (updated) {
            list->viewport()->update();
        }
    };
    
    updateList(m_chatListWidget);
    updateList(m_contactListWidget);
    updateList(m_chatList);
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
    QScrollBar* sb = m_chatList->verticalScrollBar();
    return sb->value() >= (sb->maximum() - 50);
}

void MainWindow::onChatSelected(QListWidgetItem* item) {
    QString chatId = item->data(Qt::UserRole).toString();
    m_currentChatId = chatId;
    m_highlightedMessageId.clear();

    if (m_chats.contains(chatId)) {
        Chat chat = m_chats[chatId];
        m_chatTitle->setText(resolveChatName(chat));
        renderMessages(chatId);
    }
}

void MainWindow::onContactSelected(QListWidgetItem* item) {
    QString userId = item->data(Qt::UserRole).toString();
    QString selfId = m_client->currentUserId();
    QStringList ids = { userId, selfId };
    ids.sort();
    QString potentialChatId = ids.join("_");

    if (m_chats.contains(potentialChatId)) {
        m_currentChatId = potentialChatId;
        m_highlightedMessageId.clear();
        m_chatTitle->setText(resolveChatName(m_chats[potentialChatId]));
        renderMessages(potentialChatId);
    } else {
        m_currentChatId = potentialChatId;
        m_highlightedMessageId.clear();
        m_chatTitle->setText(item->text());
        m_chatList->clear();
    }
}

void MainWindow::renderMessages(const QString& chatId) {
    m_chatList->clear();
    if (m_chats.contains(chatId)) {
        Chat& chat = m_chats[chatId];
        for (auto& msg : chat.messages) {
            msg.status = calculateMessageStatus(msg, chat);
            addMessageBubble(msg, false, false);
            
            if (msg.senderId != m_client->currentUserId() && !msg.seenBy.contains(m_client->currentUserId())) {
                m_client->sendMessageSeen(chatId, msg.messageId);
                msg.seenBy.append(m_client->currentUserId());
            }
        }
        scrollToBottom();
    }
}

QString MainWindow::getReplyPreviewText(const QString& replyToId, const QString& chatId) {
    QString replyText;
    
    // 1. Try finding in current list
    for (int i = 0; i < m_chatList->count(); i++) {
        QListWidgetItem* listItem = m_chatList->item(i);
        QString itemMsgId = listItem->data(Qt::UserRole + 6).toString();
        QString itemText = listItem->data(Qt::UserRole + 1).toString();
        if (itemMsgId == replyToId) {
            replyText = itemText;
            break;
        }
    }

    // 2. Try finding in data model
    if (replyText.isEmpty() && !chatId.isEmpty() && m_chats.contains(chatId)) {
        for (const auto& m : m_chats[chatId].messages) {
            if (m.messageId == replyToId) {
                replyText = m.text;
                break;
            }
        }
    }

    if (!replyText.isEmpty()) {
        QString trimmed = replyText.trimmed();
        bool isJustURL = trimmed.startsWith("http") && 
                         (trimmed.contains("/static/upload") || trimmed.contains("/static/upl"));
        
        if (isJustURL) {
            QStringList words = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (words.size() == 1) {
                replyText = "[Image]";
            } else if (words.size() == 2 && words.last().startsWith("http")) {
                replyText = words.first() + " [Image]";
            }
        } else if (trimmed.startsWith("[FILE:") && trimmed.endsWith("]")) {
            replyText = "[Image]";
        }
    }

    if (replyText.isEmpty()) {
        replyText = "[Original message]";
    }

    return replyText;
}

void MainWindow::addMessageBubble(const Message& msg, bool appendStretch, bool animate) {
    Q_UNUSED(appendStretch);
    Q_UNUSED(animate);

    bool isMe = (msg.senderId == m_client->currentUserId());
    QString senderName = "Unknown";
    QString avatarUrl = "";
    if (m_users.contains(msg.senderId)) {
        senderName = m_users[msg.senderId].username;
        avatarUrl = m_users[msg.senderId].avatarUrl;
    }

    QString displayText = msg.text;
    QString trimmed = displayText.trimmed();
    
    // Clean up image URLs for display
    if (trimmed.startsWith("http") && (trimmed.contains("/static/upload") || trimmed.contains("/static/upl"))) {
        QStringList words = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (words.size() == 1) {
            if (trimmed.contains(".")) {
                 QStringList parts = trimmed.split("/");
                 QString filename = parts.last();
                 if (filename.contains(".") && 
                     !filename.endsWith(".png") && !filename.endsWith(".jpg") && 
                     !filename.endsWith(".jpeg") && !filename.endsWith(".gif") &&
                     !filename.endsWith(".webp")) {
                     displayText = "[" + filename + "]";
                 } else {
                     displayText = "[Image]";
                 }
            } else {
                displayText = "[Image]";
            }
        } else if (words.size() == 2 && words.last().startsWith("http")) {
            displayText = words.first() + " [Image]";
        }
    }

    QListWidgetItem* item = new QListWidgetItem(m_chatList);
    item->setData(Qt::UserRole + 1, displayText);
    item->setData(Qt::UserRole + 2, senderName);
    item->setData(Qt::UserRole + 3, msg.timestamp);
    item->setData(Qt::UserRole + 4, isMe);
    item->setData(Qt::UserRole + 5, static_cast<int>(msg.status));
    item->setData(Qt::UserRole + 6, msg.messageId);
    item->setData(Qt::UserRole + 7, msg.editedAt);
    item->setData(Qt::UserRole + 8, msg.senderId);
    item->setData(Qt::UserRole + 9, msg.replyToId);

    if (!msg.replyToId.isEmpty()) {
        QString replyText = getReplyPreviewText(msg.replyToId, msg.chatId);
        item->setData(Qt::UserRole + 10, replyText);
        
        QString replySenderName = "";
        for (int i = 0; i < m_chatList->count(); i++) {
            QListWidgetItem* listItem = m_chatList->item(i);
            QString itemMsgId = listItem->data(Qt::UserRole + 6).toString();
            if (itemMsgId == msg.replyToId) {
                replySenderName = listItem->data(Qt::UserRole + 2).toString();
                break;
            }
        }
        item->setData(Qt::UserRole + 11, replySenderName);
    }

    if (!isMe) {
        QString fullUrl = avatarUrl;
        if (fullUrl.startsWith("/")) {
            fullUrl = API_BASE_URL + fullUrl;
        }
        item->setData(AvatarUrlRole, fullUrl);
        item->setIcon(getAvatar(senderName, fullUrl));
    }
}

void MainWindow::prependMessageBubble(const Message& msg) {
    bool isMe = (msg.senderId == m_client->currentUserId());
    QString senderName = "Unknown";
    QString avatarUrl = "";
    if (m_users.contains(msg.senderId)) {
        senderName = m_users[msg.senderId].username;
        avatarUrl = m_users[msg.senderId].avatarUrl;
    }

    QString displayText = msg.text;
    QString trimmed = displayText.trimmed();

    if (trimmed.startsWith("http") && (trimmed.contains("/static/upload") || trimmed.contains("/static/upl"))) {
        QStringList words = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (words.size() == 1) {
             if (trimmed.contains(".")) {
                 QStringList parts = trimmed.split("/");
                 QString filename = parts.last();
                 if (filename.contains(".") && 
                     !filename.endsWith(".png") && !filename.endsWith(".jpg") && 
                     !filename.endsWith(".jpeg") && !filename.endsWith(".gif") &&
                     !filename.endsWith(".webp")) {
                     displayText = "[" + filename + "]";
                 } else {
                     displayText = "[Image]";
                 }
            } else {
                displayText = "[Image]";
            }
        } else if (words.size() == 2 && words.last().startsWith("http")) {
            displayText = words.first() + " [Image]";
        }
    }

    QListWidgetItem* item = new QListWidgetItem();
    item->setData(Qt::UserRole + 1, displayText);
    item->setData(Qt::UserRole + 2, senderName);
    item->setData(Qt::UserRole + 3, msg.timestamp);
    item->setData(Qt::UserRole + 4, isMe);
    item->setData(Qt::UserRole + 5, static_cast<int>(msg.status));
    item->setData(Qt::UserRole + 6, msg.messageId);
    item->setData(Qt::UserRole + 7, msg.editedAt);
    item->setData(Qt::UserRole + 8, msg.senderId);
    item->setData(Qt::UserRole + 9, msg.replyToId);

    if (!msg.replyToId.isEmpty()) {
        QString replyText = getReplyPreviewText(msg.replyToId, msg.chatId);
        item->setData(Qt::UserRole + 10, replyText);

        QString replySenderName = "";
        for (int i = 0; i < m_chatList->count(); i++) {
            QListWidgetItem* listItem = m_chatList->item(i);
            QString itemMsgId = listItem->data(Qt::UserRole + 6).toString();
            if (itemMsgId == msg.replyToId) {
                replySenderName = listItem->data(Qt::UserRole + 2).toString();
                break;
            }
        }
        item->setData(Qt::UserRole + 11, replySenderName);
    }

    if (!isMe) {
        QString fullUrl = avatarUrl;
        if (fullUrl.startsWith("/")) {
            fullUrl = API_BASE_URL + fullUrl;
        }
        item->setData(AvatarUrlRole, fullUrl);
        item->setIcon(getAvatar(senderName, fullUrl));
    }
    m_chatList->insertItem(0, item);
}

void MainWindow::updateMessageStatus(const QString& messageId, MessageStatus newStatus) {
    for (int i = 0; i < m_chatList->count(); i++) {
        QListWidgetItem* item = m_chatList->item(i);
        QString itemMsgId = item->data(Qt::UserRole + 6).toString();
        if (itemMsgId == messageId) {
            item->setData(Qt::UserRole + 5, static_cast<int>(newStatus));
            QModelIndex modelIndex = m_chatList->model()->index(i, 0);
            m_chatList->update(m_chatList->visualRect(modelIndex));
            break;
        }
    }

    for (auto& chat : m_chats) {
        for (auto& msg : chat.messages) {
            if (msg.messageId == messageId) {
                msg.status = newStatus;
                break;
            }
        }
    }
}

void MainWindow::onMessageReceived(const Message& msg) {
    bool isPendingConfirmation = false;
    QString pendingIdToRemove;

    if (msg.senderId == m_client->currentUserId()) {
        for (auto it = m_pendingMessages.begin(); it != m_pendingMessages.end(); ++it) {
            if (it.value().text == msg.text && it.value().chatId == msg.chatId) {
                isPendingConfirmation = true;
                pendingIdToRemove = it.key();
                break;
            }
        }
    }

    if (m_chats.contains(msg.chatId)) {
        m_chats[msg.chatId].messages.push_back(msg);
        for (int i = 0; i < m_chatListWidget->count(); i++) {
            if (m_chatListWidget->item(i)->data(Qt::UserRole).toString() == msg.chatId) {
                QListWidgetItem* item = m_chatListWidget->takeItem(i);
                m_chatListWidget->insertItem(0, item);
                break;
            }
        }
    }

    if (m_currentChatId == msg.chatId) {
        bool wasAtBottom = isScrolledToBottom();
        
        if (isPendingConfirmation) {
            for (int i = 0; i < m_chatList->count(); i++) {
                QListWidgetItem* item = m_chatList->item(i);
                if (item->data(Qt::UserRole + 6).toString() == pendingIdToRemove) {
                    delete m_chatList->takeItem(i);
                    break;
                }
            }
            m_pendingMessages.remove(pendingIdToRemove);
        }

        addMessageBubble(msg, false, false);

        if (msg.senderId != m_client->currentUserId()) {
            m_client->sendMessageSeen(msg.chatId, msg.messageId);
        }

        if (wasAtBottom || msg.senderId == m_client->currentUserId()) {
            smoothScrollToBottom();
        }
    } else if (isPendingConfirmation) {
        m_pendingMessages.remove(pendingIdToRemove);
    }
    
    // NOTIFICATION HOOK
    showNotificationForMessage(msg);
}

MessageStatus MainWindow::calculateMessageStatus(const Message& msg, const Chat& chat) {
    if (msg.senderId != m_client->currentUserId()) {
        return MessageStatus::Sent; 
    }
    if (msg.messageId.startsWith("temp_")) {
        return MessageStatus::Pending;
    }

    int otherMembersCount = 0;
    int seenByOthers = 0;

    for (const QString& memberId : chat.members) {
        if (memberId != m_client->currentUserId()) {
            otherMembersCount++;
            if (msg.seenBy.contains(memberId)) {
                seenByOthers++;
            }
        }
    }

    if (seenByOthers > 0) {
        return MessageStatus::Seen;
    }
    return MessageStatus::Sent;
}

void MainWindow::onMessageSeenUpdate(const QString& chatId, const QString& messageId, const QString& userId) {
    if (m_chats.contains(chatId)) {
        for (auto& msg : m_chats[chatId].messages) {
            if (msg.messageId == messageId) {
                if (!msg.seenBy.contains(userId)) {
                    msg.seenBy.append(userId);
                }
                MessageStatus newStatus = calculateMessageStatus(msg, m_chats[chatId]);
                msg.status = newStatus;
                
                if (m_currentChatId == chatId) {
                    updateMessageStatus(messageId, newStatus);
                }
                break;
            }
        }
    }
}

void MainWindow::onChatListContextMenu(const QPoint& pos) {
    QListWidgetItem* item = m_chatList->itemAt(pos);
    if (!item) return;

    QString messageId = item->data(Qt::UserRole + 6).toString();
    QString senderId = item->data(Qt::UserRole + 8).toString();
    bool isMyMessage = (senderId == m_client->currentUserId());

    QMenu contextMenu(this);
    if (m_isDarkMode) {
        contextMenu.setStyleSheet(
            "QMenu {"
            " background-color: #2d2d2d;"
            " color: #ffffff;"
            " border: 1px solid #444444;"
            " padding: 4px;"
            "}"
            "QMenu::item {"
            " padding: 8px 24px;"
            " background-color: transparent;"
            "}"
            "QMenu::item:selected {"
            " background-color: #3b5278;"
            "}"
        );
    } else {
        contextMenu.setStyleSheet(
            "QMenu {"
            " background-color: #ffffff;"
            " color: #000000;"
            " border: 1px solid #cccccc;"
            " padding: 4px;"
            "}"
            "QMenu::item {"
            " padding: 8px 24px;"
            " background-color: transparent;"
            "}"
            "QMenu::item:selected {"
            " background-color: #e3f2fd;"
            "}"
        );
    }

    if (isMyMessage) {
        QAction* editAction = contextMenu.addAction("Edit");
        QAction* deleteAction = contextMenu.addAction("Delete");
        connect(editAction, &QAction::triggered, this, &MainWindow::onEditMessage);
        connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteMessage);
        contextMenu.addSeparator();
    }

    QAction* replyAction = contextMenu.addAction("Reply");
    QAction* forwardAction = contextMenu.addAction("Forward");

    connect(replyAction, &QAction::triggered, this, &MainWindow::onReplyToMessage);
    connect(forwardAction, &QAction::triggered, this, &MainWindow::onForwardMessage);

    contextMenu.setProperty("messageId", messageId);
    contextMenu.setProperty("messageText", item->data(Qt::UserRole + 1).toString());

    contextMenu.exec(m_chatList->mapToGlobal(pos));
}

void MainWindow::onEditMessage() {
    QMenu* menu = qobject_cast<QMenu*>(sender()->parent());
    if (!menu) return;

    QString messageId = menu->property("messageId").toString();
    QString messageText = menu->property("messageText").toString();

    m_editingMessageId = messageId;
    m_editingOriginalText = messageText;

    QString preview = messageText.length() > 30 
                      ? messageText.left(30) + "..." 
                      : messageText;

    m_editLabel->setText("Editing: " + preview);
    m_editBar->show();
    m_messageInput->setText(messageText);
    m_messageInput->setFocus();
    m_messageInput->selectAll();
    m_sendBtn->setText("Save");
}

void MainWindow::onDeleteMessage() {
    QMenu* menu = qobject_cast<QMenu*>(sender()->parent());
    if (!menu) return;
    QString messageId = menu->property("messageId").toString();

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Delete Message");
    msgBox.setText("Are you sure you want to delete this message?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    
    if (m_isDarkMode) {
        msgBox.setStyleSheet(
            "QMessageBox { background-color: #2b2b2b; }"
            "QLabel { color: #ffffff; }"
            "QPushButton { "
            " background-color: #3b3b3b; "
            " color: #ffffff; "
            " border: 1px solid #555555; "
            " border-radius: 4px; "
            " padding: 6px 16px; "
            " min-width: 60px; "
            "}"
            "QPushButton:hover { background-color: #4a4a4a; }"
            "QPushButton:pressed { background-color: #333333; }"
        );
    } else {
        msgBox.setStyleSheet(
            "QMessageBox { background-color: #ffffff; }"
            "QLabel { color: #000000; }"
            "QPushButton { "
            " background-color: #f0f0f0; "
            " color: #000000; "
            " border: 1px solid #cccccc; "
            " border-radius: 4px; "
            " padding: 6px 16px; "
            " min-width: 60px; "
            "}"
            "QPushButton:hover { background-color: #e0e0e0; }"
            "QPushButton:pressed { background-color: #d0d0d0; }"
        );
    }

    if (msgBox.exec() == QMessageBox::Yes) {
        if (m_client && !m_currentChatId.isEmpty()) {
            m_client->deleteMessage(m_currentChatId, messageId);
        }
    }
}

void MainWindow::onReplyToMessage() {
    QMenu* menu = qobject_cast<QMenu*>(sender()->parent());
    if (!menu) return;
    QString messageId = menu->property("messageId").toString();
    QString messageText = menu->property("messageText").toString();

    if (!m_chats.contains(m_currentChatId)) return;
    
    Message* replyMsg = nullptr;
    for (auto& msg : m_chats[m_currentChatId].messages) {
        if (msg.messageId == messageId) {
            replyMsg = &msg;
            break;
        }
    }
    if (!replyMsg) return;

    m_replyingToMessageId = messageId;
    m_replyingToText = messageText;
    
    QString senderName = "Unknown";
    if (m_users.contains(replyMsg->senderId)) {
        senderName = m_users[replyMsg->senderId].username;
    }
    m_replyingToSender = senderName;

    QString preview = messageText.length() > 50 
                      ? messageText.left(50) + "..." 
                      : messageText;

    m_replyLabel->setText(senderName + ": " + preview);
    m_replyBar->show();
    m_messageInput->setFocus();
}

void MainWindow::onForwardMessage() {
    QMenu* menu = qobject_cast<QMenu*>(sender()->parent());
    if (!menu) return;
    QString messageId = menu->property("messageId").toString();
    qDebug() << "Forward message:" << messageId;
}

void MainWindow::onCancelEdit() {
    m_editingMessageId.clear();
    m_editingOriginalText.clear();
    m_editBar->hide();
    m_messageInput->clear();
    m_sendBtn->setText("Send");
}

void MainWindow::onCancelReply() {
    m_replyingToMessageId.clear();
    m_replyingToText.clear();
    m_replyingToSender.clear();
    m_replyBar->hide();
}

void MainWindow::onMessageUpdated(const QString& chatId, const QString& messageId, const QString& newContent, qint64 editedAt) {
    if (m_chats.contains(chatId)) {
        for (auto& msg : m_chats[chatId].messages) {
            if (msg.messageId == messageId) {
                msg.text = newContent;
                msg.editedAt = editedAt;
                break;
            }
        }
    }

    if (m_currentChatId == chatId) {
        for (int i = 0; i < m_chatList->count(); i++) {
            QListWidgetItem* item = m_chatList->item(i);
            if (item->data(Qt::UserRole + 6).toString() == messageId) {
                item->setData(Qt::UserRole + 1, newContent);
                item->setData(Qt::UserRole + 7, editedAt);
                QModelIndex modelIndex = m_chatList->model()->index(i, 0);
                m_chatList->update(m_chatList->visualRect(modelIndex));
                break;
            }
        }
    }
}

void MainWindow::onMessageDeleted(const QString& chatId, const QString& messageId) {
    if (m_chats.contains(chatId)) {
        auto& messages = m_chats[chatId].messages;
        messages.erase(std::remove_if(messages.begin(), messages.end(),
            [&messageId](const Message& m) { return m.messageId == messageId; }), messages.end());
    }

    if (m_currentChatId == chatId) {
        for (int i = 0; i < m_chatList->count(); i++) {
            QListWidgetItem* item = m_chatList->item(i);
            if (item->data(Qt::UserRole + 6).toString() == messageId) {
                delete m_chatList->takeItem(i);
                break;
            }
        }
    }
}

void MainWindow::onSendBtnClicked() {
    QString text = m_messageInput->text().trimmed();
    if (text.isEmpty() || m_currentChatId.isEmpty()) return;

    if (!m_editingMessageId.isEmpty()) {
        if (text == m_editingOriginalText) {
            onCancelEdit();
            return;
        }
        m_client->editMessage(m_currentChatId, m_editingMessageId, text);
        onCancelEdit();
        return;
    }

    QString tempId = "temp_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    Message pendingMsg;
    pendingMsg.messageId = tempId;
    pendingMsg.chatId = m_currentChatId;
    pendingMsg.senderId = m_client->currentUserId();
    pendingMsg.text = text;
    pendingMsg.timestamp = QDateTime::currentSecsSinceEpoch();
    pendingMsg.status = MessageStatus::Pending;
    
    if (!m_replyingToMessageId.isEmpty()) {
        pendingMsg.replyToId = m_replyingToMessageId;
    }

    m_pendingMessages[tempId] = pendingMsg;
    
    bool wasAtBottom = isScrolledToBottom();
    addMessageBubble(pendingMsg, false, false);
    
    if (wasAtBottom) {
        smoothScrollToBottom();
    }

    m_client->sendMessage(m_currentChatId, text, m_replyingToMessageId);
    
    m_messageInput->clear();
    if (!m_replyingToMessageId.isEmpty()) {
        onCancelReply();
    }
}

void MainWindow::focusOnMessage(const QString& messageId) {
    for (int i = 0; i < m_chatList->count(); i++) {
        QListWidgetItem* item = m_chatList->item(i);
        QString itemMsgId = item->data(Qt::UserRole + 6).toString();
        if (itemMsgId == messageId) {
            m_chatList->scrollToItem(item, QAbstractItemView::PositionAtCenter);
            
            m_highlightedMessageId = messageId;
            
            if (MessageDelegate* delegate = dynamic_cast<MessageDelegate*>(m_chatList->itemDelegate())) {
                delegate->setHighlightedMessage(messageId);
            }
            m_chatList->viewport()->update();
            
            QTimer::singleShot(3000, this, [this]() {
                m_highlightedMessageId.clear();
                if (MessageDelegate* delegate = dynamic_cast<MessageDelegate*>(m_chatList->itemDelegate())) {
                    delegate->setHighlightedMessage("");
                }
                m_chatList->viewport()->update();
            });
            break;
        }
    }
}

void MainWindow::onChatListItemClicked(const QModelIndex& index) {
    Q_UNUSED(index);
}


void MainWindow::setupTrayIcon()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }
    if (m_trayIcon) {
        return;
    }

    m_trayIcon = new QSystemTrayIcon(this);

    QIcon icon = windowIcon();
    if (icon.isNull()) {
        if (QIcon::hasThemeIcon("mail-message-new")) {
             icon = QIcon::fromTheme("mail-message-new");
        } else {
             QPixmap p(16,16);
             p.fill(Qt::red);
             icon = QIcon(p);
        }
    }
    m_trayIcon->setIcon(icon);
    m_trayIcon->setToolTip("Noveo Desktop");

    m_trayMenu = new QMenu(this);
    m_trayShowHideAction = m_trayMenu->addAction("Show/Hide");
    m_trayQuitAction = m_trayMenu->addAction("Quit");

    connect(m_trayShowHideAction, &QAction::triggered, this, &MainWindow::onTrayShowHide);
    connect(m_trayQuitAction, &QAction::triggered, this, &MainWindow::onTrayQuit);

    m_trayIcon->setContextMenu(m_trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);

    m_trayIcon->show();
}

void MainWindow::showNotificationForMessage(const Message& msg)
{
    if (!m_notificationsEnabled) {
        return;
    }

    if (!m_trayIcon || !m_trayIcon->isVisible()) {
        return;
    }

    if (msg.senderId == m_client->currentUserId()) {
        return;
    }

    bool isAppVisible = isVisible() && !isMinimized();

    // 1. If the user is actively using the app, suppress notifications.
    if (isAppVisible && isActiveWindow()) {
        return;
    }

    // 2. If the app is visible on screen (even if not focused) and the message
    //    is for the chat currently being viewed, suppress the notification.
    //    The user can see the message appearing in real-time.
    if (isAppVisible && msg.chatId == m_currentChatId) {
        return;
    }

    QString title = "New message";
    if (m_chats.contains(msg.chatId)) {
        title = resolveChatName(m_chats[msg.chatId]);
    }

    QString body = msg.text.trimmed();
    if (body.isEmpty()) {
        body = "New message";
    }
    if (body.size() > 100) {
        body = body.left(97) + "...";
    }

    m_trayIcon->showMessage(title, body, QSystemTrayIcon::Information, 5000);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (m_trayIcon && m_trayIcon->isVisible()) {
        hide();
        event->ignore();
        return;
    }

    QMainWindow::closeEvent(event);
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        onTrayShowHide();
    }
}

void MainWindow::onTrayShowHide()
{
    if (isVisible() && !isMinimized()) {
        hide();
    } else {
        showNormal();
        raise();
        activateWindow();
    }
}

void MainWindow::onTrayQuit()
{
    if (m_trayIcon) {
        m_trayIcon->hide();
    }
    qApp->quit();
}

void MainWindow::onNotificationsToggled(bool checked)
{
    m_notificationsEnabled = checked;
    QSettings settings("Noveo", "MessengerClient");
    settings.setValue("notificationsEnabled", m_notificationsEnabled);
}


