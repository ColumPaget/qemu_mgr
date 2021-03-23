#ifndef QEMU_MGR_DEVICES_H
#define QEMU_MGR_DEVICES_H

#include "common.h"

typedef struct
{
char *Manufacturer;
char *Product;
char *Serial;
int Speed;
int MaxPower;
int BusNum;
int DevNum;
} TDevice;


void DeviceInfoDestroy(void *p_Info);
int DevicesGetUSB(ListNode *List);


#endif
