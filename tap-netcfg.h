#ifndef QEMU_MGR_TAP_NETCFG_H
#define QEMU_MGR_TAP_NETCFG_H

#include "common.h"

#define NET_TAP_FW_LOCAL  1
#define NET_TAP_FW_REMOTE 1


char *TapSetupQemuCommandLine(char *CmdLine, const char *ImageName, ListNode *Values);

#endif
