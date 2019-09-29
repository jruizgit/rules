
#include <node.h>
#include <v8.h>
#include <rules.h>
#include <stdlib.h>

using namespace v8;

#define TO_STRING(i, value)((value->IsNull() || value->IsUndefined())? NULL: *v8::String::Utf8Value(i, value->ToString(i->GetCurrentContext()).FromMaybe(v8::Local<v8::String>())))

#define CREATE_STRING(i, value) String::NewFromUtf8(i, value, v8::NewStringType::kNormal).ToLocalChecked()

#define TO_NUMBER(i, value) value->IntegerValue(i->GetCurrentContext()).FromJust()

#define ARRAY_SET_NUMBER(i, array, index, value) array->Set(i->GetCurrentContext(), index, Number::New(i, value)).Check()

#define ARRAY_SET_STRING(i, array, index, value) array->Set(i->GetCurrentContext(), index, String::NewFromUtf8(i, value, v8::NewStringType::kNormal).ToLocalChecked()).Check()

#define CALL_FUNCTION(func, i, nargs, args) func->Call(i->GetCurrentContext(), i->GetCurrentContext()->Global(), nargs, args).ToLocalChecked() 

#define DEFINE_FUNCTION(e, i, name, func) e->Set(i->GetCurrentContext(), String::NewFromUtf8(i, name, v8::NewStringType::kInternalized).ToLocalChecked(), FunctionTemplate::New(i, func)->GetFunction(i->GetCurrentContext()).ToLocalChecked()).Check()

class ObjectProxy {
public:

    explicit ObjectProxy(Local<Function> gvalue, Local<Function> svalue) { 
        Isolate* isolate = Isolate::GetCurrent();
        _gfunc.Reset(isolate, gvalue); 
        _sfunc.Reset(isolate, svalue); 
    }
    Local<Object> Wrap();

private:

    static ObjectProxy* Unwrap(Local<Object> obj);
    static void Get(Local<Name> name, const PropertyCallbackInfo<Value>& info);
    static void Set(Local<Name> name, Local<Value> value, const PropertyCallbackInfo<Value>& info);
    
    Persistent<Function> _gfunc;
    Persistent<Function> _sfunc;
};


class CallbackProxy {
public:

    explicit CallbackProxy(Local<Function> value) { 
        Isolate* isolate = Isolate::GetCurrent();
        func.Reset(isolate, value); 

    }
    
    Persistent<Function> func;
};

void jsCreateRuleset(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsString() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        unsigned int output = 0;
        unsigned int result = createRuleset(&output, 
                                            TO_STRING(isolate, args[0]), 
                                            TO_STRING(isolate, args[1]));
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not create ruleset, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            }
        } else {
            args.GetReturnValue().Set(output);
        }
    }
}

void jsDeleteRuleset(const FunctionCallbackInfo<Value>& args) {
    v8::Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 1) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        unsigned int result = deleteRuleset(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust()); 
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not delete ruleset, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            }
        }
    }
}

void jsAssertEvent(const FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        unsigned int stateOffset;
        unsigned int result = assertEvent(TO_NUMBER(isolate, args[0]),
                                          TO_STRING(isolate, args[1]),
                                          &stateOffset);
        if (result == RULES_OK || result == ERR_EVENT_OBSERVED || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_DEFERRED) {
            Local<Array> array = Array::New(isolate, 2);
            ARRAY_SET_NUMBER(isolate, array, 0, result);
            ARRAY_SET_NUMBER(isolate, array, 1, stateOffset);
            args.GetReturnValue().Set(array);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not assert event, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            } 
        } 
    }
}

void jsAssertEvents(const FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        unsigned int stateOffset;
        unsigned int result = assertEvents(TO_NUMBER(isolate, args[0]), 
                                           TO_STRING(isolate, args[1]),
                                           &stateOffset);
        if (result == RULES_OK || result == ERR_EVENT_OBSERVED || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_DEFERRED) {
            Local<Array> array = Array::New(isolate, 2);
            ARRAY_SET_NUMBER(isolate, array, 0, result);
            ARRAY_SET_NUMBER(isolate, array, 1, stateOffset);
            args.GetReturnValue().Set(array);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not assert events, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            } 
        } 
    }
}

void jsAssertFact(const FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        unsigned int stateOffset;
        unsigned int result = assertFact(TO_NUMBER(isolate, args[0]), 
                                         TO_STRING(isolate, args[1]),
                                         &stateOffset);
        if (result == RULES_OK || result == ERR_EVENT_OBSERVED || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_DEFERRED) {
            Local<Array> array = Array::New(isolate, 2);
            ARRAY_SET_NUMBER(isolate, array, 0, result);
            ARRAY_SET_NUMBER(isolate, array, 1, stateOffset);
            args.GetReturnValue().Set(array);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not assert events, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            } 
        } 
    }
}

