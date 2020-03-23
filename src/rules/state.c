#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifndef _WIN32
#include <time.h> /* for struct timeval */
#else
#include <WinSock2.h>
#endif
#include "rules.h"
#include "json.h"
#include "rete.h"

#define MAX_STATE_NODES 8
#define MAX_MESSAGE_NODES 16
#define MAX_LEFT_FRAME_NODES 8
#define MAX_RIGHT_FRAME_NODES 8
#define MAX_LOCATION_NODES 16

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

// The first node is never used as it corresponds to UNDEFINED_HASH_OFFSET
#define INIT(type, pool, length) do { \
    pool.content = malloc(length * sizeof(type)); \
    if (!pool.content) { \
        return ERR_OUT_OF_MEMORY; \
    } \
    pool.contentLength = length; \
    for (unsigned int i = 0; i < length; ++ i) { \
        ((type *)pool.content)[i].isActive = 0; \
        ((type *)pool.content)[i].nextOffset = i + 1; \
        ((type *)pool.content)[i].prevOffset = i - 1; \
    } \
    ((type *)pool.content)[length - 1].nextOffset = UNDEFINED_HASH_OFFSET; \
    ((type *)pool.content)[0].prevOffset = UNDEFINED_HASH_OFFSET; \
    pool.freeOffset = 1; \
    pool.count = 0; \
} while(0)

#define GET_FIRST(type, index, max, pool, nodeHash, valueOffset) do { \
    valueOffset = index[(nodeHash % max) * 2]; \
    while (valueOffset != UNDEFINED_HASH_OFFSET) { \
        type *value = &((type *)pool.content)[valueOffset]; \
        if (value->hash != nodeHash) { \
            valueOffset = value->nextOffset; \
        } else { \
            break; \
        } \
    } \
} while(0)

#define GET_LAST(type, index, max, pool, nodeHash, valueOffset) do { \
    valueOffset = index[(nodeHash % max) * 2 + 1]; \
    while (valueOffset != UNDEFINED_HASH_OFFSET) { \
        type *value = &((type *)pool.content)[valueOffset]; \
        if (value->hash != nodeHash) { \
            valueOffset = value->prevOffset; \
        } else { \
            break; \
        } \
    } \
} while(0)

#define NEW(type, pool, valueOffset) do { \
    valueOffset = pool.freeOffset; \
    type *value = &((type *)pool.content)[valueOffset]; \
    if (value->nextOffset == UNDEFINED_HASH_OFFSET) { \
        pool.content = realloc(pool.content, ((unsigned int)(pool.contentLength * 1.5)) * sizeof(type)); \
        if (!pool.content) { \
            return ERR_OUT_OF_MEMORY; \
        } \
        for (unsigned int i = pool.contentLength; i < (unsigned int)(pool.contentLength * 1.5); ++ i) { \
            ((type *)pool.content)[i].isActive = 0; \
            ((type *)pool.content)[i].nextOffset = i + 1; \
            ((type *)pool.content)[i].prevOffset = i - 1; \
        } \
        value = &((type *)pool.content)[valueOffset]; \
        value->nextOffset = pool.contentLength; \
        ((type *)pool.content)[pool.contentLength].prevOffset = valueOffset; \
        pool.contentLength *= 1.5; \
        ((type *)pool.content)[pool.contentLength - 1].nextOffset = UNDEFINED_HASH_OFFSET; \
    } \
    ((type *)pool.content)[value->nextOffset].prevOffset = UNDEFINED_HASH_OFFSET; \
    pool.freeOffset = value->nextOffset; \
    value->nextOffset = UNDEFINED_HASH_OFFSET; \
    value->prevOffset = UNDEFINED_HASH_OFFSET; \
    value->isActive = 1; \
    ++pool.count; \
} while(0)

#define SET(type, index, max, pool, nodeHash, valueOffset)  do { \
    type *value = &((type *)pool.content)[valueOffset]; \
    value->hash = nodeHash; \
    value->prevOffset = UNDEFINED_HASH_OFFSET; \
    value->nextOffset = index[(nodeHash % max) * 2]; \
    index[(nodeHash % max) * 2] = valueOffset; \
    if (value->nextOffset != UNDEFINED_HASH_OFFSET) { \
        ((type *)pool.content)[value->nextOffset].prevOffset = valueOffset; \
    } else { \
        index[(nodeHash % max) * 2 + 1] = valueOffset; \
    } \
} while(0)

#define DELETE(type, index, max, pool, valueOffset) do { \
    type *value = &((type *)pool.content)[valueOffset]; \
    if (!value->isActive) { \
        return ERR_NODE_DELETED; \
    } \
    if (value->prevOffset == UNDEFINED_HASH_OFFSET) { \
        index[(value->hash % max) * 2] = value->nextOffset; \
    } else { \
        type *prevValue = &((type *)pool.content)[value->prevOffset]; \
        prevValue->nextOffset = value->nextOffset; \
    } \
    if (value->nextOffset == UNDEFINED_HASH_OFFSET) { \
        index[(value->hash % max) * 2 + 1] = value->prevOffset; \
    } else { \
        type *nextValue = &((type *)pool.content)[value->nextOffset]; \
        nextValue->prevOffset = value->prevOffset; \
    } \
    value->nextOffset = pool.freeOffset; \
    value->prevOffset = UNDEFINED_HASH_OFFSET; \
    value->isActive = 0; \
    if (pool.freeOffset != UNDEFINED_HASH_OFFSET) { \
        type *freeValue = &((type *)pool.content)[pool.freeOffset]; \
        freeValue->prevOffset = valueOffset; \
    } \
    pool.freeOffset = valueOffset; \
    --pool.count; \
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

unsigned int getLocationHash(frameLocation location) {
    unsigned int hash = FNV_32_OFFSET_BASIS;
    hash ^= location.frameType;
    hash *= FNV_32_PRIME;
    hash ^= location.nodeIndex;
    hash *= FNV_32_PRIME;
    hash ^= location.frameOffset;
    hash *= FNV_32_PRIME;

    return hash;
}

unsigned int initStatePool(void *tree) {
    INIT(stateNode, ((ruleset*)tree)->statePool, MAX_STATE_NODES);
    return RULES_OK;
}

unsigned int addFrameLocation(stateNode *state,
                              frameLocation location,
                              unsigned int messageNodeOffset) { 
    messageNode *message = MESSAGE_NODE(state, messageNodeOffset);
    unsigned int locationNodeOffset;
    NEW(locationNode, 
        message->locationPool, 
        locationNodeOffset);
    
    locationNode *newLocationNode = LOCATION_NODE(message, locationNodeOffset);
    newLocationNode->location.frameType = location.frameType;
    newLocationNode->location.nodeIndex = location.nodeIndex;
    newLocationNode->location.frameOffset = location.frameOffset;
    
    unsigned int hash = getLocationHash(newLocationNode->location);
    SET(locationNode, 
        message->locationIndex, 
        MAX_LOCATION_INDEX_LENGTH, 
        message->locationPool, 
        hash, 
        locationNodeOffset);

    return RULES_OK;
}

