# ExistOS Dump Bridge for Linux

Forked from [yuuki410/ExistOS-Dump-Brige](https://github.com/yuuki410/ExistOS-Dump-Brige)

A Linux port of [edb](https://github.com/ExistOS-Team/edb).

Flashing ExistOS firmware to HP39gii.

## TODO

- [x] flash via USB MSC
- [ ] flash via COM serial port

## Compiling

```
g++ main.cpp EDBInterface.cpp -o edb
```

## Known issues

If you use `sbtool` in junction with `edb`, for example loading OSLoader into calculator's RAM, there's chance that the ExistOS disk cannot be mounted properly.

To circumvent this, put a battery into the compartment, unplug & replug the USB cable, then try again. That should be it.
