/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <androidjni/JNIBase.h>

namespace jni
{
  class CJNIXBMCSafetyNet : public CJNIBase, public CJNIInterfaceImplem<CJNIXBMCSafetyNet>
  {
  public:
    CJNIXBMCSafetyNet();
    static void RegisterNatives(JNIEnv* env);

    void Attest(const std::vector<char>& nonce, const std::string& apiKey) const;

  protected:
    virtual ~CJNIXBMCSafetyNet();

    virtual void OnAttestResponse(int status, const std::string& response) = 0;
    static void _onAttestResponse(JNIEnv* env, jobject thiz, int result, jstring response);
  };
}
