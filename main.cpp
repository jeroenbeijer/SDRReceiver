#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <iostream>

QString MainWindow::settings_filename;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QApplication::setApplicationName("SDRReceiver");
    QApplication::setApplicationVersion("0.1");

    QCommandLineParser cmdparser;
    cmdparser.setApplicationDescription("SDR Receiver for JAERO");
    cmdparser.addHelpOption();
    cmdparser.addVersionOption();

    QCommandLineOption settingsnameoption(QStringList() << "s" << "settings-filename",QApplication::translate("main", "Run with setting file name <file name>."),QApplication::translate("main", "name"));
    settingsnameoption.setDefaultValue("");
    cmdparser.addOption(settingsnameoption);

    cmdparser.process(a);

    MainWindow::settings_filename=cmdparser.value(settingsnameoption);

    MainWindow w;
    w.setWindowTitle("SDRReceiver - "  + MainWindow::settings_filename );
    w.show();
    return a.exec();
}
