#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifndef _WIN32
#include <time.h> /* for struct timeval */
#else
#include <WinSock2.h>
#endif
#include "net.h"
#include "rules.h"
#include "json.h"


#ifdef _WIN32
int asprintf(char** ret, char* format, ...){
    va_list args;
    *ret = NULL;
    if (!format) return 0;
    va_start(args, format);
    int size = _vscprintf(format, args);
    if (size == 0) {
        *ret = (char*)malloc(1);
        **ret = 0;
    }
    else {
        size++; //for null
        *ret = (char*)malloc(size + 2);
        if (*ret) {
            _vsnprintf(*ret, size, format, args);
        }
        else {
            return -1;
        }
    }

    va_end(args);
    return size;
}
#endif

static unsigned int createIdiom(ruleset *tree, jsonValue *newValue, char **idiomString) {
    char *rightProperty;
    char *rightAlias;
    char *valueString;
    idiom *newIdiom;
    switch (newValue->type) {
        case JSON_EVENT_PROPERTY:
            rightProperty = &tree->stringPool[newValue->value.property.nameOffset];
            rightAlias = &tree->stringPool[newValue->value.property.idOffset];
            if (asprintf(idiomString, "frame[\"%s\"][\"%s\"]", rightAlias, rightProperty) == -1) {
                return ERR_OUT_OF_MEMORY;
            }
            break;
        case JSON_EVENT_IDIOM:
            newIdiom = &tree->idiomPool[newValue->value.idiomOffset];
            char *op = "";
            switch (newIdiom->operator) {
                case OP_ADD:
                    op = "+";
                    break;
                case OP_SUB:
                    op = "-";
                    break;
                case OP_MUL:
                    op = "*";
                    break;
                case OP_DIV:
                    op = "/";
                    break;
            }
            
            char *rightIdiomString = NULL;
            unsigned int result = createIdiom(tree, &newIdiom->right, &rightIdiomString);
            if (result != RULES_OK) {
                return result;
            }

            char *leftIdiomString = NULL;
            result = createIdiom(tree, &newIdiom->left, &leftIdiomString);
            if (result != RULES_OK) {
                return result;
            }

            if (asprintf(idiomString, "(%s %s %s)", leftIdiomString, op, rightIdiomString) == -1) {
                return ERR_OUT_OF_MEMORY;
            }
            
            free(rightIdiomString);
            free(leftIdiomString);
            break;
        case JSON_STRING:
            valueString = &tree->stringPool[newValue->value.stringOffset];
            if (asprintf(idiomString, "%s", valueString) == -1) {
                return ERR_OUT_OF_MEMORY;
            }
            break;
        case JSON_INT:
            if (asprintf(idiomString, "%ld", newValue->value.i) == -1) {
                return ERR_OUT_OF_MEMORY;
            }
            break;
        case JSON_DOUBLE:
            if (asprintf(idiomString, "%g", newValue->value.d) == -1) {
                return ERR_OUT_OF_MEMORY;
            }
            break;
        case JSON_BOOL:
            if (newValue->value.b == 0) {
                if (asprintf(idiomString, "false") == -1) {
                    return ERR_OUT_OF_MEMORY;
                }
            }
            else {
                if (asprintf(idiomString, "true") == -1) {
                    return ERR_OUT_OF_MEMORY;
                }
            }
            break;
    }

    return RULES_OK;
}

static unsigned int createTest(ruleset *tree, expression *expr, char **test, char **primaryKey, char **primaryFrameKey) {
    char *comp = NULL;
    char *compStack[32];
    unsigned char compTop = 0;
    unsigned char first = 1;
    unsigned char setPrimaryKey = 0;
    *primaryKey = NULL;
    *primaryFrameKey = NULL;
    *test = (char*)calloc(1, sizeof(char));
    if (!*test) {
        return ERR_OUT_OF_MEMORY;
    }
    
    for (unsigned short i = 0; i < expr->termsLength; ++i) {
        unsigned int currentNodeOffset = tree->nextPool[expr->t.termsOffset + i];
        node *currentNode = &tree->nodePool[currentNodeOffset];
        if (currentNode->value.a.operator == OP_AND) {
            char *oldTest = *test;
            if (first) {
                setPrimaryKey = 1;
                if (asprintf(test, "%s(", *test) == -1) {
                    return ERR_OUT_OF_MEMORY;
                }
            } else {
                if (asprintf(test, "%s %s (", *test, comp) == -1) {
                    return ERR_OUT_OF_MEMORY;
                }
            }
            free(oldTest);
            
            compStack[compTop] = comp;
            ++compTop;
            comp = "and";
            first = 1;
        } else if (currentNode->value.a.operator == OP_OR) {    
            char *oldTest = *test;
            if (first) {
                if (asprintf(test, "%s(", *test) == -1) {
                    return ERR_OUT_OF_MEMORY;
                }
            } else {
                setPrimaryKey = 0;
                if (asprintf(test, "%s %s (", *test, comp) == -1) {
                    return ERR_OUT_OF_MEMORY;
                }
            }
            free(oldTest); 
            
            compStack[compTop] = comp;
            ++compTop;
            comp = "or";
            first = 1;           
        } else if (currentNode->value.a.operator == OP_END) {
            --compTop;
            comp = compStack[compTop];
            
            char *oldTest = *test;
            if (asprintf(test, "%s)", *test)  == -1) {
                return ERR_OUT_OF_MEMORY;
            }
            free(oldTest);            
        } else {
            char *leftProperty = &tree->stringPool[currentNode->nameOffset];
            char *op = "";
            switch (currentNode->value.a.operator) {
                case OP_LT:
                    op = "<";
                    break;
                case OP_LTE:
                    op = "<=";
                    break;
                case OP_GT:
                    op = ">";
                    break;
                case OP_GTE:
                    op = ">=";
                    break;
                case OP_EQ:
                    op = "==";
                    break;
                case OP_NEQ:
                    op = "~=";
                    break;
            }

            char *idiomString = NULL;
            unsigned int result = createIdiom(tree, &currentNode->value.a.right, &idiomString);
            if (result != RULES_OK) {
                return result;
            }

            char *oldTest = *test;
            if (first) {
                if (asprintf(test, "%smessage[\"%s\"] %s %s", *test, leftProperty, op, idiomString)  == -1) {
                    return ERR_OUT_OF_MEMORY;
                }
                first = 0;
            } else {
                if (asprintf(test, "%s %s message[\"%s\"] %s %s", *test, comp, leftProperty, op, idiomString) == -1) {
                    return ERR_OUT_OF_MEMORY;
                }
            }

            if (setPrimaryKey && currentNode->value.a.operator == OP_EQ) {
                if (*primaryKey == NULL) {
                    if (asprintf(primaryKey, 
"    if not message[\"%s\"] then\n"
"        return \"\"\n"
"    else\n"
"        result = result .. message[\"%s\"]\n"
"    end\n", 
                                 leftProperty, 
                                 leftProperty)  == -1) {
                        return ERR_OUT_OF_MEMORY;
                    }

                    if (asprintf(primaryFrameKey, 
"    if not %s then\n"
"        return \"\"\n"
"    else\n"
"        result = result .. %s\n"
"    end\n",
                                idiomString,
                                idiomString)  == -1) {
                        return ERR_OUT_OF_MEMORY;
                    }
                }
                else {
                    char *oldKey = *primaryKey;
                    if (asprintf(primaryKey, 
"%s    if not message[\"%s\"] then\n"
"        return \"\"\n"
"    else\n"
"        result = result .. message[\"%s\"]\n"
"    end\n",    
                                 *primaryKey, 
                                 leftProperty,
                                 leftProperty)  == -1) {
                        return ERR_OUT_OF_MEMORY;
                    }
                    free(oldKey);
                    oldKey = *primaryFrameKey;
                    if (asprintf(primaryFrameKey, 
"%s    if not %s then\n"
"        return \"\"\n"
"    else\n"
"        result = result .. %s\n"
"    end\n", 
                                 *primaryFrameKey, 
                                 idiomString,
                                 idiomString)  == -1) {
                        return ERR_OUT_OF_MEMORY;
                    }
                    free(oldKey);
                }
            }

            free(idiomString);
            free(oldTest);
        }
    }

    if (first) {
        free(*test);
        if (asprintf(test, "1")  == -1) {
            return ERR_OUT_OF_MEMORY;
        }
    }

    if (*primaryKey == NULL) {
        *primaryKey = (char*)calloc(1, sizeof(char));
        if (!*primaryKey) {
            return ERR_OUT_OF_MEMORY;
        }

        *primaryFrameKey = (char*)calloc(1, sizeof(char));
        if (!*primaryFrameKey) {
            return ERR_OUT_OF_MEMORY;
        }
    }
    return RULES_OK;
}

static unsigned int loadPartitionCommand(ruleset *tree, binding *rulesBinding) {
    char *name = &tree->stringPool[tree->nameOffset];
    redisContext *reContext = rulesBinding->reContext;
    redisReply *reply;
    char *lua = NULL;
    if (asprintf(&lua, 
"local partition_key = \"%s!p\"\n"
"local res = redis.call(\"hget\", partition_key, ARGV[1])\n"
"if (not res) then\n"
"   res = redis.call(\"hincrby\", partition_key, \"index\", 1)\n"
"   res = res %% tonumber(ARGV[2])\n"
"   redis.call(\"hset\", partition_key, ARGV[1], res)\n"
"end\n"
"return tonumber(res)\n", name)  == -1) {
        return ERR_OUT_OF_MEMORY;
    }

    redisAppendCommand(reContext, "SCRIPT LOAD %s", lua); 
    redisGetReply(reContext, (void**)&reply);
    if (reply->type == REDIS_REPLY_ERROR) {
        printf("%s\n", reply->str);
        freeReplyObject(reply);
        return ERR_REDIS_ERROR;
    }

    strncpy(rulesBinding->partitionHash, reply->str, 40);
    rulesBinding->partitionHash[40] = '\0';
    freeReplyObject(reply);
    free(lua);
    return RULES_OK;
}

static unsigned int loadRemoveActionCommand(ruleset *tree, binding *rulesBinding) {
    char *name = &tree->stringPool[tree->nameOffset];
    redisContext *reContext = rulesBinding->reContext;
    redisReply *reply;
    char *lua = NULL;
    if (asprintf(&lua, 
"local delete_frame = function(key, action_key)\n"
"    local rule_action_key = redis.call(\"lpop\", key)\n"
"    local raw_count = redis.call(\"lpop\", key)\n"
"    local count = 1\n"
"    if raw_count ~= \"single\" then\n"
"        count = tonumber(raw_count)\n"
"    end\n"
"    if count == 0 then\n"
"        local packed_frame = redis.call(\"lpop\", rule_action_key)\n"
"        while packed_frame ~= \"0\" do\n"
"            packed_frame = redis.call(\"lpop\", rule_action_key)\n"
"        end\n"
"    else\n"
"        for i = 0, count - 1, 1 do\n"
"            local packed_frame = redis.call(\"rpop\", rule_action_key)\n"
"        end\n"
"    end\n"
"    return (redis.call(\"llen\", key) > 0)\n"
"end\n"
"local sid = ARGV[1]\n"
"local max_score = tonumber(ARGV[2])\n"
"local action_key = \"%s!a\"\n"
"if delete_frame(action_key .. \"!\" .. sid, action_key) then\n"
"    redis.call(\"zadd\", action_key , max_score, sid)\n"
"else\n"
"    redis.call(\"zrem\", action_key, sid)\n"
"end\n", name)  == -1) {
        return ERR_OUT_OF_MEMORY;
    }

    redisAppendCommand(reContext, "SCRIPT LOAD %s", lua);
    redisGetReply(reContext, (void**)&reply);
    if (reply->type == REDIS_REPLY_ERROR) {
        printf("%s\n", reply->str);
        freeReplyObject(reply);
        free(lua);
        return ERR_REDIS_ERROR;
    }

    strncpy(rulesBinding->removeActionHash, reply->str, 40);
    rulesBinding->removeActionHash[40] = '\0';
    freeReplyObject(reply);
    free(lua);
    return RULES_OK;
}

static unsigned int loadTimerCommand(ruleset *tree, binding *rulesBinding) {
    char *name = &tree->stringPool[tree->nameOffset];
    redisContext *reContext = rulesBinding->reContext;
    redisReply *reply;
    char *lua = NULL;
    if (asprintf(&lua,
"local timer_key = \"%s!t\"\n"
"local timestamp = tonumber(ARGV[1])\n"
"local res = redis.call(\"zrangebyscore\", timer_key, 0, timestamp, \"limit\", 0, 50)\n"
"if #res > 0 then\n"
"  for i = 0, #res, 1 do\n"
"    redis.call(\"zincrby\", timer_key, 10, res[i])\n"
"  end\n"
"  return res\n"
"end\n"
"return 0\n", name)  == -1) {
        return ERR_OUT_OF_MEMORY;
    }

    redisAppendCommand(reContext, "SCRIPT LOAD %s", lua);
    redisGetReply(reContext, (void**)&reply);
    if (reply->type == REDIS_REPLY_ERROR) {
        printf("%s\n", reply->str);
        freeReplyObject(reply);
        return ERR_REDIS_ERROR;
    }

    strncpy(rulesBinding->timersHash, reply->str, 40);
    rulesBinding->timersHash[40] = '\0';
    freeReplyObject(reply);
    free(lua);
    return RULES_OK;
}

