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
#include <QCoreApplication>
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
    std::string manifestPath = m_packagePath + "/" + noty::MANIFEST_FILENAME;
    if (!fs::exists(manifestPath)) {
        // Try parent directory
        fs::path parentPath = fs::path(m_packagePath).parent_path();
        manifestPath = (parentPath / noty::MANIFEST_FILENAME).string();
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
        delete m_installEngine;
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
        std::string manifestPath = m_packagePath + "/" + noty::MANIFEST_FILENAME;
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

    // Use resource manager for optimal settings
    auto& resourceManager = noty::ResourceManager::instance();
    m_installEngine->setThreadPoolSize(resourceManager.getThreadPoolSize());
    m_installEngine->setExtractionBufferSize(resourceManager.getDecompressionBufferSize());

    noty::InstallEngine* engine = m_installEngine;

    connect(m_installThread, &QThread::started, [this, job = std::move(job), engine]() mutable {
        engine->startJob(std::move(job));
        });

    connect(m_installThread, &QThread::finished, [this, engine]() {
        delete engine;
        m_installEngine = nullptr;
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

bool SetupApplication::prepareInstallation()
{
    // Set installation directory from window
    m_installDirectory = m_mainWindow->getInstallDirectory().toStdString();
    m_createDesktopShortcut = m_mainWindow->shouldCreateDesktopShortcut();
    m_createStartMenuShortcut = m_mainWindow->shouldCreateStartMenuShortcut();

    if (m_installDirectory.empty()) {
        m_lastError = "No installation directory selected";
        return false;
    }

    // Create directory if it doesn't exist
    try {
        if (!fs::exists(m_installDirectory)) {
            fs::create_directories(m_installDirectory);
        }
    }
    catch (const std::exception& e) {
        m_lastError = "Failed to create installation directory: " + std::string(e.what());
        return false;
    }

    return true;
}

bool SetupApplication::isComponentSelected(const std::string& component) const
{
    return std::find(m_selectedComponents.begin(), m_selectedComponents.end(),
        component) != m_selectedComponents.end();
}

void SetupApplication::removeSelectedComponent(const std::string& component)
{
    auto it = std::find(m_selectedComponents.begin(), m_selectedComponents.end(),
        component);
    if (it != m_selectedComponents.end()) {
        m_selectedComponents.erase(it);
    }
}