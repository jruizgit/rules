
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "net.h"
#include "rules.h"

static unsigned int loadCommands(ruleset *tree, binding *rulesBinding) {
    redisContext *reContext = rulesBinding->reContext;
    redisReply *reply;
    rulesBinding->hashArray = malloc(tree->actionCount * sizeof(functionHash));
    char *name = &tree->stringPool[tree->nameOffset];
    int nameLength = strlen(name);
    for (unsigned int i = 0; i < tree->nodeOffset; ++i) {
        char *lua = NULL;
        char *oldLua;
        node *currentNode = &tree->nodePool[i];
        if (currentNode->type == NODE_ACTION) {
            unsigned int queryLength = currentNode->value.c.queryLength;
            char *actionName = &tree->stringPool[currentNode->nameOffset];
            for (unsigned int ii = 0; ii < queryLength; ++ii) {
                unsigned int lineOffset = tree->queryPool[currentNode->value.c.queryOffset + ii];
                char *currentLine = &tree->stringPool[lineOffset];
                char *last = currentLine;
                unsigned int clauseCount = 0;
                while (last[0] != '\0') {
                    if (last[0] == ' ') {
                        last[0] = '\0';
                        unsigned char limit = 0;
                        if (strchr(currentLine, '+')) {
                            limit = MAX_MESSAGE_BATCH - 1;
                        }

                        if (lua) {
                            oldLua = lua;
                            asprintf(&lua, "%skey = \"%s!\" .. ARGV[2]\n"
                                           "res = redis.call(\"zrange\", key, 0, %d)\n"
                                           "if (res[1]) then\n"
                                           "  i = 1\n"
                                           "  while(res[i]) do\n"
                                           "    signature = signature .. \",\" .. key .. \",\" .. res[i]\n"
                                           "    i = i + 1\n"
                                           "  end\n", 
                                    lua, currentLine, limit);
                            free(oldLua);
                        } else {
                            asprintf(&lua, "key = \"%s!\" .. ARGV[2]\n"
                                           "res = redis.call(\"zrange\", key, 0, %d)\n"
                                           "if (res[1]) then\n"
                                           "  i = 1\n"
                                           "  while (res[i]) do\n"
                                           "    signature = signature .. \",\" .. key .. \",\" .. res[i]\n"
                                           "    i = i + 1\n"
                                           "  end\n", 
                                    currentLine, limit);
                        }

                        last[0] = ' ';
                        currentLine = last + 1;
                        ++clauseCount;
                    }
                    ++last;
                } 

                oldLua = lua;
                asprintf(&lua, "%s  redis.call(\"zadd\", KEYS[2], ARGV[5], signatureKey)\n"
                               "  redis.call(\"hset\", \"%s!r\", signatureKey, signature)\n"
                               "  return result\n", lua, name);
              
                free(oldLua);

                for (unsigned int iii = 0; iii < clauseCount; ++iii) {
                    oldLua = lua;
                    asprintf(&lua, "%send\n", lua);
                    free(oldLua);
                }
            }

            oldLua = lua;
            asprintf(&lua, "local signature = \"$s\" .. \",\" .. ARGV[2]\n"
                           "local signatureKey = \"%s\" .. \"!\" .. ARGV[2]\n"
                           "local key = ARGV[1] .. \"!\" .. ARGV[2]\n"
                           "local result = 0\n"
                           "local i\n"
                           "if (ARGV[6] == \"1\") then\n"
                           "  redis.call(\"hset\", KEYS[1], ARGV[2], ARGV[4])\n"
                           "  redis.call(\"zadd\", key, ARGV[5], ARGV[2])\n"
                           "else\n"
                           "  result = redis.call(\"hsetnx\", KEYS[3], ARGV[2], \"{ \\\"id\\\":\\\"\" .. ARGV[2] .. \"\\\" }\")\n"
                           "  redis.call(\"hsetnx\", KEYS[1], ARGV[3], ARGV[4])\n"
                           "  redis.call(\"zadd\", key, ARGV[5], ARGV[3])\n"
                           "end\n"
                           "local res = redis.call(\"hexists\", \"%s!r\", signatureKey)\n" 
                           "if (res ~= 0) then\n"
                           "  return result\n"
                           "end\n%s"
                           "return result\n", actionName, name, lua);  
            free(oldLua); 
            redisAppendCommand(reContext, "SCRIPT LOAD %s", lua);
            redisGetReply(reContext, (void**)&reply);
            if (reply->type == REDIS_REPLY_ERROR) {
                freeReplyObject(reply);
                free(lua);
                return ERR_REDIS_ERROR;
            }

            functionHash *currentAssertHash = &rulesBinding->hashArray[currentNode->value.c.index];
            strncpy(*currentAssertHash, reply->str, HASH_LENGTH);
            (*currentAssertHash)[HASH_LENGTH] = '\0';
            freeReplyObject(reply);
            free(lua);
        }
    }

    redisAppendCommand(reContext, "SCRIPT LOAD %s", 
                    "local action = redis.call(\"zrange\", KEYS[3], 0, 0, \"withscores\")\n"
                    "local timestamp = tonumber(ARGV[1])\n"
                    "if (action[2] ~= nil and (tonumber(action[2]) < (timestamp + 100))) then\n"
                    "  redis.call(\"zincrby\", KEYS[3], 60, action[1])\n"
                    "  local signature = redis.call(\"hget\", KEYS[4], action[1])\n"
                    "  local res = { action[1] }\n"
                    "  local i = 0\n"
                    "  local skip = 0\n"
                    "  for token in string.gmatch(signature, \"([%w%.%-%$%+_!:]+)\") do\n"
                    "    if (i > 1 and string.match(token, \"%$s!\")) then\n"
                    "      table.insert(res, token)\n"
                    "      skip = 1\n"
                    "    elseif (skip == 1) then\n"
                    "      table.insert(res, \"null\")\n"
                    "      skip = 0\n"
                    "    elseif (i % 2 == 0) then\n"
                    "      table.insert(res, token)\n"
                    "    else\n"
                    "      if (i == 1) then\n"
                    "        table.insert(res, redis.call(\"hget\", KEYS[1], token))\n"
                    "      else\n"
                    "        table.insert(res, redis.call(\"hget\", KEYS[2], token))\n"
                    "      end\n"
                    "    end\n"
                    "    i = i + 1\n"
                    "  end\n"
                    "  return res\n"
                    "end\n"
                    "return false\n");

    redisGetReply(reContext, (void**)&reply);
    if (reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        return ERR_REDIS_ERROR;
    }

    strncpy(rulesBinding->dequeueActionHash, reply->str, 40);
    rulesBinding->dequeueActionHash[40] = '\0';
    freeReplyObject(reply);

    redisAppendCommand(reContext, "SCRIPT LOAD %s", 
                    "local res = redis.call(\"hget\", KEYS[1], ARGV[1])\n"
                    "if (not res) then\n"
                    "   res = redis.call(\"hincrby\", KEYS[1], \"index\", 1)\n"
                    "   res = res % tonumber(ARGV[2])\n"
                    "   redis.call(\"hset\", KEYS[1], ARGV[1], res)\n"
                    "end\n"
                    "return tonumber(res)\n");

    redisGetReply(reContext, (void**)&reply);
    if (reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        return ERR_REDIS_ERROR;
    }

    strncpy(rulesBinding->partitionHash, reply->str, 40);
    rulesBinding->partitionHash[40] = '\0';
    freeReplyObject(reply);

    redisAppendCommand(reContext, "SCRIPT LOAD %s", 
                    "local timestamp = tonumber(ARGV[1])\n"
                    "local res = redis.call(\"zrangebyscore\", KEYS[1], 0, timestamp)\n"
                    "if (res[1]) then\n"
                    "  local i = 1\n"
                    "  while (res[i]) do\n"
                    "    redis.call(\"zincrby\", KEYS[1], 10, res[i])\n"
                    "    i = i + 1\n"
                    "  end\n"
                    "  return res\n"
                    "end\n"
                    "return 0\n");

    redisGetReply(reContext, (void**)&reply);
    if (reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        return ERR_REDIS_ERROR;
    }

    strncpy(rulesBinding->timersHash, reply->str, 40);
    rulesBinding->timersHash[40] = '\0';
    freeReplyObject(reply);

    char *sessionHashset = malloc((nameLength + 3) * sizeof(char));
    if (!sessionHashset) {
        return ERR_OUT_OF_MEMORY;
    }

    strncpy(sessionHashset, name, nameLength);
    sessionHashset[nameLength] = '!';
    sessionHashset[nameLength + 1] = 's';
    sessionHashset[nameLength + 2] = '\0';
    rulesBinding->sessionHashset = sessionHashset;

    char *messageHashset = malloc((nameLength + 3) * sizeof(char));
    if (!messageHashset) {
        return ERR_OUT_OF_MEMORY;
    }

    strncpy(messageHashset, name, nameLength);
    messageHashset[nameLength] = '!';
    messageHashset[nameLength + 1] = 'm';
    messageHashset[nameLength + 2] = '\0';
    rulesBinding->messageHashset = messageHashset;

    char *actionSortedset = malloc((nameLength + 3) * sizeof(char));
    if (!actionSortedset) {
        return ERR_OUT_OF_MEMORY;
    }

    strncpy(actionSortedset, name, nameLength);
    actionSortedset[nameLength] = '!';
    actionSortedset[nameLength + 1] = 'a';
    actionSortedset[nameLength + 2] = '\0';
    rulesBinding->actionSortedset = actionSortedset;

    char *timersSortedset = malloc((nameLength + 3) * sizeof(char));
    if (!timersSortedset) {
        return ERR_OUT_OF_MEMORY;
    }

    strncpy(timersSortedset, name, nameLength);
    timersSortedset[nameLength] = '!';
    timersSortedset[nameLength + 1] = 't';
    timersSortedset[nameLength + 2] = '\0';
    rulesBinding->timersSortedset = timersSortedset;

    char *resultsHashset = malloc((nameLength + 3) * sizeof(char));
    if (!resultsHashset) {
        return ERR_OUT_OF_MEMORY;
    }

    strncpy(resultsHashset, name, nameLength);
    resultsHashset[nameLength] = '!';
    resultsHashset[nameLength + 1] = 'r';
    resultsHashset[nameLength + 2] = '\0';
    rulesBinding->resultsHashset = resultsHashset;

    char *partitionHashset = malloc((nameLength + 3) * sizeof(char));
    if (!partitionHashset) {
        return ERR_OUT_OF_MEMORY;
    }

    strncpy(partitionHashset, name, nameLength);
    partitionHashset[nameLength] = '!';
    partitionHashset[nameLength + 1] = 'p';
    partitionHashset[nameLength + 2] = '\0';
    rulesBinding->partitionHashset = partitionHashset;
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

    if (password != NULL) {
        int result = redisAppendCommand(reContext, "auth %s", password);
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
            free(currentBinding->actionSortedset);
            free(currentBinding->resultsHashset);
            free(currentBinding->messageHashset);
            free(currentBinding->sessionHashset);
            free(currentBinding->partitionHashset);
            free(currentBinding->hashArray);
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
                                    "evalsha %s %d %s %d %d", 
                                    firstBinding->partitionHash, 
                                    1, 
                                    firstBinding->partitionHashset, 
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

unsigned int assertMessageImmediate(void *rulesBinding, 
                                    char *key, 
                                    char *sid, 
                                    char *mid, 
                                    char *message, 
                                    unsigned int actionIndex) {

    binding *bindingContext = (binding*)rulesBinding;
    redisContext *reContext = bindingContext->reContext;
    time_t currentTime = time(NULL);
    functionHash *currentAssertHash = &bindingContext->hashArray[actionIndex];

    int result = redisAppendCommand(reContext, 
                                    "evalsha %s %d %s %s %s %s %s %s %s %ld 0", 
                                    *currentAssertHash, 
                                    3, 
                                    bindingContext->messageHashset, 
                                    bindingContext->actionSortedset, 
                                    bindingContext->sessionHashset, 
                                    key, 
                                    sid, 
                                    mid, 
                                    message, 
                                    currentTime); 
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

    if (reply->type == REDIS_REPLY_INTEGER && reply->integer) {
        freeReplyObject(reply);
        return ERR_NEW_SESSION;
    }
    
    freeReplyObject(reply);    
    return RULES_OK;
}

unsigned int assertFirstMessage(void *rulesBinding, 
                                char *key, 
                                char *sid, 
                                char *mid, 
                                char *message) {

    redisContext *reContext = ((binding*)rulesBinding)->reContext;
    int result = redisAppendCommand(reContext, "multi");
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }

    return assertMessage(rulesBinding, key, sid, mid, message);
}

unsigned int assertMessage(void *rulesBinding, 
                           char *key, 
                           char *sid, 
                           char *mid, 
                           char *message) {

    redisContext *reContext = ((binding*)rulesBinding)->reContext;

    int result = redisAppendCommand(reContext, 
                                    "hsetnx %s %s %s", 
                                    ((binding*)rulesBinding)->messageHashset, 
                                    mid, 
                                    message);
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }

    time_t currentTime = time(NULL);

    result = redisAppendCommand(reContext, 
                                "zadd %s!%s %ld %s", 
                                key, 
                                sid, 
                                currentTime, 
                                mid);
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }
    
    return RULES_OK;
}

