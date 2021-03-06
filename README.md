# Wii U Linux Launcher [![(build status)](https://travis-ci.org/neuschaefer/wiiu-linux-launcher.svg?branch=master)](https://travis-ci.org/neuschaefer/wiiu-linux-launcher)

(Insert screenshot here)


## License

This project as a whole is licensed under the [GNU GPLv2][gplv2].

The soft keyboard ([`keyboard.c`][keybc]/[`keyboard.h`][keybh]) is
[BSD3-licensed][bsd3], because I think it's useful for other homebrew software,
too.


## Dependencies

A few git repositories are pulled in as git submodules:

 - [`dynamic_libs`][dynamic_libs]: Bindings for the dynamic libraries that are
   present on the Wii U.

To automatically download them, use `git clone --recursive`.


[gplv2]: https://www.gnu.org/licenses/gpl-2.0.html
[keybc]: keyboard.c
[keybh]: keyboard.h
[bsd3]: https://directory.fsf.org/wiki/License:BSD_3Clause
[dynamic_libs]: https://github.com/Maschell/dynamic_libs
