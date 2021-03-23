
#ifndef QEMU_MGR_QMP_H
#define QEMU_MGR_QMP_H

#include "common.h"


STREAM *QMPOpen(const char *ImageName);
ListNode *QMPCommand(STREAM *S, const char *Cmd);
ListNode *QMPTransact(const char *ImageName, const char *Query);
int QMPSetVNCPassword(const char *ImageName, const char *Password);
int QMPIsError(ListNode *Qmp);

void QMPSendKey(const char *ImageName, const char *Config);
void QMPSendText(const char *ImageName);

#endif
