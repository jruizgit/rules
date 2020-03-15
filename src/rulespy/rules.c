#include <Python.h>
#include <rules.h>

static PyObject *RulesError;

#define PRINT_ARGS(args) do { \
    PyObject* or = PyObject_Repr(args); \
    const char* s = PyString_AsString(or); \
    printf("args %s\n", s); \
} while(0)


#if PY_MAJOR_VERSION >= 3

#define RESULT_TO_INT(result, errorCode) do { \
    if (result && PyLong_Check(result)) { \
        errorCode = PyLong_AsLong(result); \
    } \
} while(0)

#else

#define RESULT_TO_INT(result, errorCode) do { \
    if (result && PyInt_Check(result)) { \
        errorCode = PyInt_AsLong(result); \
    } \
} while(0)

#endif



static PyObject *pyCreateRuleset(PyObject *self, PyObject *args) {
    char *name;
    char *rules;
    if (!PyArg_ParseTuple(args, "ss", &name, &rules)) {
        PyErr_SetString(RulesError, "pyCreateRuleset Invalid argument");
        return NULL;
    }

    unsigned int output = 0;
    unsigned int result = createRuleset(&output, name, rules);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not create ruleset, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }

    return Py_BuildValue("I", output);
}

static PyObject *pyDeleteRuleset(PyObject *self, PyObject *args) {
    unsigned int handle;
    if (!PyArg_ParseTuple(args, "I", &handle)) {
        PyErr_SetString(RulesError, "pyDeleteRuleset Invalid argument");
        return NULL;
    }

    unsigned int result = deleteRuleset(handle);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not delete ruleset, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *pyAssertEvent(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *event;
    if (!PyArg_ParseTuple(args, "Is", &handle, &event)) {
        PyErr_SetString(RulesError, "pyAssertEvent Invalid argument");
        return NULL;
    }
    
    unsigned int stateOffset;
    unsigned int result = assertEvent(handle, event, &stateOffset);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED || result == ERR_EVENT_DEFERRED) {
        return Py_BuildValue("II", result, stateOffset);
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not assert event, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }
}

static PyObject *pyAssertEvents(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *events;
    if (!PyArg_ParseTuple(args, "Is", &handle, &events)) {
        PyErr_SetString(RulesError, "pyAssertEvents Invalid argument");
        return NULL;
    }

    unsigned int stateOffset;
    unsigned int result = assertEvents(handle, events, &stateOffset);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED || result == ERR_EVENT_DEFERRED) {
        return Py_BuildValue("II", result, stateOffset);
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not assert events, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }
}

static PyObject *pyAssertFact(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *fact;
    if (!PyArg_ParseTuple(args, "Is", &handle, &fact)) {
        PyErr_SetString(RulesError, "pyAssertFact Invalid argument");
        return NULL;
    }

    unsigned int stateOffset;
    unsigned int result = assertFact(handle, fact, &stateOffset);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED || result == ERR_EVENT_DEFERRED) {
        return Py_BuildValue("II", result, stateOffset);
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not assert fact, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }
}

