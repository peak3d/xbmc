#pragma once
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

#include "cores/VideoPlayer/Process/linux/VideoBufferDMABuf.h"

class CVideoBufferION : public CVideoBufferDMABuf
{
public:
  CVideoBufferION(int id) : CVideoBufferDMABuf(id) {};
  virtual ~CVideoBufferION() { Free(); };
  //CVideoBufferDMABuf overrides
  int GetWidth() override { return m_width; }
  int GetHeight() override { return m_height; }
  int GetFileDescriptor(int planeId) override { return m_fd; }
  int GetPlaneOffset(int planeId) override { return m_planeOffsets[planeId]; };
  int GetAttribute(int attributeName) override;

  //CVideoBuffer overrides
  uint8_t* GetMemPtr() override;
  void GetStrides(int(&strides)[YuvImage::MAX_PLANES]) override;
  void SetDimensions(int width, int height, const int(&strides)[YuvImage::MAX_PLANES]) override;
  void SetDimensions(int width, int height, const int(&strides)[YuvImage::MAX_PLANES], const int(&planeOffsets)[YuvImage::MAX_PLANES]) override;

  bool Allocate(int size);
  void Free();
private:
  int m_width, m_height;
  int m_fd = -1;
  uint8_t* m_ptr = nullptr;
  int m_size = 0;
  int m_planeOffsets[YuvImage::MAX_PLANES];
  int m_strides[YuvImage::MAX_PLANES];
  AVPixelFormat m_pixFormat = AV_PIX_FMT_NONE;
};

class CVideoBufferPoolION : public IVideoBufferPool
{
public:
  CVideoBufferPoolION(); : m_ionDevice(ionDevice) {};
  virtual ~CVideoBufferPoolION();

  CVideoBuffer* Get() override;
  void Return(int id) override;
  void Configure(AVPixelFormat format, int size) override;
  bool IsConfigured() override;
  bool IsCompatible(AVPixelFormat format, int size) override;
  void Released(CVideoBufferManager &videoBufferManager) override;

  int GetIONDevice() { return m_ionDevice; };
private:
  AVPixelFormat m_pixFormat = AV_PIX_FMT_NONE;
  int m_size = 0;
  bool m_configured = false;

  CCriticalSection m_criticalSection;;
  std::vector<CVideoBufferION*> m_videoBuffers;
  std::vector<int> m_freeBuffers;
  int m_ionDevice = -1;
};
