/*
 * @Author: zengqingli 1901227828@qq.com
 * @Date: 2024-09-28 21:00:22
 * @LastEditors: zengqingli 1901227828@qq.com
 * @LastEditTime: 2024-10-15 14:17:45
 * @FilePath: /gncdbflr/linux_dev/src/fragmentation/pidModification.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "hashmap.h"
#include "dbInfo.h"
#include "gncdbconstant.h"
#include "adaptiveSort.h"

int bfsTraverseMater(FILE *fp, int *free_list, int freePageNum);

int readFileHeader(FILE *fp, BYTE *page, int *pageNum, int *firstFreePid);

int modifyFileHeader(FILE *fp, FILE *new_fp, BYTE *page, int newPageNum);

int getFreePageCnt(FILE *fp, BYTE *page, int freePid, int *freePageCnt, int pageNum) ;

int getFreeList(FILE *fp, BYTE *page, int *freeList, int freePid, int pageNum);

int modifyAllPid(FILE *fp, FILE *new_fp, BYTE *page, int pageNum, int *freeList, int freePageNum);