unsigned int deleteFrameLocation(stateNode *state,
                                 unsigned int messageNodeOffset,
                                 frameLocation location) {
    messageNode *message = MESSAGE_NODE(state, messageNodeOffset);
    if (!message->isActive) {
        return RULES_OK;
    }

    unsigned int hash = getLocationHash(location);
    unsigned int locationNodeOffset;
    GET_FIRST(locationNode, 
              message->locationIndex, 
              MAX_LOCATION_INDEX_LENGTH, 
              message->locationPool, 
              hash, 
              locationNodeOffset);
    
    while (locationNodeOffset != UNDEFINED_HASH_OFFSET) {
        locationNode *newLocationNode = LOCATION_NODE(message, locationNodeOffset);
        if (newLocationNode->hash != hash) {
            locationNodeOffset = UNDEFINED_HASH_OFFSET;
        } else {
            if (newLocationNode->location.frameType == location.frameType &&
                newLocationNode->location.nodeIndex == location.nodeIndex &&
                newLocationNode->location.frameOffset == location.frameOffset) {
                DELETE(locationNode,
                       message->locationIndex, 
                       MAX_LOCATION_INDEX_LENGTH,
                       message->locationPool,
                       locationNodeOffset);

                return RULES_OK;
            }
        
            locationNodeOffset = newLocationNode->nextOffset;
        }
    }

    return RULES_OK;
}

