
#include <node.h>
#include <v8.h>
#include <rules.h>

using namespace v8;

class Proxy {
public:

    explicit Proxy(Handle<v8::Function> gvalue, Handle<v8::Function> svalue) { 
        _gfunc = v8::Persistent<v8::Function>::New(gvalue); 
        _sfunc = v8::Persistent<v8::Function>::New(svalue); 
    }
    Handle<Object> Wrap();

private:

    static Proxy* Unwrap(Handle<Object> obj);
    static Handle<Value> Get(Local<String> name, const AccessorInfo& info);
    static Handle<Value> Set(Local<String> name, Local<Value> value, const AccessorInfo& info);
    
    v8::Persistent<v8::Function> _gfunc;
    v8::Persistent<v8::Function> _sfunc;
};


Handle<Value> jsCreateRuleset(const Arguments& args) {
    HandleScope scope;

    if (args.Length() < 3) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    } else if (!args[0]->IsString() || !args[1]->IsString() || !args[2]->IsNumber()) {
        ThrowException(Exception::TypeError(String::New("Wrong argument type")));
    } else {
        void *output = NULL;
        unsigned int result = createRuleset(&output, 
                                            *v8::String::Utf8Value(args[0]->ToString()), 
                                            *v8::String::Utf8Value(args[1]->ToString()),
                                            args[2]->IntegerValue());
        if (result != RULES_OK) {
            char * message;
            asprintf(&message, "Could not create ruleset, error code: %d", result);
            ThrowException(Exception::TypeError(String::New(message)));
            free(message);
        } else {
            return scope.Close(Number::New((long)output));
        }
    }

    return scope.Close(Undefined());
}

Handle<Value> jsDeleteRuleset(const Arguments& args) {
    HandleScope scope;

    if (args.Length() < 1) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    } else if (!args[0]->IsNumber()) {
        ThrowException(Exception::TypeError(String::New("Wrong argument type")));
    } else {
        unsigned int result = deleteRuleset((void *)args[0]->IntegerValue()); 
        if (result != RULES_OK) {
            char * message;
            asprintf(&message, "Could not delete ruleset, error code: %d", result);
            ThrowException(Exception::TypeError(String::New(message)));
            free(message);
        }
    }

    return scope.Close(Undefined());
}

Handle<Value> jsBindRuleset(const Arguments& args) {
    HandleScope scope;

    if (args.Length() < 4) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString() || !args[2]->IsNumber()) {
        ThrowException(Exception::TypeError(String::New("Wrong argument type")));
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
            char * message;
            asprintf(&message, "Could not create connection, error code: %d", result);
            ThrowException(Exception::TypeError(String::New(message)));
            free(message);
        } 
    }

    return scope.Close(Undefined());
}

Handle<Value> jsAssertEvent(const Arguments& args) {
    HandleScope scope;

    if (args.Length() < 2) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        ThrowException(Exception::TypeError(String::New("Wrong argument type")));
    } else {
        unsigned int result = assertEvent((void *)args[0]->IntegerValue(),
                                           *v8::String::Utf8Value(args[1]->ToString()));
        
        if (result == RULES_OK) {
            return scope.Close(Number::New(1));
        }
        else if (result == ERR_EVENT_NOT_HANDLED) {
            return scope.Close(Number::New(0));
        } else {
            char * message;
            asprintf(&message, "Could not assert event, error code: %d", result);
            ThrowException(Exception::TypeError(String::New(message)));
            free(message);
        } 
    }

    return scope.Close(Undefined());
}

Handle<Value> jsAssertEvents(const Arguments& args) {
    HandleScope scope;

    if (args.Length() < 2) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        ThrowException(Exception::TypeError(String::New("Wrong argument type")));
    } else {
        unsigned int *results = NULL;
        unsigned int resultsLength;
        unsigned int result = assertEvents((void *)args[0]->IntegerValue(), 
                                           *v8::String::Utf8Value(args[1]->ToString()), 
                                           &resultsLength, 
                                           &results);
        
        if (result == RULES_OK) {
            Handle<Array> array = Array::New(resultsLength);
            for (unsigned int i = 0; i < resultsLength; ++i) {
                array->Set(i, Number::New(results[i]));
            }
            if (results) {
                free(results);
            }
            return scope.Close(array);
        } else {
            char * message;
            asprintf(&message, "Could not assert events, error code: %d", result);
            ThrowException(Exception::TypeError(String::New(message)));
            if (results) {
                free(results);
            }   
            free(message);
        } 
    }

    return scope.Close(Undefined());
}

