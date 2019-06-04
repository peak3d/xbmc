/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "platform/android/activity/XBMCApp.h"
#include "utils/MemUtils.h"

#include <stdlib.h>

#undef ALIGN
#define ALIGN(value, alignment) (((value)+(alignment-1))&~(alignment-1))

namespace KODI
{
namespace MEMORY
{

// aligned memory allocation.
// in order to do so - we alloc extra space and store the original allocation in it (so that we can free later on).
// the returned address will be the nearest aligned address within the space allocated.
void* AlignedMalloc(size_t s, size_t alignTo)
{
#if __ANDROID_API__ >= 28
  if (CXBMCApp::get()->getActivity()->sdkVersion >= 28)
  {
    return aligned_alloc(alignTo, s);
  }
  else
#endif
  {
    char *pFull = (char*)malloc(s + alignTo + sizeof(char *));
    char *pAligned = (char *)ALIGN(((unsigned long)pFull + sizeof(char *)), alignTo);

    *(char **)(pAligned - sizeof(char*)) = pFull;

    return(pAligned);
  }
}

void AlignedFree(void* p)
{
  if (!p)
    return;

#if __ANDROID_API__ >= 28
  if (CXBMCApp::get()->getActivity()->sdkVersion >= 28)
  {
    free(p);
  }
  else
#endif
  {
    char *pFull = *(char **)(((char *)p) - sizeof(char *));
    free(pFull);
  }
}

void GetMemoryStatus(MemoryStatus* buffer)
{
  if (!buffer)
    return;

  long availMem, totalMem;

  if (CXBMCApp::get()->GetMemoryInfo(availMem, totalMem))
  {
    memset(buffer,0, sizeof(*buffer));
    buffer->totalPhys = totalMem;
    buffer->availPhys = availMem;
  }
}

}
}
