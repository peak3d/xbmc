/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "WinSystemAndroid.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "utils/EGLUtils.h"
#include "utils/GlobalsHandling.h"

struct AVMasteringDisplayMetadata;

class CWinSystemAndroidGLESContext : public CWinSystemAndroid, public CRenderSystemGLES
{
public:
  CWinSystemAndroidGLESContext() = default;
  virtual ~CWinSystemAndroidGLESContext() = default;

  // Implementation of CWinSystemBase via CWinSystemAndroid
  CRenderSystemBase *GetRenderSystem() override { return this; }
  bool InitWindowSystem() override;
  bool CreateNewWindow(const std::string& name,
                       bool fullScreen,
                       RESOLUTION_INFO& res) override;

  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;

  virtual std::unique_ptr<CVideoSync> GetVideoSync(void *clock) override;

  float GetFrameLatencyAdjustment() override;
  bool SetHDR(const VideoPicture* videoPicture) override;

  EGLDisplay GetEGLDisplay() const;
  EGLSurface GetEGLSurface() const;
  EGLContext GetEGLContext() const;
  EGLConfig  GetEGLConfig() const;
protected:
  void SetVSyncImpl(bool enable) override;
  void PresentRenderImpl(bool rendered) override;

private:
  bool CreateSurface();

  CEGLContextUtils m_pGLContext;
  bool m_hasHDRConfig = false;

  std::unique_ptr<AVMasteringDisplayMetadata> m_displayMetadata;
  EGLint m_HDRColorSpace = EGL_NONE;
  bool m_hasEGLHDRExtensions = false;
};
