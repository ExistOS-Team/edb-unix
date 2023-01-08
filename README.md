# ExistOS Dump Bridge for Linux

_Forked from [yuuki410/ExistOS-Dump-Brige](https://github.com/yuuki410/ExistOS-Dump-Brige)_

A Linux port of [edb](https://github.com/ExistOS-Team/edb).

Flashing ExistOS firmware to HP39gII.

## TODO

- [x] flash via USB MSC
- ~~flash via COM serial port~~ no serial for you :D

## Compiling

```
mkdir build
cmake -B build
cmake --build build
```

## Flashing Firmware

```
edb -r -s -f OSLoader.sb 1408 b -f ExistOS.sys 1984
```

## Known issues

If you use `sbtool` in conjunction with `edb`, for example loading OSLoader into calculator's RAM, there's a chance that the ExistOS disk cannot be mounted properly.

To circumvent this, put a battery into the compartment (if there isn't any), unplug & replug the USB cable, then try again. That should be it.
