/// @file reg_alloc.h
/// @brief Register allocation pass

#ifndef REG_ALLOC_H
#define REG_ALLOC_H

#include "program.h"

/* Convert temporary register identifiers to real register identifiers,
 * analyzing the live interval of each temporary register. */
void doRegisterAllocation(t_program *program);

#endif
