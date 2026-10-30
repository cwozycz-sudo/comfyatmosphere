# Modified 2026-10-30: explicit community release identifier.
# Writes version.h with `git describe` (the last tag, the commits since it, the commit, and -dirty for
# changes not committed), for the client report. Run on each build; the file is written only when the
# text changes, so an unchanged version rebuilds nothing.
execute_process(
    COMMAND git describe --tags --always --dirty
    WORKING_DIRECTORY "${SRC}"
    OUTPUT_VARIABLE ver
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
if(DEFINED RELEASE_VERSION AND NOT RELEASE_VERSION STREQUAL "")
    set(ver "${RELEASE_VERSION}")
endif()
if(NOT ver)
    set(ver "unknown")
endif()
set(text "#pragma once\n#define COMFYATMOS_VERSION \"${ver}\"\n")
if(EXISTS "${OUT}")
    file(READ "${OUT}" old)
endif()
if(NOT "${old}" STREQUAL "${text}")
    file(WRITE "${OUT}" "${text}")
endif()
