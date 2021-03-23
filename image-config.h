#ifndef IMAGE_CONFIG_H
#define IMAGE_CONFIG_H

#include "common.h"

char *ImageConfigExpand(char *RetStr, ListNode *ConfTree);
void ImageConfigUpdate(ListNode *Config, const char *NewSettings);
STREAM *ImageConfigOpen(const char *ImageName, const char *Mode);
ListNode *ImageConfigLoad(const char *ImageName);
void ImageConfigSave(STREAM *S, const char *ImageName, const char *Config);




#endif
