# Tim “Goose” Gooseman

This application is the GooseOps personality and presentation layer for StackChan.
It intentionally uses the public StackChan avatar, modifier, motion, and HAL APIs so
the fork can continue merging M5Stack upstream changes.

## Current scope

- Adds `TIM.GOOSE` to the stock launcher.
- Preserves the stock local applications, hardware tests, and robot behavior.
- Applies Tim's green-on-deep-green visual identity.
- Retains breathing, blinking, idle motion, head-pet, and IMU reactions.
- Maps transport-neutral GooseOps status events to speech, emotion, and LEDs.
- Uses a local provider that emits one startup event and opens no network sockets.
- Enables `CONFIG_GOOSEOPS_LOCAL_ONLY` by default, preventing stock cloud apps
  and stored AI-on-boot settings from starting M5Stack/XiaoZhi services.
- Accepts time service only from DHCP option 42 in local-only builds, with no
  public NTP fallback.
- Starts saved Wi-Fi profiles asynchronously at boot, with background retries;
  missing or unavailable Wi-Fi never blocks Tim's local behavior.

## Integration boundary

Future network clients must implement `gooseops::StatusProvider`. They should be
outbound-only, authenticated, fail closed, and contain no infrastructure management
credentials. The app must remain usable when the provider is unavailable.

Network transports, backend schemas, and credentials do not belong in the avatar or
motion classes. This keeps Tim's identity independent of the GooseOps service topology
and allows other GoosePals to use the same event model on different hardware.

After building the default GooseOps profile, run `python verify_local_only.py`
from the firmware directory. The verifier fails if Tim's identity is missing,
the local-only settings drift, or a known XiaoZhi/M5Stack/Chinese-hosted
endpoint is embedded in the resulting firmware.
