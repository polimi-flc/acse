/*
 * Andrea Di Biagio
 * Politecnico di Milano, 2007
 * 
 * axe_engine.h
 * Formal Languages & Compilers Machine, 2007/2008
 *
 * Contains t_program_infos and some functions for label management
 * (reserve, fix, assign) 
 */

#ifndef _AXE_ENGINE_H
#define _AXE_ENGINE_H

#include "axe_labels.h"
#include "collections.h"

/* registers */
#define REG_INVALID -1
#define REG_0 0

typedef struct t_axe_register
{
   int ID;        /* an identifier of the register */
   int indirect;  /* a boolean value: 1 if the register value is a pointer */
   t_list *mcRegWhitelist;  /* the list of machine registers where this 
                             * variable can be allocated. NULL if any register 
                             * is allowed. */
}t_axe_register;

/* ADDRESS TYPES */
#define ADDRESS_TYPE 0
#define LABEL_TYPE 1

typedef struct t_axe_address
{
   int addr;               /* a Program Counter */
   t_axe_label *labelID;   /* a label identifier */
   int type;               /* one of ADDRESS_TYPE or LABEL_TYPE */
}t_axe_address;

/* data types */
#define INTEGER_TYPE 0
#define UNKNOWN_TYPE -1

/* A structure that defines the internal data of a `Acse variable' */
typedef struct t_axe_variable
{
   int type;      /* a valid data type */
   int isArray;   /* must be TRUE if the current variable is an array */
   int arraySize; /* the size of the array. This information is useful only
                   * if the field `isArray' is TRUE */
   int init_val;  /* initial value of the current variable. Actually it is
                   * implemented as a integer value. `int' is
                   * the only supported type at the moment,
                   * future developments could consist of a modification of
                   * the supported type system. Thus, maybe init_val will be
                   * modified in future. */
   char *ID;               /* variable identifier (should never be a NULL
                            * pointer or an empty string "") */
   t_axe_label *label;     /* a label that refers to the location
                            * of the variable inside the data segment */
   int reg_location;       /* the register ID associated to the variable */
} t_axe_variable;

/* MACE opcodes */
#define OPC_INVALID -1

/* a symbolic assembly instruction */
typedef struct t_axe_instruction
{
   int opcode;                   /* instruction opcode (for example: ADD) */
   t_axe_register *reg_dest;     /* destination register */
   t_axe_register *reg_src1;     /* first source register */
   t_axe_register *reg_src2;     /* second source register */
   int immediate;                /* immediate value */
   t_axe_address *address;       /* an address operand */
   int mcFlags;                  /* flags; used only by the backend */
   char *user_comment;           /* if defined it is set to the source code
                                  * instruction that generated the current
                                  * assembly. This string will be written
                                  * into the output code as a comment */
   t_axe_label *labelID;         /* a label associated with the current
                                  * instruction */
}t_axe_instruction;

/* DIRECTIVE TYPES */
#define DIR_WORD 0
#define DIR_SPACE 1
#define DIR_INVALID -1

/* this structure is used in order to define assembler directives.
 * Directives are used in many cases such the definition of variables
 * inside the data segment. Every instance `t_axe_data' contains
 * all the informations about a single directive.
 * An example is the directive .word that is required when the assembler
 * must reserve a word of data inside the data segment. */
typedef struct t_axe_data
{
   int directiveType;      /* the type of the current directive
                            * (for example: DIR_WORD) */
   int value;              /* the value associated with the directive */
   t_axe_label *labelID;   /* label associated with the current data */
}t_axe_data;

typedef struct t_program_infos
{
  t_list *variables;
  t_list *instructions;
  t_list *instrInsPtrStack;
  t_list *data;
  t_axe_label_manager *lmanager;
  int current_register;
} t_program_infos;


/*  PROGRAM INFO  */

