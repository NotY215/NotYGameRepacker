#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include "SetupApplication.h"
#include "noty/common/Logger.h"

int main(int argc, char* argv[])
{
    try {
        QApplication app(argc, argv);
        app.setApplicationName("NotY Game Installer");
        app.setOrganizationName("NotY215");
        app.setApplicationVersion("1.0.0");

        // Log startup
        noty::Logger::instance().info("NotY Game Installer starting...");

        SetupApplication installer;
        if (!installer.initialize()) {
            QMessageBox::critical(nullptr, "Initialization Error",
                "Failed to initialize the installer.\n\n" +
                QString::fromStdString(installer.getLastError()));
            return 1;
        }

        int result = app.exec();
        noty::Logger::instance().info("Installer exited normally.");
        return result;
    }
    catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Fatal Error",
            QString("Installer failed to start:\n%1").arg(e.what()));
        return 1;
    }
}