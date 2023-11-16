#ifndef SWTCH_H_
#define SWTCH_H_

#include "process/process.h"

void swtch(struct context *save, struct context *jump);

#endif
