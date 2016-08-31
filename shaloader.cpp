#include "shaloader.h"
#include <QDebug>
#include <QFile>
#include <QRegExp>

SHALoader::SHALoader(const QString &filePath, QObject *parent)
    : QObject(parent)
    , m_filePath(filePath)
{
}

bool SHALoader::loadSha()
{
    if (!loadFile())
        return false;

    if (!parse())
        return false;

    return true;
}

bool SHALoader::loadFile()
{
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    m_fileContent = QString::fromLocal8Bit(file.readAll()).split("\r\n");
    return true;
}

bool SHALoader::parse()
{
    int i = 0;
    for (const QString &line : m_fileContent) {
        ++i;
        auto type = getTypeLine(line);
        if (type == LineType::Undefined) {
            qInfo() << "NumLine:" << i;
            qInfo() << "Type:" << type;
            qInfo() << "String:" << line;
            qInfo() << "File:" << m_filePath;

            return false;
        }
    }

    return true;
}

SHALoader::LineType SHALoader::getTypeLine(const QString &line)
{
    const QString &&trimmedLine = line.trimmed();
    auto checkPattern = [&trimmedLine](const QString &pattern) {
        return QRegExp(pattern, Qt::CaseSensitive, QRegExp::WildcardUnix)
            .exactMatch(trimmedLine);
    };

    if (checkPattern("Make(*)"))
        return LineType::Make;
    if (checkPattern("Add(*)"))
        return LineType::Add;
    if (checkPattern("{"))
        return LineType::OpenBlock;
    if (checkPattern("}"))
        return LineType::CloseBlock;
    if (checkPattern("link(*)"))
        return LineType::Link;
    if (checkPattern("Point(*)"))
        return LineType::Point;
    if (checkPattern("*=*"))
        return LineType::Prop;
    if (checkPattern("BEGIN_SDK"))
        return LineType::BEGIN_SDK;
    if (checkPattern("END_SDK"))
        return LineType::END_SDK;
    if (trimmedLine.isEmpty())
        return LineType::Empty;

    //Ignore
    if (checkPattern("AddHint(*"))
        return LineType::Ignore;
    if (checkPattern("ver(*)"))
        return LineType::Ignore;
    if (checkPattern("\\**"))
        return LineType::Ignore;
    if (checkPattern("@*=*"))
        return LineType::Ignore;
    if (checkPattern("Pos(*)"))
        return LineType::Ignore;
    if (checkPattern("elink(*)"))
        return LineType::Ignore;
    if (checkPattern("MakeExt(*)"))
        return LineType::Ignore;
    if (checkPattern("PColor(*)"))
        return LineType::Ignore;
    if (checkPattern("MakeTrans(*)"))
        return LineType::Ignore;

    return LineType::Undefined;
}
