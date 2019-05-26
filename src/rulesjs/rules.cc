
#include <node.h>
#include <v8.h>
#include <rules.h>
#include <stdlib.h>

using namespace v8;


#define TO_STRING(isolate, value) ((value->IsNull() || value->IsUndefined())? NULL: *v8::String::Utf8Value(isolate, value->ToString()))

class ObjectProxy {
public:

    explicit ObjectProxy(Handle<Function> gvalue, Handle<Function> svalue) { 
        Isolate* isolate = Isolate::GetCurrent();
        _gfunc.Reset(isolate, gvalue); 
        _sfunc.Reset(isolate, svalue); 
    }
    Handle<Object> Wrap();

private:

    static ObjectProxy* Unwrap(Local<Object> obj);
    static void Get(Local<Name> name, const PropertyCallbackInfo<Value>& info);
    static void Set(Local<Name> name, Local<Value> value, const PropertyCallbackInfo<Value>& info);
    
    Persistent<Function> _gfunc;
    Persistent<Function> _sfunc;
};


void jsCreateRuleset(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsString() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int output = 0;
        unsigned int result = createRuleset(&output, 
                                            TO_STRING(isolate, args[0]), 
                                            TO_STRING(isolate, args[1]));
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not create ruleset, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
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
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int result = deleteRuleset(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust()); 
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not delete ruleset, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
            }
        }
    }
}

void jsComplete(const FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsNumber()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int result = complete(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust(),
                                       args[1]->IntegerValue(isolate->GetCurrentContext()).FromJust());
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not complete, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
            }
        } 
    }

}

void jsStartAssertEvent(const FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int replyCount;
        unsigned int stateOffset;
        unsigned int result = startAssertEvent(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust(),
                                               TO_STRING(isolate, args[1]),
                                               &stateOffset,
                                               &replyCount);
        if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED) {
            Handle<Array> array = Array::New(isolate, 2);
            array->Set(0, Number::New(isolate, (long long)stateOffset));
            array->Set(1, Number::New(isolate, (long long)replyCount));
            args.GetReturnValue().Set(array);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not start assert event, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
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
        unsigned int result = assertEvent(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust(),
                                           TO_STRING(isolate, args[1]));
        
        if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED) {
            args.GetReturnValue().Set(result);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not assert event, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
            }
        } 
    }
}

void jsStartAssertEvents(const FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int replyCount;
        unsigned int stateOffset;
        unsigned int *results = NULL;
        unsigned int result = startAssertEvents(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust(), 
                                                TO_STRING(isolate, args[1]),
                                                &stateOffset,
                                                &replyCount);
        if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED) {
            Handle<Array> array = Array::New(isolate, 2);
            array->Set(0, Number::New(isolate, (long long)stateOffset));
            array->Set(1, Number::New(isolate, (long long)replyCount));
            args.GetReturnValue().Set(array);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not start assert events, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                if (results) {
                    free(results);
                }

                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
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
        unsigned int result = assertEvents(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust(), 
                                           TO_STRING(isolate, args[1]));
        
        if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED) {
            args.GetReturnValue().Set(result);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not assert events, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
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
        unsigned int result = retractEvent(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust(),
                                           TO_STRING(isolate, args[1]));
        
        if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED) {
            args.GetReturnValue().Set(result);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not retract event, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
            }
        } 
    }
}

void jsStartAssertFact(const FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int replyCount;
        unsigned int stateOffset;
        unsigned int result = startAssertFact(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust(),
                                               TO_STRING(isolate, args[1]),
                                               &stateOffset,
                                               &replyCount);
        if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED) {
            Handle<Array> array = Array::New(isolate, 2);
            array->Set(0, Number::New(isolate, (long long)stateOffset));
            array->Set(1, Number::New(isolate, (long long)replyCount));
            args.GetReturnValue().Set(array);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not start assert fact, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
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
        unsigned int result = assertFact(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust(),
                                         TO_STRING(isolate, args[1]));
        
        if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED) {
            args.GetReturnValue().Set(result);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not assert fact, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
            }
        } 
    }
}

