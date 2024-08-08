/// @file reg_alloc.c

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include "reg_alloc.h"
#include "target_info.h"
#include "errors.h"
#include "codegen.h"
#include "program.h"
#include "list.h"
#include "cfg.h"
#include "target_asm_print.h"

/// Maximum amount of arguments to an instruction
#define MAX_INSTR_ARGS (CFG_MAX_DEFS + CFG_MAX_USES)

/// Fictitious register ID associated to registers to be spilled.
#define RA_SPILL_REQUIRED    ((t_regID)(-2))
/// Fictitious register ID marking currently unallocated temporaries.
#define RA_REGISTER_INVALID  ((t_regID)(-1))


/// Structure describing a live interval of a register in a program.
typedef struct {
  /// Identifier of the register
  t_regID tempRegID;
  /// List of all physical registers where this temporary register can be
  /// allocated.
  t_listNode *mcRegConstraints;
  /// Index of the first instruction that uses/defines this register
  int startPoint;
  /// Index of the last instruction that uses/defines this register
  int endPoint;
} t_liveInterval;

/// Structure encapsulating the state of the register allocator
struct t_regAllocator {
  /// The program where register allocation needs to be performed
  t_program *program;
  /// The temporary control flow graph produced from the program
  t_cfg *graph;
  /// List of live intervals, ordered depending on their start index
  t_listNode *liveIntervals;
  /// Number of temporary registers in the program
  int tempRegNum;
  /// Pointer to a dynamically allocated array which maps every temporary
  /// register to the corresponding physical register.
  /// Temporary registers allocated to a spill location are marked by the
  /// RA_SPILL_REQUIRED virtual register ID.
  t_regID *bindings;
  /// List of currently active intervals during the allocation process.
  t_listNode *activeIntervals;
  /// List of currently free physical registers during the allocation process.
  t_listNode *freeRegisters;
};

/// Structure used for mapping a spilled temporary register to the label
/// pointing to its physical storage location in memory.
typedef struct {
  /// The spilled temporary register ID
  t_regID tempRegID;
  /// The label pointing to the spill area
  t_label *label;
} t_spillLocation;

/// Structure representing the current state of an instruction argument during
/// the spill load/store materialization process
typedef struct {
  /// The instruction argument structure
  t_instrArg *reg;
  /// If the register is a destination register
  bool isDestination;
  /// The physical spill register index where the argument will be materialized,
  /// or -1 otherwise.
  int spillSlot;
} t_spillInstrArgState;

/// Structure representing the current state of a spill-reserved register
typedef struct {
  /// Temporary register ID associated to this spill register
  t_regID assignedTempReg;
  /// Non-zero if at least one of the instructions wrote something new into
  /// the spill register, and the value has not been written to the spill
  /// memory location yet.
  bool needsWB;
} t_spillRegState;

/// Spill register slots state
typedef struct {
  /// each array element corresponds to one of the registers reserved for
  /// spilled variables, ordered by ascending register number.
  t_spillRegState regs[NUM_SPILL_REGS];
} t_spillState;


/* Allocate and initialize a live interval data structure with a given
 * temporary register ID, starting and ending points */
t_liveInterval *newLiveInterval(
    t_regID tempRegID, t_listNode *mcRegs, int startPoint, int endPoint)
{
  t_liveInterval *result = malloc(sizeof(t_liveInterval));
  if (result == NULL)
    fatalError("out of memory");

  result->tempRegID = tempRegID;
  result->mcRegConstraints = listClone(mcRegs);
  result->startPoint = startPoint;
  result->endPoint = endPoint;
  return result;
}

/* Deallocate a live interval */
void deleteLiveInterval(t_liveInterval *interval)
{
  if (interval == NULL)
    return;
  
  free(interval->mcRegConstraints);
  free(interval);
}

/* Given two live intervals, compare them by the start point (find whichever
 * starts first) */
int compareLiveIntStartPoints(void *varA, void *varB)
{
  t_liveInterval *liA = (t_liveInterval *)varA;
  t_liveInterval *liB = (t_liveInterval *)varB;

  if (varA == NULL)
    return 0;
  if (varB == NULL)
    return 0;

  return liA->startPoint - liB->startPoint;
}

/* Given two live intervals, compare them by the end point (find whichever
 * ends first) */
