#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "rules.h"
#include "json.h"
#include "net.h"

#define MAX_STATE_NODES 8
#define MAX_MESSAGE_NODES 2
#define MAX_LEFT_FRAME_NODES 8
#define MAX_RIGHT_FRAME_NODES 8

// The first node is never used as it corresponds to UNDEFINED_HASH_OFFSET
#define INIT(type, pool, length) do { \
    pool.content = malloc(length * sizeof(type)); \
    if (!pool.content) { \
        return ERR_OUT_OF_MEMORY; \
    } \
    pool.contentLength = length; \
    for (unsigned int i = 0; i < length; ++ i) { \
        ((type *)pool.content)[i].nextOffset = i + 1; \
        ((type *)pool.content)[i].prevOffset = i - 1; \
    } \
    ((type *)pool.content)[length - 1].nextOffset = UNDEFINED_HASH_OFFSET; \
    ((type *)pool.content)[0].prevOffset = UNDEFINED_HASH_OFFSET; \
    pool.freeOffset = 1; \
} while(0)

#define GET(type, index, max, pool, nodeHash, valueOffset) do { \
    valueOffset = index[nodeHash % max]; \
    while (valueOffset != UNDEFINED_HASH_OFFSET) { \
        type *value = &((type *)pool.content)[valueOffset]; \
        if (value->hash != nodeHash) { \
            valueOffset = value->nextOffset; \
        } else { \
            break; \
        } \
    } \
} while(0)

#define NEW(type, pool, valueOffset) do { \
    valueOffset = pool.freeOffset; \
    type *value = &((type *)pool.content)[valueOffset]; \
    if (value->nextOffset == UNDEFINED_HASH_OFFSET) { \
        pool.content = realloc(pool.content, (pool.contentLength * 2) * sizeof(type)); \
        if (!pool.content) { \
            return ERR_OUT_OF_MEMORY; \
        } \
        for (unsigned int i = pool.contentLength; i < pool.contentLength * 2; ++ i) { \
            ((type *)pool.content)[i].nextOffset = i + 1; \
            ((type *)pool.content)[i].prevOffset = i - 1; \
        } \
        value->nextOffset = pool.contentLength; \
        ((type *)pool.content)[pool.contentLength].prevOffset = valueOffset; \
        pool.contentLength *= 2; \
        ((type *)pool.content)[pool.contentLength - 1].nextOffset = UNDEFINED_HASH_OFFSET; \
        printf("new length %d\n", pool.contentLength); \
    } \
    ((type *)pool.content)[value->nextOffset].prevOffset = UNDEFINED_HASH_OFFSET; \
    pool.freeOffset = value->nextOffset; \
    value->nextOffset = UNDEFINED_HASH_OFFSET; \
    value->prevOffset = UNDEFINED_HASH_OFFSET; \
} while(0)

#define SET(type, index, max, pool, nodeHash, valueOffset)  do { \
    type *value = &((type *)pool.content)[valueOffset]; \
    value->hash = nodeHash; \
    value->prevOffset = UNDEFINED_HASH_OFFSET; \
    value->nextOffset = index[nodeHash % max]; \
    index[nodeHash % max] = valueOffset; \
    if (value->nextOffset != UNDEFINED_HASH_OFFSET) { \
        ((type *)pool.content)[value->nextOffset].prevOffset = valueOffset; \
    } \
} while(0)

#define DELETE(type, index, max, pool, hash) do { \
} while (0)

unsigned int fnv1Hash32(char *str, unsigned int length) {
    unsigned int hash = FNV_32_OFFSET_BASIS;
    for(unsigned int i = 0; i < length; str++, i++) {
        hash ^= (*str);
        hash *= FNV_32_PRIME;
    }
    return hash;
}

unsigned int getHash(char *sid, char *key) {
    unsigned int fullKeyLength = strlen(sid) + strlen(key) + 2;
#ifdef _WIN32
    char *fullKey = (char *)_alloca(sizeof(char)*(fullKeyLength));
    sprintf_s(fullKey, sizeof(char)*(fullKeyLength), "%s!%s", sid, key);
#else
    char fullKey[fullKeyLength];
    snprintf(fullKey, sizeof(char)*(fullKeyLength), "%s!%s", sid, key);
#endif
    return fnv1Hash32(fullKey, fullKeyLength - 1);
}