static unsigned int loadUpdateActionCommand(ruleset *tree, binding *rulesBinding) {
    char *name = &tree->stringPool[tree->nameOffset];
    redisContext *reContext = rulesBinding->reContext;
    redisReply *reply;
    char *lua = NULL;
    if (asprintf(&lua,
"local actions_key = \"%s!a\"\n"
"local score = tonumber(ARGV[2])\n"
"local sid = ARGV[1]\n"
"if redis.call(\"zscore\", actions_key, sid) then\n"
"    redis.call(\"zadd\", actions_key , score, sid)\n"
"end\n", name)  == -1) {
        return ERR_OUT_OF_MEMORY;
    }

    redisAppendCommand(reContext, "SCRIPT LOAD %s", lua);
    redisGetReply(reContext, (void**)&reply);
    if (reply->type == REDIS_REPLY_ERROR) {
        printf("%s\n", reply->str);
        freeReplyObject(reply);
        return ERR_REDIS_ERROR;
    }

    strncpy(rulesBinding->updateActionHash, reply->str, 40);
    rulesBinding->updateActionHash[40] = '\0';
    freeReplyObject(reply);
    free(lua);
    return RULES_OK;
}

static unsigned int loadDeleteSessionCommand(ruleset *tree, binding *rulesBinding) {
    char *name = &tree->stringPool[tree->nameOffset];
    redisContext *reContext = rulesBinding->reContext;
    redisReply *reply;
    char *lua = NULL;
    char *deleteSessionLua = NULL;
    deleteSessionLua = (char*)calloc(1, sizeof(char));
    if (!deleteSessionLua) {
        return ERR_OUT_OF_MEMORY;
    }

    for (unsigned int i = 0; i < tree->nodeOffset; ++i) {
        node *currentNode = &tree->nodePool[i];
        
        if (currentNode->type == NODE_ACTION) {
            char *actionName = &tree->stringPool[currentNode->nameOffset];
            char *oldDeleteSessionLua = NULL;
            for (unsigned int ii = 0; ii < currentNode->value.c.joinsLength; ++ii) {
                unsigned int currentJoinOffset = tree->nextPool[currentNode->value.c.joinsOffset + ii];
                join *currentJoin = &tree->joinPool[currentJoinOffset];

                oldDeleteSessionLua = deleteSessionLua;
                if (asprintf(&deleteSessionLua, 
"%sredis.call(\"del\", \"%s!%d!r!\" .. sid)\n"
"redis.call(\"del\", \"%s!%d!r!\" .. sid .. \"!d\")\n",
                            deleteSessionLua,
                            actionName,
                            ii,
                            actionName,
                            ii)  == -1) {
                    return ERR_OUT_OF_MEMORY;
                }  
                free(oldDeleteSessionLua);

                for (unsigned int iii = 0; iii < currentJoin->expressionsLength; ++iii) {
                    unsigned int expressionOffset = tree->nextPool[currentJoin->expressionsOffset + iii];
                    expression *expr = &tree->expressionPool[expressionOffset];
                    char *currentKey = &tree->stringPool[expr->nameOffset];

                    oldDeleteSessionLua = deleteSessionLua;
                    if (asprintf(&deleteSessionLua, 
"%sdelete_message_keys(\"%s!f!\" .. sid)\n"
"delete_message_keys(\"%s!e!\" .. sid)\n"
"delete_closure_keys(\"%s!c!\" .. sid)\n"
"delete_closure_keys(\"%s!i!\" .. sid)\n",
                                deleteSessionLua,
                                currentKey,
                                currentKey,
                                currentKey,
                                currentKey)  == -1) {
                        return ERR_OUT_OF_MEMORY;
                    }  
                    free(oldDeleteSessionLua);
                }
            }
        }
    }


    if (asprintf(&lua, 
"local sid = ARGV[1]\n"
"local delete_message_keys = function(events_key)\n"
"    local all_keys = redis.call(\"keys\", events_key .. \"!m!*\")\n"
"    for i = 1, #all_keys, 1 do\n"
"        redis.call(\"del\", all_keys[i])\n"
"    end\n"
"    redis.call(\"del\", events_key)\n"
"end\n"
"local delete_closure_keys = function(closure_key)\n"
"    local all_keys = redis.call(\"keys\", closure_key .. \"!*\")\n"
"    for i = 1, #all_keys, 1 do\n"
"        redis.call(\"del\", all_keys[i])\n"
"    end\n"
"end\n"
"redis.call(\"hdel\", \"%s!s\", sid)\n"
"redis.call(\"zrem\", \"%s!a\", sid)\n"
"redis.call(\"del\", \"%s!a!\" .. sid)\n"
"redis.call(\"del\", \"%s!e!\" .. sid)\n"
"redis.call(\"del\", \"%s!f!\" .. sid)\n"
"redis.call(\"del\", \"%s!v!\" .. sid)\n%s",
                name,
                name,
                name, 
                name,
                name, 
                name,
                deleteSessionLua)  == -1) {
        return ERR_OUT_OF_MEMORY;
    }

    free(deleteSessionLua);
    redisAppendCommand(reContext, "SCRIPT LOAD %s", lua);
    redisGetReply(reContext, (void**)&reply);
    if (reply->type == REDIS_REPLY_ERROR) {
        printf("%s\n", reply->str);
        freeReplyObject(reply);
        free(lua);
        return ERR_REDIS_ERROR;
    }

    strncpy(rulesBinding->deleteSessionHash, reply->str, 40);
    rulesBinding->deleteSessionHash[40] = '\0';
    freeReplyObject(reply);
    free(lua);
    return RULES_OK;

}
static unsigned int loadAddMessageCommand(ruleset *tree, binding *rulesBinding) {
    char *name = &tree->stringPool[tree->nameOffset];
    redisContext *reContext = rulesBinding->reContext;
    redisReply *reply;
    char *lua = NULL;
    char *addMessageLua = NULL;
    addMessageLua = (char*)calloc(1, sizeof(char));
    if (!addMessageLua) {
        return ERR_OUT_OF_MEMORY;
    }

    for (unsigned int i = 0; i < tree->nodeOffset; ++i) {
        node *currentNode = &tree->nodePool[i];
        
        if (currentNode->type == NODE_ACTION) {
            char *oldAddMessageLua = NULL;
            for (unsigned int ii = 0; ii < currentNode->value.c.joinsLength; ++ii) {
                unsigned int currentJoinOffset = tree->nextPool[currentNode->value.c.joinsOffset + ii];
                join *currentJoin = &tree->joinPool[currentJoinOffset];

                for (unsigned int iii = 0; iii < currentJoin->expressionsLength; ++iii) {
                    unsigned int expressionOffset = tree->nextPool[currentJoin->expressionsOffset + iii];
                    expression *expr = &tree->expressionPool[expressionOffset];
                    char *currentKey = &tree->stringPool[expr->nameOffset];
                    if (iii == 0) {
                        oldAddMessageLua = addMessageLua;
                        if (!addMessageLua) {
                            addMessageLua = "";
                        }

                        if (asprintf(&addMessageLua, 
"%sif input_keys[\"%s\"] then\n"
"    primary_message_keys[\"%s\"] = function(message)\n"
"        return \"\"\n"
"    end\n"
"end\n",
                                    addMessageLua,
                                    currentKey,
                                    currentKey)  == -1) {
                            return ERR_OUT_OF_MEMORY;
                        }  
                        free(oldAddMessageLua);
                    } else {
                        char *test = NULL;
                        char *primaryKeyLua = NULL;
                        char *primaryFrameKeyLua = NULL;
                        unsigned int result = createTest(tree, expr, &test, &primaryKeyLua, &primaryFrameKeyLua);
                        if (result != RULES_OK) {
                            return result;
                        }

                        oldAddMessageLua = addMessageLua;
                        if (asprintf(&addMessageLua, 
"%sif input_keys[\"%s\"] then\n"
"    primary_message_keys[\"%s\"] = function(message)\n"
"        local result = \"\"\n%s"
"        return result\n"
"    end\n"
"end\n",
                                    addMessageLua,
                                    currentKey, 
                                    currentKey, 
                                    primaryKeyLua)  == -1) {
                            return ERR_OUT_OF_MEMORY;
                        }  
                        free(oldAddMessageLua);
                        free(test);
                        free(primaryKeyLua);
                        free(primaryFrameKeyLua);
                    }
                }
            }
        }
    }

    if (asprintf(&lua, 
"local sid = ARGV[1]\n"
"local assert_fact = tonumber(ARGV[2])\n"
"local keys_count = tonumber(ARGV[3])\n"
"local events_hashset = \"%s!e!\" .. sid\n"
"local facts_hashset = \"%s!f!\" .. sid\n"
"local visited_hashset = \"%s!v!\" .. sid\n"
"local message = {}\n"
"local primary_message_keys = {}\n"
"local input_keys = {}\n"
"local save_message = function(current_key, message, events_key, messages_key)\n"
"    redis.call(\"hsetnx\", messages_key, message[\"id\"], cmsgpack.pack(message))\n"
"    local primary_key = primary_message_keys[current_key](message)\n"
"    redis.call(\"lpush\", events_key .. \"!m!\" .. primary_key, message[\"id\"])\n"
"end\n"
"for index = 4 + keys_count, #ARGV, 3 do\n"
"    if ARGV[index + 2] == \"1\" then\n"
"        message[ARGV[index]] = ARGV[index + 1]\n"
"    elseif ARGV[index + 2] == \"2\" or  ARGV[index + 2] == \"3\" then\n"
"        message[ARGV[index]] = tonumber(ARGV[index + 1])\n"
"    elseif ARGV[index + 2] == \"4\" then\n"
"        if ARGV[index + 1] == \"true\" then\n"
"            message[ARGV[index]] = true\n"
"        else\n"
"            message[ARGV[index]] = false\n"
"        end\n"
"    end\n"
"end\n"
"local mid = message[\"id\"]\n"
"if redis.call(\"hsetnx\", visited_hashset, message[\"id\"], 1) == 0 then\n"
"    if assert_fact == 0 then\n"
"        if not redis.call(\"hget\", events_hashset, mid) then\n"
"            return false\n"
"        end\n"
"    else\n"
"        if not redis.call(\"hget\", facts_hashset, mid) then\n"
"            return false\n"
"        end\n"
"    end\n"
"end\n"
"for index = 4, 3 + keys_count, 1 do\n"
"    input_keys[ARGV[index]] = true\n"
"end\n"
"%sif assert_fact == 1 then\n"
"    message[\"$f\"] = 1\n"
"    for index = 4, 3 + keys_count, 1 do\n"
"        local key = ARGV[index]\n"
"        save_message(key, message, key .. \"!f!\" .. sid, facts_hashset)\n"
"    end\n"
"else\n"
"    for index = 4, 3 + keys_count, 1 do\n"
"        local key = ARGV[index]\n"
"        save_message(key, message, key .. \"!e!\" .. sid, events_hashset)\n"
"    end\n"
"end\n",
                name,
                name,
                name, 
                addMessageLua)  == -1) {
        return ERR_OUT_OF_MEMORY;
    }

    free(addMessageLua);
    redisAppendCommand(reContext, "SCRIPT LOAD %s", lua);
    redisGetReply(reContext, (void**)&reply);
    if (reply->type == REDIS_REPLY_ERROR) {
        printf("%s\n", reply->str);
        freeReplyObject(reply);
        free(lua);
        return ERR_REDIS_ERROR;
    }

    strncpy(rulesBinding->addMessageHash, reply->str, 40);
    rulesBinding->addMessageHash[40] = '\0';
    freeReplyObject(reply);
    free(lua);
    return RULES_OK;
}

