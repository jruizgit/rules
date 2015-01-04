
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rules.h"
#include "net.h"
#include "json.h"

#define MAX_EVENT_PROPERTIES 64
#define MAX_RESULT_NODES 32
#define MAX_NODE_RESULTS 16
#define MAX_STACK_SIZE 64

#define COMP_BOOL_BOOL 0x0404
#define COMP_BOOL_INT 0x0402
#define COMP_BOOL_DOUBLE 0x0403
#define COMP_BOOL_STRING 0x0401
#define COMP_INT_BOOL 0x0204
#define COMP_INT_INT 0x0202
#define COMP_INT_DOUBLE 0x0203
#define COMP_INT_STRING 0x0201
#define COMP_DOUBLE_BOOL 0x0304
#define COMP_DOUBLE_INT 0x0302
#define COMP_DOUBLE_DOUBLE 0x0303
#define COMP_DOUBLE_STRING 0x0301
#define COMP_STRING_BOOL 0x0104
#define COMP_STRING_INT 0x0102
#define COMP_STRING_DOUBLE 0x0103
#define COMP_STRING_STRING 0x0101

#define ACTION_ASSERT_MESSAGE_IMMEDIATE 0
#define ACTION_ASSERT_FIRST_MESSAGE 1
#define ACTION_ASSERT_MESSAGE 2
#define ACTION_ASSERT_LAST_MESSAGE 3
#define ACTION_ASSERT_TIMER 4

typedef struct actionContext {
    void *rulesBinding;
    redisReply *reply;
} actionContext;

typedef struct jsonResult {
    unsigned int hash;
    char *firstName;
    char *lastName;
    char *messages[MAX_MESSAGE_BATCH];
    unsigned short messagesLength;
    unsigned char childrenCount;
    unsigned char children[MAX_NODE_RESULTS];
} jsonResult;

static unsigned int handleEvent(ruleset *tree, 
                                char *state,
                                char *message, 
                                unsigned short actionType, 
                                unsigned char ignoreStaleState,
                                unsigned short *commandCount,
                                void **rulesBinding);

static unsigned char compareBool(unsigned char left, 
                                 unsigned char right, 
                                 unsigned char op) {
    switch(op) {
        case OP_LT:
            return (left < right);
        case OP_LTE:
            return 1;
        case OP_GT:
            return (left > right);
        case OP_GTE: 
            return 1;
        case OP_EQ:
            return (left == right);
        case OP_NEQ:
            return (left != right);
    }

    return 0;
}

static unsigned char compareInt(long left, 
                                long right, 
                                unsigned char op) {
    switch(op) {
        case OP_LT:
            return (left < right);
        case OP_LTE:
            return (left <= right);
        case OP_GT:
            return (left > right);
        case OP_GTE: 
            return (left >= right);
        case OP_EQ:
            return (left == right);
        case OP_NEQ:
            return (left != right);
    }

    return 0;
}

static unsigned char compareDouble(double left, 
                                   double right, 
                                   unsigned char op) {
    switch(op) {
        case OP_LT:
            return (left < right);
        case OP_LTE:
            return (left <= right);
        case OP_GT:
            return (left > right);
        case OP_GTE: 
            return (left >= right);
        case OP_EQ:
            return (left == right);
        case OP_NEQ:
            return (left != right);
    }

    return 0;
}

static unsigned char compareString(char *leftFirst, 
                                   unsigned short leftLength, 
                                   char *right, 
                                   unsigned char op) {
    char temp = leftFirst[leftLength];
    leftFirst[leftLength] = '\0';
    int result = strcmp(leftFirst, right);
    leftFirst[leftLength] = temp;
    switch(op) {
        case OP_LT:
            return (result < 0);
        case OP_LTE:
            return (result <= 0);
        case OP_GT:
            return (result > 0);
        case OP_GTE: 
            return (result >= 0);
        case OP_EQ:
            return (result == 0);
        case OP_NEQ:
            return (result != 0);
    }

    return 0;
}

