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

QVariantMap SHAModel::linkToVariantMap(const QString &line)
{
    QVariantMap map;
    QStringList res;
    const QString link = findBlock(line, "link(", ")");
    const QStringList blockList = link.split(':');
    if (blockList.size() < 2)
        return QVariantMap();

    // Block1
    const QStringList block1List = blockList.at(0).split(',');
    if (block1List.size() < 2)
        return QVariantMap();

    // Block2
    QString block2 = blockList.at(1);
    const QStringList nodes = findMultiBlock(block2, "(", ")", true, true);
    const QStringList block2list = block2.split(',');
    if (block2list.empty())
        return QVariantMap();

    map.insert("thisPoint", block1List.at(0));
    map.insert("targetId", block1List.at(1).toInt());
    map.insert("targetPoint", block2list.at(0));

    if (!nodes.isEmpty())
        map.insert("nodes", nodes);

    return map;
}

SHAModel::LineType SHAModel::getTypeLine(const QString &line)
{
    auto checkPattern = [&line](const QString &pattern) {
        return QRegExp(pattern, Qt::CaseSensitive, QRegExp::WildcardUnix)
            .exactMatch(line);
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
    if (line.isEmpty())
        return LineType::Empty;

    return LineType::Undefined;
}

SHAModel::LineType SHAModel::getTypeLine(const QStringList &content, int idx)
{
    int size = content.size();
    if ((idx < 0) || (idx > (size - 1)))
        return LineType::Null;

    return getTypeLine(content[idx]);
}

bool SHAModel::loadFile()
{
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QStringList tmpContent = QString::fromLocal8Bit(file.readAll()).split("\r\n");
    QStringList content;
    for (QString &s : tmpContent) {
        QString tmp = s.trimmed();
        if (!tmp.isEmpty())
            m_content << tmp;
    }

    return true;
}

QVariantMap SHAModel::parseElementBlock(const QStringList &block)
{
    QVariantMap element;
    QVariantList linkList;
    const int size = block.size();
    for (int iEBlock = 0; iEBlock < size; ++iEBlock) {
        const LineType type = getTypeLine(block, iEBlock);

        switch (type) {
        case LineType::Add: {
            QStringList params = findBlock(block[iEBlock], "Add(", ")").split(',');
            if (params.size() < 4) {
                qWarning() << "К-во аргументов меньше 4-х.";
                return QVariantMap();
            }

            //Основные параметры элемента
            element.insert("name", params[0]);
            element.insert("id", params[1].toInt());
            element.insert("x", params[2].toInt());
            element.insert("y", params[3].toInt());

            //Остальные параметры элемента
            for (int iEParam = iEBlock + 1; iEParam < size; ++iEParam) {
                switch (getTypeLine(block, iEParam)) {
                case OpenBlock:
                    continue;
                case Link: {
                    const QVariantMap link = linkToVariantMap(block[iEParam]);
                    if (link.isEmpty()) {
                        qWarning() << "Ошибка разбора параметров link(*)";
                        return QVariantMap();
                    }
                    linkList.append(link);
                    continue;
                }
                case Point:
                    continue;
                case Prop:
                    continue;
                case CloseBlock: {
                    if (!linkList.isEmpty())
                        element.insert("linkList", linkList);

                    //Если элемент является контейнером
                    if (getTypeLine(block, iEParam + 1) == LineType::BEGIN_SDK) {
                        const auto container = parseContent(block.mid(iEParam + 2, size - iEParam - 2 - 1));
                        element.insert("container", container);
                    }

                    return element;
                }
                default:
                    break;
                }
                break;
            }
        }
        case LineType::Ignore:
            continue;

        default:
            continue;
        }
    }
    return element;
}

QVariantMap SHAModel::getElementBlock(const QStringList &content, int &begin)
{
    int tmpBegin = begin;
    int size = content.size();
    for (int eBlock = begin + 1; eBlock < size; ++eBlock) {
        if (getTypeLine(content, eBlock) == LineType::CloseBlock) {
            if (getTypeLine(content, eBlock + 1) == LineType::BEGIN_SDK) {
                int sdk = 0;
                for (int cBlock = eBlock + 1; cBlock < size; ++cBlock) {
                    LineType type = getTypeLine(content, cBlock);
                    if (type == LineType::BEGIN_SDK) {
                        ++sdk;
                    } else if (type == LineType::END_SDK) {
                        --sdk;
                        if (sdk == 0) {
                            begin = cBlock;
                            return parseElementBlock(content.mid(tmpBegin, cBlock - tmpBegin + 1));
                        }
                    }
                }
            } else {
                begin = eBlock;
                return parseElementBlock(content.mid(tmpBegin, eBlock - tmpBegin + 1));
            }
        }
    }
    return QVariantMap();
}

QVariantList SHAModel::parseContent(const QStringList &content)
{
    QVariantList elementList;
    int size = content.size();
    for (int i = 0; i < size; ++i) {
        const LineType type = getTypeLine(content[i]);

        switch (type) {
        case LineType::Undefined:
            return QVariantList();
        case LineType::Add: {
            elementList << getElementBlock(content, i);
        }
        case LineType::Ignore:
            continue;

        default:
            continue;
        }
    }

    return elementList;
}

bool SHAModel::parse()
{

    QVariantList list = parseContent(m_content);
    return true;
}
