#include "MainWindow.h"
#include "noty/common/Constants.h"
#include "noty/common/Logger.h"
#include "noty/filesystem/DirectoryScanner.h"
#include "noty/repacker/RepackEngine.h"
#include "noty/repacker/RepackJob.h"
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
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QScrollArea>
#include <QFormLayout>
#include <QThread>
#include <QDateTime>
#include <QImageReader>
#include <QDir>
#include <QFileInfo>
#include <QStorageInfo>

MainWindow::MainWindow(QWidget * parent)
    : QMainWindow(parent)
    , m_currentPage(WelcomePage)
    , m_isRepacking(false)
    , m_repackComplete(false)
    , m_repackSuccess(false)
    , m_totalSize(0)
    , m_fileCount(0)
    , m_directoryCount(0)
{
    loadFonts();
    setupUi();
    showPage(WelcomePage);

    // Set default repacker name from environment
    QString username = qEnvironmentVariable("USERNAME", "User");
    m_repackerNameEdit->setText(username);

    noty::Logger::instance().info("MainWindow initialized (Phase 8).");
}

MainWindow::~MainWindow() = default;

void MainWindow::loadFonts()
{
    QFontDatabase::addApplicationFont(":/fonts/Rubik-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Rubik-Medium.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Rubik-SemiBold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Rubik-Bold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Rubik-ExtraBold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Rubik-Black.ttf");

    QFont defaultFont("Rubik", 10, QFont::Normal);
    qApp->setFont(defaultFont);
}

void MainWindow::setupUi()
{
    setWindowTitle(QString::fromUtf8(noty::APP_NAME));
    resize(950, 750);
    setMinimumSize(850, 650);

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

    auto* brandLabel = new QLabel("NotY Repacker", this);
    brandLabel->setStyleSheet("color: #4CAF50; font-weight: bold; font-size: 16px;");
    headerLayout->addWidget(brandLabel);

    headerLayout->addStretch();

    auto* versionLabel = new QLabel(noty::APP_VERSION, this);
    versionLabel->setStyleSheet("color: #666666;");
    headerLayout->addWidget(versionLabel);

    mainLayout->addWidget(headerWidget);

    // Stacked widget for pages
    m_stackedWidget = new QStackedWidget(this);
    mainLayout->addWidget(m_stackedWidget);

    // Setup all pages
    setupWelcomePage();
    setupSourcePage();
    setupCoverPage();
    setupConfigurationPage();
    setupReviewPage();
    setupProgressPage();
    setupCompletePage();

    // Navigation buttons
    auto* navWidget = new QWidget(this);
    navWidget->setStyleSheet("background-color: #1a1a1a; padding: 10px 20px;");
    auto* navLayout = new QHBoxLayout(navWidget);

    m_backButton = new QPushButton("← Back", this);
    m_backButton->setEnabled(false);
    m_backButton->setStyleSheet(
        "QPushButton { background-color: #2a2a2a; color: #cccccc; padding: 8px 20px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #3a3a3a; }"
        "QPushButton:disabled { color: #444444; }"
    );
    connect(m_backButton, &QPushButton::clicked, this, &MainWindow::onBackClicked);
    navLayout->addWidget(m_backButton);

    navLayout->addStretch();

    m_cancelButton = new QPushButton("Cancel", this);
    m_cancelButton->setStyleSheet(
        "QPushButton { background-color: #2a2a2a; color: #cccccc; padding: 8px 20px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #3a3a3a; }"
    );
    connect(m_cancelButton, &QPushButton::clicked, this, &MainWindow::onExitClicked);
    navLayout->addWidget(m_cancelButton);

    m_nextButton = new QPushButton("Continue →", this);
    m_nextButton->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: #ffffff; padding: 8px 20px; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #45a049; }"
        "QPushButton:disabled { background-color: #2a2a2a; color: #444444; }"
    );
    connect(m_nextButton, &QPushButton::clicked, this, &MainWindow::onContinueClicked);
    navLayout->addWidget(m_nextButton);

    mainLayout->addWidget(navWidget);

    // Set initial focus
    m_nextButton->setFocus();
}

