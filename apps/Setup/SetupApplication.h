#pragma once
#include <QObject>
#include <memory>
#include <string>

class SetupWindow;

class SetupApplication : public QObject
{
    Q_OBJECT
public:
    explicit SetupApplication(QObject* parent = nullptr);
    ~SetupApplication();

    bool initialize();
    void shutdown();

    std::string getLastError() const { return m_lastError; }

    // Package information
    std::string getPackagePath() const { return m_packagePath; }
    void setPackagePath(const std::string& path) { m_packagePath = path; }

    // Installation settings
    std::string getInstallDirectory() const { return m_installDirectory; }
    void setInstallDirectory(const std::string& dir) { m_installDirectory = dir; }

    // Component selection
    const std::vector<std::string>& getSelectedComponents() const { return m_selectedComponents; }
    void setSelectedComponents(const std::vector<std::string>& components) { m_selectedComponents = components; }
    void addSelectedComponent(const std::string& component) { m_selectedComponents.push_back(component); }
    void removeSelectedComponent(const std::string& component);

    bool isComponentSelected(const std::string& component) const;

    // Shortcut options
    bool createDesktopShortcut() const { return m_createDesktopShortcut; }
    void setCreateDesktopShortcut(bool create) { m_createDesktopShortcut = create; }

    bool createStartMenuShortcut() const { return m_createStartMenuShortcut; }
    void setCreateStartMenuShortcut(bool create) { m_createStartMenuShortcut = create; }

    // Encryption settings
    bool isEncryptionEnabled() const { return m_encryptionEnabled; }
    void setEncryptionEnabled(bool enabled) { m_encryptionEnabled = enabled; }

    const std::vector<uint8_t>& getEncryptionKey() const { return m_encryptionKey; }
    void setEncryptionKey(const std::vector<uint8_t>& key) { m_encryptionKey = key; }

    const std::vector<uint8_t>& getEncryptionNonce() const { return m_encryptionNonce; }
    void setEncryptionNonce(const std::vector<uint8_t>& nonce) { m_encryptionNonce = nonce; }

    // Progress
    int getProgress() const { return m_progress; }
    void setProgress(int progress) { m_progress = progress; }

    QString getStatusMessage() const { return m_statusMessage; }
    void setStatusMessage(const QString& message) { m_statusMessage = message; }

    // Package info from manifest
    QString getGameName() const { return m_gameName; }
    QString getGameVersion() const { return m_gameVersion; }
    QString getRepackerName() const { return m_repackerName; }
    uint64_t getPackageSize() const { return m_packageSize; }

signals:
    void progressUpdated(int percent, const QString& status);
    void installationComplete(bool success, const QString& message);
    void installationCancelled();

public slots:
    void startInstallation();
    void cancelInstallation();

private slots:
    void onInstallationProgress(int percent, const QString& status);
    void onInstallationComplete(bool success, const QString& message);

private:
    bool loadPackageInfo();
    bool validatePackage();
    bool prepareInstallation();

    std::unique_ptr<SetupWindow> m_mainWindow;

    std::string m_packagePath;
    std::string m_installDirectory;
    std::vector<std::string> m_selectedComponents;
    bool m_createDesktopShortcut = false;
    bool m_createStartMenuShortcut = false;
    bool m_encryptionEnabled = false;
    std::vector<uint8_t> m_encryptionKey;
    std::vector<uint8_t> m_encryptionNonce;

    QString m_gameName;
    QString m_gameVersion;
    QString m_repackerName;
    uint64_t m_packageSize = 0;

    int m_progress = 0;
    QString m_statusMessage;
    std::string m_lastError;

    bool m_installationRunning = false;
    bool m_installationCancelled = false;
};