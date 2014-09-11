
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rules.h"
#include "net.h"
#include "json.h"

#define ID_HASH 5863474
#define SID_HASH 193505797
#define MAX_PROPERTIES 256
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

typedef struct jsonProperty {
    unsigned int hash;
    char *firstValue;
    char *lastValue;
    unsigned char type;
    unsigned char isMaterial;
    union { 
        long i; 
        double d; 
        unsigned char b; 
    } value;
} jsonProperty;

typedef struct jsonResult {
    unsigned int hash;
    char *firstName;
    char *lastName;
    char *messages[MAX_MESSAGE_BATCH];
    unsigned short messagesLength;
    unsigned char childrenCount;
    unsigned char children[MAX_NODE_RESULTS];
} jsonResult;

static unsigned int constructObject(char *parentName, 
                                    char *object, 
                                    jsonProperty *properties, 
                                    unsigned int *propertiesLength, 
                                    unsigned int *midIndex, 
                                    unsigned int *sidIndex, 
                                    char **next) {
    char *firstName;
    char *lastName;
    char *first;
    char *last;
    unsigned char type;
    unsigned int hash;
    int parentNameLength = (parentName ? strlen(parentName): 0);
    unsigned int result = readNextName(object, &firstName, &lastName, &hash);
    while (result == PARSE_OK) {
        readNextValue(lastName, &first, &last, &type);
        
        if (!parentName) {
            if (type == JSON_OBJECT) {
                int nameLength = lastName - firstName;
                char newParent[nameLength + 1];
                strncpy(newParent, firstName, nameLength);
                newParent[nameLength] = '\0';
                return constructObject(newParent, first, properties, propertiesLength, midIndex, sidIndex, next);
            }
        } else {
            int nameLength = lastName - firstName;
            int fullNameLength = nameLength + parentNameLength + 1; 
            char fullName[fullNameLength + 1];
            strncpy(fullName, firstName, nameLength);
            fullName[nameLength] = '.';
            strncpy(&fullName[nameLength + 1], parentName, parentNameLength);
            fullName[fullNameLength] = '\0';
            hash = djbHash(fullName, fullNameLength);
            if (type == JSON_OBJECT) {
                return constructObject(fullName, first, properties, propertiesLength, midIndex, sidIndex, next);
            }
        }

        jsonProperty *property = &properties[*propertiesLength];
        if (hash == ID_HASH) {
            *midIndex = *propertiesLength;
        } else if (hash == SID_HASH) {
            *sidIndex = *propertiesLength;
        }

        *propertiesLength = *propertiesLength + 1;
        if (*propertiesLength == MAX_PROPERTIES) {
            return ERR_EVENT_MAX_PROPERTIES;
        }
        
        property->isMaterial = 0;
        property->hash = hash;
        property->firstValue = first;
        property->lastValue = last;
        property->type = type;
        *next = last;
        result = readNextName(last, &firstName, &lastName, &hash);
    }
 
    return (result == PARSE_END ? RULES_OK: result);
}

