
#ifndef QEMU_MGR_COMMON_H
#define QEMU_MGR_COMMON_H

#include "libUseful-4/libUseful.h"

#define VERSION "1.1"

typedef enum {IA_UNKNOWN, IA_CLI, IA_TERMINAL, IA_XDIALOG} EInteractModes;

typedef struct
{
int InteractMode;
char *DialogCmd;
char *RootPassword;
char *SoundDevices;
} TConfig;

TConfig *Config;

char *GetNamedValue(char *RetStr, const char *Config,  const char *RequestedName);

#endif
