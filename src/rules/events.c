
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
#define MAX_STATE_PROPERTY_TIME 2
#define MAX_COMMAND_COUNT 10000
#define MAX_ADD_COUNT 1000
#define MAX_EVAL_COUNT 1000

#define OP_BOOL_BOOL 0x0404
#define OP_BOOL_INT 0x0402
#define OP_BOOL_DOUBLE 0x0403
#define OP_BOOL_STRING 0x0401
#define OP_INT_BOOL 0x0204
#define OP_INT_INT 0x0202
#define OP_INT_DOUBLE 0x0203
#define OP_INT_STRING 0x0201
#define OP_DOUBLE_BOOL 0x0304
#define OP_DOUBLE_INT 0x0302
#define OP_DOUBLE_DOUBLE 0x0303
#define OP_DOUBLE_STRING 0x0301
#define OP_STRING_BOOL 0x0104
#define OP_STRING_INT 0x0102
#define OP_STRING_DOUBLE 0x0103
#define OP_STRING_STRING 0x0101

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

static unsigned int reduceStateIdiom(ruleset *tree, 
                                     char *sid,
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
                                 unsigned short *evalPriorities,
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
            unsigned int index = *evalCount;
            while (index > 0 && node->value.c.priority < evalPriorities[index - 1]) {
                evalKeys[index] = evalKeys[index - 1];
                evalPriorities[index] = evalPriorities[index - 1];
                --index;
            }

            evalKeys[index] = evalKey;
            evalPriorities[index] = node->value.c.priority;
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
                               unsigned short *evalPriorities,
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
                        evalPriorities,
                        evalCount,
                        addKeys,
                        addCount,
                        removeCommand,
                        rulesBinding);
}


static unsigned int valueToProperty(ruleset *tree,
                                    char *sid,
                                    jsonValue *sourceValue, 
                                    char **state,
                                    unsigned char *releaseState,
                                    jsonProperty **targetProperty) {

    unsigned int result = RULES_OK;
    *releaseState = 0;
    switch(sourceValue->type) {
        case JSON_STATE_IDIOM:
            result = reduceStateIdiom(tree, 
                                    sid, 
                                    sourceValue->value.idiomOffset,
                                    state,
                                    releaseState,
                                    targetProperty);

            return result;
        case JSON_STATE_PROPERTY:
            if (sourceValue->value.property.idOffset) {
                sid = &tree->stringPool[sourceValue->value.property.idOffset];
            }
            result = fetchStateProperty(tree, 
                                        sid, 
                                        sourceValue->value.property.hash, 
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
                                            sourceValue->value.property.hash, 
                                            MAX_STATE_PROPERTY_TIME, 
                                            0,
                                            state,
                                            targetProperty);
            }

            return result;
        case JSON_STRING:
            *state = &tree->stringPool[sourceValue->value.stringOffset];
            (*targetProperty)->valueLength = strlen(*state);
            (*targetProperty)->valueOffset = 0;
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
            *state = (char*)calloc(1, sizeof(char));
            if (!*state) {
                return ERR_OUT_OF_MEMORY;
            }
            return RULES_OK;
        }

        if (leftProperty->type == JSON_STRING) {
            leftTemp = leftState[leftProperty->valueOffset + leftProperty->valueLength];
            leftState[leftProperty->valueOffset + leftProperty->valueLength] = '\0';
        } 

        if (rightProperty->type == JSON_STRING) {
            rightTemp = rightState[rightProperty->valueOffset + rightProperty->valueLength];
            rightState[rightProperty->valueOffset + rightProperty->valueLength] = '\0';
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
                         rightState + rightProperty->valueOffset) == -1) {
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
                         rightState + rightProperty->valueOffset) == -1) {
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
                         rightState + rightProperty->valueOffset) == -1) {
                return ERR_OUT_OF_MEMORY;
            }

            break;
        case OP_STRING_BOOL:
            if (asprintf(state, 
                         "%s%s",
                         leftState + leftProperty->valueOffset,
                         rightProperty->value.b ? "true" : "false") == -1) {
                return ERR_OUT_OF_MEMORY;
            }
            break;
        case OP_STRING_INT: 
            if (asprintf(state, 
                         "%s%ld",
                         leftState + leftProperty->valueOffset,
                         rightProperty->value.i) == -1) {
                return ERR_OUT_OF_MEMORY;
            }
            break;
        case OP_STRING_DOUBLE: 
            if (asprintf(state, 
                         "%s%ld",
                         leftState + leftProperty->valueOffset,
                         rightProperty->value.i) == -1) {
                return ERR_OUT_OF_MEMORY;
            }
            break;
        case OP_STRING_STRING:
            if (asprintf(state, 
                         "%s%s",
                         leftState + leftProperty->valueOffset,
                         rightState + rightProperty->valueOffset) == -1) {
                return ERR_OUT_OF_MEMORY;
            }
            break;
    }    

    if (leftProperty->type == JSON_STRING || rightProperty->type == JSON_STRING) {
        targetValue->type = JSON_STRING;
        targetValue->valueOffset = 0;
        targetValue->valueLength = strlen(*state);
                
        if (leftTemp) {
            leftState[leftProperty->valueOffset + leftProperty->valueLength] = leftTemp;
        } 

        if (rightTemp) {
            rightState[rightProperty->valueOffset + rightProperty->valueLength] = rightTemp;
        }
    }

    return RULES_OK;
}

