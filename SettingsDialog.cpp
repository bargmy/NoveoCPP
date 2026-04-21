#include "SettingsDialog.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QHBoxLayout>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Settings");
    setModal(true);
    resize(460, 420);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(12);

    auto* title = new QLabel("Settings");
    title->setStyleSheet("font-size:20px;font-weight:700;");
    root->addWidget(title);

    m_darkModeCheck = new QCheckBox("Dark mode");
    m_followSystemCheck = new QCheckBox("Follow system appearance");
    m_compactModeCheck = new QCheckBox("Compact density");
    m_notificationsCheck = new QCheckBox("Notifications");
    m_themePresetCombo = new QComboBox();
    m_themePresetCombo->addItems({ "Default", "Midnight", "Sunset" });

    root->addWidget(m_darkModeCheck);
    root->addWidget(m_followSystemCheck);
    root->addWidget(m_compactModeCheck);
    root->addWidget(new QLabel("Theme preset"));
    root->addWidget(m_themePresetCombo);
    root->addWidget(m_notificationsCheck);
    root->addStretch();

    auto* buttons = new QHBoxLayout();
    m_devBtn = new QPushButton("Developer Console");
    m_logoutBtn = new QPushButton("Log Out");
    buttons->addWidget(m_devBtn);
    buttons->addStretch();
    buttons->addWidget(m_logoutBtn);
    root->addLayout(buttons);

    connect(m_darkModeCheck, &QCheckBox::toggled, this, &SettingsDialog::darkModeChanged);
    connect(m_followSystemCheck, &QCheckBox::toggled, this, &SettingsDialog::followSystemChanged);
    connect(m_compactModeCheck, &QCheckBox::toggled, this, &SettingsDialog::compactModeChanged);
    connect(m_themePresetCombo, &QComboBox::currentTextChanged, this, &SettingsDialog::themePresetChanged);
    connect(m_notificationsCheck, &QCheckBox::toggled, this, &SettingsDialog::notificationsChanged);
    connect(m_logoutBtn, &QPushButton::clicked, this, &SettingsDialog::logoutRequested);
    connect(m_devBtn, &QPushButton::clicked, this, &SettingsDialog::developerConsoleRequested);
}

void SettingsDialog::setDarkMode(bool enabled) { m_darkModeCheck->setChecked(enabled); }
void SettingsDialog::setFollowSystem(bool enabled) { m_followSystemCheck->setChecked(enabled); }
void SettingsDialog::setCompactMode(bool enabled) { m_compactModeCheck->setChecked(enabled); }
void SettingsDialog::setThemePreset(const QString& preset) {
    int i = m_themePresetCombo->findText(preset);
    m_themePresetCombo->setCurrentIndex(i >= 0 ? i : 0);
}
void SettingsDialog::setNotificationsEnabled(bool enabled) { m_notificationsCheck->setChecked(enabled); }
