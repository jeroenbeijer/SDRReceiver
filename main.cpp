#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <iostream>
#include <QTimer>

QString MainWindow::settings_filename;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QApplication::setApplicationName("SDRReceiver");
    QApplication::setApplicationVersion("1.0");

    QCommandLineParser cmdparser;
    cmdparser.setApplicationDescription("SDR Receiver for JAERO");
    cmdparser.addHelpOption();
    cmdparser.addVersionOption();

    // ini file option (-s)
    QCommandLineOption settingsnameoption(QStringList() << "s" << "settings-filename",QApplication::translate("main", "Run with setting file name <file name>."),QApplication::translate("main", "name"));
    settingsnameoption.setDefaultValue("");
    cmdparser.addOption(settingsnameoption);

    if(a.arguments().size()<=1)
    {
        fprintf(stderr, "%s\n", qPrintable(QCoreApplication::translate("main", "Error: Must specify an argument.")));
        cmdparser.showHelp(1);
    }

    MainWindow::settings_filename=cmdparser.value(settingsnameoption);
    MainWindow w;
    w.setWindowTitle("SDRReceiver - "  + MainWindow::settings_filename );
    w.show();
    return a.exec();
}