Handle<Value> jsRetractEvent(const Arguments& args) {
    HandleScope scope;

    if (args.Length() < 2) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    } else if (!args[0]->IsNumber()) {
        ThrowException(Exception::TypeError(String::New("Wrong argument type")));
    } else {
        unsigned int result = retractEvent((void *)args[0]->IntegerValue(),
                                           *v8::String::Utf8Value(args[1]->ToString()));
        
        if (result == RULES_OK) {
            return scope.Close(Number::New(1));
        }
        else if (result == ERR_EVENT_NOT_HANDLED) {
            return scope.Close(Number::New(0));
        } else {
            char * message;
            asprintf(&message, "Could not retract event, error code: %d", result);
            ThrowException(Exception::TypeError(String::New(message)));
            free(message);
        } 
    }

    return scope.Close(Undefined());
}

Handle<Value> jsAssertFact(const Arguments& args) {
    HandleScope scope;

    if (args.Length() < 2) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        ThrowException(Exception::TypeError(String::New("Wrong argument type")));
    } else {
        unsigned int result = assertFact((void *)args[0]->IntegerValue(),
                                           *v8::String::Utf8Value(args[1]->ToString()));
        
        if (result == RULES_OK) {
            return scope.Close(Number::New(1));
        }
        else if (result == ERR_EVENT_NOT_HANDLED) {
            return scope.Close(Number::New(0));
        } else {
            char * message;
            asprintf(&message, "Could not assert fact, error code: %d", result);
            ThrowException(Exception::TypeError(String::New(message)));
            free(message);
        } 
    }

    return scope.Close(Undefined());
}

Handle<Value> jsAssertFacts(const Arguments& args) {
    HandleScope scope;

    if (args.Length() < 2) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        ThrowException(Exception::TypeError(String::New("Wrong argument type")));
    } else {
        unsigned int *results = NULL;
        unsigned int resultsLength;
        unsigned int result = assertFacts((void *)args[0]->IntegerValue(), 
                                           *v8::String::Utf8Value(args[1]->ToString()), 
                                           &resultsLength, 
                                           &results);
        
        if (result == RULES_OK) {
            Handle<Array> array = Array::New(resultsLength);
            for (unsigned int i = 0; i < resultsLength; ++i) {
                array->Set(i, Number::New(results[i]));
            }
            if (results) {
                free(results);
            }
            return scope.Close(array);
        } else {
            char * message;
            asprintf(&message, "Could not assert facts, error code: %d", result);
            ThrowException(Exception::TypeError(String::New(message)));
            if (results) {
                free(results);
            }   
            free(message);
        } 
    }

    return scope.Close(Undefined());
}

Handle<Value> jsRetractFact(const Arguments& args) {
    HandleScope scope;

    if (args.Length() < 2) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    } else if (!args[0]->IsNumber()) {
        ThrowException(Exception::TypeError(String::New("Wrong argument type")));
    } else {
        unsigned int result = retractFact((void *)args[0]->IntegerValue(),
                                           *v8::String::Utf8Value(args[1]->ToString()));
        
        if (result == RULES_OK) {
            return scope.Close(Number::New(1));
        }
        else if (result == ERR_EVENT_NOT_HANDLED) {
            return scope.Close(Number::New(0));
        } else {
            char * message;
            asprintf(&message, "Could not retract fact, error code: %d", result);
            ThrowException(Exception::TypeError(String::New(message)));
            free(message);
        } 
    }

    return scope.Close(Undefined());
}

Handle<Value> jsAssertState(const Arguments& args) {
    HandleScope scope;

    if (args.Length() < 2) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        ThrowException(Exception::TypeError(String::New("Wrong argument type")));
    } else {
        unsigned int result = assertState((void *)args[0]->IntegerValue(),
                                           *v8::String::Utf8Value(args[1]->ToString()));
        
        if (result == RULES_OK) {
            return scope.Close(Number::New(1));
        }
        else if (result == ERR_EVENT_NOT_HANDLED) {
            return scope.Close(Number::New(0));
        } else {
            char * message;
            asprintf(&message, "Could not assert state, error code: %d", result);
            ThrowException(Exception::TypeError(String::New(message)));
            free(message);
        } 
    }

    return scope.Close(Undefined());
}