unsigned int assertLastMessage(void *rulesBinding, 
                               char *key, 
                               char *sid, 
                               char *mid, 
                               char *message, 
                               int actionIndex, 
                               unsigned int messageCount) {

    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext;
    time_t currentTime = time(NULL);
    functionHash *currentAssertHash = &currentBinding->hashArray[actionIndex];

    unsigned int result = redisAppendCommand(reContext, 
                                             "evalsha %s %d %s %s %s %s %s %s %s %ld 0", 
                                             *currentAssertHash, 
                                             3, 
                                             currentBinding->messageHashset, 
                                             currentBinding->actionSortedset, 
                                             currentBinding->sessionHashset, 
                                             key, 
                                             sid, 
                                             mid, 
                                             message, 
                                             currentTime); 
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }

    result = redisAppendCommand(reContext, "exec");
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }

    redisReply *reply;
    result = RULES_OK;
    for (unsigned short i = 0; i < (messageCount * 2) + 1; ++i) {
        result = redisGetReply(reContext, (void**)&reply);
        if (result != REDIS_OK) {
            result = ERR_REDIS_ERROR;
        } else {
            if (reply->type == REDIS_REPLY_ERROR) {
                result = ERR_REDIS_ERROR;
            }

            if (reply->type == REDIS_REPLY_ARRAY) {
                redisReply *lastReply = reply->element[reply->elements - 1]; 
                if (lastReply->type == REDIS_REPLY_INTEGER && lastReply->integer) {
                    result = ERR_NEW_SESSION;
                }
            }

            freeReplyObject(reply);    
        } 
    }
    
    return result;
}

