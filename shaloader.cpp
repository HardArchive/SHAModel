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

QString SHALoader::getBlockTok(const QString &line, const QString &beginTok, const QString &endTok)
{
    int beginTokLen = beginTok.length();
    auto begin = line.indexOf(beginTok) + beginTokLen;
    auto end = line.lastIndexOf(endTok) - beginTokLen;
    return line.mid(begin, end);
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
    LineType state = LineType::Null;
    QStringList elementParams;

    for (const QString v : m_fileContent) {
        const QString &&line = v.trimmed();

        switch (getTypeLine(line)) {
        case LineType::Undefined:
            qInfo() << "String:" << line;
            qInfo() << "File:" << m_filePath;

            return false;
        case LineType::Make: {
            m_package = getBlockTok(line, "Make(", ")");
            break;
        }
        case LineType::Ver: {
            m_version = getBlockTok(line, "ver(", ")");
            break;
        }
        case LineType::Add: {
            elementParams = getBlockTok(line, "Add(", ")").split(',');
            state = LineType::Add;
            break;
        }
        default:
            break;
        }

        switch (state) {
        case LineType::Add: {
        }
        case LineType::BEGIN_SDK: {
        }
        case LineType::END_SDK: {
        }
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

    //Ignore
    if (checkPattern("AddHint(*"))
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

    //Действующие
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
    if (checkPattern("*=*"))
        return LineType::Prop;
    if (checkPattern("BEGIN_SDK"))
        return LineType::BEGIN_SDK;
    if (checkPattern("END_SDK"))
        return LineType::END_SDK;
    if (trimmedLine.isEmpty())
        return LineType::Empty;

    return LineType::Undefined;
}
