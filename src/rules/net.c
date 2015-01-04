
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "net.h"
#include "rules.h"
#include "json.h"

static char *createTest(ruleset *tree, expression *expr) {
    char *test = NULL;
    char *comp = NULL;
    char *compStack[32];
    unsigned char compTop = 0;
    unsigned char first = 1;
    asprintf(&test, "");

    for (unsigned short i = 0; i < expr->termsLength; ++i) {
        unsigned int currentNodeOffset = tree->nextPool[expr->t.termsOffset + i];
        node *currentNode = &tree->nodePool[currentNodeOffset];
        if (currentNode->value.a.operator == OP_AND) {
            char *oldTest = test;
            if (first) {
                asprintf(&test, "%s(", test);
            } else {
                asprintf(&test, "%s %s (", test, comp);
            }
            free(oldTest);
            
            compStack[compTop] = comp;
            ++compTop;
            comp = "and";
            first = 1;
        } else if (currentNode->value.a.operator == OP_OR) {
            char *oldTest = test;
            if (first) {
                asprintf(&test, "%s(", test);
            } else {
                asprintf(&test, "%s %s (", test, comp);
            }
            free(oldTest); 
            
            compStack[compTop] = comp;
            ++compTop;
            comp = "or";
            first = 1;           
        } else if (currentNode->value.a.operator == OP_END) {
            --compTop;
            comp = compStack[compTop];
            
            char *oldTest = test;
            asprintf(&test, "%s)", test);
            free(oldTest);            
        } else {
            char *leftProperty = &tree->stringPool[currentNode->nameOffset];
            char *rightProperty = &tree->stringPool[currentNode->value.a.right.value.property.nameOffset];
            char *rightAlias = &tree->stringPool[currentNode->value.a.right.value.property.idOffset];
            char *op;
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

            char *oldTest = test;
            if (first) {
                asprintf(&test, "%smessage[\"%s\"] %s frame[\"%s\"][\"%s\"]", test, leftProperty, op, rightAlias, rightProperty);
                first = 0;
            } else {
                asprintf(&test, "%s %s message[\"%s\"] %s frame[\"%s\"][\"%s\"]", test, comp, leftProperty, op, rightAlias, rightProperty);
            }

            free(oldTest);
        }
    }

    if (first) {
        free(test);
        asprintf(&test, "1");
    }

    return test;
}

