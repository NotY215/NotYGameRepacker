#pragma once
#include <QObject>
#include <memory>

class MainWindow;

namespace noty {
    class RepackEngine;
}

// Register the PackageConfig type for Qt signals
struct PackageConfig;

class RepackerApplication : public QObject
{
    Q_OBJECT
public:
    explicit RepackerApplication(QObject* parent = nullptr);
    ~RepackerApplication();

    void initialize();
    void shutdown();

    bool isRepacking() const;

public slots:
    void startRepack(const PackageConfig& config);
    void cancelRepack();

signals:
    void repackStarted(const PackageConfig& config);
    void repackCancelled();

private slots:
    void onRepackStarted(const PackageConfig& config);
    void onRepackCancelled();

private:
    std::unique_ptr<MainWindow> m_mainWindow;
    noty::RepackEngine* m_repackEngine = nullptr;
    QThread* m_repackThread = nullptr;
};

// Forward declaration of PackageConfig from MainWindow
struct PackageConfig;