#include "MainWindow.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#include <QWebEngineView>

namespace {
constexpr auto kWebClientUrl = "https://web.noveo.ir/";
constexpr auto kVersion = "1.4.0.0";
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_version(QString::fromLatin1(kVersion))
{
    setupUi();
    loadClient();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("Noveo Desktop"));
    setMinimumSize(1200, 760);

    auto* central = new QWidget(this);
    auto* layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* shellSidebar = new QWidget(central);
    shellSidebar->setFixedWidth(74);
    shellSidebar->setStyleSheet(QStringLiteral("background:#0f172a;color:#cbd5e1;"));

    auto* sidebarLayout = new QVBoxLayout(shellSidebar);
    sidebarLayout->setContentsMargins(8, 16, 8, 12);
    sidebarLayout->setSpacing(10);

    auto* appBadge = new QLabel(QStringLiteral("N"), shellSidebar);
    appBadge->setAlignment(Qt::AlignCenter);
    appBadge->setFixedSize(40, 40);
    appBadge->setStyleSheet(QStringLiteral("background:#2563eb;color:white;border-radius:20px;font-weight:700;"));

    auto* homeBtn = new QPushButton(QStringLiteral("☰"), shellSidebar);
    homeBtn->setFixedHeight(38);
    homeBtn->setStyleSheet(QStringLiteral("QPushButton{background:transparent;color:#e2e8f0;border:none;font-size:18px;}"
                                         "QPushButton:hover{background:#1e293b;border-radius:10px;}"));
    connect(homeBtn, &QPushButton::clicked, this, [this]() {
        if (m_webView) {
            m_webView->setUrl(QUrl(QString::fromLatin1(kWebClientUrl)));
        }
    });

    m_statusLabel = new QLabel(QStringLiteral("Loading"), shellSidebar);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet(QStringLiteral("font-size:10px;color:#94a3b8;padding:4px;"));

    auto* versionLabel = new QLabel(QStringLiteral("v%1").arg(m_version), shellSidebar);
    versionLabel->setAlignment(Qt::AlignCenter);
    versionLabel->setStyleSheet(QStringLiteral("font-size:11px;color:#e2e8f0;padding:6px;"
                                               "background:#111827;border-radius:8px;"));

    sidebarLayout->addWidget(appBadge, 0, Qt::AlignHCenter);
    sidebarLayout->addWidget(homeBtn);
    sidebarLayout->addStretch();
    sidebarLayout->addWidget(m_statusLabel);
    sidebarLayout->addWidget(versionLabel);

    m_webView = new QWebEngineView(central);
    m_webView->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    m_webView->settings()->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    m_webView->settings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);

    connect(m_webView, &QWebEngineView::loadProgress, this, [this](int progress) {
        if (m_statusLabel) {
            m_statusLabel->setText(QStringLiteral("%1%").arg(progress));
        }
    });

    connect(m_webView, &QWebEngineView::loadFinished, this, [this](bool ok) {
        if (m_statusLabel) {
            m_statusLabel->setText(ok ? QStringLiteral("Online") : QStringLiteral("Offline"));
        }
        if (ok) {
            injectVersionIntoSidebar();
        }
    });

    layout->addWidget(shellSidebar);
    layout->addWidget(m_webView, 1);

    setCentralWidget(central);
}

void MainWindow::loadClient()
{
    if (!m_webView) {
        return;
    }
    m_webView->setUrl(QUrl(QString::fromLatin1(kWebClientUrl)));
}

void MainWindow::injectVersionIntoSidebar()
{
    if (!m_webView) {
        return;
    }

    const QString script = QStringLiteral(R"JS(
(() => {
  const version = 'v%1';
  const sidebarCandidates = [
    document.querySelector('#sidebar'),
    document.querySelector('.sidebar'),
    document.querySelector('aside'),
    document.querySelector('[class*="sidebar"]')
  ].filter(Boolean);

  if (!sidebarCandidates.length) {
    return;
  }

  const sidebar = sidebarCandidates[0];
  const old = document.querySelector('.noveocpp-version-badge');
  if (old) {
    old.remove();
  }

  const badge = document.createElement('div');
  badge.className = 'noveocpp-version-badge';
  badge.textContent = version;
  badge.style.cssText = 'margin:12px; padding:6px 10px; border-radius:8px; background:#111827; color:#e5e7eb; font-size:12px; text-align:center;';
  sidebar.appendChild(badge);
})();
)JS").arg(m_version);

    m_webView->page()->runJavaScript(script);
}
