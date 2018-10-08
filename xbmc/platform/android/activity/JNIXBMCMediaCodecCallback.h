/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <androidjni/JNIBase.h>

class CJNIMediaCodec;
class CJNIMediaCodecBufferInfo;
class CJNIMediaFormat;

namespace jni
{

class CJNIXBMCMediaCodecCallback : public CJNIBase, public CJNIInterfaceImplem<CJNIXBMCMediaCodecCallback>
{
public:
  CJNIXBMCMediaCodecCallback();
  virtual ~CJNIXBMCMediaCodecCallback();

  static void RegisterNatives(JNIEnv* env);

  // CJNINsdManagerDiscoveryListener interface
public:
  virtual void onError(const CJNIMediaCodec &codec) = 0;
  virtual void onInputBufferAvailable(const CJNIMediaCodec &codec, int index) = 0;
  virtual void onOutputBufferAvailable(const CJNIMediaCodec &codec, int index, const CJNIMediaCodecBufferInfo &info) = 0;
  virtual void onOutputFormatChanged(const CJNIMediaCodec &codec, const CJNIMediaFormat &format) = 0;

protected:
  static void _onError(JNIEnv* env, jobject thiz, jobject mediaCodec,  jobject exception);
  static void _onInputBufferAvailable(JNIEnv* env, jobject thiz, jobject mediaCodec, jint index);
  static void _onOutputBufferAvailable(JNIEnv* env, jobject thiz, jobject mediaCodec, jint index, jobject bufferInfo);
  static void _onOutputFormatChanged(JNIEnv* env, jobject thiz, jobject mediaCodec, jobject mediaFormat);
};

}

