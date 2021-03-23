#ifndef QEMU_MOUNT_H
#define QEMU_MOUNT_H

#include "common.h"

void MountDirectory(const char *ImageName, const char *MountDevice, const char *Dir, const char *Format);
void MountItem(const char *ImageName, const char *Config);


#endif
