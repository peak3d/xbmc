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

#include <mach/mach.h>
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

  uint64_t physmem;
  size_t len = sizeof physmem;

#if defined(__apple_build_version__) && __apple_build_version__ < 10000000
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif
  std::array<int, 2> mib =
  {
    CTL_HW,
    HW_MEMSIZE,
  };
#if defined(__apple_build_version__) && __apple_build_version__ < 10000000
#pragma clang diagnostic pop
#endif

  // Total physical memory.
  if (sysctl(mib.data(), mib.size(), &physmem, &len, NULL, 0) == 0 && len == sizeof (physmem))
    buffer->totalPhys = physmem;

  // Virtual memory.
  mib[0] = CTL_VM;
  mib[1] = VM_SWAPUSAGE;
  struct xsw_usage swap;
  len = sizeof(struct xsw_usage);
  if (sysctl(mib.data(), mib.size(), &swap, &len, NULL, 0) == 0)
  {
      buffer->availPageFile = swap.xsu_avail;
      buffer->totalVirtual = buffer->totalPhys + swap.xsu_total;
  }

  // In use.
  mach_port_t stat_port = mach_host_self();
  vm_statistics_data_t vm_stat;
  mach_msg_type_number_t count = sizeof(vm_stat) / sizeof(natural_t);
  if (host_statistics(stat_port, HOST_VM_INFO, (host_info_t)&vm_stat, &count) == 0)
  {
      // Find page size.
#if defined(TARGET_DARWIN_IOS)
      // on ios with 64bit ARM CPU the page size is wrongly given as 16K
      // when using the sysctl approach. We can use the host_page_size
      // function instead which will give the proper 4k pagesize
      // on both 32 and 64 bit ARM CPUs
      vm_size_t pageSize;
      host_page_size(stat_port, &pageSize);
#else
      int pageSize;
      mib[0] = CTL_HW;
      mib[1] = HW_PAGESIZE;
      len = sizeof(int);
      if (sysctl(mib.data(), mib.size(), &pageSize, &len, NULL, 0) == 0)
#endif
      {
          uint64_t used = (vm_stat.active_count + vm_stat.inactive_count + vm_stat.wire_count) * pageSize;

          buffer->availPhys = buffer->totalPhys - used;
          buffer->availVirtual  = buffer->availPhys; // FIXME.
      }
  }
}

}
}
