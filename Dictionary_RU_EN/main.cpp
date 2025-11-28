#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>
#include <QPalette>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Try multiple methods to set icon
    QIcon appIcon;

    // Method 1: From resource
    if (QFile::exists(":/images/app_icon.ico")) {
        appIcon = QIcon(":/images/app_icon.ico");
    }
    // Method 2: From external file
    else if (QFile::exists(":images/app_icon.ico")) {
        appIcon = QIcon(":images/app_icon.ico");
    }
    // Method 3: From PNG
    else if (QFile::exists(":images/app_icon.png")) {
        appIcon = QIcon(":images/app_icon.png");
    }
    // Method 4: Use built-in Qt icon as fallback
    else {
        appIcon = QIcon::fromTheme("help-contents");
        if (appIcon.isNull()) {
            appIcon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
        }
    }

    app.setWindowIcon(appIcon);

    // Set application properties
    app.setApplicationName("Ru-En");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("YourCompany");

    // Set modern Fusion style
    app.setStyle(QStyleFactory::create("Fusion"));

    // Optional: Set a dark theme for better appearance
//    QPalette darkPalette;
//    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
//    darkPalette.setColor(QPalette::WindowText, Qt::white);
//    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
//    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
//    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
//    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
//    darkPalette.setColor(QPalette::Text, Qt::white);
//    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
//    darkPalette.setColor(QPalette::ButtonText, Qt::white);
//    darkPalette.setColor(QPalette::BrightText, Qt::red);
//    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
//    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
//    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

//    app.setPalette(darkPalette);
//    app.setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");

    // Create and show main window
    MainWindow window;
    window.show();

    return app.exec();
}
