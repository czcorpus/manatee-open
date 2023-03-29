This is a fork of Manatee-open corpus search engine which is part of [NoSketch Engine project](https://nlp.fi.muni.cz/trac/noske).

Please note that the `main` branch does not contain any code. To obtain a concrete version,
please refer to a respective branch (e.g. `release-2.214.1`).

## How to build

1. clone the repository, make sure all the dependencies are installed (:construction: - list deps)
2. `autoreconf --install --force`
3. `configure --with-pcre`
4. `make`
5. `sudo make install`
6. `sudo ldconfig`
