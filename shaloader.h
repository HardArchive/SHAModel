#pragma once

#include <QObject>

class SHALoader : public QObject {
    Q_OBJECT

public:
    enum LineType {
        Undefined,
        Comment,
        Pos,
        Make,
        Ver,
        Add,
        OpenBlock,
        CloseBlock,
        Link,
        Point,
        Hint,
        AddHint,
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
    QString m_package;

private:
    bool loadFile();
    bool parse();
    LineType getLineType(const QString &str);
};
