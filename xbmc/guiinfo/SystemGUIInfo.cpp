/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "guiinfo/SystemGUIInfo.h"

#include "Application.h"
#include "LangInfo.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/BinaryAddonCache.h"
#include "dialogs/GUIDialogKeyboardGeneric.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogProgress.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "network/Network.h"
#if defined(TARGET_DARWIN_OSX)
#include "platform/darwin/osx/smc.h"
#endif
#ifdef TARGET_POSIX
#include "platform/linux/XMemUtils.h"
#endif
#include "powermanagement/PowerManager.h"
#include "profiles/ProfilesManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "storage/MediaManager.h"
#include "utils/AlarmClock.h"
#include "utils/CPUInfo.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/TimeUtils.h"
#include "windowing/WinSystem.h"

#include "guiinfo/GUIInfo.h"
#include "guiinfo/GUIInfoLabels.h"

using namespace GUIINFO;

CSystemGUIInfo::CSystemGUIInfo()
: m_lastSysHeatInfoTime(-SYSTEM_HEAT_UPDATE_INTERVAL),
  m_fanSpeed(0),
  m_fps(0.0),
  m_frameCounter(0),
  m_lastFPSTime(0)
{
}

std::string CSystemGUIInfo::GetSystemHeatInfo(int info) const
{
  if (CTimeUtils::GetFrameTime() - m_lastSysHeatInfoTime >= SYSTEM_HEAT_UPDATE_INTERVAL)
  {
    m_lastSysHeatInfoTime = CTimeUtils::GetFrameTime();
#if defined(TARGET_POSIX)
    g_cpuInfo.getTemperature(m_cpuTemp);
    m_gpuTemp = GetGPUTemperature();
#endif
  }

  std::string text;
  switch(info)
  {
    case SYSTEM_CPU_TEMPERATURE:
      return m_cpuTemp.IsValid() ? g_langInfo.GetTemperatureAsString(m_cpuTemp) : "?";
    case SYSTEM_GPU_TEMPERATURE:
      return m_gpuTemp.IsValid() ? g_langInfo.GetTemperatureAsString(m_gpuTemp) : "?";
    case SYSTEM_FAN_SPEED:
      text = StringUtils::Format("%i%%", m_fanSpeed * 2);
      break;
    case SYSTEM_CPU_USAGE:
#if defined(TARGET_DARWIN) || defined(TARGET_WINDOWS)
      text = StringUtils::Format("%d%%", g_cpuInfo.getUsedPercentage());
#else
      text = StringUtils::Format("%s", g_cpuInfo.GetCoresUsageString().c_str());
#endif
      break;
  }
  return text;
}

CTemperature CSystemGUIInfo::GetGPUTemperature() const
{
  int value = 0;
  char scale = 0;

#if defined(TARGET_DARWIN_OSX)
  value = SMCGetTemperature(SMC_KEY_GPU_TEMP);
  return CTemperature::CreateFromCelsius(value);
#elif defined(TARGET_WINDOWS_STORE)
  return CTemperature::CreateFromCelsius(0);
#else
  std::string cmd = g_advancedSettings.m_gpuTempCmd;
  int ret = 0;
  FILE* p = NULL;

  if (cmd.empty() || !(p = popen(cmd.c_str(), "r")))
    return CTemperature();

  ret = fscanf(p, "%d %c", &value, &scale);
  pclose(p);

  if (ret != 2)
    return CTemperature();
#endif

  if (scale == 'C' || scale == 'c')
    return CTemperature::CreateFromCelsius(value);
  if (scale == 'F' || scale == 'f')
    return CTemperature::CreateFromFahrenheit(value);
  return CTemperature();
}

void CSystemGUIInfo::UpdateFPS()
{
  m_frameCounter++;
  unsigned int curTime = CTimeUtils::GetFrameTime();

  float fTimeSpan = static_cast<float>(curTime - m_lastFPSTime);
  if (fTimeSpan >= 1000.0f)
  {
    fTimeSpan /= 1000.0f;
    m_fps = m_frameCounter / fTimeSpan;
    m_lastFPSTime = curTime;
    m_frameCounter = 0;
  }
}

