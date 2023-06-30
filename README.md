This is a fork of Manatee-open corpus search engine which is part of [NoSketch Engine project](https://nlp.fi.muni.cz/trac/noske).

Please note that the `main` branch does not contain any code. To obtain a concrete version,
please refer to a respective branch (e.g. `release-2.214.1`).

## How to build

0. make sure `build-essential`, `libtool`, `autoconf-archive` is installed
1. clone the repository, make sure all the dependencies are installed (:construction: - list deps)
  *   `libpcre3-dev`
3. `autoreconf --install --force`
4. `configure --with-pcre`
5. `./make`
6. `sudo make install`
7. `sudo ldconfig`

## Available versions

* [2.214.1](https://github.com/czcorpus/manatee-open/tree/release-2.214.1)
