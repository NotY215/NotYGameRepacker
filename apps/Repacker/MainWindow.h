#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include <memory>
#include <vector>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QListWidget>
#include <QGroupBox>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QProgressBar;
class QTextEdit;
class QLineEdit;
class QComboBox;
class QSpinBox;
class QCheckBox;
class QListWidget;
class QGroupBox;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    // Package configuration
    struct PackageConfig {
        QString sourceDirectory;
        QString outputDirectory;
        QString gameName;
        QString gameVersion;
        QString repackerName;
        QString setupName;
        QString coverImagePath;
        int compressionLevel = 19;
        uint64_t chunkSize = 1024 * 1024 * 1024; // 1GB
        bool enableEncryption = false;
        bool includeHiddenFiles = false;
        bool generateSetup = true;
        std::vector<std::string> components;
    };

    const PackageConfig& getConfig() const { return m_config; }
    uint64_t getTotalSize() const { return m_totalSize; }
    uint64_t getFileCount() const { return m_fileCount; }

signals:
    void repackStarted(const PackageConfig& config);
    void repackCancelled();

private slots:
    void onContinueClicked();
    void onBackClicked();
    void onExitClicked();
    void onBrowseSourceClicked();
    void onBrowseOutputClicked();
    void onBrowseCoverClicked();
    void onStartRepackClicked();
    void onCancelRepackClicked();
    void onFinishClicked();
    void onScanSourceDirectory();
    void onCoverImageSelected();

    // Progress updates from repack engine
    void onRepackProgress(int percent, const QString& status);
    void onRepackComplete(bool success, const QString& message);

private:
    void setupUi();
    void loadFonts();
    void setupWelcomePage();
    void setupSourcePage();
    void setupCoverPage();
    void setupConfigurationPage();
    void setupReviewPage();
    void setupProgressPage();
    void setupCompletePage();

    void showPage(int index);
    void updateNavigationButtons();
    void updateReviewPage();
    void updateConfigurationPage();
    void validateInputs();
    bool validateSourceDirectory();
    bool validateOutputDirectory();
    bool validateGameName();
    bool validateSetupName();
    void updateFileInfo(const QString& dir);

    void startRepacking();
    void cancelRepacking();

    // UI Components
    QStackedWidget* m_stackedWidget;
    QPushButton* m_backButton;
    QPushButton* m_nextButton;
    QPushButton* m_cancelButton;

    // Page widgets
    QWidget* m_welcomePage;
    QWidget* m_sourcePage;
    QWidget* m_coverPage;
    QWidget* m_configurationPage;
    QWidget* m_reviewPage;
    QWidget* m_progressPage;
    QWidget* m_completePage;

    // Source page
    QLineEdit* m_sourceDirEdit;
    QLabel* m_fileCountLabel;
    QLabel* m_dirCountLabel;
    QLabel* m_totalSizeLabel;
    QGroupBox* m_infoGroup;
    QPushButton* m_browseSourceBtn;

    // Cover page
    QLineEdit* m_coverPathEdit;
    QLabel* m_coverPreviewLabel;
    QPushButton* m_browseCoverBtn;

    // Configuration page
    QLineEdit* m_gameNameEdit;
    QLineEdit* m_gameVersionEdit;
    QLineEdit* m_repackerNameEdit;
    QLineEdit* m_setupNameEdit;
    QComboBox* m_compressionLevelCombo;
    QSpinBox* m_chunkSizeSpin;
    QCheckBox* m_encryptionCheck;
    QCheckBox* m_hiddenFilesCheck;
    QCheckBox* m_generateSetupCheck;
    QLineEdit* m_outputDirEdit;
    QPushButton* m_browseOutputBtn;

    // Review page
    QLabel* m_reviewSourceLabel;
    QLabel* m_reviewOutputLabel;
    QLabel* m_reviewGameNameLabel;
    QLabel* m_reviewGameVersionLabel;
    QLabel* m_reviewRepackerLabel;
    QLabel* m_reviewSetupNameLabel;
    QLabel* m_reviewCoverLabel;
    QLabel* m_reviewCompressionLabel;
    QLabel* m_reviewChunkSizeLabel;
    QLabel* m_reviewEncryptionLabel;
    QLabel* m_reviewHiddenFilesLabel;
    QLabel* m_reviewGenerateSetupLabel;

    // Progress page
    QProgressBar* m_progressBar;
    QLabel* m_progressStatusLabel;
    QTextEdit* m_progressLogText;
    QPushButton* m_cancelRepackBtn;

    // Complete page
    QLabel* m_completeIcon;
    QLabel* m_completeMessage;
    QLabel* m_completeDetailsLabel;
    QLabel* m_packageSizeLabel;

    // Page indices
    enum Pages {
        WelcomePage = 0,
        SourcePage,
        CoverPage,
        ConfigurationPage,
        ReviewPage,
        ProgressPage,
        CompletePage
    };

    int m_currentPage = 0;
    bool m_isRepacking = false;
    bool m_repackComplete = false;
    bool m_repackSuccess = false;

    PackageConfig m_config;
    uint64_t m_totalSize = 0;
    uint64_t m_fileCount = 0;
    uint64_t m_directoryCount = 0;
};