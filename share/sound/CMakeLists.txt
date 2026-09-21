# SPDX-License-Identifier: GPL-3.0-only
# MuseScore-Studio-CLA-applies
#
# MuseScore Studio
# Music Composition & Notation
#
# Copyright (C) 2021 MuseScore Limited and others
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

file(GLOB GAMELAN_SOUNDFONT_FILES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/soundfont/Gamelan_*.sf2"
    )
set(GAMELAN_ATTRIBUTION_FILE "${PROJECT_SOURCE_DIR}/soundfont/Gamelan_Open_Sample_Attribution.txt")
foreach (GAMELAN_SOUNDFONT IN LISTS GAMELAN_SOUNDFONT_FILES)
    get_filename_component(GAMELAN_SOUNDFONT_NAME "${GAMELAN_SOUNDFONT}" NAME)
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/share/sound")
    configure_file(
        "${GAMELAN_SOUNDFONT}"
        "${CMAKE_BINARY_DIR}/share/sound/${GAMELAN_SOUNDFONT_NAME}"
        COPYONLY
        )
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/src/app/mscore.app/Contents/Resources/sound")
    configure_file(
        "${GAMELAN_SOUNDFONT}"
        "${CMAKE_BINARY_DIR}/src/app/mscore.app/Contents/Resources/sound/${GAMELAN_SOUNDFONT_NAME}"
        COPYONLY
        )
endforeach ()

if (EXISTS "${GAMELAN_ATTRIBUTION_FILE}")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/share/sound")
    configure_file(
        "${GAMELAN_ATTRIBUTION_FILE}"
        "${CMAKE_BINARY_DIR}/share/sound/Gamelan_Open_Sample_Attribution.txt"
        COPYONLY
        )
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/src/app/mscore.app/Contents/Resources/sound")
    configure_file(
        "${GAMELAN_ATTRIBUTION_FILE}"
        "${CMAKE_BINARY_DIR}/src/app/mscore.app/Contents/Resources/sound/Gamelan_Open_Sample_Attribution.txt"
        COPYONLY
        )
endif ()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/MS Basic.sf3")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/share/sound")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/MS Basic.sf3"
        "${CMAKE_BINARY_DIR}/share/sound/MS Basic.sf3"
        COPYONLY
        )
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/src/app/mscore.app/Contents/Resources/sound")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/MS Basic.sf3"
        "${CMAKE_BINARY_DIR}/src/app/mscore.app/Contents/Resources/sound/MS Basic.sf3"
        COPYONLY
        )
endif ()

# Keep the complete lossless Gamelan master library beside the generated
# SoundFont resources.  FluidSynth does not load FLAC directly; this archive
# is bundled for the sample-mapping/conversion tools and for future profiles.
set(GAMELAN_MASTER_FLAC_DIR "${PROJECT_SOURCE_DIR}/soundfont/Gamelan_Master_FLAC")
if (IS_DIRECTORY "${GAMELAN_MASTER_FLAC_DIR}")
    file(COPY
        "${GAMELAN_MASTER_FLAC_DIR}"
        DESTINATION "${CMAKE_BINARY_DIR}/share/sound"
        )
    file(COPY
        "${GAMELAN_MASTER_FLAC_DIR}"
        DESTINATION "${CMAKE_BINARY_DIR}/src/app/mscore.app/Contents/Resources/sound"
        )
endif ()

if (MUE_INSTALL_SOUNDFONT)

    if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/MS Basic.sf3")
      install (FILES FluidR3Mono_GM.sf3
          DESTINATION ${MUSE_APP_INSTALL_RESOURCES_LOCATION}/sound
          RENAME "MS Basic.sf3"
          )
    else ()
      install(FILES
          "MS Basic.sf3"
          DESTINATION ${MUSE_APP_INSTALL_RESOURCES_LOCATION}/sound
          )
    endif ()

    if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/MS Basic_License.md")
      install (FILES FluidR3Mono_License.md
          DESTINATION ${MUSE_APP_INSTALL_RESOURCES_LOCATION}/sound
          RENAME "MS Basic_License.md")
    else ()
      install(FILES
          "MS Basic_License.md"
          DESTINATION ${MUSE_APP_INSTALL_RESOURCES_LOCATION}/sound
          )
    endif ()

    foreach (GAMELAN_SOUNDFONT IN LISTS GAMELAN_SOUNDFONT_FILES)
      install(FILES
          "${GAMELAN_SOUNDFONT}"
          DESTINATION ${MUSE_APP_INSTALL_RESOURCES_LOCATION}/sound
          )
    endforeach ()

    if (EXISTS "${GAMELAN_ATTRIBUTION_FILE}")
      install(FILES
          "${GAMELAN_ATTRIBUTION_FILE}"
          DESTINATION ${MUSE_APP_INSTALL_RESOURCES_LOCATION}/sound
          )
    endif ()

    if (IS_DIRECTORY "${GAMELAN_MASTER_FLAC_DIR}")
      install(DIRECTORY
          "${GAMELAN_MASTER_FLAC_DIR}"
          DESTINATION ${MUSE_APP_INSTALL_RESOURCES_LOCATION}/sound
          )
    endif ()

endif (MUE_INSTALL_SOUNDFONT)
