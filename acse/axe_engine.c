/*
 * Andrea Di Biagio
 * Politecnico di Milano, 2007
 * 
 * axe_engine.c
 * Formal Languages & Compilers Machine, 2007/2008
 * 
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "axe_engine.h"
#include "axe_errors.h"
#include "axe_gencode.h"
#include "axe_target_info.h"

/* global variable errorcode */
int errorcode;
const char *errormsg = NULL;
/* global line number (defined in Acse.y) */
extern int line_num;
/* last line number inserted in an instruction as a comment */
int prev_line_num = -1;

/* Function used when a compare is needed between two labels */
static int compareVariables (void *Var_A, void *Var_B);

/* Finalize the memory associated with an instruction */
static void finalizeInstructions(t_list *instructions);

/* Finalize the data segment */
static void finalizeDataSegment(t_list *dataDirectives);

/* finalize the informations associated with all the variables */
static void finalizeVariables(t_list *variables);

/* add a variable to the program */
static void addVariable(t_program_infos *program, t_axe_variable *variable);

/* create and initialize an instance of `t_axe_register' */
t_axe_register * initializeRegister(int ID, int indirect)
{
   t_axe_register *result;

   /* create an instance of `t_axe_register' */
   result = (t_axe_register *)malloc(sizeof(t_axe_register));
   
   /* check the postconditions */
   if (result == NULL)
      notifyError(AXE_OUT_OF_MEMORY);

   /* initialize the new label */
   result->ID = ID;
   result->mcRegWhitelist = NULL;
   result->indirect = indirect;

   /* return the label */
   return result;
}

/* create and initialize an instance of `t_axe_instruction' */
t_axe_instruction * initializeInstruction(int opcode)
{
   t_axe_instruction *result;

   /* create an instance of `t_axe_data' */
   result = (t_axe_instruction *)malloc(sizeof(t_axe_instruction));
   
   /* check the postconditions */
   if (result == NULL)
      notifyError(AXE_OUT_OF_MEMORY);

   /* ininitialize the fields of `result' */
   result->opcode = opcode;
   result->reg_dest = NULL;
   result->reg_src1 = NULL;
   result->reg_src2 = NULL;
   result->immediate = 0;
   result->labelID = NULL;
   result->address = NULL;
   result->user_comment = NULL;
   result->mcFlags = 0;

   /* return `result' */
   return result;
}

/* create and initialize an instance of `t_axe_data' */
t_axe_data * initializeData(int directiveType, int value, t_axe_label *label)
{
   t_axe_data *result;

   /* create an instance of `t_axe_data' */
   result = (t_axe_data *) malloc(sizeof(t_axe_data));
   
   /* check the postconditions */
   if (result == NULL)
      return NULL;

   /* initialize the new directive */
   result->directiveType = directiveType;
   result->value = value;
   result->labelID = label;

   /* return the new data */
   return result;
}

/* finalize an instance of `t_axe_variable' */
void finalizeVariable (t_axe_variable *variable)
{
   free(variable);
}

/* create and initialize an instance of `t_axe_variable' */
t_axe_variable * initializeVariable
      (char *ID, int type, int isArray, int arraySize, int init_val)
{
   t_axe_variable *result;

   /* allocate memory for the new variable */
   result = (t_axe_variable *)
         malloc(sizeof(t_axe_variable));

   /* check the postconditions */
   if (result == NULL)
      return NULL;

   /* initialize the content of `result' */
   result->type = type;
   result->isArray = isArray;
   result->arraySize = arraySize;
   result->ID = ID;
   result->init_val = init_val;
   result->labelID = NULL;
   result->reg_location = REG_INVALID;

   /* return the just created and initialized instance of t_axe_variable */
   return result;
}

