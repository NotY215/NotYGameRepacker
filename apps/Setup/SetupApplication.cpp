#include "SetupApplication.h"
#include "SetupWindow.h"
#include "noty/common/Logger.h"
#include "noty/common/Constants.h"
#include "noty/installer/InstallEngine.h"
#include "noty/installer/InstallJob.h"
#include "noty/package/ManifestReader.h"
#include "noty/core/ResourceManager.h"
#include "noty/core/PerformanceMonitor.h"
#include <QMessageBox>
#include <QThread>
#include <QDir>
#include <filesystem>

namespace fs = std::filesystem;

SetupApplication::SetupApplication(QObject* parent)
    : QObject(parent)
    , m_packageSize(0)
    , m_installEngine(nullptr)
    , m_installThread(nullptr)
{
    noty::Logger::instance().info("Setup application created.");

    // Initialize resource manager
    noty::ResourceManager::instance().initialize();
}

SetupApplication::~SetupApplication()
{
    shutdown();
}

bool SetupApplication::initialize()
{
    noty::Logger::instance().info("Initializing setup application...");

    // Create main window
    m_mainWindow = std::make_unique<SetupWindow>();

    // Connect signals
    connect(m_mainWindow.get(), &SetupWindow::installationStarted,
        this, &SetupApplication::startInstallation);
    connect(m_mainWindow.get(), &SetupWindow::installationCancelled,
        this, &SetupApplication::cancelInstallation);

    // Try to find package in the same directory as the setup executable
    QString appPath = QCoreApplication::applicationDirPath();
    m_packagePath = appPath.toStdString();

    // Check if manifest exists in the same directory
    std::string manifestPath = m_packagePath + "/" + noty::Constants::MANIFEST_FILENAME;
    if (!fs::exists(manifestPath)) {
        // Try parent directory
        fs::path parentPath = fs::path(m_packagePath).parent_path();
        manifestPath = (parentPath / noty::Constants::MANIFEST_FILENAME).string();
        if (fs::exists(manifestPath)) {
            m_packagePath = parentPath.string();
        }
    }

    // Load package information
    if (!loadPackageInfo()) {
        noty::Logger::instance().warning("Failed to load package info, continuing anyway");
    }

    // Update window with package info
    m_mainWindow->setPackageInfo(m_gameName, m_gameVersion, m_repackerName, m_packageSize);

    // Show main window
    m_mainWindow->show();

    noty::Logger::instance().info("Setup application initialized successfully.");
    return true;
}

void SetupApplication::shutdown()
{
    if (m_installEngine) {
        m_installEngine->shutdown();
        m_installEngine = nullptr;
    }
    if (m_installThread) {
        m_installThread->quit();
        m_installThread->wait();
        delete m_installThread;
        m_installThread = nullptr;
    }
    if (m_mainWindow) {
        m_mainWindow->close();
        m_mainWindow.reset();
    }
    noty::Logger::instance().info("Setup application shut down.");
}

bool SetupApplication::loadPackageInfo()
{
    try {
        std::string manifestPath = m_packagePath + "/" + noty::Constants::MANIFEST_FILENAME;
        if (!fs::exists(manifestPath)) {
            m_lastError = "Manifest not found: " + manifestPath;
            return false;
        }

        noty::ManifestReader reader;
        auto manifestOpt = reader.readFromFile(manifestPath);
        if (!manifestOpt.has_value()) {
            m_lastError = "Failed to read manifest";
            return false;
        }

        const auto& manifest = manifestOpt.value();
        const auto& info = manifest.getPackageInfo();

        m_gameName = QString::fromStdString(info.gameName);
        m_gameVersion = QString::fromStdString(info.gameVersion);
        m_repackerName = QString::fromStdString(info.repackerName);
        m_packageSize = info.originalSize;

        // Check if encryption is enabled
        m_encryptionEnabled = (info.encryptionMethod != "None");

        noty::Logger::instance().info("Package info loaded: " + info.gameName +
            " v" + info.gameVersion);
        return true;
    }
    catch (const std::exception& e) {
        m_lastError = "Failed to load package info: " + std::string(e.what());
        noty::Logger::instance().error(m_lastError);
        return false;
    }
}

