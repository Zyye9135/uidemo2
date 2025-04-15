#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QString>
#include <QStringList>
#include <QVector>
#include "../gncdblib/include/gncdb.h"
#include "sqlresult.h"

class DBManager {
public:
    DBManager();
    ~DBManager();

    bool open(const QString& dbPath);
    void close();
    bool isOpen() const;
    int executeQuery(const QString& sql, void* data = nullptr, int (*callback)(void*,int,char**,char**) = nullptr);
    GNCDB* getDB() const { return db; }

private:
    GNCDB* db;
};

#endif // DBMANAGER_H