void jsStartAssertFacts(const FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int replyCount;
        unsigned int stateOffset;
        unsigned int *results = NULL;
        unsigned int result = startAssertFacts(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust(), 
                                               TO_STRING(isolate, args[1]),
                                               &stateOffset,
                                               &replyCount);
        if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED) {
            Handle<Array> array = Array::New(isolate, 2);
            array->Set(0, Number::New(isolate, (long long)stateOffset));
            array->Set(1, Number::New(isolate, (long long)replyCount));
            args.GetReturnValue().Set(array);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not start assert facts, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                if (results) {
                    free(results);
                }

                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
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
        unsigned int result = assertFacts(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust(), 
                                          TO_STRING(isolate, args[1]));
        
        if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED) {
            args.GetReturnValue().Set(result);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not assert facts, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
            }
            if (results) {
                free(results);
            }   
        } 
    }
}

void jsStartRetractFact(const FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int replyCount;
        unsigned int stateOffset;
        unsigned int result = startRetractFact(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust(),
                                               TO_STRING(isolate, args[1]),
                                               &stateOffset,
                                               &replyCount);
        if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED) {
            Handle<Array> array = Array::New(isolate, 2);
            array->Set(0, Number::New(isolate, (long long)stateOffset));
            array->Set(1, Number::New(isolate, (long long)replyCount));
            args.GetReturnValue().Set(array);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not start retract fact, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
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
        unsigned int result = retractFact(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust(),
                                          TO_STRING(isolate, args[1]));
        
        if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED  || result == ERR_EVENT_OBSERVED) {
            args.GetReturnValue().Set(result);
        } else {
            char * message;
            if (asprintf(&message, "Could not retract fact, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
            }
        } 
    }
}

void jsStartRetractFacts(const FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int replyCount;
        unsigned int stateOffset;
        unsigned int *results = NULL;
        unsigned int result = startRetractFacts(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust(), 
                                                TO_STRING(isolate, args[1]), 
                                                &stateOffset,
                                                &replyCount);
        if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED) {
            Handle<Array> array = Array::New(isolate, 2);
            array->Set(0, Number::New(isolate, (long long)stateOffset));
            array->Set(1, Number::New(isolate, (long long)replyCount));
            args.GetReturnValue().Set(array);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not start retract facts, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                if (results) {
                    free(results);
                }

                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
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
        unsigned int result = retractFacts(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust(), 
                                           TO_STRING(isolate, args[1]));
        
        if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED) {
            args.GetReturnValue().Set(result);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not retract facts, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
            }
            if (results) {
                free(results);
            }   
        } 
    }
}

void jsUpdateState(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 3) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[2]->IsString()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int result = updateState(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust(),
                                          TO_STRING(isolate, args[1]),
                                          TO_STRING(isolate, args[2]));
        
        if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED) {
            args.GetReturnValue().Set(result);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not assert state, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
            }
        } 
    }
}

void jsStartUpdateState(const FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsString()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int replyCount;
        unsigned int stateOffset;
        unsigned int result = startUpdateState(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust(),
                                               TO_STRING(isolate, args[1]),
                                               &stateOffset,
                                               &replyCount);
        
        if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED) {
            Handle<Array> array = Array::New(isolate, 2);
            array->Set(0, Number::New(isolate, (long long)stateOffset));
            array->Set(1, Number::New(isolate, (long long)replyCount));
            args.GetReturnValue().Set(array);
        } else {
            char *message = NULL;
            if (asprintf(&message, "Could not start update state, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
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
        unsigned int stateOffset;
        unsigned int result = startAction(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust(), 
                                          &session, 
                                          &messages, 
                                          &stateOffset); 
        if (result == RULES_OK) {
            Handle<Array> array = Array::New(isolate, 4);
            array->Set(0, String::NewFromUtf8(isolate, session));
            array->Set(1, String::NewFromUtf8(isolate, messages));
            array->Set(2, Number::New(isolate, (long long)stateOffset));
            args.GetReturnValue().Set(array);
        } else if (result != ERR_NO_ACTION_AVAILABLE) {
            char *message = NULL;
            if (asprintf(&message, "Could not start action, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
            }
        }
    }
}

void jsCompleteAndStartAction(const FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 3) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[1]->IsNumber() || !args[2]->IsNumber()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        char *messages = NULL;
        unsigned int result = completeAndStartAction(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust(),
                                                     args[1]->IntegerValue(isolate->GetCurrentContext()).FromJust(),
                                                     args[2]->IntegerValue(isolate->GetCurrentContext()).FromJust(),
                                                     &messages);
        if (result == RULES_OK) {
            args.GetReturnValue().Set(String::NewFromUtf8(isolate, messages));
        } else if (result != ERR_NO_ACTION_AVAILABLE) {
            char *message = NULL;
            if (asprintf(&message, "Could not complete and start action, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
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
        unsigned int result = abandonAction(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust(),
                                            args[1]->IntegerValue(isolate->GetCurrentContext()).FromJust());
        
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not abandon action, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
            }
        } 
    }
}

void jsStartTimer(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 5) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[2]->IsNumber() || !args[3]->IsNumber() || !args[4]->IsString()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int result = startTimer(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust(),
                                         TO_STRING(isolate, args[1]),
                                         args[2]->IntegerValue(isolate->GetCurrentContext()).FromJust(),
                                         args[3]->IntegerValue(isolate->GetCurrentContext()).FromJust(),
                                         TO_STRING(isolate, args[4]));
        
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not start timer, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
            }
        } 
    }
}

