#include "shamodel.h"
#include <QDebug>
#include <QFile>
#include <QRegExp>
#include <QStringList>

SHAModel::SHAModel(const QString &filePath, QObject *parent)
    : QObject(parent)
    , m_filePath(filePath)
{
}

bool SHAModel::loadSha()
{
    if (!loadFile())
        return false;

    if (!parse())
        return false;

    return true;
}

QString SHAModel::findBlock(const QString &line, const QString &beginTok,
                            const QString &endTok)
{
    const qint32 beginTokLen = beginTok.length();
    const auto begin = line.indexOf(beginTok);
    const auto end = line.lastIndexOf(endTok);
    const qint32 count = end - begin;
    return line.mid(begin + beginTokLen, count - beginTokLen);
}

QStringList SHAModel::findMultiBlock(QString &str, const QString &beginTok,
                                     const QString &endTok, bool cutBlock,
                                     bool removeTok)
{
    qint32 index = 0;
    QStringList list;
    if (str.isEmpty())
        return list;

    while (true) {
        int beginTokLen = beginTok.length();
        int endTokLen = endTok.length();
        auto begin = str.indexOf(beginTok, index);
        if (begin == -1)
            break;

        auto end = str.indexOf(endTok, begin);
        if (end == -1)
            break;

        qint32 count = end - begin;
        list << str.mid(begin + beginTokLen, count - beginTokLen);
        if (cutBlock) {
            if (removeTok) {
                str = str.remove(begin, count + beginTokLen);
                index = end - count;
            } else {
                str = str.remove(begin + beginTokLen, count - beginTokLen);
                index = end - count + beginTokLen + endTokLen;
            }
        } else {
            index = end + endTokLen;
        }
    }

    return list;
}

bool SHAModel::getLinkParams(const QString &line, QString &thisPoint,
                             QString &targetPoint, qint32 &targetId,
                             QStringList &nodes)
{
    QStringList res;
    const QString link = findBlock(line, "link(", ")");
    const QStringList blockList = link.split(':');
    if (blockList.size() < 2)
        return false;

    // Block1
    const QStringList block1List = blockList.at(0).split(',');
    if (block1List.size() < 2)
        return false;

    thisPoint = block1List.at(0);
    targetId = block1List.at(1).toInt();

    // Block2
    QString block2 = blockList.at(1);
    nodes = findMultiBlock(block2, "(", ")", true, true);
    const QStringList block2list = block2.split(',');
    if (block2list.empty())
        return false;
    targetPoint = block2list.at(0);

    return true;
}

SHAModel::LineType SHAModel::getTypeLine(const QString &line)
{
    const QString trimmedLine = line.trimmed();
    auto checkPattern = [&trimmedLine](const QString &pattern) {
        return QRegExp(pattern, Qt::CaseSensitive, QRegExp::WildcardUnix)
            .exactMatch(trimmedLine);
    };

    // Ignore
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

bool SHAModel::loadFile()
{
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    m_fileContent = QString::fromLocal8Bit(file.readAll()).split("\r\n");
    return true;
}

bool SHAModel::parse()
{
    // m_fileContent

    return true;
}
