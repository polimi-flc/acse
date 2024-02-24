/// @file engine.c

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include "program.h"
#include "acse.h"
#include "gencode.h"
#include "target_info.h"
#include "target_asm_print.h"
#include "symbols.h"

/* global line number (defined in Acse.y) */
extern int line_num;
/* last line number inserted in an instruction as a comment */
int prev_line_num = -1;


static t_label *newLabel(unsigned int value)
{
  t_label *result;

  /* create an instance of t_label */
  result = (t_label *)malloc(sizeof(t_label));
  if (result == NULL)
    fatalError("out of memory");

  /* initialize the internal value of `result' */
  result->labelID = value;
  result->name = NULL;
  result->global = 0;
  result->isAlias = 0;

  /* return the just initialized new instance of `t_label' */
  return result;
}

void deleteLabel(t_label *lab)
{
  if (lab->name)
    free(lab->name);
  free(lab);
}

/* create and initialize an instance of `t_instrArg' */
t_instrArg *newInstrArg(t_regID ID)
{
  t_instrArg *result;

  /* create an instance of `t_instrArg' */
  result = (t_instrArg *)malloc(sizeof(t_instrArg));
  if (result == NULL)
    fatalError("out of memory");

  /* initialize the new label */
  result->ID = ID;
  result->mcRegWhitelist = NULL;

  /* return the label */
  return result;
}

/* create and initialize an instance of `t_instruction' */
t_instruction *newInstruction(int opcode)
{
  t_instruction *result;

  /* create an instance of `t_data' */
  result = (t_instruction *)malloc(sizeof(t_instruction));
  if (result == NULL)
    fatalError("out of memory");

  /* ininitialize the fields of `result' */
  result->opcode = opcode;
  result->rDest = NULL;
  result->rSrc1 = NULL;
  result->rSrc2 = NULL;
  result->immediate = 0;
  result->label = NULL;
  result->addressParam = NULL;
  result->comment = NULL;

  /* return `result' */
  return result;
}

/* create and initialize an instance of `t_data' */
t_data *newGlobal(int type, int value, t_label *label)
{
  /* create an instance of `t_data' */
  t_data *result = (t_data *)malloc(sizeof(t_data));
  if (result == NULL)
    fatalError("out of memory");

  /* initialize the new directive */
  result->type = type;
  result->value = value;
  result->label = label;

  /* return the new data */
  return result;
}

/* finalize an instruction info. */
void deleteInstruction(t_instruction *inst)
{
  /* preconditions */
  if (inst == NULL)
    return;

  /* free memory */
  if (inst->rDest != NULL) {
    deleteList(inst->rDest->mcRegWhitelist);
    free(inst->rDest);
  }
  if (inst->rSrc1 != NULL) {
    deleteList(inst->rSrc1->mcRegWhitelist);
    free(inst->rSrc1);
  }
  if (inst->rSrc2 != NULL) {
    deleteList(inst->rSrc2->mcRegWhitelist);
    free(inst->rSrc2);
  }
  if (inst->comment != NULL) {
    free(inst->comment);
  }

  free(inst);
}

/* finalize a data info. */
void deleteGlobal(t_data *data)
{
  if (data != NULL)
    free(data);
}

void deleteGlobals(t_listNode *dataDirectives)
{
  t_listNode *current_element;
  t_data *current_data;

  /* nothing to finalize */
  if (dataDirectives == NULL)
    return;

  current_element = dataDirectives;
  while (current_element != NULL) {
    /* retrieve the current instruction */
    current_data = (t_data *)current_element->data;
    if (current_data != NULL)
      deleteGlobal(current_data);

    current_element = current_element->next;
  }

  /* free the list of instructions */
  deleteList(dataDirectives);
}

void deleteInstructions(t_listNode *instructions)
{
  t_listNode *current_element;
  t_instruction *current_instr;

  /* nothing to finalize */
  if (instructions == NULL)
    return;

  current_element = instructions;
  while (current_element != NULL) {
    /* retrieve the current instruction */
    current_instr = (t_instruction *)current_element->data;
    if (current_instr != NULL)
      deleteInstruction(current_instr);

    current_element = current_element->next;
  }

  /* free the list of instructions */
  deleteList(instructions);
}

void deleteLabels(t_listNode *labels)
{
  t_listNode *current_element = labels;
  while (current_element != NULL) {
    /* retrieve the current label */
    t_label *current_label = (t_label *)current_element->data;

    /* free the memory associated with the current label */
    deleteLabel(current_label);

    /* fetch the next label */
    current_element = current_element->next;
  }
  deleteList(labels);
}

/* initialize an instance of `t_program' */
t_program *newProgram(void)
{
  t_program *result;
  t_label *l_start;

  /* initialize the local variable `result' */
  result = (t_program *)malloc(sizeof(t_program));
  if (result == NULL)
    fatalError("out of memory");

  /* initialize the new instance of `result' */
  result->symbols = NULL;
  result->instructions = NULL;
  result->data = NULL;
  result->firstUnusedReg = 1; /* we are excluding the register R0 */
  result->labels = NULL;
  result->firstUnusedLblID = 0;
  result->pendingLabel = NULL;

  /* Create the start label */
  l_start = createLabel(result);
  l_start->global = 1;
  setLabelName(result, l_start, "_start");
  assignLabel(result, l_start);

  /* postcondition: return an instance of `t_program' */
  return result;
}

