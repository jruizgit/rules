#include <ruby.h>
#include <rules.h>

VALUE rulesModule = Qnil;

static VALUE rbCreateRuleset(VALUE self, VALUE name, VALUE rules) {
    Check_Type(name, T_STRING);
    Check_Type(rules, T_STRING);

    void *output = NULL;
    unsigned int result = createRuleset(&output, RSTRING_PTR(name), RSTRING_PTR(rules));
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not create ruleset, error code: %d", result);
        }
    }

    return INT2FIX(output);
}

static VALUE rbDeleteRuleset(VALUE self, VALUE handle) {
    Check_Type(handle, T_FIXNUM);

    unsigned int result = deleteRuleset((void *)FIX2LONG(handle));
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not delete ruleset, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbBindRuleset(VALUE self, VALUE handle, VALUE host, VALUE port, VALUE password) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(host, T_STRING);
    Check_Type(port, T_FIXNUM);
    
    unsigned int result;
    if (TYPE(password) == T_STRING) {
        result = bindRuleset((void *)FIX2LONG(handle), RSTRING_PTR(host), FIX2INT(port), RSTRING_PTR(password));
    } else if (TYPE(password) == T_NIL) {
        result = bindRuleset((void *)FIX2LONG(handle), RSTRING_PTR(host), FIX2INT(port), NULL);
    } else {
        rb_raise(rb_eTypeError, "Wrong argument type for password");   
    }

    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not create connection, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbAssertEvent(VALUE self, VALUE handle, VALUE event) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(event, T_STRING);

    unsigned int result = assertEvent((void *)FIX2LONG(handle), RSTRING_PTR(event));
    if (result == RULES_OK) {
        return INT2FIX(1);    
    } else if (result == ERR_EVENT_NOT_HANDLED) {
        return INT2FIX(0);    
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not assert event, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbAssertEvents(VALUE self, VALUE handle, VALUE events) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(events, T_STRING);

    unsigned int *results;
    unsigned int resultsLength;
    unsigned int result = assertEvents((void *)FIX2LONG(handle), RSTRING_PTR(events), &resultsLength, &results);
    if (result == RULES_OK) {
        free(results);
        return INT2FIX(resultsLength);   
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not assert events, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbAssertState(VALUE self, VALUE handle, VALUE state) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(state, T_STRING);

    unsigned int result = assertState((void *)FIX2LONG(handle), RSTRING_PTR(state));
    if (result == RULES_OK) {
        return INT2FIX(1);    
    } else if (result == ERR_EVENT_NOT_HANDLED) {
        return INT2FIX(0);    
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not assert event, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbStartAction(VALUE self, VALUE handle) {
    Check_Type(handle, T_FIXNUM);

    char *state;
    char *messages;
    void *actionHandle;
    unsigned int result = startAction((void *)FIX2LONG(handle), &state, &messages, &actionHandle);
    if (result == ERR_NO_ACTION_AVAILABLE) {
        return Qnil;
    } else if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not start action, error code: %d", result);
        }
    }

    VALUE output = rb_ary_new(); 
    rb_ary_push(output, rb_str_new2(state));
    rb_ary_push(output, rb_str_new2(messages));
    rb_ary_push(output, INT2FIX(actionHandle));
    free(state);
    free(messages);
    return output;
}

static VALUE rbCompleteAction(VALUE self, VALUE handle, VALUE actionHandle, VALUE state) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(actionHandle, T_FIXNUM);
    Check_Type(state, T_STRING);

    unsigned int result = completeAction((void *)FIX2LONG(handle), (void *)FIX2LONG(actionHandle), RSTRING_PTR(state));
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not complete action, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbAbandonAction(VALUE self, VALUE handle, VALUE actionHandle) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(actionHandle, T_FIXNUM);

    unsigned int result = abandonAction((void *)FIX2LONG(handle), (void *)FIX2LONG(actionHandle));
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not abandon action, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbStartTimer(VALUE self, VALUE handle, VALUE sid, VALUE duration, VALUE timer) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(sid, T_STRING);
    Check_Type(duration, T_FIXNUM);
    Check_Type(timer, T_STRING);    

    unsigned int result = startTimer((void *)FIX2LONG(handle), RSTRING_PTR(sid), FIX2UINT(duration), RSTRING_PTR(timer));
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not start timer, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbAssertTimers(VALUE self, VALUE handle) {
    Check_Type(handle, T_FIXNUM);

    unsigned int result = assertTimers((void *)FIX2LONG(handle));
    if (result == RULES_OK) {
        return INT2FIX(1);    
    } else if (result == ERR_NO_TIMERS_AVAILABLE) {
        return INT2FIX(0);    
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not assert timers, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbGetState(VALUE self, VALUE handle, VALUE sid) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(sid, T_STRING);

    char *state;
    unsigned int result = getState((void *)FIX2LONG(handle), RSTRING_PTR(sid), &state);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not get state, error code: %d", result);
        }
    }

    VALUE output = rb_str_new2(state);
    free(state);
    return output;
}

static VALUE rbGetRulesetState(VALUE self, VALUE handle) {
    Check_Type(handle, T_FIXNUM);

    char *state;
    unsigned int result = getRulesetState((void *)FIX2LONG(handle), &state);
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not get ruleset state, error code: %d", result);
        }
    }

    VALUE output = rb_str_new2(state);
    free(state);
    return output;
}

static VALUE rbSetRulesetState(VALUE self, VALUE handle, VALUE state) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(state, T_STRING);

    unsigned int result = setRulesetState((void *)FIX2LONG(handle), RSTRING_PTR(state));
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not set ruleset state, error code: %d", result);
        }
    }

    return Qnil;
}

void Init_rules() {
    rulesModule = rb_define_module("Rules");
    rb_define_singleton_method(rulesModule, "create_ruleset", rbCreateRuleset, 2);
    rb_define_singleton_method(rulesModule, "delete_ruleset", rbDeleteRuleset, 1);
    rb_define_singleton_method(rulesModule, "bind_ruleset", rbBindRuleset, 4);
    rb_define_singleton_method(rulesModule, "assert_event", rbAssertEvent, 2);
    rb_define_singleton_method(rulesModule, "assert_events", rbAssertEvents, 2);
    rb_define_singleton_method(rulesModule, "assert_state", rbAssertState, 2);
    rb_define_singleton_method(rulesModule, "start_action", rbStartAction, 1);
    rb_define_singleton_method(rulesModule, "complete_action", rbCompleteAction, 3);
    rb_define_singleton_method(rulesModule, "abandon_action", rbAbandonAction, 2);
    rb_define_singleton_method(rulesModule, "start_timer", rbStartTimer, 4);
    rb_define_singleton_method(rulesModule, "assert_timers", rbAssertTimers, 1);
    rb_define_singleton_method(rulesModule, "get_state", rbGetState, 2);
    rb_define_singleton_method(rulesModule, "get_ruleset_state", rbGetRulesetState, 1);
    rb_define_singleton_method(rulesModule, "set_ruleset_state", rbSetRulesetState, 2);
}


