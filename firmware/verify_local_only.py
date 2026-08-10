"""Fail closed if a GooseOps firmware image contains prohibited cloud endpoints."""

from __future__ import annotations

import argparse
import re
from pathlib import Path


FORBIDDEN_ENDPOINTS = {
    b"api.tenclass.net": "stock XiaoZhi OTA service",
    b"tenclass": "stock XiaoZhi service",
    b"47." + b"113.125.164": "stock StackChan public server",
    b"ezdata2.m5stack.com": "M5Stack EzData service",
    b"my.m5stack.com": "M5Stack account/EzData portal",
    b"xiaozhi.me": "XiaoZhi cloud service",
    b"xiaozhi.ai": "XiaoZhi cloud service",
    b"feishu.cn": "Feishu-hosted XiaoZhi documentation",
    b"gitee.com": "Gitee-hosted dependency reference",
    b"cn.pool.ntp.org": "Chinese public NTP service",
    b"pool.ntp.org": "public NTP fallback",
    b"time.google.com": "public Google NTP fallback",
    b"apps.apple.com/us/app/stackchan-world": "stock StackChan mobile app",
    b"play.google.com/store/apps/details?id=com.m5stack.stackchan": "stock StackChan mobile app",
}

REQUIRED_MARKERS = (
    b"TIM.GOOSE",
    b"Papa. Tim is online. Systems nominal.",
    b"Tim-Goose",
    b"Local only - no account or vendor app",
)
REQUIRED_CONFIG = (
    'CONFIG_GOOSEOPS_LOCAL_ONLY=y',
    'CONFIG_STACKCHAN_SERVER_URL="http://127.0.0.1"',
    'CONFIG_OTA_URL="https://127.0.0.1/disabled"',
    'CONFIG_LWIP_DHCP_GET_NTP_SRV=y',
)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "build_dir",
        nargs="?",
        default="build-gooseops",
        help="ESP-IDF build directory (default: build-gooseops)",
    )
    args = parser.parse_args()

    root = Path(__file__).resolve().parent
    build_dir = (root / args.build_dir).resolve()
    artifacts = [build_dir / "stack-chan.bin", build_dir / "stack-chan.elf"]

    config = (root / "sdkconfig").read_text(encoding="utf-8")
    errors: list[str] = []
    for setting in REQUIRED_CONFIG:
        if setting not in config:
            errors.append(f"required local-only setting is missing: {setting}")

    payloads: dict[Path, bytes] = {}
    for artifact in artifacts:
        if not artifact.is_file():
            errors.append(f"firmware artifact is missing: {artifact}")
            continue
        payloads[artifact] = artifact.read_bytes()

    for artifact, payload in payloads.items():
        lowered = payload.lower()
        for endpoint, description in FORBIDDEN_ENDPOINTS.items():
            if endpoint.lower() in lowered:
                errors.append(
                    f"{artifact.name} contains {description}: {endpoint.decode()}"
                )
        for marker in REQUIRED_MARKERS:
            if marker not in payload:
                errors.append(
                    f"{artifact.name} is missing Tim marker: {marker.decode()}"
                )

    urls = sorted(
        {
            match.decode("ascii", errors="replace")
            for payload in payloads.values()
            for match in re.findall(rb"https?://[^\x00\x20\x22\x27<>]{1,180}", payload)
        }
    )
    print("Compiled URL inventory:")
    for url in urls:
        print(f"  {url}")

    if errors:
        print("Local-only verification FAILED:")
        for error in errors:
            print(f"  - {error}")
        return 1

    print("Local-only verification PASSED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
