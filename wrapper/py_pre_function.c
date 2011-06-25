/* ------------------------------------------------------------------------- */

/* (C) Copyright 2010-2011 New Relic Inc. All rights reserved. */

/* ------------------------------------------------------------------------- */

#include "py_pre_function.h"

#include "py_utilities.h"

#include "structmember.h"

/* ------------------------------------------------------------------------- */

static PyObject *NRPreFunctionWrapper_new(PyTypeObject *type,
                                          PyObject *args, PyObject *kwds)
{
    NRPreFunctionWrapperObject *self;

    self = (NRPreFunctionWrapperObject *)type->tp_alloc(type, 0);

    if (!self)
        return NULL;

    self->dict = NULL;
    self->next_object = NULL;
    self->last_object = NULL;
    self->function_object = NULL;
    self->run_once = 0;

    return (PyObject *)self;
}

/* ------------------------------------------------------------------------- */

static int NRPreFunctionWrapper_init(NRPreFunctionWrapperObject *self,
                                     PyObject *args, PyObject *kwds)
{
    PyObject *wrapped_object = NULL;
    PyObject *function_object = NULL;
    PyObject *run_once = NULL;

    PyObject *object = NULL;

    static char *kwlist[] = { "wrapped", "pre_function", "run_once", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO|O!:PreFunctionWrapper",
                                     kwlist, &wrapped_object, &function_object,
                                     &PyBool_Type, &run_once)) {
        return -1;
    }

    Py_INCREF(wrapped_object);

    Py_XDECREF(self->dict);
    Py_XDECREF(self->next_object);
    Py_XDECREF(self->last_object);

    self->next_object = wrapped_object;
    self->last_object = NULL;

    object = PyObject_GetAttrString(wrapped_object, "__newrelic__");

    if (object) {
        Py_DECREF(object);

        object = PyObject_GetAttrString(wrapped_object, "__last_object__");

        if (object)
            self->last_object = object;
        else
            PyErr_Clear();
    }
    else
        PyErr_Clear();

    if (!self->last_object) {
        Py_INCREF(wrapped_object);
        self->last_object = wrapped_object;
    }

    object = PyObject_GetAttrString(self->last_object, "__dict__");

    if (object)
        self->dict = object;
    else
        PyErr_Clear();

    Py_INCREF(function_object);
    Py_XDECREF(self->function_object);
    self->function_object = function_object;

    self->run_once = (run_once == Py_True);

    return 0;
}

/* ------------------------------------------------------------------------- */

static void NRPreFunctionWrapper_dealloc(NRPreFunctionWrapperObject *self)
{
    Py_XDECREF(self->dict);

    Py_XDECREF(self->next_object);
    Py_XDECREF(self->last_object);

    Py_XDECREF(self->function_object);

    Py_TYPE(self)->tp_free(self);
}

/* ------------------------------------------------------------------------- */

static PyObject *NRPreFunctionWrapper_call(NRPreFunctionWrapperObject *self,
                                           PyObject *args, PyObject *kwds)
{
    if (self->function_object) {
        PyObject *function_result = NULL;

        function_result = PyObject_Call(self->function_object, args, kwds);

        if (self->run_once) {
            self->run_once = 0;
            Py_DECREF(self->function_object);
            self->function_object = NULL;
        }

        if (!function_result)
            return NULL;

        Py_DECREF(function_result);
    }

    return PyObject_Call(self->next_object, args, kwds);
}

/* ------------------------------------------------------------------------- */

static PyObject *NRPreFunctionWrapper_get_next(
        NRPreFunctionWrapperObject *self, void *closure)
{
    if (!self->next_object) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    Py_INCREF(self->next_object);
    return self->next_object;
}

/* ------------------------------------------------------------------------- */

static PyObject *NRPreFunctionWrapper_get_last(
        NRPreFunctionWrapperObject *self, void *closure)
{
    if (!self->last_object) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    Py_INCREF(self->last_object);
    return self->last_object;
}

/* ------------------------------------------------------------------------- */

static PyObject *NRPreFunctionWrapper_get_marker(
        NRPreFunctionWrapperObject *self, void *closure)
{
    Py_INCREF(Py_None);
    return Py_None;
}

/* ------------------------------------------------------------------------- */

static PyObject *NRPreFunctionWrapper_get_module(
        NRPreFunctionWrapperObject *self)
{
    if (!self->last_object) {
      PyErr_SetString(PyExc_ValueError,
              "object wrapper has not been initialised");
      return NULL;
    }

    return PyObject_GetAttrString(self->last_object, "__module__");
}

/* ------------------------------------------------------------------------- */