bool CSystemGUIInfo::GetLabel(std::string& value, const CFileItem *item, const GUIInfo &info, std::string *fallback) const
{
  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // SYSTEM_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case SYSTEM_TIME:
      value = CDateTime::GetCurrentDateTime().GetAsLocalizedTime(static_cast<TIME_FORMAT>(info.GetData1()));
      return true;
    case SYSTEM_DATE:
      if (info.GetData3().empty())
        value = CDateTime::GetCurrentDateTime().GetAsLocalizedDate(true);
      else
        value = CDateTime::GetCurrentDateTime().GetAsLocalizedDate(info.GetData3());
      return true;
    case SYSTEM_FREE_SPACE:
    case SYSTEM_USED_SPACE:
    case SYSTEM_TOTAL_SPACE:
    case SYSTEM_FREE_SPACE_PERCENT:
    case SYSTEM_USED_SPACE_PERCENT:
      value = g_sysinfo.GetHddSpaceInfo(info.m_info);
      return true;
    case SYSTEM_CPU_TEMPERATURE:
    case SYSTEM_GPU_TEMPERATURE:
    case SYSTEM_FAN_SPEED:
    case SYSTEM_CPU_USAGE:
      value = GetSystemHeatInfo(info.m_info);
      return true;
    case SYSTEM_VIDEO_ENCODER_INFO:
    case NETWORK_MAC_ADDRESS:
    case SYSTEM_OS_VERSION_INFO:
    case SYSTEM_CPUFREQUENCY:
    case SYSTEM_INTERNET_STATE:
    case SYSTEM_UPTIME:
    case SYSTEM_TOTALUPTIME:
    case SYSTEM_BATTERY_LEVEL:
      value = g_sysinfo.GetInfo(info.m_info);
      return true;
    case SYSTEM_PRIVACY_POLICY:
      value = g_sysinfo.GetPrivacyPolicy();
      return true;
    case SYSTEM_SCREEN_RESOLUTION:
    {
      RESOLUTION_INFO& resInfo = CDisplaySettings::GetInstance().GetCurrentResolutionInfo();
      if (CServiceBroker::GetWinSystem()->IsFullScreen())
        value = StringUtils::Format("%ix%i@%.2fHz - %s", resInfo.iScreenWidth, resInfo.iScreenHeight, resInfo.fRefreshRate,
                                    g_localizeStrings.Get(244).c_str());
      else
        value = StringUtils::Format("%ix%i - %s", resInfo.iScreenWidth, resInfo.iScreenHeight,
                                    g_localizeStrings.Get(242).c_str());
      return true;
    }
    case SYSTEM_BUILD_VERSION_SHORT:
      value = CSysInfo::GetVersionShort();
      return true;
    case SYSTEM_BUILD_VERSION:
      value = CSysInfo::GetVersion();
      return true;
    case SYSTEM_BUILD_DATE:
      value = CSysInfo::GetBuildDate();
      return true;
    case SYSTEM_FREE_MEMORY:
    case SYSTEM_FREE_MEMORY_PERCENT:
    case SYSTEM_USED_MEMORY:
    case SYSTEM_USED_MEMORY_PERCENT:
    case SYSTEM_TOTAL_MEMORY:
    {
      MEMORYSTATUSEX stat;
      stat.dwLength = sizeof(MEMORYSTATUSEX);
      GlobalMemoryStatusEx(&stat);
      int iMemPercentFree = 100 - static_cast<int>(100.0f * (stat.ullTotalPhys - stat.ullAvailPhys) / stat.ullTotalPhys + 0.5f);
      int iMemPercentUsed = 100 - iMemPercentFree;

      if (info.m_info == SYSTEM_FREE_MEMORY)
        value = StringUtils::Format("%uMB", static_cast<unsigned int>(stat.ullAvailPhys / MB));
      else if (info.m_info == SYSTEM_FREE_MEMORY_PERCENT)
        value = StringUtils::Format("%i%%", iMemPercentFree);
      else if (info.m_info == SYSTEM_USED_MEMORY)
        value = StringUtils::Format("%uMB", static_cast<unsigned int>((stat.ullTotalPhys - stat.ullAvailPhys) / MB));
      else if (info.m_info == SYSTEM_USED_MEMORY_PERCENT)
        value = StringUtils::Format("%i%%", iMemPercentUsed);
      else if (info.m_info == SYSTEM_TOTAL_MEMORY)
        value = StringUtils::Format("%uMB", static_cast<unsigned int>(stat.ullTotalPhys / MB));
      return true;
    }
    case SYSTEM_SCREEN_MODE:
      value = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo().strMode;
      return true;
    case SYSTEM_SCREEN_WIDTH:
      value = StringUtils::Format("%i", CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo().iScreenWidth);
      return true;
    case SYSTEM_SCREEN_HEIGHT:
      value = StringUtils::Format("%i", CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo().iScreenHeight);
      return true;
    case SYSTEM_FPS:
      value = StringUtils::Format("%02.2f", m_fps);
      return true;
    case SYSTEM_CURRENT_WINDOW:
      value = g_localizeStrings.Get(CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog());
      return true;
    case SYSTEM_STARTUP_WINDOW:
      value = StringUtils::Format("%i", CServiceBroker::GetSettings().GetInt(CSettings::SETTING_LOOKANDFEEL_STARTUPWINDOW));
      return true;
    case SYSTEM_CURRENT_CONTROL:
    case SYSTEM_CURRENT_CONTROL_ID:
    {
      CGUIWindow *window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog());
      if (window)
      {
        CGUIControl *control = window->GetFocusedControl();
        if (control)
        {
          if (info.m_info == SYSTEM_CURRENT_CONTROL_ID)
            value = StringUtils::Format("%i", control->GetID());
          else if (info.m_info == SYSTEM_CURRENT_CONTROL)
            value = control->GetDescription();
          return true;
        }
      }
      break;
    }
