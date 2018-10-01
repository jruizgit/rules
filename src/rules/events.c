
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rules.h"
#include "net.h"
#include "json.h"
#include "regex.h"

#include <time.h> 

#define MAX_RESULT_NODES 32
#define MAX_NODE_RESULTS 16
#define MAX_STACK_SIZE 64
#define MAX_STATE_PROPERTY_TIME 2
#define MAX_COMMAND_COUNT 10000
#define MAX_ADD_COUNT 1000
#define MAX_EVAL_COUNT 1000

#define OP_BOOL_BOOL 0x0404
#define OP_BOOL_INT 0x0402
#define OP_BOOL_DOUBLE 0x0403
#define OP_BOOL_STRING 0x0401
#define OP_BOOL_NIL 0x0407
#define OP_INT_BOOL 0x0204
#define OP_INT_INT 0x0202
#define OP_INT_DOUBLE 0x0203
#define OP_INT_STRING 0x0201
#define OP_INT_NIL 0x0207
#define OP_DOUBLE_BOOL 0x0304
#define OP_DOUBLE_INT 0x0302
#define OP_DOUBLE_DOUBLE 0x0303
#define OP_DOUBLE_STRING 0x0301
#define OP_DOUBLE_NIL 0x0307
#define OP_STRING_BOOL 0x0104
#define OP_STRING_INT 0x0102
#define OP_STRING_DOUBLE 0x0103
#define OP_STRING_STRING 0x0101
#define OP_STRING_NIL 0x0107
#define OP_STRING_REGEX 0x010E
#define OP_STRING_IREGEX 0x010F
#define OP_NIL_BOOL 0x0704
#define OP_NIL_INT 0x0702
#define OP_NIL_DOUBLE 0x0703
#define OP_NIL_STRING 0x0701
#define OP_NIL_NIL 0x0707

#define HASH_I 1622948014 //$i

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

static unsigned int handleMessage(ruleset *tree,
                                  char *state,
                                  char *message, 
                                  unsigned char actionType, 
                                  char **commands,
                                  unsigned int *commandCount,
                                  void **rulesBinding);

static unsigned int reduceIdiom(ruleset *tree, 
                                char *sid,
                                char *message,
                                jsonObject *messageObject,
                                unsigned int idiomOffset,
                                char **state,
                                unsigned char *releaseState,
                                jsonProperty **targetValue);

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

static long reduceInt(long left, 
                      long right, 
                      unsigned char op) {
    switch(op) {
        case OP_ADD:
            return left + right;
        case OP_SUB:
            return left - right;
        case OP_MUL:
            return left * right;
        case OP_DIV: 
            return left / right;
    }

    return 0;
}

static double reduceDouble(double left, 
                           double right, 
                           unsigned char op) {
    switch(op) {
        case OP_ADD:
            return left + right;
        case OP_SUB:
            return left - right;
        case OP_MUL:
            return left * right;
        case OP_DIV: 
            return left / right;
    }

    return 0;
}

static void freeCommands(char **commands,
                         unsigned int commandCount) {
    for (unsigned int i = 0; i < commandCount; ++i) {
        free(commands[i]);
    }
}

static unsigned int handleAction(ruleset *tree, 
                                 char *sid, 
                                 char *mid,
                                 char *prefix, 
                                 node *node, 
                                 unsigned char actionType, 
                                 char **evalKeys,
                                 unsigned int *evalCount,
                                 char **addKeys,
                                 unsigned int *addCount,
                                 char **removeCommand,
                                 void **rulesBinding) {
    unsigned int result = ERR_UNEXPECTED_VALUE;
    if (*rulesBinding == NULL) {
        result = resolveBinding(tree, 
                                sid, 
                                rulesBinding);
        if (result != RULES_OK) {
            return result;
        }
    }

    switch (actionType) {
        case ACTION_ASSERT_EVENT:
        case ACTION_ASSERT_FACT:
        case ACTION_RETRACT_EVENT:
        case ACTION_RETRACT_FACT:
            if (*evalCount == MAX_EVAL_COUNT) {
                return ERR_MAX_EVAL_COUNT;
            }

            char *evalKey = malloc((strlen(prefix) + 1) * sizeof(char));
            if (evalKey == NULL) {
                return ERR_OUT_OF_MEMORY;
            }

            strcpy(evalKey, prefix);
            evalKeys[*evalCount] = evalKey;
            ++*evalCount;
            break;

        case ACTION_ADD_EVENT:
        case ACTION_ADD_FACT:
            if (*addCount == MAX_ADD_COUNT) {
                return ERR_MAX_ADD_COUNT;
            }
            char *addKey = malloc((strlen(prefix) + 1) * sizeof(char));
            if (addKey == NULL) {
                return ERR_OUT_OF_MEMORY;
            }

            strcpy(addKey, prefix);
            addKeys[*addCount] = addKey;
            *addCount = *addCount + 1; 
            break;

        case ACTION_REMOVE_EVENT:
        case ACTION_REMOVE_FACT:
            if (*removeCommand == NULL) {
                result = formatRemoveMessage(*rulesBinding, 
                                         sid,
                                         mid, 
                                         actionType == ACTION_REMOVE_FACT ? 1 : 0,
                                         removeCommand);  

                if (result != RULES_OK) {
                    return result;
                }
            }
            break;
    }
    return RULES_OK;
}

static unsigned int handleBeta(ruleset *tree, 
                               char *sid,
                               char *mid,
                               node *betaNode,
                               unsigned short actionType, 
                               char **evalKeys,
                               unsigned int *evalCount,
                               char **addKeys,
                               unsigned int *addCount,
                               char **removeCommand,
                               void **rulesBinding) {
    int prefixLength = 0;
    node *currentNode = betaNode;
    while (currentNode != NULL) {
        int nameLength = strlen(&tree->stringPool[currentNode->nameOffset]);
        prefixLength += nameLength + 1;

        if (currentNode->type == NODE_ACTION) {
            currentNode = NULL;
        } else {
            if (currentNode->value.b.not) {
                switch (actionType) {
                    case ACTION_ASSERT_FACT:
                        actionType = ACTION_ADD_FACT;
                        break;
                    case ACTION_ASSERT_EVENT:
                        actionType = ACTION_ADD_EVENT;
                        break;
                    case ACTION_REMOVE_FACT:
                        actionType = ACTION_RETRACT_FACT;
                        break;
                    case ACTION_REMOVE_EVENT:
                        actionType = ACTION_RETRACT_EVENT;
                        break;
                }
            }
            currentNode = &tree->nodePool[currentNode->value.b.nextOffset];
        }
    }

    node *actionNode = NULL;
#ifdef _WIN32
    char *prefix = (char *)_alloca(sizeof(char)*(prefixLength));
#else
    char prefix[prefixLength];
#endif
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
                        prefix, 
                        actionNode, 
                        actionType,
                        evalKeys,
                        evalCount,
                        addKeys,
                        addCount,
                        removeCommand,
                        rulesBinding);
}