Handle<Value> jsStartAction(const Arguments& args) {
    HandleScope scope;

    if (args.Length() < 1) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    } else if (!args[0]->IsNumber()) {
        ThrowException(Exception::TypeError(String::New("Wrong argument type")));
    } else {
        char *session;
        char *messages;
        void *actionHandle;
        unsigned int result = startAction((void *)args[0]->IntegerValue(), 
                                          &session, 
                                          &messages, 
                                          &actionHandle); 
        if (result == RULES_OK) {
            Handle<Array> array = Array::New(3);
            array->Set(0, Number::New((long)actionHandle));
            array->Set(1, String::New(session));
            array->Set(2, String::New(messages));
            return scope.Close(array);
        } else if (result != ERR_NO_ACTION_AVAILABLE) {
            char * message;
            asprintf(&message, "Could not start action, error code: %d", result);
            ThrowException(Exception::TypeError(String::New(message)));
            free(message);
        }
    }

    return scope.Close(Undefined());
}

Handle<Value> jsCompleteAction(const Arguments& args) {
    HandleScope scope;

    if (args.Length() < 3) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsNumber() || !args[2]->IsString()) {
        ThrowException(Exception::TypeError(String::New("Wrong argument type")));
    } else {
        unsigned int result = completeAction((void *)args[0]->IntegerValue(),
                                            (void *)args[1]->IntegerValue(),
                                            *v8::String::Utf8Value(args[2]->ToString()));
        
        if (result != RULES_OK) {
            char * message;
            asprintf(&message, "Could not complete action, error code: %d", result);
            ThrowException(Exception::TypeError(String::New(message)));
            free(message);
        } 
    }

    return scope.Close(Undefined());
}

Handle<Value> jsAbandonAction(const Arguments& args) {
    HandleScope scope;

    if (args.Length() < 2) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsNumber()) {
        ThrowException(Exception::TypeError(String::New("Wrong argument type")));
    } else {
        unsigned int result = abandonAction((void *)args[0]->IntegerValue(),
                                            (void *)args[1]->IntegerValue());
        
        if (result != RULES_OK) {
            char * message;
            asprintf(&message, "Could not abandon action, error code: %d", result);
            ThrowException(Exception::TypeError(String::New(message)));
            free(message);
        } 
    }

    return scope.Close(Undefined());
}

Handle<Value> jsStartTimer(const Arguments& args) {
    HandleScope scope;

    if (args.Length() < 3) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[2]->IsNumber() || !args[3]->IsString()) {
        ThrowException(Exception::TypeError(String::New("Wrong argument type")));
    } else {
        unsigned int result = startTimer((void *)args[0]->IntegerValue(),
                                         *v8::String::Utf8Value(args[1]->ToString()),
                                         args[2]->IntegerValue(),
                                         *v8::String::Utf8Value(args[3]->ToString()));
        
        if (result != RULES_OK) {
            char * message;
            asprintf(&message, "Could not start timer, error code: %d", result);
            ThrowException(Exception::TypeError(String::New(message)));
            free(message);
        } 
    }

    return scope.Close(Undefined());
}

Handle<Value> jsAssertTimers(const Arguments& args) {
    HandleScope scope;

    if (args.Length() < 1) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    } else if (!args[0]->IsNumber()) {
        ThrowException(Exception::TypeError(String::New("Wrong argument type")));
    } else {
        unsigned int result = assertTimers((void *)args[0]->IntegerValue());
        
        if (result == RULES_OK) {
            return scope.Close(Number::New(1));
        } else if (result == ERR_NO_TIMERS_AVAILABLE) {
            return scope.Close(Number::New(0));
        } else {
            char * message;
            asprintf(&message, "Could not assert timers, error code: %d", result);
            ThrowException(Exception::TypeError(String::New(message)));
            free(message);
        } 
    }

    return scope.Close(Undefined());
}

Handle<Value> jsGetState(const Arguments& args) {
    HandleScope scope;

    if (args.Length() < 2) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    } else if (!args[0]->IsNumber()) {
        ThrowException(Exception::TypeError(String::New("Wrong argument type")));
    } else {
        char *state;
        unsigned int result = getState((void *)args[0]->IntegerValue(), 
                                       *v8::String::Utf8Value(args[1]->ToString()), 
                                       &state); 
        if (result == RULES_OK) {
            Handle<Value> ret = scope.Close(String::New(state));
            free(state);
            return ret;
        } else if (result != ERR_NEW_SESSION) {
            char * message;
            asprintf(&message, "Could not get ruleset state, error code: %d", result);
            ThrowException(Exception::TypeError(String::New(message)));
            free(message);
        }
    }

    return scope.Close(Undefined());
}

