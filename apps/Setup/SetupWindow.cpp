#include "SetupWindow.h"
#include "noty/common/Constants.h"
#include "noty/common/Logger.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QFontDatabase>
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressBar>
#include <QLineEdit>
#include <QListWidget>
#include <QCheckBox>
#include <QGroupBox>
#include <QTextEdit>
#include <QDir>
#include <QFileInfo>
#include <QStorageInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QThread>
#include <filesystem>

namespace fs = std::filesystem;

SetupWindow::SetupWindow(QWidget * parent)
    : QMainWindow(parent)
    , m_currentPage(WelcomePage)
    , m_isInstalling(false)
    , m_installationComplete(false)
    , m_packageSize(0)
{
    loadFonts();
    setupUi();
    showWelcomePage();
    noty::Logger::instance().info("SetupWindow initialized (Phase 9).");
}

SetupWindow::~SetupWindow() = default;

void SetupWindow::loadFonts()
{
    // Load Rubik fonts from resources
    QFontDatabase::addApplicationFont(":/fonts/Rubik-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Rubik-Medium.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Rubik-SemiBold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Rubik-Bold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Rubik-ExtraBold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Rubik-Black.ttf");

    // Set application font
    QFont defaultFont("Rubik", 10, QFont::Normal);
    qApp->setFont(defaultFont);
}

void SetupWindow::setupUi()
{
    setWindowTitle("NotY Game Installer");
    resize(900, 700);
    setMinimumSize(800, 600);

    // Central widget
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    auto* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Header with branding
    auto* headerWidget = new QWidget(this);
    headerWidget->setStyleSheet("background-color: #1a1a1a; padding: 12px 20px;");
    auto* headerLayout = new QHBoxLayout(headerWidget);

    auto* brandLabel = new QLabel("NotY Installer", this);
    brandLabel->setStyleSheet("color: #4CAF50; font-weight: bold; font-size: 16px; font-family: Rubik;");
    headerLayout->addWidget(brandLabel);

    headerLayout->addStretch();

    auto* versionLabel = new QLabel("v1.0.0", this);
    versionLabel->setStyleSheet("color: #666666; font-family: Rubik;");
    headerLayout->addWidget(versionLabel);

    mainLayout->addWidget(headerWidget);

    // Stacked widget for pages
    m_stackedWidget = new QStackedWidget(this);
    mainLayout->addWidget(m_stackedWidget);

    // Setup all pages
    setupWelcomePage();
    setupLocationPage();
    setupComponentsPage();
    setupInstallPage();
    setupVerificationPage();
    setupFinishPage();

    // Navigation buttons
    auto* navWidget = new QWidget(this);
    navWidget->setStyleSheet("background-color: #1a1a1a; padding: 10px 20px;");
    auto* navLayout = new QHBoxLayout(navWidget);

    m_backButton = new QPushButton("← Back", this);
    m_backButton->setEnabled(false);
    m_backButton->setStyleSheet(
        "QPushButton { background-color: #2a2a2a; color: #cccccc; padding: 8px 20px; border-radius: 4px; font-family: Rubik; }"
        "QPushButton:hover { background-color: #3a3a3a; }"
        "QPushButton:disabled { color: #444444; }"
    );
    connect(m_backButton, &QPushButton::clicked, this, &SetupWindow::onBackClicked);
    navLayout->addWidget(m_backButton);

    navLayout->addStretch();

    m_cancelButton = new QPushButton("Cancel", this);
    m_cancelButton->setStyleSheet(
        "QPushButton { background-color: #2a2a2a; color: #cccccc; padding: 8px 20px; border-radius: 4px; font-family: Rubik; }"
        "QPushButton:hover { background-color: #3a3a3a; }"
    );
    connect(m_cancelButton, &QPushButton::clicked, this, &SetupWindow::onCancelClicked);
    navLayout->addWidget(m_cancelButton);

    m_nextButton = new QPushButton("Continue →", this);
    m_nextButton->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: #ffffff; padding: 8px 20px; border-radius: 4px; font-weight: bold; font-family: Rubik; }"
        "QPushButton:hover { background-color: #45a049; }"
        "QPushButton:disabled { background-color: #2a2a2a; color: #444444; }"
    );
    connect(m_nextButton, &QPushButton::clicked, this, &SetupWindow::onContinueClicked);
    navLayout->addWidget(m_nextButton);

    mainLayout->addWidget(navWidget);

    showPage(WelcomePage);
}