static unsigned int valueToProperty(ruleset *tree,
                                    char *sid,
                                    char *message,
                                    jsonObject *messageObject,
                                    jsonValue *sourceValue, 
                                    char **state,
                                    unsigned char *releaseState,
                                    jsonProperty **targetProperty) {

    unsigned int result = RULES_OK;
    *releaseState = 0;
    switch(sourceValue->type) {
        case JSON_EVENT_LOCAL_IDIOM:
        case JSON_STATE_IDIOM:
            result = reduceIdiom(tree, 
                                 sid, 
                                 message,
                                 messageObject,
                                 sourceValue->value.idiomOffset,
                                 state,
                                 releaseState,
                                 targetProperty);

            return result;
        case JSON_EVENT_LOCAL_PROPERTY:
            for (unsigned short i = 0; i < messageObject->propertiesLength; ++i) {
                if (sourceValue->value.property.nameHash == messageObject->properties[i].hash) {
                    *targetProperty = &messageObject->properties[i];
                    rehydrateProperty(*targetProperty, message);
                    return RULES_OK;
                } 
            }

            return ERR_PROPERTY_NOT_FOUND;
        case JSON_STATE_PROPERTY:
            if (sourceValue->value.property.idOffset) {
                sid = &tree->stringPool[sourceValue->value.property.idOffset];
            }
            result = fetchStateProperty(tree, 
                                        sid, 
                                        sourceValue->value.property.nameHash, 
                                        MAX_STATE_PROPERTY_TIME, 
                                        0,
                                        state,
                                        targetProperty);

            if (result == ERR_STATE_NOT_LOADED || result == ERR_STALE_STATE) {
                result = refreshState(tree,sid);
                if (result != RULES_OK) {
                    return result;
                }

                result = fetchStateProperty(tree, 
                                            sid, 
                                            sourceValue->value.property.nameHash, 
                                            MAX_STATE_PROPERTY_TIME, 
                                            0,
                                            state,
                                            targetProperty);
            }

            return result;
        case JSON_STRING:
            *state = &tree->stringPool[sourceValue->value.stringOffset];
            (*targetProperty)->valueLength = strlen(*state);
            (*targetProperty)->valueString = *state;
            break;
        case JSON_INT:
            (*targetProperty)->value.i = sourceValue->value.i;
            break;
        case JSON_DOUBLE:
            (*targetProperty)->value.d = sourceValue->value.d;
            break;
        case JSON_BOOL:
            (*targetProperty)->value.b = sourceValue->value.b;
            break;
    }

    (*targetProperty)->isMaterial = 1;
    (*targetProperty)->type = sourceValue->type;
    return result;
}

static unsigned int reduceProperties(unsigned char operator,
                                     jsonProperty *leftProperty,
                                     char *leftState, 
                                     jsonProperty *rightProperty,
                                     char *rightState,
                                     jsonProperty *targetValue,
                                     char **state) {
    *state = NULL;
    unsigned short type = leftProperty->type << 8;
    type = type + rightProperty->type;
    char leftTemp = 0;
    char rightTemp = 0;
    if (leftProperty->type == JSON_STRING || rightProperty->type == JSON_STRING) {
        if (operator != OP_ADD) {
            *state = (char *)calloc(1, sizeof(char));
            if (!*state) {
                return ERR_OUT_OF_MEMORY;
            }
            return RULES_OK;
        }

        if (leftProperty->type == JSON_STRING) {
            leftTemp = *(leftProperty->valueString + leftProperty->valueLength);
            *(leftProperty->valueString + leftProperty->valueLength) = '\0';
        } 

        if (rightProperty->type == JSON_STRING) {
            rightTemp = *(rightProperty->valueString + rightProperty->valueLength);
            *(rightProperty->valueString + rightProperty->valueLength) = '\0';
        }
    }

    switch(type) {
        case OP_BOOL_BOOL:
            targetValue->value.i = reduceInt(leftProperty->value.b, rightProperty->value.b, operator); 
            targetValue->type = JSON_INT;
            break;
        case OP_BOOL_INT: 
            targetValue->value.i = reduceInt(leftProperty->value.b, rightProperty->value.i, operator); 
            targetValue->type = JSON_INT;
            break;
        case OP_BOOL_DOUBLE: 
            targetValue->value.d = reduceDouble(leftProperty->value.b, rightProperty->value.d, operator); 
            targetValue->type = JSON_DOUBLE;
            break;
        case OP_BOOL_STRING:
            if (asprintf(state, 
                         "%s%s", 
                         leftProperty->value.b ? "true" : "false", 
                         rightProperty->valueString) == -1) {
                return ERR_OUT_OF_MEMORY;
            }
            break;
        case OP_INT_BOOL:
            targetValue->value.i = reduceInt(leftProperty->value.i, rightProperty->value.b, operator); 
            targetValue->type = JSON_INT;
            break;
        case OP_INT_INT:
            targetValue->value.i = reduceInt(leftProperty->value.i, rightProperty->value.i, operator); 
            targetValue->type = JSON_INT;
            break;
        case OP_INT_DOUBLE: 
            targetValue->value.d = reduceDouble(leftProperty->value.i, rightProperty->value.d, operator); 
            targetValue->type = JSON_DOUBLE;
            break;
        case OP_INT_STRING:
            if (asprintf(state, 
                         "%ld%s", 
                         leftProperty->value.i, 
                         rightProperty->valueString) == -1) {
                return ERR_OUT_OF_MEMORY;
            }
            break;
        case OP_DOUBLE_BOOL:
            targetValue->value.d = reduceDouble(leftProperty->value.d, rightProperty->value.b, operator); 
            targetValue->type = JSON_DOUBLE;
            break;
        case OP_DOUBLE_INT:
            targetValue->value.d = reduceDouble(leftProperty->value.d, rightProperty->value.i, operator); 
            targetValue->type = JSON_DOUBLE; 
            break;
        case OP_DOUBLE_DOUBLE:
            targetValue->value.d = reduceDouble(leftProperty->value.d, rightProperty->value.d, operator); 
            targetValue->type = JSON_DOUBLE; 
            break;
        case OP_DOUBLE_STRING:
            if (asprintf(state, 
                         "%g%s", 
                         leftProperty->value.d, 
                         rightProperty->valueString) == -1) {
                return ERR_OUT_OF_MEMORY;
            }

            break;
        case OP_STRING_BOOL:
            if (asprintf(state, 
                         "%s%s",
                         leftProperty->valueString,
                         rightProperty->value.b ? "true" : "false") == -1) {
                return ERR_OUT_OF_MEMORY;
            }
            break;
        case OP_STRING_INT: 
            if (asprintf(state, 
                         "%s%ld",
                         leftProperty->valueString,
                         rightProperty->value.i) == -1) {
                return ERR_OUT_OF_MEMORY;
            }
            break;
        case OP_STRING_DOUBLE: 
            if (asprintf(state, 
                         "%s%ld",
                         leftProperty->valueString,
                         rightProperty->value.i) == -1) {
                return ERR_OUT_OF_MEMORY;
            }
            break;
        case OP_STRING_STRING:
            if (asprintf(state, 
                         "%s%s",
                         leftProperty->valueString,
                         rightProperty->valueString) == -1) {
                return ERR_OUT_OF_MEMORY;
            }
            break;
    }    

    if (leftProperty->type == JSON_STRING || rightProperty->type == JSON_STRING) {
        targetValue->type = JSON_STRING;
        targetValue->valueString = *state;
        targetValue->valueLength = strlen(*state);
                
        if (leftTemp) {
            *(leftProperty->valueString + leftProperty->valueLength) = leftTemp;
        } 

        if (rightTemp) {
            *(rightProperty->valueString + rightProperty->valueLength) = rightTemp;
        }
    }

    return RULES_OK;
}