void deleteProgram(t_program *program)
{
  if (program == NULL)
    return;
  if (program->symbols != NULL)
    deleteSymbols(program->symbols);
  if (program->instructions != NULL)
    deleteInstructions(program->instructions);
  if (program->data != NULL)
    deleteGlobals(program->data);
  if (program->labels != NULL)
    deleteLabels(program->labels);

  free(program);
}

t_label *createLabel(t_program *program)
{
  /* initialize a new label */
  t_label *result = newLabel(program->firstUnusedLblID);
  if (result == NULL)
    fatalError("out of memory");

  /* update the value of `current_label_ID' */
  program->firstUnusedLblID++;

  /* add the new label to the list of labels */
  program->labels = listInsert(program->labels, result, -1);

  /* return the new label */
  return result;
}


/* Set a name to a label without resolving duplicates */
void setRawLabelName(t_program *program, t_label *label, const char *finalName)
{
  t_listNode *i;

  /* check the entire list of labels because there might be two
   * label objects with the same ID and they need to be kept in sync */
  for (i = program->labels; i != NULL; i = i->next) {
    t_label *thisLab = i->data;

    if (thisLab->labelID == label->labelID) {
      /* found! remove old name */
      free(thisLab->name);
      /* change to new name */
      if (finalName)
        thisLab->name = strdup(finalName);
      else
        thisLab->name = NULL;
    }
  }
}

void setLabelName(t_program *program, t_label *label, const char *name)
{
  int serial = -1;
  size_t allocatedSpace;
  bool ok;
  char *sanitizedName, *finalName, *dstp;
  const char *srcp;

  /* remove all non a-zA-Z0-9_ characters */
  sanitizedName = calloc(strlen(name) + 1, sizeof(char));
  srcp = name;
  for (dstp = sanitizedName; *srcp; srcp++) {
    if (*srcp == '_' || isalnum(*srcp))
      *dstp++ = *srcp;
  }

  /* append a sequential number to disambiguate labels with the same name */
  allocatedSpace = strlen(sanitizedName) + 24;
  finalName = calloc(allocatedSpace, sizeof(char));
  snprintf(finalName, allocatedSpace, "%s", sanitizedName);
  do {
    t_listNode *i;
    ok = true;
    for (i = program->labels; i != NULL; i = i->next) {
      t_label *thisLab = i->data;
      char *thisLabName;
      int difference;

      if (thisLab->labelID == label->labelID)
        continue;

      thisLabName = getLabelName(thisLab);
      difference = strcmp(finalName, thisLabName);
      free(thisLabName);

      if (difference == 0) {
        ok = false;
        snprintf(finalName, allocatedSpace, "%s_%d", sanitizedName, ++serial);
        break;
      }
    }
  } while (!ok);

  free(sanitizedName);
  setRawLabelName(program, label, finalName);
  free(finalName);
}

/* assign a new label identifier to the next instruction */
void assignLabel(t_program *program, t_label *label)
{
  /* check if this label has already been assigned */
  for (t_listNode *li = program->instructions; li != NULL; li = li->next) {
    t_instruction *instr = li->data;
    if (instr->label && instr->label->labelID == label->labelID)
      fatalError("bug: label already assigned");
  }

  /* test if the next instruction already has a label */
  if (program->pendingLabel != NULL) {
    /* It does: transform the label being assigned into an alias of the
     * label of the next instruction's label
     * All label aliases have the same ID and name. */

    /* Decide the name of the alias. If only one label has a name, that name
     * wins. Otherwise the name of the label with the lowest ID wins */
    char *name = program->pendingLabel->name;
    if (!name ||
        (label->labelID && label->labelID < program->pendingLabel->labelID))
      name = label->name;
    /* copy the name */
    if (name)
      name = strdup(name);

    /* Change ID and name */
    label->labelID = (program->pendingLabel)->labelID;
    setRawLabelName(program, label, name);

    /* Promote both labels to global if at least one is global */
    if (label->global)
      program->pendingLabel->global = 1;
    else if (program->pendingLabel->global)
      label->global = 1;

    /* mark the label as an alias */
    label->isAlias = 1;

    free(name);
  } else {
    program->pendingLabel = label;
  }
}

char *getLabelName(t_label *label)
{
  char *buf;

  if (label->name) {
    buf = strdup(label->name);
  } else {
    buf = calloc(24, sizeof(char));
    snprintf(buf, 24, "l_%d", label->labelID);
  }

  return buf;
}

