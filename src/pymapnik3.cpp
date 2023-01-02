
// FIXME: can pass wrong type objects without it being detected
// FIXME: string parse errors, missing files etc must be detected

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>

#include <string>

#include <mapnik/agg_renderer.hpp>
#include <mapnik/box2d.hpp>
#include <mapnik/color.hpp>
#include <mapnik/datasource.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/feature_type_style.hpp>
#include <mapnik/image_any.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/map.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/proj_transform.hpp>

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
    const char *colorspec;
    if (!PyArg_ParseTuple(args, "s", &colorspec))
        return -1;

    self->color = new mapnik::color(std::string(colorspec));
    
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
        return Py_BuildValue("");
    }
    
    MapnikColor* ourcolor = (MapnikColor*) color;
    self->symbolizer->properties.insert(std::pair<mapnik::keys, mapnik::color&>(mapnik::keys::fill, *ourcolor->color));
    return Py_BuildValue("");
}

static PyMethodDef PolygonSymbolizer_methods[] = {
    {"set_fill", (PyCFunction) PolygonSymbolizer_set_fill, METH_O,
     "Set fill color"
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
    if (!PyObject_IsInstance(symbolizer, (PyObject*) &PolygonSymbolizerType)) {
        PyErr_SetString(PyExc_RuntimeError, "add_symbolizer requires a symbolizer object");
        return Py_BuildValue("");
    }
    
    MapnikPolygonSymbolizer* oursymb = (MapnikPolygonSymbolizer*) symbolizer;
    self->rule->append(*oursymb->symbolizer);
    return Py_BuildValue("");
}

static PyMethodDef Rule_methods[] = {
    {"add_symbolizer", (PyCFunction) Rule_add_symbolizer, METH_O,
     "Add a symbolizer to the rule"
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
    MapnikRule* ourrule = (MapnikRule*) rule; // FIXME
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
    MapnikShapefile* shapefile = (MapnikShapefile*) arg; // FIXME
    self->layer->set_datasource(shapefile->source);
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
    {"set_datasource", (PyCFunction) Layer_set_datasource, METH_O,
     "Add underlying data source"
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
        return Py_BuildValue("");
    }
    
    MapnikLayer *layer = (MapnikLayer *) obj;
    self->map->add_layer(*layer->layer);
    return Py_BuildValue("");
}

static PyObject *
Map_add_style(MapnikMap *self, PyObject* args)
{
    char *c_name;
    MapnikStyle *style; // FIXME
    if (!PyArg_ParseTuple(args, "sO", &c_name, &style))
        return NULL;

    self->map->insert_style(std::string(c_name), *style->style);
    return Py_BuildValue("");
}

static PyObject *
Map_set_background(MapnikMap *self, PyObject *color)
{
    if (!PyObject_IsInstance(color, (PyObject*) &ColorType)) {
        PyErr_SetString(PyExc_RuntimeError, "set_background requires a color object");
        return Py_BuildValue("");
    }
    
    MapnikColor* ourcolor = (MapnikColor*) color;
    self->map->set_background(*ourcolor->color);
    return Py_BuildValue("");
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
    MAPNIK_LOG_DEBUG(flupp) << "setting up renderer" << std::endl;
    mapnik::agg_renderer<mapnik::image_rgba8> ren(m_, pixmap, scale_factor_, offset_x_, offset_y_);
    MAPNIK_LOG_DEBUG(flupp) << "applying renderer" << std::endl;
    ren.apply();
}

void render(mapnik::Map const& map,
            mapnik::image_any& image,
            double scale_factor = 1.0,
            unsigned offset_x = 0u,
            unsigned offset_y = 0u)
{
    MAPNIK_LOG_DEBUG(flupp) << "ready to render map" << std::endl;
    mapnik::util::apply_visitor(agg_renderer_visitor_1(map, scale_factor, offset_x, offset_y), image);
    MAPNIK_LOG_DEBUG(flupp) << "map has been rendered" << std::endl;
}

void inspect(mapnik::image_any& image, int pos)
{
    std::cout << "at " << pos << " ";
    for (int ix = 0; ix < 10; ix++) {
        std::cout << (int) image.bytes()[pos + ix] << " ";
    }
    std::cout << std::endl;
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
    inspect(image, 0);
    inspect(image, 10000);
    inspect(image, 100000);
    mapnik::save_to_file(image, std::string(filename), std::string(format));
    
    return Py_BuildValue("");
}

static PyMethodDef MapnikMethods[] = {
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
    std::string path = std::string("/usr/local/lib/mapnik/input/"); // FIXME
    mapnik::datasource_cache::instance().register_datasources(path);
 
    PyObject *m;
    if (PyType_Ready(&BoxType) < 0)
        return NULL;
    if (PyType_Ready(&ColorType) < 0)
        return NULL;
    if (PyType_Ready(&LayerType) < 0)
        return NULL;
    if (PyType_Ready(&MapType) < 0)
        return NULL;
    if (PyType_Ready(&PolygonSymbolizerType) < 0)
        return NULL;
    if (PyType_Ready(&ProjectionType) < 0)
        return NULL;
    if (PyType_Ready(&ProjTransformType) < 0)
        return NULL;
    if (PyType_Ready(&RuleType) < 0)
        return NULL;
    if (PyType_Ready(&ShapefileType) < 0)
        return NULL;
    if (PyType_Ready(&StyleType) < 0)
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

    Py_INCREF(&LayerType);
    if (PyModule_AddObject(m, "Layer", (PyObject *) &LayerType) < 0) {
        Py_DECREF(&LayerType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&MapType);
    if (PyModule_AddObject(m, "Map", (PyObject *) &MapType) < 0) {
        Py_DECREF(&MapType);
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
    
    return m;
}
