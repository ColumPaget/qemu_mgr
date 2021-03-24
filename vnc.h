#ifndef QEMU_MGR_VNC_H
#define QEMU_MGR_VNC_H

#include "common.h"


char *VNCBuildViewerList(char *RetStr);
char *VNCGetInfo(char *RetStr, const char *ImageName);
int VNCSetPassword(const char *ImageName, const char *Password);
void VNCLaunchViewer(const char *Viewer, ListNode *Config);
int VNCConnect(const char *ImageName);

#endif