unsigned int assertTimer(void *rulesBinding, 
                         char *key, 
                         char *sid, 
                         char *mid, 
                         char *timer, 
                         unsigned int actionIndex) {

    binding *bindingContext = (binding*)rulesBinding;
    redisContext *reContext = bindingContext->reContext;
    time_t currentTime = time(NULL);
    functionHash *currentAssertHash = &bindingContext->hashArray[actionIndex];

    int result = redisAppendCommand(reContext, 
                                    "evalsha %s %d %s %s %s %s %s %s %s %ld 0", 
                                    *currentAssertHash, 
                                    3, 
                                    bindingContext->messageHashset, 
                                    bindingContext->actionSortedset, 
                                    bindingContext->sessionHashset, 
                                    key, 
                                    sid, 
                                    mid, 
                                    timer, 
                                    currentTime); 
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }

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
                                        "evalsha %s %d %s %s %s %s %ld", 
                                        currentBinding->dequeueActionHash, 
                                        4, 
                                        currentBinding->sessionHashset, 
                                        currentBinding->messageHashset, 
                                        currentBinding->actionSortedset, 
                                        currentBinding->resultsHashset, 
                                        currentTime); 
        if (result != REDIS_OK) {
            continue;
        }

        result = redisGetReply(reContext, (void**)reply);
        if (result != REDIS_OK) {
            return ERR_REDIS_ERROR;
        }

        if ((*reply)->type == REDIS_REPLY_ERROR) {
            freeReplyObject(reply);
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
                                        "evalsha %s %d %s %ld", 
                                        currentBinding->timersHash, 
                                        1, 
                                        currentBinding->timersSortedset, 
                                        currentTime); 
        if (result != REDIS_OK) {
            continue;
        }

        result = redisGetReply(reContext, (void**)reply);
        if (result != REDIS_OK) {
            return ERR_REDIS_ERROR;
        }

        if ((*reply)->type == REDIS_REPLY_ERROR) {
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

unsigned int negateMessage(void *rulesBinding, 
                           char *key, 
                           char *sid, 
                           char *mid) {

    redisContext *reContext = ((binding*)rulesBinding)->reContext;

    int result = redisAppendCommand(reContext, 
                                    "zrem %s!%s %s", 
                                    key, 
                                    sid, 
                                    mid);
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }

    return RULES_OK;
}

