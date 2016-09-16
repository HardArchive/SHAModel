#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QDebug>
#include <QElapsedTimer>
#include "shamodel.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCoreApplication::setOrganizationName(APP_COMPANY);
    QCoreApplication::setOrganizationDomain(APP_COPYRIGHT);
    QCoreApplication::setApplicationName(APP_PRODUCT);
    setlocale(LC_ALL, "");

    QElapsedTimer timer;
    QCommandLineParser parser;
    parser.process(a);
    const QStringList fileList = parser.positionalArguments();
    for (const QString &file : fileList) {
        SHAModel model;
        model.setFilePath(file);
        model.loadSha();
        timer.start();
        model.saveJsonToFile();
        qInfo() << timer.elapsed() << "ms";
    }

    return a.exec();
}
