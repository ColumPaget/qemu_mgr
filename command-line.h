#ifndef QEMU_MGR_COMMAND_LINE_H
#define QEMU_MGR_COMMAND_LINE_H

#include "common.h"

void CommandLineSetConfig(ListNode *Config, const char *CommandLineOptions);
ListNode *CommandLineParse(int argc, char *argv[]);

#endif
