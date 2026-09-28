#!/usr/bin/env bash
# Build the HomeCore API reference with the G2Basic reference nested inside it.
#
# Usage: scripts/build_docs.sh [output-dir]   (default: build/docs)
#
# 1. G2Basic's own Doxyfile builds its reference into <output-dir>/g2basic and
#    writes a tag file describing its symbols and groups.
# 2. HomeCore's Doxyfile builds into <output-dir>, reading that tag file so that
#    G2Basic groups appear in the Topics tree and references link into
#    g2basic/. The "g2basic" section of the main page is enabled.
#
# Both Doxyfiles are used unchanged; overrides are appended on standard input.
# Environment: DOXYGEN (default: doxygen), HOMECORE_DOCS_VERSION and
# G2BASIC_DOCS_VERSION (default: project() version and short commit).
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
g2basic="$root/external/g2basic"
doxygen="${DOXYGEN:-doxygen}"

mkdir -p "${1:-$root/build/docs}"
output="$(cd "${1:-$root/build/docs}" && pwd)"
tagfile="$(dirname "$output")/g2basic.tag"

# The pinned G2Basic commit must include its Doxygen setup and theme.
for required in "$g2basic/docs/doxygen/groups.dox" \
                "$g2basic/external/doxygen-awesome-css/doxygen-awesome.css" \
                "$root/external/doxygen-awesome-css/doxygen-awesome.css"; do
    if [[ ! -f "$required" ]]; then
        echo "error: missing $required" >&2
        echo "Run: git submodule update --init --recursive" >&2
        exit 1
    fi
done

# Prints "<project() version> (<short commit>)" for a repository.
project_version() {
    local version
    version="$(sed -n 's/^\(project([^ ]*\)\{0,1\} *VERSION \([0-9.]*\).*/\2/p' "$1/CMakeLists.txt" \
        | head -n 1)"
    echo "$version ($(git -C "$1" rev-parse --short=7 HEAD))"
}

export HOMECORE_DOCS_VERSION="${HOMECORE_DOCS_VERSION:-$(project_version "$root")}"
export G2BASIC_DOCS_VERSION="${G2BASIC_DOCS_VERSION:-$(project_version "$g2basic")}"

# Doxygen creates only the last directory level, so <output-dir> exists first.
(
    cd "$g2basic"
    {
        cat Doxyfile
        echo "OUTPUT_DIRECTORY = \"$output\""
        echo "HTML_OUTPUT = g2basic"
        echo "GENERATE_TAGFILE = \"$tagfile\""
    } | "$doxygen" -
)

(
    cd "$root"
    {
        cat Doxyfile
        echo "OUTPUT_DIRECTORY = \"$(dirname "$output")\""
        echo "HTML_OUTPUT = \"$(basename "$output")\""
        echo "TAGFILES = \"$tagfile=g2basic\""
        echo "ENABLED_SECTIONS = g2basic"
    } | "$doxygen" -
)

echo "API documentation: $output/index.html (G2Basic: $output/g2basic/index.html)"
