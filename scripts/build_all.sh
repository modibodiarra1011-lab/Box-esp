#!/usr/bin/env bash
set -euo pipefail
python3 "$(dirname "$0")/build_all.py" --projects "${1:-projects}" --output "${2:-BUILD_OUTPUT}" --sd "${3:-SD_READY}"
