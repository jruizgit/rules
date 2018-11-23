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

#define VERIFY(result, origin) do { \
    if (result != REDIS_OK) { \
        return ERR_REDIS_ERROR; \
    } \
    redisReply *reply; \
    result = tryGetReply(reContext, &reply); \
    if (result != RULES_OK) { \
        return result; \
    } \
    if (reply->type == REDIS_REPLY_ERROR) { \
        printf(origin); \
        printf(" err string %s\n", reply->str); \
        freeReplyObject(reply); \
        return ERR_REDIS_ERROR; \
    } \
    freeReplyObject(reply);\
} while(0)

#define GET_REPLY(result, origin, reply) do { \
    if (result != REDIS_OK) { \
        return ERR_REDIS_ERROR; \
    } \
    result = tryGetReply(reContext, &reply); \
    if (result != RULES_OK) { \
        return result; \
    } \
    if (reply->type == REDIS_REPLY_ERROR) { \
        printf(origin); \
        printf(" err string %s\n", reply->str); \
        freeReplyObject(reply); \
        return ERR_REDIS_ERROR; \
    } \
} while(0)

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
        case JSON_EVENT_LOCAL_PROPERTY:
            rightProperty = &tree->stringPool[newValue->value.property.nameOffset];
            if (asprintf(idiomString, "message[\"%s\"]", rightProperty) == -1) {
                return ERR_OUT_OF_MEMORY;
            }
            break;
        case JSON_EVENT_LOCAL_IDIOM:
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
        } else if (currentNode->value.a.operator == OP_IALL || currentNode->value.a.operator == OP_IANY) {
            char *oldTest = *test;
            char *leftProperty = &tree->stringPool[currentNode->nameOffset];
            idiom *newIdiom = &tree->idiomPool[currentNode->value.a.right.value.idiomOffset];
            char *op = "";
            switch (newIdiom->operator) {
                case OP_LT:
                    op = "0";
                    break;
                case OP_LTE:
                    op = "1";
                    break;
                case OP_GT:
                    op = "2";
                    break;
                case OP_GTE:
                    op = "3";
                    break;
                case OP_EQ:
                    op = "4";
                    break;
                case OP_NEQ:
                    op = "5";
                    break;
            }

            char *par = "";
            if (currentNode->value.a.operator == OP_IALL) {
                par = "true";
            } else {
                par = "false";
            }

            char *idiomString = NULL;
            unsigned int result = createIdiom(tree, &newIdiom->right, &idiomString);
            if (result != RULES_OK) {
                return result;
            }
            
            if (first) {
                if (expr->distinct) {
                    if (asprintf(test, "is_distinct_message(frame, message) and %scompare_array(message[\"%s\"], %s, %s, %s)", *test, leftProperty, idiomString, op, par)  == -1) {
                        return ERR_OUT_OF_MEMORY;
                    }
                } else {
                    if (asprintf(test, "%scompare_array(message[\"%s\"], %s, %s, %s)", *test, leftProperty, idiomString, op, par)  == -1) {
                        return ERR_OUT_OF_MEMORY;
                    }
                }
                
                first = 0;
            } else {
                if (asprintf(test, "%s %s compare_array(message[\"%s\"], %s, %s, %s)", *test, comp, leftProperty, idiomString, op, par)  == -1) {
                    return ERR_OUT_OF_MEMORY;
                }
                
                first = 0;   
            }

            free(idiomString);
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
                if (expr->distinct) {
                    if (asprintf(test, "is_distinct_message(frame, message) and %smessage[\"%s\"] %s %s", *test, leftProperty, op, idiomString)  == -1) {
                        return ERR_OUT_OF_MEMORY;
                    }
                } else {
                    if (asprintf(test, "%smessage[\"%s\"] %s %s", *test, leftProperty, op, idiomString)  == -1) {
                        return ERR_OUT_OF_MEMORY;
                    }
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
"        result = result .. tostring(message[\"%s\"])\n"
"    end\n", 
                                 leftProperty, 
                                 leftProperty)  == -1) {
                        return ERR_OUT_OF_MEMORY;
                    }

                    if (asprintf(primaryFrameKey, 
"    if not %s then\n"
"        return \"\"\n"
"    else\n"
"        result = result .. tostring(%s)\n"
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
"        result = result .. tostring(message[\"%s\"])\n"
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
"        result = result .. tostring(%s)\n"
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

    unsigned int result = redisAppendCommand(reContext, "SCRIPT LOAD %s", lua); 
    GET_REPLY(result, "loadPartitionCommand", reply);

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
"    if raw_count then"
"        local count = tonumber(string.sub(raw_count, 3))\n"
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

    unsigned int result = redisAppendCommand(reContext, "SCRIPT LOAD %s", lua);
    GET_REPLY(result, "loadRemoveActionCommand", reply);
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
"    for i = 1, #res, 1 do\n"
"        redis.call(\"zincrby\", timer_key, 10, tostring(res[i]))\n"
"    end\n"
"    return res\n"
"end\n"
"return 0\n", name)  == -1) {
        return ERR_OUT_OF_MEMORY;
    }

    unsigned int result = redisAppendCommand(reContext, "SCRIPT LOAD %s", lua);
    GET_REPLY(result, "loadTimerCommand", reply);

    strncpy(rulesBinding->timersHash, reply->str, 40);
    rulesBinding->timersHash[40] = '\0';
    freeReplyObject(reply);
    free(lua);
    return RULES_OK;
}