/* finalize an instruction info. */
void finalizeInstruction(t_axe_instruction *inst)
{
   /* preconditions */
   if (inst == NULL)
      return;
   
   /* free memory */
   if (inst->reg_dest != NULL) {
      freeList(inst->reg_dest->mcRegWhitelist);
      free(inst->reg_dest);
   }
   if (inst->reg_src1 != NULL) {
      freeList(inst->reg_src1->mcRegWhitelist);
      free(inst->reg_src1);
   }
   if (inst->reg_src2 != NULL) {
      freeList(inst->reg_src2->mcRegWhitelist);
      free(inst->reg_src2);
   }
   if (inst->address != NULL) {
      free(inst->address);
   }
   if (inst->user_comment != NULL) {
      free(inst->user_comment);
   }

   free(inst);
}

/* finalize a data info. */
void finalizeData(t_axe_data *data)
{
   if (data != NULL)
      free(data);
}

t_axe_address * initializeAddress(int type, int address, t_axe_label *label)
{
   t_axe_address *result;

   result = (t_axe_address *)malloc(sizeof(t_axe_address));

   if (result == NULL)
      notifyError(AXE_OUT_OF_MEMORY);

   /* initialize the new instance of `t_axe_address' */
   result->type = type;
   result->addr = address;
   result->labelID = label;

   /* return the new address */
   return result;
}
      
/* create a new variable */
void createVariable(t_program_infos *program, char *ID
      , int type, int isArray, int arraySize, int init_val)
{
   t_axe_variable *var;
   int sizeofElem;
   t_axe_data *new_data_info;
   int sy_error;
         
   /* test the preconditions */
   if (program == NULL)
      notifyError(AXE_PROGRAM_NOT_INITIALIZED);

   if (type != INTEGER_TYPE)
      notifyError(AXE_INVALID_TYPE);

   /* initialize a new variable */
   var = initializeVariable(ID, type, isArray, arraySize, init_val);
   if (var == NULL)
      notifyError(AXE_OUT_OF_MEMORY);
   
   if (isArray) {
      /* arrays are stored in memory and need a variable location */
      var->labelID = newLabel(program);
      if (var->labelID == NULL)
         notifyError(AXE_OUT_OF_MEMORY);
      setLabelName(program->lmanager, var->labelID, ID);

      /* create an instance of `t_axe_data' */   
      sizeofElem = 4 / TARGET_PTR_GRANULARITY;
      new_data_info = initializeData(DIR_SPACE, var->arraySize * sizeofElem,
            var->labelID);
      if (new_data_info == NULL)
         notifyError(AXE_OUT_OF_MEMORY);

      /* update the list of directives */
      program->data = addElement(program->data, new_data_info, -1);
   } else {
      /* scalars are stored in registers */
      var->reg_location = getNewRegister(program);
   }

   /* add the new variable to program */
   addVariable(program, var);
}

void finalizeDataSegment(t_list *dataDirectives)
{
   t_list *current_element;
   t_axe_data *current_data;

   /* nothing to finalize */
   if (dataDirectives == NULL)
      return;

   current_element = dataDirectives;
   while(current_element != NULL)
   {
      /* retrieve the current instruction */
      current_data = (t_axe_data *) LDATA(current_element);
      if (current_data != NULL)
         finalizeData(current_data);

      current_element = LNEXT(current_element);
   }

   /* free the list of instructions */
   freeList(dataDirectives);
}

void finalizeInstructions(t_list *instructions)
{
   t_list *current_element;
   t_axe_instruction *current_instr;

   /* nothing to finalize */
   if (instructions == NULL)
      return;

   current_element = instructions;
   while(current_element != NULL)
   {
      /* retrieve the current instruction */
      current_instr = (t_axe_instruction *) LDATA(current_element);
      if (current_instr != NULL)
         finalizeInstruction(current_instr);

      current_element = LNEXT(current_element);
   }

   /* free the list of instructions */
   freeList(instructions);
}