static unsigned int loadCommands(ruleset *tree, binding *rulesBinding) {
    redisContext *reContext = rulesBinding->reContext;
    redisReply *reply;
    rulesBinding->hashArray = malloc(tree->actionCount * sizeof(functionHash));
    char *name = &tree->stringPool[tree->nameOffset];
    int nameLength = strlen(name);
    char actionKey[nameLength + 3];
    snprintf(actionKey, nameLength + 3, "%s!a", name);
    char *lua = NULL;
    for (unsigned int i = 0; i < tree->nodeOffset; ++i) {
        char *oldLua;
        node *currentNode = &tree->nodePool[i];
        asprintf(&lua, "");
        if (currentNode->type == NODE_ACTION) {
            char *actionName = &tree->stringPool[currentNode->nameOffset];
            char *actionLastName = strchr(actionName, '!');
            char actionAlias[actionLastName - actionName + 1];
            strncpy(actionAlias, actionName, actionLastName - actionName);
            actionAlias[actionLastName - actionName] = '\0';

            for (unsigned int ii = 0; ii < currentNode->value.c.joinsLength; ++ii) {
                unsigned int currentJoinOffset = tree->nextPool[currentNode->value.c.joinsOffset + ii];
                join *currentJoin = &tree->joinPool[currentJoinOffset];
                
                for (unsigned int iii = 0; iii < currentJoin->expressionsLength; ++iii) {
                    unsigned int expressionOffset = tree->nextPool[currentJoin->expressionsOffset + iii];
                    expression *expr = &tree->expressionPool[expressionOffset];
                    char *currentKey = &tree->stringPool[expr->nameOffset];
                    char *currentAlias = &tree->stringPool[expr->aliasOffset];
                    oldLua = lua;
                    if (iii == 0) {
                        asprintf(&lua, 
"%skeys = {}\n"
"directory = {[\"0\"] = 1}\n"
"reviewers = {}\n"
"results_key = \"%s!%d!r!\" .. sid\n"                   
"keys[1] = \"%s\"\n"
"directory[\"%s\"] = 1\n"
"reviewers[1] = function(message, frame)\n"
"    frame[\"%s\"] = message\n"
"    return true\n"
"end\n",
                                lua,
                                actionName,
                                ii,
                                currentKey, 
                                currentKey,
                                currentAlias);
                    } else {
                        char *test = createTest(tree, expr);
                        asprintf(&lua,
"%skeys[%d] = \"%s\"\n"
"directory[\"%s\"] = %d\n"
"reviewers[%d] = function(message, frame)\n"
"    if %s then\n"
"        frame[\"%s\"] = message\n"
"        return true\n"
"    end\n"
"    return false\n"
"end\n",
                                lua,
                                iii + 1, 
                                currentKey,
                                currentKey,
                                iii + 1,
                                iii + 1,
                                test,
                                currentAlias);
                        free(test);
                    }

                    free(oldLua);
                }

                oldLua = lua;
                asprintf(&lua,
"%sprocess_key(%d)\n",
                        lua,
                        currentJoin->count);
                free(oldLua);
                    
            }

            oldLua = lua;
            asprintf(&lua,
"local key = ARGV[1]\n" 
"local sid = ARGV[2]\n"
"local score = tonumber(ARGV[3])\n"
"local messages_key = \"%s!m!\" .. sid\n"
"local actions_key = \"%s!a\"\n"
"local keys\n"
"local directory\n"
"local reviewers\n"
"local results_key\n"
"local load_frame = function(packed_frame)\n"
"    local frame = cmsgpack.unpack(packed_frame)\n"
"    for name, mid in pairs(frame) do\n"
"        local packed_message = redis.call(\"hget\", messages_key, mid)\n"
"        if packed_message then\n"
"            frame[name] = cmsgpack.unpack(packed_message)\n"
"        else\n"
"            frame = nil\n"
"            break\n"
"        end\n"
"    end\n"
"    return frame\n"
"end\n"
"local save_frame = function(frames_key, frame)\n"
"    local frame_to_pack = {}\n"
"    for name, message in pairs(frame) do\n"
"        frame_to_pack[name] = message[\"id\"]\n"
"    end\n"
"    redis.call(\"rpush\", frames_key, cmsgpack.pack(frame_to_pack))\n"
"end\n"
"local save_result = function(frame)\n"
"    for name, message in pairs(frame) do\n"
"        redis.call(\"hdel\", messages_key, message[\"id\"])\n"
"    end\n"
"    redis.call(\"rpush\", results_key, cmsgpack.pack(frame))\n"
"end\n"
"local process_frame\n"
"process_frame = function(frame, index)\n"
"    local events_key = keys[index] .. \"!e!\" .. sid\n"
"    local packed_message_list_len = redis.call(\"llen\", events_key)\n"
"    for i = 0, packed_message_list_len - 1, 1  do\n"
"        local mid = redis.call(\"lpop\", events_key)\n"
"        local packed_message = redis.call(\"hget\", messages_key, mid)\n"
"        if packed_message then\n"
"            local message = cmsgpack.unpack(packed_message)\n"
"            if reviewers[index](message, frame) then\n"
"                if (index == #reviewers) then\n"
"                    save_result(frame)\n"
"                    return true\n"
"                elseif process_frame(frame, index + 1) then\n"
"                    return true\n"
"                else\n"
"                    save_frame(keys[index + 1] .. \"!f!\" .. sid, frame)\n"
"                end\n"
"            end\n"
"            redis.call(\"rpush\", events_key, mid)\n"
"        end\n"
"    end\n"
"    return false\n"
"end\n"
"local process_key = function(window)\n"
"    local start_index = directory[key]\n"
"    if start_index then\n"
"        local count = 0\n"
"        if start_index == 1 then\n"
"            while process_frame({}, 1) do\n"
"                count = count + 1\n"
"            end\n"
"        else\n"
"            local frames_key = keys[start_index] .. \"!f!\" .. sid\n"
"            local packed_frame_list_len = redis.call(\"llen\", frames_key)\n"
"            for i = 0, packed_frame_list_len - 1, 1  do\n"
"                local packed_frame = redis.call(\"lpop\", frames_key)\n"
"                local frame = load_frame(packed_frame)\n"
"                if frame then\n"
"                    if process_frame(frame, start_index) then\n"
"                        count = 1\n"
"                        break\n"
"                    else\n"
"                        redis.call(\"rpush\", frames_key, packed_frame)\n"
"                    end\n"
"                end\n"
"            end\n"
"        end\n"
"        if count > 0 then\n"
"            local length = redis.call(\"llen\", results_key)\n"
"            local prev_count, prev_remain = math.modf((length - count) / window)\n"
"            local new_count, prev_remain = math.modf(length / window)\n"
"            local diff = new_count - prev_count\n"
"            if diff > 0 then\n"
"                for i = 0, diff - 1, 1 do\n"
"                    redis.call(\"rpush\", actions_key .. \"!\" .. sid, results_key)\n"
"                    redis.call(\"rpush\", actions_key .. \"!\" .. sid, window)\n"
"                end\n"
"                redis.call(\"zadd\", actions_key , score, sid)\n"
"            end\n"
"        end\n"
"    end\n"
"end\n"
"if #ARGV > 3 then\n"
"    local message = {}\n"
"    for index = 4, #ARGV, 2 do\n"
"        message[ARGV[index]] = ARGV[index + 1]\n"
"    end\n"
"    redis.call(\"hsetnx\", messages_key, message[\"id\"], cmsgpack.pack(message))\n"
"    redis.call(\"rpush\", key .. \"!e!\" .. sid, message[\"id\"])\n"
"end\n%s"
"return tonumber(redis.call(\"hsetnx\", \"%s!s\", sid, \"{ \\\"sid\\\":\\\"\" .. sid .. \"\\\"}\"))\n",
                name,
                name,
                lua,
                name);
            
            free(oldLua);
            redisAppendCommand(reContext, "SCRIPT LOAD %s", lua);
            redisGetReply(reContext, (void**)&reply);
            if (reply->type == REDIS_REPLY_ERROR) {
                printf("%s\n", reply->str);
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

    asprintf(&lua, 
"local load_next_frame = function(key, sid)\n"
"    local rule_action_key = redis.call(\"lindex\", key, 0)\n"
"    local count = tonumber(redis.call(\"lindex\", key, 1))\n"
"    local frames = {}\n"
"    for i = 0, count - 1, 1 do\n"
"        local packed_frame = redis.call(\"lindex\", rule_action_key, i)\n"
"        frames[i + 1] = cmsgpack.unpack(packed_frame)\n"
"    end\n"
"    local last_name = string.find(rule_action_key, \"!\") - 1\n"
"    if #frames == 1 then\n"
"        return string.sub(rule_action_key, 1, last_name), frames[1]\n"
"    else\n"
"        return string.sub(rule_action_key, 1, last_name), frames\n"
"    end\n"
"end\n"
"local step = tonumber(ARGV[1])\n"
"local max_score = tonumber(ARGV[2])\n"
"local ruleset_name = \"%s\"\n"
"local action_key = ruleset_name .. \"!a\"\n"
"local current_action = redis.call(\"zrange\", action_key, 0, 0, \"withscores\")\n"
"if #current_action > 0 then\n"
"    if (tonumber(current_action[2]) > (max_score + 5)) then\n"
"        return nil\n"
"    end\n"
"    local sid = current_action[1]\n"
"    local action_name, frame = load_next_frame(action_key .. \"!\" .. sid, sid)\n"
"    redis.call(\"zincrby\", action_key, step, sid)\n"
"    local state = redis.call(\"hget\", ruleset_name .. \"!s\", sid)\n"
"    return {sid, state, cjson.encode({[action_name] = frame})}\n"
"end\n"
"return nil\n", name);
    
    redisAppendCommand(reContext, "SCRIPT LOAD %s", lua);
    redisGetReply(reContext, (void**)&reply);
    if (reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        free(lua);
        return ERR_REDIS_ERROR;
    }

    strncpy(rulesBinding->peekActionHash, reply->str, 40);
    rulesBinding->peekActionHash[40] = '\0';
    freeReplyObject(reply);
    free(lua);

    asprintf(&lua, 
"local delete_frame = function(key, sid)\n"
"    local rule_action_key = redis.call(\"lpop\", key)\n"
"    local count = tonumber(redis.call(\"lpop\", key))\n"
"    for i = 0, count - 1, 1 do\n"
"        redis.call(\"lpop\", rule_action_key .. \"!r!\" .. sid)\n"
"    end\n"
"    return (redis.call(\"llen\", key) > 0)\n"
"end\n"
"local sid = ARGV[1]\n"
"local max_score = tonumber(ARGV[2])\n"
"local ruleset_name = \"%s\"\n"
"local action_key = ruleset_name .. \"!a\"\n"
"if delete_frame(action_key .. \"!\" .. sid, sid) then\n"
"    redis.call(\"zadd\", action_key, max_score, sid)\n"
"else\n"
"    redis.call(\"zrem\", action_key, sid)\n"
"end\n", 
              name);

    redisAppendCommand(reContext, "SCRIPT LOAD %s", lua);
    redisGetReply(reContext, (void**)&reply);
    if (reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        free(lua);
        return ERR_REDIS_ERROR;
    }

    strncpy(rulesBinding->removeActionHash, reply->str, 40);
    rulesBinding->removeActionHash[40] = '\0';
    freeReplyObject(reply);
    free(lua);
    
    asprintf(&lua, 
"local message = {}\n"
"local key = ARGV[1]\n"
"local sid = ARGV[2]\n"
"for index = 3, #ARGV, 2 do\n"
"    message[ARGV[index]] = ARGV[index + 1]\n"
"end\n"
"redis.call(\"hset\", \"%s!m!\" .. sid, message[\"id\"], cmsgpack.pack(message))\n"
"redis.call(\"rpush\", key .. \"!e!\" .. sid, message[\"id\"])\n", 
              name);
    
    redisAppendCommand(reContext, "SCRIPT LOAD %s", lua);
    redisGetReply(reContext, (void**)&reply);
    if (reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        free(lua);
        return ERR_REDIS_ERROR;
    }

    strncpy(rulesBinding->assertMessageHash, reply->str, 40);
    rulesBinding->removeActionHash[40] = '\0';
    freeReplyObject(reply);
    free(lua);

    asprintf(&lua, 
"local partition_key = \"%s!p\"\n"
"local res = redis.call(\"hget\", partition_key, ARGV[1])\n"
"if (not res) then\n"
"   res = redis.call(\"hincrby\", partition_key, \"index\", 1)\n"
"   res = res %% tonumber(ARGV[2])\n"
"   redis.call(\"hset\", partition_key, ARGV[1], res)\n"
"end\n"
"return tonumber(res)\n", 
              name);

    redisAppendCommand(reContext, "SCRIPT LOAD %s", lua); 
    redisGetReply(reContext, (void**)&reply);
    if (reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        return ERR_REDIS_ERROR;
    }

    strncpy(rulesBinding->partitionHash, reply->str, 40);
    rulesBinding->partitionHash[40] = '\0';
    freeReplyObject(reply);
    free(lua);

    asprintf(&lua,
"local timer_key = \"%s!t\"\n"
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
"return 0\n",
              name);

    redisAppendCommand(reContext, "SCRIPT LOAD %s", lua);
    redisGetReply(reContext, (void**)&reply);
    if (reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        return ERR_REDIS_ERROR;
    }

    strncpy(rulesBinding->timersHash, reply->str, 40);
    rulesBinding->timersHash[40] = '\0';
    freeReplyObject(reply);
    free(lua);

    char *sessionHashset = malloc((nameLength + 3) * sizeof(char));
    if (!sessionHashset) {
        return ERR_OUT_OF_MEMORY;
    }

    strncpy(sessionHashset, name, nameLength);
    sessionHashset[nameLength] = '!';
    sessionHashset[nameLength + 1] = 's';
    sessionHashset[nameLength + 2] = '\0';
    rulesBinding->sessionHashset = sessionHashset;

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
            free(currentBinding->timersSortedset);
            free(currentBinding->sessionHashset);
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

unsigned int assertMessageImmediate(void *rulesBinding, 
                                    char *key, 
                                    char *sid, 
                                    char *message, 
                                    jsonProperty *allProperties,
                                    unsigned int propertiesLength,
                                    unsigned int actionIndex) {

    binding *bindingContext = (binding*)rulesBinding;
    redisContext *reContext = bindingContext->reContext;
    functionHash *currentAssertHash = &bindingContext->hashArray[actionIndex];
    time_t currentTime = time(NULL);
    char score[11];
    snprintf(score, 11, "%ld", currentTime);
    char *argv[6 + propertiesLength * 2];
    size_t argvl[6 + propertiesLength * 2];

    argv[0] = "evalsha";
    argvl[0] = 7;
    argv[1] = *currentAssertHash;
    argvl[1] = 40;
    argv[2] = "0";
    argvl[2] = 1;
    argv[3] = key;
    argvl[3] = strlen(key);
    argv[4] = sid;
    argvl[4] = strlen(sid);
    argv[5] = score;
    argvl[5] = 10;

    for (unsigned int i = 0; i < propertiesLength; ++i) {
        argv[6 + i * 2] = message + allProperties[i].nameOffset;
        argvl[6 + i * 2] = allProperties[i].nameLength;
        argv[6 + i * 2 + 1] = message + allProperties[i].valueOffset;
        if (allProperties[i].type == JSON_STRING) {
            argvl[6 + i * 2 + 1] = allProperties[i].valueLength;
        } else {
            argvl[6 + i * 2 + 1] = allProperties[i].valueLength + 1;
        }
    }

    int result = redisAppendCommandArgv(reContext, 6 + propertiesLength * 2, (const char**)argv, argvl); 
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }

    redisReply *reply;
    result = redisGetReply(reContext, (void**)&reply);
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }

    if (reply->type == REDIS_REPLY_ERROR) {
        printf("%s\n", reply->str);
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
                                char *message,
                                jsonProperty *allProperties,
                                unsigned int propertiesLength) {

    redisContext *reContext = ((binding*)rulesBinding)->reContext;
    int result = redisAppendCommand(reContext, "multi");
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }

    return assertMessage(rulesBinding, 
                         key, 
                         sid, 
                         message, 
                         allProperties, 
                         propertiesLength);
}

unsigned int assertMessage(void *rulesBinding, 
                           char *key, 
                           char *sid, 
                           char *message,
                           jsonProperty *allProperties,
                           unsigned int propertiesLength) {

    binding *bindingContext = (binding*)rulesBinding;
    redisContext *reContext = bindingContext->reContext;
    time_t currentTime = time(NULL);
    char score[11];
    snprintf(score, 11, "%ld", currentTime);
    char *argv[5 + propertiesLength * 2];
    size_t argvl[5 + propertiesLength * 2];

    argv[0] = "evalsha";
    argvl[0] = 7;
    argv[1] = bindingContext->assertMessageHash;
    argvl[1] = 40;
    argv[2] = "0";
    argvl[2] = 1;
    argv[3] = key;
    argvl[3] = strlen(key);
    argv[4] = sid;
    argvl[4] = strlen(sid);
    
    for (unsigned int i = 0; i < propertiesLength; ++i) {
        char propertyHash[10];
        snprintf(propertyHash, 10, "%d", allProperties[i].hash);
        argv[5 + i * 2] = message + allProperties[i].nameOffset;
        argvl[5 + i * 2] = allProperties[i].nameLength;
        argv[5 + i * 2 + 1] = message + allProperties[i].valueOffset;
        if (allProperties[i].type == JSON_STRING) {
            argvl[5 + i * 2 + 1] = allProperties[i].valueLength;
        } else {
            argvl[5 + i * 2 + 1] = allProperties[i].valueLength + 1;
        }
    }

    int result = redisAppendCommandArgv(reContext, 5 + propertiesLength * 2, (const char**)argv, argvl); 
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }

    return RULES_OK;
}

unsigned int assertLastMessage(void *rulesBinding, 
                               char *key,
                               char *sid, 
                               char *message,
                               jsonProperty *allProperties,
                               unsigned int propertiesLength,
                               int actionIndex, 
                               unsigned int messageCount) {

    unsigned int result = assertMessage(rulesBinding, 
                                        key, 
                                        sid, 
                                        message, 
                                        allProperties, 
                                        propertiesLength); 
    if (result != RULES_OK) {
        return result;
    }

    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext;
    time_t currentTime = time(NULL);
    functionHash *currentAssertHash = &currentBinding->hashArray[actionIndex];

    result = redisAppendCommand(reContext, 
                                "evalsha %s 0 0 %s %ld", 
                                *currentAssertHash, 
                                sid, 
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
    for (unsigned short i = 0; i < messageCount + 3; ++i) {
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
                                        60,
                                        currentTime); 
        if (result != REDIS_OK) {
            continue;
        }

        result = redisGetReply(reContext, (void**)reply);
        if (result != REDIS_OK) {
            return ERR_REDIS_ERROR;
        }

        if ((*reply)->type == REDIS_REPLY_ERROR) {
            printf("error %s\n", (*reply)->str);
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

unsigned int removeAction(void *rulesBinding, char *sid) {
    binding *bindingContext = (binding*)rulesBinding;
    redisContext *reContext = bindingContext->reContext; 
    time_t currentTime = time(NULL);

    int result = redisAppendCommand(reContext, 
                                    "evalsha %s 0 %s %ld", 
                                    bindingContext->removeActionHash, 
                                    sid, 
                                    currentTime); 
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }

    return RULES_OK;
}

unsigned int assertTimer(void *rulesBinding, 
                         char *key, 
                         char *sid, 
                         char *timer, 
                         jsonProperty *allProperties,
                         unsigned int propertiesLength,
                         unsigned int actionIndex) {

    binding *bindingContext = (binding*)rulesBinding;
    redisContext *reContext = bindingContext->reContext;
    functionHash *currentAssertHash = &bindingContext->hashArray[actionIndex];
    time_t currentTime = time(NULL);
    char score[11];
    snprintf(score, 11, "%ld", currentTime);
    char *argv[6 + propertiesLength * 2];
    size_t argvl[6 + propertiesLength * 2];

    argv[0] = "evalsha";
    argvl[0] = 7;
    argv[1] = *currentAssertHash;
    argvl[1] = 40;
    argv[2] = "0";
    argvl[2] = 1;
    argv[3] = key;
    argvl[3] = strlen(key);
    argv[4] = sid;
    argvl[4] = strlen(sid);
    argv[5] = score;
    argvl[5] = 10;

    for (unsigned int i = 0; i < propertiesLength; ++i) {
        argv[6 + i * 2] = timer + allProperties[i].nameOffset;
        argvl[6 + i * 2] = allProperties[i].nameLength;
        argv[6 + i * 2 + 1] = timer + allProperties[i].valueOffset;
        if (allProperties[i].type == JSON_STRING) {
            argvl[6 + i * 2 + 1] = allProperties[i].valueLength;
        } else {
            argvl[6 + i * 2 + 1] = allProperties[i].valueLength + 1;
        }
    }

    int result = redisAppendCommandArgv(reContext, 6 + propertiesLength * 2, (const char**)argv, argvl); 
    if (result != REDIS_OK) {
        return ERR_REDIS_ERROR;
    }

    return RULES_OK;
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

