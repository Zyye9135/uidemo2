/*
 * @Author: zengqingli 1901227828@qq.com
 * @Date: 2024-09-25 21:08:37
 * @LastEditors: zql 1901227828@qq.com
 * @LastEditTime: 2024-11-02 16:17:42
 * @FilePath: /gncdbflr/linux_dev/src/fragmentation/dbInfo.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */

#pragma once

//#include "mapping.h"
#include "os.h"
#include "typedefine.h"
#include "vararraylist.h"
#include <math.h>
#include <string.h>

typedef struct dbInfo {
    long file_len;
    int page_size;
    int page_count;
    int free_page_count;
    double payload;
    double fragmentation;
} DBInfo;

DBInfo *createDBInfo();

void destroyDBInfo(DBInfo *db_info);

int parseDBInfo(char *file_path, DBInfo *db_info);

int formatPrintDBinfo(DBInfo *db_info);

int outputDBInfo(char *file_path);