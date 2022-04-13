#ifndef __USERPROG_EXEC_H
#define __USERPROG_EXEC_H
#include "stdint.h"
int32_t sys_execv(const char* path, const char*  argv[]);
int32_t load_lba(uint32_t lba, uint32_t sec_cnt);
#endif