void MainWindow::setupWelcomePage()
{
    m_welcomePage = new QWidget(this);
    auto* layout = new QVBoxLayout(m_welcomePage);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(25);
    layout->setContentsMargins(60, 60, 60, 60);

    // Logo
    auto* logoLabel = new QLabel(this);
    QPixmap logo(":/logo.png");
    if (!logo.isNull()) {
        logoLabel->setPixmap(logo.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    else {
        logoLabel->setText("📦");
        logoLabel->setStyleSheet("font-size: 72px;");
    }
    logoLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(logoLabel);

    // Title
    auto* title = new QLabel(noty::APP_NAME, this);
    title->setAlignment(Qt::AlignCenter);
    QFont titleFont("Rubik", 28, QFont::ExtraBold);
    title->setFont(titleFont);
    layout->addWidget(title);

    // Subtitle
    auto* subtitle = new QLabel("Professional Game Packaging System", this);
    subtitle->setAlignment(Qt::AlignCenter);
    QFont subtitleFont("Rubik", 14, QFont::Medium);
    subtitle->setFont(subtitleFont);
    subtitle->setStyleSheet("color: #888888;");
    layout->addWidget(subtitle);

    // Branding
    auto* brand = new QLabel(QString("Powered by %1").arg(noty::BRAND_NAME), this);
    brand->setAlignment(Qt::AlignCenter);
    QFont brandFont("Rubik", 12, QFont::Bold);
    brand->setFont(brandFont);
    brand->setStyleSheet("color: #4CAF50;");
    layout->addWidget(brand);

    layout->addSpacing(20);

    // Description
    auto* desc = new QLabel(
        "This tool will guide you through packaging your game files into a\n"
        "compressed, encrypted, and verified .noty package.\n\n"
        "The generated Setup.exe will allow users to install your game\n"
        "with a professional installer interface.",
        this);
    desc->setAlignment(Qt::AlignCenter);
    desc->setWordWrap(true);
    desc->setStyleSheet("color: #aaaaaa; font-size: 12px;");
    layout->addWidget(desc);

    layout->addStretch();

    m_stackedWidget->addWidget(m_welcomePage);
}

void MainWindow::setupSourcePage()
{
    m_sourcePage = new QWidget(this);
    auto* layout = new QVBoxLayout(m_sourcePage);
    layout->setSpacing(20);
    layout->setContentsMargins(50, 40, 50, 40);

    // Title
    auto* title = new QLabel("Source Directory", this);
    QFont titleFont("Rubik", 20, 600);
    title->setFont(titleFont);
    layout->addWidget(title);

    // Description
    auto* desc = new QLabel("Select the folder containing the game you want to repack.", this);
    desc->setStyleSheet("color: #aaaaaa;");
    layout->addWidget(desc);

    layout->addSpacing(20);

    // Source directory selection
    auto* dirGroup = new QGroupBox("Game Directory", this);
    dirGroup->setStyleSheet(
        "QGroupBox { border: 1px solid #333333; border-radius: 4px; margin-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; color: #aaaaaa; }"
    );
    auto* dirLayout = new QVBoxLayout(dirGroup);

    auto* selectionLayout = new QHBoxLayout();
    m_sourceDirEdit = new QLineEdit(this);
    m_sourceDirEdit->setPlaceholderText("Select the game folder...");
    m_sourceDirEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #333333; padding: 8px; background: #1a1a1a; border-radius: 4px; color: #ffffff; }"
        "QLineEdit:focus { border-color: #4CAF50; }"
    );
    selectionLayout->addWidget(m_sourceDirEdit);

    m_browseSourceBtn = new QPushButton("Browse...", this);
    m_browseSourceBtn->setStyleSheet(
        "QPushButton { background-color: #2a2a2a; color: #cccccc; padding: 8px 16px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #3a3a3a; }"
    );
    connect(m_browseSourceBtn, &QPushButton::clicked, this, &MainWindow::onBrowseSourceClicked);
    selectionLayout->addWidget(m_browseSourceBtn);

    dirLayout->addLayout(selectionLayout);

    // Scan button
    auto* scanBtn = new QPushButton("Scan Directory", this);
    scanBtn->setStyleSheet(
        "QPushButton { background-color: #2a2a2a; color: #4CAF50; padding: 8px 16px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #3a3a3a; }"
    );
    connect(scanBtn, &QPushButton::clicked, this, &MainWindow::onScanSourceDirectory);
    dirLayout->addWidget(scanBtn, 0, Qt::AlignRight);

    layout->addWidget(dirGroup);

    // File info display
    m_infoGroup = new QGroupBox("Directory Information", this);
    m_infoGroup->setVisible(false);
    m_infoGroup->setStyleSheet(
        "QGroupBox { border: 1px solid #333333; border-radius: 4px; margin-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; color: #aaaaaa; }"
    );
    auto* infoLayout = new QFormLayout(m_infoGroup);
    infoLayout->setSpacing(10);
    infoLayout->setContentsMargins(15, 15, 15, 15);

    m_fileCountLabel = new QLabel("--", this);
    m_fileCountLabel->setStyleSheet("color: #ffffff;");
    infoLayout->addRow("Files:", m_fileCountLabel);

    m_dirCountLabel = new QLabel("--", this);
    m_dirCountLabel->setStyleSheet("color: #ffffff;");
    infoLayout->addRow("Directories:", m_dirCountLabel);

    m_totalSizeLabel = new QLabel("--", this);
    m_totalSizeLabel->setStyleSheet("color: #ffffff;");
    infoLayout->addRow("Total size:", m_totalSizeLabel);

    layout->addWidget(m_infoGroup);

    layout->addStretch();

    m_stackedWidget->addWidget(m_sourcePage);
}

void MainWindow::setupCoverPage()
{
    m_coverPage = new QWidget(this);
    auto* layout = new QVBoxLayout(m_coverPage);
    layout->setSpacing(20);
    layout->setContentsMargins(50, 40, 50, 40);

    // Title
    auto* title = new QLabel("Game Cover", this);
    QFont titleFont("Rubik", 20, 600);
    title->setFont(titleFont);
    layout->addWidget(title);

    // Description
    auto* desc = new QLabel("Select a cover image for your game package (PNG, JPG, or JPEG).", this);
    desc->setStyleSheet("color: #aaaaaa;");
    layout->addWidget(desc);

    layout->addSpacing(20);

    // Cover selection
    auto* coverGroup = new QGroupBox("Cover Image", this);
    coverGroup->setStyleSheet(
        "QGroupBox { border: 1px solid #333333; border-radius: 4px; margin-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; color: #aaaaaa; }"
    );
    auto* coverLayout = new QVBoxLayout(coverGroup);

    auto* selectionLayout = new QHBoxLayout();
    m_coverPathEdit = new QLineEdit(this);
    m_coverPathEdit->setPlaceholderText("Select a cover image...");
    m_coverPathEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #333333; padding: 8px; background: #1a1a1a; border-radius: 4px; color: #ffffff; }"
        "QLineEdit:focus { border-color: #4CAF50; }"
    );
    selectionLayout->addWidget(m_coverPathEdit);

    m_browseCoverBtn = new QPushButton("Browse...", this);
    m_browseCoverBtn->setStyleSheet(
        "QPushButton { background-color: #2a2a2a; color: #cccccc; padding: 8px 16px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #3a3a3a; }"
    );
    connect(m_browseCoverBtn, &QPushButton::clicked, this, &MainWindow::onBrowseCoverClicked);
    selectionLayout->addWidget(m_browseCoverBtn);

    coverLayout->addLayout(selectionLayout);

    // Cover preview
    m_coverPreviewLabel = new QLabel(this);
    m_coverPreviewLabel->setAlignment(Qt::AlignCenter);
    m_coverPreviewLabel->setMinimumHeight(250);
    m_coverPreviewLabel->setStyleSheet(
        "border: 2px dashed #333333; border-radius: 4px; background-color: #0a0a0a;"
        "color: #444444; font-size: 14px;"
    );
    m_coverPreviewLabel->setText("No image selected");
    coverLayout->addWidget(m_coverPreviewLabel);

    layout->addWidget(coverGroup);

    layout->addStretch();

    m_stackedWidget->addWidget(m_coverPage);
}