int compareLiveIntEndPoints(void *varA, void *varB)
{
  t_liveInterval *liA = (t_liveInterval *)varA;
  t_liveInterval *liB = (t_liveInterval *)varB;

  if (varA == NULL)
    return 0;
  if (varB == NULL)
    return 0;

  return liA->endPoint - liB->endPoint;
}

/* Given two live intervals, check if they refer to the same interval */
bool compareLiveIntWithRegID(void *a, void *b)
{
  t_liveInterval *interval = (t_liveInterval *)a;
  t_regID tempRegID = *((t_regID *)b);
  return interval->tempRegID == tempRegID;
}

/* Update the liveness interval list to account for the fact that variable 'var'
 * is live at index 'counter' in the current program.
 * If the variable already appears in the list, its live interval its prolonged
 * to include the given counter location.
 * Otherwise, a new liveness interval is generated for it.*/
t_listNode *updateIntervalsWithLiveVarAtLocation(t_listNode *intervals, t_cfgReg *var, int counter)
{
  // Search if there's already a liveness interval for the variable
  t_listNode *element_found = listFindWithCallback(intervals, &(var->tempRegID), compareLiveIntWithRegID);

  if (!element_found) {
    // It's not there: add a new interval at the end of the list
    t_liveInterval *interval =
        newLiveInterval(var->tempRegID, var->mcRegWhitelist, counter, counter);
    intervals = listInsert(intervals, interval, -1);
  } else {
    // It's there: update the interval range
    t_liveInterval *interval_found = (t_liveInterval *)element_found->data;
    // Counter should always be increasing!
    assert(interval_found->startPoint <= counter);
    assert(interval_found->endPoint <= counter);
    interval_found->endPoint = counter;
  }
  
  return intervals;
}

/* Add/augment the live interval list with the variables live at a given
 * instruction location in the program */
t_listNode *updateIntervalsWithInstrAtLocation(
    t_listNode *result, t_cfgNode *node, int counter)
{
  t_listNode *elem;

  elem = node->in;
  while (elem != NULL) {
    t_cfgReg *curCFGReg = (t_cfgReg *)elem->data;
    result = updateIntervalsWithLiveVarAtLocation(result, curCFGReg, counter);
    elem = elem->next;
  }

  elem = node->out;
  while (elem != NULL) {
    t_cfgReg *curCFGReg = (t_cfgReg *)elem->data;
    result = updateIntervalsWithLiveVarAtLocation(result, curCFGReg, counter);
    elem = elem->next;
  }

  for (int i = 0; i < CFG_MAX_DEFS; i++) {
    if (node->defs[i])
      result = updateIntervalsWithLiveVarAtLocation(result, node->defs[i], counter);
  }

  return result;
}

int getLiveIntervalsNodeCallback(
    t_basicBlock *block, t_cfgNode *node, int nodeIndex, void *context)
{
  t_listNode **list = (t_listNode **)context;
  *list = updateIntervalsWithInstrAtLocation(*list, node, nodeIndex);
  return 0;
}

/* Collect a list of live intervals from the in/out sets in the CFG.
 * Since cfgIterateNodes passes incrementing counter values to the
 * callback, the list returned from here is already ordered. */
t_listNode *getLiveIntervals(t_cfg *graph)
{
  t_listNode *result = NULL;
  cfgIterateNodes(graph, (void *)&result, getLiveIntervalsNodeCallback);
  return result;
}


/* Move the elements in list `a` which are also contained in list `b` to the
 * front of the list. */
t_listNode *optimizeRegisterSet(t_listNode *a, t_listNode *b)
{
  for (; b; b = b->next) {
    t_listNode *old;
    if ((old = listFind(a, b->data))) {
      a = listRemoveNode(a, old);
      a = listInsert(a, b->data, 0);
    }
  }
  return a;
}

t_listNode *subtractRegisterSets(t_listNode *a, t_listNode *b)
{
  for (; b; b = b->next) {
    a = listFindAndRemove(a, b->data);
  }
  return a;
}

/* Create register constraint sets for all temporaries that don't have one.
 * This is the main function that makes register allocation with constraints
 * work.
 *   The idea is that we rely on the fact that all temporaries without
 * constraints are distinguishable from temporaries with constraints.
 * When a temporary *without* constraints A is alive at the same time as a
 * temporary *with* constraints B, we prohibit allocation of A to all the
 * viable registers for B. This guarantees A won't steal a register needed by B.
 *   Of course this will stop working as nicely with multiple overlapping
 * constraints, but in ACSE this doesn't happen.
 *   The effect of this function is */