void SetupWindow::setupWelcomePage()
{
    m_welcomePage = new QWidget(this);
    auto* layout = new QVBoxLayout(m_welcomePage);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(25);
    layout->setContentsMargins(60, 60, 60, 60);

    // Logo
    auto* logoLabel = new QLabel(this);
    logoLabel->setText("🎮");
    logoLabel->setStyleSheet("font-size: 72px;");
    logoLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(logoLabel);

    // Title
    auto* title = new QLabel("Welcome to the NotY Game Installer", this);
    title->setAlignment(Qt::AlignCenter);
    QFont titleFont("Rubik", 28, QFont::ExtraBold);
    title->setFont(titleFont);
    title->setStyleSheet("color: #ffffff;");
    layout->addWidget(title);

    // Game name
    auto* gameNameLabel = new QLabel(m_gameName.isEmpty() ? "Game Package" : m_gameName, this);
    gameNameLabel->setAlignment(Qt::AlignCenter);
    QFont gameFont("Rubik", 18, QFont::DemiBold);
    gameNameLabel->setFont(gameFont);
    gameNameLabel->setStyleSheet("color: #4CAF50;");
    layout->addWidget(gameNameLabel);

    // Repacker info
    auto* repackerLabel = new QLabel(
        QString("Repacked by %1").arg(m_repackerName.isEmpty() ? "NotY215" : m_repackerName),
        this);
    repackerLabel->setAlignment(Qt::AlignCenter);
    repackerLabel->setStyleSheet("color: #888888; font-size: 12px; font-family: Rubik;");
    layout->addWidget(repackerLabel);

    layout->addSpacing(20);

    // Description
    auto* desc = new QLabel(
        "This wizard will guide you through the installation of the game package.\n\n"
        "Click Continue to proceed.",
        this);
    desc->setAlignment(Qt::AlignCenter);
    desc->setWordWrap(true);
    desc->setStyleSheet("color: #aaaaaa; font-size: 12px; font-family: Rubik;");
    layout->addWidget(desc);

    layout->addStretch();

    m_stackedWidget->addWidget(m_welcomePage);
}

void SetupWindow::setupLocationPage()
{
    m_locationPage = new QWidget(this);
    auto* layout = new QVBoxLayout(m_locationPage);
    layout->setSpacing(20);
    layout->setContentsMargins(50, 40, 50, 40);

    // Title
    auto* title = new QLabel("Installation Location", this);
    QFont titleFont("Rubik", 20, QFont::DemiBold);
    title->setFont(titleFont);
    title->setStyleSheet("color: #ffffff;");
    layout->addWidget(title);

    // Description
    auto* desc = new QLabel("Choose the directory where the game will be installed.", this);
    desc->setWordWrap(true);
    desc->setStyleSheet("color: #aaaaaa; font-family: Rubik;");
    layout->addWidget(desc);

    layout->addSpacing(20);

    // Directory selection
    auto* dirLabel = new QLabel("Installation directory:", this);
    dirLabel->setStyleSheet("color: #cccccc; font-size: 11px; font-weight: 500; font-family: Rubik;");
    layout->addWidget(dirLabel);

    auto* selectionLayout = new QHBoxLayout();
    m_installDirEdit = new QLineEdit(this);
    m_installDirEdit->setText(QDir::homePath() + "/Games");
    m_installDirEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #333333; padding: 8px; background: #1a1a1a; border-radius: 4px; color: #ffffff; font-family: Rubik; }"
        "QLineEdit:focus { border-color: #4CAF50; }"
    );
    selectionLayout->addWidget(m_installDirEdit);

    auto* browseBtn = new QPushButton("Browse...", this);
    browseBtn->setStyleSheet(
        "QPushButton { background-color: #2a2a2a; color: #cccccc; padding: 8px 16px; border-radius: 4px; font-family: Rubik; }"
        "QPushButton:hover { background-color: #3a3a3a; }"
    );
    connect(browseBtn, &QPushButton::clicked, this, &SetupWindow::onBrowseClicked);
    selectionLayout->addWidget(browseBtn);

    layout->addLayout(selectionLayout);

    layout->addSpacing(10);

    // Disk space info
    auto* spaceGroup = new QGroupBox("Disk Space", this);
    spaceGroup->setStyleSheet(
        "QGroupBox { border: 1px solid #333333; border-radius: 4px; margin-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; color: #aaaaaa; font-family: Rubik; }"
    );
    auto* spaceLayout = new QVBoxLayout(spaceGroup);

    m_diskSpaceLabel = new QLabel("Required: --  Available: --", this);
    m_diskSpaceLabel->setStyleSheet("color: #ffffff; font-family: Rubik;");
    spaceLayout->addWidget(m_diskSpaceLabel);

    layout->addWidget(spaceGroup);

    layout->addStretch();

    m_stackedWidget->addWidget(m_locationPage);
}

