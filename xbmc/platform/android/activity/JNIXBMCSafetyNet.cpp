/*
 *  Copyright (C) 2018 Christian Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JNIXBMCSafetyNet.h"

#include "CompileInfo.h"

#include <androidjni/Context.h>
#include <androidjni/jutils-details.hpp>

#include "platform/android/activity/XBMCApp.h"

using namespace jni;

static std::string s_className = std::string(CCompileInfo::GetClass()) + "/XBMCSafetyNet";

CJNIXBMCSafetyNet::CJNIXBMCSafetyNet()
  : CJNIBase(s_className)
{
  m_object = new_object(CJNIContext::getClassLoader().loadClass(GetDotClassName(s_className)));
  m_object.setGlobal();

  add_instance(m_object, this);
}

CJNIXBMCSafetyNet::~CJNIXBMCSafetyNet()
{
  remove_instance(this);
}

void CJNIXBMCSafetyNet::RegisterNatives(JNIEnv* env)
{
  jclass cClass = env->FindClass(s_className.c_str());
  if(cClass)
  {
    JNINativeMethod methods[] =
    {
      {"_onAttestResponse", "(ILjava/lang/String;)V", (void*)&CJNIXBMCSafetyNet::_onAttestResponse}
    };

    env->RegisterNatives(cClass, methods, sizeof(methods)/sizeof(methods[0]));
  }
}

void CJNIXBMCSafetyNet::Attest(const std::vector<char>& nonce, const std::string& apiKey) const
{
  call_method<void>(m_object,
                    "Attest", "(Landroid/content/Context;[BLjava/lang/String;)V",
                    CXBMCApp::get()->getContext(), jcast<jhbyteArray>(nonce), jcast<jhstring>(apiKey));
}

void CJNIXBMCSafetyNet::_onAttestResponse(JNIEnv *env, jobject thiz, int status, jstring response)
{
  (void)env;

  CJNIXBMCSafetyNet *inst = find_instance(thiz);
  if (inst)
    inst->OnAttestResponse(status, jcast<std::string>(jhstring(response)));
}
