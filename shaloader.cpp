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

QString SHALoader::getTokBlock(const QString &line, const QString &beginTok, const QString &endTok)
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

    bool openBlock = false;

    for (const QString v : m_fileContent) {
        const QString &&line = v.trimmed();
        LineType type = getTypeLine(line);

        switch (type) {
        case LineType::Undefined:
            qInfo() << "String:" << line;
            qInfo() << "File:" << m_filePath;

            return false;
        case LineType::Make:
            m_package = getTokBlock(line, "Make(", ")");
            continue;

        case LineType::Ver:
            m_version = getTokBlock(line, "ver(", ")");
            continue;

        case LineType::Add:
            if (openBlock == true) {
                qWarning() << "Отсутствует фигурная скобка \"}\" закрывающая блок!";
                return false;
            }

            elementParams = getTokBlock(line, "Add(", ")").split(',');
            state = LineType::Add;
            continue;

        case LineType::Ignore:
            continue;
        }

        switch (state) {
        case LineType::Add: {
            if (openBlock == false) {
                if (type == LineType::OpenBlock) {
                    openBlock = true;
                    continue;
                } else {
                    qWarning() << "Отсутствует фигурная скобка \"{\" открывающая блок!";
                    return false;
                }
            }
            //Начало OpenBlock

            switch (type) {
            case LineType::Link:
                qInfo() << getTokBlock(line, "link(", ")");

                continue;
                break;
            case LineType::CloseBlock:
                openBlock = false;
                state = LineType::Null;
                continue;
                break;
            }
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
