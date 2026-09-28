# MIT License
#
# Copyright (c) 2026 Grzegorz Grzęda
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# Optional API documentation target. The firmware build never depends on it.
# The repository Doxyfile is included unchanged; this wrapper only sets the
# version, the build-directory output location, and the Graphviz path.

find_package(Doxygen OPTIONAL_COMPONENTS dot)

if(NOT TARGET Doxygen::doxygen OR NOT TARGET Doxygen::dot)
    message(STATUS "Doxygen or Graphviz dot not found; 'docs' target disabled")
    return()
endif()

if(NOT EXISTS ${HOMECORE_ROOT}/external/doxygen-awesome-css/doxygen-awesome.css)
    message(STATUS "Documentation theme submodule missing; 'docs' target disabled. "
                   "Run: git submodule update --init external/doxygen-awesome-css")
    return()
endif()

cmake_path(GET DOXYGEN_DOT_EXECUTABLE PARENT_PATH HOMECORE_DOT_DIR)
set(HOMECORE_DOCS_DIR ${CMAKE_CURRENT_BINARY_DIR}/docs)
# The Doxyfile places HTML in the docs/ subdirectory of OUTPUT_DIRECTORY.
set(HOMECORE_DOXYFILE ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.docs)

file(CONFIGURE OUTPUT ${HOMECORE_DOXYFILE} CONTENT [[
@INCLUDE         = "@HOMECORE_ROOT@/Doxyfile"
PROJECT_NUMBER   = "@PROJECT_VERSION@"
OUTPUT_DIRECTORY = "@CMAKE_CURRENT_BINARY_DIR@"
DOT_PATH         = "@HOMECORE_DOT_DIR@"
]] @ONLY)

# Paths in the Doxyfile are relative to the repository root.
add_custom_target(docs
    COMMAND Doxygen::doxygen ${HOMECORE_DOXYFILE}
    WORKING_DIRECTORY ${HOMECORE_ROOT}
    COMMENT "Generating API documentation in ${HOMECORE_DOCS_DIR}"
    VERBATIM
)
