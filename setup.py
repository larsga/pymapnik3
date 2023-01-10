
from distutils.core import setup, Extension

MAPNIK = '/Users/larsga/cvs-co/mapnik/'

module1 = Extension(
    'pymapnik3',
    include_dirs = [MAPNIK + 'include/',
                    '/usr/local/opt/boost@1.76/include',
                    MAPNIK + 'deps/mapbox/variant/include/',
                    '/usr/local/opt/icu4c/include',
                    '/usr/local/opt/harfbuzz/include/',
                    '/usr/local/opt/cairo/include/'],
    libraries = ['mapnik', 'mapnik-json'],
    library_dirs = [MAPNIK + 'src/',
                    MAPNIK + 'src/json'],
    sources = ['src/pymapnik3.cpp'],
    extra_compile_args = [
        '-std=c++14', '-Wno-unused-variable', '-stdlib=libc++',
        '-DBIGINT', # mapnik is compiled with this, so we must be, too
    ],    
)

setup(
    name = 'pymapnik3',
    version = '1.0',
    description = 'A wrapper for mapnik 3',
    ext_modules = [module1]
)
