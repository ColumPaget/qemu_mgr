
#ifndef QEMU_MGR_COMMON_H
#define QEMU_MGR_COMMON_H

#include "libUseful-4/libUseful.h"

typedef enum {IA_UNKNOWN, IA_CLI, IA_TERMINAL, IA_XDIALOG} EInteractModes;

typedef struct
{
int InteractMode;
char *DialogCmd;
char *RootPassword;
char *SoundDevices;
} TConfig;

TConfig *Config;

#endif