static unsigned int loadPeekActionCommand(ruleset *tree, binding *rulesBinding) {
    char *name = &tree->stringPool[tree->nameOffset];
    redisContext *reContext = rulesBinding->reContext;
    redisReply *reply;
    char *lua = NULL;
    
    char *peekActionLua = NULL;
    peekActionLua = (char*)calloc(1, sizeof(char));
    if (!peekActionLua) {
        return ERR_OUT_OF_MEMORY;
    }

    for (unsigned int i = 0; i < tree->nodeOffset; ++i) {
        char *oldPeekActionLua;
        node *currentNode = &tree->nodePool[i];
        
        if (currentNode->type == NODE_ACTION) {
            char *unpackFrameLua = NULL;
            char *oldUnpackFrameLua = NULL;
            char *actionName = &tree->stringPool[currentNode->nameOffset];
            for (unsigned int ii = 0; ii < currentNode->value.c.joinsLength; ++ii) {
                unsigned int currentJoinOffset = tree->nextPool[currentNode->value.c.joinsOffset + ii];
                join *currentJoin = &tree->joinPool[currentJoinOffset];
                
                oldPeekActionLua = peekActionLua;
                if (asprintf(&peekActionLua, 
"%sif input_keys[\"%s!%d!r!\"] then\n" 
"local context = {}\n"
"context_directory[\"%s!%d!r!\"] = context\n"
"reviewers = {}\n"
"context[\"reviewers\"] = reviewers\n"
"keys = {}\n"
"context[\"keys\"] = keys\n"
"primary_frame_keys = {}\n"
"context[\"primary_frame_keys\"] = primary_frame_keys\n",
                            peekActionLua,
                            actionName,
                            ii,
                            actionName,
                            ii)  == -1) {
                    return ERR_OUT_OF_MEMORY;
                }  
                free(oldPeekActionLua);

                for (unsigned int iii = 0; iii < currentJoin->expressionsLength; ++iii) {
                    unsigned int expressionOffset = tree->nextPool[currentJoin->expressionsOffset + iii];
                    expression *expr = &tree->expressionPool[expressionOffset];
                    char *currentAlias = &tree->stringPool[expr->aliasOffset];
                    char *currentKey = &tree->stringPool[expr->nameOffset];
                    
                    if (iii == 0) {
                        if (expr->not) { 
                            if (asprintf(&unpackFrameLua,
"    result[\"%s\"] = \"$n\"\n",
                                        currentAlias)  == -1) {
                                return ERR_OUT_OF_MEMORY;
                            }

                            oldPeekActionLua = peekActionLua;
                            if (!peekActionLua) {
                                peekActionLua = "";
                            }

                            if (asprintf(&peekActionLua, 
"%skeys[1] = \"%s\"\n"
"reviewers[1] = function(message, frame, index)\n"
"    if not message then\n"
"        return true\n"
"    end\n"
"    return false\n"
"end\n"
"primary_frame_keys[1] = function(frame)\n"
"    return \"\"\n"
"end\n",
                                         peekActionLua,
                                         currentKey)  == -1) {
                                return ERR_OUT_OF_MEMORY;
                            }  
                            free(oldPeekActionLua);
                        } else {
                            if (asprintf(&unpackFrameLua,
"    message = fetch_message(frame[1])\n"
"    if not message then\n"
"        return nil\n"
"    end\n"
"    result[\"%s\"] = message\n",
                                        currentAlias)  == -1) {
                                return ERR_OUT_OF_MEMORY;
                            }

                        }
                    } else {
                        if (expr->not) {
                            char *test = NULL;
                            char *primaryKeyLua = NULL;
                            char *primaryFrameKeyLua = NULL;
                            unsigned int result = createTest(tree, expr, &test, &primaryKeyLua, &primaryFrameKeyLua);
                            if (result != RULES_OK) {
                                return result;
                            }

                            oldUnpackFrameLua = unpackFrameLua;
                            if (asprintf(&unpackFrameLua,
"%s    result[\"%s\"] = \"$n\"\n",
                                        unpackFrameLua,
                                        currentAlias)  == -1) {
                                return ERR_OUT_OF_MEMORY;
                            }
                            free(oldUnpackFrameLua);

                            oldPeekActionLua = peekActionLua;
                            if (asprintf(&peekActionLua, 
"%skeys[%d] = \"%s\"\n"
"reviewers[%d] = function(message, frame, index)\n"
"    if not message or not (%s) then\n"
"        return true\n"
"    end\n"
"    return false\n"
"end\n"
"primary_frame_keys[%d] = function(frame)\n"
"    local result = \"\"\n%s"
"    return result\n"
"end\n",
                                         peekActionLua,
                                         iii + 1, 
                                         currentKey,
                                         iii + 1,
                                         test,
                                         iii + 1,
                                         primaryFrameKeyLua)  == -1) {
                                return ERR_OUT_OF_MEMORY;
                            }  
                            free(oldPeekActionLua);
                            free(test);
                            free(primaryKeyLua);
                            free(primaryFrameKeyLua);
                        } else {
                            oldUnpackFrameLua = unpackFrameLua;
                            if (asprintf(&unpackFrameLua,
"%s    message = fetch_message(frame[%d])\n"
"    if not message then\n"
"        return nil\n"
"    end\n"
"    result[\"%s\"] = message\n",
                                         unpackFrameLua,
                                         iii + 1,
                                         currentAlias)  == -1) {
                                return ERR_OUT_OF_MEMORY;
                            }
                            free(oldUnpackFrameLua);
                        }
                    }
                }

                oldPeekActionLua = peekActionLua;
                if (asprintf(&peekActionLua, 
"%scontext[\"frame_restore\"] = function(frame, result)\n"
"    local message\n%s"
"    return true\n"
"end\n"
"return context\n"
"end\n",
                             peekActionLua,
                             unpackFrameLua)  == -1) {
                    return ERR_OUT_OF_MEMORY;
                }  
                free(oldPeekActionLua);

            }

            free(unpackFrameLua);
        }
    }

    if (asprintf(&lua, 
"local facts_key = \"%s!f!\"\n"
"local events_key = \"%s!e!\"\n"
"local action_key = \"%s!a\"\n"
"local state_key = \"%s!s\"\n"
"local timers_key = \"%s!t\"\n"
"local context_directory = {}\n"
"local keys\n"
"local reviewers\n"
"local primary_frame_keys\n"
"local facts_hashset\n"
"local events_hashset\n"
"local events_message_cache = {}\n"
"local facts_message_cache = {}\n"
"local facts_mids_cache = {}\n"
"local events_mids_cache = {}\n"
"local get_context\n"
"local get_mids = function(index, frame, events_key, messages_key, mids_cache, message_cache)\n"
"    local event_mids = mids_cache[events_key]\n"
"    local primary_key = primary_frame_keys[index](frame)\n"
"    local new_mids = nil\n"
"    if not event_mids then\n"
"        event_mids = {}\n"
"        mids_cache[events_key] = event_mids\n"
"    else\n"
"        new_mids = event_mids[primary_key]\n"
"    end\n"
"    if not new_mids then\n"
"        new_mids = redis.call(\"lrange\", events_key .. \"!m!\" .. primary_key, 0, -1)\n"
"        event_mids[primary_key] = new_mids\n"
"    end\n"
"    return new_mids\n"
"end\n"
"local get_message = function(new_mid, messages_key, message_cache)\n"
"    local message = false\n"
"    new_mid = tostring(new_mid)\n"
"    if message_cache[new_mid] ~= nil then\n"
"        message = message_cache[new_mid]\n"
"    else\n"
"        local packed_message = redis.call(\"hget\", messages_key, new_mid)\n"
"        if packed_message then\n"
"            message = cmsgpack.unpack(packed_message)\n"
"        end\n"
"        message_cache[new_mid] = message\n"
"    end\n"
"    return message\n"
"end\n"
"local fetch_message = function(new_mid)\n"
"    if type(new_mid) == \"table\" then\n"
"        return new_mid\n"
"    end\n"
"    return get_message(new_mid, facts_hashset, facts_message_cache)\n"
"end\n"
"local validate_frame_for_key = function(packed_frame, frame, index, events_list_key, messages_key, mids_cache, message_cache, sid)\n"
"    local new_mids = get_mids(index, frame, events_list_key, messages_key, mids_cache, message_cache)\n"
"    for i = 1, #new_mids, 1 do\n"
"        local message = get_message(new_mids[i], messages_key, message_cache)\n"
"        if message and not reviewers[index](message, frame, index) then\n"
"            local frames_key = keys[index] .. \"!i!\" .. sid .. \"!\" .. new_mids[i]\n"
"            redis.call(\"rpush\", frames_key, packed_frame)\n"
"            return false\n"
"        end\n"
"    end\n"
"    return true\n"
"end\n"
"local validate_frame = function(packed_frame, frame, index, sid)\n"
"    local first_result = validate_frame_for_key(packed_frame, frame, index, keys[index] .. \"!e!\" .. sid, events_hashset, events_mids_cache, events_message_cache, sid)\n"
"    local second_result = validate_frame_for_key(packed_frame, frame, index, keys[index] ..\"!f!\" .. sid, facts_hashset, facts_mids_cache, facts_message_cache, sid)\n"
"    return first_result and second_result\n"
"end\n"
"local review_frame = function(frame, rule_action_key, sid, max_score)\n"
"    local indexes = {}\n"
"    local action_id = string.sub(rule_action_key, 1, (string.len(sid) + 1) * -1)\n"
"    local context = get_context(action_id)\n"
"    local full_frame = {}\n"
"    local cancel = false\n"
"    events_hashset = events_key .. sid\n"
"    facts_hashset = facts_key .. sid\n"
"    keys = context[\"keys\"]\n"
"    reviewers = context[\"reviewers\"]\n"
"    primary_frame_keys = context[\"primary_frame_keys\"]\n"
"    if not context[\"frame_restore\"](frame, full_frame) then\n"
"        cancel = true\n"
"    else\n"
"        for i = 1, #frame, 1 do\n"
"            if frame[i] == \"$n\" then\n"
"                if not validate_frame(frame, full_frame, i, sid) then\n"
"                    cancel = true\n"
"                    break\n"
"                end\n"
"            end\n"
"        end\n"
"    end\n"
"    if cancel then\n"
"        for i = 1, #frame, 1 do\n"
"            if type(frame[i]) == \"table\" then\n"
"                redis.call(\"hsetnx\", events_hashset, frame[i][\"id\"], cmsgpack.pack(frame[i]))\n"
"                redis.call(\"zadd\", timers_key, max_score, \"p:\" .. cjson.encode(frame[i]))\n"
"            end\n"
"        end\n"
"        full_frame = nil\n"
"    end\n"
"    return full_frame\n"
"end\n"
"local load_frame_from_rule = function(rule_action_key, raw_count, sid, max_score)\n"
"    local frames = {}\n"
"    local packed_frames = {}\n"
"    local unwrap = true\n"
"    local count = 1\n"
"    if raw_count ~= \"single\" then\n"
"        count = tonumber(raw_count)\n"
"        unwrap = false\n"
"    end\n"
"    if count == 0 then\n"
"        local packed_frame = redis.call(\"lpop\", rule_action_key)\n"
"        while packed_frame ~= \"0\" do\n"
"            local frame = review_frame(cmsgpack.unpack(packed_frame), rule_action_key, sid, max_score)\n"
"            if frame then\n"
"                table.insert(frames, frame)\n"
"                table.insert(packed_frames, packed_frame)\n"
"            end\n"
"            packed_frame = redis.call(\"lpop\", rule_action_key)\n"
"        end\n"
"        if #packed_frames > 0 then\n"
"            redis.call(\"lpush\", rule_action_key, 0)\n"
"        end\n"
"    else\n"
"        while count > 0 do\n"
"            local packed_frame = redis.call(\"rpop\", rule_action_key)\n"
"            if not packed_frame then\n"
"                break\n"
"            else\n"
"                local frame = review_frame(cmsgpack.unpack(packed_frame), rule_action_key, sid, max_score)\n"
"                if frame then\n"
"                    table.insert(frames, frame)\n"
"                    table.insert(packed_frames, packed_frame)\n"
"                    count = count - 1\n"
"                end\n"
"            end\n"
"        end\n"
"    end\n"
"    for i = #packed_frames, 1, -1 do\n"
"        redis.call(\"rpush\", rule_action_key, packed_frames[i])\n"
"    end\n"
"    if #packed_frames == 0 then\n"
"        return nil, nil\n"
"    end\n"
"    local last_name = string.find(rule_action_key, \"!\") - 1\n"
"    if unwrap then\n"
"        return string.sub(rule_action_key, 1, last_name), frames[1]\n"
"    else\n"
"        return string.sub(rule_action_key, 1, last_name), frames\n"
"    end\n"
"end\n"
"local load_frame_from_sid = function(sid, max_score)\n"
"    local action_list = action_key .. \"!\" .. sid\n"
"    local rule_action_key = redis.call(\"lpop\", action_list)\n"
"    if not rule_action_key then\n"
"        return nil, nil\n"
"    end\n"
"    local count = redis.call(\"lpop\", action_list)\n"
"    local name, frame = load_frame_from_rule(rule_action_key, count, sid, max_score)\n"
"    while not frame do\n"
"        rule_action_key = redis.call(\"lpop\", action_list)\n"
"        if not rule_action_key then\n"
"            return nil, nil\n"
"        end\n"
"        count = redis.call(\"lpop\", action_list)\n"
"        name, frame = load_frame_from_rule(rule_action_key, count, sid, max_score)\n"
"    end\n"
"    redis.call(\"lpush\", action_list, count)\n"
"    redis.call(\"lpush\", action_list, rule_action_key)\n"
"    return name, frame\n"
"end\n"
"local load_frame = function(max_score)\n"
"    local current_action = redis.call(\"zrange\", action_key, 0, 0, \"withscores\")\n"
"    if (#current_action == 0) or (tonumber(current_action[2]) > (max_score + 5)) then\n"
"        return nil, nil, nil\n"
"    end\n"
"    local sid = current_action[1]\n"
"    local name, frame = load_frame_from_sid(sid, max_score)\n"
"    while not frame do\n"
"        redis.call(\"zremrangebyrank\", action_key, 0, 0)\n"
"        current_action = redis.call(\"zrange\", action_key, 0, 0, \"withscores\")\n"
"        if #current_action == 0 or (tonumber(current_action[2]) > (max_score + 5)) then\n"
"            return nil, nil, nil\n"
"        end\n"
"        sid = current_action[1]\n"
"        name, frame = load_frame_from_sid(sid, max_score)\n"
"    end\n"
"    return sid, name, frame\n"
"end\n"
"get_context = function(action_key)\n"
"    if context_directory[action_key] then\n"
"        return context_directory[action_key]\n"
"    end\n"
"    local input_keys = {[action_key] = true}\n%s"
"end\n"
"local new_sid, action_name, frame\n"
"if #ARGV == 3 then\n"
"    new_sid = ARGV[3]\n"
"    action_name, frame = load_frame_from_sid(new_sid, tonumber(ARGV[2]))\n"
"else\n"
"    new_sid, action_name, frame = load_frame(tonumber(ARGV[2]))\n"
"end\n"
"if frame then\n"
"    redis.call(\"zadd\", action_key, tonumber(ARGV[1]), new_sid)\n"
"    if #ARGV == 2 then\n"
"        local state = redis.call(\"hget\", state_key, new_sid)\n"
"        return {new_sid, state, cjson.encode({[action_name] = frame})}\n"
"    else\n"
"        return {new_sid, cjson.encode({[action_name] = frame})}\n"
"    end\n"
"end\n",
                name,
                name,
                name,
                name,
                name,
                peekActionLua)  == -1) {
        return ERR_OUT_OF_MEMORY;
    }
    free(peekActionLua);
    redisAppendCommand(reContext, "SCRIPT LOAD %s", lua);
    redisGetReply(reContext, (void**)&reply);
    if (reply->type == REDIS_REPLY_ERROR) {
        printf("%s\n", reply->str);
        freeReplyObject(reply);
        free(lua);
        return ERR_REDIS_ERROR;
    }

    strncpy(rulesBinding->peekActionHash, reply->str, 40);
    rulesBinding->peekActionHash[40] = '\0';
    freeReplyObject(reply);
    free(lua);
    return RULES_OK;
}