unsigned int initStatePool(void *tree) {
    INIT(stateNode, ((ruleset*)tree)->statePool, MAX_STATE_NODES);
    return RULES_OK;
}

unsigned int getMessageFromFrame(stateNode *state,
                                 messageFrame *messages,
                                 unsigned int hash,
                                 jsonObject **message) {
    unsigned short size = 0;
    unsigned short index = hash % MAX_MESSAGE_FRAMES;
    unsigned int messageNodeOffset = UNDEFINED_HASH_OFFSET;
    while (messages[index].hash && messageNodeOffset == UNDEFINED_HASH_OFFSET && size < MAX_MESSAGE_FRAMES) {
        if (messages[index].hash == hash) {
            messageNodeOffset = messages[index].messageNodeOffset;
        }
        ++size;
        index = (index + 1) % MAX_MESSAGE_FRAMES;
    }

    if (messageNodeOffset == UNDEFINED_HASH_OFFSET) {
        return ERR_MESSAGE_NOT_FOUND;
    }        

    messageNode *node = MESSAGE_NODE(state, messageNodeOffset);
    *message = &node->jo;
    return RULES_OK;
}

unsigned int setMessageInFrame(messageFrame *messages,
                               unsigned int hash, 
                               unsigned int messageNodeOffset) {
    unsigned short size = 0;
    unsigned short index = hash % MAX_MESSAGE_FRAMES;
    while (messages[index].hash) {
        index = (index + 1) % MAX_MESSAGE_FRAMES;
        ++size;
        if (size == MAX_MESSAGE_FRAMES) {
            return ERR_MAX_MESSAGES_IN_FRAME;
        }
    }
    messages[index].hash = hash;
    messages[index].messageNodeOffset = messageNodeOffset;
    return RULES_OK;   
}

unsigned int getLeftFrame(stateNode *state,
                          unsigned int index, 
                          unsigned int hash, 
                          leftFrameNode **node) {
    unsigned int valueOffset;
    GET(leftFrameNode, 
        state->betaState[index].leftFrameIndex, 
        MAX_LEFT_FRAME_INDEX_LENGTH, 
        state->betaState[index].leftFramePool, 
        hash, 
        valueOffset);
    if (valueOffset == UNDEFINED_HASH_OFFSET) {
        return ERR_FRAME_NOT_FOUND;
    }

    *node = LEFT_FRAME_NODE(state, index, valueOffset);
    return RULES_OK;
}

unsigned int setLeftFrame(stateNode *state, 
                          unsigned int index, 
                          unsigned int hash, 
                          unsigned int valueOffset) {
    SET(leftFrameNode, 
        state->betaState[index].leftFrameIndex, 
        MAX_LEFT_FRAME_INDEX_LENGTH, 
        state->betaState[index].leftFramePool, 
        hash, 
        valueOffset);
    return RULES_OK;
}

unsigned int createLeftFrame(stateNode *state,
                             unsigned int index, 
                             unsigned int *valueOffset,
                             leftFrameNode **node) {
    NEW(leftFrameNode, 
        state->betaState[index].leftFramePool, 
        *valueOffset);
    *node = LEFT_FRAME_NODE(state, index, *valueOffset);
    memset((*node)->messages, 0, MAX_MESSAGE_FRAMES * sizeof(messageFrame));
    return RULES_OK;
}

unsigned int cloneLeftFrame(stateNode *state,
                            unsigned int index, 
                            leftFrameNode *oldNode,                        
                            unsigned int *newValueOffset,
                            leftFrameNode **newNode) {
    NEW(leftFrameNode, 
        state->betaState[index].leftFramePool, 
        *newValueOffset);
    *newNode = LEFT_FRAME_NODE(state, index, *newValueOffset);
    memcpy((*newNode)->messages, oldNode->messages, MAX_MESSAGE_FRAMES * sizeof(messageFrame));
    return RULES_OK;
}

