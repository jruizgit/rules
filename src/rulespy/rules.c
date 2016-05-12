#include <Python.h>
#include <rules.h>

static PyObject *RulesError;

static PyObject *pyCreateRuleset(PyObject *self, PyObject *args) {
    char *name;
    char *rules;
    unsigned int stateCacheSize;
    if (!PyArg_ParseTuple(args, "lss", &stateCacheSize, &name, &rules)) {
        PyErr_SetString(RulesError, "pyCreateRuleset Invalid argument");
        return NULL;
    }

    void *output = NULL;
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

    return Py_BuildValue("l", output);
}

static PyObject *pyDeleteRuleset(PyObject *self, PyObject *args) {
    void *handle;
    if (!PyArg_ParseTuple(args, "l", &handle)) {
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
    if (!PyArg_ParseTuple(args, "ls", &stateCacheSize, &name)) {
        PyErr_SetString(RulesError, "pyCreateRuleset Invalid argument");
        return NULL;
    }

    void *output = NULL;
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

    return Py_BuildValue("l", output);
}

static PyObject *pyDeleteClient(PyObject *self, PyObject *args) {
    void *handle;
    if (!PyArg_ParseTuple(args, "l", &handle)) {
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
    void *handle;
    char *host;
    unsigned int port;
    char *password = NULL;
    unsigned int result;
    if (PyArg_ParseTuple(args, "zlsl", &password, &port, &host, &handle)) {
        result = bindRuleset(handle, host, port, password);
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
    if (PyArg_ParseTuple(args, "li", &rulesBinding, &replyCount)) {
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
    void *handle;
    char *event;
    if (!PyArg_ParseTuple(args, "ls", &handle, &event)) {
        PyErr_SetString(RulesError, "pyAssertEvent Invalid argument");
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
    void *handle;
    char *sid = NULL;
    char *destination = NULL;
    char *event = NULL;
    if (!PyArg_ParseTuple(args, "lsss", &handle, &sid, &destination, &event)) {
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
    void *handle;
    char *event;
    if (!PyArg_ParseTuple(args, "ls", &handle, &event)) {
        PyErr_SetString(RulesError, "pyStartAssertEvent Invalid argument");
        return NULL;
    }

    unsigned int replyCount;
    void *rulesBinding = NULL;
    unsigned int result = startAssertEvent(handle, event, &rulesBinding, &replyCount);
    if (result == RULES_OK) {
        return Py_BuildValue("li", rulesBinding, replyCount);    
    } else if (result == ERR_EVENT_NOT_HANDLED) {
        return Py_BuildValue("li", 0, 0);    
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
    void *handle;
    char *events;
    if (!PyArg_ParseTuple(args, "ls", &handle, &events)) {
        PyErr_SetString(RulesError, "pyAssertEvents Invalid argument");
        return NULL;
    }

    unsigned int *results = NULL;
    unsigned int resultsLength;
    unsigned int result = assertEvents(handle, events, &resultsLength, &results);
    if (result == RULES_OK) {
        if (results) {
            free(results);
        }
        return Py_BuildValue("i", resultsLength);
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else { 
            if (results) {
                free(results);
            }
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
    void *handle;
    char *events;
    if (!PyArg_ParseTuple(args, "ls", &handle, &events)) {
        PyErr_SetString(RulesError, "pyStartAssertEvents Invalid argument");
        return NULL;
    }

    unsigned int replyCount;
    void *rulesBinding = NULL;
    unsigned int *results = NULL;
    unsigned int resultsLength;
    unsigned int result = startAssertEvents(handle, events, &resultsLength, &results, &rulesBinding, &replyCount);
    if (result == RULES_OK) {  
        if (results) {
            free(results);
        }
        return Py_BuildValue("li", rulesBinding, replyCount);    
    } else if (result == ERR_EVENT_NOT_HANDLED) {
        return Py_BuildValue("li", 0, 0);    
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
    void *handle;
    char *event;
    if (!PyArg_ParseTuple(args, "ls", &handle, &event)) {
        PyErr_SetString(RulesError, "pyRetractEvent Invalid argument");
        return NULL;
    }

    unsigned int result = retractEvent(handle, event);
    if (result == RULES_OK) {
        return Py_BuildValue("i", 1);    
    } else if (result == ERR_EVENT_NOT_HANDLED) {
        return Py_BuildValue("i", 0);    
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
    void *handle;
    char *fact;
    if (!PyArg_ParseTuple(args, "ls", &handle, &fact)) {
        PyErr_SetString(RulesError, "pyAssertFact Invalid argument");
        return NULL;
    }

    unsigned int result = assertFact(handle, fact);
    if (result == RULES_OK) {
        return Py_BuildValue("i", 1);    
    } else if (result == ERR_EVENT_NOT_HANDLED) {
        return Py_BuildValue("i", 0);    
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
    void *handle;
    char *sid = NULL;
    char *destination = NULL;
    char *event = NULL;
    if (!PyArg_ParseTuple(args, "lsss", &handle, &sid, &destination, &event)) {
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
    void *handle;
    char *fact;
    if (!PyArg_ParseTuple(args, "ls", &handle, &fact)) {
        PyErr_SetString(RulesError, "pyStartAssertFact Invalid argument");
        return NULL;
    }

    unsigned int replyCount;
    void *rulesBinding = NULL;
    unsigned int result = startAssertFact(handle, fact, &rulesBinding, &replyCount);
    if (result == RULES_OK) {
        return Py_BuildValue("li", rulesBinding, replyCount);    
    } else if (result == ERR_EVENT_NOT_HANDLED) {
        return Py_BuildValue("li", 0, 0);    
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
    void *handle;
    char *facts;
    if (!PyArg_ParseTuple(args, "ls", &handle, &facts)) {
        PyErr_SetString(RulesError, "pyAssertFacts Invalid argument");
        return NULL;
    }

    unsigned int *results = NULL;
    unsigned int resultsLength;
    unsigned int result = assertFacts(handle, facts, &resultsLength, &results);
    if (result == RULES_OK) {   
        if (results) {
            free(results);
        }
        return Py_BuildValue("i", resultsLength);
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else {
            if (results) {
                free(results);
            } 
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
    void *handle;
    char *facts;
    if (!PyArg_ParseTuple(args, "ls", &handle, &facts)) {
        PyErr_SetString(RulesError, "pyStartAssertFacts Invalid argument");
        return NULL;
    }

    unsigned int replyCount;
    void *rulesBinding = NULL;
    unsigned int *results = NULL;
    unsigned int resultsLength;
    unsigned int result = startAssertFacts(handle, facts, &resultsLength, &results, &rulesBinding, &replyCount);
    if (result == RULES_OK) {  
        if (results) {
            free(results);
        }
        return Py_BuildValue("li", rulesBinding, replyCount);    
    } else if (result == ERR_EVENT_NOT_HANDLED) {
        return Py_BuildValue("li", 0, 0);    
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
    void *handle;
    char *fact;
    if (!PyArg_ParseTuple(args, "ls", &handle, &fact)) {
        PyErr_SetString(RulesError, "pyRetractFact Invalid argument");
        return NULL;
    }

    unsigned int result = retractFact(handle, fact);
    if (result == RULES_OK) {
        return Py_BuildValue("i", 1);    
    } else if (result == ERR_EVENT_NOT_HANDLED) {
        return Py_BuildValue("i", 0);    
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
    void *handle;
    char *sid = NULL;
    char *destination = NULL;
    char *event = NULL;
    if (!PyArg_ParseTuple(args, "lsss", &handle, &sid, &destination, &event)) {
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
    void *handle;
    char *fact;
    if (!PyArg_ParseTuple(args, "ls", &handle, &fact)) {
        PyErr_SetString(RulesError, "pyStartRetractFact Invalid argument");
        return NULL;
    }

    unsigned int replyCount;
    void *rulesBinding = NULL;
    unsigned int result = startRetractFact(handle, fact, &rulesBinding, &replyCount);
    if (result == RULES_OK) {
        return Py_BuildValue("li", rulesBinding, replyCount);    
    } else if (result == ERR_EVENT_NOT_HANDLED) {
        return Py_BuildValue("li", 0, 0);    
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
    void *handle;
    char *facts;
    if (!PyArg_ParseTuple(args, "ls", &handle, &facts)) {
        PyErr_SetString(RulesError, "pyAssertFacts Invalid argument");
        return NULL;
    }

    unsigned int *results = NULL;
    unsigned int resultsLength;
    unsigned int result = retractFacts(handle, facts, &resultsLength, &results);
    if (result == RULES_OK) {   
        if (results) {
            free(results);
        }
        return Py_BuildValue("i", resultsLength);
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            PyErr_NoMemory();
        } else {
            if (results) {
                free(results);
            } 
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
    void *handle;
    char *facts;
    if (!PyArg_ParseTuple(args, "ls", &handle, &facts)) {
        PyErr_SetString(RulesError, "pyStartRetractFacts Invalid argument");
        return NULL;
    }

    unsigned int replyCount;
    void *rulesBinding = NULL;
    unsigned int *results = NULL;
    unsigned int resultsLength;
    unsigned int result = startRetractFacts(handle, facts, &resultsLength, &results, &rulesBinding, &replyCount);
    if (result == RULES_OK) {  
        if (results) {
            free(results);
        }
        return Py_BuildValue("li", rulesBinding, replyCount);    
    } else if (result == ERR_EVENT_NOT_HANDLED) {
        return Py_BuildValue("li", 0, 0);    
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
    void *handle;
    char *state;
    if (!PyArg_ParseTuple(args, "ls", &handle, &state)) {
        PyErr_SetString(RulesError, "pyAssertState Invalid argument");
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
    void *handle;
    void *actionHandle;
    char *state;
    if (!PyArg_ParseTuple(args, "lls", &handle, &actionHandle, &state)) {
        PyErr_SetString(RulesError, "pyStartUpdateState Invalid argument");
        return NULL;
    }

    unsigned int replyCount;
    void *rulesBinding = NULL;
    unsigned int result = startUpdateState(handle, actionHandle, state, &rulesBinding, &replyCount);
    if (result == RULES_OK) {
        return Py_BuildValue("li", rulesBinding, replyCount);    
    } else if (result == ERR_EVENT_NOT_HANDLED) {
        return Py_BuildValue("li", 0, 0);    
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
    void *handle;
    if (!PyArg_ParseTuple(args, "l", &handle)) {
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

    PyObject *returnValue = Py_BuildValue("ssll", state, messages, actionHandle, actionBinding);
    return returnValue;
}

static PyObject *pyCompleteAction(PyObject *self, PyObject *args) {
    void *handle;
    void *actionHandle;
    char *state;
    if (!PyArg_ParseTuple(args, "lls", &handle, &actionHandle, &state)) {
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
    void *handle;
    void *actionHandle;
    unsigned int expectedReplies;
    if (!PyArg_ParseTuple(args, "lll", &handle, &expectedReplies, &actionHandle)) {
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
    void *handle;
    void *actionHandle;
    if (!PyArg_ParseTuple(args, "ll", &handle, &actionHandle)) {
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
    void *handle;
    char *sid;
    int duration = 0;
    char *timer = NULL;
    if (!PyArg_ParseTuple(args, "lsis", &handle, &sid, &duration, &timer)) {
        PyErr_SetString(RulesError, "pyStartTimer Invalid argument");
        return NULL;
    }

    unsigned int result = startTimer(handle, sid, duration, timer);
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
    void *handle;
    char *sid;
    char *timer = NULL;
    if (!PyArg_ParseTuple(args, "lss", &handle, &sid, &timer)) {
        PyErr_SetString(RulesError, "pyCancelTimer Invalid argument");
        return NULL;
    }

    unsigned int result = cancelTimer(handle, sid, timer);
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
    void *handle;
    if (!PyArg_ParseTuple(args, "l", &handle)) {
        PyErr_SetString(RulesError, "pyAssertTimers Invalid argument");
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
    void *handle;
    char *sid;
    if (!PyArg_ParseTuple(args, "ls", &handle, &sid)) {
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
    void *handle;
    char *sid;
    if (!PyArg_ParseTuple(args, "ls", &handle, &sid)) {
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
    void *handle;
    char *sid;
    if (!PyArg_ParseTuple(args, "ls", &handle, &sid)) {
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

PyMODINIT_FUNC initrules(void)
{
    PyObject *m = Py_InitModule("rules", myModule_methods);
    if (m != NULL) {
        RulesError = PyErr_NewException("rules.error", NULL, NULL);
        Py_INCREF(RulesError);
        PyModule_AddObject(m, "error", RulesError);
    }
}