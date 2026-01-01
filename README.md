# PNGTuberORA
simple png tuber software using open raster format

to bootstrab the build system do `cc nob.c -o nob` then call `nob` and it should build
the executable would be in the build folder

the ORA file can be made in gimp or krita.

by naming the layers specific things it will change their behavior in the program

## layers cheat sheet
* hidden layers won't show up in rendering
* call a layer `blink` or `eyes closed` or similar for the blinking animation
* any other layer with `eye` in it will turn off during blinking
* call a layer `talk` or `mouth open` for when speaking
* another layer with `mouth` will be turned off during speaking
* layers starting with `c` followed by a number `1` to `9` and then `_` will be mapped to that respective costume, example `c3_hat`
* pivot layers start with `pivot_` followed by another layer's name. this will be the pivot of that layer. this only cares about the left and top pixels, just use a single pixel as the pivot

## visemes
added support for primitive visemes you can add `[AA]`, `[OU]` or `[CH]` to mouth open layers. see vis.ora for example.

visemes need to be "trained" on your voice. To do so, open the **Settings** menu. You can see the confidence bars for each viseme and use the training buttons to associate your voice with specific mouth shapes. Don't forget to press **Save** when you're done.
