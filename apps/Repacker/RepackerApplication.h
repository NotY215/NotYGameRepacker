#pragma once
#include <QObject>
#include <memory>

class MainWindow;

namespace noty {
    class RepackEngine;
}

class RepackerApplication : public QObject
{
    Q_OBJECT
public:
    explicit RepackerApplication(QObject* parent = nullptr);
    ~RepackerApplication();

    void initialize();
    void shutdown();

    void startRepack(const MainWindow::PackageConfig& config);
    void cancelRepack();
    bool isRepacking() const;

signals:
    void repackStarted(const MainWindow::PackageConfig& config);
    void repackCancelled();

private slots:
    void onRepackStarted(const MainWindow::PackageConfig& config);
    void onRepackCancelled();

private:
    std::unique_ptr<MainWindow> m_mainWindow;
    noty::RepackEngine* m_repackEngine = nullptr;
    QThread* m_repackThread = nullptr;
};