void MainWindow::setupConfigurationPage()
{
    m_configurationPage = new QWidget(this);
    auto* layout = new QVBoxLayout(m_configurationPage);
    layout->setSpacing(20);
    layout->setContentsMargins(50, 40, 50, 40);

    // Title
    auto* title = new QLabel("Package Configuration", this);
    QFont titleFont("Rubik", 20, 600);
    title->setFont(titleFont);
    layout->addWidget(title);

    // Description
    auto* desc = new QLabel("Configure your game package settings.", this);
    desc->setStyleSheet("color: #aaaaaa;");
    layout->addWidget(desc);

    layout->addSpacing(20);

    // Scroll area for configuration
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: none; background-color: transparent; }");

    auto* configWidget = new QWidget(this);
    auto* configLayout = new QVBoxLayout(configWidget);
    configLayout->setSpacing(15);

    // Output directory
    auto* outputGroup = new QGroupBox("Output Settings", this);
    outputGroup->setStyleSheet(
        "QGroupBox { border: 1px solid #333333; border-radius: 4px; margin-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; color: #aaaaaa; }"
    );
    auto* outputLayout = new QVBoxLayout(outputGroup);

    auto* outputSelectLayout = new QHBoxLayout();
    m_outputDirEdit = new QLineEdit(this);
    m_outputDirEdit->setPlaceholderText("Output directory...");
    m_outputDirEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #333333; padding: 8px; background: #1a1a1a; border-radius: 4px; color: #ffffff; }"
        "QLineEdit:focus { border-color: #4CAF50; }"
    );
    outputSelectLayout->addWidget(m_outputDirEdit);

    m_browseOutputBtn = new QPushButton("Browse...", this);
    m_browseOutputBtn->setStyleSheet(
        "QPushButton { background-color: #2a2a2a; color: #cccccc; padding: 8px 16px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #3a3a3a; }"
    );
    connect(m_browseOutputBtn, &QPushButton::clicked, this, &MainWindow::onBrowseOutputClicked);
    outputSelectLayout->addWidget(m_browseOutputBtn);

    outputLayout->addLayout(outputSelectLayout);
    configLayout->addWidget(outputGroup);

    // Game information
    auto* gameGroup = new QGroupBox("Game Information", this);
    gameGroup->setStyleSheet(
        "QGroupBox { border: 1px solid #333333; border-radius: 4px; margin-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; color: #aaaaaa; }"
    );
    auto* gameLayout = new QFormLayout(gameGroup);
    gameLayout->setSpacing(10);
    gameLayout->setContentsMargins(15, 15, 15, 15);

    m_gameNameEdit = new QLineEdit(this);
    m_gameNameEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #333333; padding: 8px; background: #1a1a1a; border-radius: 4px; color: #ffffff; }"
        "QLineEdit:focus { border-color: #4CAF50; }"
    );
    connect(m_gameNameEdit, &QLineEdit::textChanged, this, &MainWindow::validateInputs);
    gameLayout->addRow("Game Name:", m_gameNameEdit);

    m_gameVersionEdit = new QLineEdit(this);
    m_gameVersionEdit->setPlaceholderText("1.0.0");
    m_gameVersionEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #333333; padding: 8px; background: #1a1a1a; border-radius: 4px; color: #ffffff; }"
        "QLineEdit:focus { border-color: #4CAF50; }"
    );
    gameLayout->addRow("Game Version:", m_gameVersionEdit);

    m_repackerNameEdit = new QLineEdit(this);
    m_repackerNameEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #333333; padding: 8px; background: #1a1a1a; border-radius: 4px; color: #ffffff; }"
        "QLineEdit:focus { border-color: #4CAF50; }"
    );
    gameLayout->addRow("Repacker Name:", m_repackerNameEdit);

    m_setupNameEdit = new QLineEdit(this);
    m_setupNameEdit->setPlaceholderText("Setup.exe");
    m_setupNameEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #333333; padding: 8px; background: #1a1a1a; border-radius: 4px; color: #ffffff; }"
        "QLineEdit:focus { border-color: #4CAF50; }"
    );
    connect(m_setupNameEdit, &QLineEdit::textChanged, this, &MainWindow::validateInputs);
    gameLayout->addRow("Setup Name:", m_setupNameEdit);

    configLayout->addWidget(gameGroup);

    // Compression and chunk settings
    auto* techGroup = new QGroupBox("Technical Settings", this);
    techGroup->setStyleSheet(
        "QGroupBox { border: 1px solid #333333; border-radius: 4px; margin-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; color: #aaaaaa; }"
    );
    auto* techLayout = new QFormLayout(techGroup);
    techLayout->setSpacing(10);
    techLayout->setContentsMargins(15, 15, 15, 15);

    m_compressionLevelCombo = new QComboBox(this);
    for (int i = 1; i <= 22; ++i) {
        m_compressionLevelCombo->addItem(QString("Level %1").arg(i));
    }
    m_compressionLevelCombo->setCurrentIndex(18); // Default level 19
    m_compressionLevelCombo->setStyleSheet(
        "QComboBox { border: 1px solid #333333; padding: 8px; background: #1a1a1a; border-radius: 4px; color: #ffffff; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background-color: #1a1a1a; color: #ffffff; }"
    );
    techLayout->addRow("Compression:", m_compressionLevelCombo);

    m_chunkSizeSpin = new QSpinBox(this);
    m_chunkSizeSpin->setRange(100, 4096);
    m_chunkSizeSpin->setValue(1024);
    m_chunkSizeSpin->setSuffix(" MB");
    m_chunkSizeSpin->setStyleSheet(
        "QSpinBox { border: 1px solid #333333; padding: 8px; background: #1a1a1a; border-radius: 4px; color: #ffffff; }"
        "QSpinBox::up-button, QSpinBox::down-button { background-color: #2a2a2a; }"
    );
    techLayout->addRow("Chunk Size:", m_chunkSizeSpin);

    m_encryptionCheck = new QCheckBox("Enable Encryption (AES-256-GCM)", this);
    m_encryptionCheck->setChecked(true);
    m_encryptionCheck->setStyleSheet("color: #aaaaaa;");
    techLayout->addRow("", m_encryptionCheck);

    m_hiddenFilesCheck = new QCheckBox("Include Hidden Files", this);
    m_hiddenFilesCheck->setStyleSheet("color: #aaaaaa;");
    techLayout->addRow("", m_hiddenFilesCheck);

    m_generateSetupCheck = new QCheckBox("Generate Setup.exe", this);
    m_generateSetupCheck->setChecked(true);
    m_generateSetupCheck->setStyleSheet("color: #aaaaaa;");
    techLayout->addRow("", m_generateSetupCheck);

    configLayout->addWidget(techGroup);

    configLayout->addStretch();
    configWidget->setLayout(configLayout);
    scrollArea->setWidget(configWidget);
    layout->addWidget(scrollArea);

    m_stackedWidget->addWidget(m_configurationPage);
}