static unsigned char compareStringProperty(char *left, 
                                           char *rightFirst, 
                                           unsigned short rightLength, 
                                           unsigned char op) {
    char temp = rightFirst[rightLength];
    rightFirst[rightLength] = '\0';
    int result = strcmp(left, rightFirst);
    rightFirst[rightLength] = temp;
    switch(op) {
        case OP_LT:
            return (result < 0);
        case OP_LTE:
            return (result <= 0);
        case OP_GT:
            return (result > 0);
        case OP_GTE: 
            return (result >= 0);
        case OP_EQ:
            return (result == 0);
        case OP_NEQ:
            return (result != 0);
    }

    return 0;
}

static unsigned char compareStringAndStringProperty(char *leftFirst, 
                                                    unsigned short leftLength, 
                                                    char *rightFirst, 
                                                    unsigned short rightLength,
                                                    unsigned char op) {
    
    char rightTemp = rightFirst[rightLength];
    rightFirst[rightLength] = '\0';        
    char leftTemp = leftFirst[leftLength];
    leftFirst[leftLength] = '\0';
    int result = strcmp(leftFirst, rightFirst);
    rightFirst[rightLength] = rightTemp;
    leftFirst[leftLength] = leftTemp;

    switch(op) {
        case OP_LT:
            return (result < 0);
        case OP_LTE:
            return (result <= 0);
        case OP_GT:
            return (result > 0);
        case OP_GTE: 
            return (result >= 0);
        case OP_EQ:
            return (result == 0);
        case OP_NEQ:
            return (result != 0);
    }

    return 0;

}

static unsigned int handleAction(ruleset *tree, 
                                 char *sid, 
                                 char *mid,
                                 char *message,
                                 jsonProperty *allProperties,
                                 unsigned int propertiesLength,
                                 char *prefix, 
                                 node *node, 
                                 unsigned short actionType, 
                                 unsigned short *commandCount,
                                 void **rulesBinding) {
    *commandCount = *commandCount + 1;
    unsigned int result;
    switch(actionType) {
        case ACTION_ASSERT_MESSAGE_IMMEDIATE:
            result = resolveBinding(tree, 
                                    sid, 
                                    rulesBinding);
            if (result != RULES_OK) {
                return result;
            }

            return assertMessageImmediate(*rulesBinding, 
                                          prefix, 
                                          sid, 
                                          message, 
                                          allProperties,
                                          propertiesLength,
                                          node->value.c.index);
        
        case ACTION_ASSERT_FIRST_MESSAGE:
            result = resolveBinding(tree, 
                                    sid, 
                                    rulesBinding);
            if (result != RULES_OK) {
                return result;
            }

            return assertFirstMessage(*rulesBinding, 
                                      prefix, 
                                      sid, 
                                      message,
                                      allProperties,
                                      propertiesLength);
        
        case ACTION_ASSERT_MESSAGE:
            return assertMessage(*rulesBinding, 
                                 prefix, 
                                 sid, 
                                 message,
                                 allProperties,
                                 propertiesLength);

        case ACTION_ASSERT_LAST_MESSAGE:
            return assertLastMessage(*rulesBinding, 
                                     prefix, 
                                     sid, 
                                     message, 
                                     allProperties,
                                     propertiesLength,
                                     node->value.c.index, 
                                     *commandCount);

        case ACTION_ASSERT_TIMER:
            return assertTimer(*rulesBinding, 
                               prefix, 
                               sid, 
                               message, 
                               allProperties,
                               propertiesLength,
                               node->value.c.index);
    }
    
    return RULES_OK;
}

static unsigned int handleBeta(ruleset *tree, 
                               char *sid,
                               char *mid,
                               char *message, 
                               jsonProperty *allProperties,
                               unsigned int propertiesLength,
                               node *betaNode,
                               unsigned short actionType, 
                               unsigned short *commandCount,
                               void **rulesBinding) {
    int prefixLength = 0;
    node *currentNode = betaNode;
    while (currentNode != NULL) {
        int nameLength = strlen(&tree->stringPool[currentNode->nameOffset]);
        prefixLength += nameLength + 1;

        if (currentNode->type == NODE_ACTION) {
            currentNode = NULL;
        } else {
            currentNode = &tree->nodePool[currentNode->value.b.nextOffset];
        }
    }

    node *actionNode;
    char prefix[prefixLength];
    char *currentPrefix = prefix;
    currentNode = betaNode;
    while (currentNode != NULL) {
        char *name = &tree->stringPool[currentNode->nameOffset];
        int nameLength = strlen(&tree->stringPool[currentNode->nameOffset]);
        strncpy(currentPrefix, name, nameLength);
        
        if (currentNode->type == NODE_ACTION) {
            currentPrefix[nameLength] = '\0';
            actionNode = currentNode;
            currentNode = NULL;
        }
        else {
            currentPrefix[nameLength] = '!';
            currentPrefix = &currentPrefix[nameLength + 1];
            currentNode = &tree->nodePool[currentNode->value.b.nextOffset];
        }
    }
    return handleAction(tree, 
                        sid, 
                        mid,
                        message,
                        allProperties,
                        propertiesLength,
                        prefix, 
                        actionNode, 
                        actionType, 
                        commandCount, 
                        rulesBinding);
}

