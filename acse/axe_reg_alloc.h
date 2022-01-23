/*
 * Andrea Di Biagio
 * Politecnico di Milano, 2007
 * 
 * axe_reg_alloc.h
 * Formal Languages & Compilers Machine, 2007/2008
 * 
 * Register allocation pass
 */

#ifndef _AXE_REG_ALLOC_H
#define _AXE_REG_ALLOC_H

#include "axe_engine.h"
#include "collections.h"
#include "axe_cflow_graph.h"

/* constants */
#define RA_SPILL_REQUIRED -1
#define RA_REGISTER_INVALID 0
#define RA_EXCLUDED_VARIABLE 0

/* errorcodes */
#define RA_OK 0
#define RA_INVALID_ALLOCATOR 1
#define RA_INVALID_INTERVAL 2
#define RA_INTERVAL_ALREADY_INSERTED 3
#define RA_INVALID_NUMBER_OF_REGISTERS 4

typedef struct t_live_interval
{
   int varID;     /* a variable identifier */
   t_list *mcRegConstraints; /* list of all registers where this variable can
                              * be allocated. */
   int startPoint;  /* the index of the first instruction
                     * that make use of (or define) this variable */
   int endPoint;   /* the index of the last instruction
                    * that make use of (or define) this variable */
}t_live_interval;

typedef struct t_reg_allocator
{
   t_list *live_intervals;    /* an ordered list of live intervals */
   int regNum;                /* the number of registers of the machine */
   int varNum;                /* number of variables */
   int *bindings;             /* an array of bindings of kind : varID-->register.
                               * If a certain variable X need to be spilled
                               * in memory, the value of `register' is set
                               * to the value of the macro RA_SPILL_REQUIRED */
   t_list *freeRegisters;     /* a list of free registers */
}t_reg_allocator;


/* Initialize the internal structure of the register allocator */
extern t_reg_allocator * initializeRegAlloc(t_cflow_Graph *graph);

/* finalize all the data structure associated with the given register allocator */
extern void finalizeRegAlloc(t_reg_allocator *RA);

/* execute the register allocation algorithm (Linear Scan) */
extern int executeLinearScan(t_reg_allocator *RA);

/* Replace the variable identifiers in the instructions of the CFG with the
 * register assignments in the register allocator. Materialize spilled
 * variables to the scratch registers. All new instructions are inserted
 * in the CFG. Synchronize the list of instructions with the newly
 * modified program. */
void materializeRegisterAllocation(
      t_program_infos *program, t_cflow_Graph *graph, t_reg_allocator *RA);

#endif