#ifdef HAS_DVD_DRIVE
    case SYSTEM_DVD_LABEL:
      value = g_mediaManager.GetDiskLabel();
      return true;
#endif
    case SYSTEM_ALARM_POS:
      if (g_alarmClock.GetRemaining("shutdowntimer") == 0.f)
        value.clear();
      else
      {
        double fTime = g_alarmClock.GetRemaining("shutdowntimer");
        if (fTime > 60.f)
          value = StringUtils::Format(g_localizeStrings.Get(13213).c_str(), g_alarmClock.GetRemaining("shutdowntimer")/60.f);
        else
          value = StringUtils::Format(g_localizeStrings.Get(13214).c_str(), g_alarmClock.GetRemaining("shutdowntimer"));
      }
      return true;
    case SYSTEM_PROFILENAME:
      value = CServiceBroker::GetProfileManager().GetCurrentProfile().getName();
      return true;
    case SYSTEM_PROFILECOUNT:
      value = StringUtils::Format("{0}", CServiceBroker::GetProfileManager().GetNumberOfProfiles());
      return true;
    case SYSTEM_PROFILEAUTOLOGIN:
    {
      CProfilesManager& profilesMgr = CServiceBroker::GetProfileManager();
      int iProfileId = profilesMgr.GetAutoLoginProfileId();
      if ((iProfileId < 0) || !profilesMgr.GetProfileName(iProfileId, value))
        value = g_localizeStrings.Get(37014); // Last used profile
      return true;
    }
    case SYSTEM_PROFILETHUMB:
    {
      const std::string& thumb = CServiceBroker::GetProfileManager().GetCurrentProfile().getThumb();
      value = thumb.empty() ? "DefaultUser.png" : thumb;
      return true;
    }
    case SYSTEM_LANGUAGE:
      value = g_langInfo.GetEnglishLanguageName();
      return true;
    case SYSTEM_TEMPERATURE_UNITS:
      value = g_langInfo.GetTemperatureUnitString();
      return true;
    case SYSTEM_PROGRESS_BAR:
    {
      CGUIDialogProgress *bar = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);
      if (bar && bar->IsDialogRunning())
        value = StringUtils::Format("%i", bar->GetPercentage());
      return true;
    }
    case SYSTEM_FRIENDLY_NAME:
      value = CSysInfo::GetDeviceName();
      return true;
    case SYSTEM_STEREOSCOPIC_MODE:
    {
      int iStereoMode = CServiceBroker::GetSettings().GetInt(CSettings::SETTING_VIDEOSCREEN_STEREOSCOPICMODE);
      value = StringUtils::Format("%i", iStereoMode);
      return true;
    }
    case SYSTEM_GET_CORE_USAGE:
      value = StringUtils::Format("%4.2f", g_cpuInfo.GetCoreInfo(std::atoi(info.GetData3().c_str())).m_fPct);
      return true;
    case SYSTEM_RENDER_VENDOR:
      value = CServiceBroker::GetRenderSystem()->GetRenderVendor();
      return true;
    case SYSTEM_RENDER_RENDERER:
      value = CServiceBroker::GetRenderSystem()->GetRenderRenderer();
      return true;
    case SYSTEM_RENDER_VERSION:
      value = CServiceBroker::GetRenderSystem()->GetRenderVersionString();
      return true;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // NETWORK_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case NETWORK_IP_ADDRESS:
    {
      CNetworkInterface* iface = CServiceBroker::GetNetwork().GetFirstConnectedInterface();
      if (iface)
      {
        value = iface->GetCurrentIPAddress();
        return true;
      }
      break;
    }
    case NETWORK_SUBNET_MASK:
    {
      CNetworkInterface* iface = CServiceBroker::GetNetwork().GetFirstConnectedInterface();
      if (iface)
      {
        value = iface->GetCurrentNetmask();
        return true;
      }
      break;
    }
    case NETWORK_GATEWAY_ADDRESS:
    {
      CNetworkInterface* iface = CServiceBroker::GetNetwork().GetFirstConnectedInterface();
      if (iface)
      {
        value = iface->GetCurrentDefaultGateway();
        return true;
      }
      break;
    }
    case NETWORK_DNS1_ADDRESS:
    {
      const std::vector<std::string> nss = CServiceBroker::GetNetwork().GetNameServers();
      if (nss.size() >= 1)
      {
        value = nss[0];
        return true;
      }
      break;
    }
    case NETWORK_DNS2_ADDRESS:
    {
      const std::vector<std::string> nss = CServiceBroker::GetNetwork().GetNameServers();
      if (nss.size() >= 2)
      {
        value = nss[1];
        return true;
      }
      break;
    }
    case NETWORK_DHCP_ADDRESS:
    {
      // wtf?
      std::string dhcpserver;
      value = dhcpserver;
      return true;
    }
    case NETWORK_LINK_STATE:
    {
      std::string linkStatus = g_localizeStrings.Get(151);
      linkStatus += " ";
      CNetworkInterface* iface = CServiceBroker::GetNetwork().GetFirstConnectedInterface();
      if (iface && iface->IsConnected())
        linkStatus += g_localizeStrings.Get(15207);
      else
        linkStatus += g_localizeStrings.Get(15208);
      value = linkStatus;
      return true;
    }
  }

  return false;
}

