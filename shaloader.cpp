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
        auto type = getLineType(line);
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

SHALoader::LineType
SHALoader::getLineType(const QString &str)
{
    const QString &&trimmed = str.trimmed();
    auto checkPattern = [&](const QString &pattern) {
        return QRegExp(pattern, Qt::CaseSensitive, QRegExp::WildcardUnix)
            .exactMatch(trimmed);
    };

    if (checkPattern("Make(*)"))
        return LineType::Make;
    if (checkPattern("ver(*)"))
        return LineType::Ver;
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
    if (checkPattern("@Hint=*"))
        return LineType::Hint;
    if (checkPattern("AddHint(*"))
        return LineType::AddHint;
    if (checkPattern("\\**"))
        return LineType::Comment;
    if (checkPattern("Pos(*)"))
        return LineType::Pos;
    if (checkPattern("*=*"))
        return LineType::Prop;
    if (checkPattern("BEGIN_SDK"))
        return LineType::BEGIN_SDK;
    if (checkPattern("END_SDK"))
        return LineType::END_SDK;
    if (trimmed.isEmpty())
        return LineType::Empty;

    return LineType::Undefined;
}