static unsigned int reduceIdiom(ruleset *tree, 
                                char *sid,
                                char *message,
                                jsonObject *messageObject,
                                unsigned int idiomOffset,
                                char **state,
                                unsigned char *releaseState,
                                jsonProperty **targetValue) {
    unsigned int result = RULES_OK;
    *releaseState = 0;
    idiom *currentIdiom = &tree->idiomPool[idiomOffset];
    jsonProperty leftValue;
    jsonProperty *leftProperty = &leftValue;
    unsigned char releaseLeftState = 0;
    char *leftState = NULL;
    result = valueToProperty(tree,
                             sid,
                             message,
                             messageObject,
                             &currentIdiom->left,
                             &leftState,
                             &releaseLeftState,
                             &leftProperty);
    if (result != RULES_OK) {
        return result;
    }

    jsonProperty rightValue;
    jsonProperty *rightProperty = &rightValue;
    unsigned char releaseRightState = 0;
    char *rightState = NULL;
    result = valueToProperty(tree,
                             sid,
                             message,
                             messageObject,
                             &currentIdiom->right,
                             &rightState,
                             &releaseRightState,
                             &rightProperty);
    if (result != RULES_OK) {
        if (releaseLeftState) {
            free(leftState);
        }

        return result;
    }

    result = reduceProperties(currentIdiom->operator, 
                            leftProperty, 
                            leftState,
                            rightProperty, 
                            rightState, 
                            *targetValue,
                            state);

    if (releaseLeftState) {
        free(leftState);
    }

    if (releaseRightState) {
        free(rightState);
    }

    if (state) {
        *releaseState = 1;
    }

    return result;
}

static unsigned int isMatch(ruleset *tree,
                            char *sid,
                            char *message,
                            jsonObject *messageObject,
                            jsonProperty *currentProperty, 
                            alpha *currentAlpha,
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
    jsonProperty rightValue;
    jsonProperty *rightProperty = &rightValue;
    unsigned char releaseRightState = 0;
    char *rightState = NULL;
    result = valueToProperty(tree,
                             sid,
                             message,
                             messageObject,
                             &currentAlpha->right,
                             &rightState,
                             &releaseRightState,
                             &rightProperty);
    if (result != RULES_OK) {
        if (releaseRightState) {
            free(rightState);
        }

        if (result != ERR_NEW_SESSION && result != ERR_PROPERTY_NOT_FOUND) {
            return result;    
        }

        return RULES_OK;
    }
    
    int leftLength;
    int rightLength;
    unsigned short type = propertyType << 8;
    type = type + rightProperty->type;
    switch(type) {
        case OP_BOOL_BOOL:
            *propertyMatch = compareBool(currentProperty->value.b, rightProperty->value.b, alphaOp);
            break;
        case OP_BOOL_INT: 
            *propertyMatch = compareInt(currentProperty->value.b, rightProperty->value.i, alphaOp);
            break;
        case OP_BOOL_DOUBLE: 
            *propertyMatch = compareDouble(currentProperty->value.b, rightProperty->value.d, alphaOp);
            break;
        case OP_BOOL_STRING:
            if (currentProperty->value.b) {
                *propertyMatch = compareStringProperty("true",
                                                       rightProperty->valueString, 
                                                       rightProperty->valueLength,
                                                       alphaOp);
            }
            else {
                *propertyMatch = compareStringProperty("false",
                                                       rightProperty->valueString, 
                                                       rightProperty->valueLength,
                                                       alphaOp);
            }
            
            break;
        case OP_INT_BOOL:
            *propertyMatch = compareInt(currentProperty->value.i, rightProperty->value.b, alphaOp);
            break;
        case OP_INT_INT: 
            *propertyMatch = compareInt(currentProperty->value.i, rightProperty->value.i, alphaOp);
            break;
        case OP_INT_DOUBLE: 
            *propertyMatch = compareDouble(currentProperty->value.i, rightProperty->value.d, alphaOp);
            break;
        case OP_INT_STRING:
            {
                rightLength = rightProperty->valueLength + 1;
#ifdef _WIN32
                char *leftStringInt = (char *)_alloca(sizeof(char)*(rightLength));
                sprintf_s(leftStringInt, sizeof(char)*(rightLength), "%ld", currentProperty->value.i);
#else
                char leftStringInt[rightLength];
                snprintf(leftStringInt, sizeof(char)*(rightLength), "%ld", currentProperty->value.i);
#endif         
                *propertyMatch = compareStringProperty(leftStringInt, 
                                                       rightProperty->valueString,
                                                       rightProperty->valueLength, 
                                                       alphaOp);
            }
            break;
        case OP_DOUBLE_BOOL:
            *propertyMatch = compareDouble(currentProperty->value.d, rightProperty->value.b, alphaOp);
            break;
        case OP_DOUBLE_INT: 
            *propertyMatch = compareDouble(currentProperty->value.d, rightProperty->value.i, alphaOp);
            break;
        case OP_DOUBLE_DOUBLE: 
            *propertyMatch = compareDouble(currentProperty->value.d, rightProperty->value.d, alphaOp);
            break;
        case OP_DOUBLE_STRING:
            {
                rightLength = rightProperty->valueLength + 1;
#ifdef _WIN32
                char *leftStringDouble = (char *)_alloca(sizeof(char)*(rightLength));
                sprintf_s(leftStringDouble, sizeof(char)*(rightLength), "%f", currentProperty->value.d);
#else
                char leftStringDouble[rightLength];
                snprintf(leftStringDouble, sizeof(char)*(rightLength), "%f", currentProperty->value.d);
#endif         
                *propertyMatch = compareStringProperty(leftStringDouble,
                                                       rightProperty->valueString,
                                                       rightProperty->valueLength, 
                                                       alphaOp);
            }
            break;
        case OP_STRING_BOOL:
            if (rightProperty->value.b) {
                *propertyMatch = compareString(currentProperty->valueString, 
                                               currentProperty->valueLength, 
                                               "true", 
                                               alphaOp);
            }
            else {
                *propertyMatch = compareString(currentProperty->valueString, 
                                               currentProperty->valueLength, 
                                               "false", 
                                               alphaOp);
            }
            break;
        case OP_STRING_INT: 
            {
                leftLength = currentProperty->valueLength + 1;
#ifdef _WIN32
                char *rightStringInt = (char *)_alloca(sizeof(char)*(leftLength));
                sprintf_s(rightStringInt, sizeof(char)*(leftLength), "%ld", rightProperty->value.i);
#else
                char rightStringInt[leftLength];
                snprintf(rightStringInt, sizeof(char)*(leftLength), "%ld", rightProperty->value.i);
#endif
                *propertyMatch = compareString(currentProperty->valueString, 
                                               currentProperty->valueLength, 
                                               rightStringInt, 
                                               alphaOp);
            }
            break;
        case OP_STRING_DOUBLE: 
            {
                leftLength = currentProperty->valueLength + 1;
#ifdef _WIN32
                char *rightStringDouble = (char *)_alloca(sizeof(char)*(leftLength));
                sprintf_s(rightStringDouble, sizeof(char)*(leftLength), "%f", rightProperty->value.d);
#else
                char rightStringDouble[leftLength];
                snprintf(rightStringDouble, sizeof(char)*(leftLength), "%f", rightProperty->value.d);
#endif              
                *propertyMatch = compareString(currentProperty->valueString, 
                                               currentProperty->valueLength, 
                                               rightStringDouble, 
                                               alphaOp);
            }
            break;
        case OP_STRING_STRING:
            *propertyMatch = compareStringAndStringProperty(currentProperty->valueString, 
                                                            currentProperty->valueLength, 
                                                            rightProperty->valueString,
                                                            rightProperty->valueLength,
                                                            alphaOp);
            break;
        case OP_BOOL_NIL:
        case OP_INT_NIL:
        case OP_DOUBLE_NIL:
        case OP_STRING_NIL:
            if (alphaOp == OP_NEQ) {
                *propertyMatch = 1;
            } else {
                *propertyMatch = 0;
            }

            break;
        case OP_NIL_NIL:
            if (alphaOp == OP_EQ) {
                *propertyMatch = 1;
            } else {
                *propertyMatch = 0;
            }

            break;
        case OP_STRING_REGEX:
        case OP_STRING_IREGEX:
            *propertyMatch = evaluateRegex(tree,
                                           currentProperty->valueString, 
                                           currentProperty->valueLength, 
                                           (type == OP_STRING_REGEX) ? 0 : 1,
                                           currentAlpha->right.value.regex.vocabularyLength,
                                           currentAlpha->right.value.regex.statesLength,
                                           currentAlpha->right.value.regex.stateMachineOffset);
            break;

    }
    
    if (releaseRightState) {
        free(rightState);
    }
    return result;
}