bool CSystemGUIInfo::GetInt(int& value, const CGUIListItem *gitem, const GUIInfo &info) const
{
  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // SYSTEM_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case SYSTEM_FREE_MEMORY:
    case SYSTEM_USED_MEMORY:
    {
      MEMORYSTATUSEX stat;
      stat.dwLength = sizeof(MEMORYSTATUSEX);
      GlobalMemoryStatusEx(&stat);
      int memPercentUsed = static_cast<int>(100.0f * (stat.ullTotalPhys - stat.ullAvailPhys) / stat.ullTotalPhys + 0.5f);
      if (info.m_info == SYSTEM_FREE_MEMORY)
        value = 100 - memPercentUsed;
      else
        value = memPercentUsed;
      return true;
    }
    case SYSTEM_FREE_SPACE:
    case SYSTEM_USED_SPACE:
    {
      g_sysinfo.GetHddSpaceInfo(value, info.m_info, true);
      return true;
    }
    case SYSTEM_CPU_USAGE:
      value = g_cpuInfo.getUsedPercentage();
      return true;
    case SYSTEM_BATTERY_LEVEL:
      value = CServiceBroker::GetPowerManager().BatteryLevel();
      return true;
    case SYSTEM_PROGRESS_BAR:
    {
      CGUIDialogProgress *bar = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);
      if (bar && bar->IsDialogRunning())
        value = bar->GetPercentage();
      return true;
    }
  }

  return false;
}

