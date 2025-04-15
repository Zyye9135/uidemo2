#ifndef SQLRESULT_H
#define SQLRESULT_H

#include <QString>
#include <QStringList>
#include <QVector>

struct SQLResult {
    QStringList columnNames;
    QVector<QStringList> rows;
};

#endif // SQLRESULT_H