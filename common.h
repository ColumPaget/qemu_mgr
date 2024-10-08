#ifndef QEMU_MGR_COMMON_H
#define QEMU_MGR_COMMON_H


#ifdef HAVE_LIBUSEFUL_5_LIBUSEFUL_H
#include "libUseful-5/libUseful.h"
#else
#include "libUseful-4/libUseful.h"
#endif


#define VERSION "1.5"

#define CONF_USE_SU   1

typedef enum {IA_UNKNOWN, IA_CLI, IA_TERMINAL, IA_XDIALOG} EInteractModes;


typedef struct
{
int Flags;
int InteractMode;
char *DialogCmd;
char *RootPassword;
char *SoundDevices;
char *QemuVersion;
} TConfig;

extern TConfig *Config;

int GetQemuMajorVersion();
char *GetNamedValue(char *RetStr, const char *Config,  const char *RequestedName);

#endif
