#ifndef QEMU_MGR_OS_COMMANDS_H
#define QEMU_MGR_OS_COMMANDS_H

#include "common.h"

#define RUNCMD_ROOT    1
#define RUNCMD_DAEMON  2
#define RUNCMD_NOSHELL 4


void FindOSCommands();
const char *OSCommandFindPath(const char *Command);
char *RunCommand(char *RetStr, const char *Command, int Flags);
void OSCommandQemuGetVersion();


#endif