void jsCancelTimer(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 3) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber() || !args[2]->IsString()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int result = cancelTimer(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust(),
                                         TO_STRING(isolate, args[1]),
                                         TO_STRING(isolate, args[2]));
        
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not cancel timer, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
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
        unsigned int result = assertTimers(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust());
        
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
        unsigned int result = getState(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust(), 
                                       TO_STRING(isolate, args[1]), 
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
            }
        }
    }
}

void jsDeleteState(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int result = deleteState(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust(), 
                                          TO_STRING(isolate, args[1])); 
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not delete state, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
            }
        }
    }
}

void jsRenewActionLease(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate;
    isolate = args.GetIsolate();
    if (args.Length() < 2) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong number of arguments")));
    } else if (!args[0]->IsNumber()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong argument type")));
    } else {
        unsigned int result = renewActionLease(args[0]->IntegerValue(isolate->GetCurrentContext()).FromJust(), 
                                               TO_STRING(isolate, args[1])); 
        if (result != RULES_OK) {
            char *message = NULL;
            if (asprintf(&message, "Could not renew action lease, error code: %d", result) == -1) {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Out of memory")));
            } else {
                isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, message)));
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
        ObjectProxy *p = new ObjectProxy(Handle<Function>::Cast(args[0]), Handle<Function>::Cast(args[1]));
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
    Local<Value> result = gfunc->Call(info.Holder(), 1, args);
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
    Local<Value> result = sfunc->Call(info.Holder(), 2, args);
    info.GetReturnValue().Set(result);
}

Handle<Object> ObjectProxy::Wrap() { 
    Isolate* isolate = Isolate::GetCurrent();
    Local<ObjectTemplate> proxyTempl = ObjectTemplate::New(isolate);
    proxyTempl->SetInternalFieldCount(1);
    proxyTempl->SetHandler(NamedPropertyHandlerConfiguration(Get, Set));

    Local<Object> result = proxyTempl->NewInstance();
    result->SetInternalField(0, External::New(isolate, this));

    return result;
}

ObjectProxy* ObjectProxy::Unwrap(Local<Object> obj) {
    Local<External> field = Local<External>::Cast(obj->GetInternalField(0));
    void* ptr = field->Value();
    return static_cast<ObjectProxy*>(ptr);
}


void init(Handle<Object> exports) {
    Isolate* isolate = Isolate::GetCurrent();
    exports->Set(String::NewFromUtf8(isolate, "createRuleset", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsCreateRuleset)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "deleteRuleset", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsDeleteRuleset)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "complete", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsComplete)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "startAssertEvent", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsStartAssertEvent)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "assertEvent", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsAssertEvent)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "startAssertEvents", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsStartAssertEvents)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "assertEvents", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsAssertEvents)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "retractEvent", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsRetractEvent)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "startAssertFact", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsStartAssertFact)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "assertFact", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsAssertFact)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "startAssertFacts", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsStartAssertFacts)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "assertFacts", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsAssertFacts)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "startRetractFact", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsStartRetractFact)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "retractFact", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsRetractFact)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "startRetractFacts", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsStartRetractFacts)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "retractFacts", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsRetractFacts)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "assertTimers", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsAssertTimers)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "updateState", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsUpdateState)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "startUpdateState", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsStartUpdateState)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "startAction", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsStartAction)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "completeAndStartAction", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsCompleteAndStartAction)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "abandonAction", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsAbandonAction)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "startTimer", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsStartTimer)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "cancelTimer", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsCancelTimer)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "getState", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsGetState)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "deleteState", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsDeleteState)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "renewActionLease", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsRenewActionLease)->GetFunction());

    exports->Set(String::NewFromUtf8(isolate, "createProxy", String::kInternalizedString),
        FunctionTemplate::New(isolate, jsCreateProxy)->GetFunction());
}

NODE_MODULE(rulesjs, init)
