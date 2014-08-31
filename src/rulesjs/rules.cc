
#include <node.h>
#include <v8.h>
#include <rules.h>

using namespace v8;

Handle<Value> jsCreateRuleset(const Arguments& args) {
    HandleScope scope;

    if (args.Length() < 2) {
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    } else if (!args[0]->IsString() || !args[1]->IsString()) {
        ThrowException(Exception::TypeError(String::New("Wrong argument type")));
    } else {
        void *output = NULL;
        unsigned int result = createRuleset(&output, *v8::String::Utf8Value(args[0]->ToString()), 
                                                     *v8::String::Utf8Value(args[1]->ToString()));
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
            result = bindRuleset((void *)args[0]->IntegerValue(), *v8::String::Utf8Value(args[1]->ToString()), 
                                  args[2]->IntegerValue(), *v8::String::Utf8Value(args[3]->ToString()));
        } else {
            result = bindRuleset((void *)args[0]->IntegerValue(), *v8::String::Utf8Value(args[1]->ToString()), 
                                  args[2]->IntegerValue(), NULL);
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
        unsigned int *results;
        unsigned int resultsLength;
        unsigned int result = assertEvents((void *)args[0]->IntegerValue(), 
                                           *v8::String::Utf8Value(args[1]->ToString()), &resultsLength, &results);
        
        if (result == RULES_OK) {
            Handle<Array> array = Array::New(resultsLength);
            for (unsigned int i = 0; i < resultsLength; ++i) {
                if (results[i] == RULES_OK) {
                    array->Set(i, Number::New(1));
                } else {
                    array->Set(i, Number::New(0));
                }
            }
            free(results);
            return scope.Close(array);
        } else {
            char * message;
            asprintf(&message, "Could not assert event, error code: %d", result);
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
        unsigned int result = startAction((void *)args[0]->IntegerValue(), &session, &messages, &actionHandle); 
        if (result == RULES_OK) {
            Handle<Array> array = Array::New(3);
            array->Set(0, Number::New((long)actionHandle));
            array->Set(1, String::New(session));
            array->Set(2, String::New(messages));
            free(session);
            free(messages);
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
    } else if (!args[0]->IsNumber() || !args[1]->IsString() || !args[2]->IsNumber() || !args[3]->IsString()) {
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
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        ThrowException(Exception::TypeError(String::New("Wrong argument type")));
    } else {
        char *state;
        unsigned int result = getState((void *)args[0]->IntegerValue(),  *v8::String::Utf8Value(args[1]->ToString()), &state); 
        if (result == RULES_OK) {
            Handle<Value> ret = scope.Close(String::New(state));
            free(state);
            return ret;
        } else if (result != ERR_NEW_SESSION) {
            char * message;
            asprintf(&message, "Could not get state, error code: %d", result);
            ThrowException(Exception::TypeError(String::New(message)));
            free(message);
        }
    }

    return scope.Close(Undefined());
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
}

NODE_MODULE(rules, init)
