#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
test_dir="$(mktemp -d)"
trap 'rm -rf "$test_dir"' EXIT
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror   -I"$project_dir/test/stubs" -I"$project_dir/src"   "$project_dir/src/rpm_indicator.cpp" "$project_dir/test/test_rpm_indicator.cpp"   -o "$test_dir/test_rpm_indicator"
"$test_dir/test_rpm_indicator"

"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -I"$project_dir/src" \
    "$project_dir/test/test_gauge_smoothing.cpp" -o "$test_dir/test_gauge_smoothing"
"$test_dir/test_gauge_smoothing"
