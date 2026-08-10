/*
 * SPDX-FileCopyrightText: 2026 GooseOps
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace gooseops {

enum class Severity : uint8_t {
    Healthy = 0,
    Info,
    Warning,
    Critical,
};

struct StatusEvent {
    std::string id;
    std::string source;
    std::string summary;
    Severity severity = Severity::Info;
    uint32_t expiresAtMs = 0;
};

class StatusProvider {
public:
    virtual ~StatusProvider() = default;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual std::optional<StatusEvent> poll(uint32_t nowMs) = 0;
    virtual bool isConnected() const = 0;
};

// Safe Phase-1 provider. It proves the presentation boundary without opening
// sockets or coupling the firmware to a GooseOps service.
class LocalStatusProvider final : public StatusProvider {
public:
    bool start() override;
    void stop() override;
    std::optional<StatusEvent> poll(uint32_t nowMs) override;
    bool isConnected() const override;

private:
    bool _running = false;
    bool _bootEventPending = false;
};

}  // namespace gooseops