/* initialize the informations associated with the program. This function is
 * called at the beginning of the translation process. This function
 * is called once: its only purpouse is to initialize an instance of the struct
 * `t_program_infos' that will contain all the informations about the program
 * that will be compiled */
extern t_program_infos *allocProgramInfos();

/* finalize all the data structures associated with `program' */
extern void finalizeProgramInfos(t_program_infos *program);

/*  INSTRUCTIONS  */

/* get a register still not used. This function returns
 * the ID of the register found*/
extern int getNewRegister(t_program_infos *program);

/* create an instance of `t_axe_register' */
extern t_axe_register *initializeRegister(int ID, int indirect);

/* create an instance of `t_axe_instruction' */
extern t_axe_instruction *initializeInstruction(int opcode);

/* create an instance of `t_axe_address' */
extern t_axe_address *initializeAddress(int type, int address,
      t_axe_label *label);

/* finalize an instruction info. */
extern void finalizeInstruction(t_axe_instruction *inst);

/* Set the list of allowed machine registers for a specific register object
 * to the specified list of register identifiers. The list must be terminated
 * by REG_INVALID or -1. */
extern void setMCRegisterWhitelist(t_axe_register *regObj, ...);

/* add a new instruction to the current program. This function is directly
 * called by all the functions defined in `axe_gencode.h' */
extern void addInstruction(t_program_infos *program, t_axe_instruction *instr);

extern t_axe_instruction *genInstruction(t_program_infos *program,
      int opcode, t_axe_register *r_dest, t_axe_register *r_src1,
      t_axe_register *r_src2, t_axe_label *label, int immediate);

/* remove an instruction from the program, given its link in the instruction
 * list. */
extern void removeInstructionLink(t_program_infos *program, t_list *instrLi);

/* Save the current insertion point in the instruction list, and replace it
 * with `ip`. New instructions will be inserted after the `ip` instruction.
 * To insert instructions at the beginning of the program, ip shall be NULL. */
extern void pushInstrInsertionPoint(t_program_infos *p, t_list *ip);

/* Restore the last insertion point in the instruction list. Returns the
 * previous position of the instruction insertion point. */
extern t_list *popInstrInsertionPoint(t_program_infos *p);

/*  LABELS  */

/* reserve a new label identifier and return the identifier to the caller */
extern t_axe_label *newLabel(t_program_infos *program);

/* assign the given label identifier to the next instruction. Returns
 * the label assigned; otherwise (an error occurred) NULL */
extern t_axe_label *assignLabel(t_program_infos *program, t_axe_label *label);

/* reserve and fix a new label. It returns either the label assigned or
 * NULL if an error occurred */
extern t_axe_label *assignNewLabel(t_program_infos *program);

/* Like the above functions, but with the ability to give a name to the label.
 * If another label with the same name already exists, the name assigned to
 * the new label will be modified to remove any ambiguity. */
extern t_axe_label *newNamedLabel(t_program_infos *program, const char *name);
extern t_axe_label *assignNewNamedLabel(
      t_program_infos *program, const char *name);

/*  VARIABLES  */

/* add a variable to the program */
extern void createVariable(t_program_infos *program, char *ID, int type,
      int isArray, int arraySize, int init_val);

/* get a previously allocated variable */
extern t_axe_variable *getVariable(t_program_infos *program, char *ID);

/* Given a variable identifier (ID) this function
 * returns a register location where the value is stored
 * (the value of the variable identified by `ID'). If an error occurs,
 * getRegLocationOfScalar returns REG_INVALID */
extern int getRegLocationOfScalar(t_program_infos *program, char *ID);

/* get the label that marks the starting address of the array variable
 * with name "ID" */
extern t_axe_label *getMemLocationOfArray(t_program_infos *program, char *ID);

/*  DATA DIRECTIVES  */

/* create an instance of `t_axe_data' */
extern t_axe_data *initializeData(int directiveType, int value,
      t_axe_label *label);

/* finalize a data info. */
extern void finalizeData(t_axe_data *data);

#endif