int compareVariables (void *Var_A, void *Var_B)
{
   t_axe_variable *va;
   t_axe_variable *vb;
   
   if (Var_A == NULL)
      return Var_B == NULL;

   if (Var_B == NULL)
      return 0;

   va = (t_axe_variable *) Var_A;
   vb = (t_axe_variable *) Var_B;

   /* test if the name is the same */
   return (!strcmp(va->ID, vb->ID));
}

t_axe_variable * getVariable
      (t_program_infos *program, char *ID)
{
   t_axe_variable search_pattern;
   t_list *elementFound;
   
   /* preconditions */
   if (program == NULL)
      notifyError(AXE_PROGRAM_NOT_INITIALIZED);

   if (ID == NULL)
      notifyError(AXE_VARIABLE_ID_UNSPECIFIED);

   /* initialize the pattern */
   search_pattern.ID = ID;
   
   /* search inside the list of variables */
   elementFound = findElementWithCallback
         (program->variables, &search_pattern, compareVariables);

   /* if the element is found return it to the caller. Otherwise return NULL. */
   if (elementFound != NULL)
      return (t_axe_variable *) LDATA(elementFound);

   return NULL;
}

int getRegLocationOfVariable(t_program_infos *program, char *ID)
{
   int sy_error;
   int location;

   /* preconditions: ID and program shouldn't be NULL pointer */
   if (ID == NULL)
      notifyError(AXE_VARIABLE_ID_UNSPECIFIED);

   if (program == NULL)
      notifyError(AXE_PROGRAM_NOT_INITIALIZED);
   
   /* get the location of the variable with the given ID */
   t_axe_variable *var = getVariable(program, ID);
   
   if (var->isArray)
      return REG_INVALID;

   location = var->reg_location;
   return location;
}

/* initialize an instance of `t_program_infos' */
t_program_infos * allocProgramInfos()
{
   t_program_infos *result;

   /* initialize the local variable `result' */
   result = (t_program_infos *)
         malloc(sizeof(t_program_infos));

   /* verify if an error occurred during the memory allocation
    * process */
   if (result == NULL)
      notifyError(AXE_OUT_OF_MEMORY);

   /* initialize the new instance of `result' */
   result->variables = NULL;
   result->instructions = NULL;
   result->instrInsPtrStack = addElement(NULL, NULL, -1);
   result->data = NULL;
   result->current_register = 1; /* we are excluding the register R0 */
   result->lmanager = initializeLabelManager();

   if (result->lmanager == NULL)
   {
      finalizeProgramInfos(result);
      notifyError(AXE_OUT_OF_MEMORY);
   }
   
   /* postcondition: return an instance of `t_program_infos' */
   return result;
}

/* add an instruction at the tail of the list `program->instructions'. */
void addInstruction(t_program_infos *program, t_axe_instruction *instr)
{
   /* test the preconditions */
   if (program == NULL)
      notifyError(AXE_PROGRAM_NOT_INITIALIZED);
   
   if (instr == NULL)
      notifyError(AXE_INVALID_INSTRUCTION);

   if (program->lmanager == NULL)
      notifyError(AXE_INVALID_LABEL_MANAGER);

   instr->labelID = getLastPendingLabel(program->lmanager);

   if (line_num >= 0 && line_num != prev_line_num) {
      instr->user_comment = calloc(20, sizeof(char));
      if (instr->user_comment) {
         snprintf(instr->user_comment, 20, "line %d", line_num);
      }
   }
   prev_line_num = line_num;

   /* update the list of instructions */
   t_list *ip = LDATA(program->instrInsPtrStack);
   if (!ip) {
      program->instructions = addElement(program->instructions, instr, 0);
      SET_DATA(program->instrInsPtrStack, program->instructions);
   } else {
      ip = addAfter(ip, instr);
      SET_DATA(program->instrInsPtrStack, ip);
   }
}

void setMCRegisterWhitelist(t_axe_register *regObj, ...)
{
   t_list *res = NULL;
   va_list args;

   va_start(args, regObj);
   int cur = va_arg(args, int);
   while (cur != REG_INVALID) {
      res = addElement(res, INTDATA(cur), -1);
      cur = va_arg(args, int);
   }
   va_end(args);

   if (regObj->mcRegWhitelist)
      freeList(regObj->mcRegWhitelist);
   regObj->mcRegWhitelist = res;
}

