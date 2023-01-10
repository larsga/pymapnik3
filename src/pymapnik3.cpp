
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>

#include <string>

#include <mapnik/config.hpp>

#pragma GCC diagnostic push
#include <mapnik/warning_ignore.hpp>
#include <boost/python.hpp>
#include <boost/noncopyable.hpp>
#include <boost/python/iterator.hpp>
#include <boost/python/call_method.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/to_python_converter.hpp>
#pragma GCC diagnostic pop

#include <mapnik/value_types.hpp>

#include <mapnik/agg_renderer.hpp>
#include <mapnik/box2d.hpp>
#include <mapnik/color.hpp>
#include <mapnik/datasource.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/feature_kv_iterator.hpp>
#include <mapnik/feature_type_style.hpp>
#include <mapnik/image_any.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/wkb.hpp>
#include <mapnik/json/feature_parser.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/map.hpp>
#include <mapnik/memory_datasource.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/proj_transform.hpp>
#include <mapnik/text/placements/dummy.hpp>
#include <mapnik/text/formatting/text.hpp>
#include <mapnik/font_engine_freetype.hpp>

#ifdef HAVE_CAIRO
#include <mapnik/cairo_io.hpp>
#endif

#include <iostream>

// ===========================================================================
// BOX2D

typedef struct {
    PyObject_HEAD
    mapnik::box2d<double>* box;
} MapnikBox2d;

static void
Box_dealloc(MapnikBox2d *self)
{
    delete self->box;
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int
Box_init(MapnikBox2d *self, PyObject *args)
{
    double minx, miny, maxx, maxy;
    if (!PyArg_ParseTuple(args, "dddd", &minx, &miny, &maxx, &maxy))
        return -1;

    self->box = new mapnik::box2d<double>(minx, miny, maxx, maxy);
    
    return 0;
}

static PyMemberDef Box_members[] = {
    {NULL}  /* Sentinel */
};

static PyObject *
Box_minx(MapnikBox2d *self, PyObject *Py_UNUSED(ignored))
{
    return Py_BuildValue("d", self->box->minx());
}

static PyObject *
Box_miny(MapnikBox2d *self, PyObject *Py_UNUSED(ignored))
{
    return Py_BuildValue("d", self->box->miny());
}

static PyObject *
Box_maxx(MapnikBox2d *self, PyObject *Py_UNUSED(ignored))
{
    return Py_BuildValue("d", self->box->maxx());
}

static PyObject *
Box_maxy(MapnikBox2d *self, PyObject *Py_UNUSED(ignored))
{
    return Py_BuildValue("d", self->box->maxy());
}

static PyMethodDef Box_methods[] = {
    {"minx", (PyCFunction) Box_minx, METH_NOARGS,
     "Return minx"},
    {"miny", (PyCFunction) Box_miny, METH_NOARGS,
     "Return minx"},
    {"maxx", (PyCFunction) Box_maxx, METH_NOARGS,
     "Return minx"},
    {"maxy", (PyCFunction) Box_maxy, METH_NOARGS,
     "Return minx"},
    {NULL}  /* Sentinel */
};

static PyTypeObject BoxType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pymapnik3.Box2d",
    .tp_doc = PyDoc_STR("Box objects"),
    .tp_basicsize = sizeof(MapnikBox2d),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) Box_init,
    .tp_dealloc = (destructor) Box_dealloc,
    .tp_members = Box_members,
    .tp_methods = Box_methods,    
};


// ===========================================================================
// COLOR

typedef struct {
    PyObject_HEAD
    mapnik::color* color;
} MapnikColor;