unsigned int getRightFrame(stateNode *state,
                           unsigned int index, 
                           unsigned int hash, 
                           rightFrameNode **node) {
    unsigned int valueOffset;
    GET(rightFrameNode, 
        state->betaState[index].rightFrameIndex, 
        MAX_RIGHT_FRAME_INDEX_LENGTH, 
        state->betaState[index].rightFramePool, 
        hash, 
        valueOffset);
    if (valueOffset == UNDEFINED_HASH_OFFSET) {
        return ERR_FRAME_NOT_FOUND;
    }

    *node = RIGHT_FRAME_NODE(state, index, valueOffset);
    return RULES_OK;
}

unsigned int setRightFrame(stateNode *state,
                           unsigned int index, 
                           unsigned int hash, 
                           unsigned int valueOffset) {
    SET(rightFrameNode, 
        state->betaState[index].rightFrameIndex, 
        MAX_RIGHT_FRAME_INDEX_LENGTH, 
        state->betaState[index].rightFramePool, 
        hash, 
        valueOffset);
    return RULES_OK;
}

unsigned int createRightFrame(stateNode *state,
                              unsigned int index,  
                              unsigned int *valueOffset,
                              rightFrameNode **node) {
    NEW(rightFrameNode, 
        state->betaState[index].rightFramePool, 
        *valueOffset);
    *node = RIGHT_FRAME_NODE(state, index, *valueOffset);
    return RULES_OK;
}

unsigned int storeMessage(stateNode *state,
                          char *mid,
                          jsonObject *message,
                          unsigned int *valueOffset) {
    unsigned int hash = fnv1Hash32(mid, strlen(mid));
    NEW(messageNode, state->messagePool, *valueOffset);
    SET(messageNode, state->messageIndex, MAX_MESSAGE_INDEX_LENGTH, state->messagePool, hash, *valueOffset);
    messageNode *node = MESSAGE_NODE(state, *valueOffset);
    memcpy(&node->jo, message, sizeof(jsonObject));
    unsigned int messageLength = (strlen(message->content) + 1) * sizeof(char);
    node->jo.content = malloc(messageLength);
    if (!node->jo.content) {
        return ERR_OUT_OF_MEMORY;
    }
    memcpy(node->jo.content, message->content, messageLength);
    unsigned int currentOffset = *valueOffset;
    while (currentOffset) {
        node = MESSAGE_NODE(state, currentOffset);
        printf("stored %d, %d, %d, %s\n", currentOffset, node->prevOffset, node->nextOffset, node->jo.content);  
        currentOffset =  node->nextOffset;
    }


    return RULES_OK;
}

unsigned int ensureStateNode(void *tree, 
                             char *sid, 
                             stateNode **state) {  
    printf("ensure state\n");
    unsigned int sidHash = fnv1Hash32(sid, strlen(sid));
    unsigned int nodeOffset;
    GET(stateNode, ((ruleset*)tree)->stateIndex, MAX_STATE_INDEX_LENGTH, ((ruleset*)tree)->statePool, sidHash, nodeOffset);
    if (nodeOffset != UNDEFINED_HASH_OFFSET) {
        *state = STATE_NODE(tree, nodeOffset); 
    } else {
        printf("new state\n");
        NEW(stateNode, ((ruleset*)tree)->statePool, nodeOffset);
        SET(stateNode, ((ruleset*)tree)->stateIndex, MAX_STATE_INDEX_LENGTH, ((ruleset*)tree)->statePool, sidHash, nodeOffset);
        *state = STATE_NODE(tree, nodeOffset); 

        stateNode *node = *state;
        unsigned int result = getBindingIndex(tree, sidHash, &node->bindingIndex);
        if (result != RULES_OK) {
            return result;
        }

        INIT(messageNode, node->messagePool, MAX_MESSAGE_NODES);
        memset(node->messageIndex, 0, MAX_MESSAGE_INDEX_LENGTH * sizeof(unsigned int));
        
        node->betaState = malloc(((ruleset*)tree)->betaCount * sizeof(betaStateNode));
        for (unsigned int i = 0; i < ((ruleset*)tree)->betaCount; ++i) {
            betaStateNode *betaNode = &node->betaState[i];
            INIT(leftFrameNode, betaNode->leftFramePool, MAX_LEFT_FRAME_NODES);
            memset(betaNode->leftFrameIndex, 0, MAX_LEFT_FRAME_INDEX_LENGTH * sizeof(unsigned int));
            
            INIT(rightFrameNode, betaNode->rightFramePool, MAX_RIGHT_FRAME_NODES);
            memset(betaNode->rightFrameIndex, 0, MAX_RIGHT_FRAME_INDEX_LENGTH * sizeof(unsigned int));
        }

        node->actionState = malloc(((ruleset*)tree)->actionCount * sizeof(actionStateNode));
        for (unsigned int i = 0; i < ((ruleset*)tree)->actionCount; ++i) {
            actionStateNode *actionNode = &node->actionState[i];
            INIT(leftFrameNode, actionNode->resultPool, MAX_LEFT_FRAME_NODES);
        }        
    }
    
    return RULES_OK;
}

