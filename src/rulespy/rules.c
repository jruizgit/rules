#include <Python.h>
#include <rules.h>

static PyObject *RulesError;

static PyObject *rules_createRuleset(PyObject *self, PyObject *args) {
    char *name;
    char *rules;
    if (!PyArg_ParseTuple(args, "ss", &name, &rules)) {
        PyErr_SetString(RulesError, "Invalid argument");
        return NULL;
    }

    void *output = NULL;
    unsigned int result = createRuleset(&output, name, rules);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char * message;
            asprintf(&message, "Could not create ruleset, error code: %d", result);
            PyErr_SetString(RulesError, message);
            free(message);
        }
        return NULL;
    }

    return Py_BuildValue("l", output);
}

static PyObject *rules_deleteRuleset(PyObject *self, PyObject *args) {
    void *handle;
    if (!PyArg_ParseTuple(args, "l", &handle)) {
        PyErr_SetString(RulesError, "Invalid argument");
        return NULL;
    }

    unsigned int result = deleteRuleset(handle);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char * message;
            asprintf(&message, "Could not delete ruleset, error code: %d", result);
            PyErr_SetString(RulesError, message);
            free(message);
        }
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *rules_bindRuleset(PyObject *self, PyObject *args) {
    void *handle;
    char *host;
    unsigned int port;
    char *password = NULL;
    unsigned int result;
    if (PyArg_ParseTuple(args, "lzls", &handle, &password, &port, &host)) {
        result = bindRuleset(handle, host, port, password);
    } else {
        PyErr_SetString(RulesError, "Invalid argument");
        return NULL;
    }

    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char * message;
            asprintf(&message, "Could not create connection, error code: %d", result);
            PyErr_SetString(RulesError, message);
            free(message);
        }
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *rules_assertEvent(PyObject *self, PyObject *args) {
    void *handle;
    char *event;
    if (!PyArg_ParseTuple(args, "ls", &handle, &event)) {
        PyErr_SetString(RulesError, "Invalid argument");
        return NULL;
    }

    unsigned int result = assertEvent(handle, event);
    if (result == RULES_OK) {
        return Py_BuildValue("i", 1);    
    } else if (result == ERR_EVENT_NOT_HANDLED) {
        return Py_BuildValue("i", 0);    
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char * message;
            asprintf(&message, "Could not assert event, error code: %d", result);  
            PyErr_SetString(RulesError, message);
            free(message);
        }
        return NULL;
    }
}

static PyObject *rules_assertEvents(PyObject *self, PyObject *args) {
    void *handle;
    char *events;
    if (!PyArg_ParseTuple(args, "ls", &handle, &events)) {
        PyErr_SetString(RulesError, "Invalid argument");
        return NULL;
    }

    unsigned int *results;
    unsigned int resultsLength;
    unsigned int result = assertEvents(handle, events, &resultsLength, &results);
    if (result == RULES_OK) {
        free(results);
        return Py_BuildValue("i", resultsLength);
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char * message;
            asprintf(&message, "Could not assert event, error code: %d", result);  
            PyErr_SetString(RulesError, message);
            free(message);
        }
        return NULL;
    }
}

static PyObject *rules_assertState(PyObject *self, PyObject *args) {
    void *handle;
    char *state;
    if (!PyArg_ParseTuple(args, "ls", &handle, &state)) {
        PyErr_SetString(RulesError, "Invalid argument");
        return NULL;
    }

    unsigned int result = assertState(handle, state);
    if (result == RULES_OK) {
        return Py_BuildValue("i", 1);    
    } else if (result == ERR_EVENT_NOT_HANDLED) {
        return Py_BuildValue("i", 0);    
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char * message;
            asprintf(&message, "Could not assert state, error code: %d", result);
            PyErr_SetString(RulesError, message);
            free(message);
        }
        return NULL;
    }
}

static PyObject *rules_startAction(PyObject *self, PyObject *args) {
    void *handle;
    if (!PyArg_ParseTuple(args, "l", &handle)) {
        PyErr_SetString(RulesError, "Invalid argument");
        return NULL;
    }
    
    char *state;
    char *messages;
    void *actionHandle;
    unsigned int result = startAction(handle, &state, &messages, &actionHandle);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char * message;
            asprintf(&message, "Could not start action, error code: %d", result);
            PyErr_SetString(RulesError, message);
            free(message);
        }
        return NULL;
    }

    return Py_BuildValue("ssl", state, messages, actionHandle);
}