unsigned int storeSession(void *rulesBinding, char *sid, char *state) {
    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext;

    int result = redisAppendCommand(reContext, 
                                    "hset %s %s %s", 
                                    currentBinding->sessionHashset, 
                                    sid, 
                                    state);
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }

    return RULES_OK;
}

unsigned int storeSessionImmediate(void *rulesBinding, char *sid, char *state) {
    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext;

    int result = redisAppendCommand(reContext, 
                                    "hset %s %s %s", 
                                    currentBinding->sessionHashset, 
                                    sid, 
                                    state);
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
    return RULES_OK;
}

unsigned int assertSession(void *rulesBinding, 
                           char *key, 
                           char *sid, 
                           char *state, 
                           unsigned int actionIndex) {

    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext;
    time_t currentTime = time(NULL);
    functionHash *currentAssertHash = &currentBinding->hashArray[actionIndex];

    int result = redisAppendCommand(reContext, 
                                    "evalsha %s %d %s %s %s %s %s %s %ld 1", 
                                    *currentAssertHash, 
                                    2, 
                                    currentBinding->sessionHashset, 
                                    currentBinding->actionSortedset, 
                                    key, 
                                    sid, 
                                    sid, 
                                    state, 
                                    currentTime); 
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }

    return RULES_OK;
}