static unsigned int isMatch(ruleset *tree,
                            char *sid,
                            char *message,
                            jsonProperty *currentProperty, 
                            alpha *currentAlpha,
                            unsigned short actionType,
                            unsigned char ignoreStaleState, 
                            unsigned char *propertyMatch,
                            void **rulesBinding) {
    unsigned char alphaOp = currentAlpha->operator;
    unsigned char propertyType = currentProperty->type;
    unsigned int result = RULES_OK;
    
    *propertyMatch = 0;
    if (alphaOp == OP_EX) {
        *propertyMatch = 1;
        return RULES_OK;
    }
    
    rehydrateProperty(currentProperty, message);
    jsonProperty *rightProperty;
    jsonProperty rightValue;
    char *rightState = NULL;
    if (currentAlpha->right.type == JSON_STATE_PROPERTY) {
        if (currentAlpha->right.value.property.idOffset) {
            sid = &tree->stringPool[currentAlpha->right.value.property.idOffset];
        }
        
        result = fetchStateProperty(tree, 
                                    sid, 
                                    currentAlpha->right.value.property.hash, 
                                    currentAlpha->right.value.property.time, 
                                    ignoreStaleState,
                                    &rightState,
                                    &rightProperty);
    } else {
        rightProperty = &rightValue;
        rightValue.type = currentAlpha->right.type;
        switch(rightValue.type) {
            case JSON_BOOL:
                rightValue.value.b = currentAlpha->right.value.b;
                break;
            case JSON_INT:
                rightValue.value.i = currentAlpha->right.value.i;
                break;
            case JSON_DOUBLE:
                rightValue.value.d = currentAlpha->right.value.d;
                break;
            case JSON_STRING:
                rightState = &tree->stringPool[currentAlpha->right.value.stringOffset];
                rightValue.valueOffset = 0;
                rightValue.valueLength = strlen(rightState);
                break;
        }
    }
        
    if ((result == ERR_STATE_NOT_LOADED || result == ERR_STALE_STATE) && !ignoreStaleState) {
        if (actionType == ACTION_ASSERT_MESSAGE ||
            actionType == ACTION_ASSERT_TIMER) {
            result = rollbackCommands(tree);
            if (result != RULES_OK) {
                return result;
            }

            result = refreshState(tree,sid);
        } else {
            result = refreshState(tree, sid);
        }   

        return result != RULES_OK ? result : ERR_NEED_RETRY;
    }

    if (result != RULES_OK) {
        return result != ERR_PROPERTY_NOT_FOUND ? result : RULES_OK;
    }
    
    int leftLength;
    int rightLength;
    unsigned short type = propertyType << 8;
    type = type + rightProperty->type;
    switch(type) {
        case COMP_BOOL_BOOL:
            *propertyMatch = compareBool(currentProperty->value.b, rightProperty->value.b, alphaOp);
            break;
        case COMP_BOOL_INT: 
            *propertyMatch = compareInt(currentProperty->value.b, rightProperty->value.i, alphaOp);
            break;
        case COMP_BOOL_DOUBLE: 
            *propertyMatch = compareDouble(currentProperty->value.b, rightProperty->value.d, alphaOp);
            break;
        case COMP_BOOL_STRING:
            if (currentProperty->value.b) {
                *propertyMatch = compareStringProperty("true",
                                                       rightState + rightProperty->valueOffset, 
                                                       rightProperty->valueLength,
                                                       alphaOp);
            }
            else {
                *propertyMatch = compareStringProperty("false",
                                                       rightState + rightProperty->valueOffset, 
                                                       rightProperty->valueLength,
                                                       alphaOp);
            }
            
            break;
        case COMP_INT_BOOL:
            *propertyMatch = compareInt(currentProperty->value.i, rightProperty->value.b, alphaOp);
            break;
        case COMP_INT_INT: 
            *propertyMatch = compareInt(currentProperty->value.i, rightProperty->value.i, alphaOp);
            break;
        case COMP_INT_DOUBLE: 
            *propertyMatch = compareDouble(currentProperty->value.i, rightProperty->value.d, alphaOp);
            break;
        case COMP_INT_STRING:
            {
                rightLength = rightProperty->valueLength + 1;
                char leftStringInt[rightLength];
                snprintf(leftStringInt, rightLength, "%ld", currentProperty->value.i);
                *propertyMatch = compareStringProperty(leftStringInt, 
                                                       rightState + rightProperty->valueOffset,
                                                       rightProperty->valueLength, 
                                                       alphaOp);
            }
            break;
        case COMP_DOUBLE_BOOL:
            *propertyMatch = compareDouble(currentProperty->value.i, rightProperty->value.b, alphaOp);
            break;
        case COMP_DOUBLE_INT: 
            *propertyMatch = compareDouble(currentProperty->value.i, rightProperty->value.i, alphaOp);
            break;
        case COMP_DOUBLE_DOUBLE: 
            *propertyMatch = compareDouble(currentProperty->value.i, rightProperty->value.d, alphaOp);
            break;
        case COMP_DOUBLE_STRING:
            {
                rightLength = rightProperty->valueLength + 1;
                char leftStringDouble[rightLength];
                snprintf(leftStringDouble, rightLength, "%f", currentProperty->value.d);
                *propertyMatch = compareStringProperty(leftStringDouble,
                                                       rightState + rightProperty->valueOffset,
                                                       rightProperty->valueLength, 
                                                       alphaOp);
            }
            break;
        case COMP_STRING_BOOL:
            if (rightProperty->value.b) {
                *propertyMatch = compareString(message + currentProperty->valueOffset, 
                                               currentProperty->valueLength, 
                                               "true", 
                                               alphaOp);
            }
            else {
                *propertyMatch = compareString(message + currentProperty->valueOffset, 
                                               currentProperty->valueLength, 
                                               "false", 
                                               alphaOp);
            }
            break;
        case COMP_STRING_INT: 
            {
                leftLength = currentProperty->valueLength + 1;
                char rightStringInt[leftLength];
                snprintf(rightStringInt, leftLength, "%ld", rightProperty->value.i);
                *propertyMatch = compareString(message + currentProperty->valueOffset, 
                                               currentProperty->valueLength, 
                                               rightStringInt, 
                                               alphaOp);
            }
            break;
        case COMP_STRING_DOUBLE: 
            {
                leftLength = currentProperty->valueLength + 1;
                char rightStringDouble[leftLength];
                snprintf(rightStringDouble, leftLength, "%f", rightProperty->value.d);
                *propertyMatch = compareString(message + currentProperty->valueOffset, 
                                               currentProperty->valueLength, 
                                               rightStringDouble, 
                                               alphaOp);
            }
            break;
        case COMP_STRING_STRING:
            *propertyMatch = compareStringAndStringProperty(message + currentProperty->valueOffset, 
                                                            currentProperty->valueLength, 
                                                            rightState + rightProperty->valueOffset,
                                                            rightProperty->valueLength,
                                                            alphaOp);
            break;
    }
    
    return result;
}