static PyObject *NRPreFunctionWrapper_get_name(
        NRPreFunctionWrapperObject *self)
{
    if (!self->last_object) {
      PyErr_SetString(PyExc_ValueError,
              "object wrapper has not been initialised");
      return NULL;
    }

    return PyObject_GetAttrString(self->last_object, "__name__");
}

/* ------------------------------------------------------------------------- */

static PyObject *NRPreFunctionWrapper_get_doc(
        NRPreFunctionWrapperObject *self)
{
    if (!self->last_object) {
      PyErr_SetString(PyExc_ValueError,
              "object wrapper has not been initialised");
      return NULL;
    }

    return PyObject_GetAttrString(self->last_object, "__doc__");
}

/* ------------------------------------------------------------------------- */

static PyObject *NRPreFunctionWrapper_get_dict(
        NRPreFunctionWrapperObject *self)
{
    if (!self->last_object) {
      PyErr_SetString(PyExc_ValueError,
              "object wrapper has not been initialised");
      return NULL;
    }

    if (self->dict) {
        Py_INCREF(self->dict);
        return self->dict;
    }

    return PyObject_GetAttrString(self->last_object, "__dict__");
}

/* ------------------------------------------------------------------------- */

static PyObject *NRPreFunctionWrapper_getattro(
        NRPreFunctionWrapperObject *self, PyObject *name)
{
    PyObject *object = NULL;

    if (!self->last_object) {
      PyErr_SetString(PyExc_ValueError,
              "object wrapper has not been initialised");
      return NULL;
    }

    object = PyObject_GenericGetAttr((PyObject *)self, name);

    if (object)
        return object;

    PyErr_Clear();

    return PyObject_GetAttr(self->last_object, name);
}

/* ------------------------------------------------------------------------- */

static int NRPreFunctionWrapper_setattro(
        NRPreFunctionWrapperObject *self, PyObject *name, PyObject *value)
{
    if (!self->last_object) {
      PyErr_SetString(PyExc_ValueError,
              "object wrapper has not been initialised");
      return -1;
    }

    return PyObject_SetAttr(self->last_object, name, value);
}

/* ------------------------------------------------------------------------- */

static PyObject *NRPreFunctionWrapper_descr_get(PyObject *function,
                                                PyObject *object,
                                                PyObject *type)
{
    if (object == Py_None)
        object = NULL;

    return PyMethod_New(function, object, type);
}

/* ------------------------------------------------------------------------- */

static PyGetSetDef NRPreFunctionWrapper_getset[] = {
    { "__next_object__",    (getter)NRPreFunctionWrapper_get_next,
                            NULL, 0 },
    { "__last_object__",    (getter)NRPreFunctionWrapper_get_last,
                            NULL, 0 },
    { "__newrelic__",       (getter)NRPreFunctionWrapper_get_marker,
                            NULL, 0 },
    { "__module__",         (getter)NRPreFunctionWrapper_get_module,
                            NULL, 0 },
    { "__name__",           (getter)NRPreFunctionWrapper_get_name,
                            NULL, 0 },
    { "__doc__",            (getter)NRPreFunctionWrapper_get_doc,
                            NULL, 0 },
    { "__dict__",           (getter)NRPreFunctionWrapper_get_dict,
                            NULL, 0 },
    { NULL },
};

PyTypeObject NRPreFunctionWrapper_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_newrelic.PreFunctionWrapper", /*tp_name*/
    sizeof(NRPreFunctionWrapperObject), /*tp_basicsize*/
    0,                      /*tp_itemsize*/
    /* methods */
    (destructor)NRPreFunctionWrapper_dealloc, /*tp_dealloc*/
    0,                      /*tp_print*/
    0,                      /*tp_getattr*/
    0,                      /*tp_setattr*/
    0,                      /*tp_compare*/
    0,                      /*tp_repr*/
    0,                      /*tp_as_number*/
    0,                      /*tp_as_sequence*/
    0,                      /*tp_as_mapping*/
    0,                      /*tp_hash*/
    (ternaryfunc)NRPreFunctionWrapper_call, /*tp_call*/
    0,                      /*tp_str*/
    (getattrofunc)NRPreFunctionWrapper_getattro, /*tp_getattro*/
    (setattrofunc)NRPreFunctionWrapper_setattro, /*tp_setattro*/
    0,                      /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,     /*tp_flags*/
    0,                      /*tp_doc*/
    0,                      /*tp_traverse*/
    0,                      /*tp_clear*/
    0,                      /*tp_richcompare*/
    0,                      /*tp_weaklistoffset*/
    0,                      /*tp_iter*/
    0,                      /*tp_iternext*/
    0,                      /*tp_methods*/
    0,                      /*tp_members*/
    NRPreFunctionWrapper_getset, /*tp_getset*/
    0,                      /*tp_base*/
    0,                      /*tp_dict*/
    NRPreFunctionWrapper_descr_get, /*tp_descr_get*/
    0,                      /*tp_descr_set*/
    offsetof(NRPreFunctionWrapperObject, dict), /*tp_dictoffset*/
    (initproc)NRPreFunctionWrapper_init, /*tp_init*/
    0,                      /*tp_alloc*/
    NRPreFunctionWrapper_new, /*tp_new*/
    0,                      /*tp_free*/
    0,                      /*tp_is_gc*/
};

