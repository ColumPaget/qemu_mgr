#ifndef QEMU_MGR_XDIALOG_H
#define QEMU_MGR_XDIALOG_H

#include "common.h"


void XDialogFindXDialogCommand(const char *XDialogCommandList);
int XDialogSetup(const char *Config);
char *XDialogQuery(char *RetStr, const char *Title, const char *Text);
const char *XDialogQueryRootPassword(const char *Title);

#endif