static unsigned int handleAlpha(ruleset *tree, 
                                char *sid, 
                                char *mid,
                                char *message,
                                jsonProperty *allProperties,
                                unsigned int propertiesLength,
                                alpha *alphaNode, 
                                unsigned short actionType,
                                unsigned char ignoreStaleState, 
                                unsigned short *commandCount,
                                void **rulesBinding) {
    unsigned int result = ERR_EVENT_NOT_HANDLED;
    unsigned short top = 1;
    unsigned int entry;
    alpha *stack[MAX_STACK_SIZE];
    stack[0] = alphaNode;
    alpha *currentAlpha;
    while (top) {
        --top;
        currentAlpha = stack[top];
        if (currentAlpha->nextListOffset) {
            unsigned int *nextList = &tree->nextPool[currentAlpha->nextListOffset];
            for (entry = 0; nextList[entry] != 0; ++entry) {
                node *listNode = &tree->nodePool[nextList[entry]];
                char exists = 0;
                for(unsigned int propertyIndex = 0; propertyIndex < propertiesLength; ++propertyIndex) {
                    if (listNode->value.a.hash == allProperties[propertyIndex].hash) {
                        exists = 1;
                        break;
                    }
                }

                if (!exists) {
                    if (top == MAX_STACK_SIZE) {
                        return ERR_MAX_STACK_SIZE;
                    }
                    
                    stack[top] = &listNode->value.a; 
                    ++top;
                }
            }
        }

        if (currentAlpha->nextOffset) {
            unsigned int *nextHashset = &tree->nextPool[currentAlpha->nextOffset];
            for(unsigned int propertyIndex = 0; propertyIndex < propertiesLength; ++propertyIndex) {
                jsonProperty *currentProperty = &allProperties[propertyIndex];
                for (entry = currentProperty->hash & HASH_MASK; nextHashset[entry] != 0; entry = (entry + 1) % NEXT_BUCKET_LENGTH) {
                    node *hashNode = &tree->nodePool[nextHashset[entry]];
                    if (currentProperty->hash == hashNode->value.a.hash) {
                        if (hashNode->value.a.right.type == JSON_EVENT_PROPERTY) {
                            if (top == MAX_STACK_SIZE) {
                                return ERR_MAX_STACK_SIZE;
                            }

                            stack[top] = &hashNode->value.a; 
                            ++top;
                        } else {
                            unsigned char match = 0;
                            unsigned int mresult = isMatch(tree, 
                                                           sid, 
                                                           message,
                                                           currentProperty, 
                                                           &hashNode->value.a,
                                                           actionType,
                                                           ignoreStaleState,
                                                           &match,
                                                           rulesBinding);
                            if (mresult != RULES_OK){
                                return mresult;
                            }
                            
                            if (match) {
                                if (top == MAX_STACK_SIZE) {
                                    return ERR_MAX_STACK_SIZE;
                                }

                                stack[top] = &hashNode->value.a; 
                                ++top;
                            }
                        }
                    }
                }
            }   
        }

        if (currentAlpha->betaListOffset) {
            unsigned int *betaList = &tree->nextPool[currentAlpha->betaListOffset];
            for (unsigned int entry = 0; betaList[entry] != 0; ++entry) {
                result = handleBeta(tree, 
                                    sid, 
                                    mid, 
                                    message,
                                    allProperties,
                                    propertiesLength,
                                    &tree->nodePool[betaList[entry]],  
                                    actionType, 
                                    commandCount,
                                    rulesBinding);
                if (result != RULES_OK) {
                    return result;
                }
            }
        }
    }

    return result;
}

