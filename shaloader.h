#pragma once

#include <QObject>

class SHALoader : public QObject {
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

public:
    explicit SHALoader(const QString &filePath, QObject *parent = 0);

public:
    bool loadSha();

private:
    QStringList m_fileContent;
    QString m_filePath;

private:
    QString m_package;
    QString m_version;

private:
    QString getBlockTok(const QString &line, const QString &beginTok, const QString &endTok);
    bool loadFile();
    bool parse();
    LineType getTypeLine(const QString &line);
};
