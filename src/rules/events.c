
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
#define ACTION_NEGATE_MESSAGE 4
#define ACTION_ASSERT_SESSION 5
#define ACTION_ASSERT_SESSION_IMMEDIATE 6
#define ACTION_NEGATE_SESSION 7
#define ACTION_ASSERT_TIMER 8

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

static unsigned int readLastName(char *start, 
                                 char *end, 
                                 char **first, 
                                 unsigned int *hash) {
    if (end < start) {
        return ERR_PARSE_PATH;
    }

    unsigned int currentHash = 5381;
    char *current = end;
    while (current > start) {
        currentHash = ((currentHash << 5) + currentHash) + (*current);
        if ((current - 1)[0] == '!') {
            break;
        } else {
            --current;
        }
    }

    *first = current;
    *hash = currentHash;
    return RULES_OK;
}

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

static unsigned char compareString(char* leftFirst, 
                                   unsigned short leftLength, 
                                   char* right, 
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

static unsigned char compareStringProperty(char* left, 
                                           char* rightFirst, 
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

static unsigned char compareStringAndStringProperty(char* leftFirst, 
                                                    unsigned short leftLength, 
                                                    char* rightFirst, 
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
                                 char *state, 
                                 char *prefix, 
                                 node *node, 
                                 unsigned short actionType, 
                                 unsigned short *commandCount,
                                 void **rulesBinding) {
    *commandCount = *commandCount + 1;
    unsigned int result;
    switch(actionType) {
        case ACTION_ASSERT_MESSAGE_IMMEDIATE:
            result = resolveBinding(tree, sid, rulesBinding);
            if (result != RULES_OK) {
                return result;
            }

            return assertMessageImmediate(*rulesBinding, prefix, sid, mid, state, node->value.c.index);
        case ACTION_ASSERT_FIRST_MESSAGE:
            result = resolveBinding(tree, sid, rulesBinding);
            if (result != RULES_OK) {
                return result;
            }

            return assertFirstMessage(*rulesBinding, prefix, sid, mid, state);
        case ACTION_ASSERT_MESSAGE:
            return assertMessage(*rulesBinding, prefix, sid, mid, state);
        case ACTION_ASSERT_LAST_MESSAGE:
            return assertLastMessage(*rulesBinding, prefix, sid, mid, state, node->value.c.index, *commandCount);
        case ACTION_NEGATE_MESSAGE:
            return negateMessage(*rulesBinding, prefix, sid, mid);
        case ACTION_ASSERT_SESSION:
            return assertSession(*rulesBinding, prefix, sid, state, node->value.c.index);
        case ACTION_ASSERT_SESSION_IMMEDIATE:
            result = resolveBinding(tree, sid, rulesBinding);
            if (result != RULES_OK) {
                return result;
            }

            return assertSessionImmediate(*rulesBinding, prefix, sid, state, node->value.c.index);
        case ACTION_NEGATE_SESSION:
            return negateSession(*rulesBinding, prefix, sid);
        case ACTION_ASSERT_TIMER:
            return assertTimer(*rulesBinding, prefix, sid, mid, state, node->value.c.index);
    }
    
    return RULES_OK;
}

static unsigned int handleBeta(ruleset *tree, 
                               char *sid, 
                               char *mid, 
                               char *state, 
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
    return handleAction(tree, sid, mid, state, prefix, actionNode, actionType, commandCount, rulesBinding);
}

static unsigned int isMatch(ruleset *tree,
                            char *sid,
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
    
    rehydrateProperty(currentProperty);
    jsonProperty *rightProperty;
    jsonProperty rightValue;
    if (currentAlpha->right.type == JSON_STATE_PROPERTY) {
        if (currentAlpha->right.value.property.idOffset) {
            sid = &tree->stringPool[currentAlpha->right.value.property.idOffset];
        }
        
        result = fetchStateProperty(tree, 
                                    sid, 
                                    currentAlpha->right.value.property.hash, 
                                    currentAlpha->right.value.property.time, 
                                    ignoreStaleState,
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
                rightValue.value.s = &tree->stringPool[currentAlpha->right.value.stringOffset];
                rightValue.length = strlen(rightValue.value.s);
                break;
        }
    }
        
    if ((result == ERR_STATE_NOT_LOADED || result == ERR_STALE_STATE) && !ignoreStaleState) {
        if (actionType == ACTION_ASSERT_MESSAGE ||
            actionType == ACTION_ASSERT_TIMER ||
            actionType == ACTION_ASSERT_SESSION) {
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
                                                       rightProperty->value.s, 
                                                       rightProperty->length,
                                                       alphaOp);
            }
            else {
                *propertyMatch = compareStringProperty("false",
                                                       rightProperty->value.s, 
                                                       rightProperty->length,
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
                rightLength = rightProperty->length + 1;
                char leftStringInt[rightLength];
                snprintf(leftStringInt, rightLength, "%ld", currentProperty->value.i);
                *propertyMatch = compareStringProperty(leftStringInt, 
                                                       rightProperty->value.s,
                                                       rightProperty->length, 
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
                rightLength = rightProperty->length + 1;
                char leftStringDouble[rightLength];
                snprintf(leftStringDouble, rightLength, "%f", currentProperty->value.d);
                *propertyMatch = compareStringProperty(leftStringDouble,
                                                       rightProperty->value.s,
                                                       rightProperty->length, 
                                                       alphaOp);
            }
            break;
        case COMP_STRING_BOOL:
            if (rightProperty->value.b) {
                *propertyMatch = compareString(currentProperty->value.s, 
                                               currentProperty->length, 
                                               "true", 
                                               alphaOp);
            }
            else {
                *propertyMatch = compareString(currentProperty->value.s, 
                                               currentProperty->length, 
                                               "false", 
                                               alphaOp);
            }
            break;
        case COMP_STRING_INT: 
            {
                leftLength = currentProperty->length + 1;
                char rightStringInt[leftLength];
                snprintf(rightStringInt, leftLength, "%ld", rightProperty->value.i);
                *propertyMatch = compareString(currentProperty->value.s, 
                                               currentProperty->length, 
                                               rightStringInt, 
                                               alphaOp);
            }
            break;
        case COMP_STRING_DOUBLE: 
            {
                leftLength = currentProperty->length + 1;
                char rightStringDouble[leftLength];
                snprintf(rightStringDouble, leftLength, "%f", rightProperty->value.d);
                *propertyMatch = compareString(currentProperty->value.s, 
                                               currentProperty->length, 
                                               rightStringDouble, 
                                               alphaOp);
            }
            break;
        case COMP_STRING_STRING:
            *propertyMatch = compareStringAndStringProperty(currentProperty->value.s, 
                                                            currentProperty->length, 
                                                            rightProperty->value.s,
                                                            rightProperty->length,
                                                            alphaOp);
            break;
    }
    
    return result;
}

static unsigned int handleAlpha(ruleset *tree, 
                                char *sid, 
                                char *mid, 
                                char *state, 
                                alpha *alphaNode, 
                                jsonProperty *allProperties,
                                unsigned int propertiesLength,  
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
                        unsigned char match = 0;
                        unsigned int mresult = isMatch(tree, 
                                         sid, 
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

        if (currentAlpha->betaListOffset) {
            unsigned int *betaList = &tree->nextPool[currentAlpha->betaListOffset];
            for (unsigned int entry = 0; betaList[entry] != 0; ++entry) {
                result = handleBeta(tree, 
                                    sid, 
                                    mid, 
                                    state, 
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
    *idLength = currentProperty->length;
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

static unsigned int handleState(ruleset *tree, 
                                char *state, 
                                unsigned short actionType, 
                                unsigned char store,
                                unsigned char ignoreStaleState,
                                void *rulesBinding,
                                unsigned short *commandCount) {
    char *next;
    unsigned int propertiesLength = 0;
    unsigned int dummyIndex = UNDEFINED_INDEX;
    unsigned int sidIndex = UNDEFINED_INDEX;
    jsonProperty properties[MAX_STATE_PROPERTIES];
    int result = constructObject(NULL, 
                                 state, 
                                 0, 
                                 MAX_STATE_PROPERTIES, 
                                 properties, 
                                 &propertiesLength, 
                                 &sidIndex, 
                                 &dummyIndex, 
                                 &next);
    if (result != RULES_OK) {
        return result;
    }
    
    jsonProperty *sidProperty;
    int sidLength;
    result = getId(properties, sidIndex, &sidProperty, &sidLength);
    if (result != RULES_OK) {
        return result;
    }

    char sid[sidLength + 1];
    strncpy(sid, sidProperty->value.s, sidLength);
    sid[sidLength] = '\0';
    result = handleAlpha(tree, 
                         sid, 
                         NULL, 
                         state, 
                         &tree->nodePool[NODE_S_OFFSET].value.a, properties, 
                         propertiesLength, 
                         actionType, 
                         ignoreStaleState,
                         commandCount,
                         &rulesBinding); 
    if (result == ERR_EVENT_NOT_HANDLED && store) {
        if (actionType == ACTION_ASSERT_SESSION_IMMEDIATE) {
            if (!rulesBinding) {
                result = resolveBinding(tree, sid, &rulesBinding);
                if (result != RULES_OK) {
                    return result;
                }
            }

            result = storeSessionImmediate(rulesBinding, sid, state);
        }
        else {
            *commandCount = *commandCount + 1;
            result = storeSession(rulesBinding, sid, state);
        }
    }

    return result;
}

static unsigned int handleEventCore(ruleset *tree,
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
    strncpy(mid, idProperty->value.s, idLength);
    mid[idLength] = '\0';
    
    result = getId(properties, sidIndex, &idProperty, &idLength);
    if (result != RULES_OK) {
        return result;
    }
    char sid[idLength + 1];
    strncpy(sid, idProperty->value.s, idLength);
    sid[idLength] = '\0';
    if (actionType == ACTION_NEGATE_MESSAGE) {
        result = removeMessage(*rulesBinding, mid);
        if (result != RULES_OK) {
            return result;
        }

        *commandCount = *commandCount + 1;
    }

    result =  handleAlpha(tree, 
                          sid, 
                          mid, 
                          message, 
                          &tree->nodePool[NODE_M_OFFSET].value.a, 
                          properties, 
                          propertiesLength,  
                          actionType, 
                          ignoreStaleState,
                          commandCount,
                          rulesBinding);

    if (result == ERR_NEW_SESSION) {
        char session[11 + idLength];
        strcpy(session, "{\"id\":\"");
        strncpy(session + 7, sid, idLength);
        strcpy(session + 7 + idLength, "\"}");

        result = handleState(tree, 
                             session,  
                             ACTION_ASSERT_SESSION_IMMEDIATE,
                             0,
                             ignoreStaleState,
                             *rulesBinding, 
                             commandCount);
        if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
            return result;
        }

        return RULES_OK;
    }

    return result;
}

static unsigned int handleEvent(ruleset *tree, 
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

unsigned int assertEvent(void *handle, char *message) {
    unsigned short commandCount = 0;
    void *rulesBinding = NULL;
    unsigned int result = ERR_NEED_RETRY;
    while (result == ERR_NEED_RETRY) {
        result = handleEvent(handle, 
                             message, 
                             ACTION_ASSERT_MESSAGE_IMMEDIATE, 
                             0, 
                             &commandCount, 
                             &rulesBinding);
    }
    
    return result;
}

static unsigned int handleEventsCore(void *handle, 
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

unsigned int assertEvents(void *handle, 
                          char *messages, 
                          unsigned int *resultsLength, 
                          unsigned int **results) {

    unsigned int result = ERR_NEED_RETRY;
    while (result == ERR_NEED_RETRY) {
        result = handleEventsCore(handle,
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
                             ACTION_ASSERT_SESSION_IMMEDIATE, 
                             1, 
                             0,
                             rulesBinding, 
                             &commandCount);
    }
    
    return result;
}

static unsigned int createState(redisReply *reply, char *firstSid, char *lastSid, char **state) {
    if (reply->element[2]->type == REDIS_REPLY_NIL) {        
        int idLength = lastSid - firstSid + 1;
        *state = malloc((11 + idLength) * sizeof(char));
        if (!*state) {
            return ERR_OUT_OF_MEMORY;
        }    

        strcpy(*state, "{\"id\":\"");
        strncpy(*state + 7, firstSid, idLength);
        strcpy(*state + 7 + idLength, "\"}");
    }
    else {
        *state = malloc((strlen(reply->element[2]->str) + 1) * sizeof(char));
        if (!*state) {
            return ERR_OUT_OF_MEMORY;
        }  

        strcpy(*state, reply->element[2]->str);  
    }

    return RULES_OK;
}

static unsigned int createMessages(redisReply *reply,  char **firstSid, char **lastSid, char **messages) {
    jsonResult results[MAX_RESULT_NODES];
    unsigned char nodesCount = 1;
    results[0].childrenCount = 0;
    unsigned int messagesLength = 3;
    for (unsigned long i = 3; i < reply->elements; i = i + 2) {
        char *start = reply->element[i]->str;
        char *firstName = start + strlen(start) - 1;
        char *lastName = firstName;
        unsigned int hash;
        jsonResult *current = NULL;
        while (readLastName(start, firstName, &firstName, &hash) == RULES_OK) {
            if (!current) {
                results[0].firstName = firstName;
                results[0].lastName = lastName;
                results[0].hash = hash;
                results[0].messagesLength = 0;
                current = &results[0];   
            } else {
                unsigned char found = 0;
                for (unsigned char ii = 0; ii < current->childrenCount && !found; ++ii) {
                    if (results[current->children[ii]].hash == hash) {
                        current = &results[current->children[ii]];
                        found = 1;
                    }
                }

                if (!found) {
                    if (current->childrenCount == MAX_NODE_RESULTS) {
                        return ERR_MAX_NODE_RESULTS;
                    }

                    if (nodesCount == MAX_RESULT_NODES) {
                        return ERR_MAX_RESULT_NODES;
                    }

                    current->children[current->childrenCount] = nodesCount;
                    ++current->childrenCount;
                    current = &results[nodesCount];
                    current->childrenCount = 0;
                    current->firstName = firstName;
                    char* min = strchr(firstName, '+');
                    if (!min) {
                        current->lastName = lastName;
                    }
                    else {
                        current->lastName = min - 1;   
                    }
                    current->hash = hash;
                    current->messagesLength = 0;
                    ++nodesCount;
                    messagesLength = messagesLength + current->lastName - current->firstName + 7;
                }
            }

            firstName = firstName - 2;
            lastName = firstName;
        }

        current->messages[current->messagesLength] = reply->element[i + 1]->str;
        messagesLength = messagesLength + strlen(current->messages[current->messagesLength]) + 1;
        ++current->messagesLength;
        if (current->messagesLength == 2) {
            messagesLength = messagesLength + 2;
        }
    }

    *firstSid = results[0].firstName;
    *lastSid = results[0].lastName;
    *messages = malloc(messagesLength * sizeof(char));
    if (!messages) {
        return ERR_OUT_OF_MEMORY;
    }

    char *currentPosition = *messages;
    unsigned char stack[MAX_RESULT_NODES];
    unsigned char top = 1;
    currentPosition[0] = '{';
    ++currentPosition;
    if (results[0].childrenCount) {
        stack[0] = results[0].children[0];
        while (top != 0) {
            if (stack[top - 1] == 0) {
                --top;
                if ((currentPosition - 1)[0] == ',') {
                    --currentPosition;
                }
                currentPosition[0] = '}';
                ++currentPosition;
                currentPosition[0] = ',';
                ++currentPosition;
            } else {
                jsonResult *current = &results[stack[top - 1]];
                --top;
                
                currentPosition[0] = '"';
                ++currentPosition;
                strncpy(currentPosition, current->firstName, current->lastName - current->firstName + 1);
                currentPosition = currentPosition + (current->lastName - current->firstName + 1);
                currentPosition[0] = '"';
                ++currentPosition;
                currentPosition[0] = ':';
                ++currentPosition;

                if (!current->childrenCount) {
                    if (current->messagesLength == 1) {
                        strcpy(currentPosition, current->messages[0]);
                        currentPosition = currentPosition + strlen(current->messages[0]); 
                        currentPosition[0] = ',';
                        ++currentPosition;
                    } else {
                        currentPosition[0] = '[';
                        ++currentPosition;
                        for (unsigned short i = 0; i < current->messagesLength; ++i) {
                            strcpy(currentPosition, current->messages[i]);
                            currentPosition = currentPosition + strlen(current->messages[i]); 
                            
                            if (i < (current->messagesLength - 1)) {
                                currentPosition[0] = ',';
                                ++currentPosition;
                            }
                        }
                        currentPosition[0] = ']';
                        ++currentPosition;
                        currentPosition[0] = ',';
                        ++currentPosition;
                    }
                }
                else {
                    currentPosition[0] = '{';
                    ++currentPosition;
                    stack[top] = 0;
                    ++top;

                    for (int i = 0; i < current->childrenCount; ++i) {
                        stack[top] = current->children[i];
                        ++top;
                    }
                }
            }
        }

        if ((currentPosition - 1)[0] == ',') {
            --currentPosition;
        }
    }
    currentPosition[0] = '}';
    ++currentPosition;
    currentPosition[0] = '\0';
    return RULES_OK;
}

unsigned int startAction(void *handle, char **state, char **messages, void **actionHandle) {
    redisReply *reply;
    char *firstSid;
    char *lastSid;
    void *rulesBinding;
    unsigned int result = peekAction(handle, &rulesBinding, &reply);
    if (result != RULES_OK) {
        return result;
    }

    result = createMessages(reply, &firstSid, &lastSid, messages);
    if (result != RULES_OK) {
        return result;
    }

    result = createState(reply, firstSid, lastSid, state);
    if (result != RULES_OK) {
        return result;
    } 

    actionContext *context = malloc(sizeof(actionContext));
    context->reply = reply;
    context->rulesBinding = rulesBinding;
    *actionHandle = context;
    return RULES_OK;
}

unsigned int completeAction(void *handle, void *actionHandle, char *state) {
    unsigned short commandCount = 4;
    actionContext *context = (actionContext*)actionHandle;
    redisReply *reply = context->reply;
    void *rulesBinding = context->rulesBinding;
    unsigned int result = prepareCommands(rulesBinding);
    if (result != RULES_OK) {
        return result;
    }
    
    result = removeAction(rulesBinding, reply->element[0]->str);
    if (result != RULES_OK) {
        return result;
    }

    if (reply->element[2]->type != REDIS_REPLY_NIL) {
        result = handleState(handle, 
                             reply->element[2]->str, 
                             ACTION_NEGATE_SESSION, 
                             0, 
                             1, 
                             rulesBinding, 
                             &commandCount);
        if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
            return result;
        }
    }
   
    for (unsigned long i = 4; i < reply->elements; i = i + 2) {
        if (strcmp(reply->element[i]->str, "null") != 0) {
            result = handleEvent(handle, 
                                 reply->element[i]->str, 
                                 ACTION_NEGATE_MESSAGE, 
                                 1, 
                                 &commandCount, 
                                 &rulesBinding);
            if (result != RULES_OK) {
                return result;
            }
        }
    }

    result = handleState(handle, 
                         state, 
                         ACTION_ASSERT_SESSION,  
                         1, 
                         1, 
                         rulesBinding, 
                         &commandCount);
    if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
        return result;
    }

    result = executeCommands(rulesBinding, commandCount);    
    if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED) {
        freeReplyObject(reply);
        free(actionHandle);
    }
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