Handle<Value> jsCreateProxy(const Arguments& args) {
    HandleScope scope;

    if (args.Length() < 2) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    } else if (!args[0]->IsFunction() || !args[1]->IsFunction()) {
        ThrowException(Exception::TypeError(String::New("Wrong argument type")));
    } else {
        Proxy *p = new Proxy(Handle<v8::Function>::Cast(args[0]), Handle<v8::Function>::Cast(args[1]));
        return scope.Close(p->Wrap());
    }

    return scope.Close(Undefined());
}

Handle<Value> Proxy::Get(Local<String> name, const AccessorInfo& info) {
    HandleScope scope;
    Proxy* p = Unwrap(info.Holder());

    Local<Value> args[1];
    args[0] = name;

    Handle<Value> result = p->_gfunc->Call(Context::GetCurrent()->Global(), 1, args);

    return scope.Close(result);
}

Handle<Value> Proxy::Set(Local<String> name, Local<Value> value, const AccessorInfo& info) {
    HandleScope scope;
    Proxy* p = Unwrap(info.Holder());

    Local<Value> args[2];
    args[0] = name;
    args[1] = value;

    Handle<Value> result = p->_sfunc->Call(Context::GetCurrent()->Global(), 2, args);

    return scope.Close(result);
}

Handle<Object> Proxy::Wrap() { 
    Local<ObjectTemplate> proxyTempl = ObjectTemplate::New();
    proxyTempl->SetInternalFieldCount(1);
    proxyTempl->SetNamedPropertyHandler(Get, Set);

    Local<Object> result = proxyTempl->NewInstance();
    result->SetInternalField(0, External::New(this));

    return result;
}

Proxy* Proxy::Unwrap(Handle<Object> obj) {
  Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
  void* ptr = field->Value();
  return static_cast<Proxy*>(ptr);
}


void init(Handle<Object> exports) {
    exports->Set(String::NewSymbol("createRuleset"),
        FunctionTemplate::New(jsCreateRuleset)->GetFunction());

    exports->Set(String::NewSymbol("deleteRuleset"),
        FunctionTemplate::New(jsDeleteRuleset)->GetFunction());

    exports->Set(String::NewSymbol("bindRuleset"),
        FunctionTemplate::New(jsBindRuleset)->GetFunction());

    exports->Set(String::NewSymbol("assertEvent"),
        FunctionTemplate::New(jsAssertEvent)->GetFunction());

    exports->Set(String::NewSymbol("assertEvents"),
        FunctionTemplate::New(jsAssertEvents)->GetFunction());

    exports->Set(String::NewSymbol("retractEvent"),
        FunctionTemplate::New(jsRetractEvent)->GetFunction());

    exports->Set(String::NewSymbol("assertFact"),
        FunctionTemplate::New(jsAssertFact)->GetFunction());

    exports->Set(String::NewSymbol("assertFacts"),
        FunctionTemplate::New(jsAssertFacts)->GetFunction());

    exports->Set(String::NewSymbol("retractFact"),
        FunctionTemplate::New(jsRetractFact)->GetFunction());

    exports->Set(String::NewSymbol("assertTimers"),
        FunctionTemplate::New(jsAssertTimers)->GetFunction());

    exports->Set(String::NewSymbol("assertState"),
        FunctionTemplate::New(jsAssertState)->GetFunction());

    exports->Set(String::NewSymbol("startAction"),
        FunctionTemplate::New(jsStartAction)->GetFunction());

    exports->Set(String::NewSymbol("completeAction"),
        FunctionTemplate::New(jsCompleteAction)->GetFunction());

    exports->Set(String::NewSymbol("abandonAction"),
        FunctionTemplate::New(jsAbandonAction)->GetFunction());

    exports->Set(String::NewSymbol("startTimer"),
        FunctionTemplate::New(jsStartTimer)->GetFunction());

    exports->Set(String::NewSymbol("getState"),
        FunctionTemplate::New(jsGetState)->GetFunction());

    exports->Set(String::NewSymbol("createProxy"),
        FunctionTemplate::New(jsCreateProxy)->GetFunction());
}

NODE_MODULE(rules, init)
