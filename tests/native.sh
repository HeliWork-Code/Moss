#!/usr/bin/env bash
# Runs the whole suite through the native C backend.
set -u
export MOSS_BACKEND=native
exec bash "$(dirname "$0")/run.sh"
