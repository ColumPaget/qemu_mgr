
#ifndef QEMU_MGR_QMP_H
#define QEMU_MGR_QMP_H

#include "common.h"

#define BD_INCLUDE_MEDIA 1
#define BD_MOUNTED       2
#define BD_REMOVABLE     4

STREAM *QMPOpen(const char *ImageName);
ListNode *QMPCommand(STREAM *S, const char *Cmd);
ListNode *QMPTransact(const char *ImageName, const char *Query);
int QMPSetVNCPassword(const char *ImageName, const char *Password);
int QMPIsError(ListNode *Qmp);

char *QMPListBlockDevs(char *RetStr, const char *ImageName, int Flags);
void QMPSendKey(const char *ImageName, const char *Input);
void QMPSendString(const char *ImageName, const char *String);
void QMPSendText(const char *ImageName);

#endif
