# The ESP32 app = the platform layer + the UNCHANGED portable core.
# Note the core files are pulled straight from src/game -- there is no
# copy. Fix a bug once, every platform gets it.
idf_component_register(
    SRCS
        "esp32_main.c"
        "../../../src/game/game.c"
        "../../../src/game/gfx.c"
        "../../../src/game/audio.c"
        "../../../src/game/assets.c"
        "../../../src/game/intro.c"
        "../../../src/game/overworld.c"
        "../../../src/game/battle.c"
        "../../../src/game/cutscene.c"
    INCLUDE_DIRS
        "../../../src/game"
    REQUIRES
        driver esp_lcd nvs_flash
)

# The content tables (spawns, species, items) are deliberately terse: one
# row per NPC, trailing fields omitted and zero-filled by C. That is the
# whole point of them, so don't warn about it. (Mirrors the desktop Makefile.)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)

# THE HANDHELD'S AUDIO BUDGET. The desktop runs the tracker at 44.1kHz with
# 8 voices; this little thing gets 22.05kHz and 4. Same songs, same code --
# a 4-track song just loses its last track here (see audio_music()). Raise
# these if the MAX98357A has headroom to spare and the CPU doesn't complain.
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    AUDIO_RATE=22050
    AUDIO_CHANS=4
)
