/*
 * SPDX-FileCopyrightText: 2026 GooseOps
 *
 * SPDX-License-Identifier: MIT
 */
#include "goose_status.h"

namespace gooseops {

bool LocalStatusProvider::start()
{
    _running = true;
    _bootEventPending = true;
    return true;
}

void LocalStatusProvider::stop()
{
    _running = false;
    _bootEventPending = false;
}

std::optional<StatusEvent> LocalStatusProvider::poll(uint32_t nowMs)
{
    if (!_running || !_bootEventPending) {
        return std::nullopt;
    }

    _bootEventPending = false;
    StatusEvent event;
    event.id = "local-boot";
    event.source = "tim";
    event.summary = "Papa. Tim is online. Systems nominal.";
    event.severity = Severity::Healthy;
    event.expiresAtMs = nowMs + 6000;
    return event;
}

bool LocalStatusProvider::isConnected() const
{
    return _running;
}

}  // namespace gooseops
