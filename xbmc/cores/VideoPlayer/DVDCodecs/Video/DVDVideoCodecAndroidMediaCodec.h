/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <deque>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>

#include <androidjni/Surface.h>

#include "DVDVideoCodec.h"
#include "DVDStreamInfo.h"
#include "platform/android/activity/JNIXBMCVideoView.h"
#include "platform/android/activity/JNIXBMCMediaCodecCallback.h"
#include "threads/Thread.h"
#include "threads/SingleLock.h"
#include "utils/Geometry.h"
#include "cores/VideoPlayer/Process/VideoBuffer.h"

#include <android/native_window.h>
#include <android/native_window_jni.h>

class CJNISurface;
class CJNISurfaceTexture;
class CJNIMediaCodec;
class CJNIMediaCrypto;
class CJNIMediaFormat;
class CDVDMediaCodecOnFrameAvailable;
class CJNIByteBuffer;
class CBitstreamConverter;
class CJNIMediaCodecBufferInfo;

struct DemuxCryptoInfo;
struct mpeg2_sequence;


typedef struct amc_demux {
  uint8_t  *pData;
  int       iSize;
  double    dts;
  double    pts;
} amc_demux;

class CMediaCodecVideoBufferPool;

class CMediaCodecVideoBuffer : public CVideoBuffer
{
public:
  CMediaCodecVideoBuffer(int id) : CVideoBuffer(id) {};
  virtual ~CMediaCodecVideoBuffer() {};

  void Set(int internalId, int textureId, int64_t pts,
   std::shared_ptr<CJNISurfaceTexture> surfaceTexture,
   std::shared_ptr<CDVDMediaCodecOnFrameAvailable> frameAvailable,
   std::shared_ptr<CJNIXBMCVideoView> videoView);

  // meat and potatoes
  bool                WaitForFrame(int millis);
  // MediaCodec related
  void                ReleaseOutputBuffer(bool render, int64_t displayTime, CMediaCodecVideoBufferPool* pool = nullptr);
  // SurfaceTexture released
  int                 GetBufferId() const;
  int                 GetTextureId() const;
  int64_t             GetPts() const { return m_pts; };
  void                GetTransformMatrix(float *textureMatrix);
  void                UpdateTexImage();
  void                RenderUpdate(const CRect &DestRect, int64_t displayTime);
  bool                HasSurfaceTexture() const { return m_surfacetexture.operator bool(); };

private:
  int                 m_bufferId = -1;
  unsigned int        m_textureId = 0;
  int64_t             m_pts;
  // shared_ptr bits, shared between
  // CDVDVideoCodecAndroidMediaCodec and LinuxRenderGLES.
  std::shared_ptr<CJNISurfaceTexture> m_surfacetexture;
  std::shared_ptr<CDVDMediaCodecOnFrameAvailable> m_frameready;
  std::shared_ptr<CJNIXBMCVideoView> m_videoview;
};

class CMediaCodecVideoBufferPool : public IVideoBufferPool
{
public:
  CMediaCodecVideoBufferPool(std::shared_ptr<CJNIMediaCodec> mediaCodec) : m_codec(mediaCodec) {};

  virtual ~CMediaCodecVideoBufferPool();

  virtual CVideoBuffer* Get() override;
  virtual void Return(int id) override;

  std::shared_ptr<CJNIMediaCodec> GetMediaCodec();
  void ResetMediaCodec();

private:
  CCriticalSection m_criticalSection;;
  std::shared_ptr<CJNIMediaCodec> m_codec;

  std::vector<CMediaCodecVideoBuffer*> m_videoBuffers;
  std::vector<int> m_freeBuffers;
};

class CDVDVideoCodecAndroidMediaCodec : public CDVDVideoCodec, public CJNISurfaceHolderCallback, public jni::CJNIXBMCMediaCodecCallback
{
public:
  CDVDVideoCodecAndroidMediaCodec(CProcessInfo &processInfo, bool surface_render = false);
  virtual ~CDVDVideoCodecAndroidMediaCodec();

  // registration
  static CDVDVideoCodec* Create(CProcessInfo &processInfo);
  static bool Register();

  // required overrides
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) override;
  virtual bool AddData(const DemuxPacket &packet) override;
  virtual void Reset() override;
  virtual bool Reconfigure(CDVDStreamInfo &hints) override;
  virtual VCReturn GetPicture(VideoPicture* pVideoPicture) override;
  virtual const char* GetName() override { return m_formatname.c_str(); };
  virtual void SetCodecControl(int flags) override;
  virtual unsigned GetAllowedReferences() override;

  void AddInputBuffer(int index);
  void AddOutputBuffer(CMediaCodecVideoBuffer *buffer);

  CMediaCodecVideoBuffer *AllocateVideoBuffer(int index, const CJNIMediaCodecBufferInfo &bufferInfo);
  void ConfigureOutputFormat(const CJNIMediaFormat &mediaformat);

protected:
  void            Dispose();
  void            FlushInternal(void);
  bool            ConfigureMediaCodec(void);
  void            UpdateFpsDuration();

  // surface handling functions
  static void     CallbackInitSurfaceTexture(void*);
  void            InitSurfaceTexture(void);
  void            ReleaseSurfaceTexture(void);

  // MediaCodecCallback
  void onError(const CJNIMediaCodec &codec) override;
  void onInputBufferAvailable(const CJNIMediaCodec &codec, int index) override;
  void onOutputBufferAvailable(const CJNIMediaCodec &codec, int index, const CJNIMediaCodecBufferInfo &info) override;
  void onOutputFormatChanged(const CJNIMediaCodec &codec, const CJNIMediaFormat &format) override;

  CDVDStreamInfo  m_hints;
  std::string     m_mime;
  std::string     m_codecname;
  int             m_colorFormat;
  std::string     m_formatname;
  bool            m_opened;
  int             m_codecControlFlags;
  int             m_state;
  int             m_noPictureLoop;

  std::shared_ptr<CJNIXBMCVideoView> m_jnivideoview;
  CJNISurface*    m_jnisurface;
  CJNISurface     m_jnivideosurface;
  unsigned int    m_textureId;
  std::shared_ptr<CJNIMediaCodec> m_codec;
  CJNIMediaCrypto *m_crypto = nullptr;
  std::shared_ptr<CJNISurfaceTexture> m_surfaceTexture;
  std::shared_ptr<CDVDMediaCodecOnFrameAvailable> m_frameAvailable;

  amc_demux m_demux_pkt;
  std::shared_ptr<CMediaCodecVideoBufferPool> m_videoBufferPool;

  uint32_t m_OutputDuration, m_fpsDuration;
  int64_t m_lastPTS;

  static std::atomic<bool> m_InstanceGuard;

  CBitstreamConverter *m_bitstream;
  VideoPicture m_videobuffer;

  std::mutex m_bufferLock;
  std::deque<int> m_inputBuffers;
  std::deque<CMediaCodecVideoBuffer*> m_outputBuffers;

  bool            m_render_surface;
  mpeg2_sequence  *m_mpeg2_sequence;
  int             m_src_offset[4];
  int             m_src_stride[4];

  // CJNISurfaceHolderCallback interface
public:
  virtual void surfaceChanged(CJNISurfaceHolder holder, int format, int width, int height) override;
  virtual void surfaceCreated(CJNISurfaceHolder holder) override;
  virtual void surfaceDestroyed(CJNISurfaceHolder holder) override;
};