void jsAssertFacts(const FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        unsigned int stateOffset;
        unsigned int result = assertFacts(TO_NUMBER(isolate, args[0]), 
                                          TO_STRING(isolate, args[1]),
                                          &stateOffset);
        if (result == RULES_OK || result == ERR_EVENT_OBSERVED || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_DEFERRED) {
            Local<Array> array = Array::New(isolate, 2);
            ARRAY_SET_NUMBER(isolate, array, 0, result);
            ARRAY_SET_NUMBER(isolate, array, 1, stateOffset);
            args.GetReturnValue().Set(array);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not assert facts, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            } 
        } 
    }
}

void jsRetractFact(const FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        unsigned int stateOffset;
        unsigned int result = retractFact(TO_NUMBER(isolate, args[0]), 
                                          TO_STRING(isolate, args[1]),
                                          &stateOffset);
        if (result == RULES_OK || result == ERR_EVENT_OBSERVED || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_DEFERRED) {
            Local<Array> array = Array::New(isolate, 2);
            ARRAY_SET_NUMBER(isolate, array, 0, result);
            ARRAY_SET_NUMBER(isolate, array, 1, stateOffset);
            args.GetReturnValue().Set(array);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not retract fact, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            } 
        } 
    }
}

void jsRetractFacts(const FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        unsigned int stateOffset;
        unsigned int result = retractFacts(TO_NUMBER(isolate, args[0]), 
                                           TO_STRING(isolate, args[1]),
                                           &stateOffset);
        if (result == RULES_OK || result == ERR_EVENT_OBSERVED || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_DEFERRED) {
            Local<Array> array = Array::New(isolate, 2);
            ARRAY_SET_NUMBER(isolate, array, 0, result);
            ARRAY_SET_NUMBER(isolate, array, 1, stateOffset);
            args.GetReturnValue().Set(array);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not retract facts, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            } 
        } 
    }
}

void jsUpdateState(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        unsigned int stateOffset;
        unsigned int result = updateState(TO_NUMBER(isolate, args[0]),
                                          TO_STRING(isolate, args[1]),
                                          &stateOffset);
        if (result == RULES_OK || result == ERR_EVENT_DEFERRED) {
            Local<Array> array = Array::New(isolate, 2);
            ARRAY_SET_NUMBER(isolate, array, 0, result);
            ARRAY_SET_NUMBER(isolate, array, 1, stateOffset);
            args.GetReturnValue().Set(array);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not update state, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            }
        } 
    }
}

void jsStartAction(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 1) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        char *session;
        char *messages;
        unsigned int stateOffset;
        unsigned int result = startAction(TO_NUMBER(isolate, args[0]), 
                                          &session, 
                                          &messages, 
                                          &stateOffset); 
        if (result == RULES_OK) {
            Local<Array> array = Array::New(isolate, 3);
            ARRAY_SET_STRING(isolate, array, 0, session);
            ARRAY_SET_STRING(isolate, array, 1, messages);
            ARRAY_SET_NUMBER(isolate, array, 2, stateOffset);
            args.GetReturnValue().Set(array);
        } else if (result != ERR_NO_ACTION_AVAILABLE) {
            char *message = NULL;
            if (asprintf(&message, "Could not start action, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            }
        }
    }
}

void jsStartActionForState(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsNumber()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        char *session;
        char *messages;
        unsigned int result = startActionForState(TO_NUMBER(isolate, args[0]), 
                                                  TO_NUMBER(isolate, args[1]),
                                                  &session, 
                                                  &messages); 
        if (result == RULES_OK) {
            Local<Array> array = Array::New(isolate, 2);
            ARRAY_SET_STRING(isolate, array, 0, session);
            ARRAY_SET_STRING(isolate, array, 1, messages);
            args.GetReturnValue().Set(array);
        } else if (result != ERR_NO_ACTION_AVAILABLE) {
            char *message = NULL;
            if (asprintf(&message, "Could not start action, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            }
        }
    }
}

void jsCompleteAndStartAction(const FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsNumber()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        char *messages = NULL;
        unsigned int result = completeAndStartAction(TO_NUMBER(isolate, args[0]),
                                                     TO_NUMBER(isolate, args[1]),
                                                     &messages);
        if (result == RULES_OK) {
            args.GetReturnValue().Set(CREATE_STRING(isolate, messages));
        } else if (result != ERR_NO_ACTION_AVAILABLE) {
            char *message = NULL;
            if (asprintf(&message, "Could not complete and start action, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            }
        } 
    }
}

void jsAbandonAction(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsNumber()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        unsigned int result = abandonAction(TO_NUMBER(isolate, args[0]),
                                            TO_NUMBER(isolate, args[1]));
        
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not abandon action, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            }
        } 
    }
}