static void insertSortProperties(jsonObject *jo, jsonProperty **properties) {
    for (unsigned short i = 1; i < jo->propertiesLength; ++i) {
        unsigned short ii = i; 
        while (properties[ii]->hash < properties[ii - 1]->hash) {
            jsonProperty *temp = properties[ii];
            properties[ii] = properties[ii - 1];
            properties[ii - 1] = temp;
            --ii;
        }
    }
}

static void radixSortProperties(jsonObject *jo, jsonProperty **properties) {
    unsigned char counts[43];
    memset(counts, 0, 43 * sizeof(char));

    for (unsigned char i = 0; i < jo->propertiesLength; ++i) {
        unsigned char mostSignificant = jo->properties[i].hash / 100000000;
        ++counts[mostSignificant];
    }

    unsigned char previousCount = 0;
    for (unsigned char i = 0; i < 43; ++i) {
        unsigned char nextCount = counts[i] + previousCount;
        counts[i] = previousCount;
        previousCount = nextCount;
    }

    for (unsigned char i = 0; i < jo->propertiesLength; ++i) {
        unsigned char mostSignificant = jo->properties[i].hash / 100000000;
        properties[counts[mostSignificant]] = &jo->properties[i];
        ++counts[mostSignificant];
    }
}

static void calculateId(jsonObject *jo) {

#ifdef _WIN32
    jsonProperty **properties = (jsonProperty *)_alloca(sizeof(jsonProperty *) * (jo->propertiesLength));
#else
    jsonProperty *properties[jo->propertiesLength];
#endif

    radixSortProperties(jo, properties);
    insertSortProperties(jo, properties);

    unsigned long long hash = FNV_64_OFFSET_BASIS;
    for (unsigned short i = 0; i < jo->propertiesLength; ++i) {
        jsonProperty *property = properties[i];
        for (unsigned short ii = 0; ii < property->nameLength; ++ii) {
            hash ^= jo->content[property->nameOffset + ii];
            hash *= FNV_64_PRIME;
        }

        unsigned short valueLength = property->valueLength;
        if (property->type != JSON_STRING) {
            ++valueLength;
        }

        for (unsigned short ii = 0; ii < valueLength; ++ii) {
            hash ^= jo->content[property->valueOffset + ii];
            hash *= FNV_64_PRIME;
        }
    }

#ifdef _WIN32
    sprintf_s(jo->idBuffer, sizeof(char) * ID_BUFFER_LENGTH, "$%020llu", hash); 
#else
    snprintf(jo->idBuffer, sizeof(char) * ID_BUFFER_LENGTH, "$%020llu", hash);
#endif
    jo->properties[jo->idIndex].valueLength = 20;
}