static unsigned int isArrayMatch(ruleset *tree,
                                 char *sid,
                                 char *message,
                                 jsonObject *messageObject,
                                 jsonProperty *currentProperty,
                                 alpha *arrayAlpha,
                                 unsigned char *propertyMatch,
                                 void **rulesBinding) {
    unsigned int result = RULES_OK;
    if (currentProperty->type != JSON_ARRAY) {
        return RULES_OK;
    }

    char *first = currentProperty->valueString;
    char *last;
    unsigned char type;
    jsonObject jo;
    result = readNextArrayValue(first, &first, &last, &type);
    while (result == PARSE_OK) {
        *propertyMatch = 0;
        unsigned short top = 1;
        alpha *stack[MAX_STACK_SIZE];
        stack[0] = arrayAlpha;
        alpha *currentAlpha;
        if (type == JSON_OBJECT) {
            char *next;
            jo.propertiesLength = 0;
            result = constructObject(first,
                                     "$i", 
                                     NULL, 
                                     JSON_OBJECT_SEQUENCED, 
                                     0,
                                     &jo, 
                                     &next);
            if (result != RULES_OK) {
                return result;
            }
        } else {
            jo.propertiesLength = 1;
            jo.properties[0].hash = HASH_I;
            jo.properties[0].type = type;
            jo.properties[0].isMaterial = 0;
            jo.properties[0].valueString = first;
            jo.properties[0].valueLength = last - first;
            jo.properties[0].nameLength = 2;
            strcpy(jo.properties[0].name, "$i");
        }

        while (top) {
            --top;
            currentAlpha = stack[top];
            // add all disjunctive nodes to stack
            if (currentAlpha->nextListOffset) {
                unsigned int *nextList = &tree->nextPool[currentAlpha->nextListOffset];
                for (unsigned int entry = 0; nextList[entry] != 0; ++entry) {
                    node *listNode = &tree->nodePool[nextList[entry]];
                    char exists = 0;
                    for(unsigned int propertyIndex = 0; propertyIndex < jo.propertiesLength; ++propertyIndex) {
                        if (listNode->value.a.hash == jo.properties[propertyIndex].hash) {
                            // filter out not exists (OP_NEX)
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

            // calculate conjunctive nodes
            if (currentAlpha->nextOffset) {
                unsigned int *nextHashset = &tree->nextPool[currentAlpha->nextOffset];
                for(unsigned int propertyIndex = 0; propertyIndex < jo.propertiesLength; ++propertyIndex) {
                    jsonProperty *currentProperty = &jo.properties[propertyIndex];
                    for (unsigned int entry = currentProperty->hash & HASH_MASK; nextHashset[entry] != 0; entry = (entry + 1) % NEXT_BUCKET_LENGTH) {
                        node *hashNode = &tree->nodePool[nextHashset[entry]];
                        if (currentProperty->hash == hashNode->value.a.hash) {
                            unsigned char match = 0;
                            if (hashNode->value.a.operator == OP_IALL || hashNode->value.a.operator == OP_IANY) {
                                // isArrayMatch finds a valid path, thus use propertyMatch
                                result = isArrayMatch(tree, 
                                                      sid, 
                                                      message,
                                                      messageObject,
                                                      currentProperty, 
                                                      &hashNode->value.a,
                                                      propertyMatch,
                                                      rulesBinding);
                            } else {
                                result = isMatch(tree,
                                                 sid,
                                                 message,
                                                 messageObject,
                                                 currentProperty,
                                                 &hashNode->value.a,
                                                 &match,
                                                 rulesBinding);

                                if (match) {
                                    if (top == MAX_STACK_SIZE) {
                                        return ERR_MAX_STACK_SIZE;
                                    }

                                    stack[top] = &hashNode->value.a; 
                                    ++top;
                                }
                            }

                            if (result != RULES_OK) {
                                return result;
                            }
                        }
                    }               
                }
            }

            // no next offset and no nextListOffest means we found a valid path
            if (!currentAlpha->nextOffset && !currentAlpha->nextListOffset) {
                *propertyMatch = 1;
                break;
            } 
        }
        
        // OP_IANY, one element led a a valid path
        // OP_IALL, all elements led to a valid path
        if ((arrayAlpha->operator == OP_IALL && !*propertyMatch) ||
            (arrayAlpha->operator == OP_IANY && *propertyMatch)) {
            break;
        } 

        result = readNextArrayValue(last, &first, &last, &type);   
    }

    return (result == PARSE_END ? RULES_OK: result);
}

static unsigned int handleAlpha(ruleset *tree, 
                                char *sid, 
                                char *mid,
                                char *message,
                                jsonObject *jo,
                                alpha *alphaNode, 
                                unsigned char actionType,
                                char **evalKeys,
                                unsigned int *evalCount,
                                char **addKeys,
                                unsigned int *addCount,
                                char **removeCommand,
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
        // add all disjunctive nodes to stack
        if (currentAlpha->nextListOffset) {
            unsigned int *nextList = &tree->nextPool[currentAlpha->nextListOffset];
            for (entry = 0; nextList[entry] != 0; ++entry) {
                node *listNode = &tree->nodePool[nextList[entry]];
                char exists = 0;
                for(unsigned int propertyIndex = 0; propertyIndex < jo->propertiesLength; ++propertyIndex) {
                    if (listNode->value.a.hash == jo->properties[propertyIndex].hash) {
                        // filter out not exists (OP_NEX)
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

        // calculate conjunctive nodes
        if (currentAlpha->nextOffset) {
            unsigned int *nextHashset = &tree->nextPool[currentAlpha->nextOffset];
            for(unsigned int propertyIndex = 0; propertyIndex < jo->propertiesLength; ++propertyIndex) {
                jsonProperty *currentProperty = &jo->properties[propertyIndex];
                for (entry = currentProperty->hash & HASH_MASK; nextHashset[entry] != 0; entry = (entry + 1) % NEXT_BUCKET_LENGTH) {
                    node *hashNode = &tree->nodePool[nextHashset[entry]];
                    if (currentProperty->hash == hashNode->value.a.hash) {
                        if (hashNode->value.a.right.type == JSON_EVENT_PROPERTY || hashNode->value.a.right.type == JSON_EVENT_IDIOM) {
                            if (top == MAX_STACK_SIZE) {
                                return ERR_MAX_STACK_SIZE;
                            }
                            stack[top] = &hashNode->value.a; 
                            ++top;
                        } else {
                            unsigned char match = 0;
                            unsigned int mresult = RULES_OK;
                            if (hashNode->value.a.operator == OP_IALL || hashNode->value.a.operator == OP_IANY) {
                                mresult = isArrayMatch(tree, 
                                                       sid, 
                                                       message,
                                                       jo,
                                                       currentProperty, 
                                                       &hashNode->value.a,
                                                       &match,
                                                       rulesBinding);
                            } else {
                                mresult = isMatch(tree, 
                                                  sid, 
                                                  message,
                                                  jo,
                                                  currentProperty, 
                                                  &hashNode->value.a,
                                                  &match,
                                                  rulesBinding);
                            }

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
                unsigned int bresult = handleBeta(tree, 
                                    sid, 
                                    mid,
                                    &tree->nodePool[betaList[entry]],  
                                    actionType, 
                                    evalKeys,
                                    evalCount,
                                    addKeys,
                                    addCount,
                                    removeCommand,
                                    rulesBinding);
                if (bresult != RULES_OK && bresult != ERR_NEW_SESSION) {
                    return result;
                }

                if (result != ERR_NEW_SESSION) {
                    result = bresult;
                }
            }
        }
    }

    return result;
}

static unsigned int handleMessageCore(ruleset *tree,
                                      char *state,
                                      char *message, 
                                      jsonObject *jo, 
                                      unsigned char actionType,
                                      char **commands,
                                      unsigned int *commandCount,
                                      void **rulesBinding) {
    unsigned int result;
    char *storeCommand;
    jsonProperty *sidProperty = &jo->properties[jo->sidIndex];
    jsonProperty *midProperty = &jo->properties[jo->idIndex];
    
#ifdef _WIN32
    char *sid = (char *)_alloca(sizeof(char)*(sidProperty->valueLength + 1));
#else
    char sid[sidProperty->valueLength + 1];
#endif  
    strncpy(sid, sidProperty->valueString, sidProperty->valueLength);
    sid[sidProperty->valueLength] = '\0';
    
#ifdef _WIN32
    char *mid = (char *)_alloca(sizeof(char)*(midProperty->valueLength + 1));
#else
    char mid[midProperty->valueLength + 1];
#endif
    strncpy(mid, midProperty->valueString, midProperty->valueLength);
    mid[midProperty->valueLength] = '\0';
    
    if (*commandCount == MAX_COMMAND_COUNT) {
        return ERR_MAX_COMMAND_COUNT;
    }

    if (state) {
        if (!*rulesBinding) {
            result = resolveBinding(tree, sid, rulesBinding);
            if (result != RULES_OK) {
                return result;
            }
        }

        result = formatStoreSession(*rulesBinding, sid, state, 0, &storeCommand);
        if (result != RULES_OK) {
            return result;
        }

        commands[*commandCount] = storeCommand;
        ++*commandCount;
    }
    char *removeCommand = NULL;
    char *addKeys[MAX_ADD_COUNT];
    unsigned int addCount = 0;
    char *evalKeys[MAX_EVAL_COUNT];
    unsigned int evalCount = 0;
    result = handleAlpha(tree, 
                         sid, 
                         mid,
                         message, 
                         jo,
                         &tree->nodePool[NODE_M_OFFSET].value.a, 
                         actionType, 
                         evalKeys,
                         &evalCount,
                         addKeys,
                         &addCount,
                         &removeCommand,
                         rulesBinding);
    if (result == RULES_OK) {
        if (*commandCount == MAX_COMMAND_COUNT - 3) {
            for (unsigned int i = 0; i < addCount; ++i) {
                free(addKeys[i]);
            }

            for (unsigned int i = 0; i < evalCount; ++i) {
                free(evalKeys[i]);
            }
            
            return ERR_MAX_COMMAND_COUNT;
        }

        if (removeCommand) {
            commands[*commandCount] = removeCommand;
            ++*commandCount;
        }

        if (addCount > 0) {
            char *addCommand = NULL;
            result = formatStoreMessage(*rulesBinding,
                                        sid,
                                        message,
                                        jo,
                                        actionType == ACTION_ASSERT_FACT ? 1 : 0,
                                        evalCount == 0 ? 1 : 0, 
                                        addKeys,
                                        addCount,
                                        &addCommand);
            for (unsigned int i = 0; i < addCount; ++i) {
                free(addKeys[i]);
            }

            if (result != RULES_OK) {
                for (unsigned int i = 0; i < evalCount; ++i) {
                    free(evalKeys[i]);
                }

                return result;
            }

            commands[*commandCount] = addCommand;
            ++*commandCount;
        }

        if (evalCount > 0) {
            char *evalCommand = NULL;
            result = formatEvalMessage(*rulesBinding,
                                        sid,
                                        mid,
                                        message,
                                        jo,
                                        actionType == ACTION_REMOVE_FACT ? ACTION_RETRACT_FACT : actionType,
                                        evalKeys,
                                        evalCount,
                                        &evalCommand);

            for (unsigned int i = 0; i < evalCount; ++i) {
                free(evalKeys[i]);
            }

            if (result != RULES_OK) {
                return result;
            }

            commands[*commandCount] = evalCommand;
            ++*commandCount;
        }

        if (!state) {
            // try creating state if doesn't exist
            jsonProperty *targetProperty;
            char *targetState;
            result = fetchStateProperty(tree, 
                                        sid, 
                                        HASH_SID, 
                                        MAX_STATE_PROPERTY_TIME, 
                                        1,
                                        &targetState,
                                        &targetProperty);
            if (result != ERR_STATE_NOT_LOADED && result != ERR_PROPERTY_NOT_FOUND) {
                return result;
            }

            // this sid has been tried by this process already
            if (result == ERR_PROPERTY_NOT_FOUND) {
                return RULES_OK;
            }

            result = refreshState(tree, sid);
            if (result != ERR_NEW_SESSION) {
                return result;
            }            
#ifdef _WIN32
            char *stateMessage = (char *)_alloca(sizeof(char)*(36 + sidProperty->valueLength));
            char *newState = (char *)_alloca(sizeof(char)*(12 + sidProperty->valueLength));
            sprintf_s(stateMessage, sizeof(char)*(26 + sidProperty->valueLength), "{\"sid\":\"%s\", \"$s\":1}", sid);
            sprintf_s(newState, sizeof(char)*(12 + sidProperty->valueLength), "{\"sid\":\"%s\"}", sid); 
#else
            char stateMessage[36 + sidProperty->valueLength];
            char newState[12 + sidProperty->valueLength];
            snprintf(stateMessage, sizeof(char)*(26 + sidProperty->valueLength), "{\"sid\":\"%s\", \"$s\":1}", sid);
            snprintf(newState, sizeof(char)*(12 + sidProperty->valueLength), "{\"sid\":\"%s\"}", sid);
#endif

            if (*commandCount == MAX_COMMAND_COUNT) {
                return ERR_MAX_COMMAND_COUNT;
            }

            result = formatStoreSession(*rulesBinding, sid, newState, 1, &storeCommand);
            if (result != RULES_OK) {
                return result;
            }

            commands[*commandCount] = storeCommand;
            ++*commandCount;
            result = handleMessage(tree, 
                                   NULL,
                                   stateMessage,  
                                   ACTION_ASSERT_EVENT,
                                   commands,
                                   commandCount,
                                   rulesBinding);
            if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
                return result;
            }

            if (result == ERR_EVENT_NOT_HANDLED) {
                return RULES_OK;
            }
        }

        return RULES_OK;
    } 

    return result;
}

static unsigned int handleMessage(ruleset *tree,
                                  char *state,
                                  char *message, 
                                  unsigned char actionType, 
                                  char **commands,
                                  unsigned int *commandCount,
                                  void **rulesBinding) {
    char *next;
    jsonObject jo;
    int result = constructObject(message,
                                 NULL, 
                                 NULL, 
                                 JSON_OBJECT_SEQUENCED, 
                                 actionType == ACTION_ASSERT_FACT || 
                                 actionType == ACTION_RETRACT_FACT || 
                                 actionType == ACTION_REMOVE_FACT,
                                 &jo, 
                                 &next);
    if (result != RULES_OK) {
        return result;
    }
    
    return handleMessageCore(tree,
                            state, 
                            message, 
                            &jo,
                            actionType,
                            commands,
                            commandCount,
                            rulesBinding);
}

static unsigned int handleMessages(ruleset *tree, 
                                   unsigned char actionType,
                                   char *messages, 
                                   char **commands,
                                   unsigned int *commandCount,
                                   void **rulesBinding) {
    unsigned int result;
    unsigned int returnResult = RULES_OK;
    jsonObject jo;

    char *first = messages;
    char *last = NULL;
    char lastTemp;

    while (*first != '{' && *first != '\0' ) {
        ++first;
    }

    while (constructObject(first,
                           NULL, 
                           NULL, 
                           JSON_OBJECT_SEQUENCED,
                           actionType == ACTION_ASSERT_FACT || 
                           actionType == ACTION_RETRACT_FACT || 
                           actionType == ACTION_REMOVE_FACT,
                           &jo,
                           &last) == RULES_OK) {

        while (*last != ',' && *last != '\0' ) {
            ++last;
        }

        if (*last == '\0') {
            --last;
        }

        lastTemp = *last;
        *last = '\0';
        result = handleMessageCore(tree,
                                 NULL, 
                                 first, 
                                 &jo, 
                                 actionType, 
                                 commands,
                                 commandCount,
                                 rulesBinding);
        
        *last = lastTemp;
        if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
            return result;
        }

        if (result == ERR_EVENT_NOT_HANDLED) {
            returnResult = ERR_EVENT_NOT_HANDLED;
        }
       
        first = last;
        while (*first != '{' && *first != '\0' ) {
            ++first;
        }
    }

    return returnResult;
}

static unsigned int handleState(ruleset *tree, 
                                char *state,
                                char **commands,
                                unsigned int *commandCount,
                                void **rulesBinding) {
    int stateLength = strlen(state);
    if (stateLength < 2) {
        return ERR_PARSE_OBJECT;
    }

    if (state[0] != '{') {
        return ERR_PARSE_OBJECT;
    }

    char *stateMessagePostfix = state + 1;
#ifdef _WIN32
    char *stateMessage = (char *)_alloca(sizeof(char)*(24 + stateLength - 1));
#else
    char stateMessage[24 + stateLength - 1];
#endif
    
#ifdef _WIN32
    sprintf_s(stateMessage, sizeof(char)*(24 + stateLength - 1), "{\"$s\":1, %s", stateMessagePostfix);
#else
    snprintf(stateMessage, sizeof(char)*(24 + stateLength - 1), "{\"$s\":1, %s", stateMessagePostfix);
#endif
    unsigned int result = handleMessage(tree, 
                                        state,
                                        stateMessage,  
                                        ACTION_ASSERT_EVENT,
                                        commands,
                                        commandCount,
                                        rulesBinding);

    return result;
}

static unsigned int handleTimers(ruleset *tree, 
                                 char **commands,
                                 unsigned int *commandCount,
                                 void **rulesBinding) {
    redisReply *reply;
    unsigned int result = peekTimers(tree, rulesBinding, &reply);
    if (result != RULES_OK) {
        return result;
    }

    for (unsigned long i = 0; i < reply->elements; ++i) {
        if (*commandCount == MAX_COMMAND_COUNT) {
            return ERR_MAX_COMMAND_COUNT;
        }

        char *command;
        result = formatRemoveTimer(*rulesBinding, reply->element[i]->str, &command);
        if (result != RULES_OK) {
            freeReplyObject(reply);
            return result;
        }

        unsigned int action;
        switch (reply->element[i]->str[0]) {
            case 'p':
                action  = ACTION_ASSERT_EVENT;
                break;
            case 'a':
                action  = ACTION_ASSERT_FACT;
                break;
            case 'r':
                action  = ACTION_RETRACT_FACT;
                break;
        }

        commands[*commandCount] = command;
        ++*commandCount;
        result = handleMessage(tree, 
                               NULL,
                               reply->element[i]->str + 2, 
                               action,
                               commands, 
                               commandCount, 
                               rulesBinding);
        if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
            freeReplyObject(reply);
            return result;
        }
    }

    freeReplyObject(reply);
    return RULES_OK;
}

static unsigned int startHandleMessage(ruleset *tree, 
                                       char *message, 
                                       unsigned char actionType,
                                       void **rulesBinding,
                                       unsigned int *replyCount) {
    char *commands[MAX_COMMAND_COUNT];
    unsigned int commandCount = 0;
    unsigned int result = handleMessage(tree, 
                                        NULL,
                                        message, 
                                        actionType, 
                                        commands,
                                        &commandCount,
                                        rulesBinding);
    if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
        freeCommands(commands, commandCount);
        return result;
    }

    unsigned int batchResult = startNonBlockingBatch(*rulesBinding, commands, commandCount, replyCount);
    if (batchResult != RULES_OK) {
        return batchResult;
    }

    return result;
}

static unsigned int executeHandleMessage(ruleset *tree, 
                                         char *message, 
                                         unsigned char actionType) {
    char *commands[MAX_COMMAND_COUNT];
    unsigned int commandCount = 0;
    void *rulesBinding = NULL;
    unsigned int result = handleMessage(tree, 
                                        NULL,
                                        message, 
                                        actionType, 
                                        commands,
                                        &commandCount,
                                        &rulesBinding);
    if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
        freeCommands(commands, commandCount);
        return result;
    }

    unsigned int batchResult = executeBatch(rulesBinding, commands, commandCount);
    if (batchResult != RULES_OK) {
        return batchResult;
    }

    return result;
}

static unsigned int startHandleMessages(ruleset *tree, 
                                        char *messages, 
                                        unsigned char actionType,
                                        void **rulesBinding,
                                        unsigned int *replyCount) {
    char *commands[MAX_COMMAND_COUNT];
    unsigned int commandCount = 0;
    unsigned int result = handleMessages(tree,
                                         actionType,
                                         messages,
                                         commands,
                                         &commandCount,
                                         rulesBinding);
    if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
        freeCommands(commands, commandCount);
        return result;
    }

    unsigned int batchResult = startNonBlockingBatch(*rulesBinding, commands, commandCount, replyCount);
    if (batchResult != RULES_OK) {
        return batchResult;
    }

    return result;
}

static unsigned int executeHandleMessages(ruleset *tree, 
                                          char *messages, 
                                          unsigned char actionType) {
    char *commands[MAX_COMMAND_COUNT];
    unsigned int commandCount = 0;
    void *rulesBinding = NULL;
    unsigned int result = handleMessages(tree,
                                         actionType,
                                         messages,
                                         commands,
                                         &commandCount,
                                         &rulesBinding);
    if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
        freeCommands(commands, commandCount);
        return result;
    }

    unsigned int batchResult = executeBatch(rulesBinding, commands, commandCount);
    if (batchResult != RULES_OK) {
        return batchResult;
    }

    return result;
}

unsigned int complete(void *rulesBinding, unsigned int replyCount) {
    unsigned int result = completeNonBlockingBatch(rulesBinding, replyCount);
    if (result != RULES_OK && result != ERR_EVENT_OBSERVED) {
        return result;
    }

    return RULES_OK;
}

unsigned int assertEvent(unsigned int handle, char *message) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    return executeHandleMessage(tree, message, ACTION_ASSERT_EVENT);
}

unsigned int startAssertEvent(unsigned int handle, 
                             char *message, 
                             void **rulesBinding, 
                             unsigned int *replyCount) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    return startHandleMessage(tree, message, ACTION_ASSERT_EVENT, rulesBinding, replyCount);
}

unsigned int assertEvents(unsigned int handle, 
                          char *messages) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    return executeHandleMessages(tree, messages, ACTION_ASSERT_EVENT);
}

