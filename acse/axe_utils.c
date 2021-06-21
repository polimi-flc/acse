/*
 * Andrea Di Biagio
 * Politecnico di Milano, 2007
 * 
 * axe_utils.c
 * Formal Languages & Compilers Machine, 2007/2008
 * 
 */

#include <assert.h>
#include <stdio.h>
#include <inttypes.h>
#include "axe_utils.h"
#include "axe_gencode.h"
#include "symbol_table.h"
#include "axe_labels.h"
#include "axe_cflow_graph.h"
#include "axe_reg_alloc.h"
#include "axe_io_manager.h"
#include "axe_errors.h"
#include "axe_target_info.h"

extern int errorcode;
extern int line_num;
extern int num_error;
extern int num_warning;
extern t_program_infos *program;
extern t_cflow_Graph *graph;
extern t_reg_allocator *RA;
extern t_io_infos *file_infos;

extern const char *axe_version; /* generated by the makefile */

t_while_statement createWhileStatement()
{
   t_while_statement statement;

   /* initialize the WHILE informations */
   statement.label_condition = NULL;
   statement.label_end = NULL;

   /* return a new instance of `t_while_statement' */
   return statement;
}

t_axe_declaration * initializeDeclaration
      (char *ID, int isArray, int arraySize, int init_val)
{
   t_axe_declaration *result;

   /* allocate memory for the new declaration */
   result = (t_axe_declaration *)
         malloc(sizeof(t_axe_declaration));

   /* check the postconditions */
   if (result == NULL)
      return NULL;

   /* initialize the content of `result' */
   result->isArray = isArray;
   result->arraySize = arraySize;
   result->ID = ID;
   result->init_val = init_val;

   /* return the just created and initialized instance of t_axe_declaration */
   return result;
}

static void free_new_variables(t_list *variables)
{
   t_list *current_element;
   t_axe_declaration *current_decl;

   /* preconditions */
   assert(variables != NULL);

   /* initialize the value of `current_element' */
   current_element = variables;
   while (current_element != NULL)
   {
      current_decl = (t_axe_declaration *)LDATA(current_element);
      if (current_decl != NULL)
         free(current_decl);

      current_element = LNEXT(current_element);
   }

   /* free the memory associated with the list `variables' */
   freeList(variables);
}

void addVariablesFromDecls(t_program_infos *program, int varType, t_list *variables)
{
   t_list *current_element;
   t_axe_declaration *current_decl;

   /* preconditions */
   if (program == NULL)
   {
      free_new_variables(variables);
      notifyError(AXE_PROGRAM_NOT_INITIALIZED);
   }

   /* initialize `current_element' */
   current_element = variables;

   while (current_element != NULL)
   {
      /* retrieve the current declaration infos */
      current_decl = (t_axe_declaration *)LDATA(current_element);
      if (current_decl == NULL)
      {
         free_new_variables(variables);
         notifyError(AXE_NULL_DECLARATION);
      }

      /* create and assign a new variable to program */
      createVariable(program, current_decl->ID, varType, current_decl->isArray, current_decl->arraySize, current_decl->init_val);

      /* update the value of `current_element' */
      current_element = LNEXT(current_element);
   }

   /* free the linked list */
   /* initialize `current_element' */
   current_element = variables;

   while (current_element != NULL)
   {
      /* retrieve the current declaration infos */
      current_decl = (t_axe_declaration *)LDATA(current_element);

      /* assertion -- must always be verified */
      assert(current_decl != NULL);

      /* associate a register to each declared variable
       * that is not an array type */
      if (!(current_decl->isArray))
         getRegisterForSymbol(program, current_decl->ID);

      /* free the memory associated with the current declaration */
      free(current_decl);

      /* update the value of `current_element' */
      current_element = LNEXT(current_element);
   }

   freeList(variables);
}

void setProgramEnd(t_program_infos *program)
{
   if (program == NULL)
      notifyError(AXE_PROGRAM_NOT_INITIALIZED);

   if (isAssigningLabel(program->lmanager))
   {
      genHALTInstruction(program);
      return;
   }

   if (program->instructions != NULL)
   {
      t_axe_instruction *last_instr;
      t_list *last_element;

      /* get the last element of the list */
      last_element = getLastElement(program->instructions);
      assert(last_element != NULL);

      /* retrieve the last instruction */
      last_instr = (t_axe_instruction *)LDATA(last_element);
      assert(last_instr != NULL);

      if (last_instr->opcode == HALT)
         return;
   }

   genHALTInstruction(program);
   return;
}

void shutdownCompiler(int exitStatus)
{
#ifndef NDEBUG
   fprintf(stdout, "Finalizing the compiler data structures.. \n");
#endif

   /* shutdown the asm engine */
   finalizeProgramInfos(program);
   /* finalize the control flow graph informations */
   finalizeGraph(graph);
   /* finalize the register allocator */
   finalizeRegAlloc(RA);
   /* close all the files used by the compiler */
   finalizeOutputInfos(file_infos);

#ifndef NDEBUG
   fprintf(stdout, "Done. \n");
#endif

   exit(exitStatus);
}

void initializeCompiler(int argc, char **argv)
{
#ifndef NDEBUG
   fprintf(stdout, "ACSE compiler, version %s, targeting %s\n\n", 
         axe_version, TARGET_NAME);
   fprintf(stdout, "Starting the compilation process.\n\n");
#endif

   /* initialize all the global variables */
   errorcode = AXE_OK;
   line_num = -1;
   num_error = 0;
   num_warning = 0;
   program = NULL;
   graph = NULL;
   RA = NULL;
   file_infos = NULL;

#ifndef NDEBUG
   fprintf(stdout, "Initialize the compiler internal data structures. \n");
#endif

   /* initialize all the files used by the compiler */
   file_infos = initializeOutputInfos(argc, argv);
   if (file_infos == NULL)
      errorcode = AXE_OUT_OF_MEMORY;

   /* initialize the translation infos */
   program = allocProgramInfos(&errorcode);

   /* initialize the line number */
   line_num = 1;

   checkConsistency();
}