static void
Color_dealloc(MapnikColor *self)
{
    delete self->color;
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int
Color_init(MapnikColor *self, PyObject *args)
{
    int length = PySequence_Length(args);
    
    if (length == 1) {
        const char *colorspec;
        if (!PyArg_ParseTuple(args, "s", &colorspec))
            return -1;
        self->color = new mapnik::color(std::string(colorspec));
    } else if (length == 3 || length == 4) {
        unsigned char r, g, b, t = 0;
        if (!PyArg_ParseTuple(args, "bbb|b", &r, &g, &b, &t))
            return -1;
        self->color = new mapnik::color(r, g, b, t);
    }
    
    return 0;
}

static PyMemberDef Color_members[] = {
    {NULL}  /* Sentinel */
};

static PyObject *
Color_to_hex_string(MapnikColor *self, PyObject *Py_UNUSED(ignored))
{
    std::string hex = self->color->to_hex_string();
    const char* c_hex = hex.c_str();
    PyObject* obj = Py_BuildValue("s", c_hex);
    //delete c_hex; // I assume?
    return obj;
}

static PyMethodDef Color_methods[] = {
    {"to_hex_string", (PyCFunction) Color_to_hex_string, METH_NOARGS,
     "Return the color in hex format"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject ColorType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pymapnik3.Color",
    .tp_doc = PyDoc_STR("Color objects"),
    .tp_basicsize = sizeof(MapnikColor),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) Color_init,
    .tp_dealloc = (destructor) Color_dealloc,
    .tp_members = Color_members,
    .tp_methods = Color_methods,    
};


// ===========================================================================
// LINE SYMBOLIZER

typedef struct {
    PyObject_HEAD
    mapnik::line_symbolizer* symbolizer;
} MapnikLineSymbolizer;

static void
LineSymbolizer_dealloc(MapnikLineSymbolizer *self)
{
    delete self->symbolizer;
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int
LineSymbolizer_init(MapnikLineSymbolizer *self, PyObject *args)
{
    self->symbolizer = new mapnik::line_symbolizer();   
    return 0;
}

static PyMemberDef LineSymbolizer_members[] = {
    {NULL}  /* Sentinel */
};

static PyObject *
LineSymbolizer_set_stroke(MapnikLineSymbolizer *self, PyObject *color)
{
    if (!PyObject_IsInstance(color, (PyObject*) &ColorType)) {
        PyErr_SetString(PyExc_RuntimeError, "set_stroke requires a color object");
        return NULL;
    }
    
    MapnikColor* ourcolor = (MapnikColor*) color;
    self->symbolizer->properties.insert(std::pair<mapnik::keys, mapnik::color&>(mapnik::keys::stroke, *ourcolor->color));
    return Py_BuildValue("");
}

static PyObject *
LineSymbolizer_set_stroke_width(MapnikLineSymbolizer *self, PyObject *args)
{
    double width;
    if (!PyArg_ParseTuple(args, "d", &width))
        return NULL;
    
    self->symbolizer->properties.insert(std::pair<mapnik::keys, double>(mapnik::keys::stroke_width, width));
    return Py_BuildValue("");
}

static PyMethodDef LineSymbolizer_methods[] = {
    {"set_stroke", (PyCFunction) LineSymbolizer_set_stroke, METH_O,
     "Set stroke color"
    },
    {"set_stroke_width", (PyCFunction) LineSymbolizer_set_stroke_width, METH_VARARGS,
     "Set stroke width"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject LineSymbolizerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pymapnik3.LineSymbolizer",
    .tp_doc = PyDoc_STR("LineSymbolizer objects"),
    .tp_basicsize = sizeof(MapnikLineSymbolizer),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) LineSymbolizer_init,
    .tp_dealloc = (destructor) LineSymbolizer_dealloc,
    .tp_members = LineSymbolizer_members,
    .tp_methods = LineSymbolizer_methods,    
};


// ===========================================================================
// TEXT SYMBOLIZER

typedef struct {
    PyObject_HEAD
    mapnik::text_symbolizer* symbolizer;
    mapnik::text_placements_ptr placements;
} MapnikTextSymbolizer;

static void
TextSymbolizer_dealloc(MapnikTextSymbolizer *self)
{
    delete self->symbolizer;
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int
TextSymbolizer_init(MapnikTextSymbolizer *self, PyObject *args)
{
    self->symbolizer = new mapnik::text_symbolizer();
    self->placements = std::make_shared<mapnik::text_placements_dummy>();
    self->symbolizer->properties.insert(std::pair<mapnik::keys, mapnik::text_placements_ptr>(mapnik::keys::text_placements_, self->placements));
    return 0;
}

static PyMemberDef TextSymbolizer_members[] = {
    {NULL}  /* Sentinel */
};

static PyObject *
TextSymbolizer_set_fill(MapnikTextSymbolizer *self, PyObject *color)
{
    if (!PyObject_IsInstance(color, (PyObject*) &ColorType)) {
        PyErr_SetString(PyExc_RuntimeError, "set_fill requires a color object");
        return NULL;
    }
    
    MapnikColor* ourcolor = (MapnikColor*) color;
    self->placements->defaults.format_defaults.fill = *ourcolor->color;
    return Py_BuildValue("");
}

static PyObject *
TextSymbolizer_set_halo_fill(MapnikTextSymbolizer *self, PyObject *color)
{
    if (!PyObject_IsInstance(color, (PyObject*) &ColorType)) {
        PyErr_SetString(PyExc_RuntimeError, "set_halo_fill requires a color object");
        return NULL;
    }
    
    MapnikColor* ourcolor = (MapnikColor*) color;
    self->placements->defaults.format_defaults.halo_fill = *ourcolor->color;
    return Py_BuildValue("");
}

static PyObject *
TextSymbolizer_set_halo_radius(MapnikTextSymbolizer *self, PyObject *args)
{
    double radius;
    if (!PyArg_ParseTuple(args, "d", &radius))
        return NULL;
    
    self->placements->defaults.format_defaults.halo_radius = radius;
    return Py_BuildValue("");
}

static PyObject *
TextSymbolizer_set_text_size(MapnikTextSymbolizer *self, PyObject *args)
{
    double size;
    if (!PyArg_ParseTuple(args, "d", &size))
        return NULL;
    
    self->placements->defaults.format_defaults.text_size = size;
    return Py_BuildValue("");
}

static PyObject *
TextSymbolizer_set_face_name(MapnikTextSymbolizer *self, PyObject *args)
{
    char* name;
    if (!PyArg_ParseTuple(args, "s", &name))
        return NULL;
    
    self->placements->defaults.format_defaults.face_name = name;
    return Py_BuildValue("");
}

static PyObject *
TextSymbolizer_set_name_expression(MapnikTextSymbolizer *self, PyObject *args)
{
    char* expr;
    if (!PyArg_ParseTuple(args, "s", &expr))
        return NULL;
    
    self->placements->defaults.set_format_tree(std::make_shared<mapnik::formatting::text_node>(mapnik::parse_expression(expr)));    
    return Py_BuildValue("");
}

static PyMethodDef TextSymbolizer_methods[] = {
    {"set_face_name", (PyCFunction) TextSymbolizer_set_face_name, METH_VARARGS,
     "Set text font name"
    },
    {"set_fill", (PyCFunction) TextSymbolizer_set_fill, METH_O,
     "Set fill color"
    },
    {"set_halo_fill", (PyCFunction) TextSymbolizer_set_halo_fill, METH_O,
     "Set halo outline color"
    },
    {"set_halo_radius", (PyCFunction) TextSymbolizer_set_halo_radius, METH_VARARGS,
     "Set halo outline width"
    },
    {"set_name_expression", (PyCFunction) TextSymbolizer_set_name_expression, METH_VARARGS,
     "Set expression to compute name of feature"
    },
    {"set_text_size", (PyCFunction) TextSymbolizer_set_text_size, METH_VARARGS,
     "Set text size"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject TextSymbolizerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pymapnik3.TextSymbolizer",
    .tp_doc = PyDoc_STR("TextSymbolizer objects"),
    .tp_basicsize = sizeof(MapnikTextSymbolizer),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) TextSymbolizer_init,
    .tp_dealloc = (destructor) TextSymbolizer_dealloc,
    .tp_members = TextSymbolizer_members,
    .tp_methods = TextSymbolizer_methods,    
};



// ===========================================================================
// CONTEXT

typedef struct {
    PyObject_HEAD
    mapnik::context_ptr context;
} MapnikContext;

static void
Context_dealloc(MapnikContext *self)
{
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int
Context_init(MapnikContext *self, PyObject *args)
{    
    self->context = std::make_shared<mapnik::context_type>();
    return 0;
}

static PyMemberDef Context_members[] = {
    {NULL}  /* Sentinel */
};

static PyMethodDef Context_methods[] = {
    {NULL}  /* Sentinel */
};

static PyTypeObject ContextType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pymapnik3.Context",
    .tp_doc = PyDoc_STR("Context objects"),
    .tp_basicsize = sizeof(MapnikContext),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) Context_init,
    .tp_dealloc = (destructor) Context_dealloc,
    .tp_members = Context_members,
    .tp_methods = Context_methods,    
};


// ===========================================================================
// EXPRESSION

typedef struct {
    PyObject_HEAD
    std::shared_ptr<mapnik::expr_node> expression;
} MapnikExpression;

static void
Expression_dealloc(MapnikExpression *self)
{
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int
Expression_init(MapnikExpression *self, PyObject *args)
{
    char* c_expr;
    if (!PyArg_ParseTuple(args, "s", &c_expr))
        return -1;
    
    self->expression = mapnik::parse_expression(std::string(c_expr));
    return 0;
}

static PyMemberDef Expression_members[] = {
    {NULL}  /* Sentinel */
};

static PyMethodDef Expression_methods[] = {
    {NULL}  /* Sentinel */
};

static PyTypeObject ExpressionType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pymapnik3.Expression",
    .tp_doc = PyDoc_STR("Expression objects"),
    .tp_basicsize = sizeof(MapnikExpression),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) Expression_init,
    .tp_dealloc = (destructor) Expression_dealloc,
    .tp_members = Expression_members,
    .tp_methods = Expression_methods,    
};


// ===========================================================================
// FEATURE

typedef struct {
    PyObject_HEAD
    mapnik::feature_ptr feature;
} MapnikFeature;

static void
Feature_dealloc(MapnikFeature *self)
{
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int
Feature_init(MapnikFeature *self, PyObject *args)
{
    // make an empty pointer
    self->feature = std::shared_ptr<mapnik::feature_impl>(); 
    return 0;
}

static PyMemberDef Feature_members[] = {
    {NULL}  /* Sentinel */
};

static PyMethodDef Feature_methods[] = {
    {NULL}  /* Sentinel */
};

static PyTypeObject FeatureType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pymapnik3.Feature",
    .tp_doc = PyDoc_STR("Feature objects"),
    .tp_basicsize = sizeof(MapnikFeature),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) Feature_init,
    .tp_dealloc = (destructor) Feature_dealloc,
    .tp_members = Feature_members,
    .tp_methods = Feature_methods,    
};