static unsigned int loadEvalMessageCommand(ruleset *tree, binding *rulesBinding) {
    char *name = &tree->stringPool[tree->nameOffset];
    int nameLength = strlen(name);
    redisContext *reContext = rulesBinding->reContext;
    redisReply *reply;
    char *lua = NULL;
    char *oldLua;
    
    #ifdef _WIN32
    char *actionKey = (char *)_alloca(sizeof(char)*(nameLength + 3));
    sprintf_s(actionKey, nameLength + 3, "%s!a", name);
#else
    char actionKey[nameLength + 3];
    snprintf(actionKey, nameLength + 3, "%s!a", name);
#endif
    
    lua = (char*)calloc(1, sizeof(char));
    if (!lua) {
        return ERR_OUT_OF_MEMORY;
    }

    for (unsigned int i = 0; i < tree->nodeOffset; ++i) {
        node *currentNode = &tree->nodePool[i];
        
        if (currentNode->type == NODE_ACTION) {
            char *packFrameLua = NULL;
            char *unpackFrameLua = NULL;
            char *oldPackFrameLua = NULL;
            char *oldUnpackFrameLua = NULL;
            char *actionName = &tree->stringPool[currentNode->nameOffset];
            char *actionLastName = strchr(actionName, '!');
#ifdef _WIN32
            char *actionAlias = (char *)_alloca(sizeof(char)*(actionLastName - actionName + 1));
#else
            char actionAlias[actionLastName - actionName + 1];
#endif
            
            strncpy(actionAlias, actionName, actionLastName - actionName);
            actionAlias[actionLastName - actionName] = '\0';

            for (unsigned int ii = 0; ii < currentNode->value.c.joinsLength; ++ii) {
                unsigned int currentJoinOffset = tree->nextPool[currentNode->value.c.joinsOffset + ii];
                join *currentJoin = &tree->joinPool[currentJoinOffset];
                oldLua = lua;
                if (asprintf(&lua, 
"%stoggle = false\n"
"context = {}\n"
"reviewers = {}\n"
"context[\"reviewers\"] = reviewers\n"
"frame_packers = {}\n"
"context[\"frame_packers\"] = frame_packers\n"
"frame_unpackers = {}\n"
"context[\"frame_unpackers\"] = frame_unpackers\n"
"primary_message_keys = {}\n"
"context[\"primary_message_keys\"] = primary_message_keys\n"
"primary_frame_keys = {}\n"
"context[\"primary_frame_keys\"] = primary_frame_keys\n"
"keys = {}\n"
"context[\"keys\"] = keys\n"
"inverse_directory = {}\n"
"context[\"inverse_directory\"] = inverse_directory\n"
"directory = {[\"0\"] = 1}\n"
"context[\"directory\"] = directory\n"
"context[\"results_key\"] = \"%s!%d!r!\" .. sid\n"
"context[\"expressions_count\"] = %d\n",
                            lua,
                            actionName,
                            ii,
                            currentJoin->expressionsLength)  == -1) {
                    return ERR_OUT_OF_MEMORY;
                }  
                free(oldLua);

                for (unsigned int iii = 0; iii < currentJoin->expressionsLength; ++iii) {
                    unsigned int expressionOffset = tree->nextPool[currentJoin->expressionsOffset + iii];
                    expression *expr = &tree->expressionPool[expressionOffset];
                    char *currentAlias = &tree->stringPool[expr->aliasOffset];
                    char *currentKey = &tree->stringPool[expr->nameOffset];
                    char *nextKeyTest;
                    if (iii == (currentJoin->expressionsLength - 1)) {
                        nextKeyTest = (char*)calloc(1, sizeof(char));
                        if (!nextKeyTest) {
                            return ERR_OUT_OF_MEMORY;
                        }
                    } else {
                        unsigned int nextExpressionOffset = tree->nextPool[currentJoin->expressionsOffset + iii + 1];
                        expression *nextExpr = &tree->expressionPool[nextExpressionOffset];
                        if (asprintf(&nextKeyTest, 
"or input_keys[\"%s\"]", 
                                     &tree->stringPool[nextExpr->nameOffset])  == -1) {
                            return ERR_OUT_OF_MEMORY;
                        }
                    }
                    
                    if (iii == 0) {
                        if (expr->not) { 
                            if (asprintf(&packFrameLua,
"    result[1] = \"$n\"\n")  == -1) {
                                return ERR_OUT_OF_MEMORY;
                            }

                            if (asprintf(&unpackFrameLua,
"    result[\"%s\"] = \"$n\"\n",
                                        currentAlias)  == -1) {
                                return ERR_OUT_OF_MEMORY;
                            }

                            oldLua = lua;
                            if (asprintf(&lua, 
"%sif input_keys[\"%s\"] %s then\n"
"    toggle = true\n"
"    context_directory[\"%s\"] = context\n"
"    keys[1] = \"%s\"\n"
"    inverse_directory[1] = true\n"
"    directory[\"%s\"] = 1\n"
"    reviewers[1] = function(message, frame, index)\n"
"        if not message then\n"
"            frame[\"%s\"] = \"$n\"\n"
"            return true\n"
"        end\n"
"        return false\n"
"    end\n"
"    frame_packers[1] = function(frame, full_encode)\n"
"        local result = {}\n%s"
"        return cmsgpack.pack(result)\n"
"    end\n"
"    frame_unpackers[1] = function(packed_frame)\n"
"        local frame = cmsgpack.unpack(packed_frame)\n"
"        local result = {}\n%s"
"        return result\n"
"    end\n"
"    primary_message_keys[1] = function(message)\n"
"        return \"\"\n"
"    end\n"
"    primary_frame_keys[1] = function(frame)\n"
"        return \"\"\n"
"    end\n"
"end\n",
                                         lua,
                                         currentKey,
                                         nextKeyTest,
                                         currentKey,
                                         currentKey,
                                         currentKey,
                                         currentAlias,
                                         packFrameLua,
                                         unpackFrameLua)  == -1) {
                                return ERR_OUT_OF_MEMORY;
                            }
                            free(oldLua);
                        // not (expr->not)
                        } else {
                            if (asprintf(&packFrameLua,
"    message = frame[\"%s\"]\n"
"    if full_encode and not message[\"$f\"] then\n"
"        result[1] = message\n"
"    else\n"
"        result[1] = message[\"id\"]\n"
"    end\n",
                                        currentAlias)  == -1) {
                                return ERR_OUT_OF_MEMORY;
                            }

                            if (asprintf(&unpackFrameLua,
"    message = fetch_message(frame[1])\n"
"    if not message then\n"
"        return nil\n"
"    end\n"
"    result[\"%s\"] = message\n",
                                        currentAlias)  == -1) {
                                return ERR_OUT_OF_MEMORY;
                            }

                            oldLua = lua;
                            if (asprintf(&lua, 
"%sif input_keys[\"%s\"] %s then\n"
"    toggle = true\n"
"    context_directory[\"%s\"] = context\n"
"    keys[1] = \"%s\"\n"
"    inverse_directory[1] = true\n"
"    directory[\"%s\"] = 1\n"
"    reviewers[1] = function(message, frame, index)\n"
"        if message then\n"
"            frame[\"%s\"] = message\n"
"            return true\n"
"        end\n"
"        return false\n"
"    end\n"
"    frame_packers[1] = function(frame, full_encode)\n"
"        local result = {}\n"
"        local message\n%s"
"        return cmsgpack.pack(result)\n"
"    end\n"
"    frame_unpackers[1] = function(packed_frame)\n"
"        local frame = cmsgpack.unpack(packed_frame)\n"
"        local result = {}\n"
"        local message\n%s"
"        return result\n"
"    end\n"
"    primary_message_keys[1] = function(message)\n"
"        return \"\"\n"
"    end\n"
"    primary_frame_keys[1] = function(frame)\n"
"        return \"\"\n"
"    end\n"
"end\n",
                                         lua,
                                         currentKey,
                                         nextKeyTest,
                                         currentKey,
                                         currentKey,
                                         currentKey,
                                         currentAlias,
                                         packFrameLua,
                                         unpackFrameLua)  == -1) {
                                return ERR_OUT_OF_MEMORY;
                            }
                            free(oldLua);
                        }
                    // not (iii == 0)
                    } else {
                        char *test = NULL;
                        char *primaryKeyLua = NULL;
                        char *primaryFrameKeyLua = NULL;
                        unsigned int result = createTest(tree, expr, &test, &primaryKeyLua, &primaryFrameKeyLua);
                        if (result != RULES_OK) {
                            return result;
                        }

                        if (expr->not) { 
                            oldPackFrameLua = packFrameLua;
                            if (asprintf(&packFrameLua,
"%s    result[%d] = \"$n\"\n",
                                         packFrameLua,
                                         iii + 1)  == -1) {
                                return ERR_OUT_OF_MEMORY;
                            }
                            free(oldPackFrameLua);

                            oldUnpackFrameLua = unpackFrameLua;
                            if (asprintf(&unpackFrameLua,
"%s    result[\"%s\"] = \"$n\"\n",
                                        unpackFrameLua,
                                        currentAlias)  == -1) {
                                return ERR_OUT_OF_MEMORY;
                            }
                            free(oldUnpackFrameLua);

                            oldLua = lua;
                            if (asprintf(&lua,
"%sif toggle %s then\n"
"    toggle = true\n"
"    context_directory[\"%s\"] = context\n"
"    keys[%d] = \"%s\"\n"
"    inverse_directory[%d] = true\n"
"    directory[\"%s\"] = %d\n"
"    reviewers[%d] = function(message, frame, index)\n"
"        if not message or not (%s) then\n"
"            frame[\"%s\"] = \"$n\"\n"
"            return true\n"
"        end\n"
"        return false\n"
"    end\n"
"    frame_packers[%d] = function(frame, full_encode)\n"
"        local result = {}\n"
"        local message\n%s"
"        return cmsgpack.pack(result)\n"
"    end\n"
"    frame_unpackers[%d] = function(packed_frame)\n"
"        local frame = cmsgpack.unpack(packed_frame)\n"
"        local result = {}\n"
"        local message\n%s"
"        return result\n"
"    end\n"
"    primary_message_keys[%d] = function(message)\n"
"        local result = \"\"\n%s"
"        return result\n"
"    end\n"
"    primary_frame_keys[%d] = function(frame)\n"
"        local result = \"\"\n%s"
"        return result\n"
"    end\n"
"end\n",
                                         lua,
                                         nextKeyTest,
                                         currentKey,
                                         iii + 1, 
                                         currentKey,
                                         iii + 1, 
                                         currentKey,
                                         iii + 1,
                                         iii + 1,
                                         test,
                                         currentAlias,
                                         iii + 1,
                                         packFrameLua,
                                         iii + 1,
                                         unpackFrameLua,
                                         iii + 1,
                                         primaryKeyLua,
                                         iii + 1,
                                         primaryFrameKeyLua)  == -1) {
                                return ERR_OUT_OF_MEMORY;
                            }
                            free(oldLua);

                        // not (expr->not)
                        } else {
                            oldPackFrameLua = packFrameLua;
                            if (asprintf(&packFrameLua,
"%s    message = frame[\"%s\"]\n"
"    if full_encode and not message[\"$f\"] then\n"
"        result[%d] = message\n"
"    else\n"
"        result[%d] = message[\"id\"]\n"
"    end\n",
                                         packFrameLua,
                                         currentAlias,
                                         iii + 1,
                                         iii + 1)  == -1) {
                                return ERR_OUT_OF_MEMORY;
                            }
                            free(oldPackFrameLua);

                            oldUnpackFrameLua = unpackFrameLua;
                            if (asprintf(&unpackFrameLua,
"%s    message = fetch_message(frame[%d])\n"
"    if not message then\n"
"        return nil\n"
"    end\n"
"    result[\"%s\"] = message\n",
                                         unpackFrameLua,
                                         iii + 1,
                                         currentAlias)  == -1) {
                                return ERR_OUT_OF_MEMORY;
                            }
                            free(oldUnpackFrameLua);

                            oldLua = lua;
                            if (asprintf(&lua,
"%sif toggle %s then\n"
"    toggle = true\n"
"    context_directory[\"%s\"] = context\n"
"    keys[%d] = \"%s\"\n"
"    directory[\"%s\"] = %d\n"
"    reviewers[%d] = function(message, frame, index)\n"
"        if message and %s then\n"
"            frame[\"%s\"] = message\n"
"            return true\n"
"        end\n"
"        return false\n"
"    end\n"
"    frame_packers[%d] = function(frame, full_encode)\n"
"        local result = {}\n"
"        local message\n%s"
"        return cmsgpack.pack(result)\n"
"    end\n"
"    frame_unpackers[%d] = function(packed_frame)\n"
"        local frame = cmsgpack.unpack(packed_frame)\n"
"        local result = {}\n"
"        local message\n%s"
"        return result\n"
"    end\n"
"    primary_message_keys[%d] = function(message)\n"
"        local result = \"\"\n%s"
"        return result\n"
"    end\n"
"    primary_frame_keys[%d] = function(frame)\n"
"        local result = \"\"\n%s"
"        return result\n"
"    end\n"
"end\n",
                                         lua,
                                         nextKeyTest,
                                         currentKey,
                                         iii + 1, 
                                         currentKey,
                                         currentKey,
                                         iii + 1,
                                         iii + 1,
                                         test,
                                         currentAlias,
                                         iii + 1,
                                         packFrameLua,
                                         iii + 1,
                                         unpackFrameLua,
                                         iii + 1,
                                         primaryKeyLua,
                                         iii + 1,
                                         primaryFrameKeyLua)  == -1) {
                                return ERR_OUT_OF_MEMORY;
                            }
                            free(oldLua);
                        // done not (expr->not)
                        }
                        free(nextKeyTest);
                        free(test);
                        free(primaryKeyLua);
                        free(primaryFrameKeyLua);
                    // done not (iii == 0)
                    }
                }

                if (currentNode->value.c.span > 0) {
                    oldLua = lua;
                    if (asprintf(&lua,
"%sif toggle then\n"
"    context[\"process_key\"] = process_key_with_span\n"
"    context[\"process_key_count\"] = %d\n"
"end\n",
                                 lua,
                                 currentNode->value.c.span)  == -1) {
                        return ERR_OUT_OF_MEMORY;
                    }
                    free(oldLua);

                } else if (currentNode->value.c.cap > 0) {
                    oldLua = lua;
                    if (asprintf(&lua,
"%sif toggle then\n"
"    context[\"process_key\"] = process_key_with_cap\n"
"    context[\"process_key_count\"] = %d\n"
"end\n",
                                 lua,
                                 currentNode->value.c.cap)  == -1) {
                        return ERR_OUT_OF_MEMORY;
                    }
                    free(oldLua);

                } else {
                    oldLua = lua;
                    if (asprintf(&lua,
"%sif toggle then\n"
"    context[\"process_key\"] = process_key_with_window\n"
"    context[\"process_key_count\"] = %d\n"
"end\n",
                                 lua,
                                 currentNode->value.c.count)  == -1) {
                        return ERR_OUT_OF_MEMORY;
                    }
                    free(oldLua);
                }
            }

            free(unpackFrameLua);
            free(packFrameLua);
        }
    }

    oldLua = lua;
    if (asprintf(&lua,
"local sid = ARGV[1]\n"
"local mid = ARGV[2]\n"
"local score = tonumber(ARGV[3])\n"
"local assert_fact = tonumber(ARGV[4])\n"
"local keys_count = tonumber(ARGV[5])\n"
"local events_hashset = \"%s!e!\" .. sid\n"
"local facts_hashset = \"%s!f!\" .. sid\n"
"local visited_hashset = \"%s!v!\" .. sid\n"
"local actions_key = \"%s!a\"\n"
"local state_key = \"%s!s\"\n"
"local queue_action = false\n"
"local facts_message_cache = {}\n"
"local events_message_cache = {}\n"
"local facts_mids_cache = {}\n"
"local events_mids_cache = {}\n"
"local context_directory = {}\n"
"local input_keys = {}\n"
"local toggle\n"
"local expressions_count\n"
"local results\n"
"local unpacked_results\n"
"local context\n"
"local keys\n"
"local reviewers\n"
"local frame_packers\n"
"local frame_unpackers\n"
"local primary_message_keys\n"
"local primary_frame_keys\n"
"local directory\n"
"local results_key\n"
"local inverse_directory\n"
"local key\n"
"local cleanup_mids = function(index, frame, events_key, messages_key, mids_cache, message_cache)\n"
"    local event_mids = mids_cache[events_key]\n"
"    local primary_key = primary_frame_keys[index](frame)\n"
"    local new_mids = event_mids[primary_key]\n"
"    local result_mids = {}\n"
"    for i = 1, #new_mids, 1 do\n"
"        local new_mid = new_mids[i]\n"
"        if message_cache[new_mid] ~= false then\n"
"            table.insert(result_mids, new_mid)\n"
"        end\n"
"    end\n"
"    event_mids[primary_key] = result_mids\n"
"    redis.call(\"del\", events_key .. \"!m!\" .. primary_key)\n"
"    for i = 1, #result_mids, 1 do\n"
"        redis.call(\"rpush\", events_key .. \"!m!\" .. primary_key, result_mids[i])\n"
"    end\n"
"end\n"
"local get_mids = function(index, frame, events_key, messages_key, mids_cache, message_cache)\n"
"    local event_mids = mids_cache[events_key]\n"
"    local primary_key = primary_frame_keys[index](frame)\n"
"    local new_mids = nil\n"
"    if not event_mids then\n"
"        event_mids = {}\n"
"        mids_cache[events_key] = event_mids\n"
"    else\n"
"        new_mids = event_mids[primary_key]\n"
"    end\n"
"    if not new_mids then\n"
"        new_mids = redis.call(\"lrange\", events_key .. \"!m!\" .. primary_key, 0, -1)\n"
"        event_mids[primary_key] = new_mids\n"
"    end\n"
"    return new_mids\n"
"end\n"
"local get_message = function(new_mid, messages_key, message_cache)\n"
"    local message = false\n"
"    new_mid = tostring(new_mid)\n"
"    if message_cache[new_mid] ~= nil then\n"
"        message = message_cache[new_mid]\n"
"    else\n"
"        local packed_message = redis.call(\"hget\", messages_key, new_mid)\n"
"        if packed_message then\n"
"            message = cmsgpack.unpack(packed_message)\n"
"        end\n"
"        message_cache[new_mid] = message\n"
"    end\n"
"    return message\n"
"end\n"
"local fetch_message = function(new_mid)\n"
"    local message = get_message(new_mid, events_hashset, events_message_cache)\n"
"    if not message then\n"
"        message = get_message(new_mid, facts_hashset, facts_message_cache)\n"
"    end\n"
"    return message\n"
"end\n"
"local save_message = function(index, message, events_key, messages_key)\n"
"    redis.call(\"hsetnx\", messages_key, message[\"id\"], cmsgpack.pack(message))\n"
"    local primary_key = primary_message_keys[index](message)\n"
"    redis.call(\"lpush\", events_key .. \"!m!\" .. primary_key, message[\"id\"])\n"
"end\n"
"local save_result = function(frame, index)\n"
"    table.insert(results, 1, frame_packers[index](frame, true))\n"
"    table.insert(unpacked_results, 1, frame)\n"
"    for name, message in pairs(frame) do\n"
"        if message ~= \"$n\" and not message[\"$f\"] then\n"
"            redis.call(\"hdel\", events_hashset, message[\"id\"])\n"
// message cache always looked up using strings
"            events_message_cache[tostring(message[\"id\"])] = false\n"
"        end\n"
"    end\n"
"end\n"
"local is_pure_fact = function(frame, index)\n"
"    local message_count = 0\n"
"    for name, message in pairs(frame) do\n"
"        if message ~= 1 and message[\"$f\"] ~= 1 then\n" 
"           return false\n"    
"        end\n"
"        message_count = message_count + 1\n"   
"    end\n"    
"    return (message_count == index - 1)\n"
"end\n"
"local process_frame\n"
"local process_event_and_frame = function(message, frame, index, use_facts)\n"
"    local result = 0\n"
"    local new_frame = {}\n"
"    for name, new_message in pairs(frame) do\n"
"        new_frame[name] = new_message\n"
"    end\n"
"    if reviewers[index](message, new_frame, index) then\n"
"        if (index == expressions_count) then\n"
"            save_result(new_frame, index)\n"
"            return 1\n"
"        else\n"
"            result = process_frame(new_frame, index + 1, use_facts)\n"
"            if result == 0 or use_facts then\n"
"                local frames_key\n"
"                local primary_key = primary_frame_keys[index + 1](new_frame)\n"
"                frames_key = keys[index + 1] .. \"!c!\" .. sid .. \"!\" .. primary_key\n"
"                redis.call(\"rpush\", frames_key, frame_packers[index](new_frame))\n"
"            end\n"
"        end\n"
"    end\n"
"    return result\n"
"end\n"
"local process_frame_for_key = function(frame, index, events_key, use_facts)\n"
"    local result = nil\n"
"    local inverse = inverse_directory[index]\n"
"    local messages_key = events_hashset\n"
"    local message_cache = events_message_cache\n"
"    local mids_cache = events_mids_cache\n"
"    local cleanup = false\n"
"    if use_facts then\n"
"       messages_key = facts_hashset\n"
"       message_cache = facts_message_cache\n"
"       mids_cache = facts_mids_cache\n"
"    end\n"
"    if inverse then\n"
"        local new_frame = {}\n"
"        for name, new_message in pairs(frame) do\n"
"            new_frame[name] = new_message\n"
"        end\n"
"        local new_mids = get_mids(index, frame, events_key, messages_key, mids_cache, message_cache)\n"
"        for i = 1, #new_mids, 1 do\n"
"            local message = get_message(new_mids[i], messages_key, message_cache)\n"
"            if not message then\n"
"                cleanup = true\n"
"            elseif not reviewers[index](message, new_frame, index) then\n"
"                local frames_key = keys[index] .. \"!i!\" .. sid .. \"!\" .. new_mids[i]\n"
"                redis.call(\"rpush\", frames_key, frame_packers[index - 1](new_frame))\n"
"                result = 0\n"
"                break\n"
"            end\n"
"        end\n"
"    else\n"
"        local new_mids = get_mids(index, frame, events_key, messages_key, mids_cache, message_cache)\n"
"        for i = 1, #new_mids, 1 do\n"
"            local message = get_message(new_mids[i], messages_key, message_cache)\n"
"            if not message then\n"
"                cleanup = true\n"
"            else\n"
"                local count = process_event_and_frame(message, frame, index, use_facts)\n"
"                if not result then\n"
"                    result = 0\n"
"                end\n"
"                result = result + count\n"
"                if not is_pure_fact(frame, index) then\n"
// the mid list might not be cleaned up if the first mid is always valid.
"                    if (#new_mids %% 10) == 0 then\n"
"                        cleanup = true\n"
"                    end\n"
"                    break\n"
"                end\n"
"            end\n"
"        end\n"
"    end\n"
"    if cleanup then\n"
"        cleanup_mids(index, frame, events_key, messages_key, mids_cache, message_cache)\n"
"    end\n"
"    return result\n"
"end\n"
"process_frame = function(frame, index, use_facts)\n"
"    local first_result = process_frame_for_key(frame, index, keys[index] .. \"!e!\" .. sid, false)\n"
"    local second_result = process_frame_for_key(frame, index, keys[index] .. \"!f!\" .. sid, true)\n"
"    if not first_result and not second_result then\n"
"        return process_event_and_frame(nil, frame, index, use_facts)\n"
"    elseif not first_result then\n"
"        return second_result\n"
"    elseif not second_result then\n"
"        return first_result\n"
"    else\n"
"        return first_result + second_result\n"
"    end\n"
"end\n"
"local process_inverse_event = function(message, index, events_key, use_facts)\n"
"    local result = 0\n"
"    local messages_key = events_hashset\n"
"    if use_facts then\n"
"        messages_key = facts_hashset\n"
"    end\n"
"    redis.call(\"hdel\", messages_key, mid)\n"
"    if index == 1 then\n"
"        result = process_frame({}, 1, use_facts)\n"
"    else\n"
"        local frames_key = keys[index] .. \"!i!\" .. sid .. \"!\" .. mid\n"
"        local packed_frames_len = redis.call(\"llen\", frames_key)\n"
"        for i = 1, packed_frames_len, 1 do\n"
"            local packed_frame = redis.call(\"rpop\", frames_key)\n"
"            local frame = frame_unpackers[index - 1](packed_frame)\n"
"            if frame then\n"
"                result = result + process_frame(frame, index, use_facts)\n"
"            end\n"     
"        end\n"
"    end\n"
"    return result\n"
"end\n"
"local process_event = function(message, index, events_key, use_facts)\n"
"    local result = 0\n"
"    local messages_key = events_hashset\n"
"    if use_facts then\n"
"        messages_key = facts_hashset\n"
"    end\n"
"    if index == 1 then\n"
"        result = process_event_and_frame(message, {}, 1, use_facts)\n"
"    else\n"
"        local frames_key\n"
"        local primary_key = primary_message_keys[index](message)\n"
"        if primary_key then\n"
"            frames_key = keys[index] .. \"!c!\" .. sid .. \"!\" .. primary_key\n"
"        else\n"
"            frames_key = keys[index] .. \"!c!\" .. sid\n"
"        end\n"
"        local packed_frames_len = redis.call(\"llen\", frames_key)\n"
"        for i = 1, packed_frames_len, 1 do\n"
"            local packed_frame = redis.call(\"rpop\", frames_key)\n"
"            local frame = frame_unpackers[index - 1](packed_frame)\n"
"            if frame then\n"
"                local count = process_event_and_frame(message, frame, index, use_facts)\n"
"                result = result + count\n"         
"                if count == 0 or use_facts then\n"
"                    redis.call(\"lpush\", frames_key, packed_frame)\n"
"                else\n"
"                    break\n" 
"                end\n"
"            end\n"     
"        end\n"
"    end\n"
"    if result == 0 or use_facts then\n"
"        save_message(index, message, events_key, messages_key)\n"
"    end\n"
"    return result\n"
"end\n"
"local process_key_with_span = function(message, span)\n"
"    local index = directory[key]\n"
"    local queue_lock = false\n"
"    if index then\n"
"        local last_score = redis.call(\"get\", results_key .. \"!d\")\n"
"        if not last_score then\n"
"            redis.call(\"set\", results_key .. \"!d\", score)\n"
"        else\n"
"            local new_score = last_score + span\n"
"            if score > new_score then\n"
"                redis.call(\"rpush\", results_key, 0)\n"
"                redis.call(\"rpush\", actions_key .. \"!\" .. sid, results_key)\n"
"                redis.call(\"rpush\", actions_key .. \"!\" .. sid, 0)\n"
"                local span_count, span_remain = math.modf((score - new_score) / span)\n"
"                last_score = new_score + span_count * span\n"
"                redis.call(\"set\", results_key .. \"!d\", last_score)\n"
"                queue_lock = true\n"
"            end\n"    
"        end\n"
"        local count = 0\n"
"        if not message then\n"
"            if assert_fact == 0 then\n"
"                count = process_inverse_event(message, index, keys[index] .. \"!e!\" .. sid, false)\n"
"            else\n"
"                count = process_inverse_event(message, index, keys[index] .. \"!f!\" .. sid, true)\n"
"            end\n"
"        else\n"
"            if assert_fact == 0 then\n"
"                count = process_event(message, index, keys[index] .. \"!e!\" .. sid, false)\n"
"            else\n"
"                count = process_event(message, index, keys[index] .. \"!f!\" .. sid, true)\n"
"            end\n"
"        end\n"
"        if (count > 0) then\n"
"            for i = 1, #results, 1 do\n"
"                redis.call(\"rpush\", results_key, results[i])\n"
"            end\n"
"        end\n"
"    end\n"
"    return queue_lock\n"
"end\n"
"local process_key_with_cap = function(message, cap)\n"
"    local index = directory[key]\n"
"    local queue_lock = false\n"
"    if index then\n"
"        local count = 0\n"
"        if not message then\n"
"            if assert_fact == 0 then\n"
"                count = process_inverse_event(message, index, keys[index] .. \"!e!\" .. sid, false)\n"
"            else\n"
"                count = process_inverse_event(message, index, keys[index] .. \"!f!\" .. sid, true)\n"
"            end\n"
"        else\n"
"            if assert_fact == 0 then\n"
"                count = process_event(message, index, keys[index] .. \"!e!\" .. sid, false)\n"
"            else\n"
"                count = process_event(message, index, keys[index] .. \"!f!\" .. sid, true)\n"
"            end\n"
"        end\n"
"        if (count > 0) then\n"
"            for i = #results, 1, -1 do\n"
"                redis.call(\"lpush\", results_key, results[i])\n"
"            end\n"
"            local diff\n"
"            local new_count, new_remain = math.modf(#results / cap)\n"
"            local new_remain = #results %% cap\n"
"            if new_count > 0 then\n"
"                for i = 1, new_count, 1 do\n"
"                    redis.call(\"rpush\", actions_key .. \"!\" .. sid, results_key)\n"
"                    redis.call(\"rpush\", actions_key .. \"!\" .. sid, cap)\n"
"                end\n"
"            end\n"
"            if new_remain > 0 then\n"
"                redis.call(\"rpush\", actions_key .. \"!\" .. sid, results_key)\n"
"                redis.call(\"rpush\", actions_key .. \"!\" .. sid, new_remain)\n"
"            end\n"
"            if new_count > 0 or new_remain > 0 then\n"
"                queue_lock = true\n"
"            end\n"
"        end\n"
"    end\n"
"    return queue_lock\n"
"end\n"
"local process_key_with_window = function(message, window)\n"
"    local index = directory[key]\n"
"    local queue_lock = false\n"
"    if index then\n"
"        local count = 0\n"
"        if not message then\n"
"            if assert_fact == 0 then\n"
"                count = process_inverse_event(message, index, keys[index] .. \"!e!\" .. sid, false)\n"
"            else\n"
"                count = process_inverse_event(message, index, keys[index] .. \"!f!\" .. sid, true)\n"
"            end\n"
"        else\n"
"            if assert_fact == 0 then\n"
"                count = process_event(message, index, keys[index] .. \"!e!\" .. sid, false)\n"
"            else\n"
"                count = process_event(message, index, keys[index] .. \"!f!\" .. sid, true)\n"
"            end\n"
"        end\n"
"        if (count > 0) then\n"
"            for i = #results, 1, -1 do\n"
"                redis.call(\"lpush\", results_key, results[i])\n"
"            end\n"
"            local diff\n"
"            local length = redis.call(\"llen\", results_key)\n"
"            local prev_count, prev_remain = math.modf((length - count) / window)\n"
"            local new_count, prev_remain = math.modf(length / window)\n"
"            diff = new_count - prev_count\n"
"            if diff > 0 then\n"
"                for i = 0, diff - 1, 1 do\n"
"                    redis.call(\"rpush\", actions_key .. \"!\" .. sid, results_key)\n"
"                    if window == 1 then\n"
"                        redis.call(\"rpush\", actions_key .. \"!\" .. sid, \"single\")\n"
"                    else\n"
"                        redis.call(\"rpush\", actions_key .. \"!\" .. sid, window)\n"
"                    end\n"
"                end\n"
"                queue_lock = true\n"
"             end\n"
"        end\n"
"    end\n"
"    return queue_lock\n"
"end\n"
"local message = nil\n"
"if #ARGV > (6 + keys_count) then\n"
"    message = {}\n"
"    for index = 6 + keys_count, #ARGV, 3 do\n"
"        if ARGV[index + 2] == \"1\" then\n"
"            message[ARGV[index]] = ARGV[index + 1]\n"
"        elseif ARGV[index + 2] == \"2\" or  ARGV[index + 2] == \"3\" then\n"
"            message[ARGV[index]] = tonumber(ARGV[index + 1])\n"
"        elseif ARGV[index + 2] == \"4\" then\n"
"            if ARGV[index + 1] == \"true\" then\n"
"                message[ARGV[index]] = true\n"
"            else\n"
"                message[ARGV[index]] = false\n"
"            end\n"
"        end\n"
"    end\n"
"    if assert_fact == 1 then\n"
"        message[\"$f\"] = 1\n"
"    end\n"
"end\n"
"if redis.call(\"hsetnx\", visited_hashset, mid, 1) == 0 then\n"
"    if assert_fact == 0 then\n"
"        if message and redis.call(\"hexists\", events_hashset, mid) == 0 then\n"
"            return false\n"
"        end\n"
"    else\n"
"        if message and redis.call(\"hexists\", facts_hashset, mid) == 0 then\n"
"            return false\n"
"        end\n"
"    end\n"
"end\n"
"for index = 6, 5 + keys_count, 1 do\n"
"    input_keys[ARGV[index]] = true\n"
"end\n"
"%sfor index = 6, 5 + keys_count, 1 do\n"
"    results = {}\n"
"    unpacked_results = {}\n"
"    key = ARGV[index]\n"
"    context = context_directory[key]\n"
"    keys = context[\"keys\"]\n"
"    reviewers = context[\"reviewers\"]\n"
"    frame_packers = context[\"frame_packers\"]\n"
"    frame_unpackers = context[\"frame_unpackers\"]\n"
"    primary_message_keys = context[\"primary_message_keys\"]\n"
"    primary_frame_keys = context[\"primary_frame_keys\"]\n"
"    directory = context[\"directory\"]\n"
"    results_key = context[\"results_key\"]\n"
"    inverse_directory = context[\"inverse_directory\"]\n"
"    expressions_count = context[\"expressions_count\"]\n"
"    local process_key = context[\"process_key\"]\n"
"    local process_key_count = context[\"process_key_count\"]\n"
"    queue_action = process_key(message, process_key_count)\n"
"    if assert_fact == 0 and events_message_cache[tostring(message[\"id\"])] == false then\n"
"        break\n"
"    end\n"
"end\n"
"if queue_action then\n"
"    if not redis.call(\"zscore\", actions_key, sid) then\n"
"        redis.call(\"zadd\", actions_key , score, sid)\n"
"    end\n"
"end\n"
"return nil\n",
                 name,
                 name,
                 name,
                 name,
                 name,
                 lua)  == -1) {
        return ERR_OUT_OF_MEMORY;
    }

    free(oldLua);
    redisAppendCommand(reContext, "SCRIPT LOAD %s", lua);
    redisGetReply(reContext, (void**)&reply);
    if (reply->type == REDIS_REPLY_ERROR) {
        printf("%s\n", reply->str);
        freeReplyObject(reply);
        free(lua);
        return ERR_REDIS_ERROR;
    }

    strncpy(rulesBinding->evalMessageHash, reply->str, 40);
    rulesBinding->evalMessageHash[40] = '\0';
    freeReplyObject(reply);
    free(lua);
    return RULES_OK;
}