unsigned int startAssertEvents(unsigned int handle, 
                              char *messages, 
                              void **rulesBinding, 
                              unsigned int *replyCount) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    return startHandleMessages(tree, messages, ACTION_ASSERT_EVENT, rulesBinding, replyCount);
}

unsigned int retractEvent(unsigned int handle, char *message) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    return executeHandleMessage(tree, message, ACTION_REMOVE_EVENT);
}

unsigned int startAssertFact(unsigned int handle, 
                             char *message, 
                             void **rulesBinding, 
                             unsigned int *replyCount) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    return startHandleMessage(tree, message, ACTION_ASSERT_FACT, rulesBinding, replyCount);
}

unsigned int assertFact(unsigned int handle, char *message) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    return executeHandleMessage(tree, message, ACTION_ASSERT_FACT);
}

unsigned int startAssertFacts(unsigned int handle, 
                              char *messages, 
                              void **rulesBinding, 
                              unsigned int *replyCount) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    return startHandleMessages(tree, messages, ACTION_ASSERT_FACT, rulesBinding, replyCount);
}

unsigned int assertFacts(unsigned int handle, 
                         char *messages) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    return executeHandleMessages(tree, messages, ACTION_ASSERT_FACT);
}

unsigned int retractFact(unsigned int handle, char *message) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    return executeHandleMessage(tree, message, ACTION_REMOVE_FACT);
}

