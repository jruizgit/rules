#include <Python.h>
#include <rules.h>

static PyObject *RulesError;

#define PRINT_ARGS(args) do { \
    PyObject* or = PyObject_Repr(args); \
    const char* s = PyString_AsString(or); \
    printf("args %s\n", s); \
} while(0)

static PyObject *pyCreateRuleset(PyObject *self, PyObject *args) {
    char *name;
    char *rules;
    unsigned int stateCacheSize;
    if (!PyArg_ParseTuple(args, "Iss", &stateCacheSize, &name, &rules)) {
        PyErr_SetString(RulesError, "pyCreateRuleset Invalid argument");
        return NULL;
    }

    unsigned int output = 0;
    unsigned int result = createRuleset(&output, name, rules, stateCacheSize);
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

static PyObject *pyCreateClient(PyObject *self, PyObject *args) {
    char *name;
    unsigned int stateCacheSize;
    if (!PyArg_ParseTuple(args, "Is", &stateCacheSize, &name)) {
        PyErr_SetString(RulesError, "pyCreateRuleset Invalid argument");
        return NULL;
    }

    unsigned int output = 0;
    unsigned int result = createClient(&output, name, stateCacheSize);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not create client, error code: %d", result) == -1) {
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

static PyObject *pyDeleteClient(PyObject *self, PyObject *args) {
    unsigned int handle;
    if (!PyArg_ParseTuple(args, "I", &handle)) {
        PyErr_SetString(RulesError, "pyDeleteClient Invalid argument");
        return NULL;
    }

    unsigned int result = deleteClient(handle);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not delete client, error code: %d", result) == -1) {
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

static PyObject *pyBindRuleset(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *host;
    unsigned int port;
    char *password = NULL;
    unsigned int db;
    unsigned int result;
    if (PyArg_ParseTuple(args, "IIszI", &port, &db, &host, &password, &handle)) {
        result = bindRuleset(handle, host, port, password, db);
    } else {
        PyErr_SetString(RulesError, "pyBindRuleset Invalid argument");
        return NULL;
    }

    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not create connection, error code: %d", result) == -1) {
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

static PyObject *pyComplete(PyObject *self, PyObject *args) {
    void *rulesBinding = NULL;
    unsigned int replyCount = 0;
    unsigned int result;
    if (PyArg_ParseTuple(args, "KI", &rulesBinding, &replyCount)) {
        result = complete(rulesBinding, replyCount);
    } else {
        PyErr_SetString(RulesError, "pyComplete Invalid argument");
        return NULL;
    }

    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not complete, error code: %d", result) == -1) {
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
    
    unsigned int result = assertEvent(handle, event);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED) {
        return Py_BuildValue("I", result);
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

static PyObject *pyQueueAssertEvent(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *sid = NULL;
    char *destination = NULL;
    char *event = NULL;
    if (!PyArg_ParseTuple(args, "Izss", &handle, &sid, &destination, &event)) {
        PyErr_SetString(RulesError, "pyQueueAssertEvent Invalid argument");
        return NULL;
    }

    unsigned int result = queueMessage(handle, QUEUE_ASSERT_EVENT, sid, destination, event);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *resultMessage;
            if (asprintf(&resultMessage, "Could not queue assert event, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, resultMessage);
                free(resultMessage);
            }
        }
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *pyStartAssertEvent(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *event;
    if (!PyArg_ParseTuple(args, "Is", &handle, &event)) {
        PyErr_SetString(RulesError, "pyStartAssertEvent Invalid argument");
        return NULL;
    }

    unsigned int replyCount;
    void *rulesBinding = NULL;
    unsigned int result = startAssertEvent(handle, event, &rulesBinding, &replyCount);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED) {
        return Py_BuildValue("KI", rulesBinding, replyCount);    
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

    unsigned int result = assertEvents(handle, events);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED) {
        return Py_BuildValue("I", result);
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

static PyObject *pyStartAssertEvents(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *events;
    if (!PyArg_ParseTuple(args, "Is", &handle, &events)) {
        PyErr_SetString(RulesError, "pyStartAssertEvents Invalid argument");
        return NULL;
    }

    unsigned int replyCount;
    void *rulesBinding = NULL;
    unsigned int result = startAssertEvents(handle, events, &rulesBinding, &replyCount);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED) { 
        return Py_BuildValue("Ki", rulesBinding, replyCount);    
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

static PyObject *pyRetractEvent(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *event;
    if (!PyArg_ParseTuple(args, "Is", &handle, &event)) {
        PyErr_SetString(RulesError, "pyRetractEvent Invalid argument");
        return NULL;
    }

    unsigned int result = retractEvent(handle, event);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED) {
        return Py_BuildValue("I", result);
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not retract event, error code: %d", result) == -1) {
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

    unsigned int result = assertFact(handle, fact);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED) {
        return Py_BuildValue("I", result);
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

static PyObject *pyQueueAssertFact(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *sid = NULL;
    char *destination = NULL;
    char *event = NULL;
    if (!PyArg_ParseTuple(args, "Izss", &handle, &sid, &destination, &event)) {
        PyErr_SetString(RulesError, "pyQueueAssertFact Invalid argument");
        return NULL;
    }

    unsigned int result = queueMessage(handle, QUEUE_ASSERT_FACT, sid, destination, event);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *resultMessage;
            if (asprintf(&resultMessage, "Could not queue assert fact, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, resultMessage);
                free(resultMessage);
            }
        }
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *pyStartAssertFact(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *fact;
    if (!PyArg_ParseTuple(args, "Is", &handle, &fact)) {
        PyErr_SetString(RulesError, "pyStartAssertFact Invalid argument");
        return NULL;
    }

    unsigned int replyCount;
    void *rulesBinding = NULL;
    unsigned int result = startAssertFact(handle, fact, &rulesBinding, &replyCount);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED) {
        return Py_BuildValue("KI", rulesBinding, replyCount);    
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

    unsigned int result = assertFacts(handle, facts);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED) {
        return Py_BuildValue("I", result);
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

static PyObject *pyStartAssertFacts(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *facts;
    if (!PyArg_ParseTuple(args, "Is", &handle, &facts)) {
        PyErr_SetString(RulesError, "pyStartAssertFacts Invalid argument");
        return NULL;
    }

    unsigned int replyCount;
    void *rulesBinding = NULL;
    unsigned int result = startAssertFacts(handle, facts, &rulesBinding, &replyCount);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED) {  
        return Py_BuildValue("Ki", rulesBinding, replyCount);    
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

    unsigned int result = retractFact(handle, fact);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED) {
        return Py_BuildValue("I", result);
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

static PyObject *pyQueueRetractFact(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *sid = NULL;
    char *destination = NULL;
    char *event = NULL;
    if (!PyArg_ParseTuple(args, "Izss", &handle, &sid, &destination, &event)) {
        PyErr_SetString(RulesError, "pyQueueRetractFact Invalid argument");
        return NULL;
    }

    unsigned int result = queueMessage(handle, QUEUE_RETRACT_FACT, sid, destination, event);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *resultMessage;
            if (asprintf(&resultMessage, "Could not queue retract fact, error code: %d", result) == -1) {
                PyErr_NoMemory();
            } else {
                PyErr_SetString(RulesError, resultMessage);
                free(resultMessage);
            }
        }
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *pyStartRetractFact(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *fact;
    if (!PyArg_ParseTuple(args, "Is", &handle, &fact)) {
        PyErr_SetString(RulesError, "pyStartRetractFact Invalid argument");
        return NULL;
    }

    unsigned int replyCount;
    void *rulesBinding = NULL;
    unsigned int result = startRetractFact(handle, fact, &rulesBinding, &replyCount);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED) {
        return Py_BuildValue("Ki", rulesBinding, replyCount);    
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

    unsigned int result = retractFacts(handle, facts);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED) {
        return Py_BuildValue("I", result);
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

static PyObject *pyStartRetractFacts(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *facts;
    if (!PyArg_ParseTuple(args, "Is", &handle, &facts)) {
        PyErr_SetString(RulesError, "pyStartRetractFacts Invalid argument");
        return NULL;
    }

    unsigned int replyCount;
    void *rulesBinding = NULL;
    unsigned int result = startRetractFacts(handle, facts, &rulesBinding, &replyCount);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED) {  
        return Py_BuildValue("Ki", rulesBinding, replyCount);    
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

static PyObject *pyAssertState(PyObject *self, PyObject *args) {
    unsigned int handle;
    char *state;
    char *sid;
    if (!PyArg_ParseTuple(args, "Izs", &handle, &sid, &state)) {
        PyErr_SetString(RulesError, "pyAssertState Invalid argument");
        return NULL;
    }

    unsigned int result = assertState(handle, sid, state);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED) {
        return Py_BuildValue("I", result);
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

static PyObject *pyStartUpdateState(PyObject *self, PyObject *args) {
    unsigned int handle;
    void *actionHandle;
    char *state;
    if (!PyArg_ParseTuple(args, "IKs", &handle, &actionHandle, &state)) {
        PyErr_SetString(RulesError, "pyStartUpdateState Invalid argument");
        return NULL;
    }

    unsigned int replyCount;
    void *rulesBinding = NULL;
    unsigned int result = startUpdateState(handle, actionHandle, state, &rulesBinding, &replyCount);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED) {
        return Py_BuildValue("Ki", rulesBinding, replyCount);    
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not update state, error code: %d", result) == -1) {
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
    void *actionHandle;
    void *actionBinding;
    unsigned int result = startAction(handle, &state, &messages, &actionHandle, &actionBinding);
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

    PyObject *returnValue = Py_BuildValue("ssKK", state, messages, actionHandle, actionBinding);
    return returnValue;
}

static PyObject *pyCompleteAction(PyObject *self, PyObject *args) {
    unsigned int handle;
    void *actionHandle;
    char *state;
    if (!PyArg_ParseTuple(args, "IKs", &handle, &actionHandle, &state)) {
        PyErr_SetString(RulesError, "pyCompleteAction Invalid argument");
        return NULL;
    }

    unsigned int result = completeAction(handle, actionHandle, state);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            char *message;
            if (asprintf(&message, "Could not complete action, error code: %d", result) == -1) {
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

static PyObject *pyCompleteAndStartAction(PyObject *self, PyObject *args) {
    unsigned int handle;
    void *actionHandle;
    unsigned int expectedReplies;
    if (!PyArg_ParseTuple(args, "IIK", &handle, &expectedReplies, &actionHandle)) {
        PyErr_SetString(RulesError, "pyCompleteAndStartAction Invalid argument");
        return NULL;
    }

    char *messages;
    unsigned int result = completeAndStartAction(handle, expectedReplies, actionHandle, &messages);
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
    void *actionHandle;
    if (!PyArg_ParseTuple(args, "IK", &handle, &actionHandle)) {
        PyErr_SetString(RulesError, "pyAbandonAction Invalid argument");
        return NULL;
    }

    unsigned int result = abandonAction(handle, actionHandle);
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
    if (result == RULES_OK) {
        return Py_BuildValue("I", 1);    
    } else if (result == ERR_NO_TIMERS_AVAILABLE) {
        return Py_BuildValue("I", 0);    
    } else {
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

static PyMethodDef myModule_methods[] = {
    {"create_ruleset", pyCreateRuleset, METH_VARARGS},
    {"delete_ruleset", pyDeleteRuleset, METH_VARARGS},
    {"create_client", pyCreateClient, METH_VARARGS},
    {"delete_client", pyDeleteClient, METH_VARARGS},
    {"bind_ruleset", pyBindRuleset, METH_VARARGS},
    {"complete", pyComplete, METH_VARARGS},
    {"assert_event", pyAssertEvent, METH_VARARGS},
    {"queue_assert_event", pyQueueAssertEvent, METH_VARARGS},
    {"start_assert_event", pyStartAssertEvent, METH_VARARGS},
    {"assert_events", pyAssertEvents, METH_VARARGS},
    {"start_assert_events", pyStartAssertEvents, METH_VARARGS},
    {"retract_event", pyRetractEvent, METH_VARARGS},
    {"start_assert_fact", pyStartAssertFact, METH_VARARGS},
    {"assert_fact", pyAssertFact, METH_VARARGS},
    {"queue_assert_fact", pyQueueAssertFact, METH_VARARGS},
    {"start_assert_facts", pyStartAssertFacts, METH_VARARGS},
    {"assert_facts", pyAssertFacts, METH_VARARGS},
    {"retract_fact", pyRetractFact, METH_VARARGS},
    {"queue_retract_fact", pyQueueRetractFact, METH_VARARGS},
    {"start_retract_fact", pyStartRetractFact, METH_VARARGS},
    {"retract_facts", pyRetractFacts, METH_VARARGS},
    {"start_retract_facts", pyStartRetractFacts, METH_VARARGS},
    {"assert_state", pyAssertState, METH_VARARGS},
    {"start_update_state", pyStartUpdateState, METH_VARARGS},
    {"start_action", pyStartAction, METH_VARARGS},
    {"complete_action", pyCompleteAction, METH_VARARGS},
    {"complete_and_start_action", pyCompleteAndStartAction, METH_VARARGS},
    {"abandon_action", pyAbandonAction, METH_VARARGS},
    {"start_timer", pyStartTimer, METH_VARARGS},
    {"cancel_timer", pyCancelTimer, METH_VARARGS},
    {"assert_timers", pyAssertTimers, METH_VARARGS},
    {"get_state", pyGetState, METH_VARARGS},
    {"delete_state", pyDeleteState, METH_VARARGS},
    {"renew_action_lease", pyRenewActionLease, METH_VARARGS},
    {NULL, NULL}
};

#if PY_MAJOR_VERSION >= 3

static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "rules",
        NULL,
        -1,
        myModule_methods,
        NULL,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC PyInit_rules(void)
{
    PyObject *m = PyModule_Create(&moduledef);
    if (m != NULL) {
        RulesError = PyErr_NewException("rules.error", NULL, NULL);
        Py_INCREF(RulesError);
        PyModule_AddObject(m, "error", RulesError);
    }

    return m;
}

#else

PyMODINIT_FUNC initrules(void)
{
    PyObject *m = Py_InitModule("rules", myModule_methods);
    if (m != NULL) {
        RulesError = PyErr_NewException("rules.error", NULL, NULL);
        Py_INCREF(RulesError);
        PyModule_AddObject(m, "error", RulesError);
    }
}

#endif
