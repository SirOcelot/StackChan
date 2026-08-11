# GooseOps StackChan architecture

This fork turns StackChan into Tim "Goose" Gooseman while keeping the M5Stack
repository usable as an upstream. GooseOps additions live behind
`CONFIG_GOOSEOPS_LOCAL_ONLY` and use StackChan's existing application, avatar,
motion, and hardware abstraction interfaces.

## Upstream relationship

- `upstream` tracks `m5stack/StackChan`; `origin` is the GooseOps fork.
- GooseOps work is developed on `agent/*` branches and merged intentionally.
- Third-party source changes are stored as small patches against pinned commits.
- Downloaded components, build trees, device dumps, and credentials are never
  committed.
- An upstream update is merged first, followed by a clean build, local-only
  verification, hardware tests, and only then a GooseOps release.

## Phase 1: local-only Tim MVP

The MVP keeps the launcher, ESP-NOW control, dance, setup, hardware tests, face,
motion, microphone, speaker, servos, touch, IMU, and RGB behavior. It adds Tim's
identity and a transport-neutral status presentation boundary.

The default local-only profile:

- excludes XiaoZhi, AI Agent, Avatar cloud mode, App Center, EzData, account,
  cloud OTA, and remote avatar implementations from the compiled application;
- cannot honor an old AI-on-boot preference;
- obtains NTP servers only through DHCP option 42, with no public fallback;
- provisions Wi-Fi through a device-hosted `http://192.168.4.1` portal;
- protects the temporary `Tim-Goose-*` setup network with a random 14-character
  WPA2/WPA3 password shown on Tim's screen;
- starts saved Wi-Fi profiles asynchronously with bounded retry backoff, never
  blocking the launcher or local character behavior; and
- remains useful when no LAN or GooseOps service is available.

GooseOps builds retain the upstream version and add a fork revision, for example
`1.4.3-goose.2`, so deployed images are never mistaken for stock firmware.

Run `python verify_local_only.py` after every build. It checks required settings,
required Tim markers, and known vendor/cloud endpoints in both the ELF and binary.
Run `python scan_secrets.py` before every commit.

## Phase 2: GooseOps status adapter

Implement a new `gooseops::StatusProvider`; do not add network logic to the
avatar, motion, or launcher code. Tim should make outbound connections only to a
single local GooseOps gateway and receive a small, versioned, read-only status
envelope. The adapter must use short timeouts, bounded payloads, backoff with
jitter, stale-data indicators, and an offline state that never blocks local robot
behavior.

The device must not receive OPNsense, QNAP, Docker, hypervisor, or service-admin
credentials. Infrastructure collectors remain behind the gateway. A Tim-specific
token or certificate may read only Tim's status feed and must be independently
revocable.

Tim's production placement is the trusted wireless network on VLAN 20. Tim is a
managed GooseOps operations client, not a general-purpose or untrusted IoT device.
Putting Tim on IoT VLAN 30 would require an exception back into production and
would work against that VLAN's isolation policy; reserve VLAN 30 for quarantine or
lab testing rather than normal operation.

Least privilege is enforced at the service boundary: Tim makes an outbound,
authenticated connection to one GooseOps gateway and holds no infrastructure
management credentials. If the gateway is across a routed boundary, permit only
its exact destination and service port plus the approved DNS/NTP path. If Tim and
the gateway share VLAN 20, use gateway host-firewall and application authorization
because same-subnet traffic does not traverse OPNsense for filtering. Tim may still
be denied WAN access with a source-specific VLAN 20 firewall rule.

## Phase 3: operational hardening

- Add structured local event counters and last-success/error timestamps.
- Add a device diagnostics screen that reveals no secrets.
- Use an A/B application layout and locally served, signed OTA artifacts.
- Require explicit operator approval for production firmware promotion.
- Retain a known-good image and documented cable-flash recovery procedure.
- Test upstream merges and local-only endpoint verification in CI.

## GoosePals boundary

GooseOps may define a shared, versioned status-event contract, but character
identity stays on each device. Tim is the technical lead on StackChan. DJ ch33r.io
is a separate character on Loona DeskMate and should not share device firmware,
hardware abstractions, motion code, or credentials with Tim.

## Rollback

During development, flash only the application partition when bootloader,
partition table, assets, and settings do not change. Keep full-flash backups
outside the repository. To roll back, restore the last verified application image
or the complete device backup over USB; never publish a device dump because it may
contain Wi-Fi credentials or other local configuration.