static PyObject *pyAssertFacts(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *facts;
    if (!PyArg_ParseTuple(args, "Is", &handle, &facts)) {
        PyErr_SetString(RulesError, "pyAssertFacts Invalid argument");
        return NULL;
    }

    unsigned int stateOffset;
    unsigned int result = assertFacts(handle, facts, &stateOffset);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED || result == ERR_EVENT_DEFERRED) {
        return Py_BuildValue("II", result, stateOffset);
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else {
            char *message;
            if (asprintf(&message, "Could not assert facts, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }
}

static PyObject *pyRetractFact(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *fact;
    if (!PyArg_ParseTuple(args, "Is", &handle, &fact)) {
        PyErr_SetString(RulesError, "pyRetractFact Invalid argument");
        return NULL;
    }

    unsigned int stateOffset;
    unsigned int result = retractFact(handle, fact, &stateOffset);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED || result == ERR_EVENT_DEFERRED) {
        return Py_BuildValue("II", result, stateOffset);
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not retract fact, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }
}

static PyObject *pyRetractFacts(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *facts;
    if (!PyArg_ParseTuple(args, "Is", &handle, &facts)) {
        PyErr_SetString(RulesError, "pyAssertFacts Invalid argument");
        return NULL;
    }

    unsigned int stateOffset;
    unsigned int result = retractFacts(handle, facts, &stateOffset);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED || result == ERR_EVENT_DEFERRED) {
        return Py_BuildValue("II", result, stateOffset);
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else {
            char *message;
            if (asprintf(&message, "Could not retract facts, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }
}

static PyObject *pyUpdateState(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *state;
    if (!PyArg_ParseTuple(args, "Is", &handle, &state)) {
        PyErr_SetString(RulesError, "pyUpdateState Invalid argument");
        return NULL;
    }

    unsigned int stateOffset;
    unsigned int result = updateState(handle, state, &stateOffset);
    if (result == RULES_OK) {
        return Py_BuildValue("I", stateOffset);
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not assert state, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }
}

static PyObject *pyStartAction(PyObject *self, PyObject *args) {
    unsigned int handle;
    if (!PyArg_ParseTuple(args, "I", &handle)) {
        PyErr_SetString(RulesError, "pyStartAction Invalid argument");
        return NULL;
    }
    
    char *state;
    char *messages;
    unsigned int stateOffset;
    unsigned int result = startAction(handle, &state, &messages, &stateOffset);
    if (result == ERR_NO_ACTION_AVAILABLE) {
        Py_RETURN_NONE;
    } else if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not start action, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }

    PyObject *returnValue = Py_BuildValue("ssI", state, messages, stateOffset);
    return returnValue;
}

static PyObject *pyStartActionForState(PyObject *self, PyObject *args) {
    unsigned int handle;
    unsigned int stateOffset;
    if (!PyArg_ParseTuple(args, "II", &handle, &stateOffset)) {
        PyErr_SetString(RulesError, "pyStartAction Invalid argument");
        return NULL;
    }
    
    char *state = NULL;
    char *messages = NULL;
    unsigned int result = startActionForState(handle, stateOffset, &state, &messages);
    if (result == ERR_NO_ACTION_AVAILABLE) {
        Py_RETURN_NONE;
    } else if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not start action, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }

    PyObject *returnValue = Py_BuildValue("ss", state, messages);
    return returnValue;
}

static PyObject *pyCompleteAndStartAction(PyObject *self, PyObject *args) {
    unsigned int handle;
    unsigned int stateOffset;
    if (!PyArg_ParseTuple(args, "II", &handle, &stateOffset)) {
        PyErr_SetString(RulesError, "pyCompleteAndStartAction Invalid argument");
        return NULL;
    }

    char *messages;
    unsigned int result = completeAndStartAction(handle, stateOffset, &messages);
    if (result == ERR_NO_ACTION_AVAILABLE) {
        Py_RETURN_NONE;
    } if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not complete and start action, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }

    PyObject *returnValue = Py_BuildValue("s", messages);
    return returnValue;
}

static PyObject *pyAbandonAction(PyObject *self, PyObject *args) {
    unsigned int handle;
    unsigned int stateOffset;
    if (!PyArg_ParseTuple(args, "II", &handle, &stateOffset)) {
        PyErr_SetString(RulesError, "pyAbandonAction Invalid argument");
        return NULL;
    }

    unsigned int result = abandonAction(handle, stateOffset);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not abandon action, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *pyStartTimer(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *sid;
    int duration = 0;
    char manualReset = 0;
    char *timer = NULL;   
    if (!PyArg_ParseTuple(args, "Iibsz", &handle, &duration, &manualReset, &timer, &sid)) {
        PyErr_SetString(RulesError, "pyStartTimer Invalid argument");
        return NULL;
    }

    unsigned int result = startTimer(handle, sid, duration, manualReset, timer);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not start timer, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *pyCancelTimer(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *sid;
    char *timerName = NULL;
    if (!PyArg_ParseTuple(args, "Izs", &handle, &sid, &timerName)) {
        PyErr_SetString(RulesError, "pyCancelTimer Invalid argument");
        return NULL;
    }

    unsigned int result = cancelTimer(handle, sid, timerName);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not cancel timer, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *pyAssertTimers(PyObject *self, PyObject *args) {
    unsigned int handle;
    if (!PyArg_ParseTuple(args, "I", &handle)) {
        PyErr_SetString(RulesError, "pyAssertTimers Invalid argument");
        return NULL;
    }

    unsigned int result = assertTimers(handle);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not assert timers, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *pyGetState(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *sid;
    if (!PyArg_ParseTuple(args, "Iz", &handle, &sid)) {
        PyErr_SetString(RulesError, "pyGetState Invalid argument");
        return NULL;
    }

    char *state;
    unsigned int result = getState(handle, sid, &state);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not get state, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }
    PyObject *returnValue = Py_BuildValue("s", state);
    free(state);
    return returnValue;
}

static PyObject *pyDeleteState(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *sid;
    if (!PyArg_ParseTuple(args, "Iz", &handle, &sid)) {
        PyErr_SetString(RulesError, "pyDeleteState Invalid argument");
        return NULL;
    }

    unsigned int result = deleteState(handle, sid);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not delete state, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }

     Py_RETURN_NONE;
}

static PyObject *pyRenewActionLease(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *sid;
    if (!PyArg_ParseTuple(args, "Iz", &handle, &sid)) {
        PyErr_SetString(RulesError, "pyRenewActionLease Invalid argument");
        return NULL;
    }

    unsigned int result = renewActionLease(handle, sid);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not renew action lease, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *pyGetFacts(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *sid;
    if (!PyArg_ParseTuple(args, "Iz", &handle, &sid)) {
        PyErr_SetString(RulesError, "pyGetFacts Invalid argument");
        return NULL;
    }

    char *facts;
    unsigned int result = getFacts(handle, sid, &facts);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not get facts, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }
    PyObject *returnValue = Py_BuildValue("s", facts);
    free(facts);
    return returnValue;
}

static PyObject *pyGetEvents(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *sid;
    if (!PyArg_ParseTuple(args, "Iz", &handle, &sid)) {
        PyErr_SetString(RulesError, "pyGetEvents Invalid argument");
        return NULL;
    }

    char *events;
    unsigned int result = getEvents(handle, sid, &events);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not get events, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }
    PyObject *returnValue = Py_BuildValue("s", events);
    free(events);
    return returnValue;
}

static unsigned int storeMessageCallback(void *context, char *ruleset, char *sid, char *mid, unsigned char messageType, char *content) {
    unsigned int errorCode = ERR_UNEXPECTED_TYPE;
    PyObject *arglist;
    PyObject *result;
    PyObject *callback = (PyObject *)context;

    arglist = Py_BuildValue("(sssIs)", ruleset, sid, mid, messageType, content);
    result = PyEval_CallObject(callback, arglist);
    Py_DECREF(arglist);
    
    RESULT_TO_INT(result, errorCode);

    Py_XDECREF(result);   
    return errorCode;
}

static PyObject *pySetStoreMessageCallback(PyObject *self, PyObject *args) {

    unsigned int handle;
    PyObject *callback;
    if (!PyArg_ParseTuple(args, "IO", &handle, &callback)) {
        PyErr_SetString(RulesError, "pySetStoreMessageCallback Invalid argument");
        return NULL;
    }

    Py_XINCREF(callback);

    unsigned int result = setStoreMessageCallback(handle,
                                                  callback,
                                                  &storeMessageCallback); 
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not set storage message callback, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }
    Py_RETURN_NONE;
}

static unsigned int deleteMessageCallback(void *context, char *ruleset, char *sid, char *mid) {
    unsigned int errorCode = ERR_UNEXPECTED_TYPE;
    PyObject *arglist;
    PyObject *result;
    PyObject *callback = (PyObject *)context;

    arglist = Py_BuildValue("(sss)", ruleset, sid, mid);
    result = PyEval_CallObject(callback, arglist);
    Py_DECREF(arglist);
    
    RESULT_TO_INT(result, errorCode);

    Py_XDECREF(result);   
    return errorCode;
}

static PyObject *pySetDeleteMessageCallback(PyObject *self, PyObject *args) {

    unsigned int handle;
    PyObject *callback;
    if (!PyArg_ParseTuple(args, "IO", &handle, &callback)) {
        PyErr_SetString(RulesError, "pySetDeleteMessageCallback Invalid argument");
        return NULL;
    }

    Py_XINCREF(callback);

    unsigned int result = setDeleteMessageCallback(handle,
                                                  callback,
                                                  &deleteMessageCallback); 
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not set delete message callback, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }
    Py_RETURN_NONE;
}

static unsigned int queueMessageCallback(void *context, char *ruleset, char *sid, unsigned char actionType, char *content) {
    unsigned int errorCode = ERR_UNEXPECTED_TYPE;
    PyObject *arglist;
    PyObject *result;
    PyObject *callback = (PyObject *)context;

    arglist = Py_BuildValue("(ssIs)", ruleset, sid, actionType, content);
    result = PyEval_CallObject(callback, arglist);
    Py_DECREF(arglist);
    
    RESULT_TO_INT(result, errorCode);

    Py_XDECREF(result);   
    return errorCode;
}

static PyObject *pySetQueueMessageCallback(PyObject *self, PyObject *args) {

    unsigned int handle;
    PyObject *callback;
    if (!PyArg_ParseTuple(args, "IO", &handle, &callback)) {
        PyErr_SetString(RulesError, "pySetQueueMessageCallback Invalid argument");
        return NULL;
    }

    Py_XINCREF(callback);

    unsigned int result = setQueueMessageCallback(handle,
                                                  callback,
                                                  &queueMessageCallback); 
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not set queue message callback, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }
    Py_RETURN_NONE;
}