static unsigned int loadRemoveTimerCommand(ruleset *tree, binding *rulesBinding) {
    char *name = &tree->stringPool[tree->nameOffset];
    redisContext *reContext = rulesBinding->reContext;
    redisReply *reply;
    char *lua = NULL;
    if (asprintf(&lua,
"local timer_key = \"%s!t\"\n"
"local timer_name = ARGV[1]\n"
"local res = redis.call(\"zrange\", timer_key, 0, -1)\n"
"for i = 1, #res, 1 do\n"
"    if string.find(res[i], \"\\\"%%$t\\\"%%s*:%%s*\\\"\" .. timer_name .. \"\\\"\") then\n"
"        redis.call(\"zrem\", timer_key, res[i])\n"
"    end\n"
"end\n"
"return 0\n", name)  == -1) {
        return ERR_OUT_OF_MEMORY;
    }

    unsigned int result = redisAppendCommand(reContext, "SCRIPT LOAD %s", lua);
    GET_REPLY(result, "loadRemoveTimerCommand", reply);

    strncpy(rulesBinding->removeTimerHash, reply->str, 40);
    rulesBinding->removeTimerHash[40] = '\0';
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

    unsigned int result = redisAppendCommand(reContext, "SCRIPT LOAD %s", lua);
    GET_REPLY(result, "loadUpdateActionCommand", reply);

    strncpy(rulesBinding->updateActionHash, reply->str, 40);
    rulesBinding->updateActionHash[40] = '\0';
    freeReplyObject(reply);
    free(lua);
    return RULES_OK;
}

static unsigned int loadDeleteSessionCommand(ruleset *tree, binding *rulesBinding) {
    return RULES_OK;

}
static unsigned int loadAddMessageCommand(ruleset *tree, binding *rulesBinding) {
    return RULES_OK;
}

static unsigned int loadPeekActionCommand(ruleset *tree, binding *rulesBinding) {
    return RULES_OK;
}

static void sortActions(ruleset *tree, node **sortedActions, unsigned int *actionCount) {
    *actionCount = 0;
     for (unsigned int i = 0; i < tree->nodeOffset; ++i) {
        node *currentNode = &tree->nodePool[i];   
        if (currentNode->type == NODE_ACTION) {
            unsigned int index = *actionCount;
            while (index > 0 && currentNode->value.c.priority < sortedActions[index - 1]->value.c.priority) {
                sortedActions[index] = sortedActions[index - 1];
                --index;
            }

            sortedActions[index] = currentNode;
            ++*actionCount;
        }
    }
}

static unsigned int loadEvalMessageCommand(ruleset *tree, binding *rulesBinding) {
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

    char *partitionHashset = malloc((nameLength + 3) * sizeof(char));
    if (!partitionHashset) {
        return ERR_OUT_OF_MEMORY;
    }

    strncpy(partitionHashset, name, nameLength);
    partitionHashset[nameLength] = '!';
    partitionHashset[nameLength + 1] = 'p';
    partitionHashset[nameLength + 2] = '\0';
    rulesBinding->partitionHashset = partitionHashset;

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

    result = loadRemoveTimerCommand(tree, rulesBinding);
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

unsigned int bindRuleset(unsigned int handle, 
                         char *host, 
                         unsigned int port, 
                         char *password,
                         unsigned char db) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);
    
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

    int result = REDIS_OK;

#ifndef _WIN32
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    result = redisSetTimeout(reContext, tv);
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }
#endif

    if (password != NULL) {
        result = redisAppendCommand(reContext, "auth %s", password);
        VERIFY(result, "bindRuleset");
    }

    if (db) {
        result = redisAppendCommand(reContext, "select %d", db);
        VERIFY(result, "bindRuleset");
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
                                    "evalsha %s 0 %u %d", 
                                    firstBinding->partitionHash, 
                                    sidHash, 
                                    list->bindingsLength);
    redisReply *reply;
    GET_REPLY(result, "getBindingIndex", reply);

    *bindingIndex = reply->integer;
    freeReplyObject(reply);
    return RULES_OK;
}