unsigned int assertSessionImmediate(void *rulesBinding, 
                                    char *key, 
                                    char *sid, 
                                    char *state, 
                                    unsigned int actionIndex) {

    int result = assertSession(rulesBinding, key, sid, state, actionIndex);
    if (result != RULES_OK) {
        return result;
    }

    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext;
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
    return RULES_OK;
}

unsigned int negateSession(void *rulesBinding, char *key, char *sid) {
    redisContext *reContext = ((binding*)rulesBinding)->reContext;

    int result = redisAppendCommand(reContext, 
                                    "zrem %s!%s %s", 
                                    key, 
                                    sid, 
                                    sid);
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }
    
    return RULES_OK;
}

unsigned int removeAction(void *rulesBinding, char *action) {
    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext;   

    int result = redisAppendCommand(reContext, 
                                    "zrem %s %s", 
                                    currentBinding->actionSortedset, 
                                    action);
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }
    
    result = redisAppendCommand(reContext, 
                                "hdel %s %s", 
                                currentBinding->resultsHashset, 
                                action);
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }

    return RULES_OK;
}

unsigned int removeMessage(void *rulesBinding, char *mid) {
    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext;  

    int result = redisAppendCommand(reContext, 
                                    "hdel %s %s", 
                                    currentBinding->messageHashset, 
                                    mid);
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }
    
    return RULES_OK;
}

unsigned int removeTimer(void *rulesBinding, char *timer) {
    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext;  

    int result = redisAppendCommand(reContext, 
                                    "zrem %s %s", 
                                    currentBinding->timersSortedset, 
                                    timer);
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }
    
    return RULES_OK;
}

unsigned int registerTimer(void *rulesBinding, unsigned int duration, char *timer) {
    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext;   
    time_t currentTime = time(NULL);

    int result = redisAppendCommand(reContext, 
                                    "zadd %s %ld %s", 
                                    currentBinding->timersSortedset, 
                                    currentTime + duration, 
                                    timer);
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
    return RULES_OK;
}

unsigned int prepareCommands(void *rulesBinding) {
    redisContext *reContext = ((binding*)rulesBinding)->reContext;   
    int result = redisAppendCommand(reContext, "multi");
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }
    
    return RULES_OK;
}

unsigned int rollbackCommands(void *rulesBinding) {
    redisContext *reContext = ((binding*)rulesBinding)->reContext;   
    int result = redisAppendCommand(reContext, "discard");
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
    return RULES_OK;
}

unsigned int executeCommands(void *rulesBinding, unsigned int commandCount) {
    redisContext *reContext = ((binding*)rulesBinding)->reContext;  
    unsigned int result = redisAppendCommand(reContext, "exec");
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }

    redisReply *reply;
    result = RULES_OK;
    for (unsigned int i = 0; i < commandCount; ++i) {
        result = redisGetReply(reContext, (void**)&reply);
        if (result != REDIS_OK) {
            result = ERR_REDIS_ERROR;
        } else {
            if (reply->type == REDIS_REPLY_ERROR) {
                result = ERR_REDIS_ERROR;
            }

            freeReplyObject(reply);    
        } 
    }
    
    return result;
}

unsigned int getState(void *handle, char *sid, char **state) {
    binding *bindingContext;
    unsigned int result = resolveBinding(handle, sid, (void**)&bindingContext);
    if (result != REDIS_OK) {
        return result;
    }

    redisContext *reContext = bindingContext->reContext; 

    result = redisAppendCommand(reContext, 
                                "hget %s %s", 
                                bindingContext->sessionHashset, 
                                sid);
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

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        return ERR_NEW_SESSION;
    }

    *state = malloc(strlen(reply->str) * sizeof(char));
    if (!*state) {
        return ERR_OUT_OF_MEMORY;
    }
    strcpy(*state, reply->str);
    freeReplyObject(reply); 
    return REDIS_OK;
}