/* ------------------------------------------------------------------------- */

static PyObject *NRPreFunctionDecorator_new(PyTypeObject *type,
                                            PyObject *args, PyObject *kwds)
{
    NRPreFunctionDecoratorObject *self;

    self = (NRPreFunctionDecoratorObject *)type->tp_alloc(type, 0);

    if (!self)
        return NULL;

    self->function_object = NULL;
    self->run_once = 0;

    return (PyObject *)self;
}

/* ------------------------------------------------------------------------- */

static int NRPreFunctionDecorator_init(NRPreFunctionDecoratorObject *self,
                                       PyObject *args, PyObject *kwds)
{
    PyObject *function_object = NULL;
    PyObject *run_once = NULL;

    static char *kwlist[] = { "pre_function", "run_once", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O!:PreFunctionDecorator",
                                     kwlist, &function_object, &PyBool_Type,
                                     &run_once)) {
        return -1;
    }

    Py_INCREF(function_object);
    Py_XDECREF(self->function_object);
    self->function_object = function_object;

    self->run_once = (run_once == Py_True);

    return 0;
}

/* ------------------------------------------------------------------------- */

static void NRPreFunctionDecorator_dealloc(NRPreFunctionDecoratorObject *self)
{
    Py_XDECREF(self->function_object);

    Py_TYPE(self)->tp_free(self);
}

/* ------------------------------------------------------------------------- */

static PyObject *NRPreFunctionDecorator_call(
        NRPreFunctionDecoratorObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *function_object = NULL;

    static char *kwlist[] = { "pre_function", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O:PreFunctionDecorator",
                                     kwlist, &function_object)) {
        return NULL;
    }

    return PyObject_CallFunctionObjArgs(
            (PyObject *)&NRPreFunctionWrapper_Type,
            function_object, self->function_object,
            (self->run_once ? Py_True : Py_False), NULL);
}

/* ------------------------------------------------------------------------- */

PyTypeObject NRPreFunctionDecorator_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_newrelic.PreFunctionDecorator", /*tp_name*/
    sizeof(NRPreFunctionDecoratorObject), /*tp_basicsize*/
    0,                      /*tp_itemsize*/
    /* methods */
    (destructor)NRPreFunctionDecorator_dealloc, /*tp_dealloc*/
    0,                      /*tp_print*/
    0,                      /*tp_getattr*/
    0,                      /*tp_setattr*/
    0,                      /*tp_compare*/
    0,                      /*tp_repr*/
    0,                      /*tp_as_number*/
    0,                      /*tp_as_sequence*/
    0,                      /*tp_as_mapping*/
    0,                      /*tp_hash*/
    (ternaryfunc)NRPreFunctionDecorator_call, /*tp_call*/
    0,                      /*tp_str*/
    0,                      /*tp_getattro*/
    0,                      /*tp_setattro*/
    0,                      /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,     /*tp_flags*/
    0,                      /*tp_doc*/
    0,                      /*tp_traverse*/
    0,                      /*tp_clear*/
    0,                      /*tp_richcompare*/
    0,                      /*tp_weaklistoffset*/
    0,                      /*tp_iter*/
    0,                      /*tp_iternext*/
    0,                      /*tp_methods*/
    0,                      /*tp_members*/
    0,                      /*tp_getset*/
    0,                      /*tp_base*/
    0,                      /*tp_dict*/
    0,                      /*tp_descr_get*/
    0,                      /*tp_descr_set*/
    0,                      /*tp_dictoffset*/
    (initproc)NRPreFunctionDecorator_init, /*tp_init*/
    0,                      /*tp_alloc*/
    NRPreFunctionDecorator_new, /*tp_new*/
    0,                      /*tp_free*/
    0,                      /*tp_is_gc*/
};

/* ------------------------------------------------------------------------- */

/*
 * vim: set cino=>2,e0,n0,f0,{2,}0,^0,\:2,=2,p2,t2,c1,+2,(2,u2,)20,*30,g2,h2 ts=8
 */