// ===========================================================================
// GDAL

typedef struct {
    PyObject_HEAD
    std::shared_ptr<mapnik::datasource> source;
} MapnikGdal;

static void
Gdal_dealloc(MapnikGdal *self)
{
    //delete self->source;
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int
Gdal_init(MapnikGdal *self, PyObject *args, PyObject *kwargs)
{
    char *base = NULL, *file = NULL;
    int band = -1;

    static char *kwlist[] = {"base", "file", "band", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|ssi", kwlist, 
                                     &base, &file, &band))
    {
        return -1;
    }    
    
    mapnik::parameters params;
    params[std::string("type")] = std::string("gdal");
    if (base != NULL)
        params[std::string("base")] = std::string(base);
    if (file != NULL)
        params[std::string("file")] = std::string(file);
    if (band != -1)
        params[std::string("band")] = (long long) band;
    
    self->source = mapnik::datasource_cache::instance().create(params);
    return 0;
}

static PyMemberDef Gdal_members[] = {
    {NULL}  /* Sentinel */
};

static PyMethodDef Gdal_methods[] = {
    {NULL}  /* Sentinel */
};

static PyTypeObject GdalType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pymapnik3.Gdal",
    .tp_doc = PyDoc_STR("Gdal objects"),
    .tp_basicsize = sizeof(MapnikGdal),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) Gdal_init,
    .tp_dealloc = (destructor) Gdal_dealloc,
    .tp_members = Gdal_members,
    .tp_methods = Gdal_methods,    
};


// ===========================================================================
// GEOJSON

typedef struct {
    PyObject_HEAD
    std::shared_ptr<mapnik::datasource> source;
} MapnikGeoJson;

static void
GeoJson_dealloc(MapnikGeoJson *self)
{
    //delete self->source;
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int
GeoJson_init(MapnikGeoJson *self, PyObject *args)
{
    char *filename;
    if (!PyArg_ParseTuple(args, "s", &filename))
        return -1;
    
    mapnik::parameters params;
    params[std::string("file")] = std::string(filename);
    params[std::string("type")] = std::string("geojson");
    
    self->source = mapnik::datasource_cache::instance().create(params);
    return 0;
}

static PyMemberDef GeoJson_members[] = {
    {NULL}  /* Sentinel */
};

static PyMethodDef GeoJson_methods[] = {
    {NULL}  /* Sentinel */
};

static PyTypeObject GeoJsonType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pymapnik3.GeoJSON",
    .tp_doc = PyDoc_STR("GeoJSON objects"),
    .tp_basicsize = sizeof(MapnikGeoJson),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) GeoJson_init,
    .tp_dealloc = (destructor) GeoJson_dealloc,
    .tp_members = GeoJson_members,
    .tp_methods = GeoJson_methods,    
};


// ===========================================================================
// POLYGON SYMBOLIZER

typedef struct {
    PyObject_HEAD
    mapnik::polygon_symbolizer *symbolizer;
} MapnikPolygonSymbolizer;