void SetupApplication::startInstallation()
{
    if (m_installEngine) {
        return;
    }

    // Prepare for installation
    if (!prepareInstallation()) {
        m_mainWindow->setInstallationComplete(false,
            QString::fromStdString(m_lastError));
        return;
    }

    m_installationRunning = true;
    m_installationCancelled = false;

    noty::Logger::instance().info("Starting installation...");

    // Start performance monitoring
    noty::PerformanceMonitor::instance().startOperation("Installation",
        m_packageSize, 0);

    // Create installation job
    noty::InstallJob::Configuration config;
    config.packagePath = m_packagePath;
    config.installDirectory = m_installDirectory;
    config.gameName = m_gameName.toStdString();
    config.verifyFiles = true;
    config.createDesktopShortcut = m_createDesktopShortcut;
    config.createStartMenuShortcut = m_createStartMenuShortcut;
    config.selectedComponents = m_selectedComponents;
    config.enableEncryption = m_encryptionEnabled;
    config.encryptionKey = m_encryptionKey;
    config.encryptionNonce = m_encryptionNonce;

    // Create job
    noty::InstallJob job(config);
    job.setProgressCallback([this](int percent, const std::string& status) {
        onInstallationProgress(percent, QString::fromStdString(status));
        noty::PerformanceMonitor::instance().updateProgress(
            static_cast<uint64_t>(percent * m_packageSize / 100));
        });

    // Create engine in a separate thread
    m_installThread = new QThread(this);
    m_installEngine = new noty::InstallEngine();
    m_installEngine->moveToThread(m_installThread);

    // Use resource manager for optimal settings
    auto& resourceManager = noty::ResourceManager::instance();
    m_installEngine->setThreadPoolSize(resourceManager.getThreadPoolSize());
    m_installEngine->setExtractionBufferSize(resourceManager.getDecompressionBufferSize());

    connect(m_installThread, &QThread::started, [this, job = std::move(job)]() mutable {
        m_installEngine->startJob(std::move(job));
        });

    connect(m_installThread, &QThread::finished, [this]() {
        if (m_installEngine) {
            m_installEngine->deleteLater();
            m_installEngine = nullptr;
        }
        m_installationRunning = false;
        noty::PerformanceMonitor::instance().stopOperation();
        });

    m_installThread->start();
}

void SetupApplication::cancelInstallation()
{
    if (!m_installationRunning || !m_installEngine) {
        return;
    }

    m_installationCancelled = true;
    m_installEngine->cancelCurrentJob();
    noty::Logger::instance().info("Installation cancelled by user");
    noty::PerformanceMonitor::instance().stopOperation();
    emit installationCancelled();
}

void SetupApplication::onInstallationProgress(int percent, const QString& status)
{
    m_progress = percent;
    m_statusMessage = status;

    if (m_mainWindow) {
        m_mainWindow->updateProgress(percent, status);

        // Update ETA and throughput if available
        auto& monitor = noty::PerformanceMonitor::instance();
        if (monitor.isETAStable()) {
            QString eta = QString::fromStdString(monitor.getETAString());
            QString throughput = QString::fromStdString(monitor.getThroughputString());
            m_mainWindow->updateProgress(percent, status + " (ETA: " + eta + ")");
        }
    }
}

void SetupApplication::onInstallationComplete(bool success, const QString& message)
{
    m_installationRunning = false;

    if (m_mainWindow) {
        m_mainWindow->setInstallationComplete(success, message);
    }

    if (success) {
        noty::Logger::instance().info("Installation completed successfully");
    }
    else {
        noty::Logger::instance().error("Installation failed: " + message.toStdString());
    }
}