static unsigned int fixupIds(jsonObject *jo, char generateId) {
    jsonProperty *property;
    // id and sid are coerced to strings
    // to avoid unnecessary conversions
    if (jo->sidIndex != UNDEFINED_INDEX && jo->properties[jo->sidIndex].type != JSON_NIL) {
        //coerce value to string
        property = &jo->properties[jo->sidIndex];
        if (property->type != JSON_STRING) {
            ++property->valueLength;
        }

        property->type = JSON_STRING;
    } else {
        property = &jo->properties[jo->propertiesLength];
        jo->sidIndex = jo->propertiesLength;
        ++jo->propertiesLength;
        if (jo->propertiesLength == MAX_OBJECT_PROPERTIES) {
            return ERR_EVENT_MAX_PROPERTIES;
        }

        strncpy(jo->sidBuffer, "0", 1);
        property->hash = HASH_SID;
        property->valueOffset = 0;
        property->valueLength = 1;
        property->nameLength = 0;
        property->nameOffset = 0;
        property->type = JSON_STRING;
    } 

    if (jo->idIndex != UNDEFINED_INDEX && jo->properties[jo->idIndex].type != JSON_NIL) {
        //coerce value to string
        property = &jo->properties[jo->idIndex];
        if (property->type != JSON_STRING) {
            ++property->valueLength;
        }

        property->type = JSON_STRING;
    } else {
        property = &jo->properties[jo->propertiesLength];
        jo->idIndex = jo->propertiesLength;
        ++jo->propertiesLength;
        if (jo->propertiesLength == MAX_OBJECT_PROPERTIES) {
            return ERR_EVENT_MAX_PROPERTIES;
        }

        jo->idBuffer[0] = 0;
        property->hash = HASH_ID;
        property->valueOffset = 0;
        property->valueLength = 0;
        property->nameLength = 0;
        property->nameOffset = 0;
        property->type = JSON_STRING;
        if (generateId) {
            calculateId(jo);
        }
    }

    return RULES_OK;
}

unsigned int getObjectProperty(jsonObject *jo, unsigned int hash, jsonProperty **property) {
    unsigned int candidate = hash % MAX_OBJECT_PROPERTIES;
    for (unsigned short i = 0; i < MAX_OBJECT_PROPERTIES; ++i) {
        unsigned char index = jo->propertyIndex[(candidate + i) % MAX_OBJECT_PROPERTIES];
        if (index == 0) {
            break;
        }

        // Property index offset by 1
        if (jo->properties[index - 1].hash == hash) {
            *property = &jo->properties[index - 1];   
            return RULES_OK;
        }
    }
    return ERR_PROPERTY_NOT_FOUND;
}

