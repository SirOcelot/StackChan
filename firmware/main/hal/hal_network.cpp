/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "hal.h"
#include <stackchan/stackchan.h>
#include <mooncake.h>
#include <mooncake_log.h>
#include <wifi_manager.h>
#include <board.h>
#include <mutex>
#include <queue>
#include <vector>
#include <ctime>
#include <sys/time.h>
#include <esp_sntp.h>
#include <atomic>
#include <sdkconfig.h>
#include <esp_random.h>

static std::string _tag           = "Network";
static bool _is_network_connected = false;

#if CONFIG_GOOSEOPS_LOCAL_ONLY
static std::string generate_local_ap_password()
{
    static constexpr char alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789";
    std::string password;
    password.reserve(14);
    for (int i = 0; i < 14; ++i) {
        password.push_back(alphabet[esp_random() % (sizeof(alphabet) - 1)]);
    }
    return password;
}

static bool ensure_local_wifi_manager()
{
    auto& wifi = WifiManager::GetInstance();
    if (wifi.IsInitialized()) {
        return true;
    }

    WifiManagerConfig config;
    config.ssid_prefix = "Tim-Goose";
    config.language = "en-US";
    config.ap_password = generate_local_ap_password();
    return wifi.Initialize(config);
}
#endif

static void time_sync_notification_cb(struct timeval* tv)
{
    mclog::tagInfo(_tag, "SNTP time synchronized");
    GetHAL().syncSystemTimeToRtc();
}

void Hal::startSntp()
{
    mclog::tagInfo(_tag, "SNTP init");

    if (esp_sntp_enabled()) {
    } else {
        esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);

#if CONFIG_GOOSEOPS_LOCAL_ONLY
        // In local-only mode the NTP server must come from DHCP option 42.
        // If OPNsense does not advertise one, time remains on the local RTC;
        // there is deliberately no public fallback.
#if !CONFIG_LWIP_DHCP_GET_NTP_SRV
#error "GOOSEOPS_LOCAL_ONLY requires CONFIG_LWIP_DHCP_GET_NTP_SRV"
#endif
#else
        esp_sntp_setservername(0, "pool.ntp.org");
        esp_sntp_setservername(1, "time.google.com");
        esp_sntp_setservername(2, "cn.pool.ntp.org");
#endif

        sntp_set_time_sync_notification_cb(time_sync_notification_cb);

        esp_sntp_init();
    }
}

void Hal::startNetwork(std::function<void(std::string_view)> onLog)
{
    if (_is_network_connected) {
        mclog::tagInfo(_tag, "network already connected");
        return;
    }

    std::atomic<bool> network_connected = false;

    auto& board = Board::GetInstance();
    mclog::tagInfo(_tag, "start and wait for network connected...");

#if CONFIG_GOOSEOPS_LOCAL_ONLY
    // Enable DHCP-provided NTP before starting DHCP so option 42 is captured.
    esp_sntp_servermode_dhcp(true);
    if (!ensure_local_wifi_manager()) {
        mclog::tagError(_tag, "local WiFi manager initialization failed");
        return;
    }
#endif

    board.SetNetworkEventCallback([&network_connected, &onLog](NetworkEvent event, const std::string& data) {
        switch (event) {
            case NetworkEvent::Scanning:
                if (onLog) {
                    onLog("WiFi scanning...");
                }
                break;
            case NetworkEvent::Connecting: {
                if (data.empty()) {
                    if (onLog) {
                        onLog("WiFi connecting...");
                    }
                } else {
                    if (onLog) {
                        onLog(fmt::format("Connecting to {} ...", data));
                    }
                }
                break;
            }
            case NetworkEvent::Connected: {
                network_connected = true;
                break;
            }
            case NetworkEvent::Disconnected:
                break;
            case NetworkEvent::WifiConfigModeEnter: {
                auto& wifi_manager = WifiManager::GetInstance();
                auto msg = fmt::format("Enter WiFi config mode. Hotspot: {}, Config URL: {}", wifi_manager.GetApSsid(),
                                       wifi_manager.GetApWebUrl());
                if (onLog) {
                    onLog(msg);
                }
                break;
            }
            case NetworkEvent::WifiConfigModeExit:
                // WiFi config mode exit is handled by WifiBoard internally
                break;
            // Cellular modem specific events
            case NetworkEvent::ModemDetecting:
                break;
            case NetworkEvent::ModemErrorNoSim:
                break;
            case NetworkEvent::ModemErrorRegDenied:
                break;
            case NetworkEvent::ModemErrorInitFailed:
                break;
            case NetworkEvent::ModemErrorTimeout:
                break;
        }
    });
    board.StartNetwork();

    while (!network_connected) {
        GetHAL().delay(500);
    }
    mclog::tagInfo(_tag, "network connected");
    board.SetNetworkEventCallback(nullptr);

    startSntp();

    _is_network_connected = true;
}

LocalWifiProvisioningInfo Hal::startLocalWifiProvisioning()
{
    LocalWifiProvisioningInfo info;
#if CONFIG_GOOSEOPS_LOCAL_ONLY
    if (!ensure_local_wifi_manager()) {
        return info;
    }

    auto& wifi = WifiManager::GetInstance();
    wifi.StartConfigAp();
    info.active = wifi.IsConfigMode();
    info.ssid = wifi.GetApSsid();
    info.password = wifi.GetApPassword();
    info.url = wifi.GetApWebUrl();
#endif
    return info;
}

void Hal::stopLocalWifiProvisioning()
{
#if CONFIG_GOOSEOPS_LOCAL_ONLY
    WifiManager::GetInstance().StopConfigAp();
#endif
}

bool Hal::isLocalWifiProvisioningActive()
{
#if CONFIG_GOOSEOPS_LOCAL_ONLY
    return WifiManager::GetInstance().IsConfigMode();
#else
    return false;
#endif
}

WifiStatus Hal::getWifiStatus()
{
    auto& wifi = WifiManager::GetInstance();

    if (wifi.IsConfigMode()) {
        return WifiStatus::None;
    }
    if (!wifi.IsConnected()) {
        return WifiStatus::None;
    }

    int rssi = wifi.GetRssi();
    if (rssi >= -65) {
        return WifiStatus::High;
    } else if (rssi >= -75) {
        return WifiStatus::Medium;
    }
    return WifiStatus::Low;
}
