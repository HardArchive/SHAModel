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
    typedef QList<QStringList> ElementList;

private:
    QStringList m_content;
    QString m_filePath;

private:
    QString m_package;
    QString m_version;

public:
    explicit SHAModel(const QString &filePath, QObject *parent = 0);
    bool loadSha();

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

    bool loadFile();
    bool parse();
};
