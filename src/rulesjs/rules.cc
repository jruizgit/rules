
#include <node.h>
#include <v8.h>
#include <rules.h>
#include <stdlib.h>

using namespace v8;

class Proxy {
public:

    explicit Proxy(Handle<Function> gvalue, Handle<Function> svalue) { 
        Isolate* isolate = Isolate::GetCurrent();
        _gfunc.Reset(isolate, gvalue); 
        _sfunc.Reset(isolate, svalue); 
    }
    Handle<Object> Wrap();

private:

    static Proxy* Unwrap(Local<Object> obj);
    static void Get(Local<String> name, const PropertyCallbackInfo<Value>& info);
    static void Set(Local<String> name, Local<Value> value, const PropertyCallbackInfo<Value>& info);
    
    Persistent<Function> _gfunc;
    Persistent<Function> _sfunc;
};


void jsCreateRuleset(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 3) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsString() || !args[1]->IsString() || !args[2]->IsNumber()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        void *output = NULL;
        unsigned int result = createRuleset(&output, 
                                            *String::Utf8Value(args[0]->ToString()), 
                                            *String::Utf8Value(args[1]->ToString()),
                                            args[2]->IntegerValue());
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not create ruleset, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
                free(message);
            }
        } else {
            args.GetReturnValue().Set(static_cast<double>((long)output));
        }
    }
}

void jsDeleteRuleset(const FunctionCallbackInfo<Value>& args) {
    v8::Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 1) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int result = deleteRuleset((void *)args[0]->IntegerValue()); 
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not delete ruleset, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
                free(message);
            }
        }
    }
}

void jsBindRuleset(const FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 4) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString() || !args[2]->IsNumber()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int result;
        if (args[3]->IsString()) {
            result = bindRuleset((void *)args[0]->IntegerValue(), 
                                 *v8::String::Utf8Value(args[1]->ToString()), 
                                 args[2]->IntegerValue(), 
                                 *v8::String::Utf8Value(args[3]->ToString()));
        } else {
            result = bindRuleset((void *)args[0]->IntegerValue(), 
                                 *v8::String::Utf8Value(args[1]->ToString()), 
                                args[2]->IntegerValue(), 
                                NULL);
        }

        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not create connection, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
                free(message);
            }
        } 
    }
}

void jsAssertEvent(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int result = assertEvent((void *)args[0]->IntegerValue(),
                                           *String::Utf8Value(args[1]->ToString()));
        
        if (result == RULES_OK) {
            args.GetReturnValue().Set(1);
        }
        else if (result == ERR_EVENT_NOT_HANDLED) {
            args.GetReturnValue().Set(0);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not assert event, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
                free(message);
            }
        } 
    }
}

void jsAssertEvents(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int *results = NULL;
        unsigned int resultsLength;
        unsigned int result = assertEvents((void *)args[0]->IntegerValue(), 
                                           *String::Utf8Value(args[1]->ToString()), 
                                           &resultsLength, 
                                           &results);
        
        if (result == RULES_OK) {
            Handle<Array> array = Array::New(isolate, resultsLength);
            for (unsigned int i = 0; i < resultsLength; ++i) {
                array->Set(i, Number::New(isolate, results[i]));
            }
            if (results) {
                free(results);
            }
            args.GetReturnValue().Set(array);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not assert events, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
                free(message);
            }

            if (results) {
                free(results);
            }   
        } 
    }
}

void jsRetractEvent(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int result = retractEvent((void *)args[0]->IntegerValue(),
                                           *String::Utf8Value(args[1]->ToString()));
        
        if (result == RULES_OK) {
            args.GetReturnValue().Set(1);
        }
        else if (result == ERR_EVENT_NOT_HANDLED) {
            args.GetReturnValue().Set(0);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not retract event, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
                free(message);
            }
        } 
    }
}

void jsAssertFact(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int result = assertFact((void *)args[0]->IntegerValue(),
                                          *String::Utf8Value(args[1]->ToString()));
        
        if (result == RULES_OK) {
            args.GetReturnValue().Set(1);
        }
        else if (result == ERR_EVENT_NOT_HANDLED) {
            args.GetReturnValue().Set(0);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not assert fact, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
                free(message);
            }
        } 
    }
}

