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
# It runs scripts/build_docs.sh, which builds the G2Basic reference and then
# HomeCore's, linked through a Doxygen tag file.

find_package(Doxygen OPTIONAL_COMPONENTS dot)

if(NOT TARGET Doxygen::doxygen OR NOT TARGET Doxygen::dot)
    message(STATUS "Doxygen or Graphviz dot not found; 'docs' target disabled")
    return()
endif()

# scripts/build_docs.sh also builds the G2Basic reference, which needs the
# G2Basic submodule's Doxygen setup and its own theme submodule.
foreach(required
        external/doxygen-awesome-css/doxygen-awesome.css
        external/g2basic/docs/doxygen/groups.dox
        external/g2basic/external/doxygen-awesome-css/doxygen-awesome.css)
    if(NOT EXISTS ${HOMECORE_ROOT}/${required})
        message(STATUS "Missing ${required}; 'docs' target disabled. "
                       "Run: git submodule update --init --recursive")
        return()
    endif()
endforeach()

cmake_path(GET DOXYGEN_DOT_EXECUTABLE PARENT_PATH HOMECORE_DOT_DIR)
set(HOMECORE_DOCS_DIR ${CMAKE_CURRENT_BINARY_DIR}/docs)

# The script's defaults stamp each project() version and short commit.
add_custom_target(docs
    COMMAND ${CMAKE_COMMAND} -E env
        DOXYGEN=${DOXYGEN_EXECUTABLE}
        --modify PATH=path_list_prepend:${HOMECORE_DOT_DIR}
        bash ${HOMECORE_ROOT}/scripts/build_docs.sh ${HOMECORE_DOCS_DIR}
    WORKING_DIRECTORY ${HOMECORE_ROOT}
    COMMENT "Generating API documentation in ${HOMECORE_DOCS_DIR}"
    VERBATIM
)
