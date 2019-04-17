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

    unsigned int result = deleteRuleset(FIX2INT(handle));
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not delete ruleset, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbCreateClient(VALUE self, VALUE name) {
    Check_Type(name, T_STRING);
    
    void *output = NULL;
    unsigned int result = createClient(&output, RSTRING_PTR(name));
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not create client, error code: %d", result);
        }
    }

    return INT2FIX(output);
}

static VALUE rbDeleteClient(VALUE self, VALUE handle) {
    Check_Type(handle, T_FIXNUM);

    unsigned int result = deleteClient((void *)FIX2LONG(handle));
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not delete client, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbBindRuleset(VALUE self, VALUE handle, VALUE host, VALUE port, VALUE password, VALUE db) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(host, T_STRING);
    Check_Type(port, T_FIXNUM);
    Check_Type(db, T_FIXNUM);

    unsigned int result;
    if (TYPE(password) == T_STRING) {
        result = bindRuleset(FIX2INT(handle), RSTRING_PTR(host), FIX2INT(port), RSTRING_PTR(password), FIX2INT(db));
    } else if (TYPE(password) == T_NIL) {
        result = bindRuleset(FIX2INT(handle), RSTRING_PTR(host), FIX2INT(port), NULL, FIX2INT(db));
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

static VALUE rbComplete(VALUE self, VALUE stateOffset, VALUE replyCount) {
    Check_Type(stateOffset, T_FIXNUM);
    Check_Type(replyCount, T_FIXNUM);
    
    unsigned int result = complete(FIX2LONG(stateOffset), FIX2LONG(replyCount));
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not complete action, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbStartAssertEvent(VALUE self, VALUE handle, VALUE event) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(event, T_STRING);

    unsigned int replyCount;
    unsigned int stateOffset;
    unsigned int result = startAssertEvent(FIX2INT(handle), RSTRING_PTR(event), &stateOffset, &replyCount);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED) {
        VALUE output = rb_ary_new(); 
        rb_ary_push(output, INT2FIX(stateOffset));
        rb_ary_push(output, INT2FIX(replyCount));
        return output;  
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not assert event, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbAssertEvent(VALUE self, VALUE handle, VALUE event) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(event, T_STRING);

    unsigned int result = assertEvent(FIX2INT(handle), RSTRING_PTR(event));
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED) {
        return INT2FIX(result);    
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not assert event, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbQueueAssertEvent(VALUE self, VALUE handle, VALUE sid, VALUE destination, VALUE event) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(sid, T_STRING);
    Check_Type(destination, T_STRING);
    Check_Type(event, T_STRING);

    unsigned int result = queueMessage(FIX2INT(handle), QUEUE_ASSERT_EVENT, RSTRING_PTR(sid), RSTRING_PTR(destination), RSTRING_PTR(event));
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not queue assert event, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbStartAssertEvents(VALUE self, VALUE handle, VALUE events) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(events, T_STRING);

    unsigned int replyCount;
    unsigned int stateOffset;
    unsigned int result = startAssertEvents(FIX2INT(handle), RSTRING_PTR(events), &stateOffset, &replyCount);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED) {
        VALUE output = rb_ary_new(); 
        rb_ary_push(output, INT2FIX(stateOffset));
        rb_ary_push(output, INT2FIX(replyCount));
        return output;  
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not assert events, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbAssertEvents(VALUE self, VALUE handle, VALUE events) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(events, T_STRING);

    unsigned int result = assertEvents(FIX2INT(handle), RSTRING_PTR(events));
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED) {
        return INT2FIX(result);    
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not assert events, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbRetractEvent(VALUE self, VALUE handle, VALUE event) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(event, T_STRING);

    unsigned int result = retractEvent(FIX2INT(handle), RSTRING_PTR(event));
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED) {
        return INT2FIX(result);    
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not retract event, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbStartAssertFact(VALUE self, VALUE handle, VALUE fact) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(fact, T_STRING);

    unsigned int replyCount;
    unsigned int stateOffset;
    unsigned int result = startAssertFact(FIX2INT(handle), RSTRING_PTR(fact), &stateOffset, &replyCount);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED) {
        VALUE output = rb_ary_new(); 
        rb_ary_push(output, INT2FIX(stateOffset));
        rb_ary_push(output, INT2FIX(replyCount));
        return output;  
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not assert fact, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbAssertFact(VALUE self, VALUE handle, VALUE fact) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(fact, T_STRING);

    unsigned int result = assertFact(FIX2INT(handle), RSTRING_PTR(fact));
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED) {
        return INT2FIX(result);    
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not assert fact, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbQueueAssertFact(VALUE self, VALUE handle, VALUE sid, VALUE destination, VALUE event) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(sid, T_STRING);
    Check_Type(destination, T_STRING);
    Check_Type(event, T_STRING);

    unsigned int result = queueMessage(FIX2INT(handle), QUEUE_ASSERT_FACT, RSTRING_PTR(sid), RSTRING_PTR(destination), RSTRING_PTR(event));
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not queue assert fact, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbStartAssertFacts(VALUE self, VALUE handle, VALUE facts) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(facts, T_STRING);

    unsigned int replyCount;
    unsigned int stateOffset;
    unsigned int result = startAssertFacts(FIX2INT(handle), RSTRING_PTR(facts), &stateOffset, &replyCount);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED) {
        VALUE output = rb_ary_new(); 
        rb_ary_push(output, INT2FIX(stateOffset));
        rb_ary_push(output, INT2FIX(replyCount));
        return output;  
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not assert facts, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbAssertFacts(VALUE self, VALUE handle, VALUE facts) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(facts, T_STRING);

    unsigned int result = assertFacts(FIX2INT(handle), RSTRING_PTR(facts));
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED) {
        return INT2FIX(result);    
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not assert facts, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbStartRetractFact(VALUE self, VALUE handle, VALUE fact) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(fact, T_STRING);

    unsigned int replyCount;
    unsigned int stateOffset;
    unsigned int result = startRetractFact(FIX2INT(handle), RSTRING_PTR(fact), &stateOffset, &replyCount);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED) {
        VALUE output = rb_ary_new(); 
        rb_ary_push(output, INT2FIX(stateOffset));
        rb_ary_push(output, INT2FIX(replyCount));
        return output;  
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not retract fact, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbRetractFact(VALUE self, VALUE handle, VALUE fact) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(fact, T_STRING);

    unsigned int result = retractFact(FIX2INT(handle), RSTRING_PTR(fact));
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED) {
        return INT2FIX(result);    
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not retract fact, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbQueueRetractFact(VALUE self, VALUE handle, VALUE sid, VALUE destination, VALUE event) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(sid, T_STRING);
    Check_Type(destination, T_STRING);
    Check_Type(event, T_STRING);

    unsigned int result = queueMessage(FIX2INT(handle), QUEUE_RETRACT_FACT, RSTRING_PTR(sid), RSTRING_PTR(destination), RSTRING_PTR(event));
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not queue retract fact, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbStartRetractFacts(VALUE self, VALUE handle, VALUE facts) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(facts, T_STRING);

    unsigned int replyCount;
    unsigned int stateOffset;
    unsigned int result = startRetractFacts(FIX2INT(handle), RSTRING_PTR(facts), &stateOffset, &replyCount);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED) {
        VALUE output = rb_ary_new(); 
        rb_ary_push(output, INT2FIX(stateOffset));
        rb_ary_push(output, INT2FIX(replyCount));
        return output;  
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not retract facts, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbRetractFacts(VALUE self, VALUE handle, VALUE facts) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(facts, T_STRING);

    unsigned int result = retractFacts(FIX2INT(handle), RSTRING_PTR(facts));
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not retract facts, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbUpdateState(VALUE self, VALUE handle, VALUE sid, VALUE state) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(sid, T_STRING);
    Check_Type(state, T_STRING);

    unsigned int result = updateState(FIX2INT(handle), RSTRING_PTR(sid), RSTRING_PTR(state));
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED || result == ERR_EVENT_OBSERVED) {
        return INT2FIX(result);    
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not assert event, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbStartUpdateState(VALUE self, VALUE handle, VALUE actionHandle, VALUE state) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(actionHandle, T_FIXNUM);
    Check_Type(state, T_STRING);

    unsigned int replyCount;
    unsigned int stateOffset;
    unsigned int result = startUpdateState(FIX2INT(handle), (void *)FIX2LONG(actionHandle), RSTRING_PTR(state), &stateOffset, &replyCount);
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED) {
        VALUE output = rb_ary_new(); 
        rb_ary_push(output, INT2FIX(stateOffset));
        rb_ary_push(output, INT2FIX(replyCount));
        return output;  
    } else {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not start update state, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbStartAction(VALUE self, VALUE handle) {
    Check_Type(handle, T_FIXNUM);

    char *state;
    char *messages;
    unsigned int stateOffset;
    unsigned int result = startAction(FIX2INT(handle), &state, &messages, &stateOffset);
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
    rb_ary_push(output, INT2FIX(stateOffset));
    return output;
}

static VALUE rbCompleteAndStartAction(VALUE self, VALUE handle, VALUE expectedReplies, VALUE stateOffset) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(expectedReplies, T_FIXNUM);
    Check_Type(stateOffset, T_FIXNUM);

    char *messages;
    unsigned int result = completeAndStartAction(FIX2INT(handle), FIX2LONG(expectedReplies), FIX2LONG(stateOffset), &messages);
    if (result == ERR_NO_ACTION_AVAILABLE) {
        return Qnil;
    } else if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not complete and start action, error code: %d", result);
        }
    }

    return rb_str_new2(messages);
}