void SetupWindow::setupComponentsPage()
{
    m_componentsPage = new QWidget(this);
    auto* layout = new QVBoxLayout(m_componentsPage);
    layout->setSpacing(20);
    layout->setContentsMargins(50, 40, 50, 40);

    // Title
    auto* title = new QLabel("Select Components", this);
    QFont titleFont("Rubik", 20, QFont::DemiBold);
    title->setFont(titleFont);
    title->setStyleSheet("color: #ffffff;");
    layout->addWidget(title);

    // Description
    auto* desc = new QLabel("Choose which components you want to install.", this);
    desc->setWordWrap(true);
    desc->setStyleSheet("color: #aaaaaa; font-family: Rubik;");
    layout->addWidget(desc);

    layout->addSpacing(20);

    // Components list
    m_componentsList = new QListWidget(this);
    m_componentsList->setStyleSheet(
        "QListWidget { background-color: #1a1a1a; border: 1px solid #333333; border-radius: 4px; }"
        "QListWidget::item { padding: 10px; color: #ffffff; font-family: Rubik; }"
        "QListWidget::item:selected { background-color: #2a2a2a; }"
        "QListWidget::item:hover { background-color: #252525; }"
    );
    m_componentsList->setSelectionMode(QAbstractItemView::MultiSelection);
    connect(m_componentsList, &QListWidget::itemSelectionChanged,
        this, &SetupWindow::onComponentSelectionChanged);
    layout->addWidget(m_componentsList);

    // Required size
    m_requiredSizeLabel = new QLabel("Total size: --", this);
    m_requiredSizeLabel->setStyleSheet("color: #888888; font-family: Rubik;");
    layout->addWidget(m_requiredSizeLabel);

    layout->addStretch();

    m_stackedWidget->addWidget(m_componentsPage);
}

