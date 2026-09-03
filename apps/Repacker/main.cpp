#include <QApplication>
#include <QMessageBox>
#include "RepackerApplication.h"
#include "noty/common/Logger.h"

int main(int argc, char* argv[])
{
    try {
        QApplication app(argc, argv);
        app.setApplicationName("NotY Repacker");
        app.setOrganizationName("NotY215");
        app.setApplicationVersion("1.0.0");

        // Log startup
        noty::Logger::instance().info("NotY Game Repacker starting...");

        RepackerApplication repacker;
        repacker.initialize();

        int result = app.exec();
        noty::Logger::instance().info("Application exited normally.");
        return result;
    }
    catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Fatal Error",
            QString("Application failed to start:\n%1").arg(e.what()));
        return 1;
    }
}