static VALUE rbAbandonAction(VALUE self, VALUE handle, VALUE stateOffset) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(stateOffset, T_FIXNUM);

    unsigned int result = abandonAction(FIX2INT(handle), (void *)FIX2LONG(stateOffset));
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not abandon action, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbStartTimer(VALUE self, VALUE handle, VALUE sid, VALUE duration, VALUE manualReset, VALUE timer) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(sid, T_STRING);
    Check_Type(duration, T_FIXNUM);
    Check_Type(manualReset, T_FIXNUM);
    Check_Type(timer, T_STRING);    

    unsigned int result = startTimer(FIX2INT(handle), RSTRING_PTR(sid), FIX2UINT(duration), FIX2SHORT(manualReset), RSTRING_PTR(timer));
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not start timer, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbCancelTimer(VALUE self, VALUE handle, VALUE sid, VALUE timerName) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(sid, T_STRING);
    Check_Type(timerName, T_STRING);    

    unsigned int result = cancelTimer(FIX2INT(handle), RSTRING_PTR(sid), RSTRING_PTR(timerName));
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not cancel timer, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbAssertTimers(VALUE self, VALUE handle) {
    Check_Type(handle, T_FIXNUM);

    unsigned int result = assertTimers(FIX2INT(handle));
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
    unsigned int result = getState(FIX2INT(handle), RSTRING_PTR(sid), &state);
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

static VALUE rbDeleteState(VALUE self, VALUE handle, VALUE sid) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(sid, T_STRING);

    unsigned int result = deleteState(FIX2INT(handle), RSTRING_PTR(sid));
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not delete state, error code: %d", result);
        }
    }

    return Qnil;
}

