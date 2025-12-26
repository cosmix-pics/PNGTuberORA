# PNGTuberORA
simple png tuber software using open raster format

this needs raylib.a (libraylib.a on linux) inside a lib folder. see [raylib builds](https://github.com/raysan5/raylib/releases)

to bootstrab the build system do `cc nob.c -o nob` then call `nob` and it should build
the executable would be in the build folder

the ORA file can be made in gimp or krita.

by naming the layers specific things it will change there behavior in the program

## layers cheat sheet
* hidden layers wont show up in rendering
* call a layer `blink` or `eyes closed` or similar for the blinking animation
* any other layer with `eye` in it will turn of during blinking
* call a layer `talk` or `mouth open` for wen speaking
* another layer with `mouth` will be turned off during speaking
* layers starting with `c` folowed by a number `1` to `9` and then `_` will be mapped to that respective costume, example `c3_hat`
* pivot layers start with `pivot_` folowed by another layers name. this will be the pivot of that layer. this only cares about the left and top pixels, just use a single pixel as the pivot

## visimes
added suport for primitive visime's you can add `[AA]`, `[OU]` or `[CH]` to mouth open layers. see vis.ora for example.

visimes need to be "trained" on your voice to do so start the program with the argument `-visime` use the numrow keys 1 to 4 to train the specific sound then it can be saved by pressing 0
