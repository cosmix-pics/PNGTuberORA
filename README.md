# PNGTuberORA
simple png tuber software using open raster format

this needs raylib.a inside a lib folder. see [raylib builds](https://github.com/raysan5/raylib/releases)

to bootstrab the build system do `cc nob.c -o nob` then call `nob` and it should build


## layers cheat sheet
* hidden layers wont show up in rendering
* call a layer `blink` or `eyes closed` or similar for the blinking animation
* any other layer with `eye` in it will turn of during blinking
* call a layer `talk` or `mouth open` for wen speaking
* another layer with `mouth` will be turned off during speaking
* layers starting with `c` folowed by a number `1` to `9` and then `_` will be mapped to that respective costume, example `c3_hat`
* pivot layers start with `pivot_` folowed by another layers name. this will be the pivot of that layer. this only cares about the left and top pixels, just use a single pixel as the pivot