void initializeRegisterConstraints(t_regAllocator *ra)
{
  t_listNode *i = ra->liveIntervals;
  for (; i; i = i->next) {
    t_liveInterval *interval = i->data;
    // Skip instructions that already have constraints
    if (interval->mcRegConstraints)
      continue;
    // Initial set consists of all registers.
    interval->mcRegConstraints = getListOfGenPurposeMachineRegisters();

    // Scan the temporary registers that are alive together with this one and
    // already have constraints.
    t_listNode *j = i->next;
    for (; j; j = j->next) {
      t_liveInterval *overlappingIval = j->data;
      if (overlappingIval->startPoint > interval->endPoint)
        break;
      if (!overlappingIval->mcRegConstraints)
        continue;
      if (overlappingIval->startPoint == interval->endPoint) {
        // Some instruction is using our temporary register as a source and the
        // other temporary register as a destination. Optimize the constraint
        // order to allow allocating source and destination to the same register
        // if possible.
        interval->mcRegConstraints = optimizeRegisterSet(
            interval->mcRegConstraints, overlappingIval->mcRegConstraints);
      } else {
        // Another variable (defined after this one) wants to be allocated
        // to a restricted set of registers. Punch a hole in the current
        // variable's set of allowed registers to ensure that this is
        // possible.
        interval->mcRegConstraints = subtractRegisterSets(
            interval->mcRegConstraints, overlappingIval->mcRegConstraints);
      }
    }
  }
}

int handleCallerSaveRegistersNodeCallback(
    t_basicBlock *block, t_cfgNode *node, int nodeIndex, void *context)
{
  t_regAllocator *ra = (t_regAllocator *)context;

  if (!isCallInstruction(node->instr))
    return 0;

  t_listNode *clobberedRegs = getListOfCallerSaveMachineRegisters();
  for (int i = 0; i < CFG_MAX_DEFS; i++) {
    if (node->defs[i] != NULL)
      clobberedRegs =
          subtractRegisterSets(clobberedRegs, node->defs[i]->mcRegWhitelist);
  }
  for (int i = 0; i < CFG_MAX_USES; i++) {
    if (node->uses[i] != NULL)
      clobberedRegs =
          subtractRegisterSets(clobberedRegs, node->uses[i]->mcRegWhitelist);
  }

  t_listNode *li_ival = ra->liveIntervals;
  while (li_ival) {
    t_liveInterval *ival = li_ival->data;

    if (ival->startPoint <= nodeIndex && nodeIndex <= ival->endPoint) {
      ival->mcRegConstraints =
          subtractRegisterSets(ival->mcRegConstraints, clobberedRegs);
    }

    li_ival = li_ival->next;
  }

  return 0;
}

/* Restrict register constraints in order to avoid register corrupted by
 * function calls. */
void handleCallerSaveRegisters(t_regAllocator *ra, t_cfg *cfg)
{
  cfgIterateNodes(cfg, (void *)ra, handleCallerSaveRegistersNodeCallback);
}

/* Allocate and initialize the register allocator */
t_regAllocator *newRegAllocator(t_program *program)
{
  t_regAllocator *result = (t_regAllocator *)calloc(1, sizeof(t_regAllocator));
  if (result == NULL)
    fatalError("out of memory");

  // Create a CFG from the given program and compute the liveness intervals
  result->program = program;
  result->graph = programToCFG(program);
  cfgComputeLiveness(result->graph);

  // Find the maximum temporary register ID in the program, then allocate the
  // array of register bindings with that size. If there are unused register
  // IDs, the array will have holes, but that's not a problem.
  t_regID maxTempRegID = 0;
  t_listNode *curCFGRegNode = result->graph->registers;
  while (curCFGRegNode != NULL) {
    t_cfgReg *curCFGReg = (t_cfgReg *)curCFGRegNode->data;
    if (maxTempRegID < curCFGReg->tempRegID)
      maxTempRegID = curCFGReg->tempRegID;
    curCFGRegNode = curCFGRegNode->next;
  }
  result->tempRegNum = maxTempRegID + 1;

  // allocate space for the binding array, and initialize it
  result->bindings = malloc(sizeof(t_regID) * (size_t)result->tempRegNum);
  if (result->bindings == NULL)
    fatalError("out of memory");
  for (int counter = 0; counter < result->tempRegNum; counter++)
    result->bindings[counter] = RA_REGISTER_INVALID;
  
  // If the target has a special meaning for register zero, allocate it to
  // itself immediately
  if (TARGET_REG_ZERO_IS_CONST)
    result->bindings[REG_0] = REG_0;

  // Compute the ordered list of live intervals
  result->liveIntervals = getLiveIntervals(result->graph);

  // Create the list of free physical (machine) registers
  result->freeRegisters = getListOfMachineRegisters();

  /* Initialize register constraints */
  initializeRegisterConstraints(result);
  handleCallerSaveRegisters(result, result->graph);

  /* return the new register allocator */
  return result;
}