bool CSystemGUIInfo::GetBool(bool& value, const CGUIListItem *gitem, const GUIInfo &info) const
{
  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // SYSTEM_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case SYSTEM_ALWAYS_TRUE:
      value = true;
      return true;
    case SYSTEM_ALWAYS_FALSE:
      value = false;
      return true;
    case SYSTEM_ETHERNET_LINK_ACTIVE:
      // wtf: not implementated - always returns true?!
      value = true;
      return true;
    case SYSTEM_PLATFORM_LINUX:
#if defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
      value = true;
#else
      value = false;
#endif
      return true;
    case SYSTEM_PLATFORM_WINDOWS:
#ifdef TARGET_WINDOWS
      value = true;
#else
      value = false;
#endif
      return true;
    case SYSTEM_PLATFORM_DARWIN:
#ifdef TARGET_DARWIN
      value = true;
#else
      value = false;
#endif
      return true;
    case SYSTEM_PLATFORM_DARWIN_OSX:
#ifdef TARGET_DARWIN_OSX
      value = true;
#else
      value = false;
#endif
      return true;
    case SYSTEM_PLATFORM_DARWIN_IOS:
#ifdef TARGET_DARWIN_IOS
      value = true;
#else
      value = false;
#endif
      return true;
    case SYSTEM_PLATFORM_ANDROID:
#if defined(TARGET_ANDROID)
      value = true;
#else
      value = false;
#endif
      return true;
    case SYSTEM_PLATFORM_LINUX_RASPBERRY_PI:
#if defined(TARGET_RASPBERRY_PI)
      value = true;
#else
      value = false;
#endif
      return true;
    case SYSTEM_MEDIA_DVD:
      value = g_mediaManager.IsDiscInDrive();
      return true;
#ifdef HAS_DVD_DRIVE
    case SYSTEM_DVDREADY:
      value = g_mediaManager.GetDriveStatus() != DRIVE_NOT_READY;
      return true;
    case SYSTEM_TRAYOPEN:
      value = g_mediaManager.GetDriveStatus() == DRIVE_OPEN;
      return true;
#endif
    case SYSTEM_CAN_POWERDOWN:
      value = CServiceBroker::GetPowerManager().CanPowerdown();
      return true;
    case SYSTEM_CAN_SUSPEND:
      value = CServiceBroker::GetPowerManager().CanSuspend();
      return true;
    case SYSTEM_CAN_HIBERNATE:
      value = CServiceBroker::GetPowerManager().CanHibernate();
      return true;
    case SYSTEM_CAN_REBOOT:
      value = CServiceBroker::GetPowerManager().CanReboot();
      return true;
    case SYSTEM_SCREENSAVER_ACTIVE:
      value = g_application.IsInScreenSaver();
      return true;
    case SYSTEM_DPMS_ACTIVE:
      value = g_application.IsDPMSActive();
      return true;
    case SYSTEM_HASLOCKS:
      value = CServiceBroker::GetProfileManager().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE;
      return true;
    case SYSTEM_HAS_PVR:
      value = true;
      return true;
    case SYSTEM_HAS_PVR_ADDON:
    {
      ADDON::VECADDONS pvrAddons;
      ADDON::CBinaryAddonCache &addonCache = CServiceBroker::GetBinaryAddonCache();
      addonCache.GetAddons(pvrAddons, ADDON::ADDON_PVRDLL);
      value = (pvrAddons.size() > 0);
      return true;
    }
    case SYSTEM_HAS_CMS:
#if defined(HAS_GL) || defined(HAS_DX)
      value = true;
#else
      value = false;
#endif
      return true;
    case SYSTEM_ISMASTER:
      value = CServiceBroker::GetProfileManager().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && g_passwordManager.bMasterUser;
      return true;
    case SYSTEM_ISFULLSCREEN:
      value = CServiceBroker::GetWinSystem()->IsFullScreen();
      return true;
    case SYSTEM_ISSTANDALONE:
      value = g_application.IsStandAlone();
      return true;
    case SYSTEM_ISINHIBIT:
      value = g_application.IsIdleShutdownInhibited();
      return true;
    case SYSTEM_HAS_SHUTDOWN:
      value = (CServiceBroker::GetSettings().GetInt(CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNTIME) > 0);
      return true;
    case SYSTEM_LOGGEDON:
      value = !(CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_LOGIN_SCREEN);
      return true;
    case SYSTEM_SHOW_EXIT_BUTTON:
      value = g_advancedSettings.m_showExitButton;
      return true;
    case SYSTEM_HAS_LOGINSCREEN:
      value = CServiceBroker::GetProfileManager().UsingLoginScreen();
      return true;
    case SYSTEM_HAS_ACTIVE_MODAL_DIALOG:
      value = CServiceBroker::GetGUI()->GetWindowManager().HasModalDialog();
      return true;
    case SYSTEM_HAS_VISIBLE_MODAL_DIALOG:
      value = CServiceBroker::GetGUI()->GetWindowManager().HasVisibleModalDialog();
      return true;
    case SYSTEM_INTERNET_STATE:
    {
      g_sysinfo.GetInfo(info.m_info);
      value = g_sysinfo.HasInternet();
      return true;
    }
    case SYSTEM_HAS_INPUT_HIDDEN:
    {
      CGUIDialogNumeric *pNumeric = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogNumeric>(WINDOW_DIALOG_NUMERIC);
      CGUIDialogKeyboardGeneric *pKeyboard = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogKeyboardGeneric>(WINDOW_DIALOG_KEYBOARD);

      if (pNumeric && pNumeric->IsActive())
        value = pNumeric->IsInputHidden();
      else if (pKeyboard && pKeyboard->IsActive())
        value = pKeyboard->IsInputHidden();
      return true;
    }
    case SYSTEM_IDLE_TIME:
      value = g_application.GlobalIdleTime() >= static_cast<int>(info.GetData1());
      return true;
    case SYSTEM_HAS_CORE_ID:
      value = g_cpuInfo.HasCoreId(info.GetData1());
      return true;
    case SYSTEM_DATE:
    {
      if (info.GetData2() == -1) // info doesn't contain valid startDate
        return false;
      const CDateTime date = CDateTime::GetCurrentDateTime();
      int currentDate = date.GetMonth() * 100 + date.GetDay();
      int startDate = info.GetData1();
      int stopDate = info.GetData2();

      if (stopDate < startDate)
        value = currentDate >= startDate || currentDate < stopDate;
      else
        value = currentDate >= startDate && currentDate < stopDate;
      return true;
    }
    case SYSTEM_TIME:
    {
      int currentTime = CDateTime::GetCurrentDateTime().GetMinuteOfDay();
      int startTime = info.GetData1();
      int stopTime = info.GetData2();

      if (stopTime < startTime)
        value = currentTime >= startTime || currentTime < stopTime;
      else
        value = currentTime >= startTime && currentTime < stopTime;
      return true;
    }
    case SYSTEM_ALARM_LESS_OR_EQUAL:
    {
      int time = std::lrint(g_alarmClock.GetRemaining(info.GetData3()));
      int timeCompare = info.GetData2();
      if (time > 0)
        value = timeCompare >= time;
      else
        value = false;
      return true;
    }
    case SYSTEM_HAS_ALARM:
      value = g_alarmClock.HasAlarm(info.GetData3());
      return true;
    case SYSTEM_GET_BOOL:
      value = CServiceBroker::GetSettings().GetBool(info.GetData3());
      return true;
    case SYSTEM_HAS_ADDON:
    {
      ADDON::AddonPtr addon;
      value = CServiceBroker::GetAddonMgr().GetAddon(info.GetData3(), addon) && addon;
      return true;
    }
  }

  return false;
}
