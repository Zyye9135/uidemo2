/*
 * @Author: zengqingli 1901227828@qq.com
 * @Date: 2024-10-08 15:14:58
 * @LastEditors: zql 1901227828@qq.com
 * @LastEditTime: 2024-11-02 17:33:00
 * @FilePath: /gncdbflr/linux_dev/src/fragmentation/vacuum.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */

#ifndef _VACUUM_H_
#define _VACUUM_H_

#include "hashmap.h"
#include "adaptiveSort.h"
#include "gncdb.h"
#include "fileModification.h"
#include "vararraylist.h"
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

int vacuum(char *file_path, char *new_file_path, long *contract_size);

#ifdef __cplusplus
}
#endif

#endif /* _VACUUM_H_ */