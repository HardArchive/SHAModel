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

SHAModel::LineType SHAModel::getTypeLine(int idx)
{
    int size = m_content.size();
    if ((idx < 0) || (idx > (size - 1)))
        return LineType::Null;

    return getTypeLine(m_content[idx]);
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

SHAModel::ElementList SHAModel::splitContent(const QStringList &content)
{
    ElementList list;
    int size = content.size();
    for (int i = 0; i < size; ++i) {
        const LineType type = getTypeLine(content[i]);

        switch (type) {
        case LineType::Undefined:
            return ElementList();
        case LineType::Add: {
            int begin = i;

            //Остальные параметры элемента
            for (int idx = i + 1; idx < size; ++idx) {
                switch (getTypeLine(idx)) {
                case CloseBlock: {
                    if (getTypeLine(idx + 1) == LineType::BEGIN_SDK) {
                        int sdk = 0;
                        for (int idxC = idx + 1; idxC < size; ++idxC) {
                            LineType type = getTypeLine(idxC);
                            if (type == LineType::BEGIN_SDK) {
                                ++sdk;
                            } else if (type == LineType::END_SDK) {
                                --sdk;
                                if (sdk == 0) {
                                    list << content.mid(begin, idxC - begin + 1);
                                    i = idxC;
                                    break;
                                }
                            }
                        }
                    } else {
                        list << content.mid(begin, idx - begin + 1);
                        i = idx;
                        break;
                    }
                }
                default:
                    continue;
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

    return list;
}

bool SHAModel::parse()
{

    splitContent(m_content);
    /*
    QVariantMap data;

    QStack<QVariantList> stackContainer;
    QVariantList container;

    QVariantMap element;
    QVariantList links;

    const auto &content = m_content;
    int size = content.size();
    for (int i = 0; i < size; ++i) {
        const LineType type = getTypeLine(content[i]);

        switch (type) {
        case LineType::Undefined:
            return false;
        case LineType::Make:
            data.insert("package", findBlock(content[i], "Make(", ")"));
            continue;
        case LineType::Ver:
            data.insert("version", findBlock(content[i], "ver(", ")"));
            continue;

        case LineType::Add: {
            QStringList params = findBlock(content[i], "Add(", ")").split(',');
            if (params.size() < 4) {
                qWarning() << "К-во аргументов меньше 4-х.";
                return false;
            }

            //Основные параметры элемента
            element.insert("name", params[0]);
            element.insert("id", params[1].toInt());
            element.insert("x", params[2].toInt());
            element.insert("y", params[3].toInt());

            //Остальные параметры элемента
            for (int idx = i + 1; idx < size; ++idx) {
                switch (getTypeLine(idx)) {
                case OpenBlock:
                    continue;
                case Link: {
                    const QVariantMap link = linkToVariantMap(content[idx]);
                    if (link.isEmpty()) {
                        qWarning() << "Ошибка разбора параметров link(*)";
                        return false;
                    }
                    links.append(link);
                    continue;
                }
                case Point:
                    continue;
                case Prop:
                    continue;
                case CloseBlock: {
                    if (!links.isEmpty()) {
                        element.insert("links", links);
                        links.clear();
                    }
                    bool isContainer = (getTypeLine(idx + 1) == LineType::BEGIN_SDK);
                    if (isContainer) {
                    } else {
                    }

                    container.append(element);
                    element.clear();

                    i = idx;
                    break;
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

    return true;
    */

    /*
  LineType state = LineType::Null;
  bool openBlock = false;

  QVariantMap data;
  QVariantList elementList;

  QVariantMap element;
  QVariantMap container;
  QVariantList links;

  for (const QString v : m_fileContent) {
    const QString line = v.trimmed();
    LineType type = getTypeLine(line);

    switch (type) {
      case LineType::Undefined:
        qInfo() << "String:" << line;
        qInfo() << "File:" << m_filePath;

        return false;
      case LineType::Make:
        data.insert("package", findBlock(line, "Make(", ")"));
        continue;

      case LineType::Ver:
        data.insert("version", findBlock(line, "ver(", ")"));
        continue;

      case LineType::Add: {
        if (openBlock == true) {
          qWarning() << "Отсутствует фигурная скобка \"}\" закрывающая
  блок!";
          return false;
        }
        QStringList params = findBlock(line, "Add(", ")").split(',');
        if (params.size() < 4) {
          qWarning() << "К-во аргументов меньше 4-х.";
          return false;
        }
        //Инициализация
        element.insert("name", params[0]);
        element.insert("id", params[1].toInt());
        element.insert("x", params[2].toInt());
        element.insert("y", params[3].toInt());

        state = LineType::Add;
        continue;
      }
      case LineType::Ignore:
        continue;
      default:
        break;
    }

    switch (state) {
      case LineType::Add: {
        if (openBlock == false) {
          if (type == LineType::OpenBlock) {
            openBlock = true;
            continue;
          } else {
            qWarning() << "Отсутствует фигурная скобка \"{\" открывающая
  блок!";
            return false;
          }
        }
        //Начало OpenBlock
        //
  http://www.jsoneditoronline.org/?id=6c2b44125a2456164e333cf59f300fac
        //
        switch (type) {
          case LineType::Link: {
            QString thisPoint;
            QString targetPoint;
            qint32 targetId;
            QStringList nodes;

            if (!getLinkParams(line, thisPoint, targetPoint, targetId,
  nodes)) {
              qWarning() << "Ошибка разбора параметров link(*)";
              return false;
            }
            QVariantMap map;
            map.insert("thisPoint", thisPoint);
            map.insert("targetPoint", targetPoint);
            map.insert("targetId", targetId);
            if (!nodes.isEmpty()) map.insert("nodes", nodes);
            links.append(map);

            continue;
          }
          case LineType::CloseBlock:
            openBlock = false;
            state = LineType::Null;

            if (!links.isEmpty()) {
              element.insert("links", links);
              links.clear();
            }
            elementList.append(element);
            element.clear();

            continue;
        }
      }
      case LineType::BEGIN_SDK: {
      }
      case LineType::END_SDK: {
      }
      default:
        break;
    }
  }
*/
}
