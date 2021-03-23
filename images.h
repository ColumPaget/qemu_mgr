#ifndef QEMU_MGR_IMAGES_H
#define QEMU_MGR_IMAGES_H

#include "common.h"

#define IMG_KVM_PRESENT 1
#define IMG_KVM_ACTIVE  2
#define IMG_PAUSED      4

typedef struct
{
int flags;
char *cpus;
int mem;
char *chardevs;
char *blockdevs;
char *jobs;
time_t start_time;
} TImageInfo;


TImageInfo *ImageGetRunningInfo(const char *Name);
void ImageInfoDestroy(void *p_ImageInfo);

void ImageAdd(const char *Name, const char *Config);
void ImageCreate(const char *Name, const char *Config);
void ImageChange(const char *Name, const char *Config);
void ImageDelete(const char *Name);
void ImagesList(const char *Tag, const char *ListType);
void OutputImageInfo(TImageInfo *Info);
int ImageStart(const char *Name, const char *Config);
int ImageStop(const char *Name, const char *Config);
int ImagePause(const char *Name, const char *Config);
int ImageResume(const char *Name, const char *Config);
int ImageReboot(const char *Name, const char *Config);
int ImageWakeup(const char *Name, const char *Config);
int ImageMediaAdd(const char *ImageName, const char *Options);
int ImageMediaRemove(const char *ImageName, const char *Options);
int ImageTakeSnapshot(const char *ImageName, const char *Options);
int ImageDriveBackup(const char *ImageName, const char *Options);
void ImagePerformAction(int Action, const char *ImageName, const char *Config);

#endif

