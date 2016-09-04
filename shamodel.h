#pragma once
#include <QObject>
#include <QVector>
#include <QSharedPointer>
#include <QVariantMap>
#include <QVariantList>
#include <QStack>

class SHAModel : public QObject
{
    Q_OBJECT

public:
    enum LineType {
        Null,
        Undefined,
        Ignore,
        Make,
        Ver,
        Add,
        OpenBlock,
        CloseBlock,
        Link,
        Point,
        Prop,
        BEGIN_SDK,
        END_SDK,
        Empty
    };
    Q_ENUM(LineType)

private:
    QStringList m_content;

public:
    explicit SHAModel(const QString &filePath, QObject *parent = 0);
    bool loadSha(const QString &path);
    QJsonDocument toJson();

private:
    static QString findBlock(const QString &line, const QString &beginTok,
                             const QString &endTok);
    static QStringList findMultiBlock(QString &str, const QString &beginTok,
                                      const QString &endTok,
                                      bool cutBlock = false,
                                      bool removeTok = false);
    static QVariantMap linkToVariantMap(const QString &line);
    static LineType getLineType(const QString &line);
    static LineType getLineType(const QStringList &content, int idx);
    QVariantMap parseHeader();
    QVariantList parseElements(int begin = 0, int _size = 0, int *prev = 0);
};