unsigned int constructObject(char *root,
                             char *parentName, 
                             char *object,
                             char generateId,
                             jsonObject *jo,
                             char **next) {
    char *firstName;
    char *lastName;
    char *first;
    char *last;
    char temp;
    unsigned char type;
    unsigned int hash;
    int parentNameLength;
    if (parentName) {
        parentNameLength = strlen(parentName);
    } else {
        parentNameLength = 0;
        jo->idIndex = UNDEFINED_INDEX;
        jo->sidIndex = UNDEFINED_INDEX;
        jo->propertiesLength = 0;
        jo->content = root;
        memset(jo->propertyIndex, 0, MAX_OBJECT_PROPERTIES * sizeof(unsigned char));
    }

    object = (object ? object : root);
    unsigned int result = readNextName(object, &firstName, &lastName, &hash);
    while (result == PARSE_OK) {
        result = readNextValue(lastName, &first, &last, &type);
        if (result != PARSE_OK) {
            return result;
        }

        jsonProperty *property = NULL;
        if (type != JSON_OBJECT) {
            unsigned int candidate = hash % MAX_OBJECT_PROPERTIES;
            while (jo->propertyIndex[candidate] != 0) {
                candidate = (candidate + 1) % MAX_OBJECT_PROPERTIES;
            }

            // Property index offset by 1
            jo->propertyIndex[candidate] = jo->propertiesLength + 1;
            property = &jo->properties[jo->propertiesLength]; 
            ++jo->propertiesLength;
            if (jo->propertiesLength == MAX_OBJECT_PROPERTIES) {
                return ERR_EVENT_MAX_PROPERTIES;
            } 

            if (!parentName) {
                if (hash == HASH_ID) {
                    jo->idIndex = candidate;
                } else if (hash == HASH_SID) {
                    jo->sidIndex = candidate;
                }
            }

            property->valueOffset = first - root;
            property->valueLength = last - first + 1;
            property->type = type;

            switch(type) {
                case JSON_INT:
                    temp = first[property->valueLength];
                    first[property->valueLength] = '\0';
                    property->value.i = atol(first);
                    first[property->valueLength] = temp;
                    break;
                case JSON_DOUBLE:
                    temp = first[property->valueLength];
                    first[property->valueLength] = '\0';
                    property->value.d = atof(first);
                    first[property->valueLength] = temp;
                    break;
                case JSON_BOOL:
                    if (property->valueLength == 4 && strncmp("true", first, 4) == 0) {
                        property->value.b = 1;
                    } else {
                        property->value.b = 0;
                    }

                    break;
                case JSON_STRING:
                    property->value.s = first;
                    property->valueLength = property->valueLength - 1;
                    break;
            }

        }
        
        if (!parentName) {
            int nameLength = lastName - firstName;
            if (type != JSON_OBJECT) {
                property->nameOffset = firstName - root;
                property->nameLength = nameLength;
                property->hash = hash;
            } else {            
#ifdef _WIN32
                char *newParent = (char *)_alloca(sizeof(char)*(nameLength + 1));
#else
                char newParent[nameLength + 1];
#endif
                strncpy(newParent, firstName, nameLength);
                newParent[nameLength] = '\0';
                result = constructObject(root,
                                         newParent, 
                                         first,
                                         0, 
                                         jo,
                                         next);
                if (result != RULES_OK) {
                    return result;
                }
            }
        } else {
            int nameLength = lastName - firstName;
            int fullNameLength = nameLength + parentNameLength + 1;
#ifdef _WIN32
            char *fullName = (char *)_alloca(sizeof(char)*(fullNameLength + 1));
#else
            char fullName[fullNameLength + 1];
#endif
            strncpy(fullName, parentName, parentNameLength);
            fullName[parentNameLength] = '.';
            strncpy(&fullName[parentNameLength + 1], firstName, nameLength);
            fullName[fullNameLength] = '\0';
            if (type != JSON_OBJECT) {
                property->nameOffset = firstName - root;
                property->nameLength = nameLength;
                property->hash = fnv1Hash32(fullName, fullNameLength);
            } else {

                result = constructObject(root,
                                         fullName, 
                                         first,
                                         0, 
                                         jo, 
                                         next);
                if (result != RULES_OK) {
                    return result;
                }
            }
        }
        
        *next = last;
        result = readNextName(last, &firstName, &lastName, &hash);
    }
 
    if (!parentName) {
        int idResult = fixupIds(jo, generateId);
        if (idResult != RULES_OK) {
            return idResult;
        }
    }

    return (result == PARSE_END ? RULES_OK: result);
}

unsigned int resolveBinding(void *tree, 
                            char *sid, 
                            void **rulesBinding) {  
    return RULES_OK;
}

unsigned int getState(unsigned int handle, char *sid, char **state) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    void *rulesBinding = NULL;
    if (!sid) {
        sid = "0";
    }

    unsigned int result = resolveBinding(tree, sid, &rulesBinding);
    if (result != RULES_OK) {
      return result;
    }

    return getSession(rulesBinding, sid, state);
}

unsigned int getStateVersion(void *tree, char *sid, unsigned long *stateVersion) {
    void *rulesBinding = NULL;
    unsigned int result = resolveBinding(tree, sid, &rulesBinding);
    if (result != RULES_OK) {
      return result;
    }

    return getSessionVersion(rulesBinding, sid, stateVersion);
}


unsigned int deleteState(unsigned int handle, char *sid) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    if (!sid) {
        sid = "0";
    }

    void *rulesBinding = NULL;
    unsigned int result = resolveBinding(tree, sid, &rulesBinding);
    if (result != RULES_OK) {
      return result;
    }

    unsigned int sidHash = fnv1Hash32(sid, strlen(sid));
    result = deleteSession(tree, rulesBinding, sid, sidHash);
    if (result != RULES_OK) {
      return result;
    }

    DELETE(stateNode, tree->stateIndex, MAX_STATE_INDEX_LENGTH, tree->statePool, sidHash);
    return RULES_OK;
}
