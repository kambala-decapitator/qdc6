# qdc6

Diablo 2 dc6 image converter using [Qt framework](https://qt.io/).

## Using

Check the help message for details.

You can find palettes in `d2data.mpq` under `data\global\palette\*`. The default one already embedded in the application is `data\global\palette\units\pal.dat`.

## Building

You will need:

1. C++11 compiler.
2. Qt 4 or higher.

Standard `qmake` approach. Create build directory anywhere you like and run

    /path/to/qmake /path/to/qdc6.pro

After that use `jom` or `nmake` on Windows and `make` on other OS.

## Resources

- [dc6 format info](https://d2mods.info/forum/viewtopic.php?t=724#p148076)
- [other dc6 tools](https://d2mods.info/forum/downloadsystemcat?id=14)

## Why

Got really fed up with:

- dc6con -> pcx (who uses this format nowadays anyway?) -> imagemagik to obtain png
- run DC6Creator under wine and make lots of clicks to convert skill icons into separate images

Besides, none of the tools I know allow saving multi-frame dc6 into separate files from command-line.

And it's cross-platform!
