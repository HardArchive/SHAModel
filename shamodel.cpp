#include "shamodel.h"
#include <QDebug>
#include <QFile>
#include <QRegExp>
#include <QStringList>

SHAModel::SHAModel(const QString &filePath, QObject *parent)
    : QObject(parent), m_filePath(filePath) {}

bool SHAModel::loadSha() {
  if (!loadFile()) return false;

  if (!parse()) return false;

  return true;
}

void SHAModel::getLinkParams(const QString &line) {
  QStringList res;
  const QString link = findBlock(line, "link(", ")");
  const QStringList blocks = link.split(':');
  const QString block1 = blocks.at(0);
  QString block2 = blocks.at(1);

  const QStringList block1List = block1.split(',');
  // const QStringList block2List = block2.split(',');

  qInfo() << findMultiBlock(block2, "(", ")");
  qInfo() << block2;

  // const QString thisPoint = block1List.at(0);
  // const qint32 targetId = block1List.at(1).toInt();
  // const QString targetPoint = block2List.at(0);
  // const QString nodes = block2List.at(1);

  if (blocks.size() < 4) {
    return;
  }
}

QString SHAModel::findBlock(const QString &line, const QString &beginTok,
                            const QString &endTok) {
  const qint32 beginTokLen = beginTok.length();
  const auto begin = line.indexOf(beginTok);
  const auto end = line.lastIndexOf(endTok);
  const qint32 count = end - begin;
  return line.mid(begin + beginTokLen, count - beginTokLen);
}
QStringList SHAModel::findMultiBlock(QString &str, const QString &beginTok,
                                     const QString &endTok, bool cutBlock,
                                     bool removeTok) {
  qint32 index = 0;
  QStringList list;
  while (true) {
    int beginTokLen = beginTok.length();
    int endTokLen = endTok.length();
    auto begin = str.indexOf(beginTok, index);
    if (begin == -1) break;

    auto end = str.indexOf(endTok, begin);
    if (end == -1) break;

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

bool SHAModel::loadFile() {
  QFile file(m_filePath);
  if (!file.open(QIODevice::ReadOnly)) return false;

  m_fileContent = QString::fromLocal8Bit(file.readAll()).split("\r\n");
  return true;
}

bool SHAModel::parse() {
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
        data.insert("package", findBlock(line, "Make(", ")"));
        continue;

      case LineType::Ver:
        data.insert("version", findBlock(line, "ver(", ")"));
        continue;

      case LineType::Add: {
        if (openBlock == true) {
          qWarning() << "Отсутствует фигурная скобка \"}\" закрывающая блок!";
          return false;
        }
        QStringList params = findBlock(line, "Add(", ")").split(',');
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
        // http://www.jsoneditoronline.org/?id=6c2b44125a2456164e333cf59f300fac
        switch (type) {
          case LineType::Link: {
            QVariantMap map;

            getLinkParams(line);
            // element.insertMulti("link", );

            continue;
            break;
          }
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

SHAModel::LineType SHAModel::getTypeLine(const QString &line) {
  const QString &&trimmedLine = line.trimmed();
  auto checkPattern = [&trimmedLine](const QString &pattern) {
    return QRegExp(pattern, Qt::CaseSensitive, QRegExp::WildcardUnix)
        .exactMatch(trimmedLine);
  };

  // Ignore
  if (checkPattern("AddHint(*")) return LineType::Ignore;
  if (checkPattern("\\**")) return LineType::Ignore;
  if (checkPattern("@*=*")) return LineType::Ignore;
  if (checkPattern("Pos(*)")) return LineType::Ignore;
  if (checkPattern("elink(*)")) return LineType::Ignore;
  if (checkPattern("MakeExt(*)")) return LineType::Ignore;
  if (checkPattern("PColor(*)")) return LineType::Ignore;
  if (checkPattern("MakeTrans(*)")) return LineType::Ignore;

  //Действующие
  if (checkPattern("Make(*)")) return LineType::Make;
  if (checkPattern("ver(*)")) return LineType::Ver;
  if (checkPattern("Add(*)")) return LineType::Add;
  if (checkPattern("{")) return LineType::OpenBlock;
  if (checkPattern("}")) return LineType::CloseBlock;
  if (checkPattern("link(*)")) return LineType::Link;
  if (checkPattern("Point(*)")) return LineType::Point;
  if (checkPattern("*=*")) return LineType::Prop;
  if (checkPattern("BEGIN_SDK")) return LineType::BEGIN_SDK;
  if (checkPattern("END_SDK")) return LineType::END_SDK;
  if (trimmedLine.isEmpty()) return LineType::Empty;

  return LineType::Undefined;
}