static unsigned int setNames(ruleset *tree, binding *rulesBinding) {
    char *name = &tree->stringPool[tree->nameOffset];
    int nameLength = strlen(name);
    char *sessionHashset = malloc((nameLength + 3) * sizeof(char));
    if (!sessionHashset) {
        return ERR_OUT_OF_MEMORY;
    }

    strncpy(sessionHashset, name, nameLength);
    sessionHashset[nameLength] = '!';
    sessionHashset[nameLength + 1] = 's';
    sessionHashset[nameLength + 2] = '\0';
    rulesBinding->sessionHashset = sessionHashset;

    char *factsHashset = malloc((nameLength + 3) * sizeof(char));
    if (!factsHashset) {
        return ERR_OUT_OF_MEMORY;
    }

    strncpy(factsHashset, name, nameLength);
    factsHashset[nameLength] = '!';
    factsHashset[nameLength + 1] = 'f';
    factsHashset[nameLength + 2] = '\0';
    rulesBinding->factsHashset = factsHashset;

    char *eventsHashset = malloc((nameLength + 3) * sizeof(char));
    if (!eventsHashset) {
        return ERR_OUT_OF_MEMORY;
    }

    strncpy(eventsHashset, name, nameLength);
    eventsHashset[nameLength] = '!';
    eventsHashset[nameLength + 1] = 'e';
    eventsHashset[nameLength + 2] = '\0';
    rulesBinding->eventsHashset = eventsHashset;

    char *timersSortedset = malloc((nameLength + 3) * sizeof(char));
    if (!timersSortedset) {
        return ERR_OUT_OF_MEMORY;
    }

    strncpy(timersSortedset, name, nameLength);
    timersSortedset[nameLength] = '!';
    timersSortedset[nameLength + 1] = 't';
    timersSortedset[nameLength + 2] = '\0';
    rulesBinding->timersSortedset = timersSortedset;
    return RULES_OK;
}