bool compareFreeRegListNodes(void *freeReg, void *constraintReg)
{
  return INT_TO_LIST_DATA(constraintReg) == INT_TO_LIST_DATA(freeReg);
}

/* Remove from RA->activeIntervals all the live intervals that end before the
 * beginning of the current live interval */
void expireOldIntervals(t_regAllocator *RA, t_liveInterval *interval)
{
  /* No active intervals, bail out! */
  if (RA->activeIntervals == NULL)
    return;

  /* Iterate over the set of active intervals */
  t_listNode *curNode = RA->activeIntervals;
  while (curNode != NULL) {
    /* Get the live interval */
    t_liveInterval *curInterval = (t_liveInterval *)curNode->data;

    /* If the considered interval ends before the beginning of
     * the current live interval, we don't need to keep track of
     * it anymore; otherwise, this is the first interval we must
     * still take into account when assigning registers. */
    if (curInterval->endPoint > interval->startPoint)
      return;

    /* when curInterval->endPoint == interval->startPoint,
     * the variable associated to curInterval is being used by the
     * instruction that defines interval. As a result, we can allocate
     * interval to the same reg as curInterval. */
    if (curInterval->endPoint == interval->startPoint) {
      t_regID curIntReg = RA->bindings[curInterval->tempRegID];
      if (curIntReg >= 0) {
        t_listNode *allocated =
            listInsert(NULL, INT_TO_LIST_DATA(curIntReg), 0);
        interval->mcRegConstraints =
            optimizeRegisterSet(interval->mcRegConstraints, allocated);
        deleteList(allocated);
      }
    }

    /* Get the next live interval */
    t_listNode *nextNode = curNode->next;

    /* Remove the current element from the list */
    RA->activeIntervals =
        listFindAndRemove(RA->activeIntervals, curInterval);

    /* Free all the registers associated with the removed interval */
    RA->freeRegisters = listInsert(RA->freeRegisters,
        INT_TO_LIST_DATA(RA->bindings[curInterval->tempRegID]), 0);

    /* Step to the next interval */
    curNode = nextNode;
  }
}

/* Get a new register from the free list */
t_regID assignRegister(t_regAllocator *RA, t_listNode *constraints)
{
  if (constraints == NULL)
    return RA_SPILL_REQUIRED;

  for (t_listNode *i = constraints; i; i = i->next) {
    t_regID tempRegID = (t_regID)LIST_DATA_TO_INT(i->data);
    t_listNode *freeReg = listFindWithCallback(
        RA->freeRegisters, INT_TO_LIST_DATA(tempRegID), compareFreeRegListNodes);
    if (freeReg) {
      RA->freeRegisters = listRemoveNode(RA->freeRegisters, freeReg);
      return tempRegID;
    }
  }

  return RA_SPILL_REQUIRED;
}

/* Perform a spill that allows the allocation of the given
 * interval, given the list of active live intervals */
void spillAtInterval(t_regAllocator *RA, t_liveInterval *interval)
{
  t_listNode *lastNode;
  t_liveInterval *lastInterval;

  /* get the last element of the list of active intervals */
  /* Precondition: if the list of active intervals is empty
   * we are working on a machine with 0 registers available
   * for the register allocation */
  if (RA->activeIntervals == NULL) {
    RA->bindings[interval->tempRegID] = RA_SPILL_REQUIRED;
    return;
  }

  lastNode = listGetLastNode(RA->activeIntervals);
  lastInterval = (t_liveInterval *)lastNode->data;

  /* If the current interval ends before the last one, spill
   * the last one, otherwise spill the current interval. */
  if (lastInterval->endPoint > interval->endPoint) {
    t_regID attempt = RA->bindings[lastInterval->tempRegID];
    if (listFind(interval->mcRegConstraints, INT_TO_LIST_DATA(attempt))) {
      RA->bindings[interval->tempRegID] = RA->bindings[lastInterval->tempRegID];
      RA->bindings[lastInterval->tempRegID] = RA_SPILL_REQUIRED;

      RA->activeIntervals = listFindAndRemove(RA->activeIntervals, lastInterval);

      RA->activeIntervals =
          listInsertSorted(RA->activeIntervals, interval, compareLiveIntEndPoints);
      return;
    }
  }

  RA->bindings[interval->tempRegID] = RA_SPILL_REQUIRED;
}