void removeInstructionLink(t_program_infos *program, t_list *instrLi)
{
   t_axe_instruction *instrToRemove = (t_axe_instruction *)LDATA(instrLi);

   /* move the label and/or the comment to the next instruction */
   if (instrToRemove->labelID || instrToRemove->user_comment) {
      /* find the next instruction, if it exists */
      t_list *nextPos = LNEXT(instrLi);
      t_axe_instruction *nextInst = NULL;
      if (nextPos)
         nextInst = LDATA(nextPos);
         
      /* move the label */
      if (instrToRemove->labelID) {
         /* generate a nop if there was no next instruction or if the next instruction
          * is already labeled */
         if (!nextInst || (nextInst->labelID)) {
            pushInstrInsertionPoint(program, instrLi);
            nextInst = genNOPInstruction(program);
            popInstrInsertionPoint(program);
         }
         nextInst->labelID = instrToRemove->labelID;
         instrToRemove->labelID = NULL;
      }
      
      /* move the comment, if possible; otherwise it will be discarded */
      if (nextInst && instrToRemove->user_comment && !nextInst->user_comment) {
         nextInst->user_comment = instrToRemove->user_comment;
         instrToRemove->user_comment = NULL;
      }
   }

   /* fixup the insertion pointer stack */
   t_list *ipi = program->instrInsPtrStack;
   while (ipi) {
      if (LDATA(ipi) && LDATA(ipi) == instrLi)
         SET_DATA(ipi, LPREV(instrLi));
      ipi = LNEXT(ipi);
   }

   /* remove the instruction */
   program->instructions = removeElementLink(program->instructions, instrLi);
   finalizeInstruction(instrToRemove);
}

void pushInstrInsertionPoint(t_program_infos *p, t_list *ip)
{
   prev_line_num = -1;
   p->instrInsPtrStack = addFirst(p->instrInsPtrStack, ip);
}

t_list *popInstrInsertionPoint(t_program_infos *p)
{
   prev_line_num = -1;
   t_list *ip = LDATA(p->instrInsPtrStack);

   /* affix the currently pending label, if needed */
   t_axe_label *label = getLastPendingLabel(p->lmanager);
   if (label) {
      t_list *labelPos = ip ? LNEXT(ip) : NULL;
      t_axe_instruction *instrToLabel;
      if (!labelPos)
         instrToLabel = genNOPInstruction(p);
      else
         instrToLabel = LDATA(labelPos);
      instrToLabel->labelID = label;
   }

   p->instrInsPtrStack = removeFirst(p->instrInsPtrStack);
   return ip;
}

/* reserve a new label identifier for future uses */
t_axe_label *newNamedLabel(t_program_infos *program, const char *name)
{
   /* test the preconditions */
   if (program == NULL)
      notifyError(AXE_PROGRAM_NOT_INITIALIZED);

   if (program->lmanager == NULL)
      notifyError(AXE_INVALID_LABEL_MANAGER);

   t_axe_label *label = newLabelID(program->lmanager);
   if (name)
      setLabelName(program->lmanager, label, name);
   return label;
}

t_axe_label * newLabel(t_program_infos *program)
{
   return newNamedLabel(program, NULL);
}

/* assign a new label identifier to the next instruction */
t_axe_label * assignLabel(t_program_infos *program, t_axe_label *label)
{
   /* test the preconditions */
   if (program == NULL)
      notifyError(AXE_PROGRAM_NOT_INITIALIZED);

   if (program->lmanager == NULL)
      notifyError(AXE_INVALID_LABEL_MANAGER);
   
   for (t_list *li = program->instructions; li != NULL; li = LNEXT(li)) {
      t_axe_instruction *instr = LDATA(li);
      if (instr->labelID && instr->labelID->labelID == label->labelID)
         notifyError(AXE_LABEL_ALREADY_ASSIGNED);
   }

   /* fix the label */
   return assignLabelID(program->lmanager, label);
}

