
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "net.h"
#include "rules.h"
#include "json.h"

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
            char *op;
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
    }

    return RULES_OK;
}

static unsigned int createTest(ruleset *tree, expression *expr, char **test) {
    char *comp = NULL;
    char *compStack[32];
    unsigned char compTop = 0;
    unsigned char first = 1;
    if (asprintf(test, "") == -1) {
        return ERR_OUT_OF_MEMORY;
    }

    for (unsigned short i = 0; i < expr->termsLength; ++i) {
        unsigned int currentNodeOffset = tree->nextPool[expr->t.termsOffset + i];
        node *currentNode = &tree->nodePool[currentNodeOffset];
        if (currentNode->value.a.operator == OP_AND) {
            char *oldTest = *test;
            if (first) {
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
   
    return RULES_OK;
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
        if (asprintf(&lua, "")  == -1) {
            return ERR_OUT_OF_MEMORY;
        }

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
                        if (expr->not) { 
                            if (asprintf(&lua, 
"%skeys = {}\n"
"notKeys = {}\n"
"directory = {[\"0\"] = 1}\n"
"reviewers = {}\n"
"results_key = \"%s!%d!r!\" .. sid\n"                   
"keys[1] = \"%s\"\n"
"directory[\"%s\"] = 1\n"
"reviewers[1] = function(message, frame, index)\n"
"    if not message then\n"
"        frame[\"$\" .. index] = 1\n"
"        return true\n"
"    end\n"
"    return false\n"
"end\n",
                                         lua,
                                         actionName,
                                         ii,
                                         currentKey,
                                         currentKey)  == -1) {
                                return ERR_OUT_OF_MEMORY;
                            }
                        } else {
                            if (asprintf(&lua, 
"%skeys = {}\n"
"directory = {[\"0\"] = 1}\n"
"reviewers = {}\n"
"results_key = \"%s!%d!r!\" .. sid\n"                   
"keys[1] = \"%s\"\n"
"directory[\"%s\"] = 1\n"
"reviewers[1] = function(message, frame, index)\n"
"    if message then\n"
"        frame[\"%s\"] = message\n"
"        return true\n"
"    end\n"
"    return false\n"
"end\n",
                                         lua,
                                         actionName,
                                         ii,
                                         currentKey,
                                         currentKey,
                                         currentAlias)  == -1) {
                                return ERR_OUT_OF_MEMORY;
                            }
                        }
                    } else {
                        char *test = NULL;
                        unsigned int result = createTest(tree, expr, &test);
                        if (result != RULES_OK) {
                            return result;
                        }

                        if (expr->not) { 
                            if (asprintf(&lua,
"%skeys[%d] = \"%s\"\n"
"directory[\"%s\"] = %d\n"
"reviewers[%d] = function(message, frame, index)\n"
"    if not message or not (%s) then\n"
"        frame[\"$\" .. index] = 1\n"
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
                                         test)  == -1) {
                                return ERR_OUT_OF_MEMORY;
                            }
                        } else {
                            if (asprintf(&lua,
"%skeys[%d] = \"%s\"\n"
"directory[\"%s\"] = %d\n"
"reviewers[%d] = function(message, frame, index)\n"
"    if message and %s then\n"
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
                                         currentAlias)  == -1) {
                                return ERR_OUT_OF_MEMORY;
                            }
                        }
                        free(test);
                    }

                    free(oldLua);
                }

                oldLua = lua;
                if (asprintf(&lua,
"%sprocess_key(message, %d)\n",
                             lua,
                             currentJoin->count)  == -1) {
                    return ERR_OUT_OF_MEMORY;
                }
                free(oldLua);
            }

            oldLua = lua;
            if (asprintf(&lua,
"local key = ARGV[1]\n"
"local sid = ARGV[2]\n"
"local mid = ARGV[3]\n"
"local score = tonumber(ARGV[4])\n"
"local assert_fact = tonumber(ARGV[5])\n"
"local events_hashset = \"%s!e!\" .. sid\n"
"local facts_hashset = \"%s!f!\" .. sid\n"
"local visited_hashset = \"%s!v!\" .. sid\n"
"local actions_key = \"%s!a\"\n"
"local keys\n"
"local directory\n"
"local reviewers\n"
"local results_key\n"
"local validate_frame_for_key = function(frame, index, events_key, messages_key)\n"
"    local packed_message_list_len = redis.call(\"llen\", events_key)\n"
"    for i = 0, packed_message_list_len - 1, 1  do\n"
"        local new_mid = redis.call(\"lpop\", events_key)\n"
"        local packed_message = redis.call(\"hget\", messages_key, new_mid)\n"
"        if packed_message then\n"
"            local message = cmsgpack.unpack(packed_message)\n"
"            if not reviewers[index](message, frame, index) then\n"
"                return false\n"
"            end\n"
"        end\n"
"    end\n"
"    return true\n"
"end\n"
"local validate_frame = function(frame, index)\n"
"    local first_result = validate_frame_for_key(frame, index, keys[index] .. \"!e!\" .. sid, events_hashset)\n"
"    local second_result = validate_frame_for_key(frame, index, keys[index] ..\"!f!\" .. sid, facts_hashset)\n"
"    return first_result and second_result\n"
"end\n"
"local load_frame = function(packed_frame)\n"
"    local frame = cmsgpack.unpack(packed_frame)\n"
"    local indexes = {}\n"
"    for name, new_mid in pairs(frame) do\n"
"        if string.sub(name, 1, 1) == \"$\" then\n"
"            table.insert(indexes, tonumber(string.sub(name, 2)))\n"
"        else\n"
"            local packed_message = redis.call(\"hget\", events_hashset, new_mid)\n"
"            if not packed_message then\n"
"                packed_message = redis.call(\"hget\", facts_hashset, new_mid)\n"
"            end\n"
"            if packed_message then\n"
"                frame[name] = cmsgpack.unpack(packed_message)\n"
"            else\n"
"                return nil\n"
"            end\n"
"        end\n"
"    end\n"
"    for i = 1, #indexes, 1 do\n"
"        if not validate_frame(frame, indexes[i]) then\n"
"            return nil\n"
"        end\n"
"    end\n"
"    return frame\n"
"end\n"
"local save_frame = function(frames_key, frame)\n"
"    local frame_to_pack = {}\n"
"    for name, message in pairs(frame) do\n"
"        if message == 1 then\n"
"            frame_to_pack[name] = 1\n"
"        else\n"
"            frame_to_pack[name] = message[\"id\"]\n"
"        end\n"
"    end\n"
"    redis.call(\"rpush\", frames_key, cmsgpack.pack(frame_to_pack))\n"
"end\n"
"local save_result = function(frame)\n"
"    if assert_fact == 1 then\n"
"        frame[\"$f\"] = 1\n"
"    end\n"
"    redis.call(\"rpush\", results_key, cmsgpack.pack(frame))\n"
"    if assert_fact == 1 then\n"
"        frame[\"$f\"] = nil\n"
"    end\n"
"    for name, message in pairs(frame) do\n"
"        if message ~= 1 and message[\"$f\"] ~= 1 then\n"
"           redis.call(\"hdel\", events_hashset, message[\"id\"])\n"
"           frame[name] = nil\n"
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
"    if reviewers[index](message, frame, index) then\n"
"        if (index == #reviewers) then\n"
"            save_result(frame)\n"
"            return 1\n"
"        else\n"
"            result = process_frame(frame, index + 1, use_facts)\n"
"            if result == 0 or use_facts then\n"
"                save_frame(keys[index + 1] .. \"!c!\" .. sid, frame)\n"
"            end\n"
"        end\n"
"    end\n"
"    return result\n"
"end\n"
"local process_frame_for_key = function(frame, index, events_key, use_facts)\n"
"    local packed_message_list_len = redis.call(\"llen\", events_key)\n"
"    local result = nil\n"
"    local messages_key = events_hashset\n"
"    if use_facts then\n"
"       messages_key = facts_hashset\n"
"    end\n"
"    for i = 0, packed_message_list_len - 1, 1  do\n"
"        local new_mid = redis.call(\"lpop\", events_key)\n"
"        local packed_message = redis.call(\"hget\", messages_key, new_mid)\n"
"        if packed_message then\n"
"            local message = cmsgpack.unpack(packed_message)\n"
"            local count = process_event_and_frame(message, frame, index, use_facts)\n"
"            if not result then\n"
"                result = 0\n"
"            end\n"
"            result = result + count\n"
"            if count == 0 or use_facts then\n"
"                redis.call(\"rpush\", events_key, new_mid)\n"
"            elseif not is_pure_fact(frame, index) then\n"
"                break\n"
"            end\n"
"        end\n"
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
"local process_event = function(message, index, events_key, use_facts)\n"
"    local messages_key = events_hashset\n"
"    if use_facts then\n"
"        messages_key = facts_hashset\n"
"    end\n"
"    if not message then\n"
"        redis.call(\"hdel\", messages_key, mid)\n"
"    end\n"
"    local result = 0\n"
"    if index == 1 then\n"
"        if message then\n"
"            result = process_event_and_frame(message, {}, 1, use_facts)\n"
"        else\n"
"            result = process_frame({}, index, use_facts)\n"
"        end\n"
"    else\n"
"        local frames_key = keys[index] .. \"!c!\" .. sid\n"
"        local packed_frame_list_len = redis.call(\"llen\", frames_key)\n"
"        for i = 0, packed_frame_list_len - 1, 1  do\n"
"            local packed_frame = redis.call(\"lpop\", frames_key)\n"
"            local frame = load_frame(packed_frame)\n"
"            if frame then\n"
"                local count\n"
"                if message then\n"
"                    count = process_event_and_frame(message, frame, index, use_facts)\n"
"                else\n" 
"                    count = process_frame(frame, index, use_facts)\n"
"                end\n"     
"                result = result + count\n"         
"                if count == 0 or use_facts then\n"
"                    redis.call(\"rpush\", frames_key, packed_frame)\n"
"                else\n"
"                    break\n"
"                end\n" 
"            end\n"
"        end\n"
"    end\n"
"    if message and (result == 0 or use_facts) then\n"
"        redis.call(\"rpush\", events_key, mid)\n"
"        redis.call(\"hsetnx\", messages_key, mid, cmsgpack.pack(message))\n"
"    end\n"
"    return result\n"
"end\n"
"local process_key = function(message, window)\n"
"    local index = directory[key]\n"
"    if index then\n"
"        local count = 0\n"
"        if assert_fact == 0 then\n"
"            count = process_event(message, index, keys[index] .. \"!e!\" .. sid, false)\n"
"        else\n"
"            count = process_event(message, index, keys[index] .. \"!f!\" .. sid, true)\n"
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
"local message = nil\n"
"if #ARGV > 5 then\n"
"    message = {}\n"
"    for index = 6, #ARGV, 3 do\n"
"        if ARGV[index + 2] == \"1\" then\n"
"            message[ARGV[index]] = ARGV[index + 1]\n"
"        elseif ARGV[index + 2] == \"2\" or  ARGV[index + 2] == \"3\" then\n"
"            message[ARGV[index]] = tonumber(ARGV[index + 1])\n"
"        elseif ARGV[index + 2] == \"4\" then\n"
"            message[ARGV[index]] = toboolean(ARGV[index + 1])\n"
"        end\n"
"    end\n"
"    if assert_fact == 1 then\n"
"        message[\"$f\"] = 1\n"
"    end\n"
"end\n"
"if redis.call(\"hsetnx\", visited_hashset, mid, 1) == 0 then\n"
"    if assert_fact == 0 then\n"
"        if not redis.call(\"hget\", events_hashset, mid) then\n"
"            return false\n"
"        end\n"
"    else\n"
"        if not redis.call(\"hget\", facts_hashset, mid) then\n"
"            return false\n"
"        end\n"
"    end\n"
"end\n%s"
"return true\n",
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

            functionHash *currentAssertHash = &rulesBinding->hashArray[currentNode->value.c.index];
            strncpy(*currentAssertHash, reply->str, HASH_LENGTH);
            (*currentAssertHash)[HASH_LENGTH] = '\0';
            freeReplyObject(reply);
            free(lua);
        }
    }

    if (asprintf(&lua, 
"local facts_key = \"%s!f!\"\n"
"local action_key = \"%s!a\"\n"
"local state_key = \"%s!s\"\n"
"local review_frame = function(frame, sid)\n"
"    local facts_hashset = facts_key .. sid\n"
"    if frame[\"$f\"] ~= 1 then\n"
"         return true\n"
"    else\n"
"        frame[\"$f\"] = nil\n"
"        for name, message in pairs(frame) do\n"
"            if message ~= 1 then\n"
"                if message[\"$f\"] == 1 and (not redis.call(\"hget\", facts_hashset, message[\"id\"])) then\n"
"                    return false\n"
"                end\n"
"            end\n"
"        end\n"
"        return true\n"
"    end\n"
"end\n"
"local load_frame_from_rule = function(rule_action_key, count, sid)\n"
"    local frames = {}\n"
"    local packed_frames = {}\n"
"    while count > 0 do\n"
"        local packed_frame = redis.call(\"lpop\", rule_action_key)\n"
"        if not packed_frame then\n"
"            break\n"
"        else\n"
"            local frame = cmsgpack.unpack(packed_frame)\n"
"            if review_frame(frame, sid) then\n"
"                table.insert(frames, frame)\n"
"                table.insert(packed_frames, packed_frame)\n"
"                count = count - 1\n"
"            end\n"
"        end\n"
"    end\n"
"    for i = #packed_frames, 1, -1 do\n"
"       redis.call(\"lpush\", rule_action_key, packed_frames[i])\n"
"    end\n"
"    if count ~= 0 then\n"
"        return nil, nil\n"
"    else\n"
"        local last_name = string.find(rule_action_key, \"!\") - 1\n"
"        if #frames == 1 then\n"
"            return string.sub(rule_action_key, 1, last_name), frames[1]\n"
"        else\n"
"            return string.sub(rule_action_key, 1, last_name), frames\n"
"        end\n"
"    end\n"
"end\n"
"local load_frame_from_sid = function(sid)\n"
"    local action_list = action_key .. \"!\" .. sid\n"
"    local rule_action_key = redis.call(\"lindex\", action_list, 0)\n"
"    local count = tonumber(redis.call(\"lindex\", action_list, 1))\n"
"    local name, frame = load_frame_from_rule(rule_action_key, count, sid)\n"
"    while not frame do\n"
"        redis.call(\"lpop\", action_list)\n"
"        redis.call(\"lpop\", action_list)\n"
"        rule_action_key = redis.call(\"lindex\", action_list, 0)\n"
"        if not rule_action_key then\n"
"            return nil, nil\n"
"        end\n"
"        count = tonumber(redis.call(\"lindex\", action_list, 1))\n"
"        name, frame = load_frame_from_rule(rule_action_key, count, sid)\n"
"    end\n"
"    return name, frame\n"
"end\n"
"local load_frame = function(max_score)\n"
"    local current_action = redis.call(\"zrange\", action_key, 0, 0, \"withscores\")\n"
"    if #current_action == 0 or (tonumber(current_action[2]) > (max_score + 5)) then\n"
"        return nil, nil, nil\n"
"    end\n"
"    local sid = current_action[1]\n"
"    local name, frame = load_frame_from_sid(sid)\n"
"    while not frame do\n"
"        redis.call(\"zremrangebyrank\", action_key, 0, 0)\n"
"        current_action = redis.call(\"zrange\", action_key, 0, 0, \"withscores\")\n"
"        if #current_action == 0 or (tonumber(current_action[2]) > (max_score + 5)) then\n"
"            return nil, nil, nil\n"
"        end\n"
"        sid = current_action[1]\n"
"        name, frame = load_frame_from_sid(sid)\n"
"    end\n"
"    return sid, name, frame\n"
"end\n"
"local sid, action_name, frame = load_frame(tonumber(ARGV[2]))\n"
"if not frame then\n"
"    return nil\n"
"else\n"
"    redis.call(\"zincrby\", action_key, tonumber(ARGV[1]), sid)\n"
"    local state_fact = redis.call(\"hget\", state_key, sid .. \"!f\")\n"
"    local state = redis.call(\"hget\", state_key, sid)\n"
"    return {sid, state_fact, state, cjson.encode({[action_name] = frame})}\n"
"end\n",
                name,
                name,
                name)  == -1) {
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

    strncpy(rulesBinding->peekActionHash, reply->str, 40);
    rulesBinding->peekActionHash[40] = '\0';
    freeReplyObject(reply);
    free(lua);

    if (asprintf(&lua, 
"local key = ARGV[1]\n"
"local sid = ARGV[2]\n"
"local assert_fact = tonumber(ARGV[3])\n"
"local events_hashset = \"%s!e!\" .. sid\n"
"local facts_hashset = \"%s!f!\" .. sid\n"
"local visited_hashset = \"%s!v!\" .. sid\n"
"local message = {}\n"
"for index = 4, #ARGV, 3 do\n"
"    if ARGV[index + 2] == \"1\" then\n"
"        message[ARGV[index]] = ARGV[index + 1]\n"
"    elseif ARGV[index + 2] == \"2\" or  ARGV[index + 2] == \"3\" then\n"
"        message[ARGV[index]] = tonumber(ARGV[index + 1])\n"
"    elseif ARGV[index + 2] == \"4\" then\n"
"        message[ARGV[index]] = toboolean(ARGV[index + 1])\n"
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
"if assert_fact == 1 then\n"
"    message[\"$f\"] = 1\n"
"    redis.call(\"rpush\", key .. \"!f!\" .. sid, mid)\n"
"    redis.call(\"hsetnx\", facts_hashset, mid, cmsgpack.pack(message))\n"
"else\n"
"    redis.call(\"rpush\", key .. \"!e!\" .. sid, mid)\n"
"    redis.call(\"hsetnx\", events_hashset, mid, cmsgpack.pack(message))\n"
"end\n",
                name,
                name,
                name)  == -1) {
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

    strncpy(rulesBinding->addMessageHash, reply->str, 40);
    rulesBinding->addMessageHash[40] = '\0';
    freeReplyObject(reply);
    free(lua);

    if (asprintf(&lua, 
"local delete_frame = function(key)\n"
"    local rule_action_key = redis.call(\"lpop\", key)\n"
"    local count = redis.call(\"lpop\", key)\n"
"    for i = 0, count - 1, 1 do\n"
"        redis.call(\"lpop\", rule_action_key)\n"
"    end\n"
"    return (redis.call(\"llen\", key) > 0)\n"
"end\n"
"local sid = ARGV[1]\n"
"local max_score = tonumber(ARGV[2])\n"
"local action_key = \"%s!a\"\n"
"if delete_frame(action_key .. \"!\" .. sid) then\n"
"    redis.call(\"zadd\", action_key, max_score, sid)\n"
"else\n"
"    redis.call(\"zrem\", action_key, sid)\n"
"end\n", name)  == -1) {
        return ERR_OUT_OF_MEMORY;
    }

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
        freeReplyObject(reply);
        return ERR_REDIS_ERROR;
    }

    strncpy(rulesBinding->partitionHash, reply->str, 40);
    rulesBinding->partitionHash[40] = '\0';
    freeReplyObject(reply);
    free(lua);

    if (asprintf(&lua,
"local timer_key = \"%s!t\"\n"
"local timestamp = tonumber(ARGV[1])\n"
"local res = redis.call(\"zrangebyscore\", timer_key, 0, timestamp)\n"
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
            free(currentBinding->factsHashset);
            free(currentBinding->eventsHashset);
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

unsigned int formatEvalMessage(void *rulesBinding, 
                               char *key, 
                               char *sid, 
                               char *mid,
                               char *message, 
                               jsonProperty *allProperties,
                               unsigned int propertiesLength,
                               unsigned int actionIndex,
                               unsigned char assertFact,
                               char **command) {
    binding *bindingContext = (binding*)rulesBinding;
    functionHash *currentAssertHash = &bindingContext->hashArray[actionIndex];
    time_t currentTime = time(NULL);
    char score[11];
    snprintf(score, 11, "%ld", currentTime);
    char *argv[8 + propertiesLength * 3];
    size_t argvl[8 + propertiesLength * 3];

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
    argv[5] = mid;
    argvl[5] = strlen(mid);
    argv[6] = score;
    argvl[6] = 10;
    argv[7] = assertFact ? "1" : "0";
    argvl[7] = 1;

    for (unsigned int i = 0; i < propertiesLength; ++i) {
        argv[8 + i * 3] = message + allProperties[i].nameOffset;
        argvl[8 + i * 3] = allProperties[i].nameLength;
        argv[8 + i * 3 + 1] = message + allProperties[i].valueOffset;
        if (allProperties[i].type == JSON_STRING) {
            argvl[8 + i * 3 + 1] = allProperties[i].valueLength;
        } else {
            argvl[8 + i * 3 + 1] = allProperties[i].valueLength + 1;
        }

        switch(allProperties[i].type) {
            case JSON_STRING:
                argv[8 + i * 3 + 2] = "1";
                break;
            case JSON_INT:
                argv[8 + i * 3 + 2] = "2";
                break;
            case JSON_DOUBLE:
                argv[8 + i * 3 + 2] = "3";
                break;
            case JSON_BOOL:
                argv[8 + i * 3 + 2] = "4";
                break;
        }
        argvl[8 + i * 3 + 2] = 1; 
    }

    int result = redisFormatCommandArgv(command, 8 + propertiesLength * 3, (const char**)argv, argvl); 
    if (result == 0) {
        return ERR_OUT_OF_MEMORY;
    }
    return RULES_OK;
}

unsigned int formatStoreMessage(void *rulesBinding, 
                                char *key, 
                                char *sid, 
                                char *message, 
                                jsonProperty *allProperties,
                                unsigned int propertiesLength,
                                unsigned char storeFact,
                                char **command) {
    binding *bindingContext = (binding*)rulesBinding;
    char *argv[6 + propertiesLength * 3];
    size_t argvl[6 + propertiesLength * 3];

    argv[0] = "evalsha";
    argvl[0] = 7;
    argv[1] = bindingContext->addMessageHash;
    argvl[1] = 40;
    argv[2] = "0";
    argvl[2] = 1;
    argv[3] = key;
    argvl[3] = strlen(key);
    argv[4] = sid;
    argvl[4] = strlen(sid);
    argv[5] = assertFact ? "1" : "0";
    argvl[5] = 1;

    for (unsigned int i = 0; i < propertiesLength; ++i) {
        argv[6 + i * 3] = message + allProperties[i].nameOffset;
        argvl[6 + i * 3] = allProperties[i].nameLength;
        argv[6 + i * 3 + 1] = message + allProperties[i].valueOffset;
        if (allProperties[i].type == JSON_STRING) {
            argvl[6 + i * 3 + 1] = allProperties[i].valueLength;
        } else {
            argvl[6 + i * 3 + 1] = allProperties[i].valueLength + 1;
        }

        switch(allProperties[i].type) {
            case JSON_STRING:
                argv[6 + i * 3 + 2] = "1";
                break;
            case JSON_INT:
                argv[6 + i * 3 + 2] = "2";
                break;
            case JSON_DOUBLE:
                argv[6 + i * 3 + 2] = "3";
                break;
            case JSON_BOOL:
                argv[6 + i * 3 + 2] = "4";
                break;
        }
        argvl[6 + i * 3 + 2] = 1; 
    }

    int result = redisFormatCommandArgv(command, 6 + propertiesLength * 3, (const char**)argv, argvl); 
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

unsigned int executeBatch(void *rulesBinding,
                          char **commands,
                          unsigned short commandCount) {
    if (commandCount == 0) {
        return RULES_OK;
    }

    unsigned int result;
    unsigned short replyCount = commandCount;
    binding *currentBinding = (binding*)rulesBinding;
    redisContext *reContext = currentBinding->reContext;
    if (commandCount > 1) {
        ++replyCount;
        result = redisAppendCommand(reContext, "multi");
        if (result != REDIS_OK) {
            for (unsigned short i = 0; i < commandCount; ++i) {
                free(commands[i]);
            }

            return ERR_REDIS_ERROR;
        }
    }

    for (unsigned short i = 0; i < commandCount; ++i) {
        sds newbuf;
        newbuf = sdscatlen(reContext->obuf, commands[i], strlen(commands[i]));
        if (newbuf == NULL) {
            return ERR_OUT_OF_MEMORY;
        }

        reContext->obuf = newbuf;
        free(commands[i]);
    }

    if (commandCount > 1) {
        ++replyCount;
        unsigned int result = redisAppendCommand(reContext, "exec");
        if (result != REDIS_OK) {
            return ERR_REDIS_ERROR;
        }
    }

    redisReply *reply;
    for (unsigned short i = 0; i < replyCount; ++i) {
        result = redisGetReply(reContext, (void**)&reply);
        if (result != REDIS_OK) {
            result = ERR_REDIS_ERROR;
        } else {
            if (reply->type == REDIS_REPLY_ERROR) {
                printf("%s\n", reply->str);
                result = ERR_REDIS_ERROR;
            }

            freeReplyObject(reply);    
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
            printf("%s\n", (*reply)->str);
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


