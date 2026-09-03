================================================================================
FILE: apps / Setup / SetupWindow.h
================================================================================
#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include <memory>
#include <vector>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QProgressBar;
class QLineEdit;
class QListWidget;
class QCheckBox;
class QGroupBox;
class QTextEdit;
class QVBoxLayout;
class QHBoxLayout;
QT_END_NAMESPACE

class SetupWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit SetupWindow(QWidget* parent = nullptr);
    ~SetupWindow();

    void setPackageInfo(const QString& gameName, const QString& version,
        const QString& repacker, uint64_t size);

    void updateProgress(int percent, const QString& status);
    void updateFileProgress(const QString& filename, int percent);
    void updateVerificationProgress(int percent, const QString& status);
    void setInstallationComplete(bool success, const QString& message);

signals:
    void installationStarted();
    void installationCancelled();

private slots:
    void onContinueClicked();
    void onBackClicked();
    void onCancelClicked();
    void onInstallClicked();
    void onBrowseClicked();
    void onFinishClicked();
    void onComponentSelectionChanged();

private:
    void setupUi();
    void loadFonts();
    void setupWelcomePage();
    void setupLocationPage();
    void setupComponentsPage();
    void setupInstallPage();
    void setupVerificationPage();
    void setupFinishPage();

    void showPage(int index);
    void updateNavigationButtons();
    void showWelcomePage();
    void showLocationPage();
    void showComponentsPage();
    void showInstallPage();
    void showVerificationPage();
    void showFinishPage();

    void updateDiskSpaceInfo();
    void updateComponentsList();
    void updateComponentSizes();

    // UI Components
    QStackedWidget* m_stackedWidget;
    QPushButton* m_backButton;
    QPushButton* m_nextButton;
    QPushButton* m_cancelButton;

    // Page widgets
    QWidget* m_welcomePage;
    QWidget* m_locationPage;
    QWidget* m_componentsPage;
    QWidget* m_installPage;
    QWidget* m_verificationPage;
    QWidget* m_finishPage;

    // Location page
    QLineEdit* m_installDirEdit;
    QLabel* m_diskSpaceLabel;

    // Components page
    QListWidget* m_componentsList;
    QLabel* m_requiredSizeLabel;

    // Install page
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;
    QLabel* m_fileProgressLabel;

    // Verification page
    QProgressBar* m_verificationProgress;

    // Finish page
    QLabel* m_completeIcon;
    QLabel* m_completeMessage;
    QCheckBox* m_launchCheckbox;

    // Page indices
    enum Pages {
        WelcomePage = 0,
        LocationPage,
        ComponentsPage,
        InstallPage,
        VerificationPage,
        FinishPage
    };

    int m_currentPage = 0;
    bool m_isInstalling = false;
    bool m_installationComplete = false;

    // Package info
    QString m_gameName;
    QString m_gameVersion;
    QString m_repackerName;
    uint64_t m_packageSize = 0;
};