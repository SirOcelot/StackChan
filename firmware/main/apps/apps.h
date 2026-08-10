/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "app_launcher/app_launcher.h"
#include <sdkconfig.h>
#if !CONFIG_GOOSEOPS_LOCAL_ONLY
#include "app_ai_agent/app_ai_agent.h"
#include "app_avatar/app_avatar.h"
#endif
#include "app_setup/app_setup.h"
#include "app_espnow_ctrl/app_espnow_ctrl.h"
#if !CONFIG_GOOSEOPS_LOCAL_ONLY
#include "app_app_center/app_app_center.h"
#include "app_ezdata/app_ezdata.h"
#endif
#include "app_dance/app_dance.h"
#include "app_gooseops_tim/app_gooseops_tim.h"