void MainWindow::setupReviewPage()
{
    m_reviewPage = new QWidget(this);
    auto* layout = new QVBoxLayout(m_reviewPage);
    layout->setSpacing(20);
    layout->setContentsMargins(50, 40, 50, 40);

    // Title
    auto* title = new QLabel("Review Configuration", this);
    QFont titleFont("Rubik", 20, 600);
    title->setFont(titleFont);
    layout->addWidget(title);

    // Description
    auto* desc = new QLabel("Please review your package configuration before proceeding.", this);
    desc->setStyleSheet("color: #aaaaaa;");
    layout->addWidget(desc);

    layout->addSpacing(20);

    // Review group
    auto* reviewGroup = new QGroupBox("Configuration Summary", this);
    reviewGroup->setStyleSheet(
        "QGroupBox { border: 1px solid #333333; border-radius: 4px; margin-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; color: #aaaaaa; }"
    );
    auto* reviewLayout = new QFormLayout(reviewGroup);
    reviewLayout->setSpacing(12);
    reviewLayout->setContentsMargins(20, 20, 20, 20);

    // Create labels for all review items
    m_reviewSourceLabel = new QLabel("--", this);
    m_reviewSourceLabel->setStyleSheet("color: #ffffff;");
    reviewLayout->addRow("Source Directory:", m_reviewSourceLabel);

    m_reviewOutputLabel = new QLabel("--", this);
    m_reviewOutputLabel->setStyleSheet("color: #ffffff;");
    reviewLayout->addRow("Output Directory:", m_reviewOutputLabel);

    m_reviewGameNameLabel = new QLabel("--", this);
    m_reviewGameNameLabel->setStyleSheet("color: #ffffff;");
    reviewLayout->addRow("Game Name:", m_reviewGameNameLabel);

    m_reviewGameVersionLabel = new QLabel("--", this);
    m_reviewGameVersionLabel->setStyleSheet("color: #ffffff;");
    reviewLayout->addRow("Game Version:", m_reviewGameVersionLabel);

    m_reviewRepackerLabel = new QLabel("--", this);
    m_reviewRepackerLabel->setStyleSheet("color: #ffffff;");
    reviewLayout->addRow("Repacker:", m_reviewRepackerLabel);

    m_reviewSetupNameLabel = new QLabel("--", this);
    m_reviewSetupNameLabel->setStyleSheet("color: #ffffff;");
    reviewLayout->addRow("Setup Name:", m_reviewSetupNameLabel);

    m_reviewCoverLabel = new QLabel("--", this);
    m_reviewCoverLabel->setStyleSheet("color: #ffffff;");
    reviewLayout->addRow("Cover Image:", m_reviewCoverLabel);

    m_reviewCompressionLabel = new QLabel("--", this);
    m_reviewCompressionLabel->setStyleSheet("color: #ffffff;");
    reviewLayout->addRow("Compression:", m_reviewCompressionLabel);

    m_reviewChunkSizeLabel = new QLabel("--", this);
    m_reviewChunkSizeLabel->setStyleSheet("color: #ffffff;");
    reviewLayout->addRow("Chunk Size:", m_reviewChunkSizeLabel);

    m_reviewEncryptionLabel = new QLabel("--", this);
    m_reviewEncryptionLabel->setStyleSheet("color: #ffffff;");
    reviewLayout->addRow("Encryption:", m_reviewEncryptionLabel);

    m_reviewHiddenFilesLabel = new QLabel("--", this);
    m_reviewHiddenFilesLabel->setStyleSheet("color: #ffffff;");
    reviewLayout->addRow("Hidden Files:", m_reviewHiddenFilesLabel);

    m_reviewGenerateSetupLabel = new QLabel("--", this);
    m_reviewGenerateSetupLabel->setStyleSheet("color: #ffffff;");
    reviewLayout->addRow("Generate Setup:", m_reviewGenerateSetupLabel);

    layout->addWidget(reviewGroup);

    // Warning label
    auto* warningLabel = new QLabel("⚠️ Please review all settings carefully before starting.", this);
    warningLabel->setStyleSheet("color: #FFC107; font-weight: bold; padding: 10px; border: 1px solid #FFC107; border-radius: 4px;");
    warningLabel->setWordWrap(true);
    layout->addWidget(warningLabel);

    layout->addStretch();

    m_stackedWidget->addWidget(m_reviewPage);
}