void SetupWindow::setupInstallPage()
{
    m_installPage = new QWidget(this);
    auto* layout = new QVBoxLayout(m_installPage);
    layout->setSpacing(20);
    layout->setContentsMargins(50, 40, 50, 40);

    // Title
    auto* title = new QLabel("Installing...", this);
    QFont titleFont("Rubik", 20, QFont::DemiBold);
    title->setFont(titleFont);
    title->setStyleSheet("color: #ffffff;");
    layout->addWidget(title);

    // Status
    m_statusLabel = new QLabel("Preparing to install...", this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet("color: #aaaaaa; font-family: Rubik;");
    layout->addWidget(m_statusLabel);

    layout->addSpacing(20);

    // Progress bar
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setStyleSheet(
        "QProgressBar { border: 1px solid #333333; border-radius: 4px; background-color: #1a1a1a; height: 25px; }"
        "QProgressBar::chunk { background-color: #4CAF50; border-radius: 3px; }"
    );
    layout->addWidget(m_progressBar);

    // File progress
    m_fileProgressLabel = new QLabel("", this);
    m_fileProgressLabel->setStyleSheet("color: #888888; font-family: Rubik;");
    layout->addWidget(m_fileProgressLabel);

    layout->addStretch();

    // Install button
    auto* installBtn = new QPushButton("Install", this);
    installBtn->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: #ffffff; padding: 12px 32px; border-radius: 4px; font-weight: bold; font-size: 14px; font-family: Rubik; }"
        "QPushButton:hover { background-color: #45a049; }"
    );
    connect(installBtn, &QPushButton::clicked, this, &SetupWindow::onInstallClicked);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(installBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    m_stackedWidget->addWidget(m_installPage);
}

void SetupWindow::setupVerificationPage()
{
    m_verificationPage = new QWidget(this);
    auto* layout = new QVBoxLayout(m_verificationPage);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(25);
    layout->setContentsMargins(60, 60, 60, 60);

    // Icon
    auto* iconLabel = new QLabel("✓", this);
    iconLabel->setStyleSheet("color: #4CAF50; font-size: 64px; font-family: Rubik;");
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);

    // Title
    auto* title = new QLabel("Verifying Installation...", this);
    title->setAlignment(Qt::AlignCenter);
    QFont titleFont("Rubik", 20, QFont::DemiBold);
    title->setFont(titleFont);
    title->setStyleSheet("color: #ffffff;");
    layout->addWidget(title);

    // Status
    auto* status = new QLabel("Checking file integrity...", this);
    status->setAlignment(Qt::AlignCenter);
    status->setWordWrap(true);
    status->setStyleSheet("color: #aaaaaa; font-family: Rubik;");
    layout->addWidget(status);

    layout->addStretch();

    // Verification progress
    m_verificationProgress = new QProgressBar(this);
    m_verificationProgress->setRange(0, 100);
    m_verificationProgress->setValue(0);
    m_verificationProgress->setStyleSheet(
        "QProgressBar { border: 1px solid #333333; border-radius: 4px; background-color: #1a1a1a; height: 20px; }"
        "QProgressBar::chunk { background-color: #4CAF50; border-radius: 3px; }"
    );
    layout->addWidget(m_verificationProgress);

    layout->addStretch();

    m_stackedWidget->addWidget(m_verificationPage);
}

void SetupWindow::setupFinishPage()
{
    m_finishPage = new QWidget(this);
    auto* layout = new QVBoxLayout(m_finishPage);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(20);
    layout->setContentsMargins(60, 60, 60, 60);

    // Complete icon
    m_completeIcon = new QLabel("✓", this);
    m_completeIcon->setStyleSheet("color: #4CAF50; font-size: 64px; font-family: Rubik;");
    m_completeIcon->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_completeIcon);

    // Complete message
    m_completeMessage = new QLabel("Installation Complete!", this);
    m_completeMessage->setAlignment(Qt::AlignCenter);
    QFont msgFont("Rubik", 24, QFont::Bold);
    m_completeMessage->setFont(msgFont);
    m_completeMessage->setStyleSheet("color: #ffffff;");
    layout->addWidget(m_completeMessage);

    // Details
    auto* details = new QLabel(
        "The game has been successfully installed.\n"
        "You can now launch the game from the Start Menu or desktop shortcut.",
        this);
    details->setAlignment(Qt::AlignCenter);
    details->setWordWrap(true);
    details->setStyleSheet("color: #aaaaaa; font-family: Rubik;");
    layout->addWidget(details);

    layout->addSpacing(20);

    // Launch checkbox
    m_launchCheckbox = new QCheckBox("Launch game when finished", this);
    m_launchCheckbox->setChecked(true);
    m_launchCheckbox->setStyleSheet("color: #aaaaaa; font-family: Rubik;");
    layout->addWidget(m_launchCheckbox);

    layout->addStretch();

    // Finish button
    auto* finishBtn = new QPushButton("Finish", this);
    finishBtn->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: #ffffff; padding: 12px 32px; border-radius: 4px; font-weight: bold; font-size: 14px; font-family: Rubik; }"
        "QPushButton:hover { background-color: #45a049; }"
    );
    connect(finishBtn, &QPushButton::clicked, this, &SetupWindow::onFinishClicked);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(finishBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    m_stackedWidget->addWidget(m_finishPage);
}

void SetupWindow::showPage(int index)
{
    m_stackedWidget->setCurrentIndex(index);
    updateNavigationButtons();
}