static unsigned int reduceStateIdiom(ruleset *tree, 
                                     char *sid,
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
				sprintf_s(leftStringInt, rightLength, "%ld", currentProperty->value.i);
#else
				char leftStringInt[rightLength];
				snprintf(leftStringInt, rightLength, "%ld", currentProperty->value.i);
#endif         
                *propertyMatch = compareStringProperty(leftStringInt, 
                                                       rightState + rightProperty->valueOffset,
                                                       rightProperty->valueLength, 
                                                       alphaOp);
            }
            break;
        case OP_DOUBLE_BOOL:
            *propertyMatch = compareDouble(currentProperty->value.i, rightProperty->value.b, alphaOp);
            break;
        case OP_DOUBLE_INT: 
            *propertyMatch = compareDouble(currentProperty->value.i, rightProperty->value.i, alphaOp);
            break;
        case OP_DOUBLE_DOUBLE: 
            *propertyMatch = compareDouble(currentProperty->value.d, rightProperty->value.d, alphaOp);
            break;
        case OP_DOUBLE_STRING:
            {
                rightLength = rightProperty->valueLength + 1;
#ifdef _WIN32
				char *leftStringDouble = (char *)_alloca(sizeof(char)*(rightLength));
				sprintf_s(leftStringDouble, rightLength, "%f", currentProperty->value.d);
#else
				char leftStringDouble[rightLength];
				snprintf(leftStringDouble, rightLength, "%f", currentProperty->value.d);
#endif         
                *propertyMatch = compareStringProperty(leftStringDouble,
                                                       rightState + rightProperty->valueOffset,
                                                       rightProperty->valueLength, 
                                                       alphaOp);
            }
            break;
        case OP_STRING_BOOL:
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
        case OP_STRING_INT: 
            {
                leftLength = currentProperty->valueLength + 1;
#ifdef _WIN32
				char *rightStringInt = (char *)_alloca(sizeof(char)*(leftLength));
				sprintf_s(rightStringInt, leftLength, "%ld", rightProperty->value.i);
#else
				char rightStringInt[leftLength];
				snprintf(rightStringInt, leftLength, "%ld", rightProperty->value.i);
#endif
                *propertyMatch = compareString(message + currentProperty->valueOffset, 
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
				sprintf_s(rightStringDouble, leftLength, "%f", rightProperty->value.d);
#else
				char rightStringDouble[leftLength];
				snprintf(rightStringDouble, leftLength, "%f", rightProperty->value.d);
#endif              
                *propertyMatch = compareString(message + currentProperty->valueOffset, 
                                               currentProperty->valueLength, 
                                               rightStringDouble, 
                                               alphaOp);
            }
            break;
        case OP_STRING_STRING:
            *propertyMatch = compareStringAndStringProperty(message + currentProperty->valueOffset, 
                                                            currentProperty->valueLength, 
                                                            rightState + rightProperty->valueOffset,
                                                            rightProperty->valueLength,
                                                            alphaOp);
            break;
    }
    
    if (releaseRightState) {
        free(rightState);
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
                                unsigned char actionType,
                                char **evalKeys,
                                unsigned short *evalPriorities,
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
                        if (hashNode->value.a.right.type == JSON_EVENT_PROPERTY || hashNode->value.a.right.type == JSON_EVENT_IDIOM) {
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
                unsigned int bresult = handleBeta(tree, 
                                    sid, 
                                    mid,
                                    &tree->nodePool[betaList[entry]],  
                                    actionType, 
                                    evalKeys,
                                    evalPriorities,
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

static unsigned int handleMessageCore(ruleset *tree,
                                      char *state,
                                      char *message, 
                                      jsonProperty *properties, 
                                      unsigned int propertiesLength, 
                                      unsigned int midIndex, 
                                      unsigned int sidIndex, 
                                      unsigned char actionType,
                                      char **commands,
                                      unsigned int *commandCount,
                                      void **rulesBinding) {
    jsonProperty *midProperty;
    int midLength; 
    jsonProperty *sidProperty;
    int sidLength;
    char *storeCommand;
    int result = getId(properties, sidIndex, &sidProperty, &sidLength);
    if (result != RULES_OK) {
        return result;
    }
#ifdef _WIN32
	char *sid = (char *)_alloca(sizeof(char)*(sidLength + 1));
#else
	char sid[sidLength + 1];
#endif  
    strncpy(sid, message + sidProperty->valueOffset, sidLength);
    sid[sidLength] = '\0';

    result = getId(properties, midIndex, &midProperty, &midLength);
    if (result != RULES_OK) {
        return result;
    }
#ifdef _WIN32
	char *mid = (char *)_alloca(sizeof(char)*(midLength + 1));
#else
	char mid[midLength + 1];
#endif  
    strncpy(mid, message + midProperty->valueOffset, midLength);
    mid[midLength] = '\0';
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

        if (*commandCount == MAX_COMMAND_COUNT) {
            return ERR_MAX_COMMAND_COUNT;
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
    unsigned short evalPriorities[MAX_EVAL_COUNT];
    unsigned int evalCount = 0;
    result = handleAlpha(tree, 
                         sid, 
                         mid,
                         message, 
                         properties, 
                         propertiesLength,
                         &tree->nodePool[NODE_M_OFFSET].value.a, 
                         actionType, 
                         evalKeys,
                         evalPriorities,
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
                                        properties,
                                        propertiesLength,
                                        actionType == ACTION_ASSERT_FACT ? 1 : 0,
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
                                        properties,
                                        propertiesLength,
                                        actionType,
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

            result = refreshState(tree,sid);
            if (result != ERR_NEW_SESSION) {
                return result;
            }            
#ifdef _WIN32
			char *stateMessage = (char *)_alloca(sizeof(char)*(36 + sidLength));
			char *newState = (char *)_alloca(sizeof(char)*(12 + sidLength));
			if (sidProperty->type == JSON_STRING) {
				sprintf_s(stateMessage, 36 + sidLength, "{\"id\":\"$s\", \"sid\":\"%s\", \"$s\":1}", sid);
				sprintf_s(newState, 12 + sidLength, "{\"sid\":\"%s\"}", sid);
			}
			else {
				sprintf_s(stateMessage, 36 + sidLength, "{\"id\":\"$s\", \"sid\":%s, \"$s\":1}", sid);
				sprintf_s(newState, 12 + sidLength, "{\"sid\":%s}", sid);
			}
#else
			char stateMessage[36 + sidLength];
			char newState[12 + sidLength];
			if (sidProperty->type == JSON_STRING) {
				snprintf(stateMessage, 36 + sidLength, "{\"id\":\"$s\", \"sid\":\"%s\", \"$s\":1}", sid);
				snprintf(newState, 12 + sidLength, "{\"sid\":\"%s\"}", sid);
			}
			else {
				snprintf(stateMessage, 36 + sidLength, "{\"id\":\"$s\", \"sid\":%s, \"$s\":1}", sid);
				snprintf(newState, 12 + sidLength, "{\"sid\":%s}", sid);
			}
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

    return handleMessageCore(tree,
                            state, 
                            message, 
                            properties, 
                            propertiesLength, 
                            midIndex, 
                            sidIndex,  
                            actionType,
                            commands,
                            commandCount,
                            rulesBinding);
}

static unsigned int handleMessages(void *handle, 
                                   unsigned char actionType,
                                   char *messages, 
                                   char **commands,
                                   unsigned int *commandCount,
                                   unsigned int *resultsLength,  
                                   unsigned int **results,
                                   void **rulesBinding) {
    unsigned int messagesLength = 64;
    unsigned int *resultsArray = malloc(sizeof(unsigned int) * messagesLength);
    if (!resultsArray) {
        return ERR_OUT_OF_MEMORY;
    }

    *results = resultsArray;
    *resultsLength = 0;
    unsigned int result;
    jsonProperty properties[MAX_EVENT_PROPERTIES];
    unsigned int propertiesLength = 0;
    unsigned int propertiesMidIndex = UNDEFINED_INDEX;
    unsigned int propertiesSidIndex = UNDEFINED_INDEX;
    
    char *first = messages;
    char *last = NULL;
    char lastTemp;

    while (*first != '{' && *first != '\0' ) {
        ++first;
    }

    while (constructObject(NULL, 
                           first, 
                           0,
                           MAX_EVENT_PROPERTIES,
                           properties, 
                           &propertiesLength, 
                           &propertiesMidIndex,
                           &propertiesSidIndex,
                           &last) == RULES_OK) {

        while (*last != ',' && *last != ']' ) {
            ++last;
        }

        lastTemp = *last;
        *last = '\0';
        result = handleMessageCore(handle,
                                 NULL, 
                                 first, 
                                 properties, 
                                 propertiesLength,
                                 propertiesMidIndex,
                                 propertiesSidIndex, 
                                 actionType, 
                                 commands,
                                 commandCount,
                                 rulesBinding);
        
        *last = lastTemp;
        if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
            free(resultsArray);
            return result;
        }

        if (*resultsLength >= messagesLength) {
            messagesLength = messagesLength * 2;
            resultsArray = realloc(resultsArray, sizeof(unsigned int) * messagesLength);
            if (!resultsArray) {
                return ERR_OUT_OF_MEMORY;
            }
            *results = resultsArray;
        }

        resultsArray[*resultsLength] = result;
        ++*resultsLength;
        propertiesLength = 0;
        propertiesMidIndex = UNDEFINED_INDEX;
        propertiesSidIndex = UNDEFINED_INDEX;        

        first = last;
        while (*first != '{' && *first != '\0' ) {
            ++first;
        }
    }

    return RULES_OK;
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
	char *stateMessage = (char *)_alloca(sizeof(char)*(30 + stateLength - 1));
#else
	char stateMessage[30 + stateLength - 1];
#endif
    int randomMid = rand();
#ifdef _WIN32
    sprintf_s(stateMessage, 30 + stateLength - 1, "{\"id\":%d, \"$s\":1, %s", randomMid, stateMessagePostfix);
#else
	snprintf(stateMessage, 30 + stateLength - 1, "{\"id\":%d, \"$s\":1, %s", randomMid, stateMessagePostfix);
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

static unsigned int handleTimers(void *handle, 
                                 char **commands,
                                 unsigned int *commandCount,
                                 void **rulesBinding) {
    redisReply *reply;
    unsigned int result = peekTimers(handle, rulesBinding, &reply);
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
        result = handleMessage(handle, 
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

static unsigned int startHandleMessage(void *handle, 
                                       char *message, 
                                       unsigned char actionType,
                                       void **rulesBinding,
                                       unsigned int *replyCount) {
    char *commands[MAX_COMMAND_COUNT];
    unsigned int commandCount = 0;
    unsigned int result = handleMessage(handle, 
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

static unsigned int executeHandleMessage(void *handle, 
                                         char *message, 
                                         unsigned char actionType) {
    char *commands[MAX_COMMAND_COUNT];
    unsigned int commandCount = 0;
    void *rulesBinding = NULL;
    unsigned int result = handleMessage(handle, 
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

static unsigned int startHandleMessages(void *handle, 
                                        char *messages, 
                                        unsigned char actionType,
                                        unsigned int *resultsLength, 
                                        unsigned int **results,
                                        void **rulesBinding,
                                        unsigned int *replyCount) {
    char *commands[MAX_COMMAND_COUNT];
    unsigned int commandCount = 0;
    unsigned int result = handleMessages(handle,
                                         actionType,
                                         messages,
                                         commands,
                                         &commandCount,
                                         resultsLength,
                                         results,
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

static unsigned int executeHandleMessages(void *handle, 
                                          char *messages, 
                                          unsigned char actionType,
                                          unsigned int *resultsLength, 
                                          unsigned int **results) {
    char *commands[MAX_COMMAND_COUNT];
    unsigned int commandCount = 0;
    void *rulesBinding = NULL;
    unsigned int result = handleMessages(handle,
                                         actionType,
                                         messages,
                                         commands,
                                         &commandCount,
                                         resultsLength,
                                         results,
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
    return completeNonBlockingBatch(rulesBinding, replyCount);
}

unsigned int assertEvent(void *handle, char *message) {
    return executeHandleMessage(handle, message, ACTION_ASSERT_EVENT);
}

unsigned int startAssertEvent(void *handle, 
                             char *message, 
                             void **rulesBinding, 
                             unsigned int *replyCount) {
    return startHandleMessage(handle, message, ACTION_ASSERT_EVENT, rulesBinding, replyCount);
}

unsigned int assertEvents(void *handle, 
                          char *messages, 
                          unsigned int *resultsLength, 
                          unsigned int **results) {
    return executeHandleMessages(handle, messages, ACTION_ASSERT_EVENT, resultsLength, results);
}

unsigned int startAssertEvents(void *handle, 
                              char *messages, 
                              unsigned int *resultsLength, 
                              unsigned int **results, 
                              void **rulesBinding, 
                              unsigned int *replyCount) {
    return startHandleMessages(handle, messages, ACTION_ASSERT_EVENT, resultsLength, results, rulesBinding, replyCount);
}

unsigned int retractEvent(void *handle, char *message) {
    return executeHandleMessage(handle, message, ACTION_REMOVE_EVENT);
}

unsigned int startAssertFact(void *handle, 
                             char *message, 
                             void **rulesBinding, 
                             unsigned int *replyCount) {
    return startHandleMessage(handle, message, ACTION_ASSERT_FACT, rulesBinding, replyCount);
}

unsigned int assertFact(void *handle, char *message) {
    return executeHandleMessage(handle, message, ACTION_ASSERT_FACT);
}

unsigned int startAssertFacts(void *handle, 
                              char *messages, 
                              unsigned int *resultsLength, 
                              unsigned int **results, 
                             void **rulesBinding, 
                             unsigned int *replyCount) {
    return startHandleMessages(handle, messages, ACTION_ASSERT_FACT, resultsLength, results, rulesBinding, replyCount);
}

unsigned int assertFacts(void *handle, 
                          char *messages, 
                          unsigned int *resultsLength, 
                          unsigned int **results) {
    return executeHandleMessages(handle, messages, ACTION_ASSERT_FACT, resultsLength, results);
}

unsigned int retractFact(void *handle, char *message) {
    return executeHandleMessage(handle, message, ACTION_REMOVE_FACT);
}

unsigned int startRetractFact(void *handle, 
                             char *message, 
                             void **rulesBinding, 
                             unsigned int *replyCount) {
    return startHandleMessage(handle, message, ACTION_REMOVE_FACT, rulesBinding, replyCount);
}

unsigned int retractFacts(void *handle, 
                          char *messages, 
                          unsigned int *resultsLength, 
                          unsigned int **results) {
    return executeHandleMessages(handle, messages, ACTION_REMOVE_FACT, resultsLength, results);
}

unsigned int startRetractFacts(void *handle, 
                              char *messages, 
                              unsigned int *resultsLength, 
                              unsigned int **results, 
                              void **rulesBinding, 
                              unsigned int *replyCount) {
    return startHandleMessages(handle, messages, ACTION_REMOVE_FACT, resultsLength, results, rulesBinding, replyCount);
}

unsigned int assertState(void *handle, char *state) {
    char *commands[MAX_COMMAND_COUNT];
    unsigned int commandCount = 0;
    void *rulesBinding = NULL;
    unsigned int result = handleState(handle, 
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

unsigned int assertTimers(void *handle) {
    char *commands[MAX_COMMAND_COUNT];
    unsigned int commandCount = 0;
    void *rulesBinding = NULL;
    unsigned int result = handleTimers(handle, 
                                       commands,
                                       &commandCount,
                                       &rulesBinding);
    if (result != RULES_OK) {
        freeCommands(commands, commandCount);
        return result;
    }

    return executeBatch(rulesBinding, commands, commandCount);
}

unsigned int startAction(void *handle, 
                         char **state, 
                         char **messages, 
                         void **actionHandle,
                         void **actionBinding) {
    redisReply *reply;
    void *rulesBinding;
    unsigned int result = peekAction(handle, &rulesBinding, &reply);
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

unsigned int startUpdateState(void *handle, 
                              void *actionHandle, 
                              char *state,
                              void **rulesBinding,
                              unsigned int *replyCount) {
    char *commands[MAX_COMMAND_COUNT];
    unsigned int result = RULES_OK;
    unsigned int commandCount = 0;
    result = handleState(handle, 
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

unsigned int completeAction(void *handle, 
                            void *actionHandle, 
                            char *state) {
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
    result = handleState(handle, 
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
    if (result != RULES_OK) {
        //reply object should be freed by the app during abandonAction
        return result;
    }

    freeReplyObject(reply);
    free(actionHandle);
    return result;
}

unsigned int completeAndStartAction(void *handle, 
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
    if (result != RULES_OK) {
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

unsigned int abandonAction(void *handle, void *actionHandle) {
    freeReplyObject(((actionContext*)actionHandle)->reply);
    free(actionHandle);
    return RULES_OK;
}

unsigned int queueMessage(void *handle, unsigned int queueAction, char *sid, char *destination, char *message) {
    void *rulesBinding;
    unsigned int result = resolveBinding(handle, sid, &rulesBinding);
    if (result != RULES_OK) {
        return result;
    }

    return registerMessage(rulesBinding, queueAction, destination, message);
}

unsigned int startTimer(void *handle, char *sid, unsigned int duration, char *timer) {
    void *rulesBinding;
    unsigned int result = resolveBinding(handle, sid, &rulesBinding);
    if (result != RULES_OK) {
        return result;
    }

    return registerTimer(rulesBinding, duration, timer);
}

unsigned int cancelTimer(void *handle, char *sid, char *timer) {
    void *rulesBinding;
    unsigned int result = resolveBinding(handle, sid, &rulesBinding);
    if (result != RULES_OK) {
        return result;
    }

    return removeTimer(rulesBinding, timer);
}

unsigned int renewActionLease(void *handle, char *sid) {
    void *rulesBinding;
    unsigned int result = resolveBinding(handle, sid, &rulesBinding);
    if (result != RULES_OK) {
        return result;
    }

    return updateAction(rulesBinding, sid);
}

