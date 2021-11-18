# qdc6

Diablo 2 dc6 image converter using [Qt framework](https://qt.io/).

## Using

Check the help message for details.

You can find palettes in `d2data.mpq` under `data\global\palette\*`. The default one already embedded in the application is `data\global\palette\units\pal.dat`.

## Building

You will need:

1. C++11 compiler.
2. Qt 4 or higher.
3. Optionally: CMake.

To build from GUI, simply open `qdc6.pro` or `CMakeLists.txt` in Qt Creator. The following instructions are for command line.

### qmake

Standard `qmake` approach. Create build directory anywhere you like and run

    /path/to/qmake /path/to/qdc6.pro

If you want to build with SVG support (requires respective Qt module), also pass `CONFIG+=svg` parameter:

    /path/to/qmake CONFIG+=svg /path/to/qdc6.pro

After that use `jom` or `nmake` on Windows and `make` on other OS.

### CMake

Standard CMake approach.

    cmake -B /path/to/build/dir -S /path/to/source/dir

- pass `-DSVG=ON` if you want to build with SVG support (requires respective Qt module)
- pass `"-DCMAKE_PREFIX_PATH=<your Qt directory>/lib/cmake"` to find your Qt installation
- pass other parameters like `-G` as needed

After that:

    cmake --build /path/to/build/dir

## Resources

- [dc6 format info](https://d2mods.info/forum/viewtopic.php?t=724#p148076)
- [other dc6 tools](https://d2mods.info/forum/downloadsystemcat?id=14)

## Why

Got really fed up with:

- dc6con -> pcx (who uses this format nowadays anyway?) -> imagemagik to obtain png
- run DC6Creator under wine and make lots of clicks to convert skill icons into separate images

Besides, none of the tools I know allow saving multi-frame dc6 into separate files from command-line.

And it's cross-platform!