unsigned int startRetractFact(unsigned int handle, 
                             char *message, 
                             void **rulesBinding, 
                             unsigned int *replyCount) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    return startHandleMessage(tree, message, ACTION_REMOVE_FACT, rulesBinding, replyCount);
}

unsigned int retractFacts(unsigned int handle, 
                          char *messages) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    return executeHandleMessages(tree, messages, ACTION_REMOVE_FACT);
}

unsigned int startRetractFacts(unsigned int handle, 
                              char *messages, 
                              void **rulesBinding, 
                              unsigned int *replyCount) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    return startHandleMessages(tree, messages, ACTION_REMOVE_FACT, rulesBinding, replyCount);
}

unsigned int assertState(unsigned int handle, char *sid, char *state) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    char *commands[MAX_COMMAND_COUNT];
    unsigned int commandCount = 0;
    void *rulesBinding = NULL;

    unsigned int result = handleState(tree, 
                                      state, 
                                      commands,
                                      &commandCount,
                                      &rulesBinding);
    if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
        freeCommands(commands, commandCount);
        return result;
    }

    unsigned int batchResult = executeBatch(rulesBinding, commands, commandCount);
    if (batchResult != RULES_OK) {
        return batchResult;
    }

    return result;
}

unsigned int assertTimers(unsigned int handle) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    char *commands[MAX_COMMAND_COUNT];
    unsigned int commandCount = 0;
    void *rulesBinding = NULL;
    unsigned int result = handleTimers(tree, 
                                       commands,
                                       &commandCount,
                                       &rulesBinding);
    if (result != RULES_OK) {
        freeCommands(commands, commandCount);
        return result;
    }

    result = executeBatch(rulesBinding, commands, commandCount);
    if (result != RULES_OK && result != ERR_EVENT_OBSERVED) {
        return result;
    }

    return RULES_OK;
}

