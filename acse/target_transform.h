/*
 * Daniele Cattaneo
 * Politecnico di Milano, 2020
 * 
 * target_transform.h
 * Formal Languages & Compilers Machine, 2007-2020
 * 
 * Transformation passes used to abstract machine-dependent details from the
 * intermediate representation
 */

#ifndef TARGET_TRANSFORM_H
#define TARGET_TRANSFORM_H

#include "program.h"

/* Perform lowering of the program to a subset of the IR which can be
 * represented as instructions of the target architecture. */
void doTargetSpecificTransformations(t_program_infos *program);

#endif