void jsAssertFacts(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int *results = NULL;
        unsigned int resultsLength;
        unsigned int result = assertFacts((void *)args[0]->IntegerValue(), 
                                           *String::Utf8Value(args[1]->ToString()), 
                                           &resultsLength, 
                                           &results);
        
        if (result == RULES_OK) {
            Handle<Array> array = Array::New(isolate, resultsLength);
            for (unsigned int i = 0; i < resultsLength; ++i) {
                array->Set(i, Number::New(isolate, results[i]));
            }
            if (results) {
                free(results);
            }
            args.GetReturnValue().Set(array);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not assert facts, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
                free(message);
            }
            if (results) {
                free(results);
            }   
        } 
    }
}

void jsRetractFact(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int result = retractFact((void *)args[0]->IntegerValue(),
                                           *String::Utf8Value(args[1]->ToString()));
        
        if (result == RULES_OK) {
            args.GetReturnValue().Set(1);
        }
        else if (result == ERR_EVENT_NOT_HANDLED) {
            args.GetReturnValue().Set(0);
        } else {
            char * message;
            if (asprintf(&message, "Could not retract fact, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
                free(message);
            }
        } 
    }
}

void jsRetractFacts(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int *results = NULL;
        unsigned int resultsLength;
        unsigned int result = retractFacts((void *)args[0]->IntegerValue(), 
                                           *String::Utf8Value(args[1]->ToString()), 
                                           &resultsLength, 
                                           &results);
        
        if (result == RULES_OK) {
            Handle<Array> array = Array::New(isolate, resultsLength);
            for (unsigned int i = 0; i < resultsLength; ++i) {
                array->Set(i, Number::New(isolate, results[i]));
            }
            if (results) {
                free(results);
            }
            args.GetReturnValue().Set(array);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not retract facts, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
                free(message);
            }
            if (results) {
                free(results);
            }   
        } 
    }
}

void jsAssertState(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int result = assertState((void *)args[0]->IntegerValue(),
                                           *String::Utf8Value(args[1]->ToString()));
        
        if (result == RULES_OK) {
            args.GetReturnValue().Set(1);
        }
        else if (result == ERR_EVENT_NOT_HANDLED) {
            args.GetReturnValue().Set(0);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not assert state, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
                free(message);
            }
        } 
    }
}

void jsStartAction(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 1) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        char *session;
        char *messages;
        void *actionHandle;
        void *actionBinding;
        unsigned int result = startAction((void *)args[0]->IntegerValue(), 
                                          &session, 
                                          &messages, 
                                          &actionHandle,
                                          &actionBinding); 
        if (result == RULES_OK) {
            Handle<Array> array = Array::New(isolate, 4);
            array->Set(0, Number::New(isolate, (long)actionHandle));
            array->Set(1, String::NewFromUtf8(isolate, session));
            array->Set(2, String::NewFromUtf8(isolate, messages));
            array->Set(3, String::NewFromUtf8(isolate, actionBinding));
            args.GetReturnValue().Set(array);
        } else if (result != ERR_NO_ACTION_AVAILABLE) {
            char *message = NULL;
            if (asprintf(&message, "Could not start action, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
                free(message);
            }
        }
    }
}

void jsCompleteAction(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 3) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsNumber() || !args[2]->IsString()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int result = completeAction((void *)args[0]->IntegerValue(),
                                            (void *)args[1]->IntegerValue(),
                                            *String::Utf8Value(args[2]->ToString()));
        
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not complete action, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
                free(message);
            }
        } 
    }
}

void jsAbandonAction(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsNumber()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int result = abandonAction((void *)args[0]->IntegerValue(),
                                            (void *)args[1]->IntegerValue());
        
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not abandon action, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
                free(message);
            }
        } 
    }
}