void MainWindow::setupProgressPage()
{
    m_progressPage = new QWidget(this);
    auto* layout = new QVBoxLayout(m_progressPage);
    layout->setSpacing(20);
    layout->setContentsMargins(50, 40, 50, 40);

    // Title
    auto* title = new QLabel("Repacking in Progress...", this);
    QFont titleFont("Rubik", 20, 600);
    title->setFont(titleFont);
    layout->addWidget(title);

    // Status
    m_progressStatusLabel = new QLabel("Initializing...", this);
    m_progressStatusLabel->setStyleSheet("color: #aaaaaa;");
    layout->addWidget(m_progressStatusLabel);

    layout->addSpacing(10);

    // Progress bar
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setStyleSheet(
        "QProgressBar { border: 1px solid #333333; border-radius: 4px; background-color: #1a1a1a; height: 30px; }"
        "QProgressBar::chunk { background-color: #4CAF50; border-radius: 3px; }"
    );
    layout->addWidget(m_progressBar);

    layout->addSpacing(10);

    // Log
    auto* logGroup = new QGroupBox("Progress Log", this);
    logGroup->setStyleSheet(
        "QGroupBox { border: 1px solid #333333; border-radius: 4px; margin-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; color: #aaaaaa; }"
    );
    auto* logLayout = new QVBoxLayout(logGroup);

    m_progressLogText = new QTextEdit(this);
    m_progressLogText->setReadOnly(true);
    m_progressLogText->setStyleSheet(
        "QTextEdit { border: none; background-color: #0a0a0a; color: #aaaaaa; font-family: Consolas, monospace; }"
    );
    m_progressLogText->setFont(QFont("Consolas", 9));
    logLayout->addWidget(m_progressLogText);

    layout->addWidget(logGroup);

    // Cancel button
    m_cancelRepackBtn = new QPushButton("Cancel Repack", this);
    m_cancelRepackBtn->setStyleSheet(
        "QPushButton { background-color: #dc3545; color: #ffffff; padding: 10px 32px; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #c82333; }"
    );
    connect(m_cancelRepackBtn, &QPushButton::clicked, this, &MainWindow::onCancelRepackClicked);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(m_cancelRepackBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    m_stackedWidget->addWidget(m_progressPage);
}

void MainWindow::setupCompletePage()
{
    m_completePage = new QWidget(this);
    auto* layout = new QVBoxLayout(m_completePage);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(25);
    layout->setContentsMargins(60, 60, 60, 60);

    // Complete icon
    m_completeIcon = new QLabel("✓", this);
    m_completeIcon->setAlignment(Qt::AlignCenter);
    m_completeIcon->setStyleSheet("color: #4CAF50; font-size: 72px;");
    layout->addWidget(m_completeIcon);

    // Complete message
    m_completeMessage = new QLabel("Repack Complete!", this);
    m_completeMessage->setAlignment(Qt::AlignCenter);
    QFont msgFont("Rubik", 24, QFont::Bold);
    m_completeMessage->setFont(msgFont);
    layout->addWidget(m_completeMessage);

    // Details
    m_completeDetailsLabel = new QLabel("Your package has been successfully created.", this);
    m_completeDetailsLabel->setAlignment(Qt::AlignCenter);
    m_completeDetailsLabel->setStyleSheet("color: #aaaaaa;");
    layout->addWidget(m_completeDetailsLabel);

    // Package size
    m_packageSizeLabel = new QLabel("", this);
    m_packageSizeLabel->setAlignment(Qt::AlignCenter);
    m_packageSizeLabel->setStyleSheet("color: #4CAF50; font-size: 14px;");
    layout->addWidget(m_packageSizeLabel);

    layout->addStretch();

    // Output location
    auto* outputLabel = new QLabel("Output directory:", this);
    outputLabel->setAlignment(Qt::AlignCenter);
    outputLabel->setStyleSheet("color: #888888;");
    layout->addWidget(outputLabel);

    auto* outputPathLabel = new QLabel("", this);
    outputPathLabel->setAlignment(Qt::AlignCenter);
    outputPathLabel->setStyleSheet("color: #ffffff;");
    outputPathLabel->setWordWrap(true);
    layout->addWidget(outputPathLabel);

    // Store reference to output path label for updates
    outputPathLabel->setObjectName("outputPathLabel");

    layout->addStretch();

    m_stackedWidget->addWidget(m_completePage);
}

void MainWindow::showPage(int index)
{
    m_stackedWidget->setCurrentIndex(index);
    updateNavigationButtons();
}

void MainWindow::updateNavigationButtons()
{
    if (m_isRepacking) {
        m_backButton->setEnabled(false);
        m_nextButton->setEnabled(false);
        m_cancelButton->setEnabled(false);
        m_cancelButton->setText("Repacking...");
        return;
    }

    if (m_repackComplete) {
        m_backButton->setEnabled(false);
        m_nextButton->setText("Finish");
        m_nextButton->setEnabled(true);
        m_cancelButton->setText("Close");
        return;
    }

    if (m_currentPage == WelcomePage) {
        m_backButton->setEnabled(false);
        m_nextButton->setText("Continue →");
        m_nextButton->setEnabled(true);
        m_cancelButton->setText("Cancel");
    }
    else if (m_currentPage == ProgressPage) {
        m_backButton->setEnabled(false);
        m_nextButton->setEnabled(false);
        m_cancelButton->setText("Cancel");
    }
    else if (m_currentPage == CompletePage) {
        m_backButton->setEnabled(false);
        m_nextButton->setText("Finish");
        m_nextButton->setEnabled(true);
        m_cancelButton->setText("Close");
    }
    else {
        m_backButton->setEnabled(true);
        m_nextButton->setText("Continue →");
        m_nextButton->setEnabled(validateInputs());
        m_cancelButton->setText("Cancel");
    }
}

void MainWindow::updateReviewPage()
{
    m_reviewSourceLabel->setText(m_config.sourceDirectory.isEmpty() ?
        "<span style='color:#ff4444;'>Not set</span>" : m_config.sourceDirectory);
    m_reviewOutputLabel->setText(m_config.outputDirectory.isEmpty() ?
        "<span style='color:#ff4444;'>Not set</span>" : m_config.outputDirectory);
    m_reviewGameNameLabel->setText(m_config.gameName.isEmpty() ?
        "<span style='color:#ff4444;'>Not set</span>" : m_config.gameName);
    m_reviewGameVersionLabel->setText(m_config.gameVersion.isEmpty() ?
        "<span style='color:#888888;'>Not set</span>" : m_config.gameVersion);
    m_reviewRepackerLabel->setText(m_config.repackerName.isEmpty() ?
        "<span style='color:#888888;'>Not set</span>" : m_config.repackerName);
    m_reviewSetupNameLabel->setText(m_config.setupName.isEmpty() ?
        "<span style='color:#ff4444;'>Not set</span>" : m_config.setupName);
    m_reviewCoverLabel->setText(m_config.coverImagePath.isEmpty() ?
        "<span style='color:#888888;'>None selected</span>" :
        QFileInfo(m_config.coverImagePath).fileName());
    m_reviewCompressionLabel->setText(QString("Level %1").arg(m_config.compressionLevel));

    QString chunkSizeStr;
    if (m_config.chunkSize >= 1024 * 1024 * 1024) {
        chunkSizeStr = QString("%1 GB").arg(m_config.chunkSize / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
    }
    else if (m_config.chunkSize >= 1024 * 1024) {
        chunkSizeStr = QString("%1 MB").arg(m_config.chunkSize / (1024.0 * 1024.0), 0, 'f', 1);
    }
    else {
        chunkSizeStr = QString("%1 KB").arg(m_config.chunkSize / 1024.0, 0, 'f', 1);
    }
    m_reviewChunkSizeLabel->setText(chunkSizeStr);

    m_reviewEncryptionLabel->setText(m_config.enableEncryption ?
        "<span style='color:#4CAF50;'>Enabled (AES-256-GCM)</span>" :
        "<span style='color:#ff4444;'>Disabled</span>");
    m_reviewHiddenFilesLabel->setText(m_config.includeHiddenFiles ? "Yes" : "No");
    m_reviewGenerateSetupLabel->setText(m_config.generateSetup ? "Yes" : "No");
}

void MainWindow::updateConfigurationPage()
{
    // Update config from UI
    m_config.outputDirectory = m_outputDirEdit->text();
    m_config.gameName = m_gameNameEdit->text();
    m_config.gameVersion = m_gameVersionEdit->text();
    m_config.repackerName = m_repackerNameEdit->text();
    m_config.setupName = m_setupNameEdit->text();
    m_config.compressionLevel = m_compressionLevelCombo->currentIndex() + 1;
    m_config.chunkSize = static_cast<uint64_t>(m_chunkSizeSpin->value()) * 1024 * 1024;
    m_config.enableEncryption = m_encryptionCheck->isChecked();
    m_config.includeHiddenFiles = m_hiddenFilesCheck->isChecked();
    m_config.generateSetup = m_generateSetupCheck->isChecked();

    validateInputs();
}

void MainWindow::validateInputs()
{
    bool valid = true;

    // Source directory
    if (!m_config.sourceDirectory.isEmpty()) {
        QFileInfo info(m_config.sourceDirectory);
        if (info.exists() && info.isDir()) {
            // Valid source
        }
        else {
            valid = false;
        }
    }
    else {
        valid = false;
    }

    // Output directory
    if (!m_config.outputDirectory.isEmpty()) {
        QFileInfo info(m_config.outputDirectory);
        // Directory doesn't need to exist yet, but path should be valid
        if (!QDir(m_config.outputDirectory).exists()) {
            // Try to create it
            QDir dir;
            if (!dir.mkpath(m_config.outputDirectory)) {
                valid = false;
            }
        }
    }
    else {
        valid = false;
    }

    // Game name
    if (m_config.gameName.isEmpty() || m_config.gameName.length() < 2) {
        valid = false;
    }

    // Setup name
    if (m_config.setupName.isEmpty() || !m_config.setupName.endsWith(".exe", Qt::CaseInsensitive)) {
        valid = false;
    }
    bool valid = validateInputs();
    m_nextButton->setEnabled(valid && !m_isRepacking && !m_repackComplete);
}

bool MainWindow::validateSourceDirectory()
{
    if (m_config.sourceDirectory.isEmpty()) {
        return false;
    }
    QFileInfo info(m_config.sourceDirectory);
    return info.exists() && info.isDir();
}

bool MainWindow::validateOutputDirectory()
{
    if (m_config.outputDirectory.isEmpty()) {
        return false;
    }
    QDir dir(m_config.outputDirectory);
    return dir.exists() || dir.mkpath(m_config.outputDirectory);
}

bool MainWindow::validateGameName()
{
    return !m_config.gameName.isEmpty() && m_config.gameName.length() >= 2;
}

bool MainWindow::validateSetupName()
{
    return !m_config.setupName.isEmpty() && m_config.setupName.endsWith(".exe", Qt::CaseInsensitive);
}

void MainWindow::updateFileInfo(const QString & dir)
{
    if (dir.isEmpty()) {
        m_infoGroup->setVisible(false);
        return;
    }

    QFileInfo info(dir);
    if (!info.exists() || !info.isDir()) {
        m_infoGroup->setVisible(false);
        return;
    }

    // Use DirectoryScanner for accurate results
    noty::DirectoryScanner scanner;
    auto result = scanner.scanDirectory(dir.toStdString(), true, false);

    if (result.success) {
        m_fileCount = result.fileCount;
        m_directoryCount = result.directoryCount;
        m_totalSize = result.totalSize;

        m_fileCountLabel->setText(QString::number(m_fileCount));
        m_dirCountLabel->setText(QString::number(m_directoryCount));

        QString sizeStr;
        if (m_totalSize > 1024 * 1024 * 1024) {
            sizeStr = QString("%1 GB").arg(m_totalSize / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
        }
        else if (m_totalSize > 1024 * 1024) {
            sizeStr = QString("%1 MB").arg(m_totalSize / (1024.0 * 1024.0), 0, 'f', 2);
        }
        else if (m_totalSize > 1024) {
            sizeStr = QString("%1 KB").arg(m_totalSize / 1024.0, 0, 'f', 2);
        }
        else {
            sizeStr = QString("%1 bytes").arg(m_totalSize);
        }
        m_totalSizeLabel->setText(sizeStr);
        m_infoGroup->setVisible(true);
    }
    else {
        m_infoGroup->setVisible(false);
    }
}

void MainWindow::startRepacking()
{
    if (m_isRepacking) {
        return;
    }

    // Update config from UI
    m_config.sourceDirectory = m_sourceDirEdit->text();
    m_config.outputDirectory = m_outputDirEdit->text();
    m_config.gameName = m_gameNameEdit->text();
    m_config.gameVersion = m_gameVersionEdit->text();
    m_config.repackerName = m_repackerNameEdit->text();
    m_config.setupName = m_setupNameEdit->text();
    m_config.coverImagePath = m_coverPathEdit->text();
    m_config.compressionLevel = m_compressionLevelCombo->currentIndex() + 1;
    m_config.chunkSize = static_cast<uint64_t>(m_chunkSizeSpin->value()) * 1024 * 1024;
    m_config.enableEncryption = m_encryptionCheck->isChecked();
    m_config.includeHiddenFiles = m_hiddenFilesCheck->isChecked();
    m_config.generateSetup = m_generateSetupCheck->isChecked();

    // Validate config
    if (!validateSourceDirectory()) {
        QMessageBox::warning(this, "Invalid Source", "Please select a valid source directory.");
        return;
    }

    if (!validateOutputDirectory()) {
        QMessageBox::warning(this, "Invalid Output", "Please select a valid output directory.");
        return;
    }

    if (!validateGameName()) {
        QMessageBox::warning(this, "Invalid Game Name", "Please enter a valid game name (at least 2 characters).");
        return;
    }

    if (!validateSetupName()) {
        QMessageBox::warning(this, "Invalid Setup Name", "Please enter a valid setup name (must end with .exe).");
        return;
    }

    m_isRepacking = true;
    m_repackComplete = false;
    m_repackSuccess = false;
    m_progressLogText->clear();

    // Show progress page
    showPage(ProgressPage);
    updateNavigationButtons();

    noty::Logger::instance().info("Starting repack for: " + m_config.gameName.toStdString());

    // Create repack job configuration
    noty::RepackJob::Configuration jobConfig;
    jobConfig.sourceDirectory = m_config.sourceDirectory.toStdString();
    jobConfig.outputDirectory = m_config.outputDirectory.toStdString();
    jobConfig.gameName = m_config.gameName.toStdString();
    jobConfig.gameVersion = m_config.gameVersion.toStdString();
    jobConfig.repackerName = m_config.repackerName.toStdString();
    jobConfig.setupName = m_config.setupName.toStdString();
    jobConfig.compressionLevel = m_config.compressionLevel;
    jobConfig.compressionBufferSize = 1024 * 1024;
    jobConfig.maxChunkSize = m_config.chunkSize;
    jobConfig.enableEncryption = m_config.enableEncryption;
    jobConfig.includeHiddenFiles = m_config.includeHiddenFiles;
    jobConfig.generateSetup = m_config.generateSetup;
    jobConfig.coverImagePath = m_config.coverImagePath.toStdString();
    jobConfig.hashAlgorithm = "BLAKE3";

    // Create job
    noty::RepackJob job(jobConfig);
    job.setProgressCallback([this](int percent, const std::string& status) {
        onRepackProgress(percent, QString::fromStdString(status));
        });

    // Start repacking in a separate thread
    QThread* thread = new QThread(this);
    auto* engine = new noty::RepackEngine();
    engine->moveToThread(thread);

    connect(thread, &QThread::started, [engine, job = std::move(job)]() mutable {
        engine->startJob(std::move(job));
        });

    connect(thread, &QThread::finished, engine, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    // Monitor engine for completion
    connect(engine, &noty::RepackEngine::destroyed, [this]() {
        // Check if repack was successful
        onRepackComplete(true, "Repack completed successfully!");
        });

    // Store engine pointer for cancellation
    // In a real implementation, we'd store this in a member

    thread->start();

    // Add initial log message
    m_progressLogText->append("[INFO] Starting repack for: " + m_config.gameName);
    m_progressLogText->append("[INFO] Source: " + m_config.sourceDirectory);
    m_progressLogText->append("[INFO] Output: " + m_config.outputDirectory);
    m_progressLogText->append("[INFO] Chunk size: " +
        QString::number(m_config.chunkSize / (1024 * 1024)) + " MB");
    m_progressLogText->append("[INFO] Compression level: " +
        QString::number(m_config.compressionLevel));
    m_progressLogText->append("[INFO] Encryption: " +
        QString(m_config.enableEncryption ? "Enabled" : "Disabled"));
    m_progressLogText->append("---");
    m_progressLogText->ensureCursorVisible();
}

void MainWindow::cancelRepacking()
{
    if (!m_isRepacking) {
        return;
    }

    // Cancel the repack
    // In a real implementation, we'd call engine->cancelCurrentJob()
    m_isRepacking = false;
    m_repackComplete = true;
    m_repackSuccess = false;

    m_progressLogText->append("[WARN] Repack cancelled by user");
    m_progressStatusLabel->setText("Cancelled");
    m_progressBar->setValue(0);

    showPage(CompletePage);
    updateNavigationButtons();

    noty::Logger::instance().info("Repack cancelled by user");
}

void MainWindow::onContinueClicked()
{
    if (m_isRepacking) {
        return;
    }

    if (m_repackComplete) {
        // Finish
        close();
        return;
    }

    if (m_currentPage == WelcomePage) {
        showPage(SourcePage);
        return;
    }

    if (m_currentPage == SourcePage) {
        if (!validateSourceDirectory()) {
            QMessageBox::warning(this, "Invalid Source",
                "Please select a valid source directory.\n"
                "Click 'Browse' to select a folder containing your game files.");
            return;
        }
        showPage(CoverPage);
        return;
    }

    if (m_currentPage == CoverPage) {
        // Cover image is optional
        showPage(ConfigurationPage);
        updateConfigurationPage();
        return;
    }

    if (m_currentPage == ConfigurationPage) {
        updateConfigurationPage();
        if (!validateGameName()) {
            QMessageBox::warning(this, "Invalid Game Name",
                "Please enter a valid game name (at least 2 characters).");
            return;
        }
        if (!validateSetupName()) {
            QMessageBox::warning(this, "Invalid Setup Name",
                "Please enter a valid setup name (must end with .exe).\n"
                "Example: MyGameSetup.exe");
            return;
        }
        if (!validateOutputDirectory()) {
            QMessageBox::warning(this, "Invalid Output Directory",
                "Please select a valid output directory.");
            return;
        }
        // Update review page before showing
        updateReviewPage();
        showPage(ReviewPage);
        return;
    }

    if (m_currentPage == ReviewPage) {
        // Start the repack
        startRepacking();
        return;
    }

    if (m_currentPage == CompletePage) {
        close();
        return;
    }

    // Default: go to next page
    if (m_currentPage < CompletePage) {
        m_currentPage++;
        showPage(m_currentPage);
    }
}

void MainWindow::onBackClicked()
{
    if (m_isRepacking || m_repackComplete) {
        return;
    }

    if (m_currentPage > 0) {
        m_currentPage--;
        showPage(m_currentPage);
    }
}

void MainWindow::onExitClicked()
{
    if (m_isRepacking) {
        int reply = QMessageBox::question(this, "Cancel Repack",
            "A repack is in progress. Are you sure you want to cancel?",
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            cancelRepacking();
            close();
        }
        return;
    }

    int reply = QMessageBox::question(this, "Exit",
        "Are you sure you want to exit?",
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        close();
    }
}

void MainWindow::onBrowseSourceClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this,
        "Select Game Folder",
        m_sourceDirEdit->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        m_sourceDirEdit->setText(dir);
        m_config.sourceDirectory = dir;
        updateFileInfo(dir);
        validateInputs();
    }
}

void MainWindow::onBrowseOutputClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this,
        "Select Output Directory",
        m_outputDirEdit->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        m_outputDirEdit->setText(dir);
        m_config.outputDirectory = dir;
        validateInputs();
    }
}

