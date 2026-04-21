#ifndef CHATSETTINGSDIALOG_H
#define CHATSETTINGSDIALOG_H

#include <QDialog>
#include <QMap>

#include "DataStructures.h"

class QLabel;
class QPushButton;
class QVBoxLayout;
class QWidget;

class ChatSettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ChatSettingsDialog(QWidget* parent = nullptr);
    void setChatData(const Chat& chat, const QMap<QString, User>& users, const QString& currentUserId);
    void setDarkMode(bool enabled);
    QString chatId() const;

signals:
    void removeMemberRequested(const QString& chatId, const QString& memberId);
    void addMembersRequested(const QString& chatId);
    void removeHandleRequested(const QString& chatId);
    void deleteChatRequested(const QString& chatId);

private:
    void applyTheme();
    void clearMemberRows();
    QWidget* createMemberRow(const QString& userId, const QString& username, bool isOwner, bool canRemove);

    QString m_chatId;
    bool m_darkMode = false;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_chatNameLabel = nullptr;
    QWidget* m_membersHost = nullptr;
    QVBoxLayout* m_membersLayout = nullptr;
    QPushButton* m_addMembersButton = nullptr;
    QPushButton* m_removeHandleButton = nullptr;
    QPushButton* m_deleteChatButton = nullptr;
};

#endif // CHATSETTINGSDIALOG_H
