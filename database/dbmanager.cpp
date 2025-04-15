#include "dbmanager.h"
#include <QDebug>

DBManager::DBManager() : db(nullptr) {}

DBManager::~DBManager() {
    close();
}

bool DBManager::open(const QString& dbPath) {
    if(db) {
        qDebug() << "数据库已经打开";
        return false;
    }

    db = new GNCDB();
    QByteArray pathBytes = dbPath.toLocal8Bit();
    int rc = GNCDB_open(&db, pathBytes.data());
    
    if(rc != 0) {
        delete db;
        db = nullptr;
        return false;
    }
    return true;
}

void DBManager::close() {
    if(db) {
        GNCDB_close(&db);
        db = nullptr;
    }
}

bool DBManager::isOpen() const {
    return db != nullptr;
}

int DBManager::executeQuery(const QString& sql, void* data, int (*callback)(void*,int,char**,char**)) {
    if(!db) return -1;
    
    char* errmsg = nullptr;
    int rc = GNCDB_exec(db, sql.toUtf8().constData(), callback, data, &errmsg);
    if(errmsg) {
        qDebug() << "SQL错误:" << errmsg;
        free(errmsg);
    }
    return rc;
}