static void
PolygonSymbolizer_dealloc(MapnikPolygonSymbolizer *self)
{
    delete self->symbolizer;
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int
PolygonSymbolizer_init(MapnikPolygonSymbolizer *self, PyObject *args)
{
    self->symbolizer = new mapnik::polygon_symbolizer();
    return 0;
}

static PyMemberDef PolygonSymbolizer_members[] = {
    {NULL}  /* Sentinel */
};

static PyObject *
PolygonSymbolizer_set_fill(MapnikPolygonSymbolizer *self, PyObject *color)
{
    if (!PyObject_IsInstance(color, (PyObject*) &ColorType)) {
        PyErr_SetString(PyExc_RuntimeError, "set_fill requires a color object");
        return NULL;
    }
    
    MapnikColor* ourcolor = (MapnikColor*) color;
    self->symbolizer->properties.insert(std::pair<mapnik::keys, mapnik::color&>(mapnik::keys::fill, *ourcolor->color));
    return Py_BuildValue("");
}

static PyObject *
PolygonSymbolizer_set_fill_opacity(MapnikPolygonSymbolizer *self, PyObject *args)
{
    double opacity;
    if (!PyArg_ParseTuple(args, "d", &opacity))
        return NULL;
    
    self->symbolizer->properties.insert(std::pair<mapnik::keys, double>(mapnik::keys::fill_opacity, opacity));
    return Py_BuildValue("");
}

static PyMethodDef PolygonSymbolizer_methods[] = {
    {"set_fill", (PyCFunction) PolygonSymbolizer_set_fill, METH_O,
     "Set fill color"
    },
    {"set_fill_opacity", (PyCFunction) PolygonSymbolizer_set_fill_opacity, METH_VARARGS,
     "Set fill opacity (0.0-1.0)"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject PolygonSymbolizerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pymapnik3.PolygonSymbolizer",
    .tp_doc = PyDoc_STR("PolygonSymbolizer objects"),
    .tp_basicsize = sizeof(MapnikPolygonSymbolizer),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) PolygonSymbolizer_init,
    .tp_dealloc = (destructor) PolygonSymbolizer_dealloc,
    .tp_members = PolygonSymbolizer_members,
    .tp_methods = PolygonSymbolizer_methods,    
};


// ===========================================================================
// POINT SYMBOLIZER

typedef struct {
    PyObject_HEAD
    mapnik::point_symbolizer *symbolizer;
} MapnikPointSymbolizer;

static void
PointSymbolizer_dealloc(MapnikPointSymbolizer *self)
{
    delete self->symbolizer;
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int
PointSymbolizer_init(MapnikPointSymbolizer *self, PyObject *args)
{
    self->symbolizer = new mapnik::point_symbolizer();
    return 0;
}

static PyMemberDef PointSymbolizer_members[] = {
    {NULL}  /* Sentinel */
};

static PyObject *
PointSymbolizer_set_file(MapnikPointSymbolizer *self, PyObject *args)
{
    char* c_file;
    if (!PyArg_ParseTuple(args, "s", &c_file))
        return NULL;
    
    self->symbolizer->properties.insert(std::pair<mapnik::keys, std::string>(mapnik::keys::file, std::string(c_file)));
    return Py_BuildValue("");
}

static PyObject *
PointSymbolizer_set_allow_overlap(MapnikPointSymbolizer *self, PyObject *args)
{
    int v;
    if (!PyArg_ParseTuple(args, "p", &v))
        return NULL;

    bool cppv = v == 1;
    self->symbolizer->properties.insert(std::pair<mapnik::keys, bool>(mapnik::keys::allow_overlap, cppv));
    return Py_BuildValue("");
}

static PyObject *
PointSymbolizer_set_ignore_placement(MapnikPointSymbolizer *self, PyObject *args)
{
    int v;
    if (!PyArg_ParseTuple(args, "p", &v))
        return NULL;

    bool cppv = v == 1;
    self->symbolizer->properties.insert(std::pair<mapnik::keys, bool>(mapnik::keys::ignore_placement, cppv));
    return Py_BuildValue("");
}

static PyMethodDef PointSymbolizer_methods[] = {
    {"set_file", (PyCFunction) PointSymbolizer_set_file, METH_VARARGS,
     "Set SVG icon file"
    },
    {"set_allow_overlap", (PyCFunction) PointSymbolizer_set_allow_overlap, METH_VARARGS,
     "Set whether to allow overlap"
    },
    {"set_ignore_placement", (PyCFunction) PointSymbolizer_set_ignore_placement, METH_VARARGS,
     "Set whether to ignore placement"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject PointSymbolizerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pymapnik3.PointSymbolizer",
    .tp_doc = PyDoc_STR("PointSymbolizer objects"),
    .tp_basicsize = sizeof(MapnikPointSymbolizer),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) PointSymbolizer_init,
    .tp_dealloc = (destructor) PointSymbolizer_dealloc,
    .tp_members = PointSymbolizer_members,
    .tp_methods = PointSymbolizer_methods,    
};


// ===========================================================================
// PROJECTION

typedef struct {
    PyObject_HEAD
    mapnik::projection *projection;
} MapnikProjection;

static void
Projection_dealloc(MapnikProjection *self)
{
    delete self->projection;
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int
Projection_init(MapnikProjection *self, PyObject *args)
{
    const char *params;
    if (!PyArg_ParseTuple(args, "s", &params))
        return -1;

    self->projection = new mapnik::projection(std::string(params), false);
   
    return 0;
}

static PyMemberDef Projection_members[] = {
    {NULL}  /* Sentinel */
};

static PyObject *
Projection_expanded(MapnikProjection *self, PyObject *Py_UNUSED(ignored))
{
    std::string hex = self->projection->expanded();
    const char* c_hex = hex.c_str();
    PyObject* obj = Py_BuildValue("s", c_hex);
    //delete c_hex; // I assume?
    return obj;
}

static PyMethodDef Projection_methods[] = {
    {"expanded", (PyCFunction) Projection_expanded, METH_NOARGS,
     "Return expanded name of projection"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject ProjectionType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pymapnik3.Projection",
    .tp_doc = PyDoc_STR("Projection objects"),
    .tp_basicsize = sizeof(MapnikProjection),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) Projection_init,
    .tp_dealloc = (destructor) Projection_dealloc,
    .tp_members = Projection_members,
    .tp_methods = Projection_methods,    
};


// ===========================================================================
// PROJ_TRANSFORM

typedef struct {
    PyObject_HEAD
    mapnik::proj_transform *proj_transform;
} MapnikProjTransform;

static void
ProjTransform_dealloc(MapnikProjTransform *self)
{
    delete self->proj_transform;
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int
ProjTransform_init(MapnikProjTransform *self, PyObject *args)
{
    MapnikProjection *source, *dest;
    if (!PyArg_ParseTuple(args, "OO", &source, &dest))
        return -1;

    self->proj_transform = new mapnik::proj_transform(
        *source->projection,
        *dest->projection
    );
    
    return 0;
}

static PyMemberDef ProjTransform_members[] = {
    {NULL}  /* Sentinel */
};

static PyObject *
ProjTransform_forward(MapnikProjTransform *self, PyObject *box2d)
{
    if (!PyObject_IsInstance(box2d, (PyObject*) &BoxType)) {
        PyErr_SetString(PyExc_RuntimeError, "forward requires a box object");
        return NULL;
    }
    
    MapnikBox2d* ourbox2d = (MapnikBox2d*) box2d;
    mapnik::box2d<double> *box = ourbox2d->box;

    // the .forward() method is destructive of the input, so we make a
    // copy to work on that we'll return afterwards
    mapnik::box2d<double> copy = mapnik::box2d<double>(box->minx(), box->miny(), box->maxx(), box->maxy());
    if (!self->proj_transform->forward(copy)) {
        std::cout << "OUCH OUCH OUCH" << std::endl; // FIXME
    }

    PyObject* args = Py_BuildValue("(dddd)", copy.minx(), copy.miny(), copy.maxx(), copy.maxy());    

    return PyObject_CallObject((PyObject *)&BoxType, args);
}

static PyMethodDef ProjTransform_methods[] = {
    {"forward", (PyCFunction) ProjTransform_forward, METH_O,
     "Run the transform forwards and return a box2d object"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject ProjTransformType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pymapnik3.ProjTransform",
    .tp_doc = PyDoc_STR("ProjTransform objects"),
    .tp_basicsize = sizeof(MapnikProjTransform),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) ProjTransform_init,
    .tp_dealloc = (destructor) ProjTransform_dealloc,
    .tp_members = ProjTransform_members,
    .tp_methods = ProjTransform_methods,    
};


// ===========================================================================
// RASTER COLORIZER

typedef struct {
    PyObject_HEAD
    mapnik::raster_colorizer *colorizer;
} MapnikRasterColorizer;

static void
RasterColorizer_dealloc(MapnikRasterColorizer *self)
{
    delete self->colorizer;
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static mapnik::colorizer_mode_enum int_to_colorizer_mode(int mode)
{
    switch(mode)
    {
    case 0:
        return mapnik::COLORIZER_INHERIT;
    case 1:
        return mapnik::COLORIZER_LINEAR;
    case 2:
        return mapnik::COLORIZER_DISCRETE;
    case 3:
        return mapnik::COLORIZER_EXACT;
    default:
        return mapnik::colorizer_mode_enum_MAX; // signal error
    }
}

static int
RasterColorizer_init(MapnikRasterColorizer *self, PyObject *args)
{
    int mode;
    PyObject *color;
    if (!PyArg_ParseTuple(args, "iO", &mode, &color))
        return -1;

    mapnik::colorizer_mode_enum mmode = int_to_colorizer_mode(mode);
    
    if (!PyObject_IsInstance(color, (PyObject*) &ColorType)) {
        PyErr_SetString(PyExc_RuntimeError, "second argument must be a color object");
        return -1;
    }
    MapnikColor *mcolor = (MapnikColor*) color;
    
    self->colorizer = new mapnik::raster_colorizer(mmode, *mcolor->color);
    return 0;
}

static PyMemberDef RasterColorizer_members[] = {
    {NULL}  /* Sentinel */
};

static PyObject *
RasterColorizer_add_stop(MapnikRasterColorizer *self, PyObject *args)
{
    double limit;
    PyObject *color;
    if (!PyArg_ParseTuple(args, "dO", &limit, &color))
        return NULL;
   
    if (!PyObject_IsInstance(color, (PyObject*) &ColorType)) {
        PyErr_SetString(PyExc_RuntimeError, "second argument must be a color object");
        return NULL;
    }
    MapnikColor *mcolor = (MapnikColor*) color;

    self->colorizer->add_stop(mapnik::colorizer_stop(limit, mapnik::COLORIZER_INHERIT, *mcolor->color));
    
    return Py_BuildValue("");
}

static PyMethodDef RasterColorizer_methods[] = {
    {"add_stop", (PyCFunction) RasterColorizer_add_stop, METH_VARARGS,
     "Add a coloring step with limit value and color"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject RasterColorizerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pymapnik3.RasterColorizer",
    .tp_doc = PyDoc_STR("RasterColorizer objects"),
    .tp_basicsize = sizeof(MapnikRasterColorizer),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) RasterColorizer_init,
    .tp_dealloc = (destructor) RasterColorizer_dealloc,
    .tp_members = RasterColorizer_members,
    .tp_methods = RasterColorizer_methods,    
};


// ===========================================================================
// RASTER SYMBOLIZER

typedef struct {
    PyObject_HEAD
    mapnik::raster_symbolizer *symbolizer;
} MapnikRasterSymbolizer;

static void
RasterSymbolizer_dealloc(MapnikRasterSymbolizer *self)
{
    delete self->symbolizer;
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int
RasterSymbolizer_init(MapnikRasterSymbolizer *self, PyObject *args)
{
    self->symbolizer = new mapnik::raster_symbolizer();
    return 0;
}

static PyMemberDef RasterSymbolizer_members[] = {
    {NULL}  /* Sentinel */
};

static PyObject *
RasterSymbolizer_set_colorizer(MapnikRasterSymbolizer *self, PyObject *arg)
{
    if (!PyObject_IsInstance(arg, (PyObject*) &RasterColorizerType)) {
        PyErr_SetString(PyExc_RuntimeError, "set_colorizer requires a RasterColorizer object");
        return NULL;
    }

    MapnikRasterColorizer* colorizer = (MapnikRasterColorizer*) arg;
    mapnik::raster_colorizer_ptr wrapped = std::make_shared<mapnik::raster_colorizer>();
    // looks like we have to manually copy every aspect of the colorizer :-(
    wrapped->set_default_mode(colorizer->colorizer->get_default_mode());
    wrapped->set_default_color(colorizer->colorizer->get_default_color());

    mapnik::colorizer_stops stops = colorizer->colorizer->get_stops();
    for (auto i = stops.begin(); i != stops.end(); ++i)
        wrapped->add_stop(*i);

    // FIXME: the problem with this is that later changes to the colorizer
    //        will not be copied over ...
    //
    // could we solve this by making the colorizer object refer to
    // this new wrapped thing? not a full solution, unfortunately
    self->symbolizer->properties.insert(std::pair<mapnik::keys, mapnik::raster_colorizer_ptr>(mapnik::keys::colorizer, wrapped));
    
    return Py_BuildValue("");
}

static PyMethodDef RasterSymbolizer_methods[] = {
    {"set_colorizer", (PyCFunction) RasterSymbolizer_set_colorizer, METH_O,
     "Set the colorizer"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject RasterSymbolizerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pymapnik3.RasterSymbolizer",
    .tp_doc = PyDoc_STR("RasterSymbolizer objects"),
    .tp_basicsize = sizeof(MapnikRasterSymbolizer),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) RasterSymbolizer_init,
    .tp_dealloc = (destructor) RasterSymbolizer_dealloc,
    .tp_members = RasterSymbolizer_members,
    .tp_methods = RasterSymbolizer_methods,    
};


// ===========================================================================
// RULE

typedef struct {
    PyObject_HEAD
    mapnik::rule *rule;
} MapnikRule;

static void
Rule_dealloc(MapnikRule *self)
{
    delete self->rule;
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int
Rule_init(MapnikRule *self, PyObject *args)
{
    self->rule = new mapnik::rule();
    return 0;
}

static PyMemberDef Rule_members[] = {
    {NULL}  /* Sentinel */
};

static PyObject *
Rule_add_symbolizer(MapnikRule *self, PyObject *symbolizer)
{
    if (PyObject_IsInstance(symbolizer, (PyObject*) &PolygonSymbolizerType)) {
        MapnikPolygonSymbolizer* oursymb = (MapnikPolygonSymbolizer*) symbolizer;
        self->rule->append(*oursymb->symbolizer);
    } else if (PyObject_IsInstance(symbolizer, (PyObject*) &LineSymbolizerType)) {
        MapnikLineSymbolizer* oursymb = (MapnikLineSymbolizer*) symbolizer;
        self->rule->append(*oursymb->symbolizer);
    } else if (PyObject_IsInstance(symbolizer, (PyObject*) &PointSymbolizerType)) {
        MapnikPointSymbolizer* oursymb = (MapnikPointSymbolizer*) symbolizer;
        self->rule->append(*oursymb->symbolizer);
    } else if (PyObject_IsInstance(symbolizer, (PyObject*) &RasterSymbolizerType)) {
        MapnikRasterSymbolizer* oursymb = (MapnikRasterSymbolizer*) symbolizer;
        self->rule->append(*oursymb->symbolizer);
    } else if (PyObject_IsInstance(symbolizer, (PyObject*) &TextSymbolizerType)) {
        MapnikTextSymbolizer* oursymb = (MapnikTextSymbolizer*) symbolizer;
        self->rule->append(*oursymb->symbolizer);
    } else {
        PyErr_SetString(PyExc_RuntimeError, "add_symbolizer requires a symbolizer object");
        return NULL;
    }
    return Py_BuildValue("");    
}

static PyObject *
Rule_set_filter(MapnikRule *self, PyObject *arg)
{
    if (!PyObject_IsInstance(arg, (PyObject*) &ExpressionType)) {
        PyErr_SetString(PyExc_RuntimeError, "set_filter requires an expression object");
        return NULL;
    }

    MapnikExpression* expr = (MapnikExpression*) arg;
    self->rule->set_filter(expr->expression);
    
    return Py_BuildValue("");
}

static PyMethodDef Rule_methods[] = {
    {"add_symbolizer", (PyCFunction) Rule_add_symbolizer, METH_O,
     "Add a symbolizer to the rule"
    },
    {"set_filter", (PyCFunction) Rule_set_filter, METH_O,
     "Set a filter on the rule"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject RuleType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pymapnik3.Rule",
    .tp_doc = PyDoc_STR("Rule objects"),
    .tp_basicsize = sizeof(MapnikRule),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) Rule_init,
    .tp_dealloc = (destructor) Rule_dealloc,
    .tp_members = Rule_members,
    .tp_methods = Rule_methods,    
};


// ===========================================================================
// SHAPEFILE

typedef struct {
    PyObject_HEAD
    std::shared_ptr<mapnik::datasource> source;
} MapnikShapefile;

static void
Shapefile_dealloc(MapnikShapefile *self)
{
    //delete self->source;
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int
Shapefile_init(MapnikShapefile *self, PyObject *args)
{
    char *filename;
    if (!PyArg_ParseTuple(args, "s", &filename))
        return -1;
    
    mapnik::parameters params;
    params[std::string("file")] = std::string(filename);
    params[std::string("type")] = std::string("shape");
    
    self->source = mapnik::datasource_cache::instance().create(params);
    return 0;
}

static PyMemberDef Shapefile_members[] = {
    {NULL}  /* Sentinel */
};

static PyMethodDef Shapefile_methods[] = {
    {NULL}  /* Sentinel */
};

static PyTypeObject ShapefileType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pymapnik3.Shapefile",
    .tp_doc = PyDoc_STR("Shapefile objects"),
    .tp_basicsize = sizeof(MapnikShapefile),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) Shapefile_init,
    .tp_dealloc = (destructor) Shapefile_dealloc,
    .tp_members = Shapefile_members,
    .tp_methods = Shapefile_methods,    
};


// ===========================================================================
// STYLE

typedef struct {
    PyObject_HEAD
    mapnik::feature_type_style *style;
} MapnikStyle;

static void
Style_dealloc(MapnikStyle *self)
{
    delete self->style;
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int
Style_init(MapnikStyle *self, PyObject *args)
{
    self->style = new mapnik::feature_type_style();
    return 0;
}

static PyMemberDef Style_members[] = {
    {NULL}  /* Sentinel */
};

static PyObject *
Style_add_rule(MapnikStyle *self, PyObject *rule)
{
    if (!PyObject_IsInstance(rule, (PyObject*) &RuleType)) {
        PyErr_SetString(PyExc_RuntimeError, "add_rule requires a rule object");
        return NULL;
    }
    
    MapnikRule* ourrule = (MapnikRule*) rule;
    self->style->add_rule(mapnik::rule(*ourrule->rule));
    return Py_BuildValue("");
}

static PyMethodDef Style_methods[] = {
    {"add_rule", (PyCFunction) Style_add_rule, METH_O,
     "Add a rule to the style"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject StyleType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pymapnik3.Style",
    .tp_doc = PyDoc_STR("Style objects"),
    .tp_basicsize = sizeof(MapnikStyle),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) Style_init,
    .tp_dealloc = (destructor) Style_dealloc,
    .tp_members = Style_members,
    .tp_methods = Style_methods,    
};


// ===========================================================================
// MEMORY DATASOURCE

typedef struct {
    PyObject_HEAD
    std::shared_ptr<mapnik::memory_datasource> source;
} MapnikMemoryDatasource;

static void
MemoryDatasource_dealloc(MapnikMemoryDatasource *self)
{
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int
MemoryDatasource_init(MapnikMemoryDatasource *self, PyObject *args)
{
    mapnik::parameters params;
    params[std::string("type")] = std::string("memory");
    
    self->source = std::make_shared<mapnik::memory_datasource>(params);
    return 0;
}

static PyMemberDef MemoryDatasource_members[] = {
    {NULL}  /* Sentinel */
};

static PyObject *
MemoryDatasource_add_feature(MapnikMemoryDatasource *self, PyObject* args)
{
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "O", &obj))
        return NULL;

    if (!PyObject_IsInstance(obj, (PyObject*) &FeatureType)) {
        PyErr_SetString(PyExc_RuntimeError, "add_feature requires a feature object");
        return NULL;
    }
    
    MapnikFeature *feature = (MapnikFeature*) obj;
    self->source->push(feature->feature);
    return Py_BuildValue("");
}

static PyMethodDef MemoryDatasource_methods[] = {
    {"add_feature", (PyCFunction) MemoryDatasource_add_feature, METH_VARARGS,
     "Add a feature to the data source"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject MemoryDatasourceType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pymapnik3.MemoryDatasource",
    .tp_doc = PyDoc_STR("MemoryDatasource objects"),
    .tp_basicsize = sizeof(MapnikMemoryDatasource),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) MemoryDatasource_init,
    .tp_dealloc = (destructor) MemoryDatasource_dealloc,
    .tp_members = MemoryDatasource_members,
    .tp_methods = MemoryDatasource_methods,    
};


// ===========================================================================
// LAYER

typedef struct {
    PyObject_HEAD
    mapnik::layer* layer;
} MapnikLayer;

static void
Layer_dealloc(MapnikLayer *self)
{
    delete self->layer;
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int
Layer_init(MapnikLayer *self, PyObject *args)
{
    const char *name;
    if (!PyArg_ParseTuple(args, "s", &name))
        return -1;

    self->layer = new mapnik::layer(std::string(name));
    
    return 0;
}

static PyMemberDef Layer_members[] = {
    {NULL}  /* Sentinel */
};

static PyObject *
Layer_add_style(MapnikLayer *self, PyObject* args)
{
    char *style;
    if (!PyArg_ParseTuple(args, "s", &style))
        return NULL;

    self->layer->add_style(std::string(style));
    return Py_BuildValue("");
}

static PyObject *
Layer_set_datasource(MapnikLayer *self, PyObject *arg)
{
    if (PyObject_IsInstance(arg, (PyObject*) &ShapefileType)) {
        MapnikShapefile* shapefile = (MapnikShapefile*) arg;
        self->layer->set_datasource(shapefile->source);
    } else if (PyObject_IsInstance(arg, (PyObject*) &MemoryDatasourceType)) {
        MapnikMemoryDatasource* memory = (MapnikMemoryDatasource*) arg;
        self->layer->set_datasource(memory->source);
    } else if (PyObject_IsInstance(arg, (PyObject*) &GdalType)) {
        MapnikGdal* gdal = (MapnikGdal*) arg;
        self->layer->set_datasource(gdal->source);
    } else if (PyObject_IsInstance(arg, (PyObject*) &GeoJsonType)) {
        MapnikGeoJson* geojson = (MapnikGeoJson*) arg;
        self->layer->set_datasource(geojson->source);
    } else {
        PyErr_SetString(PyExc_RuntimeError, "set_datasource requires a datasource object");
        return NULL;
    }
    return Py_BuildValue("");
}

static PyObject *
Layer_get_srs(MapnikLayer *self, PyObject *Py_UNUSED(ignored))
{
    const char* c_srs = self->layer->srs().c_str();
    return Py_BuildValue("s", c_srs);
}

static PyObject *
Layer_set_clear_label_cache(MapnikLayer *self, PyObject* args)
{
    int flag;
    if (!PyArg_ParseTuple(args, "p", &flag))
        return NULL;

    self->layer->set_clear_label_cache(flag == 1);
    return Py_BuildValue("");
}

static PyObject *
Layer_set_srs(MapnikLayer *self, PyObject* args)
{
    char *c_srs;
    if (!PyArg_ParseTuple(args, "s", &c_srs))
        return NULL;

    self->layer->set_srs(std::string(c_srs));
    return Py_BuildValue("");
}

static PyMethodDef Layer_methods[] = {
    {"add_style", (PyCFunction) Layer_add_style, METH_VARARGS,
     "Add style by reference"
    },
    {"get_srs", (PyCFunction) Layer_get_srs, METH_NOARGS,
     "Get projection"
    },
    {"set_datasource", (PyCFunction) Layer_set_datasource, METH_O,
     "Add underlying data source"
    },
    {"set_clear_label_cache", (PyCFunction) Layer_set_clear_label_cache, METH_VARARGS,
     "Sets bool flag clear label cache"
    },
    {"set_srs", (PyCFunction) Layer_set_srs, METH_VARARGS,
     "Set projection"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject LayerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pymapnik3.Layer",
    .tp_doc = PyDoc_STR("Layer objects"),
    .tp_basicsize = sizeof(MapnikLayer),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) Layer_init,
    .tp_dealloc = (destructor) Layer_dealloc,
    .tp_members = Layer_members,
    .tp_methods = Layer_methods,    
};


// ===========================================================================
// MAP

typedef struct {
    PyObject_HEAD
    mapnik::Map* map;
} MapnikMap;

static void
Map_dealloc(MapnikMap *self)
{
    delete self->map;
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int
Map_init(MapnikMap *self, PyObject *args)
{
    int width, height;
    if (!PyArg_ParseTuple(args, "ii", &width, &height))
        return -1;

    self->map = new mapnik::Map(width, height);
    
    return 0;
}

static PyMemberDef Map_members[] = {
    {NULL}  /* Sentinel */
};

static PyObject *
Map_add_layer(MapnikMap *self, PyObject* args)
{
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "O", &obj))
        return NULL;

    if (!PyObject_IsInstance(obj, (PyObject*) &LayerType)) {
        PyErr_SetString(PyExc_RuntimeError, "add_layer requires a layer object");
        return NULL;
    }
    
    MapnikLayer *layer = (MapnikLayer *) obj;
    self->map->add_layer(*layer->layer);
    return Py_BuildValue("");
}

static PyObject *
Map_add_style(MapnikMap *self, PyObject* args)
{
    char *c_name;
    MapnikStyle *style;
    if (!PyArg_ParseTuple(args, "sO", &c_name, &style))
        return NULL;

    if (!PyObject_IsInstance((PyObject*) style, (PyObject*) &StyleType)) {
        PyErr_SetString(PyExc_RuntimeError, "add_style requires a style object");
        return NULL;
    }
    
    self->map->insert_style(std::string(c_name), *style->style);
    return Py_BuildValue("");
}

static PyObject *
Map_set_background(MapnikMap *self, PyObject *color)
{
    if (!PyObject_IsInstance(color, (PyObject*) &ColorType)) {
        PyErr_SetString(PyExc_RuntimeError, "set_background requires a color object");
        return NULL;
    }
    
    MapnikColor* ourcolor = (MapnikColor*) color;
    self->map->set_background(*ourcolor->color);
    return Py_BuildValue("");
}

static PyObject *
Map_get_srs(MapnikMap *self, PyObject *Py_UNUSED(ignored))
{
    const char* c_srs = self->map->srs().c_str();
    return Py_BuildValue("s", c_srs);
}

static PyObject *
Map_set_srs(MapnikMap *self, PyObject* args)
{
    char *c_srs;
    if (!PyArg_ParseTuple(args, "s", &c_srs))
        return NULL;

    self->map->set_srs(std::string(c_srs));
    return Py_BuildValue("");
}

static PyObject *
Map_zoom_to_box(MapnikMap *self, PyObject *box)
{
    if (!PyObject_IsInstance(box, (PyObject*) &BoxType)) {
        PyErr_SetString(PyExc_RuntimeError, "zoom_to_box requires a box object");
        return NULL;
    }
    
    MapnikBox2d* ourbox = (MapnikBox2d*) box;
    self->map->zoom_to_box(*ourbox->box);
    return Py_BuildValue("");
}

static PyMethodDef Map_methods[] = {
    {"add_layer", (PyCFunction) Map_add_layer, METH_VARARGS,
     "Add a layer to the map"
    },
    {"add_style", (PyCFunction) Map_add_style, METH_VARARGS,
     "Add a named style to the map"
    },
    {"get_srs", (PyCFunction) Map_get_srs, METH_NOARGS,
     "Return the map's projection"
    },
    {"set_background", (PyCFunction) Map_set_background, METH_O,
     "Set background color for the map"
    },
    {"set_srs", (PyCFunction) Map_set_srs, METH_VARARGS,
     "Set map projection"
    },
    {"zoom_to_box", (PyCFunction) Map_zoom_to_box, METH_O,
     "Zoom the map to show the specified rectangle"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject MapType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pymapnik3.Map",
    .tp_doc = PyDoc_STR("Map objects"),
    .tp_basicsize = sizeof(MapnikMap),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) Map_init,
    .tp_dealloc = (destructor) Map_dealloc,
    .tp_members = Map_members,
    .tp_methods = Map_methods,    
};


// ===========================================================================
// FUNCTIONS

struct agg_renderer_visitor_1
{
    agg_renderer_visitor_1(mapnik::Map const& m, double scale_factor, unsigned offset_x, unsigned offset_y)
        : m_(m), scale_factor_(scale_factor), offset_x_(offset_x), offset_y_(offset_y) {}

    template <typename T>
    void operator() (T & pixmap)
    {
        throw std::runtime_error("This image type is not currently supported for rendering.");
    }

  private:
    mapnik::Map const& m_;
    double scale_factor_;
    unsigned offset_x_;
    unsigned offset_y_;
};

template <>
void agg_renderer_visitor_1::operator()<mapnik::image_rgba8> (mapnik::image_rgba8 & pixmap)
{
    mapnik::agg_renderer<mapnik::image_rgba8> ren(m_, pixmap, scale_factor_, offset_x_, offset_y_);
    ren.apply();
}

void render(mapnik::Map const& map,
            mapnik::image_any& image,
            double scale_factor = 1.0,
            unsigned offset_x = 0u,
            unsigned offset_y = 0u)
{
    mapnik::util::apply_visitor(agg_renderer_visitor_1(map, scale_factor, offset_x, offset_y), image);
}

static PyObject *
mapnik_parse_from_geojson(PyObject *self, PyObject *args)
{
    char* c_json;
    PyObject* c_ctx;
    if (!PyArg_ParseTuple(args, "sO", &c_json, &c_ctx))
        return NULL;

    if (!PyObject_IsInstance(c_ctx, (PyObject*) &ContextType)) {
        PyErr_SetString(PyExc_RuntimeError, "second argument to from_geojson must be context object");
        return NULL;
    }

    MapnikContext* ctx = (MapnikContext*) c_ctx;
    mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx->context, 1));
    if (!mapnik::json::from_geojson(std::string(c_json), *feature))
    {
        throw std::runtime_error("Failed to parse geojson feature");
    }

    PyObject *argList = Py_BuildValue("()");
    MapnikFeature* featureobj = (MapnikFeature*) PyObject_CallObject((PyObject *) &FeatureType, argList);
    Py_DECREF(argList);
    
    featureobj->feature = feature;
    PyObject* pyfeature = (PyObject*) featureobj;
    return Py_BuildValue("O", pyfeature);
}

static PyObject *
mapnik_register_datasources(PyObject *self, PyObject *args)
{
    const char *path;
    if (!PyArg_ParseTuple(args, "s", &path))
        return NULL;

    mapnik::datasource_cache::instance().register_datasources(std::string(path));
    return Py_BuildValue("");
}

static PyObject *
mapnik_register_font(PyObject *self, PyObject *args)
{
    const char *path;
    if (!PyArg_ParseTuple(args, "s", &path))
        return NULL;

    mapnik::freetype_engine::register_font(path);
    return Py_BuildValue("");
}

static PyObject *
mapnik_render_to_file(PyObject *self, PyObject *args)
{
    const char *filename, *format;
    const MapnikMap* themap;

    if (!PyArg_ParseTuple(args, "Oss", &themap, &filename, &format))
        return NULL;

#ifdef HAVE_CAIRO
    if (!strcmp(format, "SVG")) {
        mapnik::save_to_cairo_file(*themap->map, std::string(filename), std::string(format), 1.0);
        return Py_BuildValue("");
    }
#endif

    mapnik::image_any image = mapnik::image_rgba8(themap->map->width(), themap->map->height());
    render(*themap->map, image, 1.0, 0, 0);
    mapnik::save_to_file(image, std::string(filename), std::string(format));
    
    return Py_BuildValue("");
}

static PyMethodDef MapnikMethods[] = {
    {"parse_from_geojson", (PyCFunction) mapnik_parse_from_geojson, METH_VARARGS,
     "Build feature from geojson string"
    },
    {"register_datasources",  mapnik_register_datasources, METH_VARARGS,
     "Tell mapnik where to find datasource plugins"},
    {"register_font",  mapnik_register_font, METH_VARARGS,
     "Import a font file into mapnik"},
    {"render_to_file",  mapnik_render_to_file, METH_VARARGS,
     "Render a map to file."},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

// ===========================================================================
// MODULE

static PyModuleDef mapnikmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "pymapnik3",
    .m_doc = "mapnik 3 wrapper for Python.",
    .m_size = -1,
    MapnikMethods
};

PyMODINIT_FUNC
PyInit_pymapnik3(void)
{
    PyObject *m;
    if (PyType_Ready(&BoxType) < 0)
        return NULL;
    if (PyType_Ready(&ColorType) < 0)
        return NULL;
    if (PyType_Ready(&ContextType) < 0)
        return NULL;
    if (PyType_Ready(&ExpressionType) < 0)
        return NULL;
    if (PyType_Ready(&FeatureType) < 0)
        return NULL;
    if (PyType_Ready(&GdalType) < 0)
        return NULL;
    if (PyType_Ready(&GeoJsonType) < 0)
        return NULL;
    if (PyType_Ready(&LayerType) < 0)
        return NULL;
    if (PyType_Ready(&LineSymbolizerType) < 0)
        return NULL;
    if (PyType_Ready(&MapType) < 0)
        return NULL;
    if (PyType_Ready(&MemoryDatasourceType) < 0)
        return NULL;
    if (PyType_Ready(&PointSymbolizerType) < 0)
        return NULL;
    if (PyType_Ready(&PolygonSymbolizerType) < 0)
        return NULL;
    if (PyType_Ready(&ProjectionType) < 0)
        return NULL;
    if (PyType_Ready(&ProjTransformType) < 0)
        return NULL;
    if (PyType_Ready(&RasterColorizerType) < 0)
        return NULL;
    if (PyType_Ready(&RasterSymbolizerType) < 0)
        return NULL;
    if (PyType_Ready(&RuleType) < 0)
        return NULL;
    if (PyType_Ready(&ShapefileType) < 0)
        return NULL;
    if (PyType_Ready(&StyleType) < 0)
        return NULL;
    if (PyType_Ready(&TextSymbolizerType) < 0)
        return NULL;

    m = PyModule_Create(&mapnikmodule);
    if (m == NULL)
        return NULL;

    Py_INCREF(&BoxType);
    if (PyModule_AddObject(m, "Box2d", (PyObject *) &BoxType) < 0) {
        Py_DECREF(&BoxType);
        Py_DECREF(m);
        return NULL;
    }
    
    Py_INCREF(&ColorType);
    if (PyModule_AddObject(m, "Color", (PyObject *) &ColorType) < 0) {
        Py_DECREF(&ColorType);
        Py_DECREF(m);
        return NULL;
    }
    
    Py_INCREF(&ContextType);
    if (PyModule_AddObject(m, "Context", (PyObject *) &ContextType) < 0) {
        Py_DECREF(&ContextType);
        Py_DECREF(m);
        return NULL;
    }
    
    Py_INCREF(&ExpressionType);
    if (PyModule_AddObject(m, "Expression", (PyObject *) &ExpressionType) < 0) {
        Py_DECREF(&ExpressionType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&FeatureType);
    if (PyModule_AddObject(m, "Feature", (PyObject *) &FeatureType) < 0) {
        Py_DECREF(&FeatureType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&GdalType);
    if (PyModule_AddObject(m, "Gdal", (PyObject *) &GdalType) < 0) {
        Py_DECREF(&GdalType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&GeoJsonType);
    if (PyModule_AddObject(m, "GeoJSON", (PyObject *) &GeoJsonType) < 0) {
        Py_DECREF(&GeoJsonType);
        Py_DECREF(m);
        return NULL;
    }
    
    Py_INCREF(&LayerType);
    if (PyModule_AddObject(m, "Layer", (PyObject *) &LayerType) < 0) {
        Py_DECREF(&LayerType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&LineSymbolizerType);
    if (PyModule_AddObject(m, "LineSymbolizer", (PyObject *) &LineSymbolizerType) < 0) {
        Py_DECREF(&LineSymbolizerType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&MapType);
    if (PyModule_AddObject(m, "Map", (PyObject *) &MapType) < 0) {
        Py_DECREF(&MapType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&MemoryDatasourceType);
    if (PyModule_AddObject(m, "MemoryDatasource", (PyObject *) &MemoryDatasourceType) < 0) {
        Py_DECREF(&MemoryDatasourceType);
        Py_DECREF(m);
        return NULL;
    }
    
    Py_INCREF(&PointSymbolizerType);
    if (PyModule_AddObject(m, "PointSymbolizer", (PyObject *) &PointSymbolizerType) < 0) {
        Py_DECREF(&PointSymbolizerType);
        Py_DECREF(m);
        return NULL;
    }
    
    Py_INCREF(&PolygonSymbolizerType);
    if (PyModule_AddObject(m, "PolygonSymbolizer", (PyObject *) &PolygonSymbolizerType) < 0) {
        Py_DECREF(&PolygonSymbolizerType);
        Py_DECREF(m);
        return NULL;
    }
    
    Py_INCREF(&ProjectionType);
    if (PyModule_AddObject(m, "Projection", (PyObject *) &ProjectionType) < 0) {
        Py_DECREF(&ProjectionType);
        Py_DECREF(m);
        return NULL;
    }
    
    Py_INCREF(&ProjTransformType);
    if (PyModule_AddObject(m, "ProjTransform", (PyObject *) &ProjTransformType) < 0) {
        Py_DECREF(&ProjTransformType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&RasterColorizerType);
    if (PyModule_AddObject(m, "RasterColorizer", (PyObject *) &RasterColorizerType) < 0) {
        Py_DECREF(&RasterColorizerType);
        Py_DECREF(m);
        return NULL;
    }
    
    Py_INCREF(&RasterSymbolizerType);
    if (PyModule_AddObject(m, "RasterSymbolizer", (PyObject *) &RasterSymbolizerType) < 0) {
        Py_DECREF(&RasterSymbolizerType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&RuleType);
    if (PyModule_AddObject(m, "Rule", (PyObject *) &RuleType) < 0) {
        Py_DECREF(&RuleType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&ShapefileType);
    if (PyModule_AddObject(m, "Shapefile", (PyObject *) &ShapefileType) < 0) {
        Py_DECREF(&ShapefileType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&StyleType);
    if (PyModule_AddObject(m, "Style", (PyObject *) &StyleType) < 0) {
        Py_DECREF(&StyleType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&TextSymbolizerType);
    if (PyModule_AddObject(m, "TextSymbolizer", (PyObject *) &TextSymbolizerType) < 0) {
        Py_DECREF(&TextSymbolizerType);
        Py_DECREF(m);
        return NULL;
    }

    if (PyModule_AddIntConstant(m, "COLORIZER_INHERIT", 0)) {
        return NULL;
    }
    if (PyModule_AddIntConstant(m, "COLORIZER_LINEAR", 1)) {
        return NULL;
    }
    if (PyModule_AddIntConstant(m, "COLORIZER_DISCRETE", 2)) {
        return NULL;
    }
    if (PyModule_AddIntConstant(m, "COLORIZER_EXACT", 3)) {
        return NULL;
    }
    
    return m;
}