static unsigned int getQueuedMessagesCallback(void *context, char *ruleset, char *sid) {
    unsigned int errorCode = ERR_UNEXPECTED_TYPE;
    PyObject *arglist;
    PyObject *result;
    PyObject *callback = (PyObject *)context;

    arglist = Py_BuildValue("(ss)", ruleset, sid);
    result = PyEval_CallObject(callback, arglist);
    Py_DECREF(arglist);

    RESULT_TO_INT(result, errorCode);

    Py_XDECREF(result);  
    return errorCode;
}

static PyObject *pySetGetQueuedMessagesCallback(PyObject *self, PyObject *args) {

    unsigned int handle;
    PyObject *callback;
    if (!PyArg_ParseTuple(args, "IO", &handle, &callback)) {
        PyErr_SetString(RulesError, "pySetGetQueuedMessagesCallback Invalid argument");
        return NULL;
    }

    Py_XINCREF(callback);

    unsigned int result = setGetQueuedMessagesCallback(handle,
                                                       callback,
                                                       &getQueuedMessagesCallback); 
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not set get queued messages callback, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *pyCompleteGetQueuedMessages(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *sid;
    char *messages = NULL;
    if (!PyArg_ParseTuple(args, "Izs", &handle, &sid, &messages)) {
        PyErr_SetString(RulesError, "pyCompleteGetQueuedMessages Invalid argument");
        return NULL;
    }

    unsigned int result = completeGetQueuedMessages(handle, sid, messages);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not complete get queued messages, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }

    Py_RETURN_NONE;
}