unsigned int startAction(unsigned int handle, 
                         char **state, 
                         char **messages, 
                         void **actionHandle,
                         void **actionBinding) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    redisReply *reply;
    void *rulesBinding;
    unsigned int result = peekAction(tree, &rulesBinding, &reply);
    if (result != RULES_OK) {
        return result;
    }

    *state = reply->element[1]->str;
    *messages = reply->element[2]->str;
    actionContext *context = malloc(sizeof(actionContext));
    if (!context) {
        return ERR_OUT_OF_MEMORY;
    }
    
    context->reply = reply;
    context->rulesBinding = rulesBinding;
    *actionHandle = context;
    *actionBinding = rulesBinding;
    return RULES_OK;
}

unsigned int startUpdateState(unsigned int handle, 
                              void *actionHandle, 
                              char *state,
                              void **rulesBinding,
                              unsigned int *replyCount) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    char *commands[MAX_COMMAND_COUNT];
    unsigned int result = RULES_OK;
    unsigned int commandCount = 0;
    result = handleState(tree, 
                         state,
                         commands,
                         &commandCount,
                         rulesBinding);
    if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
        //reply object should be freed by the app during abandonAction
        freeCommands(commands, commandCount);
        return result;
    }

    result = startNonBlockingBatch(*rulesBinding, commands, commandCount, replyCount);
    return result;

}