/* add an instruction at the tail of the list `program->instructions'. */
void addInstruction(t_program *program, t_instruction *instr)
{
  /* assign the currently pending label if there is one */
  instr->label = program->pendingLabel;
  program->pendingLabel = NULL;

  /* add a comment with the line number */
  if (line_num >= 0 && line_num != prev_line_num) {
    instr->comment = calloc(20, sizeof(char));
    if (instr->comment) {
      snprintf(instr->comment, 20, "line %d", line_num);
    }
  }
  prev_line_num = line_num;

  /* update the list of instructions */
  program->instructions = listInsert(program->instructions, instr, -1);
}

t_instruction *genInstruction(t_program *program, int opcode, t_regID r_dest,
    t_regID r_src1, t_regID r_src2, t_label *label, int immediate)
{
  t_instruction *instr;

  /* create an instance of `t_instruction' */
  instr = newInstruction(opcode);

  /* initialize the instruction's registers */
  if (r_dest != REG_INVALID)
    instr->rDest = newInstrArg(r_dest);
  if (r_src1 != REG_INVALID)
    instr->rSrc1 = newInstrArg(r_src1);
  if (r_src2 != REG_INVALID)
    instr->rSrc2 = newInstrArg(r_src2);

  /* attach an address if needed */
  if (label)
    instr->addressParam = label;

  /* initialize the immediate field */
  instr->immediate = immediate;

  /* add the newly created instruction to the current program */
  if (program != NULL)
    addInstruction(program, instr);

  /* return the load instruction */
  return instr;
}

void removeInstructionAt(t_program *program, t_listNode *instrLi)
{
  t_instruction *instrToRemove = (t_instruction *)instrLi->data;

  /* move the label and/or the comment to the next instruction */
  if (instrToRemove->label || instrToRemove->comment) {
    /* find the next instruction, if it exists */
    t_listNode *nextPos = instrLi->next;
    t_instruction *nextInst = NULL;
    if (nextPos)
      nextInst = nextPos->data;

    /* move the label */
    if (instrToRemove->label) {
      /* generate a nop if there was no next instruction or if the next
       * instruction is already labeled */
      if (!nextInst || (nextInst->label)) {
        nextInst = genNOP(NULL);
        program->instructions =
            listInsertAfter(program->instructions, instrLi, nextInst);
      }
      nextInst->label = instrToRemove->label;
      instrToRemove->label = NULL;
    }

    /* move the comment, if possible; otherwise it will be discarded */
    if (nextInst && instrToRemove->comment && !nextInst->comment) {
      nextInst->comment = instrToRemove->comment;
      instrToRemove->comment = NULL;
    }
  }

  /* remove the instruction */
  program->instructions = listRemoveNode(program->instructions, instrLi);
  deleteInstruction(instrToRemove);
}

t_regID getNewRegister(t_program *program)
{
  t_regID result = program->firstUnusedReg;
  program->firstUnusedReg++;

  /* return the current label identifier */
  return result;
}

t_data *genDataDeclaration(
    t_program *program, int type, int value, t_label *label)
{
  t_data *res = newGlobal(type, value, label);
  program->data = listInsert(program->data, res, -1);
  return res;
}

void genProgramEpilog(t_program *program)
{
  if (program->pendingLabel != NULL) {
    genExit0Syscall(program);
    return;
  }

  if (program->instructions != NULL) {
    /* get the last element of the list */
    t_listNode *last_element = listGetLastNode(program->instructions);

    /* retrieve the last instruction */
    t_instruction *last_instr = (t_instruction *)last_element->data;

    if (last_instr->opcode == OPC_CALL_EXIT_0)
      return;
  }

  genExit0Syscall(program);
  return;
}

void dumpProgram(t_program *program, FILE *fout)
{
  t_listNode *cur_var, *cur_inst;

  fprintf(fout, "**************************\n");
  fprintf(fout, "          PROGRAM         \n");
  fprintf(fout, "**************************\n\n");

  fprintf(fout, "-----------\n");
  fprintf(fout, " VARIABLES\n");
  fprintf(fout, "-----------\n");
  cur_var = program->symbols;
  while (cur_var) {
    t_symbol *var = cur_var->data;
    fprintf(fout, "[%s]\n", var->ID);

    if (var->type == TYPE_INT) {
      fprintf(fout, "   type = int\n");
    } else if (var->type == TYPE_INT_ARRAY) {
      fprintf(fout, "   type = int[%d]\n", var->arraySize);
    } else {
      fprintf(fout, "   type = invalid\n");
    }
    char *labelName = getLabelName(var->label);
    fprintf(fout, "   label = %s (ID=%d)\n", labelName, var->label->labelID);
    free(labelName);

    cur_var = cur_var->next;
  }

  fprintf(fout, "\n--------------\n");
  fprintf(fout, " INSTRUCTIONS\n");
  fprintf(fout, "--------------\n");
  cur_inst = program->instructions;
  while (cur_inst) {
    t_instruction *instr = cur_inst->data;
    if (instr == NULL)
      fprintf(fout, "(null)");
    else
      printInstruction(instr, fout, false);
    fprintf(fout, "\n");
    cur_inst = cur_inst->next;
  }

  fflush(fout);
}