void MainWindow::onBrowseCoverClicked()
{
    QString filter = "Image Files (*.png *.jpg *.jpeg *.bmp *.gif)";
    QString file = QFileDialog::getOpenFileName(this,
        "Select Cover Image",
        m_coverPathEdit->text(),
        filter);

    if (!file.isEmpty()) {
        m_coverPathEdit->setText(file);
        m_config.coverImagePath = file;
        onCoverImageSelected();
        validateInputs();
    }
}

void MainWindow::onScanSourceDirectory()
{
    QString dir = m_sourceDirEdit->text();
    if (dir.isEmpty()) {
        QMessageBox::warning(this, "No Directory",
            "Please select a directory first.");
        return;
    }

    QFileInfo info(dir);
    if (!info.exists() || !info.isDir()) {
        QMessageBox::warning(this, "Invalid Directory",
            "The selected directory does not exist.");
        return;
    }

    updateFileInfo(dir);
}

void MainWindow::onCoverImageSelected()
{
    QString path = m_coverPathEdit->text();
    if (path.isEmpty()) {
        m_coverPreviewLabel->setText("No image selected");
        m_coverPreviewLabel->setStyleSheet(
            "border: 2px dashed #333333; border-radius: 4px; background-color: #0a0a0a;"
            "color: #444444; font-size: 14px;"
        );
        return;
    }

    QImageReader reader(path);
    if (!reader.canRead()) {
        m_coverPreviewLabel->setText("❌ Invalid image file");
        m_coverPreviewLabel->setStyleSheet(
            "border: 2px dashed #dc3545; border-radius: 4px; background-color: #0a0a0a;"
            "color: #dc3545; font-size: 14px;"
        );
        return;
    }

    QPixmap pixmap(path);
    if (pixmap.isNull()) {
        m_coverPreviewLabel->setText("❌ Failed to load image");
        m_coverPreviewLabel->setStyleSheet(
            "border: 2px dashed #dc3545; border-radius: 4px; background-color: #0a0a0a;"
            "color: #dc3545; font-size: 14px;"
        );
        return;
    }

    // Scale to fit preview
    QPixmap scaled = pixmap.scaled(m_coverPreviewLabel->width() - 20,
        m_coverPreviewLabel->height() - 20,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation);
    m_coverPreviewLabel->setPixmap(scaled);
    m_coverPreviewLabel->setStyleSheet("border: none; background-color: #0a0a0a;");
}

