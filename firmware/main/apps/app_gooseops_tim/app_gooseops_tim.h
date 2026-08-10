/*
 * SPDX-FileCopyrightText: 2026 GooseOps
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "goose_status.h"
#include <mooncake.h>
#include <cstdint>
#include <memory>
#include <string>

class AppGooseOpsTim : public mooncake::AppAbility {
public:
    AppGooseOpsTim();

    void onCreate() override;
    void onOpen() override;
    void onRunning() override;
    void onClose() override;

private:
    void present(const gooseops::StatusEvent& event);
    void clearExpiredStatus(uint32_t nowMs);

    std::unique_ptr<gooseops::StatusProvider> _statusProvider;
    std::string _activeEventId;
    uint32_t _activeEventExpiresAtMs = 0;
};
