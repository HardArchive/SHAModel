#include "shamodel.h"
#include <QDebug>
#include <QFile>
#include <QRegExp>

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

QString SHAModel::getTokBlock(const QString &line, const QString &beginTok, const QString &endTok)
{
    int beginTokLen = beginTok.length();
    auto begin = line.indexOf(beginTok) + beginTokLen;
    auto end = line.lastIndexOf(endTok) - beginTokLen;
    return line.mid(begin, end);
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
    LineType state = LineType::Null;
    bool openBlock = false;

    QVariantMap data;
    QVariantList elementList;
    QVariantMap element;
    QVariantMap container;

    for (const QString v : m_fileContent) {
        const QString &&line = v.trimmed();
        LineType type = getTypeLine(line);

        switch (type) {
        case LineType::Undefined:
            qInfo() << "String:" << line;
            qInfo() << "File:" << m_filePath;

            return false;
        case LineType::Make:
            data.insert("package", getTokBlock(line, "Make(", ")"));
            continue;

        case LineType::Ver:
            data.insert("version", getTokBlock(line, "ver(", ")"));
            continue;

        case LineType::Add: {
            if (openBlock == true) {
                qWarning() << "Отсутствует фигурная скобка \"}\" закрывающая блок!";
                return false;
            }
            QStringList params = getTokBlock(line, "Add(", ")").split(',');
            if (params.size() < 4) {
                qWarning() << "К-во аргументов меньше 4-х.";
                return false;
            }
            element = QVariantMap();
            element.insert("name", params[0]);
            element.insert("id", params[1].toInt());
            element.insert("x", params[2].toInt());
            element.insert("y", params[3].toInt());

            state = LineType::Add;
            continue;
        }
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
            //http://www.jsoneditoronline.org/?id=6c2b44125a2456164e333cf59f300fac
            switch (type) {
            case LineType::Link:
                element.insert()
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

SHAModel::LineType SHAModel::getTypeLine(const QString &line)
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
