#ifndef RC2MSG_H
#define RC2MSG_H

#include <QString>
#include "gncdbconstant.h"

class Rc2Msg {
public:
    static QString getErrorMsg(int rc);
    static QString getErrorMsg(int rc, const QString& customMsg);
};

#endif // RC2MSG_H 