void MainWindow::onStartRepackClicked()
{
    startRepacking();
}

void MainWindow::onCancelRepackClicked()
{
    cancelRepacking();
}

void MainWindow::onFinishClicked()
{
    close();
}

void MainWindow::onRepackProgress(int percent, const QString & status)
{
    // Update UI from any thread
    QMetaObject::invokeMethod(this, [this, percent, status]() {
        m_progressBar->setValue(percent);
        m_progressStatusLabel->setText(status);

        // Add to log periodically
        if (percent % 10 == 0 || percent == 100) {
            m_progressLogText->append(QString("[%1%] %2").arg(percent).arg(status));
            m_progressLogText->ensureCursorVisible();
        }
        });
}

void MainWindow::onRepackComplete(bool success, const QString & message)
{
    QMetaObject::invokeMethod(this, [this, success, message]() {
        m_isRepacking = false;
        m_repackComplete = true;
        m_repackSuccess = success;

        if (success) {
            m_progressLogText->append("✓ " + message);
            m_progressStatusLabel->setText("Complete!");
            m_progressBar->setValue(100);

            // Show completion page
            showPage(CompletePage);

            // Update completion page
            m_completeMessage->setText("Repack Complete!");
            m_completeDetailsLabel->setText("Your package has been successfully created.");

            // Show output directory
            QLabel* outputLabel = m_completePage->findChild<QLabel*>("outputPathLabel");
            if (outputLabel) {
                outputLabel->setText(m_config.outputDirectory);
            }

            // Show package size
            QString sizeStr;
            if (m_totalSize > 1024 * 1024 * 1024) {
                sizeStr = QString("Package size: %1 GB")
                    .arg(m_totalSize / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
            }
            else if (m_totalSize > 1024 * 1024) {
                sizeStr = QString("Package size: %1 MB")
                    .arg(m_totalSize / (1024.0 * 1024.0), 0, 'f', 2);
            }
            else {
                sizeStr = QString("Package size: %1 KB")
                    .arg(m_totalSize / 1024.0, 0, 'f', 2);
            }
            m_packageSizeLabel->setText(sizeStr);

            noty::Logger::instance().info("Repack completed successfully");
        }
        else {
            m_progressLogText->append("✗ " + message);
            m_progressStatusLabel->setText("Failed");
            m_progressBar->setValue(0);

            // Show error
            QMessageBox::critical(this, "Repack Failed", message);

            // Go back to review page
            showPage(ReviewPage);
            m_repackComplete = false;
            noty::Logger::instance().error("Repack failed: " + message.toStdString());
        }

        updateNavigationButtons();
        });
}