unsigned int formatEvalMessage(void *rulesBinding, 
                               char *sid, 
                               char *mid,
                               jsonObject *jo,
                               unsigned char actionType,
                               char **keys,
                               unsigned int keysLength,
                               char **command) {
    
    return RULES_OK;
}

unsigned int formatStoreMessage(void *rulesBinding, 
                                char *sid, 
                                jsonObject *jo,
                                unsigned char storeFact,
                                unsigned char markVisited,
                                char **keys,
                                unsigned int keysLength,
                                char **command) {
    return RULES_OK;
}

unsigned int formatStoreSession(void *rulesBinding, 
                                char *sid, 
                                char *state,
                                unsigned char tryExists, 
                                char **storeCommand) {
    binding *currentBinding = (binding*)rulesBinding;

    int result;
    if (tryExists) {
        result = redisFormatCommand(storeCommand,
                                    "hsetnx %s %s %s", 
                                    currentBinding->sessionHashset, 
                                    sid, 
                                    state);
    } else {
        result = redisFormatCommand(storeCommand,
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

    sdsfree(reContext->obuf);
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
            } else if (reply->type == REDIS_REPLY_INTEGER) {
                if (reply->integer == ERR_EVENT_OBSERVED && result == RULES_OK) {
                    result = ERR_EVENT_OBSERVED;
                }
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

    sdsfree(reContext->obuf);
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
            } else if (reply->type == REDIS_REPLY_INTEGER) {
                if (reply->integer == ERR_EVENT_OBSERVED && result == RULES_OK) {
                    result = ERR_EVENT_OBSERVED;
                }

                freeReplyObject(reply);
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
    VERIFY(result, "removeMessage");   
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
            if ((*reply)->elements < 3) {
                freeReplyObject(*reply);
                return ERR_REDIS_ERROR;       
            }
            
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

unsigned int registerTimer(void *rulesBinding, unsigned int duration, char assert, char *timer) {
    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext;   
    time_t currentTime = time(NULL);

    int result = RULES_OK;
    if (assert) {
        result = redisAppendCommand(reContext, 
                                    "zadd %s %ld a:%s", 
                                    currentBinding->timersSortedset, 
                                    currentTime + duration, 
                                    timer);
    } else {
        result = redisAppendCommand(reContext, 
                                    "zadd %s %ld p:%s", 
                                    currentBinding->timersSortedset, 
                                    currentTime + duration, 
                                    timer);
    } 

    VERIFY(result, "registerTimer");   
    return RULES_OK;
}

unsigned int removeTimer(void *rulesBinding, char *timerName) {
    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext;   
    int result = redisAppendCommand(reContext, 
                                    "evalsha %s 0 %s", 
                                    currentBinding->removeTimerHash,
                                    timerName); 
    VERIFY(result, "removeTimer");  
    return RULES_OK;
}

unsigned int registerMessage(void *rulesBinding, unsigned int queueAction, char *destination, char *message) {
    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext;   
    time_t currentTime = time(NULL);

    int result = RULES_OK;

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
    
    VERIFY(result, "registerMessage");  
    return RULES_OK;
}

unsigned int getSession(void *rulesBinding, char *sid, char **state) {
    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext; 
    unsigned int result = redisAppendCommand(reContext, 
                                "hget %s %s", 
                                currentBinding->sessionHashset, 
                                sid);
    redisReply *reply;
    GET_REPLY(result, "getSession", reply);
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

unsigned int getSessionVersion(void *rulesBinding, char *sid, unsigned long *stateVersion) {
    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext; 
    unsigned int result = redisAppendCommand(reContext, 
                                             "hget %s!v %s", 
                                             currentBinding->sessionHashset, 
                                             sid);
    
    redisReply *reply;
    GET_REPLY(result, "getSessionVersion", reply);
    if (reply->type != REDIS_REPLY_INTEGER) {
        *stateVersion = 0;
    } else {
        *stateVersion = reply->integer;
    }

    freeReplyObject(reply); 
    return REDIS_OK;
}

unsigned int deleteSession(ruleset *tree, void *rulesBinding, char *sid, unsigned int sidHash) {
    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext; 

    int result = redisAppendCommand(reContext, 
                                    "evalsha %s 0 %s %u", 
                                    currentBinding->deleteSessionHash,
                                    sid,
                                    sidHash); 
    VERIFY(result, "deleteSession");  

    bindingsList *list = tree->bindingsList;
    binding *firstBinding = &list->bindings[0];
    if (firstBinding != currentBinding) {
        reContext = firstBinding->reContext;
        result = redisAppendCommand(reContext, 
                                    "hdel %s %u", 
                                    firstBinding->partitionHashset,
                                    sidHash);
        VERIFY(result, "deleteSession");
    }

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
    VERIFY(result, "updateAction");  
    return RULES_OK;
}