unsigned int deleteMessageFromFrame(unsigned int messageNodeOffset, 
                                    leftFrameNode *frame) {

    for (int i = 0, c = 0; c < frame->messageCount; ++i) {
        if (frame->messages[i].hash) {
            ++c;
            if (frame->messages[i].messageNodeOffset == messageNodeOffset) {
                frame->messages[i].hash = 0;
                frame->messages[i].messageNodeOffset = UNDEFINED_HASH_OFFSET;
            }
        }
    }

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

unsigned int setMessageInFrame(leftFrameNode *node,
                               unsigned int nameOffset,
                               unsigned int hash, 
                               unsigned int messageNodeOffset) {
    unsigned short size = 0;
    unsigned short index = hash % MAX_MESSAGE_FRAMES;
    while (node->messages[index].hash) {
        index = (index + 1) % MAX_MESSAGE_FRAMES;
        ++size;
        if (size == MAX_MESSAGE_FRAMES) {
            return ERR_MAX_MESSAGES_IN_FRAME;
        }
    }
    node->messages[index].nameOffset = nameOffset;
    node->messages[index].hash = hash;
    node->messages[index].messageNodeOffset = messageNodeOffset;
    node->reverseIndex[node->messageCount] = index;
    ++node->messageCount;
    return RULES_OK;   
}

unsigned int getLastLeftFrame(stateNode *state,
                          unsigned int index, 
                          unsigned int hash, 
                          frameLocation *location,
                          leftFrameNode **node) {
    unsigned int valueOffset;
    GET_LAST(leftFrameNode, 
             state->betaState[index].leftFrameIndex, 
             MAX_LEFT_FRAME_INDEX_LENGTH, 
             state->betaState[index].leftFramePool, 
             hash, 
             valueOffset);
    if (valueOffset == UNDEFINED_HASH_OFFSET) {
        *node = NULL;
        return RULES_OK;
    }

    *node = LEFT_FRAME_NODE(state, index, valueOffset);
    if (location) {
        location->frameType = LEFT_FRAME;
        location->nodeIndex = index;
        location->frameOffset = valueOffset;
    }
    return RULES_OK;
}

unsigned int setLeftFrame(stateNode *state, 
                          unsigned int hash, 
                          frameLocation location) {
    SET(leftFrameNode, 
        state->betaState[location.nodeIndex].leftFrameIndex, 
        MAX_LEFT_FRAME_INDEX_LENGTH, 
        state->betaState[location.nodeIndex].leftFramePool, 
        hash, 
        location.frameOffset);
    return RULES_OK;
}

unsigned int deleteLeftFrame(stateNode *state,
                             frameLocation location) {
    DELETE(leftFrameNode,
           state->betaState[location.nodeIndex].leftFrameIndex, 
           MAX_LEFT_FRAME_INDEX_LENGTH,
           state->betaState[location.nodeIndex].leftFramePool,
           location.frameOffset);
    return RULES_OK;
}

static unsigned int copyLeftFrame(stateNode *state,
                          leftFrameNode *oldNode, 
                          leftFrameNode *targetNode, 
                          frameLocation newLocation) {
    if (!oldNode) {
        memset(targetNode->messages, 0, MAX_MESSAGE_FRAMES * sizeof(messageFrame));
        memset(targetNode->reverseIndex, 0, MAX_MESSAGE_FRAMES * sizeof(unsigned short));
        targetNode->messageCount = 0;
    } else {
        memcpy(targetNode->messages, oldNode->messages, MAX_MESSAGE_FRAMES * sizeof(messageFrame));
        memcpy(targetNode->reverseIndex, oldNode->reverseIndex, MAX_MESSAGE_FRAMES * sizeof(unsigned short));
        targetNode->messageCount = oldNode->messageCount;
        for (unsigned short i = 0; i < targetNode->messageCount; ++i) {
            CHECK_RESULT(addFrameLocation(state,
                                          newLocation, 
                                          targetNode->messages[targetNode->reverseIndex[i]].messageNodeOffset));
        }
    } 

    return RULES_OK;
} 

unsigned int createLeftFrame(stateNode *state,
                             node *reteNode,
                             leftFrameNode *oldNode,                        
                             leftFrameNode **newNode,
                             frameLocation *newLocation) {
    unsigned int newValueOffset;
    NEW(leftFrameNode, 
        state->betaState[reteNode->value.b.index].leftFramePool, 
        newValueOffset);
    
    leftFrameNode *targetNode = LEFT_FRAME_NODE(state, reteNode->value.b.index, newValueOffset);
    newLocation->frameType = LEFT_FRAME;
    newLocation->nodeIndex = reteNode->value.b.index;
    newLocation->frameOffset = newValueOffset;
    targetNode->nameOffset = reteNode->nameOffset;
    targetNode->isDispatching = 0;
    
    CHECK_RESULT(copyLeftFrame(state,
                               oldNode, 
                               targetNode, 
                               *newLocation));
    
    *newNode = targetNode;
    state->betaState[reteNode->value.b.index].reteNode = reteNode;
    return RULES_OK;
}

unsigned int getLastConnectorFrame(stateNode *state,
                                   unsigned int frameType,
                                   unsigned int index, 
                                   unsigned int *valueOffset,
                                   leftFrameNode **node) {
    if (frameType == A_FRAME) {
        GET_LAST(leftFrameNode, 
                 state->connectorState[index].aFrameIndex, 
                 1, 
                 state->connectorState[index].aFramePool, 
                 0, 
                 *valueOffset);
        if (*valueOffset == UNDEFINED_HASH_OFFSET) {
            *node = NULL;
            return RULES_OK;
        }

        *node = A_FRAME_NODE(state, index, *valueOffset);
    } else {
        GET_LAST(leftFrameNode, 
                 state->connectorState[index].bFrameIndex, 
                 1, 
                 state->connectorState[index].bFramePool, 
                 0, 
                 *valueOffset);
        if (*valueOffset == UNDEFINED_HASH_OFFSET) {
            *node = NULL;
            return RULES_OK;
        }

        *node = B_FRAME_NODE(state, index, *valueOffset);
    }

    return RULES_OK;
}

unsigned int setConnectorFrame(stateNode *state, 
                               unsigned int frameType,
                               frameLocation location) {
    if (frameType == A_FRAME) {
        SET(leftFrameNode, 
            state->connectorState[location.nodeIndex].aFrameIndex, 
            1, 
            state->connectorState[location.nodeIndex].aFramePool, 
            0, 
            location.frameOffset);
    } else {
        SET(leftFrameNode, 
            state->connectorState[location.nodeIndex].bFrameIndex, 
            1, 
            state->connectorState[location.nodeIndex].bFramePool, 
            0, 
            location.frameOffset);
    }

    return RULES_OK;
}

unsigned int deleteConnectorFrame(stateNode *state,
                                  unsigned int frameType,
                                  frameLocation location) {
    if (frameType == A_FRAME) {
        DELETE(leftFrameNode,
               state->connectorState[location.nodeIndex].aFrameIndex, 
               1,
               state->connectorState[location.nodeIndex].aFramePool,
               location.frameOffset);
    } else {
        DELETE(leftFrameNode,
               state->connectorState[location.nodeIndex].bFrameIndex, 
               1,
               state->connectorState[location.nodeIndex].bFramePool,
               location.frameOffset);

    }

    return RULES_OK;
}

unsigned int createConnectorFrame(stateNode *state,
                                  unsigned int frameType,
                                  node *reteNode,
                                  leftFrameNode *oldNode,                        
                                  leftFrameNode **newNode,
                                  frameLocation *newLocation) {
    unsigned int newValueOffset;
    leftFrameNode *targetNode = NULL;
    if (frameType == A_FRAME) {
        NEW(leftFrameNode, 
            state->connectorState[reteNode->value.b.index].aFramePool, 
            newValueOffset);

        targetNode = A_FRAME_NODE(state, reteNode->value.b.index, newValueOffset);
    } else {
        NEW(leftFrameNode, 
            state->connectorState[reteNode->value.b.index].bFramePool, 
            newValueOffset);

        targetNode = B_FRAME_NODE(state, reteNode->value.b.index, newValueOffset);
    }

    newLocation->frameType = frameType;
    newLocation->nodeIndex = reteNode->value.b.index;
    newLocation->frameOffset = newValueOffset;
    targetNode->nameOffset = reteNode->nameOffset;
    targetNode->isDispatching = 0;
    
    CHECK_RESULT(copyLeftFrame(state,
                               oldNode, 
                               targetNode, 
                               *newLocation));
    
    *newNode = targetNode;
    state->connectorState[reteNode->value.b.index].reteNode = reteNode;
    return RULES_OK;
}

unsigned int getActionFrame(stateNode *state,
                            frameLocation resultLocation,
                            leftFrameNode **resultNode) {
    actionStateNode *resultStateNode = &state->actionState[resultLocation.nodeIndex];
    *resultNode = RESULT_FRAME(resultStateNode, resultLocation.frameOffset);
    if (!(*resultNode)->isActive) {
        *resultNode = NULL;
        return ERR_NODE_DELETED;
    }
    return RULES_OK;
}

unsigned int setActionFrame(stateNode *state, 
                            frameLocation location) {
    SET(leftFrameNode, 
        state->actionState[location.nodeIndex].resultIndex, 
        1, 
        state->actionState[location.nodeIndex].resultPool, 
        0, 
        location.frameOffset);
    return RULES_OK;
}

unsigned int deleteDispatchingActionFrame(stateNode *state,
                                          frameLocation location) {    
    DELETE(leftFrameNode,
           state->actionState[location.nodeIndex].resultIndex, 
           1,
           state->actionState[location.nodeIndex].resultPool,
           location.frameOffset);
    return RULES_OK;
}

unsigned int deleteActionFrame(stateNode *state,
                               frameLocation location) {    

    leftFrameNode *targetNode = ACTION_FRAME_NODE(state, location.nodeIndex, location.frameOffset);
    if (targetNode->isDispatching) {
        return ERR_NODE_DISPATCHING;
    }

    DELETE(leftFrameNode,
           state->actionState[location.nodeIndex].resultIndex, 
           1,
           state->actionState[location.nodeIndex].resultPool,
           location.frameOffset);
    return RULES_OK;
}

unsigned int createActionFrame(stateNode *state,
                               node *reteNode,
                               leftFrameNode *oldNode,                        
                               leftFrameNode **newNode,
                               frameLocation *newLocation) {
    unsigned int newValueOffset;
    actionStateNode *actionNode = &state->actionState[reteNode->value.c.index];
    NEW(leftFrameNode, 
        actionNode->resultPool, 
        newValueOffset);
    leftFrameNode *targetNode = ACTION_FRAME_NODE(state, reteNode->value.c.index, newValueOffset);
    newLocation->frameType = ACTION_FRAME;
    newLocation->nodeIndex = reteNode->value.c.index;
    newLocation->frameOffset = newValueOffset;
    targetNode->nameOffset = reteNode->nameOffset;
    targetNode->isDispatching = 0;
    
    CHECK_RESULT(copyLeftFrame(state,
                               oldNode, 
                               targetNode, 
                               *newLocation));
    
    *newNode = targetNode;
    state->actionState[reteNode->value.c.index].reteNode = reteNode;
    return RULES_OK;
}

unsigned int getLastRightFrame(stateNode *state,
                           unsigned int index, 
                           unsigned int hash, 
                           rightFrameNode **node) {
    unsigned int valueOffset;
    GET_LAST(rightFrameNode, 
             state->betaState[index].rightFrameIndex, 
             MAX_RIGHT_FRAME_INDEX_LENGTH, 
             state->betaState[index].rightFramePool, 
             hash, 
             valueOffset);
    if (valueOffset == UNDEFINED_HASH_OFFSET) {
        *node = NULL;
        return RULES_OK;
    }

    *node = RIGHT_FRAME_NODE(state, index, valueOffset);
    return RULES_OK;
}

unsigned int setRightFrame(stateNode *state,
                           unsigned int hash, 
                           frameLocation location) {
    SET(rightFrameNode, 
        state->betaState[location.nodeIndex].rightFrameIndex, 
        MAX_RIGHT_FRAME_INDEX_LENGTH, 
        state->betaState[location.nodeIndex].rightFramePool, 
        hash, 
        location.frameOffset);
    return RULES_OK;
}

unsigned int deleteRightFrame(stateNode *state,
                              frameLocation location) { 
    DELETE(rightFrameNode,
           state->betaState[location.nodeIndex].rightFrameIndex, 
           MAX_RIGHT_FRAME_INDEX_LENGTH,
           state->betaState[location.nodeIndex].rightFramePool,
           location.frameOffset);
    return RULES_OK;
}

unsigned int createRightFrame(stateNode *state,
                              node *reteNode,
                              rightFrameNode **node,
                              frameLocation *location) {
    unsigned int valueOffset;
    NEW(rightFrameNode, 
        state->betaState[reteNode->value.b.index].rightFramePool, 
        valueOffset);
    *node = RIGHT_FRAME_NODE(state, reteNode->value.b.index, valueOffset);

    location->frameType = RIGHT_FRAME;
    location->nodeIndex = reteNode->value.b.index;
    location->frameOffset = valueOffset;

    state->betaState[reteNode->value.b.index].reteNode = reteNode;
    return RULES_OK;
}

unsigned int deleteMessage(void *tree,
                           stateNode *state,
                           char *mid,
                           unsigned int messageNodeOffset) {
    messageNode *node = MESSAGE_NODE(state, messageNodeOffset);
    ruleset *rulesetTree = (ruleset*)tree;
    if (mid == NULL) {
        mid = (char*)node->jo.idBuffer;
    }
    if (rulesetTree->deleteMessageCallback) {
        CHECK_RESULT(rulesetTree->deleteMessageCallback(rulesetTree->deleteMessageCallbackContext,
                                                        &rulesetTree->stringPool[rulesetTree->nameOffset],
                                                        state->sid, 
                                                        mid));
    }

    if (node->jo.content) {
        free(node->jo.content);
        free(node->locationPool.content);
        node->jo.content = NULL;
        node->locationPool.content = NULL;
    }
    
    DELETE(messageNode,
           state->messageIndex, 
           MAX_MESSAGE_INDEX_LENGTH,
           state->messagePool,
           messageNodeOffset);

    return RULES_OK;
}

unsigned int getMessage(stateNode *state,
                        char *mid,
                        unsigned int *valueOffset) {
    *valueOffset = UNDEFINED_HASH_OFFSET;
    unsigned int hash = fnv1Hash32(mid, strlen(mid));

    GET_FIRST(messageNode, 
              state->messageIndex, 
              MAX_MESSAGE_INDEX_LENGTH, 
              state->messagePool, 
              hash, 
              *valueOffset);

    return RULES_OK;
}

static unsigned int copyMessage(jsonObject *targetMessage,
                                jsonObject *sourceMessage) {
    memcpy(targetMessage, sourceMessage, sizeof(jsonObject));
    unsigned int messageLength = (strlen(sourceMessage->content) + 1) * sizeof(char);
    targetMessage->content = malloc(messageLength);
    if (!targetMessage->content) {
        return ERR_OUT_OF_MEMORY;
    }
    memcpy(targetMessage->content, sourceMessage->content, messageLength);
    for (unsigned int i = 0; i < targetMessage->propertiesLength; ++i) {
        if (targetMessage->properties[i].type == JSON_STRING &&
            targetMessage->properties[i].hash != HASH_SID && 
            targetMessage->properties[i].hash != HASH_ID) {
            targetMessage->properties[i].value.s = targetMessage->content + targetMessage->properties[i].valueOffset;
        }
    }
    return RULES_OK;
}

unsigned int storeMessage(void *tree,
                          stateNode *state,
                          char *mid,
                          jsonObject *message,
                          unsigned char messageType,
                          unsigned char sideEffect,
                          unsigned int *valueOffset) {
    unsigned int hash = fnv1Hash32(mid, strlen(mid));
    *valueOffset = UNDEFINED_HASH_OFFSET;

    GET_FIRST(messageNode, 
              state->messageIndex, 
              MAX_MESSAGE_INDEX_LENGTH, 
              state->messagePool, 
              hash, 
              *valueOffset);

    if (*valueOffset != UNDEFINED_HASH_OFFSET) {
        return ERR_EVENT_OBSERVED;
    }

    NEW(messageNode, 
        state->messagePool, 
        *valueOffset);

    SET(messageNode, 
        state->messageIndex, 
        MAX_MESSAGE_INDEX_LENGTH, 
        state->messagePool, 
        hash, 
        *valueOffset);

    messageNode *node = MESSAGE_NODE(state, *valueOffset);
    INIT(locationNode, 
         node->locationPool, 
         MAX_LOCATION_NODES);

    memset(node->locationIndex, 0, MAX_LOCATION_INDEX_LENGTH * sizeof(unsigned int) * 2);

    node->messageType = messageType;
    CHECK_RESULT(copyMessage(&node->jo, message));

    ruleset *rulesetTree = (ruleset*)tree;
    if (sideEffect && rulesetTree->storeMessageCallback) {
        unsigned char actionType = ACTION_ASSERT_EVENT;
        switch (messageType) {
            case MESSAGE_TYPE_EVENT:
                actionType = ACTION_ASSERT_EVENT;
                break;
            case MESSAGE_TYPE_FACT:
                actionType = ACTION_ASSERT_FACT;
                break;
            case MESSAGE_TYPE_STATE:
                actionType = ACTION_UPDATE_STATE;
                break;
        }
        
        return rulesetTree->storeMessageCallback(rulesetTree->storeMessageCallbackContext, 
                                                 &rulesetTree->stringPool[rulesetTree->nameOffset],
                                                 state->sid, 
                                                 mid, 
                                                 actionType,
                                                 message->content);
    }

    return RULES_OK;
}

unsigned int getStateNode(void *tree, 
                          char *sid, 
                          stateNode **state) { 
    unsigned int sidHash = fnv1Hash32(sid, strlen(sid));
    unsigned int nodeOffset;
    ruleset *rulesetTree = (ruleset*)tree;

    GET_FIRST(stateNode, 
              rulesetTree->stateIndex, 
              MAX_STATE_INDEX_LENGTH, 
              ((ruleset*)tree)->statePool, 
              sidHash, 
              nodeOffset);
    if (nodeOffset == UNDEFINED_HASH_OFFSET) {
      return ERR_SID_NOT_FOUND;
    }

    *state = STATE_NODE(tree, nodeOffset); 
    return RULES_OK; 
} 

unsigned int createStateNode(void *tree, 
                             char *sid, 
                             stateNode **state) {
    unsigned int sidHash = fnv1Hash32(sid, strlen(sid));
    unsigned int nodeOffset;
    ruleset *rulesetTree = (ruleset*)tree;

    NEW(stateNode, 
        rulesetTree->statePool, 
        nodeOffset);

    SET(stateNode, 
        rulesetTree->stateIndex, 
        MAX_STATE_INDEX_LENGTH, 
        rulesetTree->statePool, 
        sidHash, 
        nodeOffset);

    if (rulesetTree->statePool.count > MAX_STATE_INDEX_LENGTH) {
        return ERR_OUT_OF_MEMORY;
    }
    rulesetTree->reverseStateIndex[rulesetTree->statePool.count - 1] = nodeOffset;
    stateNode *node = STATE_NODE(tree, nodeOffset); 
    node->offset = nodeOffset;

    int sidLength = sizeof(char) * (strlen(sid) + 1);
    node->sid = malloc(sidLength);
    if (!node->sid) {
        return ERR_OUT_OF_MEMORY;
    }
    memcpy(node->sid, sid, sidLength);

    INIT(messageNode, 
         node->messagePool, 
         MAX_MESSAGE_NODES);

    memset(node->messageIndex, 0, MAX_MESSAGE_INDEX_LENGTH * sizeof(unsigned int) * 2);
    
    node->betaState = malloc(((ruleset*)tree)->betaCount * sizeof(betaStateNode));
    for (unsigned int i = 0; i < ((ruleset*)tree)->betaCount; ++i) {
        betaStateNode *betaNode = &node->betaState[i];
        betaNode->reteNode = NULL;

        INIT(leftFrameNode, 
             betaNode->leftFramePool, 
             MAX_LEFT_FRAME_NODES);

        memset(betaNode->leftFrameIndex, 0, MAX_LEFT_FRAME_INDEX_LENGTH * sizeof(unsigned int) * 2);
        
        INIT(rightFrameNode, 
             betaNode->rightFramePool, 
             MAX_RIGHT_FRAME_NODES);

        memset(betaNode->rightFrameIndex, 0, MAX_RIGHT_FRAME_INDEX_LENGTH * sizeof(unsigned int) * 2);
    }

    node->connectorState = malloc(((ruleset*)tree)->connectorCount * sizeof(connectorStateNode));
    for (unsigned int i = 0; i < ((ruleset*)tree)->connectorCount; ++i) {
        connectorStateNode *connectorNode = &node->connectorState[i];
        connectorNode->reteNode = NULL;

        INIT(leftFrameNode, 
             connectorNode->aFramePool, 
             MAX_LEFT_FRAME_NODES);

        connectorNode->aFrameIndex[0] = UNDEFINED_HASH_OFFSET;
        connectorNode->aFrameIndex[1] = UNDEFINED_HASH_OFFSET;

        INIT(leftFrameNode, 
             connectorNode->bFramePool, 
             MAX_LEFT_FRAME_NODES);

        connectorNode->bFrameIndex[0] = UNDEFINED_HASH_OFFSET;
        connectorNode->bFrameIndex[1] = UNDEFINED_HASH_OFFSET;
    }

    node->actionState = malloc(((ruleset*)tree)->actionCount * sizeof(actionStateNode));
    for (unsigned int i = 0; i < ((ruleset*)tree)->actionCount; ++i) {
        actionStateNode *actionNode = &node->actionState[i];
        actionNode->reteNode = NULL;
        
        actionNode->resultIndex[0] = UNDEFINED_HASH_OFFSET;
        actionNode->resultIndex[1] = UNDEFINED_HASH_OFFSET;

        INIT(leftFrameNode, 
             actionNode->resultPool, 
             MAX_LEFT_FRAME_NODES);
    }        

    node->factOffset = UNDEFINED_HASH_OFFSET;
    node->lockExpireTime = 0;
    *state = node;
    
    return RULES_OK;
}

unsigned int deleteStateNode(void *tree, 
                             stateNode *node) {
    ruleset *rulesetTree = (ruleset*)tree;

    free(node->sid);

    if (node->context.messages) {
        free(node->context.messages);
        node->context.messages = NULL;
    }

    if (node->context.stateFact) {
        free(node->context.stateFact);
        node->context.stateFact = NULL;
    }

    for (unsigned int i = 0; i < rulesetTree->betaCount; ++i) {
        betaStateNode *betaNode = &node->betaState[i];
        free(betaNode->leftFramePool.content);
        free(betaNode->rightFramePool.content);
    }
    free(node->betaState);

    for (unsigned int i = 0; i < rulesetTree->connectorCount; ++i) {
        connectorStateNode *connectorNode = &node->connectorState[i];
        free(connectorNode->aFramePool.content);
        free(connectorNode->bFramePool.content);
    }
    free(node->connectorState);

    for (unsigned int i = 0; i < rulesetTree->actionCount; ++i) {
        actionStateNode *actionNode = &node->actionState[i];
        free(actionNode->resultPool.content);
    }   
    free(node->actionState);

    for (unsigned int i = 0; i < MAX_MESSAGE_INDEX_LENGTH; ++i) {
        unsigned int currentMessageOffset = node->messageIndex[i * 2];
        while (currentMessageOffset != UNDEFINED_HASH_OFFSET) {
            messageNode *currentMessageNode = MESSAGE_NODE(node, currentMessageOffset);
            unsigned int nextMessageOffset = currentMessageNode->nextOffset; 
            deleteMessage(tree, node, NULL, currentMessageOffset);
            currentMessageOffset = nextMessageOffset;
        }
    }
    free(node->messagePool.content);

    DELETE(stateNode, 
           rulesetTree->stateIndex, 
           MAX_STATE_INDEX_LENGTH, 
           rulesetTree->statePool, 
           node->offset);

    return RULES_OK;
}

static unsigned int getResultFrameLength(ruleset *tree, 
                                         stateNode *state, 
                                         leftFrameNode *frame) {
    unsigned int resultLength = 2;
    for (int i = 0; i < frame->messageCount; ++i) {
        messageFrame *currentFrame = &frame->messages[frame->reverseIndex[i]]; 
        messageNode *currentNode = MESSAGE_NODE(state, currentFrame->messageNodeOffset);
        char *name = &tree->stringPool[currentFrame->nameOffset];
        char *value = currentNode->jo.content;
        if (i < (frame->messageCount -1)) {
            resultLength += strlen(name) + strlen(value) + 4;
        } else {
            resultLength += strlen(name) + strlen(value) + 3;
        }
    }

    return resultLength;
}

static void serializeResultFrame(ruleset *tree, 
                                 stateNode *state, 
                                 leftFrameNode *frame, 
                                 char *first,
                                 char **last) {
    
    first[0] = '{'; 
    ++first;
    for (int i = 0; i < frame->messageCount; ++i) {
        unsigned int tupleLength;
        messageFrame *currentFrame = &frame->messages[frame->reverseIndex[i]]; 
        messageNode *currentNode = MESSAGE_NODE(state, currentFrame->messageNodeOffset);
        char *name = &tree->stringPool[currentFrame->nameOffset];
        char *value = currentNode->jo.content;
        if (i < (frame->messageCount -1)) {
            tupleLength = strlen(name) + strlen(value) + 5;
#ifdef _WIN32
                sprintf_s(first, tupleLength, "\"%s\":%s,", name, value);
#else
                snprintf(first, tupleLength, "\"%s\":%s,", name, value);
#endif
        } else {
            tupleLength = strlen(name) + strlen(value) + 4;
#ifdef _WIN32
                sprintf_s(first, tupleLength, "\"%s\":%s", name, value);
#else
                snprintf(first, tupleLength, "\"%s\":%s", name, value);
#endif
        }

        first += (tupleLength - 1); 
    }

    first[0] = '}';
    *last = first + 1;
}

unsigned int serializeResult(void *tree, 
                             stateNode *state, 
                             actionStateNode *actionNode, 
                             unsigned int count,
                             char **result) {
    
    if (actionNode->reteNode->value.c.count && count == 1) {
        leftFrameNode *resultFrame = RESULT_FRAME(actionNode, actionNode->resultIndex[0]);
        if (!resultFrame->isActive) {
            return ERR_NODE_DELETED;
        }

        char *actionName = &((ruleset *)tree)->stringPool[resultFrame->nameOffset];
        unsigned int resultLength = strlen(actionName) + 6 + getResultFrameLength(tree, 
                                                                                  state, 
                                                                                  resultFrame);
        *result = malloc(resultLength * sizeof(char));
        if (!*result) {
            return ERR_OUT_OF_MEMORY;
        }

        char *first = *result;
        // +1 to leave space for the 0 character
        unsigned int tupleLength = strlen(actionName) + 5;
#ifdef _WIN32
        sprintf_s(first, tupleLength, "{\"%s\":", actionName);
#else
        snprintf(first, tupleLength, "{\"%s\":", actionName);
#endif
        first += tupleLength - 1;

        char *last;
        resultFrame->isDispatching = 1;
        serializeResultFrame(tree, 
                             state, 
                             resultFrame, 
                             first,
                             &last);
        last[0] = '}';
        last[1] = 0;
    } else {
        leftFrameNode *resultFrame = RESULT_FRAME(actionNode, actionNode->resultIndex[0]);
        if (!resultFrame->isActive) {
            return ERR_NODE_DELETED;
        }

        char *actionName = &((ruleset *)tree)->stringPool[resultFrame->nameOffset];
        unsigned int resultLength = strlen(actionName) + 8;
        for (unsigned int currentCount = 0; currentCount < count; ++currentCount) {
            resultLength += 1 + getResultFrameLength(tree, 
                                                     state, 
                                                     resultFrame);

            if (resultFrame->nextOffset != UNDEFINED_HASH_OFFSET) {
                resultFrame = RESULT_FRAME(actionNode, resultFrame->nextOffset);
            }
        }

        *result = malloc(resultLength * sizeof(char));
        if (!*result) {
            return ERR_OUT_OF_MEMORY;
        }

        char *first = *result;
        // +1 to leave space for the 0 character
        unsigned int tupleLength = strlen(actionName) + 5;
#ifdef _WIN32
        sprintf_s(first, tupleLength, "{\"%s\":", actionName);
#else
        snprintf(first, tupleLength, "{\"%s\":", actionName);
#endif
        first[tupleLength - 1] = '[';
        first += tupleLength ;

        char *last;
        resultFrame = RESULT_FRAME(actionNode, actionNode->resultIndex[0]);
        for (unsigned int currentCount = 0; currentCount < count; ++currentCount) {
            resultFrame->isDispatching = 1;
            serializeResultFrame(tree, 
                                 state, 
                                 resultFrame, 
                                 first,
                                 &last);

            if (resultFrame->nextOffset != UNDEFINED_HASH_OFFSET) {
                resultFrame = RESULT_FRAME(actionNode, resultFrame->nextOffset);
            }

            if (currentCount < count - 1) {
                last[0] = ',';
                first = last + 1;
            }
        }

        last[0] = ']';
        last[1] = '}';
        last[2] = 0;
    }

    return RULES_OK;
}

unsigned int getNextResultInState(void *tree, 
                                  time_t currentTime,
                                  stateNode *state,
                                  unsigned int *actionStateIndex,
                                  unsigned int *resultCount,
                                  unsigned int *resultFrameOffset, 
                                  actionStateNode **resultAction) {

    ruleset *rulesetTree = (ruleset*)tree;
    *resultAction = NULL;
    if (currentTime - state->lockExpireTime > STATE_LEASE_TIME) { 
        for (unsigned int index = 0; index < rulesetTree->actionCount; ++index) {
            actionStateNode *actionNode = &state->actionState[index];
            if (actionNode->reteNode) {
                if ((actionNode->reteNode->value.c.cap && actionNode->resultPool.count) ||
                    (actionNode->reteNode->value.c.count && actionNode->resultPool.count >= actionNode->reteNode->value.c.count)) {
                    *resultAction = actionNode;
                    *actionStateIndex = index;
                    *resultFrameOffset = actionNode->resultIndex[0];
                    if (actionNode->reteNode->value.c.count) {
                        *resultCount = actionNode->reteNode->value.c.count;
                    } else {
                        *resultCount = (actionNode->reteNode->value.c.cap > actionNode->resultPool.count ? 
                                        actionNode->resultPool.count : 
                                        actionNode->reteNode->value.c.cap);
                    }

                    return RULES_OK;
                }
            }
        }
    }

    return ERR_NO_ACTION_AVAILABLE;
}

unsigned int getNextResult(void *tree, 
                           time_t currentTime,
                           stateNode **resultState, 
                           unsigned int *actionStateIndex,
                           unsigned int *resultCount,
                           unsigned int *resultFrameOffset, 
                           actionStateNode **resultAction) {
    unsigned int count = 0;
    ruleset *rulesetTree = (ruleset*)tree;
    *resultAction = NULL;
    
    // In case state pool has been modified.
    if (rulesetTree->statePool.count) {
        rulesetTree->currentStateIndex = rulesetTree->currentStateIndex % rulesetTree->statePool.count;
    }

    while (count < rulesetTree->statePool.count && !*resultAction) {
        unsigned int nodeOffset = rulesetTree->reverseStateIndex[rulesetTree->currentStateIndex];
        *resultState = STATE_NODE(tree, nodeOffset);
        
        unsigned int result = getNextResultInState(tree,
                                                   currentTime,
                                                   *resultState,
                                                   actionStateIndex,
                                                   resultCount,
                                                   resultFrameOffset,
                                                   resultAction);
        if (result != ERR_NO_ACTION_AVAILABLE) {
            return result;
        }
        
        rulesetTree->currentStateIndex = (rulesetTree->currentStateIndex + 1) % rulesetTree->statePool.count;
        ++count;
    }

    return ERR_NO_ACTION_AVAILABLE;
}

static void insertSortProperties(jsonObject *jo, jsonProperty **properties) {
    for (unsigned short i = 1; i < jo->propertiesLength; ++i) {
        unsigned short ii = i; 
        while (ii >= 1 && (properties[ii]->hash < properties[ii - 1]->hash)) {
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

unsigned int calculateId(jsonObject *jo) {

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
        if (property->hash != HASH_ID) {
            hash ^= property->hash;
            hash *= FNV_64_PRIME;
        
            unsigned short valueLength = property->valueLength;
            if (property->type == JSON_STRING) {
                // Using value.s to cover generated sid
                for (unsigned short ii = 0; ii < valueLength; ++ii) {
                    hash ^= property->value.s[ii];
                    hash *= FNV_64_PRIME;
                }
            } else {
                for (unsigned short ii = 0; ii < valueLength; ++ii) {
                    hash ^= jo->content[property->valueOffset + ii];
                    hash *= FNV_64_PRIME;
                }
            }
        }
    }

#ifdef _WIN32
    sprintf_s(jo->idBuffer, sizeof(char) * ID_BUFFER_LENGTH, "$%020llu", hash); 
#else
    snprintf(jo->idBuffer, sizeof(char) * ID_BUFFER_LENGTH, "$%020llu", hash);
#endif

    jsonProperty *property;
    property = &jo->properties[jo->propertiesLength];
    jo->idIndex = jo->propertiesLength;
    ++jo->propertiesLength;
    if (jo->propertiesLength == MAX_OBJECT_PROPERTIES) {
        return ERR_EVENT_MAX_PROPERTIES;
    }

    unsigned int candidate = HASH_ID % MAX_OBJECT_PROPERTIES;
    while (jo->propertyIndex[candidate] != 0) {
        candidate = (candidate + 1) % MAX_OBJECT_PROPERTIES;
    }

    // Index intentionally offset by 1 to enable getObject 
    jo->propertyIndex[candidate] = jo->propertiesLength;
    property->hash = HASH_ID;
    property->valueOffset = 0;
    property->valueLength = 20;
    property->type = JSON_STRING;

    return RULES_OK;
}

static unsigned int fixupIds(jsonObject *jo, char generateId) {
    jsonProperty *property;
    // id and sid are coerced to strings
    // to avoid unnecessary conversions
    if (jo->sidIndex != UNDEFINED_INDEX && jo->properties[jo->sidIndex].type != JSON_NIL) {
        //coerce value to string
        property = &jo->properties[jo->sidIndex];
        if (property->type != JSON_STRING) {
            property->value.s = jo->content + property->valueOffset;
            property->type = JSON_STRING;
        }
    } else {
        property = &jo->properties[jo->propertiesLength];
        jo->sidIndex = jo->propertiesLength;
        ++jo->propertiesLength;
        if (jo->propertiesLength == MAX_OBJECT_PROPERTIES) {
            return ERR_EVENT_MAX_PROPERTIES;
        }

        unsigned int candidate = HASH_SID % MAX_OBJECT_PROPERTIES;
        while (jo->propertyIndex[candidate] != 0) {
            candidate = (candidate + 1) % MAX_OBJECT_PROPERTIES;
        }

        // Index intentionally offset by 1 to enable getObject 
        jo->propertyIndex[candidate] = jo->propertiesLength;

        strncpy(jo->sidBuffer, "0", 1);
        property->hash = HASH_SID;
        property->valueOffset = 0;
        property->valueLength = 1;
        property->value.s = jo->sidBuffer;
        property->type = JSON_STRING;
    } 

    if (jo->idIndex != UNDEFINED_INDEX && jo->properties[jo->idIndex].type != JSON_NIL) {
        //coerce value to string
        property = &jo->properties[jo->idIndex];
        if (property->type != JSON_STRING) {
            property->value.s = jo->content + property->valueOffset;
            property->type = JSON_STRING;
        }
    } else if (generateId) {
        return calculateId(jo);
    }

    return RULES_OK;
}

unsigned int getObjectProperty(jsonObject *jo, 
                               unsigned int hash, 
                               jsonProperty **property) {
    unsigned short size = 0;
    unsigned short index = hash % MAX_OBJECT_PROPERTIES;
    while (jo->propertyIndex[index] && size < MAX_OBJECT_PROPERTIES) {
        unsigned short subIndex = jo->propertyIndex[index] - 1;
        if (jo->properties[subIndex].hash == hash) {
            *property = &jo->properties[subIndex];
            return RULES_OK;
        }

        ++size;
        index = (index + 1) % MAX_OBJECT_PROPERTIES;
    }

    return ERR_PROPERTY_NOT_FOUND;
}

unsigned int setObjectProperty(jsonObject *jo, 
                               unsigned int hash, 
                               unsigned char type, 
                               unsigned short valueOffset, 
                               unsigned short valueLength) {
    jsonProperty *property = &jo->properties[jo->propertiesLength]; 
    ++jo->propertiesLength;
    if (jo->propertiesLength == MAX_OBJECT_PROPERTIES) {
        return ERR_EVENT_MAX_PROPERTIES;
    }

    unsigned int candidate = hash % MAX_OBJECT_PROPERTIES;
    while (jo->propertyIndex[candidate] != 0) {
        candidate = (candidate + 1) % MAX_OBJECT_PROPERTIES;
    }

    // Index intentionally offset by 1 to enable getObject 
    jo->propertyIndex[candidate] = jo->propertiesLength;
    if (hash == HASH_ID) {
        jo->idIndex = jo->propertiesLength - 1;
    } else if (hash == HASH_SID) {
        jo->sidIndex = jo->propertiesLength - 1;
    }
    
    property->hash = hash;
    property->valueOffset = valueOffset;
    property->valueLength = valueLength;
    property->type = type;
    
    char *first = jo->content + property->valueOffset;
    char temp;
    switch(type) {
        case JSON_INT:
            temp = first[property->valueLength];
            first[property->valueLength] = '\0';
            property->value.i = atoll(first);
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

    return RULES_OK;
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
        memset(jo->propertyIndex, 0, MAX_OBJECT_PROPERTIES * sizeof(unsigned short));
    }

    object = (object ? object : root);
    unsigned int result = readNextName(object, &firstName, &lastName, &hash);
    while (result == PARSE_OK) {
        result = readNextValue(lastName, &first, &last, &type);
        if (result != PARSE_OK) {
            return result;
        }

        if (!parentName) {
            if (type != JSON_OBJECT) {
                CHECK_RESULT(setObjectProperty(jo,
                                               hash,
                                               type,
                                               first - root,
                                               last - first + 1));
            } else {   
                int nameLength = lastName - firstName;         
#ifdef _WIN32
                char *newParent = (char *)_alloca(sizeof(char)*(nameLength + 1));
#else
                char newParent[nameLength + 1];
#endif
                strncpy(newParent, firstName, nameLength);
                newParent[nameLength] = '\0';

                hash = fnv1Hash32(newParent, nameLength);
                CHECK_RESULT(setObjectProperty(jo,
                                               hash,
                                               type,
                                               first - root,
                                               last - first + 1));

                CHECK_RESULT(constructObject(root,
                                             newParent, 
                                             first,
                                             0, 
                                             jo,
                                             next));
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
            CHECK_RESULT(setObjectProperty(jo,
                                           fnv1Hash32(fullName, fullNameLength),
                                           type,
                                           first - root,
                                           last - first + 1));
            if (type == JSON_OBJECT) {
                CHECK_RESULT(constructObject(root,
                                             fullName, 
                                             first,
                                             0, 
                                             jo, 
                                             next));
            }
        }
        
        *next = last;
        result = readNextName(last, &firstName, &lastName, &hash);
    }
 
    if (!parentName) {
        CHECK_RESULT(fixupIds(jo, generateId));
    }

    return (result == PARSE_END ? RULES_OK: result);
}

static unsigned int getMessagesLength(stateNode *state, 
                                      char messageType,
                                      unsigned int *messageCount) {
    unsigned int resultLength = 2;
    *messageCount = 0;
    for (unsigned int i = 0; i < MAX_MESSAGE_INDEX_LENGTH; ++i) {
        unsigned int currentMessageOffset = state->messageIndex[i * 2];
        while (currentMessageOffset != UNDEFINED_HASH_OFFSET) {
            messageNode *currentMessageNode = MESSAGE_NODE(state, currentMessageOffset);
            if (currentMessageNode->messageType == messageType) {
                ++*messageCount;
                resultLength += strlen(currentMessageNode->jo.content) + 1;
            }
            currentMessageOffset = currentMessageNode->nextOffset;
        }
    }

    if (!*messageCount) {
        ++resultLength;
    }
    
    return resultLength;
}
 
static unsigned int serializeMessages(stateNode *state, 
                                      char messageType,
                                      unsigned int messageCount, 
                                      char *messages) {

    messages[0] = '['; 
    ++messages;
    unsigned int currentMessageCount = 0;
    for (unsigned int i = 0; i < MAX_MESSAGE_INDEX_LENGTH && currentMessageCount < messageCount; ++i) {
        unsigned int currentMessageOffset = state->messageIndex[i * 2];
        while (currentMessageOffset != UNDEFINED_HASH_OFFSET && currentMessageCount < messageCount) {
            messageNode *currentMessageNode = MESSAGE_NODE(state, currentMessageOffset);
            if (currentMessageNode->messageType == messageType) {
                ++currentMessageCount;
                unsigned int tupleLength = strlen(currentMessageNode->jo.content) + 1;
                if (currentMessageCount < messageCount) {
                    ++tupleLength;
#ifdef _WIN32
                    sprintf_s(messages, tupleLength, "%s,", currentMessageNode->jo.content);
#else
                    snprintf(messages, tupleLength, "%s,", currentMessageNode->jo.content);
#endif
                } else {
#ifdef _WIN32
                    sprintf_s(messages, tupleLength, "%s", currentMessageNode->jo.content);
#else
                    snprintf(messages, tupleLength, "%s", currentMessageNode->jo.content);
#endif   
                }

                messages += (tupleLength - 1);
            }
            currentMessageOffset = currentMessageNode->nextOffset;
        }
    }

    messages[0] = ']';
    messages[1] = 0;
    return RULES_OK;
}


static unsigned int getMessagesForType(unsigned int handle, 
                                       char *sid, 
                                       char messageType, 
                                       char **messages) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    if (!sid) {
        sid = "0";
    }

    stateNode *state = NULL;
    CHECK_RESULT(getStateNode(tree, sid, &state));

    unsigned int messageCount = 0;
    unsigned int messagesLength = getMessagesLength(state, 
                                                    messageType, 
                                                    &messageCount);

    *messages = malloc(messagesLength * sizeof(char));
    if (!*messages) {
        return ERR_OUT_OF_MEMORY;
    }

    return serializeMessages(state, 
                             messageType, 
                             messageCount, 
                             *messages);
    
}

unsigned int getEvents(unsigned int handle, 
                       char *sid, 
                       char **messages) {

    return getMessagesForType(handle,
                              sid,
                              MESSAGE_TYPE_EVENT,
                              messages);

}

unsigned int getFacts(unsigned int handle, 
                      char *sid, 
                      char **messages) {

    return getMessagesForType(handle,
                              sid,
                              MESSAGE_TYPE_FACT,
                              messages);

}

unsigned int serializeState(stateNode *state, char **stateFact) {
    messageNode *stateFactNode = MESSAGE_NODE(state, state->factOffset);
    unsigned int stateFactLength = strlen(stateFactNode->jo.content) + 1;
    *stateFact = malloc(stateFactLength * sizeof(char));
    if (!*stateFact) {
        return ERR_OUT_OF_MEMORY;
    }
    memcpy(*stateFact, stateFactNode->jo.content, stateFactLength);

    return RULES_OK;
}

unsigned int getState(unsigned int handle, char *sid, char **state) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    if (!sid) {
        sid = "0";
    }

    stateNode *node = NULL;
    CHECK_RESULT(getStateNode(tree, sid, &node));

    return serializeState(node, state);
}

unsigned int deleteState(unsigned int handle, char *sid) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    if (!sid) {
        sid = "0";
    }
    unsigned int sidHash = fnv1Hash32(sid, strlen(sid));
    unsigned int nodeOffset;

    GET_FIRST(stateNode, 
             tree->stateIndex, 
             MAX_STATE_INDEX_LENGTH, 
             ((ruleset*)tree)->statePool, 
             sidHash, 
             nodeOffset);

    if (nodeOffset == UNDEFINED_HASH_OFFSET) {
      return ERR_SID_NOT_FOUND;
    }

    stateNode *node = STATE_NODE(tree, nodeOffset); 
    return deleteStateNode(tree, node);
}
