#ifndef COMPILER_H
#define COMPILER_H

#include "scanner.h"
#include "vm.h"

Inst* compile(int *programLength);

#endif