void jsStartTimer(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 5) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[2]->IsNumber() || !args[3]->IsNumber() || !args[4]->IsString()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        unsigned int result = startTimer(TO_NUMBER(isolate, args[0]),
                                         TO_STRING(isolate, args[1]),
                                         TO_NUMBER(isolate, args[2]),
                                         TO_NUMBER(isolate, args[3]),
                                         TO_STRING(isolate, args[4]));
        
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not start timer, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            }
        } 
    }
}

void jsCancelTimer(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 3) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[2]->IsString()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        unsigned int result = cancelTimer(TO_NUMBER(isolate, args[0]),
                                          TO_STRING(isolate, args[1]),
                                          TO_STRING(isolate, args[2]));
        
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not cancel timer, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            }
        } 
    }
}

void jsAssertTimers(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 1) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        unsigned int result = assertTimers(TO_NUMBER(isolate, args[0]));
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not assert timers, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            }
        } 
    }
}

void jsGetState(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        char *state;
        unsigned int result = getState(TO_NUMBER(isolate, args[0]), 
                                       TO_STRING(isolate, args[1]), 
                                       &state); 
        if (result == RULES_OK) {
            args.GetReturnValue().Set(CREATE_STRING(isolate, state));
            free(state);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not get state, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            }
        }
    }
}

void jsDeleteState(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        unsigned int result = deleteState(TO_NUMBER(isolate, args[0]), 
                                          TO_STRING(isolate, args[1])); 
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not delete state, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            }
        }
    }
}

void jsRenewActionLease(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        unsigned int result = renewActionLease(TO_NUMBER(isolate, args[0]), 
                                               TO_STRING(isolate, args[1])); 
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not renew action lease, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            }
        }
    }
}

static unsigned int storeMessageCallback(void *context, char *ruleset, char *sid, char *mid, unsigned char messageType, char *content) {
    CallbackProxy *p = (CallbackProxy *)context;
    Isolate* isolate = Isolate::GetCurrent();
     
    Local<Value> args[5];
    args[0] = CREATE_STRING(isolate, ruleset);
    args[1] = CREATE_STRING(isolate, sid);
    args[2] = CREATE_STRING(isolate, mid);
    args[3] = Number::New(isolate, messageType);
    args[4] = CREATE_STRING(isolate, content);


    Local<Function> func = Local<Function>::New(isolate, p->func);
    Local<Value> result = CALL_FUNCTION(func, isolate, 5, args);
    if (result->IsNumber()) {  
        return TO_NUMBER(isolate, result);
    } else {
        return ERR_UNEXPECTED_TYPE;
    }
}

void jsSetStoreMessageCallback(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsFunction()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        CallbackProxy *p = new CallbackProxy(Local<Function>::Cast(args[1]));
        unsigned int result = setStoreMessageCallback(TO_NUMBER(isolate, args[0]),
                                                      p,
                                                      &storeMessageCallback);  
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not set storage message callback, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            }
        }  
    }
}

static unsigned int deleteMessageCallback(void *context, char *ruleset, char *sid, char *mid) {
    CallbackProxy *p = (CallbackProxy *)context;
    Isolate* isolate = Isolate::GetCurrent();
     
    Local<Value> args[3];
    args[0] = CREATE_STRING(isolate, ruleset);
    args[1] = CREATE_STRING(isolate, sid);
    args[2] = CREATE_STRING(isolate, mid);
    
    Local<Function> func = Local<Function>::New(isolate, p->func);
    Local<Value> result = CALL_FUNCTION(func, isolate, 3, args);
    if (result->IsNumber()) {  
        return TO_NUMBER(isolate, result);
    } else {
        return ERR_UNEXPECTED_TYPE;
    }
}

void jsSetDeleteMessageCallback(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsFunction()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        CallbackProxy *p = new CallbackProxy(Local<Function>::Cast(args[1]));
        unsigned int result = setDeleteMessageCallback(TO_NUMBER(isolate, args[0]),
                                                       p,
                                                       &deleteMessageCallback);  
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not set delete message callback, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            }
        }  
    }
}

