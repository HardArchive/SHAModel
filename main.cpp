#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include "shaloader.h"

QStringList loadFiles(const QString &startDir, QStringList filters)
{
    QDir dir(startDir);
    QStringList list;

    foreach (QString file, dir.entryList(filters, QDir::Files))
        list += startDir + "/" + file;

    foreach (QString subdir, dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot))
        list += loadFiles(startDir + "/" + subdir, filters);
    return list;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    setlocale(LC_ALL, "");

    QString examples = "D:/Users/Admin/AppData/Roaming/HiAsm_AltBuild/Elements/delphi/Example";
    QString ex2 = "D:/dev/Qt/MainProjects/SHALoader/test.sha";

    //auto list = loadFiles(examples, { "*.sha", "*.SHA" });
    //
    //for (auto var : list) {
    //    SHALoader loader(var);
    //    if (!loader.loadSha())
    //        break;
    //}
    SHALoader loader(ex2);
    loader.loadSha();

    return a.exec();
}
