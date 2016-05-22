#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "DVDVideoCodec.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "cores/IPlayer.h"
#include "guilib/Geometry.h"
#include "rendering/RenderSystem.h"
#include "threads/Thread.h"

#include <linux/videodev2.h>
#include <sys/mman.h>
#include "utils/log.h"

typedef struct am_private_t am_private_t;

class DllLibAmCodec;

class PosixFile;
typedef std::shared_ptr<PosixFile> PosixFilePtr;

class VideoFrame;
typedef std::shared_ptr<VideoFrame> VideoFramePtr;

class CAMLCodec : public CThread
{
public:
  CAMLCodec();
  virtual ~CAMLCodec();

  bool          OpenDecoder(CDVDStreamInfo &hints);
  void          CloseDecoder();
  void          Reset();

  int           Decode(uint8_t *pData, size_t size, double dts, double pts);

  bool          GetPicture(DVDVideoPicture* pDvdVideoPicture);
  void          SetDropState(bool bDrop);

private:
  bool          OpenIonVideo(const CDVDStreamInfo &hints);
  bool          QueueFrame(VideoFramePtr frame);
  bool          DequeueFrame(VideoFramePtr &frame);
  bool          StartStreaming();
  bool          StopStreaming();
  void          CloseIonVideo();

  DllLibAmCodec   *m_dll;
  bool             m_opened;
  am_private_t    *am_private;
  CDVDStreamInfo   m_hints;
  volatile int     m_speed;
  volatile int64_t m_1st_pts;
  volatile int64_t m_cur_pts;
  volatile int64_t m_cur_pictcnt;
  volatile int64_t m_old_pictcnt;
  volatile double  m_timesize;
  volatile int64_t m_vbufsize;
  int64_t          m_start_dts;
  int64_t          m_start_pts;
  CEvent           m_ready_event;

  int              m_view_mode;
  RENDER_STEREO_MODE m_stereo_mode;
  RENDER_STEREO_VIEW m_stereo_view;

  PosixFilePtr               m_ionFile;
  PosixFilePtr               m_ionVideoFile;
  std::vector<VideoFramePtr> m_videoFrames;
  VideoFramePtr              m_lastFrame;
  bool                       m_dropState;
};
