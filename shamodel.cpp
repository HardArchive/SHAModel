#include "shamodel.h"
#include <QDebug>
#include <QFile>
#include <QRegExp>
#include <QStringList>
#include <QtCore>

SHAModel::SHAModel(const QString &filePath, QObject *parent)
    : QObject(parent)
    , m_filePath(filePath)
{
}

SHAModel::SHAModel(QObject *parent)
    : QObject(parent)
{
}

bool SHAModel::loadSha()
{
    m_content.clear();
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    const QStringList tmpContent = QString::fromLocal8Bit(file.readAll()).split("\r\n");
    for (const QString &s : tmpContent) {
        const QString tmp = s.trimmed();
        if (!tmp.isEmpty())
            m_content << tmp;
    }

    return true;
}

QJsonDocument SHAModel::toJson()
{
    QVariantMap content = parseHeader();
    QVariantList elements = parseElements();
    content.insert("container", elements);

    return QJsonDocument::fromVariant(content);
}

QByteArray SHAModel::toBson()
{
    return toJson().toBinaryData();
}

bool SHAModel::saveJsonToFile(const QString &filePath, QJsonDocument::JsonFormat format)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    const QJsonDocument doc = toJson();
    if (doc.isEmpty())
        return false;

    file.write(doc.toJson(format));
    return true;
}

bool SHAModel::saveJsonToFile(QJsonDocument::JsonFormat format)
{
    QFileInfo info = QFileInfo(m_filePath);
    const QString jsonFilePath = info.absolutePath() + QDir::separator() + info.baseName() + ".json";
    return saveJsonToFile(jsonFilePath, format);
}

QString SHAModel::getFilePath() const
{
    return m_filePath;
}

