/*
 * SPDX-FileCopyrightText: 2026 GooseOps
 *
 * SPDX-License-Identifier: MIT
 */
#include "app_gooseops_tim.h"

#include <apps/common/common.h>
#include <assets/assets.h>
#include <hal/hal.h>
#include <mooncake_log.h>
#include <smooth_lvgl.hpp>
#include <stackchan/stackchan.h>

using namespace smooth_ui_toolkit::lvgl_cpp;
using namespace stackchan;

namespace {
constexpr uint32_t kTimGreen = 0x33CC99;
constexpr uint32_t kTimBackground = 0x071B18;

avatar::Emotion emotionFor(gooseops::Severity severity)
{
    switch (severity) {
        case gooseops::Severity::Healthy:
            return avatar::Emotion::Happy;
        case gooseops::Severity::Info:
            return avatar::Emotion::Neutral;
        case gooseops::Severity::Warning:
            return avatar::Emotion::Doubt;
        case gooseops::Severity::Critical:
            return avatar::Emotion::Angry;
    }
    return avatar::Emotion::Neutral;
}

void setStatusLights(gooseops::Severity severity)
{
    switch (severity) {
        case gooseops::Severity::Healthy:
            GetHAL().showRgbColor(0, 64, 24);
            break;
        case gooseops::Severity::Info:
            GetHAL().showRgbColor(0, 24, 64);
            break;
        case gooseops::Severity::Warning:
            GetHAL().showRgbColor(96, 48, 0);
            break;
        case gooseops::Severity::Critical:
            GetHAL().showRgbColor(96, 0, 0);
            break;
    }
}
}  // namespace

AppGooseOpsTim::AppGooseOpsTim()
{
    setAppInfo().name = "TIM.GOOSE";
    static auto icon = assets::get_image("icon_sentinel.bin");
    setAppInfo().icon = (void*)&icon;
    static uint32_t themeColor = kTimGreen;
    setAppInfo().userData = (void*)&themeColor;
}

void AppGooseOpsTim::onCreate()
{
    mclog::tagInfo(getAppInfo().name, "on create");
}

void AppGooseOpsTim::onOpen()
{
    mclog::tagInfo(getAppInfo().name, "on open");

    LvglLockGuard lock;
    auto& tim = GetStackChan();

    auto face = std::make_unique<avatar::DefaultAvatar>();
    face->primaryColor = lv_color_hex(kTimGreen);
    face->secondaryColor = lv_color_hex(kTimBackground);
    face->init(lv_screen_active());
    tim.attachAvatar(std::move(face));

    tim.clearModifiers();
    tim.addModifier(std::make_unique<BreathModifier>());
    tim.addModifier(std::make_unique<BlinkModifier>());
    tim.addModifier(std::make_unique<IdleExpressionModifier>());
    tim.addModifier(std::make_unique<IdleMotionModifier>());
    tim.addModifier(std::make_unique<HeadPetModifier>());
    tim.addModifier(std::make_unique<ImuEventModifier>());

    view::create_home_indicator([this]() { close(); }, kTimGreen, kTimBackground);
    view::create_status_bar(kTimGreen, kTimBackground);

    _statusProvider = std::make_unique<gooseops::LocalStatusProvider>();
    if (!_statusProvider->start()) {
        mclog::tagError(getAppInfo().name, "status provider failed to start");
    }
}

void AppGooseOpsTim::onRunning()
{
    const uint32_t now = GetHAL().millis();

    if (_statusProvider) {
        auto event = _statusProvider->poll(now);
        if (event.has_value() && event->id != _activeEventId) {
            present(*event);
        }
    }

    clearExpiredStatus(now);

    LvglLockGuard lock;
    GetStackChan().update();
    view::update_home_indicator();
    view::update_status_bar();
}

void AppGooseOpsTim::onClose()
{
    mclog::tagInfo(getAppInfo().name, "on close");

    if (_statusProvider) {
        _statusProvider->stop();
        _statusProvider.reset();
    }

    LvglLockGuard lock;
    auto& tim = GetStackChan();
    tim.clearModifiers();
    tim.resetAvatar();
    GetHAL().showRgbColor(0, 0, 0);
    view::destroy_home_indicator();
    view::destroy_status_bar();

    _activeEventId.clear();
    _activeEventExpiresAtMs = 0;
}

void AppGooseOpsTim::present(const gooseops::StatusEvent& event)
{
    LvglLockGuard lock;
    auto& tim = GetStackChan();
    if (!tim.hasAvatar()) {
        return;
    }

    _activeEventId = event.id;
    _activeEventExpiresAtMs = event.expiresAtMs;
    tim.avatar().setSpeech(event.summary);
    tim.avatar().setEmotion(emotionFor(event.severity));
    setStatusLights(event.severity);

    mclog::tagInfo(getAppInfo().name, "status event [{}] from {}", event.id, event.source);
}

void AppGooseOpsTim::clearExpiredStatus(uint32_t nowMs)
{
    if (_activeEventId.empty() || _activeEventExpiresAtMs == 0 ||
        static_cast<int32_t>(nowMs - _activeEventExpiresAtMs) < 0) {
        return;
    }

    LvglLockGuard lock;
    auto& tim = GetStackChan();
    if (tim.hasAvatar()) {
        tim.avatar().setSpeech("");
        tim.avatar().setEmotion(avatar::Emotion::Neutral);
    }
    GetHAL().showRgbColor(0, 16, 6);
    _activeEventId.clear();
    _activeEventExpiresAtMs = 0;
}
