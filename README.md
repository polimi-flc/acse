# ACSE: Advanced Compiler System for Education

### [Downloads and quick reference](https://github.com/polimi-flc/acse/releases)
### [Documentation](https://polimi-flc.github.io/acse-docs/index.html)

ACSE is a complete toolchain consisting of a compiler (`acse` itself), an
assembler (`asrv32im`), and a simulator (`simrv32im`), developed for education
on the topic of compilers in the "Formal Languages and Compilers" course at
[Politecnico di Milano](https://www.polimi.it).

ACSE, together with its supporting tools, aims to provide a simple but
reasonably accurate sandbox for learning how a compilation toolchain works.
It is based on the standard [RISC-V](https://riscv.org) specification:
specifically, ACSE emits RV32IM assembly code files that are also compatible
with the GNU toolchain and [RARS](https://github.com/TheThirdOne/rars).

## The LANCE language

The language accepted by ACSE is called LANCE (Language for Compiler Education),
and is an extremely simplified subset of C. More in detail:

- Variable declarations must be global
- Variable initialization cannot be performed at declaration time.
- No support for functions, code is allowed in global scope
- Only two types are supported, `int` and `int[]` (fixed size array of `int`).
- Compound operation/assignment operators are missing (`+=`, `++`, ...)
- The assignment operator is not an expression operator (assignments are kinds
  of statements)
- Reduced set of control flow statements (`while`, `do-while`, `if`),
  no `break` and `continue`.
- In control statements, all code blocks must be enclosed in braces
- Only two (builtin) I/O primitives: `read` and `write` which read and write
  integers from standard input and output respectively.

Here is an example LANCE program which computes the first 10 Fibonacci numbers
by storing them in an array, and then prints them out:

```
int v[10], i;

v[0] = 0;
v[1] = 1;
i = 2;
while (i < 10) {
  v[i] = v[i-1] + v[i-2];
  i = i + 1;
}

i = 0;
while (i < 10) {
  write(v[i]);
  i = i + 1;
}
```

## How to use ACSE

ACSE is written in C99 and is supported on the following operating systems:

- **Linux** (any recent 32 bit or 64 bit distribution should work)
- **macOS** (any recent version should work)
- **Windows** (both 32 bit or 64 bit) when built with
  [MinGW](http://www.mingw.org) under [MSYS2](https://www.msys2.org), or inside
  **Windows Services for Linux** (WSL).

If you are using **Linux** or **macOS**, ACSE requires the following programs
to be installed:

- a **C compiler** (for example *GCC* or *clang*)
- **GNU bison**
- **GNU flex**


**Windows (native build via Chocolatey)**

To build ACSE directly on Windows (32- or 64-bit), we recommend using [Chocolatey](https://docs.chocolatey.org/en-us/choco/setup/) to install all required tools:

1. **Install Chocolatey**  
   Open PowerShell _as Administrator_ and run:  
   ```powershell
   Set-ExecutionPolicy Bypass -Scope Process -Force; `
     [System.Net.ServicePointManager]::SecurityProtocol = `
       [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; `
     iex ((New-Object System.Net.WebClient).DownloadString(
       'https://community.chocolatey.org/install.ps1'))

2. **Install MinGW (GCC toolchain)**

   ```powershell
   choco install mingw -y
   ```

3. **Install Bison & Flex**

   ```powershell
   choco install winflexbison3 -y
   ```

4. **Install GNU Make**

   ```powershell
   choco install make -y
   ```

5. **Adjust the ACSE Makefiles**
   In `acse/Makefile`, change the parser/lexer rules to point at the Windows executables:

   ```make
   BISON := win_bison.exe
   FLEX  := win_flex.exe
   ```

   Then, in the top-level `Makefile`, force use of `gcc` instead of `cc`:

   ```make
    .PHONY: all
    all: acse simrv32im asrv32im
    
    .PHONY: acse
    acse:
    	$(MAKE) -C acse CC=gcc
    
    .PHONY: simrv32im
    simrv32im:
    	$(MAKE) -C simrv32im CC=gcc
    
    .PHONY: asrv32im
    asrv32im:
    	$(MAKE) -C asrv32im CC=gcc
    
    .PHONY: tests
    tests: acse simrv32im asrv32im
    	$(MAKE) -C tests CC=gcc
    
    .PHONY: clean
    clean:
    	$(MAKE) -C acse clean
    	$(MAKE) -C simrv32im clean
    	$(MAKE) -C asrv32im clean
    	$(MAKE) -C tests clean
    	rm -rf bin
   ```

Alternatively you can install either the
[MSYS2](https://www.msys2.org) environment or **Windows Services for Linux**
(WSL). Both MSYS2 and Windows Services for Linux (WSL) provide a Linux-like
environment inside of Windows.

Once you have installed either MSYS2 or WSL, you can use the following
instructions just as if you were using Linux or macOS.

### Building ACSE

To build the ACSE compiler toolchain, open a terminal and type:

      make

The built executables will be located in the `bin` directory.

### Testing ACSE

To compile some examples (located in the directory `tests`) type:

      make tests

### Running ACSE

You can compile and run new Lance programs in this way (suppose you
have saved a Lance program in `myprog.src`):

      ./bin/acse myprog.src -o myprog.asm
      ./bin/asrv32im myprog.asm -o myprog.o
      ./bin/simrv32im myprog.o

Alternatively, you can add a test to the `tests` directory by following these
steps:

1. Create a new directory inside `tests`. You can choose whatever directory
   name you wish, as long as it is not `test`.
2. Move the source code files to be compiled inside the directory you have
   created. The extension of these files **must** be `.src`.
3. Run the command `make tests` to compile all tests, included the one you have
   just added.
   
The `make tests` command only runs the ACSE compiler and the assembler, you
will have to invoke the simulator manually.

All assembly files produced by ACSE are compatible with
[RARS](https://github.com/TheThirdOne/rars) so you can also run any compiled
program through it.

## Internals of ACSE

### Naming conventions and code style

All symbols are in lower camel case, and all functions that operate on a
structure have a prefix that depends on the structure. Additional prefixes are
used for special purposes:

- Functions that allocate and initialize a structure are prefixed "new".
- Functions that initialize a structure allocated externally are
  prefixed "init".
- Functions that allocate and initialize a structure, and then add it to
  a parent object, are prefixed "create".
- Functions that deinitialize and deallocate a structure are prefixed "delete".
- Functions that deinitialize a structure without deallocating it are
  prefixed "deinit".

Additional rules are used for ACSE alone:

- Functions that modify a t_program structure may have no specific prefix.
- Functions that add instructions to a t_program are prefixed "gen".

All source code files use 2 spaces for alignment and indentation.

### The compilation process

In ACSE, the lexer and parser defined in scanner.l and parser.y are generated
by Flex and Bison respectively.
The semantic actions in the parser also take care of initial code generation
-- which makes ACSE a syntax-driven translator.

The current state of the compilation process is stored in an object of type
t_program. This object contains the current intermediate representation,
consisting of the instruction list and the symbol table.
The intermediate representation is manipulated by the semantic actions by
using the functions defined in program.h and codegen.h, and is in a
symbolic and simplified form of RISC-V assembly code.
Specifically, the function prefixed with `gen` add new instructions at
the end of the instruction list, and the functions createLabel() and
assignLabel() allow for the creation and placement of labels.
The ACSE IR provides infinite temporary registers, and new (unused) registers
can be retrieved by calling getNewRegister().

After the generation of the initial IR by the frontend, the IR is normalized
by translating system function calls to lower level code, and other
pseudo-instructions not defined by the RISC-V standard are translated to
legal equivalent instruction sequences.
Then, each temporary register is allocated to a physical machine register,
spilling values to memory if the number of physical registers is not
sufficient.
Finally, the instructions of the program are written to the assembly-language
output file specified by the command line arguments.

## Authors

ACSE is copyright (c) 2008-2024 Politecnico di Milano, and is licensed as
GNU GPL version 3. It has been developed by the following contributors:

- Andrea di Biagio (main author of the original non-RISC-V version)
- Giovanni Agosta (wrote the simulator in the original non-RISC-V version,
  advisor to the RISC-V version)
- Niccolò Izzo (maintainer 2015-2020)
- Daniele Cattaneo (maintainer from 2019, main author of the RISC-V version)

Additional help and input has been provided by:

- Alessandro Barenghi
- Marcello Bersani
- Luca Breveglieri
- Stefano Cherubin
- Massimo Fioravanti
- Gabriele Magnani
- Angelo Morzenti
- Michele Scandale
- Michele Scuttari
- Ettore Speziale
- Michele Tartara

Please report any suspected bugs or defects to
`daniele.cattaneo <at> polimi.it`.