/* Main register allocation function */
void executeLinearScan(t_regAllocator *RA)
{
  /* Iterate over the list of live intervals */
  for (t_listNode *curNode = RA->liveIntervals; curNode != NULL;
       curNode = curNode->next) {
    /* Get the live interval */
    t_liveInterval *curInterval = (t_liveInterval *)curNode->data;

    /* Check which intervals are ended and remove
     * them from the active set, thus freeing registers */
    expireOldIntervals(RA, curInterval);

    t_regID reg = assignRegister(RA, curInterval->mcRegConstraints);

    /* If all registers are busy, perform a spill */
    if (reg == RA_SPILL_REQUIRED) {
      /* perform a spill */
      spillAtInterval(RA, curInterval);
    } else {
      /* Otherwise, assign a new register to the current live interval */
      RA->bindings[curInterval->tempRegID] = reg;

      /* Add the current interval to the list of active intervals, in
       * order of ending points (to allow easier expire management) */
      RA->activeIntervals = listInsertSorted(
          RA->activeIntervals, curInterval, compareLiveIntEndPoints);
    }
  }

  /* free the list of active intervals */
  RA->activeIntervals = deleteList(RA->activeIntervals);
}

/* Deallocate the register allocator data structures */
void deleteRegAllocator(t_regAllocator *RA)
{
  if (RA == NULL)
    return;

  /* finalize the memory blocks associated with all
   * the live intervals */
  for (t_listNode *curNode = RA->liveIntervals; curNode != NULL;
       curNode = curNode->next) {
    /* fetch the current interval */
    t_liveInterval *curInterval = (t_liveInterval *)curNode->data;
    if (curInterval != NULL) {
      /* finalize the memory block associated with
       * the current interval */
      deleteLiveInterval(curInterval);
    }
  }

  /* deallocate the list of intervals */
  deleteList(RA->liveIntervals);

  /* Free memory used for the variable/register bindings */
  free(RA->bindings);
  deleteList(RA->activeIntervals);
  deleteList(RA->freeRegisters);

  // Delete the temporary CFG
  deleteCFG(RA->graph);

  free(RA);
}


/*
 * Materialization
 */

bool compareSpillLocWithRegId(void *a, void *b)
{
  t_spillLocation *spillLoc = (t_spillLocation *)a;
  t_regID reg = *((t_regID *)b);

  if (spillLoc == NULL)
    return 0;
  return spillLoc->tempRegID == reg;
}

t_spillLocation *newSpillLocation(t_label *label, t_regID tempRegID)
{
  t_spillLocation *result = malloc(sizeof(t_spillLocation));
  if (result == NULL)
    fatalError("out of memory");

  result->label = label;
  result->tempRegID = tempRegID;
  return result;
}

void deleteSpillLocationList(t_listNode *spills)
{
  if (spills == NULL)
    return;

  t_listNode *curNode = spills;
  while (curNode != NULL) {
    t_spillLocation *spillLoc = (t_spillLocation *)curNode->data;
    free(spillLoc);
    curNode = curNode->next;
  }

  deleteList(spills);
}

/* For each spilled variable, this function statically allocates memory for
 * that variable, returns a list of t_templabel structures mapping the
 * spilled variables and the label that points to the allocated memory block.
 * Returns an error code. */
t_listNode *materializeSpillMemory(t_program *program, t_regAllocator *RA)
{
  /* initialize the local variable `result' */
  t_listNode *result = NULL;

  /* allocate some memory for all spilled temporary variables */
  for (t_regID counter = 0; counter < RA->tempRegNum; counter++) {
    if (RA->bindings[counter] != RA_SPILL_REQUIRED)
      continue;

    // statically allocate some room for the spilled variable and add it to the
    // list of spills
    char name[32];
    sprintf(name, ".t%d", counter);
    t_symbol *sym = createSymbol(program, strdup(name), TYPE_INT, 0);
    t_spillLocation *spillLoc = newSpillLocation(sym->label, counter);
    result = listInsert(result, spillLoc, -1);
  }

  return result;
}

