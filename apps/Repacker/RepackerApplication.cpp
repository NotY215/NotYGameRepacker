#include "RepackerApplication.h"
#include "MainWindow.h"
#include "noty/common/Logger.h"
#include "noty/core/ResourceManager.h"
#include "noty/core/PerformanceMonitor.h"
#include "noty/repacker/RepackEngine.h"
#include "noty/repacker/RepackJob.h"
#include <QMessageBox>
#include <QThread>
#include <QTimer>

RepackerApplication::RepackerApplication(QObject * parent)
    : QObject(parent)
    , m_repackEngine(nullptr)
    , m_repackThread(nullptr)
{
    noty::Logger::instance().info("Repacker application created.");

    // Initialize resource manager
    noty::ResourceManager::instance().initialize();

    // Start performance monitoring
    noty::PerformanceMonitor::instance().updateSystemMetrics();
}

RepackerApplication::~RepackerApplication()
{
    shutdown();
}

void RepackerApplication::initialize()
{
    m_mainWindow = std::make_unique<MainWindow>();

    // Connect repack signals
    connect(m_mainWindow.get(), &MainWindow::repackStarted,
        this, &RepackerApplication::onRepackStarted);
    connect(m_mainWindow.get(), &MainWindow::repackCancelled,
        this, &RepackerApplication::onRepackCancelled);

    m_mainWindow->show();
    noty::Logger::instance().info("Main window shown.");
}

void RepackerApplication::shutdown()
{
    if (m_repackEngine) {
        delete m_repackEngine;
        m_repackEngine = nullptr;
    }
    if (m_repackThread) {
        m_repackThread->quit();
        m_repackThread->wait();
        delete m_repackThread;
        m_repackThread = nullptr;
    }
    noty::Logger::instance().info("Repacker application shut down.");
}

void RepackerApplication::onRepackStarted(const PackageConfig& config)
{
    if (m_repackEngine) {
        noty::Logger::instance().warning("Repack already in progress");
        return;
    }

    // Start performance monitoring
    noty::PerformanceMonitor::instance().startOperation("Repack",
        m_mainWindow->getTotalSize(),
        m_mainWindow->getFileCount());

    // Create repack engine in a separate thread
    m_repackThread = new QThread(this);
    m_repackEngine = new noty::RepackEngine();

    // Use resource manager for optimal settings
    auto& resourceManager = noty::ResourceManager::instance();
    m_repackEngine->setThreadPoolSize(resourceManager.getThreadPoolSize());
    m_repackEngine->setCompressionBufferSize(resourceManager.getCompressionBufferSize());

    // Move engine to thread using a lambda since RepackEngine is not a QObject
    QThread* thread = m_repackThread;
    noty::RepackEngine* engine = m_repackEngine;

    connect(m_repackThread, &QThread::started, [this, config, engine]() {
        // Create job configuration
        noty::RepackJob::Configuration jobConfig;
        jobConfig.sourceDirectory = config.sourceDirectory.toStdString();
        jobConfig.outputDirectory = config.outputDirectory.toStdString();
        jobConfig.gameName = config.gameName.toStdString();
        jobConfig.gameVersion = config.gameVersion.toStdString();
        jobConfig.repackerName = config.repackerName.toStdString();
        jobConfig.setupName = config.setupName.toStdString();
        jobConfig.compressionLevel = config.compressionLevel;
        jobConfig.compressionBufferSize =
            noty::ResourceManager::instance().getCompressionBufferSize();
        jobConfig.maxChunkSize =
            noty::ResourceManager::instance().calculateOptimalChunkSize(config.chunkSize);
        jobConfig.enableEncryption = config.enableEncryption;
        jobConfig.includeHiddenFiles = config.includeHiddenFiles;
        jobConfig.generateSetup = config.generateSetup;
        jobConfig.coverImagePath = config.coverImagePath.toStdString();
        jobConfig.hashAlgorithm = "BLAKE3";

        noty::RepackJob job(jobConfig);
        engine->startJob(std::move(job));
        });

    connect(m_repackThread, &QThread::finished, [this]() {
        if (m_repackEngine) {
            delete m_repackEngine;
            m_repackEngine = nullptr;
        }
        noty::PerformanceMonitor::instance().stopOperation();
        });

    m_repackThread->start();
}

void RepackerApplication::onRepackCancelled()
{
    if (m_repackEngine) {
        m_repackEngine->cancelCurrentJob();
        noty::Logger::instance().info("Repack cancellation requested");
        noty::PerformanceMonitor::instance().stopOperation();
    }
}

void RepackerApplication::startRepack(const PackageConfig& config)
{
    emit repackStarted(config);
}

void RepackerApplication::cancelRepack()
{
    emit repackCancelled();
}

bool RepackerApplication::isRepacking() const
{
    return m_repackEngine != nullptr && m_repackEngine->isBusy();
}