static unsigned int queueMessageCallback(void *context, char *ruleset, char *sid, unsigned char actionType, char *content) {
    CallbackProxy *p = (CallbackProxy *)context;
    Isolate* isolate = Isolate::GetCurrent();
     
    Local<Value> args[4];
    args[0] = CREATE_STRING(isolate, ruleset);
    args[1] = CREATE_STRING(isolate, sid);
    args[2] = Number::New(isolate, actionType);
    args[3] = CREATE_STRING(isolate, content);

    Local<Function> func = Local<Function>::New(isolate, p->func);
    Local<Value> result = CALL_FUNCTION(func, isolate, 4, args);
    if (result->IsNumber()) {  
        return TO_NUMBER(isolate, result);
    } else {
        return ERR_UNEXPECTED_TYPE;
    }
}

void jsSetQueueMessageCallback(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsFunction()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        CallbackProxy *p = new CallbackProxy(Local<Function>::Cast(args[1]));
        unsigned int result = setQueueMessageCallback(TO_NUMBER(isolate, args[0]),
                                                       p,
                                                       &queueMessageCallback);  
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not set queue message callback, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            }
        }  
    }
}

static unsigned int getQueuedMessagesCallback(void *context, char *ruleset, char *sid) {
    CallbackProxy *p = (CallbackProxy *)context;
    Isolate* isolate = Isolate::GetCurrent();
     
    Local<Value> args[2];
    args[0] = CREATE_STRING(isolate, ruleset);
    args[1] = CREATE_STRING(isolate, sid);
    
    Local<Function> func = Local<Function>::New(isolate, p->func);
    Local<Value> result = CALL_FUNCTION(func, isolate, 2, args);   
    if (result->IsNumber()) {  
        return TO_NUMBER(isolate, result);
    } else {
        return ERR_UNEXPECTED_TYPE;
    }
}

void jsSetGetQueuedMessagesCallback(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsFunction()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        CallbackProxy *p = new CallbackProxy(Local<Function>::Cast(args[1]));
        unsigned int result = setGetQueuedMessagesCallback(TO_NUMBER(isolate, args[0]),
                                                           p,
                                                           &getQueuedMessagesCallback);  
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not set get queued messages callback, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            }
        }  
    }
}

void jsCompleteGetQueuedMessages(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 3) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString() || !args[2]->IsString()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        unsigned int result = completeGetQueuedMessages(TO_NUMBER(isolate, args[0]), 
                                                        TO_STRING(isolate, args[1]),
                                                        TO_STRING(isolate, args[2])); 
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not complete get queued messages, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            }
        }
    }
}

static unsigned int getIdleStateCallback(void *context, char *ruleset) {
    CallbackProxy *p = (CallbackProxy *)context;
    Isolate* isolate = Isolate::GetCurrent();
     
    Local<Value> args[1];
    args[0] = CREATE_STRING(isolate, ruleset);
    
    Local<Function> func = Local<Function>::New(isolate, p->func);
    Local<Value> result = CALL_FUNCTION(func, isolate, 1, args);   
    if (result->IsNumber()) {  
        return TO_NUMBER(isolate, result);
    } else {
        return ERR_UNEXPECTED_TYPE;
    }
}

void jsSetGetIdleStateCallback(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsFunction()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        CallbackProxy *p = new CallbackProxy(Local<Function>::Cast(args[1]));
        unsigned int result = setGetIdleStateCallback(TO_NUMBER(isolate, args[0]),
                                                      p,
                                                      &getIdleStateCallback);  
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not set get idle state callback, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            }
        }  
    }
}

void jsCompleteGetIdleState(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 3) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString() || !args[2]->IsString()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        unsigned int result = completeGetIdleState(TO_NUMBER(isolate, args[0]), 
                                                   TO_STRING(isolate, args[1]),
                                                   TO_STRING(isolate, args[2])); 
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not complete get idle state, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(CREATE_STRING(isolate, message)));
            }
        }
    }
}

void jsCreateProxy(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsFunction() || !args[1]->IsFunction()) {
        isolate->ThrowException(Exception::TypeError(CREATE_STRING(isolate, "Wrong argument type")));
    } else {
        ObjectProxy *p = new ObjectProxy(Local<Function>::Cast(args[0]), Local<Function>::Cast(args[1]));
        args.GetReturnValue().Set(p->Wrap());
    }
}

void ObjectProxy::Get(Local<Name> name, const PropertyCallbackInfo<Value>& info) {
    Isolate* isolate;
    isolate = info.GetIsolate();
    ObjectProxy* p = Unwrap(info.Holder());
    Local<Value> args[1];
    args[0] = name;
    Local<Function> gfunc = Local<Function>::New(isolate, p->_gfunc);
    Local<Value> result = gfunc->Call(isolate->GetCurrentContext(), info.Holder(), 1, args).ToLocalChecked();
    info.GetReturnValue().Set(result);
}