static unsigned int loadCommands(ruleset *tree, binding *rulesBinding) {
    unsigned int result = loadPartitionCommand(tree, rulesBinding);
    if (result != RULES_OK) {
        return result;
    }

    // client queues have no commands to load, 
    if (!tree->stringPool) {
        return RULES_OK;
    }

    result = loadTimerCommand(tree, rulesBinding);
    if (result != RULES_OK) {
        return result;
    }

    result = loadEvalMessageCommand(tree, rulesBinding);
    if (result != RULES_OK) {
        return result;
    }

    result = loadAddMessageCommand(tree, rulesBinding);
    if (result != RULES_OK) {
        return result;
    }    

    result = loadPeekActionCommand(tree, rulesBinding);
    if (result != RULES_OK) {
        return result;
    }

    result = loadUpdateActionCommand(tree, rulesBinding);
    if (result != RULES_OK) {
        return result;
    }

    result = loadRemoveActionCommand(tree, rulesBinding);
    if (result != RULES_OK) {
        return result;
    }

    result = loadDeleteSessionCommand(tree, rulesBinding);
    if (result != RULES_OK) {
        return result;
    }

    result = setNames(tree, rulesBinding);
    if (result != RULES_OK) {
        return result;
    }

    return RULES_OK;
}

unsigned int bindRuleset(void *handle, 
                         char *host, 
                         unsigned int port, 
                         char *password) {
    ruleset *tree = (ruleset*)handle;
    bindingsList *list;
    if (tree->bindingsList) {
        list = tree->bindingsList;
    }
    else {
        list = malloc(sizeof(bindingsList));
        if (!list) {
            return ERR_OUT_OF_MEMORY;
        }

        list->bindings = NULL;
        list->bindingsLength = 0;
        list->lastBinding = 0;
        list->lastTimersBinding = 0;
        tree->bindingsList = list;
    }

    redisContext *reContext;
    if (port == 0) {
        reContext = redisConnectUnix(host);
    } else {
        reContext = redisConnect(host, port);
    }
    
    if (reContext->err) {
        redisFree(reContext);
        return ERR_CONNECT_REDIS;
    }

    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    int result = redisSetTimeout(reContext, tv);
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }

    if (password != NULL) {
        result = redisAppendCommand(reContext, "auth %s", password);
        if (result != REDIS_OK) {
            return ERR_REDIS_ERROR;
        }

        redisReply *reply;
        result = redisGetReply(reContext, (void**)&reply);
        if (result != REDIS_OK) {
            return ERR_REDIS_ERROR;
        }
        
        if (reply->type == REDIS_REPLY_ERROR) {
            freeReplyObject(reply);   
            return ERR_REDIS_ERROR;
        }

        freeReplyObject(reply);
    }

    if (!list->bindings) {
        list->bindings = malloc(sizeof(binding));
    }
    else {
        list->bindings = realloc(list->bindings, sizeof(binding) * (list->bindingsLength + 1));
    }

    if (!list->bindings) {
        redisFree(reContext);
        return ERR_OUT_OF_MEMORY;
    }
    list->bindings[list->bindingsLength].reContext = reContext;
    ++list->bindingsLength;
    return loadCommands(tree, &list->bindings[list->bindingsLength -1]);
}