static VALUE rbRenewActionLease(VALUE self, VALUE handle, VALUE sid) {
    Check_Type(handle, T_FIXNUM);
    Check_Type(sid, T_STRING);

    unsigned int result = renewActionLease(FIX2INT(handle), RSTRING_PTR(sid));
    if (result != RULES_OK) {
        if (result == ERR_OUT_OF_MEMORY) {
            rb_raise(rb_eNoMemError, "Out of memory");
        } else { 
            rb_raise(rb_eException, "Could not renew action lease, error code: %d", result);
        }
    }

    return Qnil;
}

void Init_rules() {
    rulesModule = rb_define_module("Rules");
    rb_define_singleton_method(rulesModule, "create_ruleset", rbCreateRuleset, 2);
    rb_define_singleton_method(rulesModule, "delete_ruleset", rbDeleteRuleset, 1);
    rb_define_singleton_method(rulesModule, "create_client", rbCreateClient, 1);
    rb_define_singleton_method(rulesModule, "delete_client", rbDeleteClient, 1);
    rb_define_singleton_method(rulesModule, "bind_ruleset", rbBindRuleset, 5);
    rb_define_singleton_method(rulesModule, "complete", rbComplete, 2);
    rb_define_singleton_method(rulesModule, "assert_event", rbAssertEvent, 2);
    rb_define_singleton_method(rulesModule, "queue_assert_event", rbQueueAssertEvent, 4);
    rb_define_singleton_method(rulesModule, "start_assert_event", rbStartAssertEvent, 2);
    rb_define_singleton_method(rulesModule, "assert_events", rbAssertEvents, 2);
    rb_define_singleton_method(rulesModule, "start_assert_events", rbStartAssertEvents, 2);
    rb_define_singleton_method(rulesModule, "retract_event", rbRetractEvent, 2);
    rb_define_singleton_method(rulesModule, "start_assert_fact", rbStartAssertFact, 2);
    rb_define_singleton_method(rulesModule, "assert_fact", rbAssertFact, 2);
    rb_define_singleton_method(rulesModule, "queue_assert_fact", rbQueueAssertFact, 4);
    rb_define_singleton_method(rulesModule, "start_assert_facts", rbStartAssertFacts, 2);
    rb_define_singleton_method(rulesModule, "assert_facts", rbAssertFacts, 2);
    rb_define_singleton_method(rulesModule, "start_retract_fact", rbStartRetractFact, 2);
    rb_define_singleton_method(rulesModule, "retract_fact", rbRetractFact, 2);
    rb_define_singleton_method(rulesModule, "queue_retract_fact", rbQueueRetractFact, 4);
    rb_define_singleton_method(rulesModule, "start_retract_facts", rbStartRetractFacts, 2);
    rb_define_singleton_method(rulesModule, "retract_facts", rbRetractFacts, 2);
    rb_define_singleton_method(rulesModule, "update_state", rbUpdateState, 3);
    rb_define_singleton_method(rulesModule, "start_update_state", rbStartUpdateState, 3);
    rb_define_singleton_method(rulesModule, "start_action", rbStartAction, 1);
    rb_define_singleton_method(rulesModule, "complete_and_start_action", rbCompleteAndStartAction, 3);
    rb_define_singleton_method(rulesModule, "abandon_action", rbAbandonAction, 2);
    rb_define_singleton_method(rulesModule, "start_timer", rbStartTimer, 5);
    rb_define_singleton_method(rulesModule, "cancel_timer", rbCancelTimer, 3);
    rb_define_singleton_method(rulesModule, "assert_timers", rbAssertTimers, 1);
    rb_define_singleton_method(rulesModule, "get_state", rbGetState, 2);
    rb_define_singleton_method(rulesModule, "delete_state", rbDeleteState, 2);
    rb_define_singleton_method(rulesModule, "renew_action_lease", rbRenewActionLease, 2);
}