static unsigned int getId(jsonProperty *allProperties, 
                          unsigned int idIndex, 
                          jsonProperty **idProperty, 
                          int *idLength) {
    jsonProperty *currentProperty;
    if (idIndex == UNDEFINED_INDEX) {
        return ERR_NO_ID_DEFINED;
    }

    currentProperty = &allProperties[idIndex];
    *idLength = currentProperty->valueLength;
    switch(currentProperty->type) {
        case JSON_INT:
        case JSON_DOUBLE:
            *idLength = *idLength + 1;
        case JSON_STRING:
            break;
        default:
            return ERR_INVALID_ID;
    }

    *idProperty = currentProperty;
    return RULES_OK;
}

static unsigned int handleEventCore(ruleset *tree,
                                    char *state,
                                    char *message, 
                                    jsonProperty *properties, 
                                    unsigned int propertiesLength, 
                                    unsigned int midIndex, 
                                    unsigned int sidIndex, 
                                    unsigned short actionType, 
                                    unsigned char ignoreStaleState,
                                    unsigned short *commandCount,
                                    void **rulesBinding) {
    jsonProperty *idProperty;
    int idLength;
    int result = getId(properties, midIndex, &idProperty, &idLength);
    if (result != RULES_OK) {
        return result;
    }

    char mid[idLength + 1];
    strncpy(mid, message + idProperty->valueOffset, idLength);
    mid[idLength] = '\0';
    
    result = getId(properties, sidIndex, &idProperty, &idLength);
    if (result != RULES_OK) {
        return result;
    }

    char sid[idLength + 1];
    strncpy(sid, message + idProperty->valueOffset, idLength);
    sid[idLength] = '\0';

    if (state) {
        if (!*rulesBinding) {
            result = resolveBinding(tree, sid, rulesBinding);
            if (result != RULES_OK) {
                return result;
            }
        }

        if (actionType == ACTION_ASSERT_MESSAGE_IMMEDIATE) {
            result = storeSessionImmediate(*rulesBinding, sid, state);
        }
        else {
            *commandCount = *commandCount + 1;
            result = storeSession(*rulesBinding, sid, state);
        }

        if (result != RULES_OK) {
            return result;
        }
    }

    result =  handleAlpha(tree, 
                          sid, 
                          mid,
                          message, 
                          properties, 
                          propertiesLength,
                          &tree->nodePool[NODE_M_OFFSET].value.a, 
                          actionType, 
                          ignoreStaleState,
                          commandCount,
                          rulesBinding);

    if (result == ERR_NEW_SESSION) {
        char stateMessage[36 + idLength];
        int randomMid = rand();
        if (idProperty->type == JSON_STRING) {
            snprintf(stateMessage, 36 + idLength, "{\"id\":%d, \"sid\":\"%s\", \"$s\":1}", randomMid, sid);
        }
        else {
            snprintf(stateMessage, 36 + idLength, "{\"id\":%d, \"sid\":%s, \"$s\":1}", randomMid, sid);
        }
        result = handleEvent(tree, 
                             NULL,
                             stateMessage,  
                             ACTION_ASSERT_MESSAGE_IMMEDIATE,
                             ignoreStaleState,
                             commandCount,
                             rulesBinding);
        if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
            return result;
        }

        return RULES_OK;
    } 

    return result;
}

