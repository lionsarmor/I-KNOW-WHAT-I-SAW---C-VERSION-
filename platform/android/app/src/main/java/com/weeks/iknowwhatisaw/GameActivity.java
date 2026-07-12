package com.weeks.iknowwhatisaw;

/**
 * The app's entry point.
 *
 * This exists because the manifest names an activity RELATIVE to the app's
 * package. SDL's own activity lives in org.libsdl.app, so without this class
 * Android looks for com.weeks.iknowwhatisaw.SDLActivity, fails to find it,
 * and the app dies with a ClassNotFoundException the instant you tap it --
 * it installs perfectly and then simply will not start.
 *
 * All it does is say which native libraries to load. Everything else -- the
 * window, the GL surface, audio, gamepads, the touch events -- is SDL's, and
 * the game itself is libmain.so: the same C sources as every other platform.
 */
public class GameActivity extends org.libsdl.app.SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "main" };
    }
}