void genStoreSpillVariable(t_regID rSpilled, t_regID rSrc,
    t_cfg *graph, t_basicBlock *curBlock, t_cfgNode *curCFGNode,
    t_listNode *labelBindings, bool before)
{
  t_listNode *elementFound =
      listFindWithCallback(labelBindings, &rSpilled, compareSpillLocWithRegId);
  if (elementFound == NULL)
    fatalError("bug: t%d missing from the spill label list", rSpilled);

  t_spillLocation *tlabel = (t_spillLocation *)elementFound->data;

  /* create a store instruction */
  t_instruction *storeInstr = genSWGlobal(NULL, rSrc, tlabel->label, REG_T6);
  t_cfgNode *storeNode = createCFGNode(graph, storeInstr);

  /* test if we have to insert the node `storeNode' before `curCFGNode'
   * inside the basic block */
  if (before) {
    bbInsertNodeBefore(curBlock, curCFGNode, storeNode);
  } else {
    bbInsertNodeAfter(curBlock, curCFGNode, storeNode);
  }
}

void genLoadSpillVariable(t_regID rSpilled, t_regID rDest,
    t_cfg *graph, t_basicBlock *block, t_cfgNode *curCFGNode,
    t_listNode *labelBindings, bool before)
{
  t_listNode *elementFound =
      listFindWithCallback(labelBindings, &rSpilled, compareSpillLocWithRegId);
  if (elementFound == NULL)
    fatalError("bug: t%d missing from the spill label list", rSpilled);

  t_spillLocation *tlabel = (t_spillLocation *)elementFound->data;

  /* create a load instruction */
  t_instruction *loadInstr = genLWGlobal(NULL, rDest, tlabel->label);
  t_cfgNode *loadNode = createCFGNode(graph, loadInstr);

  if (before) {
    /* insert the node `loadNode' before `curCFGNode' */
    bbInsertNodeBefore(block, curCFGNode, loadNode);
    /* if the `curCFGNode' instruction has a label, move it to the new
     * load instruction */
    if ((curCFGNode->instr)->label != NULL) {
      loadInstr->label = (curCFGNode->instr)->label;
      (curCFGNode->instr)->label = NULL;
    }
  } else {
    bbInsertNodeAfter(block, curCFGNode, loadNode);
  }
}