void ObjectProxy::Set(Local<Name> name, Local<Value> value, const PropertyCallbackInfo<Value>& info) {
    Isolate* isolate;
    isolate = info.GetIsolate();
    ObjectProxy* p = Unwrap(info.Holder());
    Local<Value> args[2];
    args[0] = name;
    args[1] = value;
    Local<Function> sfunc = Local<Function>::New(isolate, p->_sfunc);
    Local<Value> result = sfunc->Call(isolate->GetCurrentContext(), info.Holder(), 2, args).ToLocalChecked();
    info.GetReturnValue().Set(result);
}

Local<Object> ObjectProxy::Wrap() { 
    Isolate* isolate = Isolate::GetCurrent();
    Local<ObjectTemplate> proxyTempl = ObjectTemplate::New(isolate);
    proxyTempl->SetInternalFieldCount(1);
    proxyTempl->SetHandler(NamedPropertyHandlerConfiguration(Get, Set));

    Local<Object> result = proxyTempl->NewInstance(isolate->GetCurrentContext()).ToLocalChecked();
    result->SetInternalField(0, External::New(isolate, this));

    return result;
}

ObjectProxy* ObjectProxy::Unwrap(Local<Object> obj) {
    Local<External> field = Local<External>::Cast(obj->GetInternalField(0));
    void* ptr = field->Value();
    return static_cast<ObjectProxy*>(ptr);
}


void init(Local<Object> exports) {
    Isolate* isolate = Isolate::GetCurrent();
    DEFINE_FUNCTION(exports, isolate, "createRuleset", jsCreateRuleset);
    
    DEFINE_FUNCTION(exports, isolate, "deleteRuleset", jsDeleteRuleset);
    
    DEFINE_FUNCTION(exports, isolate, "assertEvent", jsAssertEvent);
    
    DEFINE_FUNCTION(exports, isolate, "assertEvents", jsAssertEvents);
    
    DEFINE_FUNCTION(exports, isolate, "assertFact", jsAssertFact);
    
    DEFINE_FUNCTION(exports, isolate, "assertFacts", jsAssertFacts);
    
    DEFINE_FUNCTION(exports, isolate, "retractFact", jsRetractFact);

    DEFINE_FUNCTION(exports, isolate, "retractFacts", jsRetractFacts);
    
    DEFINE_FUNCTION(exports, isolate, "assertTimers", jsAssertTimers);
    
    DEFINE_FUNCTION(exports, isolate, "updateState", jsUpdateState);
    
    DEFINE_FUNCTION(exports, isolate, "startAction", jsStartAction);
    
    DEFINE_FUNCTION(exports, isolate, "startActionForState", jsStartActionForState);
    
    DEFINE_FUNCTION(exports, isolate, "completeAndStartAction", jsCompleteAndStartAction);
    
    DEFINE_FUNCTION(exports, isolate, "abandonAction", jsAbandonAction);
    
    DEFINE_FUNCTION(exports, isolate, "startTimer", jsStartTimer);
    
    DEFINE_FUNCTION(exports, isolate, "cancelTimer", jsCancelTimer);
    
    DEFINE_FUNCTION(exports, isolate, "getState", jsGetState);
    
    DEFINE_FUNCTION(exports, isolate, "deleteState", jsDeleteState);
    
    DEFINE_FUNCTION(exports, isolate, "renewActionLease", jsRenewActionLease);
    
    DEFINE_FUNCTION(exports, isolate, "createProxy", jsCreateProxy);
    
    DEFINE_FUNCTION(exports, isolate, "setStoreMessageCallback", jsSetStoreMessageCallback);
    
    DEFINE_FUNCTION(exports, isolate, "setDeleteMessageCallback", jsSetDeleteMessageCallback);
    
    DEFINE_FUNCTION(exports, isolate, "setQueueMessageCallback", jsSetQueueMessageCallback);
    
    DEFINE_FUNCTION(exports, isolate, "setGetQueuedMessagesCallback", jsSetGetQueuedMessagesCallback);
    
    DEFINE_FUNCTION(exports, isolate, "completeGetQueuedMessages", jsCompleteGetQueuedMessages);
    
    DEFINE_FUNCTION(exports, isolate, "setGetIdleStateCallback", jsSetGetIdleStateCallback);
    
    DEFINE_FUNCTION(exports, isolate, "completeGetIdleState", jsCompleteGetIdleState);
}

NODE_MODULE(rulesjs, init)