static unsigned int getIdleStateCallback(void *context, char *ruleset) {
    unsigned int errorCode = ERR_UNEXPECTED_TYPE;
    PyObject *arglist;
    PyObject *result;
    PyObject *callback = (PyObject *)context;

    arglist = Py_BuildValue("(s)", ruleset);
    result = PyEval_CallObject(callback, arglist);
    Py_DECREF(arglist);

    RESULT_TO_INT(result, errorCode);
    
    Py_XDECREF(result);   
    return errorCode;
}

static PyObject *pySetGetIdleStateCallback(PyObject *self, PyObject *args) {

    unsigned int handle;
    PyObject *callback;
    if (!PyArg_ParseTuple(args, "IO", &handle, &callback)) {
        PyErr_SetString(RulesError, "pySetGetIdleStateCallback Invalid argument");
        return NULL;
    }

    Py_XINCREF(callback);

    unsigned int result = setGetIdleStateCallback(handle,
                                                  callback,
                                                  &getIdleStateCallback); 
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not set get idle state callback, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *pyCompleteGetIdleState(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *sid;
    char *messages = NULL;
    if (!PyArg_ParseTuple(args, "Izs", &handle, &sid, &messages)) {
        PyErr_SetString(RulesError, "pyCompleteGetIdleState Invalid argument");
        return NULL;
    }

    unsigned int result = completeGetIdleState(handle, sid, messages);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not complete get idle state, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, message);
                free(message);
            }
        }
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyMethodDef myModule_methods[] = {
    {"create_ruleset", pyCreateRuleset, METH_VARARGS},
    {"delete_ruleset", pyDeleteRuleset, METH_VARARGS},
    {"assert_event", pyAssertEvent, METH_VARARGS},
    {"assert_events", pyAssertEvents, METH_VARARGS},
    {"assert_fact", pyAssertFact, METH_VARARGS},
    {"assert_facts", pyAssertFacts, METH_VARARGS},
    {"retract_fact", pyRetractFact, METH_VARARGS},
    {"retract_facts", pyRetractFacts, METH_VARARGS},
    {"update_state", pyUpdateState, METH_VARARGS},
    {"start_action", pyStartAction, METH_VARARGS},
    {"start_action_for_state", pyStartActionForState, METH_VARARGS},
    {"complete_and_start_action", pyCompleteAndStartAction, METH_VARARGS},
    {"abandon_action", pyAbandonAction, METH_VARARGS},
    {"start_timer", pyStartTimer, METH_VARARGS},
    {"cancel_timer", pyCancelTimer, METH_VARARGS},
    {"assert_timers", pyAssertTimers, METH_VARARGS},
    {"get_state", pyGetState, METH_VARARGS},
    {"delete_state", pyDeleteState, METH_VARARGS},
    {"renew_action_lease", pyRenewActionLease, METH_VARARGS},
    {"get_facts", pyGetFacts, METH_VARARGS},
    {"get_events", pyGetEvents, METH_VARARGS},
    {"set_store_message_callback", pySetStoreMessageCallback, METH_VARARGS},
    {"set_delete_message_callback", pySetDeleteMessageCallback, METH_VARARGS},
    {"set_queue_message_callback", pySetQueueMessageCallback, METH_VARARGS},
    {"set_get_queued_messages_callback", pySetGetQueuedMessagesCallback, METH_VARARGS},
    {"complete_get_queued_messages", pyCompleteGetQueuedMessages, METH_VARARGS},
    {"set_get_idle_state_callback", pySetGetIdleStateCallback, METH_VARARGS},
    {"complete_get_idle_state", pyCompleteGetIdleState, METH_VARARGS},
    {NULL, NULL}
};

#if PY_MAJOR_VERSION >= 3

static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "durable_rules_engine",
        NULL,
        -1,
        myModule_methods,
        NULL,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC PyInit_durable_rules_engine(void)
{
    PyObject *m = PyModule_Create(&moduledef);
    if (m != NULL) {
        RulesError = PyErr_NewException("durable_rules_engine.error", NULL, NULL);
        Py_INCREF(RulesError);
        PyModule_AddObject(m, "error", RulesError);
    }

    return m;
}

#else

PyMODINIT_FUNC initdurable_rules_engine(void)
{
    PyObject *m = Py_InitModule("durable_rules_engine", myModule_methods);
    if (m != NULL) {
        RulesError = PyErr_NewException("durable_rules_engine.error", NULL, NULL);
        Py_INCREF(RulesError);
        PyModule_AddObject(m, "error", RulesError);
    }
}

#endif
