#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QDebug>
#include "shamodel.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCoreApplication::setOrganizationName(APP_COMPANY);
    QCoreApplication::setOrganizationDomain(APP_COPYRIGHT);
    QCoreApplication::setApplicationName(APP_PRODUCT);
    setlocale(LC_ALL, "");

    QCommandLineParser parser;
    parser.process(a);
    const QStringList fileList = parser.positionalArguments();
    for (const QString &file : fileList) {
        SHAModel model;
        model.setFilePath(file);
        model.loadSha();
        model.saveJsonToFile();
    }

    return a.exec();
}