unsigned int completeAction(unsigned int handle, 
                            void *actionHandle, 
                            char *state) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    char *commands[MAX_COMMAND_COUNT];
    unsigned int commandCount = 0;
    actionContext *context = (actionContext*)actionHandle;
    redisReply *reply = context->reply;
    void *rulesBinding = context->rulesBinding;
    
    unsigned int result = formatRemoveAction(rulesBinding, 
                                             reply->element[0]->str, 
                                             &commands[commandCount]);
    if (result != RULES_OK) {
        //reply object should be freed by the app during abandonAction
        return result;
    }

    ++commandCount;
    result = handleState(tree, 
                         state,
                         commands,
                         &commandCount,
                         &rulesBinding);
    if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
        //reply object should be freed by the app during abandonAction
        freeCommands(commands, commandCount);
        return result;
    }

    result = executeBatch(rulesBinding, commands, commandCount); 
    if (result != RULES_OK && result != ERR_EVENT_OBSERVED) {
        //reply object should be freed by the app during abandonAction
        return result;
    }

    freeReplyObject(reply);
    free(actionHandle);
    return RULES_OK;
}

unsigned int completeAndStartAction(unsigned int handle, 
                                    unsigned int expectedReplies,
                                    void *actionHandle, 
                                    char **messages) {
    char *commands[MAX_COMMAND_COUNT];
    unsigned int commandCount = 0;
    actionContext *context = (actionContext*)actionHandle;
    redisReply *reply = context->reply;
    void *rulesBinding = context->rulesBinding;
    
    unsigned int result = formatRemoveAction(rulesBinding, 
                                             reply->element[0]->str, 
                                             &commands[commandCount]);
    if (result != RULES_OK) {
        //reply object should be freed by the app during abandonAction
        return result;
    }

    ++commandCount;
    if (commandCount == MAX_COMMAND_COUNT) {
        //reply object should be freed by the app during abandonAction
        freeCommands(commands, commandCount);
        return ERR_MAX_COMMAND_COUNT;
    }

    result = formatPeekAction(rulesBinding, 
                              reply->element[0]->str,
                              &commands[commandCount]);
    if (result != RULES_OK) {
        //reply object should be freed by the app during abandonAction
        freeCommands(commands, commandCount);
        return result;
    }

    ++commandCount;
    redisReply *newReply;
    result = executeBatchWithReply(rulesBinding, 
                                   expectedReplies, 
                                   commands, 
                                   commandCount, 
                                   &newReply);  
    if (result != RULES_OK && result != ERR_EVENT_OBSERVED) {
        //reply object should be freed by the app during abandonAction
        return result;
    }
    
    freeReplyObject(reply);
    if (newReply == NULL) {
        free(actionHandle);
        return ERR_NO_ACTION_AVAILABLE;
    }

    *messages = newReply->element[1]->str;
    context->reply = newReply;
    return RULES_OK;
}

unsigned int abandonAction(unsigned int handle, void *actionHandle) {
    freeReplyObject(((actionContext*)actionHandle)->reply);
    free(actionHandle);
    return RULES_OK;
}

unsigned int queueMessage(unsigned int handle, unsigned int queueAction, char *sid, char *destination, char *message) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    void *rulesBinding;
    if (!sid) {
        sid = "0";
    }

    unsigned int result = resolveBinding(tree, sid, &rulesBinding);
    if (result != RULES_OK) {
        return result;
    }

    return registerMessage(rulesBinding, queueAction, destination, message);
}

unsigned int startTimer(unsigned int handle, char *sid, unsigned int duration, char manualReset, char *timer) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    void *rulesBinding;
    if (!sid) {
        sid = "0";
    }
    
    unsigned int result = resolveBinding(tree, sid, &rulesBinding);
    if (result != RULES_OK) {
        return result;
    }

    return registerTimer(rulesBinding, duration, manualReset, timer);
}

unsigned int cancelTimer(unsigned int handle, char *sid, char *timerName) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    void *rulesBinding;
    if (!sid) {
        sid = "0";
    }

    unsigned int result = resolveBinding(tree, sid, &rulesBinding);
    if (result != RULES_OK) {
        return result;
    }

    return removeTimer(rulesBinding, timerName);
}

unsigned int renewActionLease(unsigned int handle, char *sid) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    void *rulesBinding;
    if (!sid) {
        sid = "0";
    }

    unsigned int result = resolveBinding(tree, sid, &rulesBinding);
    if (result != RULES_OK) {
        return result;
    }

    return updateAction(rulesBinding, sid);
}