void SHAModel::setFilePath(const QString &filePath)
{
    m_filePath = filePath;
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

QPair<QString, QString> SHAModel::splitSLine(const QString &sline, const QChar &del, ParseType type)
{
    QString first;
    QString second;

    bool split = false;
    auto delimiter = [&](int idx) {
        const QChar c = sline.at(idx);
        if ((split == false) && (c == del)) {
            split = true;
            return;
        }

        if (!split) {
            first += c;
        } else {
            second += c;
        }
    };

    int size = sline.size();
    if (type == BeginToEnd) {
        for (int i = 0; i < size; ++i) {
            delimiter(i);
        }
    } else {
        for (int i = size; i > 0; --i) {
            delimiter(i);
        }
    }

    return {first, second};
}

SHAModel::DataType SHAModel::getValueType(const QString &svalue)
{
    if (svalue.isEmpty())
        return data_null;

    const QChar c = svalue.at(0);
    if (c == '"')
        return data_str;

    if(c == '#')
        return data_list;

    if(c == '[')
        return data_list;

}

QVariantMap SHAModel::linkToVariantMap(const QString &sline)
{
    QVariantMap map;
    QStringList res;
    const QString link = findBlock(sline, "link(", ")");
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

QVariantMap SHAModel::propToVariantMap(const QString &sline)
{
    if (sline.isEmpty())
        return QVariantMap();

    auto pairBlock = splitSLine(sline, '=');
    QString first = pairBlock.first;
    QString second = pairBlock.second;

    //Скрытое свойство
    bool isHide = false;
    if (first.at(0) == QLatin1Char('@')) {
        first.remove(0, 1);
        isHide = true;
    }

    return QVariantMap();
}

SHAModel::LineType SHAModel::getLineType(const QString &sline)
{
    auto checkPattern = [&sline](const QString &pattern) {
        return QRegExp(pattern, Qt::CaseSensitive, QRegExp::WildcardUnix)
            .exactMatch(sline);
    };

    // Ignore
    if (checkPattern("AddHint(*"))
        return LineType::Ignore;
    if (checkPattern("\\**"))
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

    //Актуальные
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
    if (checkPattern("@*=*"))
        return LineType::HideProp;
    if (checkPattern("*=*"))
        return LineType::Prop;
    if (checkPattern("BEGIN_SDK"))
        return LineType::BEGIN_SDK;
    if (checkPattern("END_SDK"))
        return LineType::END_SDK;
    if (sline.isEmpty())
        return LineType::Empty;

    return LineType::Undefined;
}

SHAModel::LineType SHAModel::getLineType(const QStringList &content, int idx)
{
    int size = content.size();
    if ((idx < 0) || (idx > (size - 1)))
        return LineType::Null;

    return getLineType(content[idx]);
}

QVariantMap SHAModel::parseHeader()
{
    QVariantMap header;
    for (const QString &line : m_content) {
        switch (getLineType(line)) {
        case LineType::Make:
            header.insert("package", findBlock(line, "Make(", ")"));
            continue;
        case LineType::Ver:
            header.insert("version", findBlock(line, "ver(", ")"));
            continue;
        case LineType::Add:
            return header;
        case LineType::Ignore:
        case SHAModel::Null:
        case SHAModel::Undefined:
        case SHAModel::OpenBlock:
        case SHAModel::CloseBlock:
        case SHAModel::Link:
        case SHAModel::Point:
        case SHAModel::HideProp:
        case SHAModel::Prop:
        case SHAModel::BEGIN_SDK:
        case SHAModel::END_SDK:
        case SHAModel::Empty:
            continue;
        }
    }

    return header;
}

QVariantList SHAModel::parseElements(int begin, int _size, int *prev)
{
    QVariantList elementList;
    QVariantMap element;
    QVariantList linkList;
    QVariantList pointList;
    QVariantList propList;

    const int size = (_size <= 0) ? m_content.size() : _size;
    for (int i = begin; i < size; ++i) {

        const QString sline = m_content[i];
        const LineType type = getLineType(sline);

        switch (type) {
        case LineType::Add: {
            QStringList params = findBlock(sline, "Add(", ")").split(',');
            if (params.size() < 4) {
                qWarning() << "К-во аргументов меньше 4-х.";
                return QVariantList();
            }

            //Основные параметры элемента
            element.insert("name", params[0]);
            element.insert("id", params[1].toInt());
            element.insert("x", params[2].toInt());
            element.insert("y", params[3].toInt());
            break;
        }
        case LineType::Link: {
            const QVariantMap link = linkToVariantMap(sline);
            if (link.isEmpty()) {
                qWarning() << "Ошибка при разборе параметров link(*)";
                return QVariantList();
            }
            linkList.append(link);
            continue;
        }
        case LineType::Point: {
            const QString point = findBlock(sline, "(", ")");
            if (!point.isEmpty())
                pointList << point;
            else
                qWarning() << "Ошибка при разборе параметров Point(*)";

            continue;
        }
        case LineType::HideProp:
        case LineType::Prop: {
            const QVariantMap prop = propToVariantMap(sline);
            if (!prop.isEmpty())
                propList << prop;
            else
                qWarning() << "Ошибка при разборе свойства";

            continue;
        }
        case LineType::CloseBlock: {
            //Элемент является контейнером
            if (getLineType(m_content, i + 1) == LineType::BEGIN_SDK) {
                const QVariantList list = parseElements(i + 2, size, &i);
                element.insert("container", list);
            }

            if (!linkList.isEmpty())
                element.insert("links", linkList);
            if (!pointList.isEmpty())
                element.insert("points", pointList);
            if (!propList.isEmpty())
                element.insert("props", propList);

            linkList.clear();
            pointList.clear();
            propList.clear();
            elementList += element;
            element.clear();

            continue;
        }
        case LineType::END_SDK: {
            *prev = i + 1;
            return elementList;
        }
        case SHAModel::Null:
        case SHAModel::Undefined:
        case SHAModel::Ignore:
        case SHAModel::Make:
        case SHAModel::Ver:
        case SHAModel::OpenBlock:
        case SHAModel::BEGIN_SDK:
        case SHAModel::Empty:
            continue;
        }
    }

    return elementList;
}