static PyObject *rules_completeAction(PyObject *self, PyObject *args) {
    void *handle;
    void *actionHandle;
    char *state;
    if (!PyArg_ParseTuple(args, "lls", &handle, &actionHandle, &state)) {
        PyErr_SetString(RulesError, "Invalid argument");
        return NULL;
    }

    unsigned int result = completeAction(handle, actionHandle, state);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char * message;
            asprintf(&message, "Could not complete action, error code: %d", result);
            PyErr_SetString(RulesError, message);
            free(message);
        }
        return NULL;
    }

    Py_RETURN_NONE;  
}

static PyObject *rules_abandonAction(PyObject *self, PyObject *args) {
    void *handle;
    void *actionHandle;
    if (!PyArg_ParseTuple(args, "ll", &handle, &actionHandle)) {
        PyErr_SetString(RulesError, "Invalid argument");
        return NULL;
    }

    unsigned int result = abandonAction(handle, actionHandle);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char * message;
            asprintf(&message, "Could not abandon action, error code: %d", result);
            PyErr_SetString(RulesError, message);
            free(message);
        }
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *rules_startTimer(PyObject *self, PyObject *args) {
    void *handle;
    char *sid;
    unsigned int duration;
    char *timer;
    if (!PyArg_ParseTuple(args, "lsls", &handle, &sid, &duration, &timer)) {
        PyErr_SetString(RulesError, "Invalid argument");
        return NULL;
    }

    unsigned int result = startTimer(handle, sid, duration, timer);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char * message;
            asprintf(&message, "Could not start timer, error code: %d", result);
            PyErr_SetString(RulesError, message);
            free(message);
        }
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *rules_assertTimers(PyObject *self, PyObject *args) {
    void *handle;
    if (!PyArg_ParseTuple(args, "l", &handle)) {
        PyErr_SetString(RulesError, "Invalid argument");
        return NULL;
    }

    unsigned int result = assertTimers(handle);
    if (result == RULES_OK) {
        return Py_BuildValue("i", 1);    
    } else if (result == ERR_NO_TIMERS_AVAILABLE) {
        return Py_BuildValue("i", 0);    
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char * message;
            asprintf(&message, "Could not assert timers, error code: %d", result);
            PyErr_SetString(RulesError, message);
            free(message);
        }
        return NULL;
    }
}

static PyObject *rules_getState(PyObject *self, PyObject *args) {
    void *handle;
    char *sid;
    if (!PyArg_ParseTuple(args, "ls", &handle, &sid)) {
        PyErr_SetString(RulesError, "Invalid argument");
        return NULL;
    }

    char *state;
    unsigned int result = getState(handle, sid, &state);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char * message;
            asprintf(&message, "Could not get state, error code: %d", result);
            PyErr_SetString(RulesError, message);
            free(message);
        }
        return NULL;
    }
    PyObject *returnValue = Py_BuildValue("s", state);
    free(state);
    return returnValue;
}

static PyMethodDef myModule_methods[] = {
    {"createRuleset", rules_createRuleset, METH_VARARGS},
    {"deleteRuleset", rules_deleteRuleset, METH_VARARGS},
    {"bindRuleset", rules_bindRuleset, METH_VARARGS},
    {"assertEvent", rules_assertEvent, METH_VARARGS},
    {"assertEvents", rules_assertEvents, METH_VARARGS},
    {"assertState", rules_assertState, METH_VARARGS},
    {"startAction", rules_startAction, METH_VARARGS},
    {"completeAction", rules_completeAction, METH_VARARGS},
    {"abandonAction", rules_abandonAction, METH_VARARGS},
    {"startTimer", rules_startTimer, METH_VARARGS},
    {"assertTimers", rules_assertTimers, METH_VARARGS},
    {"getState", rules_getState, METH_VARARGS},
    {NULL, NULL}
};

PyMODINIT_FUNC initrules()
{
    PyObject *m = Py_InitModule("rules", myModule_methods);
    if (m != NULL) {
        RulesError = PyErr_NewException("rules.error", NULL, NULL);
        Py_INCREF(RulesError);
        PyModule_AddObject(m, "error", RulesError);
    }
}