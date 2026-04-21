#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QCheckBox;
class QComboBox;
class QPushButton;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    void setDarkMode(bool enabled);
    void setFollowSystem(bool enabled);
    void setCompactMode(bool enabled);
    void setThemePreset(const QString& preset);
    void setNotificationsEnabled(bool enabled);

signals:
    void darkModeChanged(bool enabled);
    void followSystemChanged(bool enabled);
    void compactModeChanged(bool enabled);
    void themePresetChanged(const QString& preset);
    void notificationsChanged(bool enabled);
    void logoutRequested();
    void developerConsoleRequested();

private:
    QCheckBox* m_darkModeCheck = nullptr;
    QCheckBox* m_followSystemCheck = nullptr;
    QCheckBox* m_compactModeCheck = nullptr;
    QCheckBox* m_notificationsCheck = nullptr;
    QComboBox* m_themePresetCombo = nullptr;
    QPushButton* m_logoutBtn = nullptr;
    QPushButton* m_devBtn = nullptr;
};

#endif // SETTINGSDIALOG_H