void jsStartTimer(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 3) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[2]->IsNumber() || !args[3]->IsString()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int result = startTimer((void *)args[0]->IntegerValue(),
                                         *String::Utf8Value(args[1]->ToString()),
                                         args[2]->IntegerValue(),
                                         *String::Utf8Value(args[3]->ToString()));
        
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not start timer, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
                free(message);
            }
        } 
    }
}

void jsAssertTimers(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 1) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int result = assertTimers((void *)args[0]->IntegerValue());
        
        if (result == RULES_OK) {
            args.GetReturnValue().Set(1);
        } else if (result == ERR_NO_TIMERS_AVAILABLE) {
            args.GetReturnValue().Set(0);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not assert timers, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
                free(message);
            }
        } 
    }
}

void jsGetState(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        char *state;
        unsigned int result = getState((void *)args[0]->IntegerValue(), 
                                       *v8::String::Utf8Value(args[1]->ToString()), 
                                       &state); 
        if (result == RULES_OK) {
            args.GetReturnValue().Set(String::NewFromUtf8(isolate, state));
            free(state);
        } else if (result != ERR_NEW_SESSION) {
            char *message = NULL;
            if (asprintf(&message, "Could not get state, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
                free(message);
            }
        }
    }
}

void jsCreateProxy(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsFunction() || !args[1]->IsFunction()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        Proxy *p = new Proxy(Handle<Function>::Cast(args[0]), Handle<Function>::Cast(args[1]));
        args.GetReturnValue().Set(p->Wrap());
    }
}

void Proxy::Get(Local<String> name, const PropertyCallbackInfo<Value>& info) {
    Isolate* isolate;
    isolate = info.GetIsolate();
    Proxy* p = Unwrap(info.Holder());
    Local<Value> args[1];
    args[0] = name;
    Local<Function> gfunc = Local<Function>::New(isolate, p->_gfunc);
    Local<Value> result = gfunc->Call(info.Holder(), 1, args);
    info.GetReturnValue().Set(result);
}

void Proxy::Set(Local<String> name, Local<Value> value, const PropertyCallbackInfo<Value>& info) {
    Isolate* isolate;
    isolate = info.GetIsolate();
    Proxy* p = Unwrap(info.Holder());
    Local<Value> args[2];
    args[0] = name;
    args[1] = value;
    Local<Function> sfunc = Local<Function>::New(isolate, p->_sfunc);
    Local<Value> result = sfunc->Call(info.Holder(), 2, args);
    info.GetReturnValue().Set(result);
}

Handle<Object> Proxy::Wrap() { 
    Isolate* isolate = Isolate::GetCurrent();
    Local<ObjectTemplate> proxyTempl = ObjectTemplate::New(isolate);
    proxyTempl->SetInternalFieldCount(1);
    proxyTempl->SetNamedPropertyHandler(Get, Set);

    Local<Object> result = proxyTempl->NewInstance();
    result->SetInternalField(0, External::New(isolate, this));

    return result;
}

Proxy* Proxy::Unwrap(Local<Object> obj) {
    Local<External> field = Local<External>::Cast(obj->GetInternalField(0));
    void* ptr = field->Value();
    return static_cast<Proxy*>(ptr);
}


void init(Handle<Object> exports) {
    Isolate* isolate = Isolate::GetCurrent();
    exports->Set(String::NewFromUtf8(isolate, "createRuleset", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsCreateRuleset)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "deleteRuleset", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsDeleteRuleset)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "bindRuleset", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsBindRuleset)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "assertEvent", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsAssertEvent)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "assertEvents", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsAssertEvents)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "retractEvent", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsRetractEvent)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "assertFact", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsAssertFact)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "assertFacts", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsAssertFacts)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "retractFact", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsRetractFact)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "retractFacts", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsRetractFacts)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "assertTimers", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsAssertTimers)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "assertState", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsAssertState)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "startAction", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsStartAction)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "completeAction", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsCompleteAction)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "abandonAction", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsAbandonAction)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "startTimer", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsStartTimer)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "getState", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsGetState)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "createProxy", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsCreateProxy)->GetFunction());
}

NODE_MODULE(rulesjs, init)