void SetupWindow::updateNavigationButtons()
{
    if (m_isInstalling) {
        m_backButton->setEnabled(false);
        m_nextButton->setEnabled(false);
        m_cancelButton->setText("Cancel");
        return;
    }

    if (m_installationComplete) {
        m_backButton->setEnabled(false);
        m_nextButton->setEnabled(false);
        m_cancelButton->setEnabled(false);
        return;
    }

    if (m_currentPage == WelcomePage) {
        m_backButton->setEnabled(false);
        m_nextButton->setText("Continue →");
        m_nextButton->setEnabled(true);
        m_cancelButton->setText("Cancel");
    }
    else if (m_currentPage == FinishPage) {
        m_backButton->setEnabled(true);
        m_nextButton->setText("Finish");
        m_nextButton->setEnabled(true);
        m_cancelButton->setText("Close");
    }
    else if (m_currentPage == InstallPage) {
        m_backButton->setEnabled(false);
        m_nextButton->setEnabled(false);
        m_cancelButton->setText("Cancel");
    }
    else if (m_currentPage == VerificationPage) {
        m_backButton->setEnabled(false);
        m_nextButton->setEnabled(false);
        m_cancelButton->setText("Cancel");
    }
    else {
        m_backButton->setEnabled(true);
        m_nextButton->setText("Continue →");
        m_nextButton->setEnabled(true);
        m_cancelButton->setText("Cancel");
    }
}

void SetupWindow::showWelcomePage()
{
    m_currentPage = WelcomePage;
    showPage(WelcomePage);
}

void SetupWindow::showLocationPage()
{
    m_currentPage = LocationPage;
    updateDiskSpaceInfo();
    showPage(LocationPage);
}

void SetupWindow::showComponentsPage()
{
    m_currentPage = ComponentsPage;
    updateComponentsList();
    showPage(ComponentsPage);
}

void SetupWindow::showInstallPage()
{
    m_currentPage = InstallPage;
    m_progressBar->setValue(0);
    m_statusLabel->setText("Ready to install...");
    m_fileProgressLabel->setText("");
    showPage(InstallPage);
}

void SetupWindow::showVerificationPage()
{
    m_currentPage = VerificationPage;
    m_verificationProgress->setValue(0);
    showPage(VerificationPage);
}

void SetupWindow::showFinishPage()
{
    m_currentPage = FinishPage;
    m_installationComplete = true;
    showPage(FinishPage);
}

void SetupWindow::updateProgress(int percent, const QString & status)
{
    m_progressBar->setValue(percent);
    m_statusLabel->setText(status);
}

void SetupWindow::updateFileProgress(const QString & filename, int percent)
{
    m_fileProgressLabel->setText(QString("Processing: %1 (%2%)").arg(filename).arg(percent));
}

void SetupWindow::updateVerificationProgress(int percent, const QString & status)
{
    m_verificationProgress->setValue(percent);
    if (!status.isEmpty()) {
        QLabel* statusLabel = m_verificationPage->findChild<QLabel*>("statusLabel");
        if (statusLabel) {
            statusLabel->setText(status);
        }
    }
}

