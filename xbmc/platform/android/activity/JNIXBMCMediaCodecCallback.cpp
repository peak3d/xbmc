/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JNIXBMCMediaCodecCallback.h"

#include <androidjni/jutils-details.hpp>
#include <androidjni/Context.h>

#include <androidjni/MediaCodec.h>
#include <androidjni/MediaFormat.h>
#include <androidjni/MediaCodecBufferInfo.h>

#include "CompileInfo.h"

using namespace jni;

static std::string s_className = std::string(CCompileInfo::GetClass()) + "/interfaces/XBMCMediaCodecCallback";

CJNIXBMCMediaCodecCallback::CJNIXBMCMediaCodecCallback()
  : CJNIBase(s_className)
{
  m_object = new_object(CJNIContext::getClassLoader().loadClass(GetDotClassName(s_className)));
  m_object.setGlobal();

  add_instance(m_object, this);
}

CJNIXBMCMediaCodecCallback::~CJNIXBMCMediaCodecCallback()
{
  remove_instance(this);
}

void CJNIXBMCMediaCodecCallback::RegisterNatives(JNIEnv* env)
{
  jclass cClass = env->FindClass(s_className.c_str());
  if(cClass)
  {
    JNINativeMethod methods[] =
    {
      {"_onError", "(Landroid/media/MediaCodec;Landroid/media/MediaCodec$CodecException;)V", reinterpret_cast<void*>(&CJNIXBMCMediaCodecCallback::_onError)},
      {"_onInputBufferAvailable", "(Landroid/media/MediaCodec;I)V", reinterpret_cast<void*>(&CJNIXBMCMediaCodecCallback::_onInputBufferAvailable)},
      {"_onOutputBufferAvailable", "(Landroid/media/MediaCodec;ILandroid/media/MediaCodec$BufferInfo;)V", reinterpret_cast<void*>(&CJNIXBMCMediaCodecCallback::_onOutputBufferAvailable)},
      {"_onOutputFormatChanged", "(Landroid/media/MediaCodec;Landroid/media/MediaFormat;)V", reinterpret_cast<void*>(&CJNIXBMCMediaCodecCallback::_onOutputFormatChanged)},
    };

    env->RegisterNatives(cClass, methods, sizeof(methods)/sizeof(methods[0]));
  }
}


void CJNIXBMCMediaCodecCallback::_onError(JNIEnv* env, jobject thiz, jobject mediaCodec,  jobject exception)
{
  (void)env;

  CJNIXBMCMediaCodecCallback *inst = find_instance(thiz);
  if (inst)
    inst->onError(jhobject::fromJNI(mediaCodec));
}

void CJNIXBMCMediaCodecCallback::_onInputBufferAvailable(JNIEnv* env, jobject thiz, jobject mediaCodec, jint index)
{
  (void)env;

  CJNIXBMCMediaCodecCallback *inst = find_instance(thiz);
  if (inst)
    inst->onInputBufferAvailable(jhobject::fromJNI(mediaCodec), index);
}

void CJNIXBMCMediaCodecCallback::_onOutputBufferAvailable(JNIEnv* env, jobject thiz, jobject mediaCodec, jint index, jobject bufferInfo)
{
  (void)env;

  CJNIXBMCMediaCodecCallback *inst = find_instance(thiz);
  if (inst)
    inst->onOutputBufferAvailable(jhobject::fromJNI(mediaCodec), index, jhobject::fromJNI(bufferInfo));
}

void CJNIXBMCMediaCodecCallback::_onOutputFormatChanged(JNIEnv* env, jobject thiz, jobject mediaCodec, jobject mediaFormat)
{
  (void)env;

  CJNIXBMCMediaCodecCallback *inst = find_instance(thiz);
  if (inst)
    inst->onOutputFormatChanged(jhobject::fromJNI(mediaCodec), jhobject::fromJNI(mediaFormat));
}
