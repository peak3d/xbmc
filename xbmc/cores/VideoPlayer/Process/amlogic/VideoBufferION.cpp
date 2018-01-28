/*
*      Copyright (C) 2017 Team XBMC
*      http://kodi.tv
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#include "VideoBufferION.h"
#include <cstring>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/ioctl.h>

#include "utils/log.h"
#include "threads/Thread.h"

typedef int ion_handle;

struct ion_handle_data
{
  ion_handle handle;
};

struct ion_fd_data
{
  ion_handle handle;
  int fd;
};

struct ion_allocation_data
{
  size_t len;
  size_t align;
  unsigned int heap_id_mask;
  unsigned int flags;
  ion_handle handle;
};

#define ION_IOC_MAGIC 'I'

#define ION_IOC_ALLOC _IOWR(ION_IOC_MAGIC, 0, struct ion_allocation_data)
#define ION_IOC_FREE  _IOWR(ION_IOC_MAGIC, 1, struct ion_handle_data)
#define ION_IOC_SHARE _IOWR(ION_IOC_MAGIC, 4, struct ion_fd_data)

enum ion_heap_type
{
  ION_HEAP_TYPE_SYSTEM,
  ION_HEAP_TYPE_SYSTEM_CONTIG,
  ION_HEAP_TYPE_CARVEOUT,
  ION_HEAP_TYPE_CHUNK,
  ION_HEAP_TYPE_CUSTOM,
  ION_NUM_HEAPS = 16
};

static const int ION_HEAP_SYSTEM_MASK        = (1 << ION_HEAP_TYPE_SYSTEM);
static const int ION_HEAP_SYSTEM_CONTIG_MASK = (1 << ION_HEAP_TYPE_SYSTEM_CONTIG);
static const int ION_HEAP_CARVEOUT_MASK      = (1 << ION_HEAP_TYPE_CARVEOUT);

int CVideoBufferION::GetAttribute(int attributeName)
{
}

uint8_t* CVideoBufferION::GetMemPtr()
{
  if (m_fd < 0 || m_size <=0)
    return nullptr;
  if (!m_ptr)
    m_ptr = static_cast<uint8_t*>(mmap(NULL, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0));
  return m_ptr;
}

void CVideoBufferION::GetStrides(int(&strides)[YuvImage::MAX_PLANES])
{
  std::memcpy(strides, m_strides, sizeof(strides));
}

void CVideoBufferION::SetDimensions(int width, int height, const int(&strides)[YuvImage::MAX_PLANES])
{
  m_width = width;
  m_height = height;
  memcpy(m_strides, strides, sizeof(m_strides));
}

void CVideoBufferION::SetDimensions(int width, int height, const int(&strides)[YuvImage::MAX_PLANES], const int(&planeOffsets)[YuvImage::MAX_PLANES])
{
  SetDimensions(width, height, strides);
  memcpy(m_planeOffsets, planeOffsets, sizeof(m_planeOffsets));
}

bool CVideoBufferION::Allocate(int size)
{
  if (m_fd != -1)
  {
    if (m_size != size)
      Free();
    else
      return true;
  }

  ion_allocation_data data =
  {
    .len = static_cast<size_t>(size),
    .align = 0,
    .heap_id_mask = ION_HEAP_CARVEOUT_MASK,
    .flags = 0
  };

  if (ioctl(dynamic_cast<CVideoBufferPoolION*>(m_pool.get())->GetIONDevice(), ION_IOC_ALLOC, &data) < 0)
  {
    CLog::Log(LOGERROR, "CVideoBufferION::Allocate: ION_IOC_ALLOC failed (len = %ld): %s\n", data.len, strerror(errno));
    return false;
  }

  ion_fd_data dma_data =
  {
    .handle = data.handle,
    .fd = -1
  };

  if (ioctl(dynamic_cast<CVideoBufferPoolION*>(m_pool.get())->GetIONDevice(), ION_IOC_SHARE, &dma_data) < 0)
  {
    CLog::Log(LOGERROR, "CVideoBufferION::Allocate: ION_IOC_SHARE failed: %s\n", strerror(errno));
    return false;
  }
  m_fd = dma_data.fd;
  return true;
}

void CVideoBufferION::Free()
{
  if (m_fd < 0)
    return;
  if (m_ptr)
    munmap(m_ptr, m_size);

  ion_handle_data data =
  {
    .handle = m_fd
  };
  if (ioctl(dynamic_cast<CVideoBufferPoolION*>(m_pool.get())->GetIONDevice(), ION_IOC_FREE, &data) < 0)
  {
    CLog::Log(LOGERROR, "CVideoBufferION::Free: ION_IOC_FREE failed: %s\n", strerror(errno));
  }
  m_fd = -1;
  m_ptr = nullptr;
}

/***************************************************************************/

CVideoBufferPoolION::CVideoBufferPoolION()
{
  if ((m_ionDevice = open("/dev/ion", O_RDWR)) < 0)
  {
    CLog::Log(LOGERROR, "CVideoBufferION::Free: ION_IOC_FREE failed: %s\n", strerror(errno));
  }
};

CVideoBufferPoolION::~CVideoBufferPoolION()
{
  CLog::Log(LOGERROR, "~CVideoBufferPoolION");
  close(m_ionDevice);
  m_ionDevice = -1;
  for (auto buffer : m_videoBuffers)
    delete buffer;
}

CVideoBuffer* CVideoBufferPoolION::Get()
{
  CSingleLock lock(m_criticalSection);

  if (m_freeBuffers.empty())
  {
    m_freeBuffers.push_back(m_videoBuffers.size());
    m_videoBuffers.push_back(new CVideoBufferION(static_cast<int>(m_videoBuffers.size())));
  }
  int bufferIdx(m_freeBuffers.back());
  m_freeBuffers.pop_back();

  m_videoBuffers[bufferIdx]->Allocate(m_size);
  m_videoBuffers[bufferIdx]->Acquire(shared_from_this());

  return m_videoBuffers[bufferIdx];
}

void CVideoBufferPoolION::Return(int id)
{
  CSingleLock lock(m_criticalSection);
  m_videoBuffers[id]->Free();
  m_freeBuffers.push_back(id);
}

void CVideoBufferPoolION::Configure(AVPixelFormat format, int size)
{
  m_pixFormat = format;
  m_size = size;
  m_configured = true;
}

inline bool CVideoBufferPoolION::IsConfigured()
{
  return m_configured;
}

bool CVideoBufferPoolION::IsCompatible(AVPixelFormat format, int size)
{
  if (m_pixFormat == format && m_size == size)
    return true;

  return false;
}

void CVideoBufferPoolION::Released(CVideoBufferManager &videoBufferManager)
{
  CLog::Log(LOGERROR, "CVideoBufferPoolION::Released");
  for (auto buffer : m_videoBuffers)
    delete buffer;

  m_videoBuffers.clear();
  m_freeBuffers.clear();
}