unsigned int deleteBindingsList(ruleset *tree) {
    bindingsList *list = tree->bindingsList;
    if (tree->bindingsList != NULL) {
        for (unsigned int i = 0; i < list->bindingsLength; ++i) {
            binding *currentBinding = &list->bindings[i];
            redisFree(currentBinding->reContext);
            free(currentBinding->timersSortedset);
            free(currentBinding->sessionHashset);
            free(currentBinding->factsHashset);
            free(currentBinding->eventsHashset);
        }

        free(list->bindings);
        free(list);
    }
    return RULES_OK;
}

unsigned int getBindingIndex(ruleset *tree, unsigned int sidHash, unsigned int *bindingIndex) {
    bindingsList *list = tree->bindingsList;
    binding *firstBinding = &list->bindings[0];
    redisContext *reContext = firstBinding->reContext;

    int result = redisAppendCommand(reContext, 
                                    "evalsha %s 0 %d %d", 
                                    firstBinding->partitionHash, 
                                    sidHash, 
                                    list->bindingsLength);
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }

    redisReply *reply;
    result = redisGetReply(reContext, (void**)&reply);
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }
    
    if (reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        return ERR_REDIS_ERROR;
    } 

    *bindingIndex = reply->integer;
    freeReplyObject(reply);
    return RULES_OK;
}

unsigned int formatEvalMessage(void *rulesBinding, 
                               char *sid, 
                               char *mid,
                               char *message, 
                               jsonProperty *allProperties,
                               unsigned int propertiesLength,
                               unsigned char actionType,
                               char **keys,
                               unsigned int keysLength,
                               char **command) {
    if (actionType == ACTION_RETRACT_FACT || actionType == ACTION_RETRACT_EVENT) {
        propertiesLength = 0; 
    }

    binding *bindingContext = (binding*)rulesBinding;
    time_t currentTime = time(NULL);
    char score[11];
    char keysLengthString[5];
#ifdef _WIN32
    sprintf_s(keysLengthString, 5, "%d", keysLength);
    sprintf_s(score, 11, "%ld", currentTime);
    char **argv = (char **)_alloca(sizeof(char*)*(8 + keysLength + propertiesLength * 3));
    size_t *argvl = (size_t *)_alloca(sizeof(size_t)*(8 + keysLength +  propertiesLength * 3));
#else
    snprintf(keysLengthString, 5, "%d", keysLength);
    snprintf(score, 11, "%ld", currentTime);
    char *argv[8 + keysLength + propertiesLength * 3];
    size_t argvl[8 + keysLength + propertiesLength * 3];
#endif

    argv[0] = "evalsha";
    argvl[0] = 7;
    argv[1] = bindingContext->evalMessageHash;
    argvl[1] = 40;
    argv[2] = "0";
    argvl[2] = 1;
    argv[3] = sid;
    argvl[3] = strlen(sid);
    argv[4] = mid;
    argvl[4] = strlen(mid);
    argv[5] = score;
    argvl[5] = strlen(score);
    argv[6] = (actionType == ACTION_ASSERT_FACT || actionType == ACTION_RETRACT_FACT) ? "1" : "0";
    argvl[6] = 1;
    argv[7] = keysLengthString;
    argvl[7] = strlen(keysLengthString);

    for (unsigned int i = 0; i < keysLength; ++i) {
        argv[8 + i] = keys[i];
        argvl[8 + i] = strlen(keys[i]);
    }

    unsigned int offset = 8 + keysLength;
    for (unsigned int i = 0; i < propertiesLength; ++i) {
        argv[offset + i * 3] = message + allProperties[i].nameOffset;
        argvl[offset + i * 3] = allProperties[i].nameLength;
        argv[offset + i * 3 + 1] = message + allProperties[i].valueOffset;
        if (allProperties[i].type == JSON_STRING) {
            argvl[offset + i * 3 + 1] = allProperties[i].valueLength;
        } else {
            argvl[offset + i * 3 + 1] = allProperties[i].valueLength + 1;
        }

        switch(allProperties[i].type) {
            case JSON_STRING:
                argv[offset + i * 3 + 2] = "1";
                break;
            case JSON_INT:
                argv[offset + i * 3 + 2] = "2";
                break;
            case JSON_DOUBLE:
                argv[offset + i * 3 + 2] = "3";
                break;
            case JSON_BOOL:
                argv[offset + i * 3 + 2] = "4";
                break;
            case JSON_ARRAY:
                argv[offset + i * 3 + 2] = "5";
                break;
            case JSON_NIL:
                argv[offset + i * 3 + 2] = "7";
                break;
        }
        argvl[offset + i * 3 + 2] = 1; 
    }

    int result = redisFormatCommandArgv(command, offset + propertiesLength * 3, (const char**)argv, argvl); 
    if (result == 0) {
        return ERR_OUT_OF_MEMORY;
    }
    return RULES_OK;
}

unsigned int formatStoreMessage(void *rulesBinding, 
                                char *sid, 
                                char *message, 
                                jsonProperty *allProperties,
                                unsigned int propertiesLength,
                                unsigned char storeFact,
                                char **keys,
                                unsigned int keysLength,
                                char **command) {
    binding *bindingContext = (binding*)rulesBinding;
    char keysLengthString[5];
#ifdef _WIN32
    sprintf_s(keysLengthString, 5, "%d", keysLength);
    char **argv = (char **)_alloca(sizeof(char*)*(6 + keysLength + propertiesLength * 3));
    size_t *argvl = (size_t *)_alloca(sizeof(size_t)*(6 + keysLength + propertiesLength * 3));
#else
    snprintf(keysLengthString, 5, "%d", keysLength);
    char *argv[6 + keysLength + propertiesLength * 3];
    size_t argvl[6 + keysLength + propertiesLength * 3];
#endif

    argv[0] = "evalsha";
    argvl[0] = 7;
    argv[1] = bindingContext->addMessageHash;
    argvl[1] = 40;
    argv[2] = "0";
    argvl[2] = 1;
    argv[3] = sid;
    argvl[3] = strlen(sid);
    argv[4] = storeFact ? "1" : "0";
    argvl[4] = 1;
    argv[5] = keysLengthString;
    argvl[5] = strlen(keysLengthString);

    for (unsigned int i = 0; i < keysLength; ++i) {
        argv[6 + i] = keys[i];
        argvl[6 + i] = strlen(keys[i]);
    }

    unsigned int offset = 6 + keysLength;
    for (unsigned int i = 0; i < propertiesLength; ++i) {
        argv[offset +  i * 3] = message + allProperties[i].nameOffset;
        argvl[offset + i * 3] = allProperties[i].nameLength;
        argv[offset + i * 3 + 1] = message + allProperties[i].valueOffset;
        if (allProperties[i].type == JSON_STRING) {
            argvl[offset + i * 3 + 1] = allProperties[i].valueLength;
        } else {
            argvl[offset + i * 3 + 1] = allProperties[i].valueLength + 1;
        }

        switch(allProperties[i].type) {
            case JSON_STRING:
                argv[offset + i * 3 + 2] = "1";
                break;
            case JSON_INT:
                argv[offset + i * 3 + 2] = "2";
                break;
            case JSON_DOUBLE:
                argv[offset + i * 3 + 2] = "3";
                break;
            case JSON_BOOL:
                argv[offset + i * 3 + 2] = "4";
                break;
            case JSON_ARRAY:
                argv[offset + i * 3 + 2] = "5";
                break;
            case JSON_NIL:
                argv[offset + i * 3 + 2] = "7";
                break;
        }
        argvl[offset + i * 3 + 2] = 1; 
    }

    int result = redisFormatCommandArgv(command, offset + propertiesLength * 3, (const char**)argv, argvl); 
    if (result == 0) {
        return ERR_OUT_OF_MEMORY;
    }
    return RULES_OK;
}

unsigned int formatStoreSession(void *rulesBinding, 
                                char *sid, 
                                char *state,
                                unsigned char tryExists, 
                                char **command) {
    binding *currentBinding = (binding*)rulesBinding;

    int result;
    if (tryExists) {
        result = redisFormatCommand(command,
                                    "hsetnx %s %s %s", 
                                    currentBinding->sessionHashset, 
                                    sid, 
                                    state);
    } else {
        result = redisFormatCommand(command,
                                    "hset %s %s %s", 
                                    currentBinding->sessionHashset, 
                                    sid, 
                                    state);
    }

    if (result == 0) {
        return ERR_OUT_OF_MEMORY;
    }
    return RULES_OK;
}

unsigned int formatStoreSessionFact(void *rulesBinding, 
                                    char *sid, 
                                    char *message,
                                    unsigned char tryExists, 
                                    char **command) {
    binding *currentBinding = (binding*)rulesBinding;

    int result;
    if (tryExists) {
        result = redisFormatCommand(command,
                                    "hsetnx %s %s!f %s", 
                                    currentBinding->sessionHashset, 
                                    sid, 
                                    message);
    } else {
        result = redisFormatCommand(command,
                                    "hset %s %s!f %s", 
                                    currentBinding->sessionHashset, 
                                    sid, 
                                    message);
    }

    if (result == 0) {
        return ERR_OUT_OF_MEMORY;
    }
    return RULES_OK;
}

unsigned int formatRemoveTimer(void *rulesBinding, 
                               char *timer, 
                               char **command) {
    binding *currentBinding = (binding*)rulesBinding;
    int result = redisFormatCommand(command,
                                    "zrem %s %s", 
                                    currentBinding->timersSortedset, 
                                    timer);
    if (result == 0) {
        return ERR_OUT_OF_MEMORY;
    }
    return RULES_OK;
}

unsigned int formatRemoveAction(void *rulesBinding, 
                                char *sid, 
                                char **command) {
    binding *bindingContext = (binding*)rulesBinding;
    time_t currentTime = time(NULL);

    int result = redisFormatCommand(command,
                                    "evalsha %s 0 %s %ld", 
                                    bindingContext->removeActionHash, 
                                    sid, 
                                    currentTime); 
    if (result == 0) {
        return ERR_OUT_OF_MEMORY;
    }
    return RULES_OK;
}

