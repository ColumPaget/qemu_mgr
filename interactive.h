#ifndef QEMU_MGR_INTERACTIVE_H
#define QEMU_MGR_INTERACTIVE_H

#include "common.h"

int InteractiveSetup(const char *Config);
int InteractiveChooseMode(const char *Hint);
const char *InteractiveQueryRootPassword(const char *Title);

#endif