static unsigned int readLastName(char *start, char *end, char **first, unsigned int *hash) {
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

static unsigned char compareBool(unsigned char left, unsigned char right, unsigned char op) {
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

static unsigned char compareInt(long left, long right, unsigned char op) {
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

static unsigned char compareDouble(double left, double right, unsigned char op) {
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

static unsigned char compareString(char* leftFirst, char* leftLast, char* right, unsigned char op) {
    char temp = leftLast[0];
    leftLast[0] = '\0';
    int result = strcmp(leftFirst, right);
    leftLast[0] = temp;
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
                                 void **rulesBinding, 
                                 unsigned short actionType, 
                                 unsigned short *commandCount) {
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
                               void **rulesBinding, 
                               unsigned short actionType, 
                               unsigned short *commandCount) {
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
    return handleAction(tree, sid, mid, state, prefix, actionNode, rulesBinding, actionType, commandCount);
}

static unsigned char isMatch(ruleset *tree, jsonProperty *currentProperty, alpha *currentAlpha) {
    char *propertyLast = currentProperty->lastValue;
    char *propertyFirst = currentProperty->firstValue;
    unsigned char alphaOp = currentAlpha->operator;
    unsigned char propertyType = currentProperty->type;
    unsigned char propertyMatch = 0;
    char temp;
    
    if (alphaOp == OP_EX) {
        return 1;
    }
    
    if (!currentProperty->isMaterial) {
        switch(propertyType) {
            case JSON_INT:
                ++propertyLast;
                temp = propertyLast[0];
                propertyLast[0] = '\0';
                currentProperty->value.i = atol(propertyFirst);
                propertyLast[0] = temp;
                break;
            case JSON_DOUBLE:
                ++propertyLast;
                temp = propertyLast[0];
                propertyLast[0] = '\0';
                currentProperty->value.i = atof(propertyFirst);
                propertyLast[0] = temp;
                break;
            case JSON_BOOL:
                ++propertyLast;
                unsigned int leftLength = propertyLast - propertyFirst;
                unsigned char b = 1;
                if (leftLength == 5 && strncmp("false", propertyFirst, 5)) {
                    b = 0;
                }
                currentProperty->value.b = b;
                break;
        }

        currentProperty->isMaterial = 1;
    }

    unsigned short type = propertyType << 8;
    type = type + currentAlpha->right.type;
    int leftLength;
    switch(type) {
        case COMP_BOOL_BOOL:
            propertyMatch = compareBool(currentProperty->value.b, currentAlpha->right.value.b, alphaOp);
            break;
        case COMP_BOOL_INT: 
            propertyMatch = compareInt(currentProperty->value.b, currentAlpha->right.value.i, alphaOp);
            break;
        case COMP_BOOL_DOUBLE: 
            propertyMatch = compareDouble(currentProperty->value.b, currentAlpha->right.value.d, alphaOp);
            break;
        case COMP_BOOL_STRING:
            propertyMatch = compareString(propertyFirst, 
                                          propertyLast, 
                                          &tree->stringPool[currentAlpha->right.value.stringOffset], 
                                          alphaOp);
            break;
        case COMP_INT_BOOL:
            propertyMatch = compareInt(currentProperty->value.i, currentAlpha->right.value.b, alphaOp);
            break;
        case COMP_INT_INT: 
            propertyMatch = compareInt(currentProperty->value.i, currentAlpha->right.value.i, alphaOp);
            break;
        case COMP_INT_DOUBLE: 
            propertyMatch = compareDouble(currentProperty->value.i, currentAlpha->right.value.d, alphaOp);
            break;
        case COMP_INT_STRING:
            propertyMatch = compareString(propertyFirst, 
                                          propertyLast, 
                                          &tree->stringPool[currentAlpha->right.value.stringOffset], 
                                          alphaOp);
            break;
        case COMP_DOUBLE_BOOL:
            propertyMatch = compareDouble(currentProperty->value.i, currentAlpha->right.value.b, alphaOp);
            break;
        case COMP_DOUBLE_INT: 
            propertyMatch = compareDouble(currentProperty->value.i, currentAlpha->right.value.i, alphaOp);
            break;
        case COMP_DOUBLE_DOUBLE: 
            propertyMatch = compareDouble(currentProperty->value.i, currentAlpha->right.value.d, alphaOp);
            break;
        case COMP_DOUBLE_STRING:
            propertyMatch = compareString(propertyFirst, 
                                          propertyLast, 
                                          &tree->stringPool[currentAlpha->right.value.stringOffset], 
                                          alphaOp);
            break;
        case COMP_STRING_BOOL:
            if (currentAlpha->right.value.b) {
                propertyMatch = compareString(propertyFirst, propertyLast, "true", alphaOp);
            }
            else {
                propertyMatch = compareString(propertyFirst, propertyLast, "false", alphaOp);
            }
            break;
        case COMP_STRING_INT: 
            {
                leftLength = propertyLast - propertyFirst + 1;
                char rightStringInt[leftLength];
                snprintf(rightStringInt, leftLength, "%ld", currentAlpha->right.value.i);
                propertyMatch = compareString(propertyFirst, propertyLast, rightStringInt, alphaOp);
            }
            break;
        case COMP_STRING_DOUBLE: 
            {
                leftLength = propertyLast - propertyFirst + 1;
                char rightStringDouble[leftLength];
                snprintf(rightStringDouble, leftLength, "%f", currentAlpha->right.value.d);
                propertyMatch = compareString(propertyFirst, propertyLast, rightStringDouble, alphaOp);
                break;
            }
        case COMP_STRING_STRING:
            propertyMatch = compareString(propertyFirst, 
                                          propertyLast, 
                                          &tree->stringPool[currentAlpha->right.value.stringOffset], 
                                          alphaOp);
            break;
    }

    return propertyMatch;
}

static unsigned int handleAlpha(ruleset *tree, 
                                char *sid, 
                                char *mid, 
                                char *state, 
                                alpha *alphaNode, 
                                jsonProperty *allProperties,
                                unsigned int propertiesLength, 
                                void **rulesBinding, 
                                unsigned short actionType, 
                                unsigned short *commandCount) {
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
                    if (currentProperty->hash == hashNode->value.a.hash && isMatch(tree, currentProperty, &hashNode->value.a)) {
                        if (top == MAX_STACK_SIZE) {
                            return ERR_MAX_STACK_SIZE;
                        }
                        stack[top] = &hashNode->value.a; 
                        ++top;
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
                                    rulesBinding, 
                                    actionType, 
                                    commandCount);
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
    if (idIndex == MAX_PROPERTIES) {
        return ERR_NO_ID_DEFINED;
    }

    currentProperty = &allProperties[idIndex];
    *idLength = currentProperty->lastValue - currentProperty->firstValue;
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

static unsigned int handleSession(ruleset *tree, 
                                  char *state, 
                                  void *rulesBinding, 
                                  unsigned short actionType, 
                                  unsigned short *commandCount, 
                                  unsigned char store) {
    char *next;
    unsigned int propertiesLength = 0;
    unsigned int dummyIndex = MAX_PROPERTIES;
    unsigned int sidIndex = MAX_PROPERTIES;
    jsonProperty properties[MAX_PROPERTIES];
    int result = constructObject(NULL, state, properties, &propertiesLength, &sidIndex, &dummyIndex, &next);
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
    strncpy(sid, sidProperty->firstValue, sidLength);
    sid[sidLength] = '\0';
    result = handleAlpha(tree, 
                         sid, 
                         NULL, 
                         state, 
                         &tree->nodePool[NODE_S_OFFSET].value.a, properties, 
                         propertiesLength, 
                         &rulesBinding, 
                         actionType, 
                         commandCount); 

    if (result == ERR_EVENT_NOT_HANDLED && store) {
        *commandCount = *commandCount + 1;
        result = storeSession(rulesBinding, sid, state);
    }

    return result;
}

static unsigned int handleEventCore(ruleset *tree,
                                    char *message, 
                                    jsonProperty *properties, 
                                    unsigned int propertiesLength, 
                                    unsigned int midIndex, 
                                    unsigned int sidIndex, 
                                    void **rulesBinding, 
                                    unsigned short actionType, 
                                    unsigned short *commandCount) {
    jsonProperty *idProperty;
    int idLength;
    int result = getId(properties, midIndex, &idProperty, &idLength);
    if (result != RULES_OK) {
        return result;
    }
    char mid[idLength + 1];
    strncpy(mid, idProperty->firstValue, idLength);
    mid[idLength] = '\0';
    
    result = getId(properties, sidIndex, &idProperty, &idLength);
    if (result != RULES_OK) {
        return result;
    }
    char sid[idLength + 1];
    strncpy(sid, idProperty->firstValue, idLength);
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
                          rulesBinding, 
                          actionType, 
                          commandCount);

    if (result == ERR_NEW_SESSION) {
        char session[11 + idLength];
        strcpy(session, "{\"id\":\"");
        strncpy(session + 7, sid, idLength);
        strcpy(session + 7 + idLength, "\"}");

        result = handleSession(tree, session, *rulesBinding, ACTION_ASSERT_SESSION_IMMEDIATE, commandCount, 0);
        if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
            return result;
        }

        return RULES_OK;
    }

    return result;
}

static unsigned int handleEvent(ruleset *tree, 
                                char *message, 
                                void **rulesBinding, 
                                unsigned short actionType, 
                                unsigned short *commandCount) {
    char *next;
    unsigned int propertiesLength = 0;
    unsigned int midIndex = MAX_PROPERTIES;
    unsigned int sidIndex = MAX_PROPERTIES;
    jsonProperty properties[MAX_PROPERTIES];
    int result = constructObject(NULL, message, properties, &propertiesLength, &midIndex, &sidIndex, &next);
    if (result != RULES_OK) {
        return result;
    }

    return handleEventCore(tree, 
                           message, 
                           properties, 
                           propertiesLength, 
                           midIndex, 
                           sidIndex, 
                           rulesBinding, 
                           actionType, 
                           commandCount);
}

unsigned int assertEvent(void *handle, char *message) {
    unsigned short commandCount = 0;
    void *rulesBinding = NULL;
    return handleEvent(handle, message, &rulesBinding, ACTION_ASSERT_MESSAGE_IMMEDIATE, &commandCount);
}

unsigned int assertEvents(void *handle, char *messages, unsigned int *resultsLength, unsigned int **results) {
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
    jsonProperty properties0[MAX_PROPERTIES];
    jsonProperty properties1[MAX_PROPERTIES];
    jsonProperty *currentProperties = NULL;
    unsigned int currentPropertiesLength = 0;
    unsigned int currentPropertiesMidIndex = MAX_PROPERTIES;
    unsigned int currentPropertiesSidIndex = MAX_PROPERTIES;
    unsigned int nextPropertiesLength = 0;
    unsigned int nextPropertiesMidIndex = MAX_PROPERTIES;
    unsigned int nextPropertiesSidIndex = MAX_PROPERTIES;
    
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
                                     &rulesBinding, 
                                     actionType, 
                                     &commandCount);
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
        nextPropertiesMidIndex = MAX_PROPERTIES;
        nextPropertiesSidIndex = MAX_PROPERTIES;
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
                             &rulesBinding, 
                             actionType, 
                             &commandCount);
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

unsigned int assertState(void *handle, char *state) {
    unsigned short commandCount = 0;
    void *rulesBinding = NULL;

    return handleSession(handle, state, rulesBinding, ACTION_ASSERT_SESSION_IMMEDIATE, &commandCount, 0);
}

static unsigned int createSession(redisReply *reply, char *firstSid, char *lastSid, char **session) {
    if (reply->element[2]->type == REDIS_REPLY_NIL) {        
        int idLength = lastSid - firstSid + 1;
        *session = malloc((11 + idLength) * sizeof(char));
        if (!*session) {
            return ERR_OUT_OF_MEMORY;
        }    

        strcpy(*session, "{\"id\":\"");
        strncpy(*session + 7, firstSid, idLength);
        strcpy(*session + 7 + idLength, "\"}");
    }
    else {
        *session = malloc((strlen(reply->element[2]->str) + 1) * sizeof(char));
        if (!*session) {
            return ERR_OUT_OF_MEMORY;
        }  

        strcpy(*session, reply->element[2]->str);  
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
                    if (*lastName != '+') {
                        current->lastName = lastName;
                    }
                    else {
                        current->lastName = lastName - 1;   
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

    result = createSession(reply, firstSid, lastSid, state);
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

    if (reply->element[2]->type != REDIS_REPLY_NIL) {
        result = handleSession(handle, reply->element[2]->str, rulesBinding, ACTION_NEGATE_SESSION, &commandCount, 0);
        if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
            freeReplyObject(reply);
            free(actionHandle);
            return result;
        }
    }
   
    for (unsigned long i = 4; i < reply->elements; i = i + 2) {
        if (strcmp(reply->element[i]->str, "null") != 0) {
            result = handleEvent(handle, reply->element[i]->str, &rulesBinding, ACTION_NEGATE_MESSAGE, &commandCount);
            if (result != RULES_OK) {
                freeReplyObject(reply);
                free(actionHandle);
                return result;
            }
        }
    }

    result = handleSession(handle, state, rulesBinding, ACTION_ASSERT_SESSION, &commandCount, 1);
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

unsigned int assertTimers(void *handle) {
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
        result = handleEvent(handle, reply->element[i]->str, &rulesBinding, ACTION_ASSERT_TIMER, &commandCount);
        if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
            freeReplyObject(reply);
            return result;
        }
    }

    result = executeCommands(rulesBinding, commandCount);
    freeReplyObject(reply);
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