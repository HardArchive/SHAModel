#include "project.h"
#include <QFile>
#include <QDebug>
#include <QRegExp>

Project::Project(const QString &filePath, QObject *parent)
    : QObject(parent)
    , m_filePath(filePath)
{
    load();
    parse();
}

bool Project::load()
{
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    m_fileContent = QString(file.readAll()).split("\r\n");
    return true;
}

void Project::parse()
{
    int i = 0;
    for (const QString &s : m_fileContent) {
        ++i;
        qInfo() << i << ":" << getLineType(s);
    }
}

Project::LineType Project::getLineType(const QString &str)
{
    const QString &&trimmed = str.trimmed();
    auto checkPattern = [&](const QString &pattern) {
        return QRegExp(pattern, Qt::CaseSensitive, QRegExp::Wildcard).exactMatch(trimmed);
    };

    if (checkPattern("*Make(*)"))
        return LineType::Make;
    if (checkPattern("*ver(*)"))
        return LineType::Ver;
    if (checkPattern("*Add(*)"))
        return LineType::Add;
    if (checkPattern("*{*"))
        return LineType::OpenBlock;
    if (checkPattern("*}*"))
        return LineType::CloseBlock;
    if (checkPattern("*link(*)"))
        return LineType::Link;
    if (checkPattern("*Point(*)"))
        return LineType::Point;
    if (checkPattern("*@Hint=*"))
        return LineType::Hint;
    if (checkPattern("*AddHint(*"))
        return LineType::AddHint;
    if (checkPattern("*=*"))
        return LineType::Prop;
    if (checkPattern("*BEGIN_SDK"))
        return LineType::BEGIN_SDK;
    if (checkPattern("*END_SDK"))
        return LineType::END_SDK;
    if (str.)

        return LineType::Undefined;
}