static unsigned int handleEvent(ruleset *tree,
                                char *state, 
                                char *message, 
                                unsigned short actionType, 
                                unsigned char ignoreStaleState,
                                unsigned short *commandCount,
                                void **rulesBinding) {
    char *next;
    unsigned int propertiesLength = 0;
    unsigned int midIndex = UNDEFINED_INDEX;
    unsigned int sidIndex = UNDEFINED_INDEX;
    jsonProperty properties[MAX_EVENT_PROPERTIES];
    int result = constructObject(NULL, 
                                 message, 
                                 0, 
                                 MAX_EVENT_PROPERTIES,
                                 properties, 
                                 &propertiesLength, 
                                 &midIndex, 
                                 &sidIndex, 
                                 &next);
    if (result != RULES_OK) {
        return result;
    }

    return handleEventCore(tree,
                           state, 
                           message, 
                           properties, 
                           propertiesLength, 
                           midIndex, 
                           sidIndex,  
                           actionType,
                           ignoreStaleState,
                           commandCount,
                           rulesBinding);
}

static unsigned int handleEvents(void *handle, 
                                 char *messages, 
                                 unsigned char ignoreStaleState,
                                 unsigned int *resultsLength,  
                                 unsigned int **results) {
    unsigned int messagesLength = 64;
    unsigned int *resultsArray = malloc(sizeof(unsigned int) * messagesLength);
    if (!resultsArray) {
        return ERR_OUT_OF_MEMORY;
    }

    *resultsLength = 0;
    unsigned short commandCount = 0;
    unsigned short actionType = ACTION_ASSERT_FIRST_MESSAGE;
    unsigned int result;
    void *rulesBinding = NULL;
    jsonProperty properties0[MAX_EVENT_PROPERTIES];
    jsonProperty properties1[MAX_EVENT_PROPERTIES];
    jsonProperty *currentProperties = NULL;
    unsigned int currentPropertiesLength = 0;
    unsigned int currentPropertiesMidIndex = UNDEFINED_INDEX;
    unsigned int currentPropertiesSidIndex = UNDEFINED_INDEX;
    unsigned int nextPropertiesLength = 0;
    unsigned int nextPropertiesMidIndex = UNDEFINED_INDEX;
    unsigned int nextPropertiesSidIndex = UNDEFINED_INDEX;
    
    jsonProperty *nextProperties = properties0;
    char *first = NULL;
    char *last = NULL;
    char *next = NULL;
    char *nextFirst = messages;
    char lastTemp;

    while (nextFirst[0] != '{' && nextFirst[0] != '\0' ) {
        ++nextFirst;
    }

    while (constructObject(NULL, 
                           nextFirst, 
                           0,
                           MAX_EVENT_PROPERTIES,
                           nextProperties, 
                           &nextPropertiesLength, 
                           &nextPropertiesMidIndex,
                           &nextPropertiesSidIndex,
                           &next) == RULES_OK) {
        if (currentProperties) {

            result = handleEventCore(handle,
                                     NULL, 
                                     first, 
                                     currentProperties, 
                                     currentPropertiesLength,
                                     currentPropertiesMidIndex,
                                     currentPropertiesSidIndex, 
                                     actionType, 
                                     ignoreStaleState,
                                     &commandCount,
                                     &rulesBinding);
            last[0] = lastTemp;
            if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
                free(resultsArray);
                return result;
            }

            if (result == RULES_OK) {
                actionType = ACTION_ASSERT_MESSAGE;
            }

            if (*resultsLength >= messagesLength) {
                messagesLength = messagesLength * 2;
                resultsArray = realloc(resultsArray, sizeof(unsigned int) * messagesLength);
                if (!resultsArray) {
                    return ERR_OUT_OF_MEMORY;
                }
            }

            resultsArray[*resultsLength] = result;
            ++*resultsLength;
        }

        while (next[0] != ',' && next[0] != ']' ) {
            ++next;
        }
        last = next;
        lastTemp = next[0];
        next[0] = '\0';
        currentProperties = nextProperties;
        currentPropertiesLength = nextPropertiesLength;
        currentPropertiesMidIndex = nextPropertiesMidIndex;
        currentPropertiesSidIndex = nextPropertiesSidIndex;
        first = nextFirst;
        nextFirst = next + 1;
        if (nextProperties == properties0) {
            nextProperties = properties1;
        } else {
            nextProperties = properties0;
        }

        nextPropertiesLength = 0;
        nextPropertiesMidIndex = UNDEFINED_INDEX;
        nextPropertiesSidIndex = UNDEFINED_INDEX;
    }

    if (actionType == ACTION_ASSERT_FIRST_MESSAGE) {
        actionType = ACTION_ASSERT_MESSAGE_IMMEDIATE;
    } else {
        actionType = ACTION_ASSERT_LAST_MESSAGE;
    }

    result = handleEventCore(handle, 
                             NULL,
                             first, 
                             currentProperties, 
                             currentPropertiesLength,
                             currentPropertiesMidIndex,
                             currentPropertiesSidIndex, 
                             actionType, 
                             ignoreStaleState,
                             &commandCount,
                             &rulesBinding);
    last[0] = lastTemp;
    if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
        free(resultsArray);
        return result;
    }

    if (*resultsLength >= messagesLength) {
        ++messagesLength;
        resultsArray = realloc(resultsArray, sizeof(unsigned int) * messagesLength);
        if (!resultsArray) {
            return ERR_OUT_OF_MEMORY;
        }
    }

    resultsArray[*resultsLength] = result;
    ++*resultsLength;
    *results = resultsArray;
    return RULES_OK;
}

