#pragma once

#include <QObject>

class Project : public QObject {
    Q_OBJECT

public:
    enum LineType {
        Undefined,
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
        END_SDK
    };
    Q_ENUM(LineType)

public:
    explicit Project(const QString &filePath, QObject *parent = 0);

private:
    QStringList m_fileContent;
    QString m_filePath;
    QString m_package;

private:
    bool load();
    void parse();
    LineType getLineType(const QString &str);
};
