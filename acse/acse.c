/// @file options.c

#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdarg.h>
#include "utils.h"
#include "acse.h"
#include "target_info.h"
#include "program.h"
#include "utils.h"
#include "cflow_graph.h"
#include "target_asm_print.h"
#include "target_transform.h"
#include "target_info.h"
#include "cflow_graph.h"
#include "reg_alloc.h"
#include "parser.h"

typedef struct t_options {
  const char *outputFileName;
  const char *inputFileName;
} t_options;
t_options compilerOptions;

int num_error;

/* generated bxwy the makefile */
extern const char *axe_version;

static void printMessage(const char *category, const char *fmt, va_list arg)
{
  const char *fn = compilerOptions.inputFileName;
  if (line_num >= 0)
    fprintf(stderr, "%s:%d: %s: ", fn, line_num, category);
  else
    fprintf(stderr, "%s: ", category);
  vfprintf(stderr, fmt, arg);
  fputc('\n', stderr);
}

void emitWarning(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  printMessage("warning", format, args);
  va_end(args);
}

void emitError(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  printMessage("error", format, args);
  va_end(args);
  num_error++;
}

void fatalError(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  printMessage("fatal error", format, args);
  va_end(args);
  exit(1);
}


char *getLogFileName(const char *logType)
{
  char *outfn, *basename;
  size_t nameLen;
  int lastDot, i;

  basename = strdup(compilerOptions.outputFileName);
  if (!basename)
    fatalError("out of memory");

  lastDot = -1;
  for (i = 0; basename[i] != '\0'; i++) {
    if (basename[i] == '.')
      lastDot = i;
  }
  if (lastDot >= 0)
    basename[lastDot] = '\0';

  nameLen = strlen(basename) + strlen(logType) + (size_t)8;
  outfn = calloc(nameLen, sizeof(char));
  if (!outfn)
    fatalError("out of memory");

  snprintf(outfn, nameLen, "%s_%s.log", basename, logType);
  free(basename);
  return outfn;
}


void banner(void)
{
  printf("ACSE %s compiler, version %s\n", TARGET_NAME, axe_version);
}

void usage(const char *name)
{
  banner();
  printf("usage: %s [options] input\n\n", name);
  puts("Options:");
  puts("  -o ASMFILE    Name the output ASMFILE (default output.asm)");
  puts("  -h, --help    Displays available options");
}

int main(int argc, char *argv[])
{
  char *name = argv[0];
  int ch;
  static const struct option options[] = {
      {"help", no_argument, NULL, 'h'},
  };

  compilerOptions.inputFileName = NULL;
  compilerOptions.outputFileName = "output.asm";

  while ((ch = getopt_long(argc, argv, "ho:", options, NULL)) != -1) {
    switch (ch) {
      case 'o':
        compilerOptions.outputFileName = optarg;
        break;
      case 'h':
        usage(name);
        return 1;
      default:
        usage(name);
        return 1;
    }
  }
  argc -= optind;
  argv += optind;

  if (argc > 1) {
    fprintf(stderr, "Cannot compile more than one file, exiting.\n");
    return 1;
  } else if (argc == 1) {
    compilerOptions.inputFileName = argv[0];
  }

#ifndef NDEBUG
  banner();
  printf("\n");
#endif

  /* initialize the translation infos */
  program = newProgram();

  debugPrintf("Parsing the input program\n");
  /* Open the input file */
  FILE *fp;
  if (compilerOptions.inputFileName != NULL) {
    fp = fopen(compilerOptions.inputFileName, "r");
    if (fp == NULL)
      fatalError("could not open the input file");
    debugPrintf(" -> Reading input from \"%s\"\n", compilerOptions.inputFileName);
  } else {
    fp = stdin;
    debugPrintf(" -> Reading from standard input\n");
  }
  int num_errors = parseProgram(fp);

  if (num_errors > 0) {
    /* Syntax errors have happened... */
    fprintf(stderr, "Input contains errors, no assembly file written.\n");
    fprintf(stderr, "%d error(s) found.\n", num_errors);
  } else {
    debugPrintf("Lowering of pseudo-instructions to machine instructions.\n");
    doTargetSpecificTransformations(program);

    debugPrintf("Performing register allocation.\n");
    doRegisterAllocation(program);

    debugPrintf("Writing the assembly file.\n");
    debugPrintf(" -> Output file name: \"%s\"\n", compilerOptions.outputFileName);
    fp = fopen(compilerOptions.outputFileName, "w");
    if (fp == NULL)
      fatalError("could not open the output file");
    writeAssembly(program, fp);
  }
  
  debugPrintf("Finalizing the compiler data structures.\n");
  deleteProgram(program);

  debugPrintf("Done.\n");
  return 0;
}