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
    INCLUDE_DIRS
        "../../../src/game"
    REQUIRES
        driver esp_lcd
)
