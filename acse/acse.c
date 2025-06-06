/// @file acse.c
/// @brief Main file of the ACSE compiler

#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include "list.h"
#include "target_info.h"
#include "program.h"
#include "target_asm_print.h"
#include "target_transform.h"
#include "cfg.h"
#include "reg_alloc.h"
#include "parser.h"
#include "errors.h"


char *getLogFileName(const char *logType, const char *fn)
{
  char *basename = strdup(fn);
  if (!basename)
    fatalError("out of memory");

  int lastDot = -1;
  for (int i = 0; basename[i] != '\0'; i++) {
    if (basename[i] == '.')
      lastDot = i;
  }
  if (lastDot >= 0)
    basename[lastDot] = '\0';

  size_t nameLen = strlen(basename) + strlen(logType) + (size_t)8;
  char *outfn = calloc(nameLen, sizeof(char));
  if (!outfn)
    fatalError("out of memory");

  snprintf(outfn, nameLen, "%s_%s.log", basename, logType);
  free(basename);
  return outfn;
}


void banner(void)
{
  printf("ACSE %s compiler, (c) 2008-24 Politecnico di Milano\n", TARGET_NAME);
}

void version(void)
{
  puts("ACSE toolchain version 2.0.2");
  printf("Target: %s\n", TARGET_NAME);
}

void usage(const char *name)
{
  banner();
  printf("usage: %s [options] input\n\n", name);
  puts("Options:");
  puts("  -o ASMFILE    Name the output ASMFILE (default output.asm)");
  puts("  -v, --version Display version number");
  puts("  -h, --help    Displays available options");
}

int main(int argc, char *argv[])
{
  char *name = argv[0];
  int ch, res = 0;
#ifndef NDEBUG
  char *logFn;
  FILE *logFp;
#endif
  static const struct option options[] = {
      {   "help", no_argument, NULL, 'h'},
      {"version", no_argument, NULL, 'v'},
  };

  char *outputFn = "output.asm";

  while ((ch = getopt_long(argc, argv, "ho:v", options, NULL)) != -1) {
    switch (ch) {
      case 'o':
        outputFn = optarg;
        break;
      case 'h':
        usage(name);
        return 1;
      case 'v':
        version();
        return 1;
      default:
        usage(name);
        return 1;
    }
  }
  argc -= optind;
  argv += optind;

  if (argc < 1) {
    usage(name);
    return 1;
  } else if (argc > 1) {
    emitError(nullFileLocation, "cannot assemble more than one file");
    return 1;
  }

#ifndef NDEBUG
  banner();
  printf("\n");
#endif

  res = 1;

#ifndef NDEBUG
  fprintf(stderr, "Parsing the input program\n");
  fprintf(stderr, " -> Reading input from \"%s\"\n", argv[0]);
#endif
  t_program *program = parseProgram(argv[0]);
  if (!program)
    goto fail;
#ifndef NDEBUG
  logFn = getLogFileName("frontend", outputFn);
  logFp = fopen(logFn, "w");
  if (logFp) {
    fprintf(stderr, " -> Writing the output of parsing to \"%s\"\n", logFn);
    programDump(program, logFp);
    fclose(logFp);
  }
  free(logFn);
#endif

#ifndef NDEBUG
  fprintf(stderr, "Lowering of pseudo-instructions to machine instructions.\n");
#endif
  doTargetSpecificTransformations(program);

#ifndef NDEBUG
  fprintf(stderr, "Performing register allocation.\n");
  logFn = getLogFileName("controlFlow", outputFn);
  logFp = fopen(logFn, "w");
  if (logFp) {
    fprintf(stderr, " -> Writing the control flow graph to \"%s\"\n", logFn);
    t_cfg *cfg = programToCFG(program);
    cfgComputeLiveness(cfg);
    cfgDump(cfg, logFp, true);
    deleteCFG(cfg);
    fclose(logFp);
  }
  free(logFn);
#endif
  t_regAllocator *regAlloc = newRegAllocator(program);
  regallocRun(regAlloc);
#ifndef NDEBUG
  logFn = getLogFileName("regAlloc", outputFn);
  logFp = fopen(logFn, "w");
  if (logFp) {
    fprintf(stderr, " -> Writing the register bindings to \"%s\"\n", logFn);
    regallocDump(regAlloc, logFp);
    fclose(logFp);
  }
  free(logFn);
#endif
  deleteRegAllocator(regAlloc);

#ifndef NDEBUG
  fprintf(stderr, "Writing the assembly file.\n");
  fprintf(stderr, " -> Output file name: \"%s\"\n", outputFn);
  fprintf(stderr, " -> Code segment size: %d instructions\n",
      listLength(program->instructions));
  fprintf(stderr, " -> Data segment size: %d elements\n",
      listLength(program->symbols));
  fprintf(stderr, " -> Number of labels: %d\n", listLength(program->labels));
#endif
  bool ok = writeAssembly(program, outputFn);
  if (!ok) {
    emitError(nullFileLocation, "could not write output file");
    goto fail;
  }

  res = 0;
fail:
  deleteProgram(program);
#ifndef NDEBUG
  fprintf(stderr, "Finished.\n");
#endif
  return res;
}