/* reserve a new label identifier */
t_axe_label *assignNewNamedLabel(t_program_infos *program, const char *name)
{
   t_axe_label * reserved_label;

   /* reserve a new label */
   reserved_label = newNamedLabel(program, name);
   if (reserved_label == NULL)
      return NULL;

   /* fix the label */
   return assignLabel(program, reserved_label);
}

t_axe_label * assignNewLabel(t_program_infos *program)
{
   return assignNewNamedLabel(program, NULL);
}

void addVariable(t_program_infos *program, t_axe_variable *variable)
{
   t_axe_variable *variableFound;
   
   /* test the preconditions */
   if (variable == NULL)
   {
      notifyError(AXE_INVALID_VARIABLE);
      return;
   }

   if (program == NULL)
   {
      finalizeVariable(variable);
      notifyError(AXE_PROGRAM_NOT_INITIALIZED);
      return;
   }

   if (variable->ID == NULL)
   {
      finalizeVariable(variable);
      notifyError(AXE_VARIABLE_ID_UNSPECIFIED);
      return;
   }

   if (variable->type == UNKNOWN_TYPE)
   {
      finalizeVariable(variable);
      notifyError(AXE_INVALID_TYPE);
      return;
   }

   if (variable->isArray)
   {
      if (variable->arraySize <= 0)
      {
         finalizeVariable(variable);
         notifyError(AXE_INVALID_ARRAY_SIZE);
         return;
      }
      if (variable->labelID == NULL)
      {
         finalizeVariable(variable);
         notifyError(AXE_INVALID_LABEL);
         return;
      }
   }
   else
   {
      if (variable->reg_location == REG_INVALID)
      {
         finalizeVariable(variable);
         notifyError(AXE_INVALID_VARIABLE);
         return;
      }
   }
   
   /* we have to test if already exists a variable with the same ID */
   variableFound = getVariable(program, variable->ID);

   if (variableFound != NULL)
   {
      finalizeVariable(variable);
      notifyError(AXE_VARIABLE_ALREADY_DECLARED);
      return;
   }

   /* now we can add the new variable to the program */
   program->variables = addElement(program->variables, variable, -1);
}

int getNewRegister(t_program_infos *program)
{
   int result;
   
   /* test the preconditions */
   if (program == NULL)
      notifyError(AXE_PROGRAM_NOT_INITIALIZED);

   result = program->current_register;
   program->current_register++;
   
   /* return the current label identifier */
   return result;
}

void finalizeProgramInfos(t_program_infos *program)
{
   if (program == NULL)
      return;
   if (program->variables != NULL)
      finalizeVariables(program->variables);
   if (program->instructions != NULL)
      finalizeInstructions(program->instructions);
   if (program->data != NULL)
      finalizeDataSegment(program->data);
   if (program->lmanager != NULL)
      finalizeLabelManager(program->lmanager);

   free(program);
}

t_axe_label * getLabelFromVariableID(t_program_infos *program, char *ID)
{
   t_axe_variable *var;
   
   var = getVariable(program, ID);
   if (var == NULL)
      return NULL;

   /* test the postconditions */
   assert(var->labelID != NULL);
   
   return var->labelID;
}

void finalizeVariables(t_list *variables)
{
   t_list *current_element;
   t_axe_variable *current_var;

   if (variables == NULL)
      return;

   /* initialize the `current_element' */
   current_element = variables;
   while(current_element != NULL)
   {
      current_var = (t_axe_variable *) LDATA(current_element);
      if (current_var != NULL)
      {
         if (current_var->ID != NULL)
            free(current_var->ID);
         
         free(current_var);
      }
      
      current_element = LNEXT(current_element);
   }

   /* free the list of variables */
   freeList(variables);
}