void materializeRegAllocInBBForInstructionNode(t_cfg *graph,
    t_basicBlock *curBlock, t_spillState *state, t_cfgNode *curCFGNode,
    t_regAllocator *RA, t_listNode *spills)
{
  /* The elements in this array indicate whether the corresponding spill
   * register will be used or not by this instruction */
  bool spillSlotInUse[NUM_SPILL_REGS] = {false};
  /* This array stores whether each argument of the instruction is allocated
   * to a spill register or not.
   * For example, if argState[1].spillSlot == 2, the argState[1].reg register
   * will be materialized to the third spill register. */
  t_spillInstrArgState argState[MAX_INSTR_ARGS];

  // Analyze the current instruction
  t_instruction *instr = curCFGNode->instr;
  int numArgs = 0;
  if (instr->rDest) {
    argState[numArgs].reg = instr->rDest;
    argState[numArgs].isDestination = true;
    argState[numArgs].spillSlot = -1;
    numArgs++;
  }
  if (instr->rSrc1) {
    argState[numArgs].reg = instr->rSrc1;
    argState[numArgs].isDestination = false;
    argState[numArgs].spillSlot = -1;
    numArgs++;
  }
  if (instr->rSrc2) {
    argState[numArgs].reg = instr->rSrc2;
    argState[numArgs].isDestination = false;
    argState[numArgs].spillSlot = -1;
    numArgs++;
  }

  /* Test if a requested variable is already loaded into a register
   * from a previous instruction. */
  for (int argIdx = 0; argIdx < numArgs; argIdx++) {
    if (RA->bindings[argState[argIdx].reg->ID] != RA_SPILL_REQUIRED)
      continue;

    for (int rowIdx = 0; rowIdx < NUM_SPILL_REGS; rowIdx++) {
      if (state->regs[rowIdx].assignedTempReg != argState[argIdx].reg->ID)
        continue;

      /* update the value of used_Register */
      argState[argIdx].spillSlot = rowIdx;

      /* update the value of `assignedRegisters` */
      /* set currently used flag */
      spillSlotInUse[rowIdx] = true;

      /* test if a write back is needed. Writebacks are needed
       * when an instruction modifies a spilled register. */
      if (argState[argIdx].isDestination)
        state->regs[rowIdx].needsWB = true;

      /* a slot was found, stop searching */
      break;
    }
  }

  /* Find a slot for all other variables. Write back the variable associated
   * with the slot if necessary. */
  for (int argIdx = 0; argIdx < numArgs; argIdx++) {
    if (RA->bindings[argState[argIdx].reg->ID] != RA_SPILL_REQUIRED)
      continue;
    if (argState[argIdx].spillSlot != -1)
      continue;

    /* Check if we already have found a slot for this variable */
    bool alreadyFound = false;
    for (int otherArg = 0; otherArg < argIdx && !alreadyFound; otherArg++) {
      if (argState[argIdx].reg->ID == argState[otherArg].reg->ID) {
        argState[argIdx].spillSlot = argState[otherArg].spillSlot;
        alreadyFound = true;
      }
    }
    /* No need to do anything else in this case, the state of the
     * spill slot is already up to date */
    if (alreadyFound)
      continue;

    /* Otherwise a slot one by iterating through the slots available */
    int rowIdx;
    for (rowIdx = 0; rowIdx < NUM_SPILL_REGS; rowIdx++) {
      if (spillSlotInUse[rowIdx] == false)
        break;
    }
    /* If we don't find anything, we don't have enough spill registers!
     * This should never happen, bail out! */
    if (rowIdx == NUM_SPILL_REGS)
      fatalError("bug: spill slots exhausted");

    /* If needed, write back the old variable that was assigned to this
     * slot before reassigning it */
    if (state->regs[rowIdx].needsWB) {
      genStoreSpillVariable(state->regs[rowIdx].assignedTempReg,
          getSpillMachineRegister(rowIdx), graph, curBlock, curCFGNode,
          spills, true);
    }

    /* Update the state of this spill slot */
    spillSlotInUse[rowIdx] = true;
    argState[argIdx].spillSlot = rowIdx;
    state->regs[rowIdx].assignedTempReg = argState[argIdx].reg->ID;
    state->regs[rowIdx].needsWB = argState[argIdx].isDestination;

    /* Load the value of the variable in the spill register if not a
     * destination of the instruction */
    if (!argState[argIdx].isDestination) {
      genLoadSpillVariable(argState[argIdx].reg->ID,
          getSpillMachineRegister(rowIdx), graph, curBlock, curCFGNode,
          spills, true);
    }
  }

  /* rewrite the register identifiers to use the appropriate
   * register number instead of the variable number. */
  for (int argIdx = 0; argIdx < numArgs; argIdx++) {
    t_instrArg *curReg = argState[argIdx].reg;
    if (argState[argIdx].spillSlot == -1) {
      /* normal case */
      curReg->ID = RA->bindings[curReg->ID];
    } else {
      /* spilled register case */
      curReg->ID = getSpillMachineRegister(argState[argIdx].spillSlot);
    }
  }
}

void materializeRegAllocInBB(t_cfg *graph, t_basicBlock *curBlock,
    t_regAllocator *RA, t_listNode *spills)
{
  t_spillState state;

  /* initialize the state for this block */
  for (int counter = 0; counter < NUM_SPILL_REGS; counter++) {
    state.regs[counter].assignedTempReg = REG_INVALID;
    state.regs[counter].needsWB = false;
  }

  /* iterate through the instructions in the block */
  t_cfgNode *curCFGNode;
  t_listNode *curInnerNode = curBlock->nodes;
  while (curInnerNode != NULL) {
    curCFGNode = (t_cfgNode *)curInnerNode->data;

    /* Change the register IDs of the argument of the instruction accoring
     * to the given register allocation. Generate load and stores for spilled
     * registers */
    materializeRegAllocInBBForInstructionNode(
        graph, curBlock, &state, curCFGNode, RA, spills);

    curInnerNode = curInnerNode->next;
  }

  bool bbHasTermInstr = curBlock->nodes &&
      (isJumpInstruction(curCFGNode->instr) ||
          isHaltOrRetInstruction(curCFGNode->instr));

  /* writeback everything at the end of the basic block */
  for (int counter = 0; counter < NUM_SPILL_REGS; counter++) {
    if (state.regs[counter].needsWB == false)
      continue;
    genStoreSpillVariable(state.regs[counter].assignedTempReg,
        getSpillMachineRegister(counter), graph, curBlock, curCFGNode,
        spills, bbHasTermInstr);
  }
}