static unsigned int handleState(ruleset *tree, 
                                char *state, 
                                unsigned short actionType, 
                                unsigned char ignoreStaleState,
                                void *rulesBinding,
                                unsigned short *commandCount) {

    int stateLength = strlen(state);
    if (stateLength < 2) {
        return ERR_PARSE_OBJECT;
    }

    if (state[0] != '{') {
        return ERR_PARSE_OBJECT;
    }

    char *stateMessagePostfix = state + 1;
    char stateMessage[30 + stateLength - 1];
    int randomMid = rand();
    snprintf(stateMessage, 30 + stateLength - 1, "{\"id\":%d, \"$s\":1, %s", randomMid, stateMessagePostfix);
    unsigned int result = handleEvent(tree, 
                                      state,
                                      stateMessage,  
                                      actionType,
                                      ignoreStaleState,
                                      commandCount,
                                      &rulesBinding);

    return result;
}

unsigned int assertEvent(void *handle, char *message) {
    unsigned short commandCount = 0;
    void *rulesBinding = NULL;
    unsigned int result = ERR_NEED_RETRY;
    while (result == ERR_NEED_RETRY) {
        result = handleEvent(handle, 
                             NULL,
                             message, 
                             ACTION_ASSERT_MESSAGE_IMMEDIATE, 
                             0, 
                             &commandCount, 
                             &rulesBinding);
    }
    
    return result;
}

