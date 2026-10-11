#ifndef USER_H
#define USER_H

#include "user.h"
#include "trap.h"
#include "buddy.h"
#include "dynamic_alloc.h"
#include "initrd.h"
#include "uart.h"

void exec_user(const char* filename);

#endif