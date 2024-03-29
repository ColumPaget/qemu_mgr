#ifndef QEMU_MGR_XDIALOG_H
#define QEMU_MGR_XDIALOG_H

#include "common.h"


void XDialogFindXDialogCommand(const char *XDialogCommandList);
int XDialogSetup(const char *Config);
char *XDialogQuery(char *RetStr, const char *Title, const char *Text);
char *XDialogQueryPassword(char *RetStr, const char *Title, const char *Text);
pid_t XDialogBusyWindow(const char *Title, const char *Msg);
pid_t XDialogErrorMessage(const char *Title, const char *Msg);

#endif