unsigned int assertEvents(void *handle, 
                          char *messages, 
                          unsigned int *resultsLength, 
                          unsigned int **results) {

    unsigned int result = ERR_NEED_RETRY;
    while (result == ERR_NEED_RETRY) {
        result = handleEvents(handle,
                                  messages,
                                  0,
                                  resultsLength,
                                  results);
    }
    
    return result;
}

unsigned int assertState(void *handle, char *state) {
    unsigned short commandCount = 0;
    void *rulesBinding = NULL;

    unsigned int result = ERR_NEED_RETRY;
    while(result == ERR_NEED_RETRY) {
        result = handleState(handle, 
                             state, 
                             ACTION_ASSERT_MESSAGE_IMMEDIATE, 
                             0,
                             rulesBinding, 
                             &commandCount);
    }
    
    return result;
}

unsigned int startAction(void *handle, char **state, char **messages, void **actionHandle) {
    redisReply *reply;
    void *rulesBinding;
    unsigned int result = peekAction(handle, &rulesBinding, &reply);
    if (result != RULES_OK) {
        return result;
    }

    *state = reply->element[1]->str;
    *messages = reply->element[2]->str;
    actionContext *context = malloc(sizeof(actionContext));
    context->reply = reply;
    context->rulesBinding = rulesBinding;
    *actionHandle = context;
    return RULES_OK;
}

unsigned int completeAction(void *handle, void *actionHandle, char *state) {
    unsigned short commandCount = 3;
    actionContext *context = (actionContext*)actionHandle;
    redisReply *reply = context->reply;
    void *rulesBinding = context->rulesBinding;
    unsigned int result = prepareCommands(rulesBinding);
    if (result != RULES_OK) {
        freeReplyObject(reply);
        free(actionHandle);
        return result;
    }

    result = removeAction(rulesBinding, reply->element[0]->str);
    if (result != RULES_OK) {
        freeReplyObject(reply);
        free(actionHandle);
        return result;
    }

    result = handleState(handle, 
                         state, 
                         ACTION_ASSERT_MESSAGE,  
                         1, 
                         rulesBinding, 
                         &commandCount);
    if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
        freeReplyObject(reply);
        free(actionHandle);
        return result;
    }

    result = executeCommands(rulesBinding, commandCount);    
    freeReplyObject(reply);
    free(actionHandle);
    return result;
}

unsigned int abandonAction(void *handle, void *actionHandle) {
    freeReplyObject(((actionContext*)actionHandle)->reply);
    free(actionHandle);
    return RULES_OK;
}

static unsigned int assertTimersCore(void *handle, unsigned char ignoreStaleState) {
    unsigned short commandCount = 2;
    redisReply *reply;
    void *rulesBinding;
    unsigned int result = peekTimers(handle, &rulesBinding, &reply);
    if (result != RULES_OK) {
        return result;
    }

    result = prepareCommands(rulesBinding);
    if (result != RULES_OK) {
        freeReplyObject(reply);
        return result;
    }

    for (unsigned long i = 0; i < reply->elements; ++i) {
        result = removeTimer(rulesBinding, reply->element[i]->str);
        if (result != RULES_OK) {
            freeReplyObject(reply);
            return result;
        }

        ++commandCount;
        result = handleEvent(handle, 
                             NULL,
                             reply->element[i]->str, 
                             ACTION_ASSERT_TIMER, 
                             ignoreStaleState,
                             &commandCount, 
                             &rulesBinding);
        if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
            freeReplyObject(reply);
            return result;
        }
    }

    result = executeCommands(rulesBinding, commandCount);
    freeReplyObject(reply);
    return result;
}

unsigned int assertTimers(void *handle) {
    unsigned int result = assertTimersCore(handle, 0);
    if (result == ERR_NEED_RETRY) {
        return assertTimersCore(handle, 1);
    }

    return result;
}

unsigned int startTimer(void *handle, char *sid, unsigned int duration, char *timer) {
    void *rulesBinding;
    unsigned int result = resolveBinding(handle, sid, &rulesBinding);
    if (result != RULES_OK) {
        return result;
    }

    return registerTimer(rulesBinding, duration, timer);
}