/* update the control flow informations by unsing the result
 * of the register allocation process and a list of bindings
 * between new assembly labels and spilled variables */
void materializeRegAllocInCFG(
    t_cfg *graph, t_regAllocator *RA, t_listNode *spills)
{
  t_listNode *curBlockNode = graph->blocks;
  while (curBlockNode != NULL) {
    t_basicBlock *curBlock = (t_basicBlock *)curBlockNode->data;

    materializeRegAllocInBB(graph, curBlock, RA, spills);

    /* retrieve the next basic block element */
    curBlockNode = curBlockNode->next;
  }
}


void doRegisterAllocation(t_regAllocator *regalloc)
{
  // Bind each temporary register to a physical register using the linear scan
  // algorithm. Spilled registers are all tagged with the fictitious register
  // RA_SPILL_REQUIRED.
  executeLinearScan(regalloc);

  // Generate statically allocated globals for each spilled temporary register
  t_listNode *spills = materializeSpillMemory(regalloc->program, regalloc);

  // Replace temporary register IDs with physical register IDs. In case of
  // spilled registers, add load/store instructions appropriately.
  materializeRegAllocInCFG(regalloc->graph, regalloc, spills);
  deleteSpillLocationList(spills);

  // Rewrite the program object from the CFG
  cfgToProgram(regalloc->program, regalloc->graph);
}


void dumpVariableBindings(t_regID *bindings, int numVars, FILE *fout)
{
  if (bindings == NULL)
    return;
  if (fout == NULL)
    return;

  for (int counter = 0; counter < numVars; counter++) {
    if (bindings[counter] == RA_SPILL_REQUIRED) {
      fprintf(fout, "Variable T%-3d will be spilled\n", counter);
    } else if (bindings[counter] == RA_REGISTER_INVALID) {
      fprintf(fout, "Variable T%-3d has not been assigned to any register\n",
          counter);
    } else {
      char *reg = registerIDToString(bindings[counter], 1);
      fprintf(
          fout, "Variable T%-3d is assigned to register %s\n", counter, reg);
      free(reg);
    }
  }

  fflush(fout);
}

void dumpLiveIntervals(t_listNode *intervals, FILE *fout)
{
  if (fout == NULL)
    return;

  t_listNode *curNode = intervals;
  while (curNode != NULL) {
    t_liveInterval *interval = (t_liveInterval *)curNode->data;

    fprintf(fout, "[T%-3d] Live interval: [%3d, %3d]\n", interval->tempRegID,
        interval->startPoint, interval->endPoint);
    fprintf(fout, "       Constraint set: {");

    t_listNode *i = interval->mcRegConstraints;
    while (i) {
      char *reg;

      reg = registerIDToString((t_regID)LIST_DATA_TO_INT(i->data), 1);
      fprintf(fout, "%s", reg);
      free(reg);

      if (i->next != NULL)
        fprintf(fout, ", ");
      i = i->next;
    }
    fprintf(fout, "}\n");

    curNode = curNode->next;
  }
  fflush(fout);
}

void dumpRegAllocation(t_regAllocator *RA, FILE *fout)
{
  if (RA == NULL)
    return;
  if (fout == NULL)
    return;

  fprintf(fout, "*************************\n");
  fprintf(fout, "   REGISTER ALLOCATION   \n");
  fprintf(fout, "*************************\n\n");

  fprintf(fout, "------------\n");
  fprintf(fout, " STATISTICS \n");
  fprintf(fout, "------------\n");
  fprintf(fout, "Number of available registers: %d\n", NUM_GP_REGS);
  fprintf(fout, "Number of used variables: %d\n\n", RA->tempRegNum);

  fprintf(fout, "----------------\n");
  fprintf(fout, " LIVE INTERVALS \n");
  fprintf(fout, "----------------\n");
  dumpLiveIntervals(RA->liveIntervals, fout);
  fprintf(fout, "\n");

  fprintf(fout, "----------------------------\n");
  fprintf(fout, " VARIABLE/REGISTER BINDINGS \n");
  fprintf(fout, "----------------------------\n");
  dumpVariableBindings(RA->bindings, RA->tempRegNum, fout);

  fflush(fout);
}