void SetupWindow::updateDiskSpaceInfo()
{
    QString installDir = m_installDirEdit->text();
    if (installDir.isEmpty()) {
        return;
    }

    // Calculate required size
    QString sizeStr;
    if (m_packageSize > 1024 * 1024 * 1024) {
        sizeStr = QString("%1 GB").arg(m_packageSize / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    }
    else if (m_packageSize > 1024 * 1024) {
        sizeStr = QString("%1 MB").arg(m_packageSize / (1024.0 * 1024.0), 0, 'f', 2);
    }
    else {
        sizeStr = QString("%1 KB").arg(m_packageSize / 1024.0, 0, 'f', 2);
    }

    // Check disk space
    QStorageInfo storage(installDir);
    if (storage.isValid() && storage.isReady()) {
        qint64 available = storage.bytesAvailable();
        QString availableStr;
        if (available > 1024 * 1024 * 1024) {
            availableStr = QString("%1 GB").arg(available / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
        }
        else if (available > 1024 * 1024) {
            availableStr = QString("%1 MB").arg(available / (1024.0 * 1024.0), 0, 'f', 2);
        }
        else {
            availableStr = QString("%1 KB").arg(available / 1024.0, 0, 'f', 2);
        }

        bool hasSpace = available >= static_cast<qint64>(m_packageSize);
        QString color = hasSpace ? "#4CAF50" : "#dc3545";
        m_diskSpaceLabel->setText(
            QString("Required: <span style='color:#ffffff;'>%1</span>  Available: <span style='color:%2;'>%3</span>")
            .arg(sizeStr).arg(color).arg(availableStr));
    }
    else {
        m_diskSpaceLabel->setText("Required: " + sizeStr + "  Available: Unknown");
    }
}

void SetupWindow::updateComponentsList()
{
    // Clear existing items
    m_componentsList->clear();

    // Add components from manifest
    // For now, add some example components
    QList<QListWidgetItem*> items;

    auto* item1 = new QListWidgetItem("Game Files (Required)");
    item1->setCheckState(Qt::Checked);
    item1->setFlags(item1->flags() & ~Qt::ItemIsSelectable);
    m_componentsList->addItem(item1);

    auto* item2 = new QListWidgetItem("Additional Content");
    item2->setCheckState(Qt::Checked);
    m_componentsList->addItem(item2);

    auto* item3 = new QListWidgetItem("Documentation");
    item3->setCheckState(Qt::Checked);
    m_componentsList->addItem(item3);

    m_componentsList->setCurrentRow(0);
}

void SetupWindow::updateComponentSizes()
{
    // Calculate and display total size of selected components
    // For now, just show the package size
    QString sizeStr;
    if (m_packageSize > 1024 * 1024 * 1024) {
        sizeStr = QString("%1 GB").arg(m_packageSize / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    }
    else if (m_packageSize > 1024 * 1024) {
        sizeStr = QString("%1 MB").arg(m_packageSize / (1024.0 * 1024.0), 0, 'f', 2);
    }
    else {
        sizeStr = QString("%1 KB").arg(m_packageSize / 1024.0, 0, 'f', 2);
    }
    m_requiredSizeLabel->setText("Total size: " + sizeStr);
}

void SetupWindow::setPackageInfo(const QString & gameName, const QString & version,
    const QString & repacker, uint64_t size)
{
    m_gameName = gameName;
    m_gameVersion = version;
    m_repackerName = repacker;
    m_packageSize = size;

    // Update welcome page
    QLabel* gameLabel = m_welcomePage->findChild<QLabel*>("gameNameLabel");
    if (gameLabel) {
        gameLabel->setText(gameName);
    }
    QLabel* repackerLabel = m_welcomePage->findChild<QLabel*>("repackerLabel");
    if (repackerLabel) {
        repackerLabel->setText("Repacked by " + repacker);
    }

    // Set default install directory
    QString defaultDir = QDir::homePath() + "/Games/" + gameName;
    if (m_installDirEdit) {
        m_installDirEdit->setText(defaultDir);
    }

    updateDiskSpaceInfo();
}

void SetupWindow::onContinueClicked()
{
    if (m_isInstalling) {
        return;
    }

    if (m_currentPage == FinishPage) {
        onFinishClicked();
        return;
    }

    if (m_currentPage == WelcomePage) {
        showLocationPage();
        return;
    }

    if (m_currentPage == LocationPage) {
        // Validate installation directory
        QString installDir = m_installDirEdit->text();
        if (installDir.isEmpty()) {
            QMessageBox::warning(this, "Invalid Location",
                "Please select a valid installation directory.");
            return;
        }

        QFileInfo dirInfo(installDir);
        if (!dirInfo.exists()) {
            // Directory doesn't exist, will be created
            QDir dir;
            if (!dir.mkpath(installDir)) {
                QMessageBox::warning(this, "Invalid Location",
                    "Cannot create installation directory.\n"
                    "Please check your permissions.");
                return;
            }
        }

        // Check disk space
        QStorageInfo storage(installDir);
        if (storage.isValid() && storage.isReady()) {
            if (storage.bytesAvailable() < static_cast<qint64>(m_packageSize)) {
                QMessageBox::warning(this, "Insufficient Disk Space",
                    "Not enough disk space available.\n"
                    "Required: " + QString::number(m_packageSize / (1024 * 1024)) + " MB");
                return;
            }
        }

        showComponentsPage();
        return;
    }

    if (m_currentPage == ComponentsPage) {
        // Collect selected components
        showInstallPage();
        return;
    }

    if (m_currentPage == InstallPage) {
        // Should not happen - Install button should be used
        return;
    }

    if (m_currentPage == VerificationPage) {
        showFinishPage();
        return;
    }

    // Default: go to next page
    if (m_currentPage < FinishPage) {
        m_currentPage++;
        showPage(m_currentPage);
    }
}

void SetupWindow::onBackClicked()
{
    if (m_isInstalling) {
        return;
    }

    if (m_currentPage > 0 && m_currentPage != InstallPage && m_currentPage != VerificationPage) {
        m_currentPage--;
        showPage(m_currentPage);
    }
}

void SetupWindow::onCancelClicked()
{
    if (m_isInstalling) {
        // Cancel installation
        int reply = QMessageBox::question(this, "Cancel Installation",
            "Are you sure you want to cancel the installation?",
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            emit installationCancelled();
        }
        return;
    }

    int reply = QMessageBox::question(this, "Exit Installer",
        "Are you sure you want to exit the installer?",
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        close();
    }
}

void SetupWindow::onInstallClicked()
{
    if (m_isInstalling) {
        return;
    }

    // Validate installation directory again
    QString installDir = m_installDirEdit->text();
    QStorageInfo storage(installDir);
    if (storage.isValid() && storage.isReady()) {
        if (storage.bytesAvailable() < static_cast<qint64>(m_packageSize)) {
            QMessageBox::warning(this, "Insufficient Disk Space",
                "Not enough disk space available.\n"
                "Required: " + QString::number(m_packageSize / (1024 * 1024)) + " MB");
            return;
        }
    }

    m_isInstalling = true;
    m_installationComplete = false;
    updateNavigationButtons();

    // Show install page
    showInstallPage();

    // Emit signal to start installation
    emit installationStarted();
}

void SetupWindow::onBrowseClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this,
        "Select Installation Directory",
        m_installDirEdit->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        m_installDirEdit->setText(dir);
        updateDiskSpaceInfo();
    }
}

void SetupWindow::onFinishClicked()
{
    if (m_launchCheckbox->isChecked()) {
        // Try to launch the game
        QString gameExe = m_installDirEdit->text() + "/" + m_gameName + ".exe";
        if (QFile::exists(gameExe)) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(gameExe));
        }
        else {
            // Try alternative locations
            QStringList alternatives;
            alternatives << "/Game.exe" << "/Launcher.exe" << "/" + m_gameName + ".exe";
            for (const QString& alt : alternatives) {
                QString path = m_installDirEdit->text() + alt;
                if (QFile::exists(path)) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
                    break;
                }
            }
        }
    }

    close();
}

