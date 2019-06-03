/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/MemUtils.h"

#include <array>
#include <cstdlib>
#include <cstring>

#include <stdio.h>
#include <unistd.h>

#include <sys/sysctl.h>
#include <sys/types.h>

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
  char *pFull = (char*)malloc(s + alignTo + sizeof(char *));
  char *pAligned = (char *)ALIGN(((unsigned long)pFull + sizeof(char *)), alignTo);

  *(char **)(pAligned - sizeof(char*)) = pFull;

  return(pAligned);
}

void AlignedFree(void* p)
{
  if (!p)
    return;

  char *pFull = *(char **)(((char *)p) - sizeof(char *));
  free(pFull);
}

void GetMemoryStatus(MemoryStatus* buffer)
{
  if (!buffer)
    return;

  /* sysctl hw.physmem */
  size_t len = 0;

  /* physmem */
  size_t physmem = 0;
  len = sizeof(physmem);
  if (sysctlbyname("hw.physmem", &physmem, &len, NULL, 0) == 0)
  {
    buffer->totalPhys = physmem;
    buffer->totalVirtual = physmem;
  }

  /* pagesize */
  size_t pagesize = 0;
  len = sizeof(pagesize);
  if (sysctlbyname("hw.pagesize", &pagesize, &len, NULL, 0) != 0)
    pagesize = 4096;

  /* mem_inactive */
  size_t mem_inactive = 0;
  len = sizeof(mem_inactive);
  if (sysctlbyname("vm.stats.vm.v_inactive_count", &mem_inactive, &len, NULL, 0) == 0)
    mem_inactive *= pagesize;

  /* mem_cache */
  size_t mem_cache = 0;
  len = sizeof(mem_cache);
  if (sysctlbyname("vm.stats.vm.v_cache_count", &mem_cache, &len, NULL, 0) == 0)
    mem_cache *= pagesize;

  /* mem_free */
  size_t mem_free = 0;
  len = sizeof(mem_free);
  if (sysctlbyname("vm.stats.vm.v_free_count", &mem_free, &len, NULL, 0) == 0)
    mem_free *= pagesize;

  /* mem_avail = mem_inactive + mem_cache + mem_free */
  buffer->availPhys = mem_inactive + mem_cache + mem_free;
  buffer->availVirtual = mem_inactive + mem_cache + mem_free;

  size_t swap_free = 0;
  if (sysctlbyname("vm.stats.vm.v_swappgsout", &swap_free, &len, NULL, 0) == 0)
    buffer->availPageFile = swap_free * pagesize;
}

}
}