unsigned int formatRemoveMessage(void *rulesBinding, 
                                  char *sid, 
                                  char *mid,
                                  unsigned char removeFact,
                                  char **command) {
    binding *currentBinding = (binding*)rulesBinding;

    int result = 0;
    if (removeFact) {
        result = redisFormatCommand(command,
                                    "hdel %s!%s %s", 
                                    currentBinding->factsHashset, 
                                    sid, 
                                    mid);
    } else {
        result = redisFormatCommand(command,
                                    "hdel %s!%s %s", 
                                    currentBinding->eventsHashset, 
                                    sid, 
                                    mid);
    }

    if (result == 0) {
        return ERR_OUT_OF_MEMORY;
    }
    return RULES_OK;
}

unsigned int formatPeekAction(void *rulesBinding,
                              char *sid,
                              char **command) {
    binding *currentBinding = (binding*)rulesBinding;
   
    time_t currentTime = time(NULL);
    int result = redisFormatCommand(command, 
                                    "evalsha %s 0 %d %ld %s", 
                                    currentBinding->peekActionHash, 
                                    currentTime + 15,
                                    currentTime,
                                    sid); 
    if (result == 0) {
        return ERR_OUT_OF_MEMORY;
    }

    return RULES_OK;
}


unsigned int startNonBlockingBatch(void *rulesBinding,
                                   char **commands,
                                   unsigned int commandCount,
                                   unsigned int *replyCount) {
    *replyCount = commandCount;
    if (commandCount == 0) {
        return RULES_OK;
    }

    unsigned int result = RULES_OK;
    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext;
    
    sds newbuf = sdsempty();
    for (unsigned int i = 0; i < commandCount; ++i) {
        newbuf = sdscatlen(newbuf, commands[i], strlen(commands[i]));
        if (newbuf == NULL) {
            return ERR_OUT_OF_MEMORY;
        }

        free(commands[i]);
    }

    reContext->obuf = newbuf;
    int wdone = 0;
    do {
        if (redisBufferWrite(reContext, &wdone) == REDIS_ERR) {
            printf("start non blocking batch error %u %s\n", reContext->err, reContext->errstr);
            return ERR_REDIS_ERROR;
        }
    } while (!wdone);

    return result;
}

static unsigned int tryGetReply(redisContext *reContext, 
                                redisReply **reply) {

    if (redisGetReply(reContext, (void**)reply) != REDIS_OK) {
        printf("getReply err %d, %d, %s\n", reContext->err, errno, reContext->errstr);
        #ifndef _WIN32
        if (redisReconnect(reContext) != REDIS_OK) {
            printf("reconnect err %d, %d, %s\n", reContext->err, errno, reContext->errstr);
            return ERR_REDIS_ERROR;
        }
        return ERR_TRY_AGAIN;
        #else
        return ERR_REDIS_ERROR;
        #endif
    }

    return RULES_OK;
}

unsigned int completeNonBlockingBatch(void *rulesBinding,
                                      unsigned int replyCount) {
    if (replyCount == 0) {
        return RULES_OK;
    }

    unsigned int result = RULES_OK;
    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext;
    redisReply *reply;
    for (unsigned int i = 0; i < replyCount; ++i) {
        result = tryGetReply(reContext, &reply);
        if (result != RULES_OK) {
            return result;
        } else {
            if (reply->type == REDIS_REPLY_ERROR) {
                printf("complete non blocking batch error %d %s\n", i, reply->str);
                result = ERR_REDIS_ERROR;
            }

            freeReplyObject(reply);    
        } 
    }
    
    return result;
}

unsigned int executeBatch(void *rulesBinding,
                          char **commands,
                          unsigned int commandCount) {
    return executeBatchWithReply(rulesBinding, 0, commands, commandCount, NULL);
}

unsigned int executeBatchWithReply(void *rulesBinding,
                                   unsigned int expectedReplies,
                                   char **commands,
                                   unsigned int commandCount,
                                   redisReply **lastReply) {
    if (commandCount == 0) {
        return RULES_OK;
    }

    unsigned int result = RULES_OK;
    unsigned int replyCount = commandCount + expectedReplies;
    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext;
    if (lastReply) {
        *lastReply = NULL;
    }

    sds newbuf = sdsempty();
    for (unsigned int i = 0; i < commandCount; ++i) {
        newbuf = sdscatlen(newbuf, commands[i], strlen(commands[i]));
        if (newbuf == NULL) {
            return ERR_OUT_OF_MEMORY;
        }

        free(commands[i]);
    }

    reContext->obuf = newbuf;
    redisReply *reply;
    for (unsigned int i = 0; i < replyCount; ++i) {
        result = tryGetReply(reContext, &reply);
        if (result != RULES_OK) {
            return result;
        } else {
            if (reply->type == REDIS_REPLY_ERROR) {
                printf("%s\n", reply->str);
                freeReplyObject(reply);
                result = ERR_REDIS_ERROR;
            } else if (reply->type == REDIS_REPLY_ARRAY) {
                if (lastReply == NULL) {
                    freeReplyObject(reply); 
                } else {
                    if (*lastReply != NULL) {    
                        freeReplyObject(*lastReply);  
                    }

                    *lastReply = reply;
                }
            } else {
                freeReplyObject(reply);    
            }
        } 
    }
    
    return result;
}

unsigned int removeMessage(void *rulesBinding, char *sid, char *mid) {
    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext;  
    int result = redisAppendCommand(reContext, 
                                    "hdel %s!%s %s", 
                                    currentBinding->factsHashset, 
                                    sid, 
                                    mid);
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }

    redisReply *reply;
    result = tryGetReply(reContext, &reply);
    if (result != RULES_OK) {
        return result;
    }

    if (reply->type == REDIS_REPLY_ERROR) {
        printf("removeMessage err string %s\n", reply->str);
        freeReplyObject(reply);
        return ERR_REDIS_ERROR;
    }
    
    freeReplyObject(reply);    
    return RULES_OK;
}

unsigned int peekAction(ruleset *tree, void **bindingContext, redisReply **reply) {
    bindingsList *list = tree->bindingsList;
    for (unsigned int i = 0; i < list->bindingsLength; ++i) {
        binding *currentBinding = &list->bindings[list->lastBinding % list->bindingsLength];
        ++list->lastBinding;
        redisContext *reContext = currentBinding->reContext;
        time_t currentTime = time(NULL);

        int result = redisAppendCommand(reContext, 
                                        "evalsha %s 0 %d %ld", 
                                        currentBinding->peekActionHash, 
                                        currentTime + 15,
                                        currentTime); 
        if (result != REDIS_OK) {
            continue;
        }

        result = tryGetReply(reContext, reply);
        if (result != RULES_OK) {
            return result;
        }

        if ((*reply)->type == REDIS_REPLY_ERROR) {
            printf("peekAction err string %s\n", (*reply)->str);
            freeReplyObject(*reply);
            return ERR_REDIS_ERROR;
        }
        
        if ((*reply)->type == REDIS_REPLY_ARRAY) {
            *bindingContext = currentBinding;
            return RULES_OK;
        } else {
            freeReplyObject(*reply);
        }
    }

    return ERR_NO_ACTION_AVAILABLE;
}

unsigned int peekTimers(ruleset *tree, void **bindingContext, redisReply **reply) {
    bindingsList *list = tree->bindingsList;
    for (unsigned int i = 0; i < list->bindingsLength; ++i) {
        binding *currentBinding = &list->bindings[list->lastTimersBinding % list->bindingsLength];
        ++list->lastTimersBinding;
        redisContext *reContext = currentBinding->reContext;
        time_t currentTime = time(NULL);

        int result = redisAppendCommand(reContext, 
                                        "evalsha %s 0 %ld", 
                                        currentBinding->timersHash,
                                        currentTime); 
        if (result != REDIS_OK) {
            continue;
        }

        result = tryGetReply(reContext, reply);
        if (result != RULES_OK) {
            return result;
        }

        if ((*reply)->type == REDIS_REPLY_ERROR) {
            printf("peekTimers err string %s\n", (*reply)->str);
            freeReplyObject(*reply);
            return ERR_REDIS_ERROR;
        }
        
        if ((*reply)->type == REDIS_REPLY_ARRAY) {
            *bindingContext = currentBinding;
            return RULES_OK;
        } else {
            freeReplyObject(*reply);
        }
    }

    return ERR_NO_TIMERS_AVAILABLE;
}

unsigned int registerTimer(void *rulesBinding, unsigned int duration, char *timer) {
    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext;   
    time_t currentTime = time(NULL);

    int result = redisAppendCommand(reContext, 
                                    "zadd %s %ld p:%s", 
                                    currentBinding->timersSortedset, 
                                    currentTime + duration, 
                                    timer);
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }
    
    redisReply *reply;
    result = tryGetReply(reContext, &reply);
    if (result != RULES_OK) {
        return result;
    }

    if (reply->type == REDIS_REPLY_ERROR) {
        printf("registerTimer err string %s\n", reply->str);
        freeReplyObject(reply);
        return ERR_REDIS_ERROR;
    }

    freeReplyObject(reply);    
    return RULES_OK;
}

unsigned int removeTimer(void *rulesBinding, char *timer) {
    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext;   
    
    int result = redisAppendCommand(reContext, 
                                    "zrem %s p:%s", 
                                    currentBinding->timersSortedset,
                                    timer);
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }
    
    redisReply *reply;
    result = tryGetReply(reContext, &reply);
    if (result != RULES_OK) {
        return result;
    }

    if (reply->type == REDIS_REPLY_ERROR) {
        printf("deleteTimer err string %s\n", reply->str);
        freeReplyObject(reply);
        return ERR_REDIS_ERROR;
    }

    freeReplyObject(reply);    
    return RULES_OK;
}

unsigned int registerMessage(void *rulesBinding, unsigned int queueAction, char *destination, char *message) {
    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext;   
    time_t currentTime = time(NULL);

    int result = REDIS_OK;

    switch (queueAction) {
        case QUEUE_ASSERT_FACT:
            result = redisAppendCommand(reContext, 
                                        "zadd %s!t %ld a:%s", 
                                        destination, 
                                        currentTime, 
                                        message);
            break;
        case QUEUE_ASSERT_EVENT:
            result = redisAppendCommand(reContext, 
                                        "zadd %s!t %ld p:%s", 
                                        destination, 
                                        currentTime, 
                                        message);
            break;
        case QUEUE_RETRACT_FACT:
            result = redisAppendCommand(reContext, 
                                        "zadd %s!t %ld r:%s", 
                                        destination, 
                                        currentTime, 
                                        message);
            break;
    }
    
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }
    
    redisReply *reply;
    result = tryGetReply(reContext, &reply);
    if (result != RULES_OK) {
        return result;
    }

    if (reply->type == REDIS_REPLY_ERROR) {
        printf("registerMessage err string %s\n", reply->str);
        freeReplyObject(reply);
        return ERR_REDIS_ERROR;
    }

    freeReplyObject(reply);    
    return RULES_OK;
}

unsigned int getSession(void *rulesBinding, char *sid, char **state) {
    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext; 
    unsigned int result = redisAppendCommand(reContext, 
                                "hget %s %s", 
                                currentBinding->sessionHashset, 
                                sid);
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }

    redisReply *reply;
    result = tryGetReply(reContext, &reply);
    if (result != RULES_OK) {
        return result;
    }

    if (reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        return ERR_REDIS_ERROR;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        return ERR_NEW_SESSION;
    }

    *state = malloc((strlen(reply->str) + 1) * sizeof(char));
    if (!*state) {
        return ERR_OUT_OF_MEMORY;
    }
    strcpy(*state, reply->str);
    freeReplyObject(reply); 
    return REDIS_OK;
}

unsigned int deleteSession(void *rulesBinding, char *sid) {
    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext; 

    int result = redisAppendCommand(reContext, 
                                    "evalsha %s 0 %s", 
                                    currentBinding->deleteSessionHash,
                                    sid); 
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }
    
    redisReply *reply;
    result = tryGetReply(reContext, &reply);
    if (result != RULES_OK) {
        return result;
    }

    if (reply->type == REDIS_REPLY_ERROR) {
        printf("deleteSession err string %s\n", reply->str);
        freeReplyObject(reply);
        return ERR_REDIS_ERROR;
    }

    freeReplyObject(reply);  
    return REDIS_OK;
}

unsigned int updateAction(void *rulesBinding, char *sid) {
    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext;   
    time_t currentTime = time(NULL);

    int result = redisAppendCommand(reContext, 
                                    "evalsha %s 0 %s %ld", 
                                    currentBinding->updateActionHash,
                                    sid,
                                    currentTime + 15); 
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }
    
    redisReply *reply;
    result = tryGetReply(reContext, &reply);
    if (result != RULES_OK) {
        return result;
    }

    if (reply->type == REDIS_REPLY_ERROR) {
        printf("updateAction err string %s\n", reply->str);
        freeReplyObject(reply);
        return ERR_REDIS_ERROR;
    }

    freeReplyObject(reply);    
    return RULES_OK;
}