void SetupWindow::onComponentSelectionChanged()
{
    // Calculate size of selected components
    updateComponentSizes();
}

void SetupWindow::setInstallationComplete(bool success, const QString & message)
{
    m_isInstalling = false;

    if (success) {
        showFinishPage();
        m_completeMessage->setText("Installation Complete!");
        m_completeIcon->setStyleSheet("color: #4CAF50; font-size: 64px; font-family: Rubik;");
        m_completeMessage->setStyleSheet("color: #ffffff; font-size: 24px; font-weight: 700; font-family: Rubik;");

        // Update completion message
        QLabel* details = m_finishPage->findChild<QLabel*>("detailsLabel");
        if (details) {
            details->setText("The game has been successfully installed.\n"
                "You can now launch the game from the Start Menu or desktop shortcut.");
        }
    }
    else {
        // Show error
        m_completeMessage->setText("Installation Failed");
        m_completeIcon->setStyleSheet("color: #dc3545; font-size: 64px; font-family: Rubik;");
        m_completeMessage->setStyleSheet("color: #dc3545; font-size: 24px; font-weight: 700; font-family: Rubik;");

        QLabel* details = m_finishPage->findChild<QLabel*>("detailsLabel");
        if (details) {
            details->setText("The installation encountered an error:\n" + message);
        }

        m_launchCheckbox->setVisible(false);
        showFinishPage();

        QMessageBox::critical(this, "Installation Failed", message);
    }

    updateNavigationButtons();
}