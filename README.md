
# pymapnik3

Python 3.x bindings for the [mapnik](https://github.com/mapnik/mapnik)
map-rendering engine.

Currently in development and *not* ready for use. Contributions welcome.

## Goals

 * Easy, reliable build.
 * No magic code -- all simple, straightforward, readable.
 * Support Mapnik 3.1.
 * Support Python 3, but not 2.
 * Similar API to [python-mapnik](https://github.com/mapnik/python-mapnik).
 * Support what's needed by [fhdb](https://github.com/larsga/fhdb) 
   *plus* `TextSymbolizer` and `ShieldSymbolizer`.

## Outstanding work

 * `RasterSymbolizer` doesn't do anything.
 * Need to detect mapnik exceptions and handle them.
 * Build system needs to automatically detect whether or not to define `BIGINT`.
 * `setup.py` has lots of hard-coded paths.
 * Needs documentation to show how to use the bindings.
 * Implement CSV data source.