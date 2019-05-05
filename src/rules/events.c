
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
#define MAX_INT_LENGTH 256
#define MAX_DOUBLE_LENGTH 256

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

#define ONE_HASH 873244444

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
                                  char *message, 
                                  unsigned char actionType, 
                                  char **commands,
                                  unsigned int *commandCount,
                                  unsigned int *messageOffset,
                                  unsigned int *stateOffset);

static unsigned int reduceExpression(ruleset *tree,
                                     stateNode *state, 
                                     expression *currentExpression,
                                     jsonObject *messageObject,
                                     leftFrameNode *context,
                                     jsonProperty *targetProperty);

static unsigned int reduceString(char *left, 
                                 unsigned short leftLength, 
                                 char *right, 
                                 unsigned short rightLength,
                                 unsigned char op,
                                 jsonProperty *targetProperty) {
    int length = (leftLength > rightLength ? leftLength: rightLength);
    int result = strncmp(left, right, length);
    switch(op) {
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV: 
            return ERR_OPERATION_NOT_SUPPORTED;
        case OP_LT:
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = (result < 0);
            break;
        case OP_LTE:
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = (result <= 0);
            break;
        case OP_GT:
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = (result > 0);
            break;
        case OP_GTE: 
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = (result >= 0);
            break;
        case OP_EQ:
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = (result == 0);
            break;
        case OP_NEQ:
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = (result != 0);
            break;
    }

    return RULES_OK;
}

static unsigned int reduceBool(unsigned char left, 
                               unsigned char right, 
                               unsigned char op,
                               jsonProperty *targetProperty) {
    switch(op) {
        case OP_DIV: 
        case OP_SUB:
            return ERR_OPERATION_NOT_SUPPORTED;
        case OP_ADD:
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = left & right;
            break;
        case OP_MUL:
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = left ^ right;
            break;
        case OP_LT:
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = (left < right);
            break;
        case OP_LTE:
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = (left <= right);
            break;
        case OP_GT:
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = (left > right);
            break;
        case OP_GTE: 
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = (left >= right);
            break;
        case OP_EQ:
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = (left == right);
            break;
        case OP_NEQ:
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = (left != right);
            break;
    }

    return RULES_OK;
}

static unsigned int reduceInt(long left, 
                              long right, 
                              unsigned char op,
                              jsonProperty *targetProperty) {
    switch(op) {
        case OP_ADD:
            targetProperty->type = JSON_INT;
            targetProperty->value.i = left + right;
            break;
        case OP_SUB:
            targetProperty->type = JSON_INT;
            targetProperty->value.i = left - right;
            break;
        case OP_MUL:
            targetProperty->type = JSON_INT;
            targetProperty->value.i = left * right;
            break;
        case OP_DIV: 
            targetProperty->type = JSON_INT;
            targetProperty->value.i = left / right;
            break;
        case OP_LT:
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = (left < right);
            break;
        case OP_LTE:
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = (left <= right);
            break;
        case OP_GT:
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = (left > right);
            break;
        case OP_GTE: 
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = (left >= right);
            break;
        case OP_EQ:
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = (left == right);
            break;
        case OP_NEQ:
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = (left != right);
            break;
    }

    return RULES_OK;
}

static unsigned int reduceDouble(double left, 
                                 double right, 
                                 unsigned char op,
                                 jsonProperty *targetProperty) {
    switch(op) {
        case OP_ADD:
            targetProperty->type = JSON_DOUBLE;
            targetProperty->value.d = left + right;
            break;
        case OP_SUB:
            targetProperty->type = JSON_DOUBLE;
            targetProperty->value.d = left - right;
            break;
        case OP_MUL:
            targetProperty->type = JSON_DOUBLE;
            targetProperty->value.d = left * right;
            break;
        case OP_DIV: 
            targetProperty->type = JSON_DOUBLE;
            targetProperty->value.d = left / right;
            break;
        case OP_LT:
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = (left < right);
            break;
        case OP_LTE:
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = (left <= right);
            break;
        case OP_GT:
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = (left > right);
            break;
        case OP_GTE: 
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = (left >= right);
            break;
        case OP_EQ:
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = (left == right);
            break;
        case OP_NEQ:
            targetProperty->type = JSON_BOOL;
            targetProperty->value.b = (left != right);
            break;
    }

    return RULES_OK;
}

static unsigned int reduceOperand(ruleset *tree,
                                  stateNode *state, 
                                  operand *sourceOperand, 
                                  jsonObject *messageObject,
                                  leftFrameNode *context,
                                  jsonProperty **targetProperty) {
    (*targetProperty)->type = sourceOperand->type;
    switch(sourceOperand->type) {
        case JSON_EXPRESSION:
        case JSON_MESSAGE_EXPRESSION:
            {
                expression *currentExpression = &tree->expressionPool[sourceOperand->value.expressionOffset];
                return reduceExpression(tree, 
                                        state,
                                        currentExpression,
                                        messageObject,
                                        context,
                                        *targetProperty);
            }
        case JSON_IDENTIFIER:
            CHECK_RESULT(getMessageFromFrame(state,
                                             context->messages, 
                                             sourceOperand->value.id.nameHash, 
                                             &messageObject));
            // break omitted intentionally, continue to next case
        case JSON_MESSAGE_IDENTIFIER:
            CHECK_RESULT(getObjectProperty(messageObject,
                                           sourceOperand->value.id.propertyNameHash,
                                           targetProperty));
            return RULES_OK;
        case JSON_STRING:
            {
                char *stringValue = &tree->stringPool[sourceOperand->value.stringOffset];
                (*targetProperty)->valueLength = strlen(stringValue);
                (*targetProperty)->valueOffset = 0;
                (*targetProperty)->value.s = stringValue;
                return RULES_OK;
            }
        case JSON_INT:
            (*targetProperty)->value.i = sourceOperand->value.i;
            return RULES_OK;
        case JSON_DOUBLE:
            (*targetProperty)->value.d = sourceOperand->value.d;
            return RULES_OK;
        case JSON_BOOL:
            (*targetProperty)->value.b = sourceOperand->value.b;
            return RULES_OK;
    }

    return ERR_OPERATION_NOT_SUPPORTED;
}

static unsigned int reduceProperties(unsigned char operator,
                                     jsonProperty *leftProperty,
                                     jsonProperty *rightProperty,
                                     jsonProperty *targetProperty) {
    unsigned short type = leftProperty->type << 8;
    type = type + rightProperty->type;
    switch(type) {
        case OP_BOOL_BOOL:
            return reduceBool(leftProperty->value.b, rightProperty->value.b, operator, targetProperty); 
        case OP_BOOL_INT: 
            return reduceInt(leftProperty->value.b, rightProperty->value.i, operator, targetProperty);
        case OP_BOOL_DOUBLE: 
            return reduceDouble(leftProperty->value.b, rightProperty->value.d, operator, targetProperty); 
        case OP_INT_BOOL:
            return reduceInt(leftProperty->value.i, rightProperty->value.b, operator, targetProperty); 
        case OP_INT_INT:
            return reduceInt(leftProperty->value.i, rightProperty->value.i, operator, targetProperty); 
        case OP_INT_DOUBLE: 
            return reduceDouble(leftProperty->value.i, rightProperty->value.d, operator, targetProperty); 
        case OP_DOUBLE_BOOL:
            return reduceDouble(leftProperty->value.d, rightProperty->value.b, operator, targetProperty); 
        case OP_DOUBLE_INT:
            return reduceDouble(leftProperty->value.d, rightProperty->value.i, operator, targetProperty); 
        case OP_DOUBLE_DOUBLE:
            return reduceDouble(leftProperty->value.d, rightProperty->value.d, operator, targetProperty); 
        case OP_BOOL_STRING:
            if (leftProperty->value.b) {
                return reduceString("true ",
                                    4,
                                    rightProperty->value.s,
                                    rightProperty->valueLength, 
                                    operator,
                                    targetProperty);
            }
            else {
                return reduceString("false ",
                                    5,
                                    rightProperty->value.s,
                                    rightProperty->valueLength, 
                                    operator,
                                    targetProperty);
            }
        case OP_INT_STRING:
            {
#ifdef _WIN32
                char *leftString = (char *)_alloca(sizeof(char)*(MAX_INT_LENGTH + 1));
                sprintf_s(leftString, sizeof(char)*(MAX_INT_LENGTH + 1), "%ld", leftProperty->value.i);
#else
                char leftString[MAX_INT_LENGTH + 1];
                snprintf(leftString, sizeof(char)*(MAX_INT_LENGTH + 1), "%ld", leftProperty->value.i);
#endif         
                return reduceString(leftString,
                                    leftProperty->valueLength,
                                    rightProperty->value.s,
                                    rightProperty->valueLength, 
                                    operator,
                                    targetProperty);
            }
        case OP_DOUBLE_STRING:
            {
#ifdef _WIN32
                char *leftString = (char *)_alloca(sizeof(char)*(MAX_DOUBLE_LENGTH + 1));
                sprintf_s(leftString, sizeof(char)*(MAX_DOUBLE_LENGTH + 1), "%f", leftProperty->value.d);
#else
                char leftString[MAX_DOUBLE_LENGTH + 1];
                snprintf(leftString, sizeof(char)*(MAX_DOUBLE_LENGTH + 1), "%f", leftProperty->value.d);
#endif         
                return reduceString(leftString,
                                    leftProperty->valueLength,
                                    rightProperty->value.s,
                                    rightProperty->valueLength, 
                                    operator,
                                    targetProperty);
            }
        case OP_STRING_BOOL:
            if (rightProperty->value.b) {
                return reduceString(leftProperty->value.s,
                                    leftProperty->valueLength, 
                                    "true ",
                                    4,
                                    operator,
                                    targetProperty);
            }
            else {
                return reduceString(leftProperty->value.s,
                                    leftProperty->valueLength,
                                    "false ",
                                    5, 
                                    operator,
                                    targetProperty);
            }
        case OP_STRING_INT: 
            {
#ifdef _WIN32
                char *rightString = (char *)_alloca(sizeof(char)*(MAX_INT_LENGTH + 1));
                sprintf_s(rightString, sizeof(char)*(MAX_INT_LENGTH + 1), "%ld", rightProperty->value.i);
#else
                char rightString[MAX_INT_LENGTH + 1];
                snprintf(rightString, sizeof(char)*(MAX_INT_LENGTH + 1), "%ld", rightProperty->value.i);
#endif         
                return reduceString(leftProperty->value.s,
                                    leftProperty->valueLength,
                                    rightString,
                                    rightProperty->valueLength,
                                    operator,
                                    targetProperty);
            }
        case OP_STRING_DOUBLE: 
            {
#ifdef _WIN32
                char *rightString = (char *)_alloca(sizeof(char)*(MAX_DOUBLE_LENGTH + 1));
                sprintf_s(rightString, sizeof(char)*(MAX_DOUBLE_LENGTH + 1), "%f", rightProperty->value.d);
#else
                char rightString[MAX_DOUBLE_LENGTH + 1];
                snprintf(rightString, sizeof(char)*(MAX_DOUBLE_LENGTH + 1), "%f", rightProperty->value.d);
#endif         
                return reduceString(leftProperty->value.s,
                                    leftProperty->valueLength,
                                    rightString,
                                    rightProperty->valueLength,
                                    operator,
                                    targetProperty);
            }
        case OP_STRING_STRING:
            return reduceString(leftProperty->value.s,
                                leftProperty->valueLength,
                                rightProperty->value.s,
                                rightProperty->valueLength,
                                operator,
                                targetProperty);
        case OP_BOOL_NIL:
        case OP_INT_NIL:
        case OP_DOUBLE_NIL:
        case OP_STRING_NIL:
            targetProperty->type = JSON_BOOL;
            if (operator == OP_NEQ) {
                targetProperty->value.b = 1;
            } else {
                targetProperty->value.b = 0;
            }

            return RULES_OK;
        case OP_NIL_NIL:
            targetProperty->type = JSON_BOOL;
            if (operator == OP_EQ) {
                targetProperty->value.b = 1;
            } else {
                targetProperty->value.b = 0;
            }

            return RULES_OK;
    }    

    return ERR_OPERATION_NOT_SUPPORTED;
}

static unsigned int reduceExpression(ruleset *tree,
                                     stateNode *state, 
                                     expression *currentExpression,
                                     jsonObject *messageObject,
                                     leftFrameNode *context,
                                     jsonProperty *targetProperty) {
    jsonProperty leftValue;
    jsonProperty *leftProperty = &leftValue;
    CHECK_RESULT(reduceOperand(tree,
                               state,
                               &currentExpression->left,
                               messageObject,
                               context,
                               &leftProperty));

    if (currentExpression->right.type == JSON_REGEX || currentExpression->right.type == JSON_IREGEX) {
        targetProperty->type = JSON_BOOL;
        targetProperty->value.b = evaluateRegex(tree,
                                                leftProperty->value.s, 
                                                leftProperty->valueLength, 
                                                (currentExpression->right.type == JSON_REGEX) ? 0 : 1,
                                                currentExpression->right.value.regex.vocabularyLength,
                                                currentExpression->right.value.regex.statesLength,
                                                currentExpression->right.value.regex.stateMachineOffset);
        return RULES_OK;
    }

    jsonProperty rightValue;
    jsonProperty *rightProperty = &rightValue;
    CHECK_RESULT(reduceOperand(tree,
                               state,
                               &currentExpression->right,
                               messageObject,
                               context,
                               &rightProperty));

    return reduceProperties(currentExpression->operator, 
                            leftProperty, 
                            rightProperty, 
                            targetProperty);
}

static unsigned int reduceExpressionSequence(ruleset *tree,
                                             stateNode *state, 
                                             expressionSequence *exprs,
                                             unsigned short operator,
                                             jsonObject *messageObject,
                                             leftFrameNode *context,
                                             unsigned short *i,
                                             jsonProperty *targetProperty) {
    targetProperty->type = JSON_BOOL;
    targetProperty->value.b = 1;    
    while (*i < exprs->length) {
        expression *currentExpression = &exprs->expressions[*i];
        if (currentExpression->operator == OP_END) {
            return RULES_OK;          
        } 

        if ((operator != OP_NOP) &&
            ((operator == OP_OR && targetProperty->value.b) || 
             (operator == OP_AND && !targetProperty->value.b))) {
            if (currentExpression->operator == OP_AND || currentExpression->operator == OP_OR) {
                jsonProperty dummyProperty;
                dummyProperty.type = JSON_BOOL;
                if (currentExpression->operator == OP_AND) {
                    dummyProperty.value.b = 0;
                } else {
                    dummyProperty.value.b = 1;
                }

                ++*i;
                CHECK_RESULT(reduceExpressionSequence(tree,
                                                      state,
                                                      exprs,
                                                      currentExpression->operator,
                                                      messageObject,
                                                      context,
                                                      i,
                                                      &dummyProperty));
            }
        } else {
            if (currentExpression->operator == OP_AND || currentExpression->operator == OP_OR) {
                ++*i;
                CHECK_RESULT(reduceExpressionSequence(tree,
                                                      state,
                                                      exprs,
                                                      currentExpression->operator,
                                                      messageObject,
                                                      context,
                                                      i,
                                                      targetProperty));
            } else if (currentExpression->operator == OP_IALL || currentExpression->operator == OP_IANY) {
                
            } else {
                CHECK_RESULT(reduceExpression(tree,
                                              state,
                                              currentExpression,
                                              messageObject,
                                              context,
                                              targetProperty));
                if (targetProperty->type != JSON_BOOL) {
                    return ERR_OPERATION_NOT_SUPPORTED;
                }
            }
        }

        ++*i;
    }

    return RULES_OK;
}

static unsigned int getFrameHashForExpression(ruleset *tree,
                                              stateNode *state, 
                                              expression *currentExpression,
                                              jsonObject *messageObject,
                                              leftFrameNode *context,
                                              unsigned int *hash) {
    jsonProperty value;
    jsonProperty *property = &value;
    *hash = ONE_HASH;
                    
    if (context) {
        CHECK_RESULT(reduceOperand(tree,
                                   state,
                                   &currentExpression->right,
                                   NULL,
                                   context,
                                   &property));    
    } else {
        CHECK_RESULT(reduceOperand(tree,
                                   state,
                                   &currentExpression->left,
                                   messageObject,
                                   NULL,
                                   &property));
    }

    switch (property->type) {
        case JSON_STRING:
            *hash = fnv1Hash32(property->value.s, property->valueLength);
            break;
        case JSON_BOOL:
            if (property->value.b) {
                *hash = fnv1Hash32("true", 4);
            } else {
                *hash = fnv1Hash32("false", 5);
            }
            break;
        case JSON_INT:
        {
#ifdef _WIN32
            char *string = (char *)_alloca(sizeof(char)*(MAX_INT_LENGTH + 1));
            sprintf_s(string, sizeof(char)*(MAX_INT_LENGTH + 1), "%ld", property->value.i);
#else
            char string[MAX_INT_LENGTH + 1];
            snprintf(string, sizeof(char)*(MAX_INT_LENGTH + 1), "%ld", property->value.i);
#endif   
            *hash = fnv1Hash32(string, strlen(string));
            break;
        }
        case JSON_DOUBLE:
        {
#ifdef _WIN32
            char *string = (char *)_alloca(sizeof(char)*(MAX_INT_LENGTH + 1));
            sprintf_s(string, sizeof(char)*(MAX_INT_LENGTH + 1), "%f", property->value.d);
#else
            char string[MAX_INT_LENGTH + 1];
            snprintf(string, sizeof(char)*(MAX_INT_LENGTH + 1), "%f", property->value.d);
#endif   
            *hash = fnv1Hash32(string, strlen(string));
            break;
        }
    }

    return RULES_OK;
}

static unsigned int getFrameHash(ruleset *tree,
                                 stateNode *state, 
                                 beta *currentBeta, 
                                 jsonObject *messageObject,
                                 leftFrameNode *context,
                                 unsigned int *hash) {
    expressionSequence *exprs = &currentBeta->expressionSequence;
    *hash = ONE_HASH;
    if (exprs->length == 1) {
        if (exprs->expressions[0].operator == OP_EQ) {
            return getFrameHashForExpression(tree,
                                             state,
                                             &exprs->expressions[0],
                                             messageObject,
                                             context,
                                             hash);
        }
    } else if (exprs->length > 0) {
        if (exprs->expressions[0].operator == OP_AND) {
            *hash = FNV_32_OFFSET_BASIS;
            unsigned int result;
            for (unsigned int i = 1; i < exprs->length; ++ i) {

                if (exprs->expressions[i].operator == OP_EQ) {
                    unsigned int newHash;
                    result = getFrameHashForExpression(tree,
                                                       state,
                                                       &exprs->expressions[i],
                                                       messageObject,
                                                       context,
                                                       &newHash);
                    if (result == RULES_OK) {
                        *hash ^= newHash;
                        *hash *= FNV_32_PRIME;
                    }
                }
            }
        }
    }
    
    return RULES_OK;
}

static unsigned int isBetaMatch(ruleset *tree,
                                stateNode *state, 
                                beta *currentBeta, 
                                jsonObject *messageObject,
                                leftFrameNode *context,
                                unsigned char *propertyMatch) {

    jsonProperty resultProperty;
    unsigned short i = 0;
    CHECK_RESULT(reduceExpressionSequence(tree,
                                         state,
                                         &currentBeta->expressionSequence,
                                         OP_NOP,
                                         messageObject,
                                         context,
                                         &i,
                                         &resultProperty));

    if (resultProperty.type != JSON_BOOL) {
        return ERR_OPERATION_NOT_SUPPORTED;
    }

    *propertyMatch = resultProperty.value.b;

    return RULES_OK;
}

static unsigned int isAlphaMatch(ruleset *tree,
                                 alpha *currentAlpha,
                                 jsonObject *messageObject,
                                 unsigned char *propertyMatch) {

    *propertyMatch = 0;
    if (currentAlpha->expression.operator == OP_EX) {
        *propertyMatch = 1;
    } else {
        jsonProperty resultProperty;
        unsigned int result = reduceExpression(tree,
                                               NULL,
                                               &currentAlpha->expression,
                                               messageObject,
                                               NULL,
                                               &resultProperty);

        if (result == ERR_OPERATION_NOT_SUPPORTED || resultProperty.type != JSON_BOOL) {
            *propertyMatch = 0;
        } else {
            *propertyMatch = resultProperty.value.b;
        }
    }

    return RULES_OK;
}

static void freeCommands(char **commands,
                         unsigned int commandCount) {
    for (unsigned int i = 0; i < commandCount; ++i) {
        free(commands[i]);
    }
}

static unsigned char isDistinct(leftFrameNode *currentFrame, unsigned int currentMessageOffset) {
    for (unsigned short i = 0; i < currentFrame->messageCount; ++i) {
        if (currentFrame->messages[currentFrame->reverseIndex[i]].messageNodeOffset == currentMessageOffset) {
            return 0;
        }
    }

    return 1;
}

static unsigned int handleAction(ruleset *tree, 
                                 stateNode *state, 
                                 node *node,
                                 frameLocation currentFrameLocation,
                                 leftFrameNode *frame) {
#ifdef _PRINT

    printf("handle action %s\n", &tree->stringPool[node->nameOffset]);
    printf("frame %d, %d\n", currentFrameLocation.nodeIndex, currentFrameLocation.frameOffset);
    for (int i = 0; i < MAX_MESSAGE_FRAMES; ++i) {
        if (frame->messages[i].hash) {
            messageNode *node = MESSAGE_NODE(state, frame->messages[i].messageNodeOffset);
            printf("   %d -> %s\n", frame->messages[i].messageNodeOffset, node->jo.content);
        }
    }

#endif

    return RULES_OK;
}

static unsigned int handleBetaFrame(ruleset *tree, 
                                    stateNode *state,
                                    unsigned int oldMessageOffset,
                                    node *oldNode,
                                    node *currentNode,
                                    leftFrameNode *oldFrame,
                                    leftFrameNode *currentFrame) {
    frameLocation currentFrameLocation;
    unsigned int frameType = LEFT_FRAME; 
    
    if (currentNode->type == NODE_ACTION) {
        CHECK_RESULT(createActionFrame(state,
                                       currentNode,
                                       currentFrame,
                                       &currentFrame,
                                       &currentFrameLocation));
    } else if (currentNode->type == NODE_BETA) {
        CHECK_RESULT(createLeftFrame(state,
                                     currentNode,
                                     currentFrame,
                                     &currentFrame,
                                     &currentFrameLocation));
    } else {
        node *aNode = &tree->nodePool[currentNode->value.b.aOffset];
        frameType = (aNode == oldNode) ? A_FRAME : B_FRAME;
        CHECK_RESULT(createConnectorFrame(state,
                                          frameType,
                                          currentNode,
                                          currentFrame,
                                          &currentFrame,
                                          &currentFrameLocation));
    }

    if (oldMessageOffset != UNDEFINED_HASH_OFFSET) {
        CHECK_RESULT(setMessageInFrame(currentFrame,
                                       oldNode->nameOffset,
                                       oldNode->value.b.hash,
                                       oldMessageOffset));

        CHECK_RESULT(addFrameLocation(state,
                                      currentFrameLocation,
                                      oldMessageOffset));  
    }

    if (oldFrame) {
        for (int i = 0; i < oldFrame->messageCount; ++i) {
            messageFrame *currentMessageFrame = &oldFrame->messages[oldFrame->reverseIndex[i]]; 
            CHECK_RESULT(setMessageInFrame(currentFrame,
                                           currentMessageFrame->nameOffset,
                                           currentMessageFrame->hash,
                                           currentMessageFrame->messageNodeOffset));

            CHECK_RESULT(addFrameLocation(state,
                                          currentFrameLocation,
                                          currentMessageFrame->messageNodeOffset));     
        }
    }

    unsigned int frameHash = UNDEFINED_HASH_OFFSET;
    if (currentNode->type == NODE_ACTION) {
        CHECK_RESULT(setActionFrame(state, 
                                    currentFrameLocation));

        return handleAction(tree, 
                            state, 
                            currentNode,
                            currentFrameLocation,
                            currentFrame);
    } else if (currentNode->type == NODE_BETA) {
        CHECK_RESULT(getFrameHash(tree,
                                  state,
                                  &currentNode->value.b,
                                  NULL,
                                  currentFrame,
                                  &frameHash));

        CHECK_RESULT(setLeftFrame(state,
                                  frameHash,
                                  currentFrameLocation));
    } else {
        CHECK_RESULT(setConnectorFrame(state,
                                       frameType,                         
                                       currentFrameLocation));
    }


    node *nextNode = &tree->nodePool[currentNode->value.b.nextOffset];
    if (currentNode->value.b.not) {
        // Apply filter to frame and clone old closure
        rightFrameNode *rightFrame = NULL;
        CHECK_RESULT(getLastRightFrame(state,
                                       currentNode->value.b.index,
                                       frameHash,
                                       &rightFrame));
        while (rightFrame) {
            messageNode *rightMessage = MESSAGE_NODE(state, rightFrame->messageOffset); 
            unsigned char match = 0;
            CHECK_RESULT(isBetaMatch(tree,
                                     state,
                                     &currentNode->value.b,
                                     &rightMessage->jo,
                                     currentFrame,
                                     &match));
            if (match) {
                if (!currentNode->value.b.distinct || isDistinct(currentFrame, rightFrame->messageOffset)) {
                    return RULES_OK;
                }
            }
            
            unsigned int rightFrameOffset = rightFrame->prevOffset;
            rightFrame = NULL;
            while (rightFrameOffset != UNDEFINED_HASH_OFFSET && !rightFrame) {
                rightFrame = RIGHT_FRAME_NODE(state,
                                              currentNode->value.b.index, 
                                              rightFrameOffset); 
                if (rightFrame->hash != frameHash) {
                    rightFrameOffset = rightFrame->prevOffset;
                    rightFrame = NULL;
                } 
            }
        }

        return handleBetaFrame(tree,
                               state, 
                               UNDEFINED_HASH_OFFSET,
                               currentNode,
                               nextNode,
                               NULL,
                               currentFrame);

    } else if (currentNode->type == NODE_BETA) {
        // Find all messages for frame 
        rightFrameNode *rightFrame;
        CHECK_RESULT(getLastRightFrame(state,
                                       currentNode->value.b.index,
                                       frameHash,
                                       &rightFrame));

        
        while (rightFrame) {
            messageNode *rightMessage = MESSAGE_NODE(state, rightFrame->messageOffset); 
            unsigned char match = 0;
            CHECK_RESULT(isBetaMatch(tree,
                                     state,
                                     &currentNode->value.b,
                                     &rightMessage->jo,
                                     currentFrame,
                                     &match));

            if (match && (!currentNode->value.b.distinct || isDistinct(currentFrame, rightFrame->messageOffset))) {
                CHECK_RESULT(handleBetaFrame(tree,
                                             state, 
                                             rightFrame->messageOffset,
                                             currentNode,
                                             nextNode,
                                             NULL,
                                             currentFrame));
            }
            
            unsigned int rightFrameOffset = rightFrame->prevOffset;
            rightFrame = NULL;
            while (rightFrameOffset != UNDEFINED_HASH_OFFSET && !rightFrame) {
                rightFrame = RIGHT_FRAME_NODE(state,
                                              currentNode->value.b.index, 
                                              rightFrameOffset); 
                if (rightFrame->hash != frameHash) {
                    rightFrameOffset = rightFrame->prevOffset;
                    rightFrame = NULL;
                } 
            }
        }   
    } else {
        // Get all other frames 
        leftFrameNode *connectorFrame;
        unsigned int connectorFrameOffset;
        unsigned int otherFrameType = (frameType == B_FRAME) ? A_FRAME : B_FRAME;
        CHECK_RESULT(getLastConnectorFrame(state,
                                           otherFrameType,
                                           currentNode->value.b.index,
                                           &connectorFrameOffset,
                                           &connectorFrame));

        while (connectorFrame) {
            CHECK_RESULT(handleBetaFrame(tree,
                                         state, 
                                         UNDEFINED_HASH_OFFSET,
                                         currentNode,
                                         nextNode,
                                         connectorFrame,
                                         currentFrame));

            connectorFrameOffset = connectorFrame->prevOffset;
            if (connectorFrameOffset == UNDEFINED_HASH_OFFSET) {
                connectorFrame = NULL;
            } else if (otherFrameType == A_FRAME) {
                connectorFrame = A_FRAME_NODE(state,
                                              currentNode->value.b.index, 
                                              connectorFrameOffset); 
            } else {
                connectorFrame = B_FRAME_NODE(state,
                                              currentNode->value.b.index, 
                                              connectorFrameOffset);
            }
        }
    }

    if (currentNode->value.b.gateType == GATE_OR || currentNode->value.b.isFirst) {
        CHECK_RESULT(handleBetaFrame(tree,
                                     state, 
                                     UNDEFINED_HASH_OFFSET,
                                     currentNode,
                                     nextNode,
                                     NULL,
                                     currentFrame));
    } 

    return RULES_OK;
}

static unsigned int handleDeleteMessage(ruleset *tree,
                                        stateNode *state,
                                        unsigned int messageOffset) {
    unsigned int result;
    unsigned int count = 0;
    unsigned int i = 1;
    messageNode *message = MESSAGE_NODE(state, messageOffset);
    if (!message->isActive) {
        return RULES_OK;
    }

    while (count < message->locationPool.count) {
        locationNode *currentLocationNode = LOCATION_NODE(message, i++);
        if (currentLocationNode->isActive) {
            ++count;
            leftFrameNode *frame = NULL;
            switch(currentLocationNode->location.frameType) {
                case A_FRAME:
                    frame = A_FRAME_NODE(state, 
                                         currentLocationNode->location.nodeIndex,
                                         currentLocationNode->location.frameOffset);

                case B_FRAME:
                    frame = B_FRAME_NODE(state, 
                                         currentLocationNode->location.nodeIndex,
                                         currentLocationNode->location.frameOffset);
                    
                    result = deleteConnectorFrame(state, 
                                                  currentLocationNode->location.frameType, 
                                                  currentLocationNode->location);
                    break;
                case LEFT_FRAME:
                    frame = LEFT_FRAME_NODE(state, 
                                            currentLocationNode->location.nodeIndex,
                                            currentLocationNode->location.frameOffset);
                    
                    result = deleteLeftFrame(state, 
                                             currentLocationNode->location);
                    break;
                case ACTION_FRAME:
                    frame = ACTION_FRAME_NODE(state, 
                                              currentLocationNode->location.nodeIndex,
                                              currentLocationNode->location.frameOffset);

                    result = deleteActionFrame(state, 
                                               currentLocationNode->location);
                    break;
                case RIGHT_FRAME:
                    result = deleteRightFrame(state, 
                                              currentLocationNode->location);
                    break;
            }

            if (result != RULES_OK && result != ERR_NODE_DISPATCHING && result != ERR_NODE_DELETED) {
                return result;
            } else if (result == ERR_NODE_DISPATCHING) {
                CHECK_RESULT(deleteMessageFromFrame(messageOffset, frame));
            } else if (result == RULES_OK) {
                if (currentLocationNode->location.frameType != RIGHT_FRAME) {
                    for (unsigned int ii = 0; ii < frame->messageCount; ++ii) {
                        messageFrame *currentFrame = &frame->messages[frame->reverseIndex[ii]]; 
                        if (currentFrame->messageNodeOffset != messageOffset) {
                            CHECK_RESULT(deleteFrameLocation(state,
                                                             currentFrame->messageNodeOffset,
                                                             currentLocationNode->location));
                        }
                    }                  
                } else {
                    node *currentNode = state->betaState[currentLocationNode->location.nodeIndex].reteNode;
                    if (currentNode->value.b.not) {
                        if (currentNode->value.b.isFirst) {
                            if (!state->betaState[currentLocationNode->location.nodeIndex].rightFramePool.count) {
                                node *nextNode = &tree->nodePool[currentNode->value.b.nextOffset];
                                CHECK_RESULT(handleBetaFrame(tree,
                                                             state, 
                                                             UNDEFINED_HASH_OFFSET,
                                                             currentNode,
                                                             nextNode,
                                                             NULL,
                                                             NULL));
                            }
                        } else {
                            unsigned int messageHash;
                            CHECK_RESULT(getFrameHash(tree,
                                                      state,
                                                      &currentNode->value.b,
                                                      &message->jo,
                                                      NULL,
                                                      &messageHash));

                            node *nextNode = &tree->nodePool[currentNode->value.b.nextOffset];

                            // Find all frames for message
                            leftFrameNode *currentFrame = NULL;
                            CHECK_RESULT(getLastLeftFrame(state,
                                                          currentNode->value.b.index,
                                                          messageHash,
                                                          NULL,
                                                          &currentFrame));
                            while (currentFrame) {
                                unsigned char match = 0;
                                CHECK_RESULT(isBetaMatch(tree,
                                                         state,
                                                         &currentNode->value.b,
                                                         &message->jo,
                                                         currentFrame,
                                                         &match));

                                if (match && (!currentNode->value.b.distinct || isDistinct(currentFrame, messageOffset))) {
                                    CHECK_RESULT(handleBetaFrame(tree,
                                                                 state, 
                                                                 UNDEFINED_HASH_OFFSET,
                                                                 currentNode,
                                                                 nextNode,
                                                                 NULL,
                                                                 currentFrame));
                                }

                                unsigned int currentFrameOffset = currentFrame->prevOffset;
                                currentFrame = NULL;
                                while (currentFrameOffset != UNDEFINED_HASH_OFFSET) {
                                    currentFrame = LEFT_FRAME_NODE(state, 
                                                                   currentNode->value.b.index, 
                                                                   currentFrameOffset);
                                    if (currentFrame->hash != messageHash) {
                                        currentFrameOffset = currentFrame->prevOffset;
                                        currentFrame = NULL;
                                    } else {
                                        currentFrameOffset = UNDEFINED_HASH_OFFSET;
                                    }   
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    result = deleteMessage(state, messageOffset);
    if (result == ERR_NODE_DELETED) {
        return RULES_OK;
    } else if (result != RULES_OK) {
        return result;
    }

    return RULES_OK;
}

static unsigned char isSubset(leftFrameNode *currentFrame, 
                              leftFrameNode *targetFrame) {

    for (unsigned int i = 0; i < currentFrame->messageCount; ++i) {
        unsigned char found = 0;
        for (unsigned int ii = 0; ii < targetFrame->messageCount; ++ii) {
            if (currentFrame->messages[currentFrame->reverseIndex[i]].messageNodeOffset == 
                targetFrame->messages[targetFrame->reverseIndex[ii]].messageNodeOffset) {
                found = 1;
                break;
            }
        }

        if (!found) {
            return 0;
        }
    }

    return 1;
}

static unsigned int deleteDownstreamFrames(ruleset *tree, 
                                           stateNode *state,
                                           node *currentNode,
                                           leftFrameNode *currentFrame) {

    messageNode *currentMessage = NULL;

    // Find message with the smallest location count
    for (unsigned int i = 0; i < currentFrame->messageCount; ++i) {
        if (currentFrame->messages[i].messageNodeOffset != UNDEFINED_HASH_OFFSET) {
            messageNode *message = MESSAGE_NODE(state, currentFrame->messages[i].messageNodeOffset);
            if (!currentMessage ||
                message->locationPool.count < currentMessage->locationPool.count) {
                currentMessage = message;
            }
        }
    }

    if (currentMessage) {
        unsigned int i = 1;
        unsigned int count = 0;
        while (count < currentMessage->locationPool.count) {
            locationNode *candidateLocationNode = LOCATION_NODE(currentMessage, i++);
            if (candidateLocationNode->isActive) {
                ++count;

                leftFrameNode *candidateFrame = NULL;
                if (candidateLocationNode->location.frameType == LEFT_FRAME &&
                    candidateLocationNode->location.nodeIndex > currentNode->value.b.index) {
                    candidateFrame = LEFT_FRAME_NODE(state, 
                                                     candidateLocationNode->location.nodeIndex, 
                                                     candidateLocationNode->location.frameOffset);
                } else if (candidateLocationNode->location.frameType == ACTION_FRAME) {
                    candidateFrame = ACTION_FRAME_NODE(state, 
                                                       candidateLocationNode->location.nodeIndex, 
                                                       candidateLocationNode->location.frameOffset);
                }

                if (candidateFrame && isSubset(currentFrame, candidateFrame)) {

                    unsigned int result;
                    if (candidateLocationNode->location.frameType == LEFT_FRAME) {
                        result = deleteLeftFrame(state, 
                                                 candidateLocationNode->location);
                    } else {
                        result = deleteActionFrame(state, 
                                                   candidateLocationNode->location);   
                    }

                    if (result != ERR_NODE_DELETED && result != ERR_NODE_DISPATCHING && result != RULES_OK) {
                        return result;
                    }
                }
            }
        }
    }

    return RULES_OK;
}

static unsigned int handleFilterFrames(ruleset *tree, 
                                       stateNode *state,
                                       node *currentNode,
                                       jsonObject *messageObject,
                                       unsigned int currentMessageOffset,
                                       unsigned int messageHash) { 

    leftFrameNode *currentFrame = NULL;
    frameLocation location;
    CHECK_RESULT(getLastLeftFrame(state,
                                  currentNode->value.b.index,
                                  messageHash,
                                  &location,
                                  &currentFrame));

    while (currentFrame) {

        unsigned char match = 0;
        CHECK_RESULT(isBetaMatch(tree,
                                 state,
                                 &currentNode->value.b,
                                 messageObject,
                                 currentFrame,
                                 &match));

        if (match) {
            if (!currentNode->value.b.distinct || isDistinct(currentFrame, currentMessageOffset)) {
                CHECK_RESULT(deleteDownstreamFrames(tree,
                                                    state,
                                                    currentNode,
                                                    currentFrame));
            }
        } 

        location.frameOffset = currentFrame->prevOffset;
        currentFrame = NULL;
        while (location.frameOffset != UNDEFINED_HASH_OFFSET) {
            currentFrame = LEFT_FRAME_NODE(state, 
                                           location.nodeIndex, 
                                           location.frameOffset);
            
            if (currentFrame->hash == messageHash) {
                break;
            } else {
                location.frameOffset = currentFrame->prevOffset;
                currentFrame = NULL;
            } 
        }
    }   

    return RULES_OK;
}

static unsigned int handleMatchFrames(ruleset *tree, 
                                      stateNode *state,
                                      node *currentNode,
                                      jsonObject *messageObject,
                                      unsigned int currentMessageOffset,
                                      unsigned int messageHash) { 
    // Find frames for message
    leftFrameNode *currentFrame = NULL;
    node *nextNode = &tree->nodePool[currentNode->value.b.nextOffset];
    CHECK_RESULT(getLastLeftFrame(state,
                                  currentNode->value.b.index,
                                  messageHash,
                                  NULL,
                                  &currentFrame));

    while (currentFrame) {
        unsigned char match = 0;
        CHECK_RESULT(isBetaMatch(tree,
                                 state,
                                 &currentNode->value.b,
                                 messageObject,
                                 currentFrame,
                                 &match));

        if (match && (!currentNode->value.b.distinct || isDistinct(currentFrame, currentMessageOffset))) {
            CHECK_RESULT(handleBetaFrame(tree,
                                         state, 
                                         currentMessageOffset,
                                         currentNode,
                                         nextNode,
                                         NULL,
                                         currentFrame));
        }

        unsigned int currentFrameOffset = currentFrame->prevOffset;
        currentFrame = NULL;
        while (currentFrameOffset != UNDEFINED_HASH_OFFSET) {
            currentFrame = LEFT_FRAME_NODE(state, 
                                           currentNode->value.b.index, 
                                           currentFrameOffset);
            if (currentFrame->hash == messageHash) {
                break;
            } else {
                currentFrameOffset = currentFrame->prevOffset;
                currentFrame = NULL;
            }  
        }
    }

    if(currentNode->value.b.gateType == GATE_OR && !currentNode->value.b.expressionSequence.length) {
        return handleBetaFrame(tree,
                               state,
                               currentMessageOffset,
                               currentNode, 
                               nextNode,
                               NULL,
                               NULL);   
    }

    return RULES_OK;
}

static unsigned int handleBetaMessage(ruleset *tree, 
                                      stateNode *state,
                                      node *betaNode,
                                      jsonObject *messageObject,
                                      unsigned int currentMessageOffset) { 
    if (betaNode->value.b.isFirst && !betaNode->value.b.not) {
        node *nextNode = &tree->nodePool[betaNode->value.b.nextOffset];
        return handleBetaFrame(tree,
                               state,
                               currentMessageOffset,
                               betaNode, 
                               nextNode,
                               NULL,
                               NULL);
    }

    frameLocation rightFrameLocation;
    rightFrameNode *rightFrame;

    CHECK_RESULT(createRightFrame(state,
                                  betaNode,
                                  &rightFrame,
                                  &rightFrameLocation));

    unsigned int messageHash;
    CHECK_RESULT(getFrameHash(tree,
                              state,
                              &betaNode->value.b,
                              messageObject,
                              NULL,
                              &messageHash));

    rightFrame->messageOffset = currentMessageOffset;
    CHECK_RESULT(addFrameLocation(state,
                                  rightFrameLocation,
                                  currentMessageOffset));

    CHECK_RESULT(setRightFrame(state,
                               messageHash,
                               rightFrameLocation)); 

    if (betaNode->value.b.not) {
        return handleFilterFrames(tree,
                                  state,
                                  betaNode,
                                  messageObject,
                                  currentMessageOffset,
                                  messageHash);
    } 

    return handleMatchFrames(tree,
                             state,
                             betaNode,
                             messageObject,
                             currentMessageOffset,
                             messageHash);
}


static unsigned int handleAplhaArray(ruleset *tree,
                                     jsonObject *messageObject,
                                     jsonProperty *currentProperty,
                                     alpha *arrayAlpha,
                                     unsigned char *propertyMatch) {
    unsigned int result = RULES_OK;
    if (currentProperty->type != JSON_ARRAY) {
        return RULES_OK;
    }

    char *first = messageObject->content + currentProperty->valueOffset;
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
            jo.content = first;
            jo.idIndex = UNDEFINED_INDEX;
            jo.sidIndex = UNDEFINED_INDEX;
            memset(jo.propertyIndex, 0, MAX_OBJECT_PROPERTIES * sizeof(unsigned short));

            CHECK_RESULT(constructObject(first,
                                         "$i", 
                                         NULL, 
                                         0,
                                         &jo, 
                                         &next));
        } else {
            memset(jo.propertyIndex, 0, MAX_OBJECT_PROPERTIES * sizeof(unsigned short));
            jo.propertiesLength = 0;
            jo.content = first;
            CHECK_RESULT(setObjectProperty(&jo, 
                                           HASH_I, 
                                           type, 
                                           0, 
                                           last - first + 1));
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
                        if (listNode->value.a.expression.left.value.id.propertyNameHash == jo.properties[propertyIndex].hash) {
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
                        if (currentProperty->hash == hashNode->value.a.expression.left.value.id.propertyNameHash) {
                            unsigned char match = 0;
                            if (hashNode->value.a.expression.operator == OP_IALL || hashNode->value.a.expression.operator == OP_IANY) {
                                // handleAplhaArray finds a valid path, thus use propertyMatch
                                CHECK_RESULT(handleAplhaArray(tree, 
                                                              &jo,
                                                              currentProperty, 
                                                              &hashNode->value.a,
                                                              propertyMatch));
                            } else {
                                CHECK_RESULT(isAlphaMatch(tree,
                                                          &hashNode->value.a,
                                                          &jo,
                                                          &match));

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

            // no next offset and no nextListOffest means we found a valid path
            if (!currentAlpha->nextOffset && !currentAlpha->nextListOffset) {
                *propertyMatch = 1;
                break;
            } 
        }
        
        // OP_IANY, one element led a a valid path
        // OP_IALL, all elements led to a valid path
        if ((arrayAlpha->expression.operator == OP_IALL && !*propertyMatch) ||
            (arrayAlpha->expression.operator == OP_IANY && *propertyMatch)) {
            break;
        } 

        result = readNextArrayValue(last, &first, &last, &type);   
    }

    return (result == PARSE_END ? RULES_OK: result);
}

static unsigned int handleAlpha(ruleset *tree,
                                stateNode *state,
                                char *mid,
                                jsonObject *jo,
                                unsigned int currentMessageOffset,
                                alpha *alphaNode) { 
    unsigned short top = 1;
    unsigned int entry;
    alpha *stack[MAX_STACK_SIZE];
    stack[0] = alphaNode;
    alpha *currentAlpha;
    unsigned int result = ERR_EVENT_NOT_HANDLED;

    while (top) {
        --top;
        currentAlpha = stack[top];
        // add all disjunctive nodes to stack
        if (currentAlpha->nextListOffset) {
            unsigned int *nextList = &tree->nextPool[currentAlpha->nextListOffset];
            for (entry = 0; nextList[entry] != 0; ++entry) {
                node *listNode = &tree->nodePool[nextList[entry]];
                jsonProperty *property;
                unsigned int aresult = getObjectProperty(jo, listNode->value.a.expression.left.value.id.propertyNameHash, &property);
                if (aresult == ERR_PROPERTY_NOT_FOUND) {
                    // filter out not exists (OP_NEX)
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
                    if (currentProperty->hash == hashNode->value.a.expression.left.value.id.propertyNameHash) {
                        if (hashNode->value.a.expression.right.type == JSON_IDENTIFIER || hashNode->value.a.expression.right.type == JSON_EXPRESSION) {
                            if (top == MAX_STACK_SIZE) {
                                return ERR_MAX_STACK_SIZE;
                            }
                            stack[top] = &hashNode->value.a; 
                            ++top;
                        } else {
                            unsigned char match = 0;
                            if (hashNode->value.a.expression.operator == OP_IALL || hashNode->value.a.expression.operator == OP_IANY) {
                                CHECK_RESULT(handleAplhaArray(tree,
                                                              jo,
                                                              currentProperty, 
                                                              &hashNode->value.a,
                                                              &match));
                            } else {
                                CHECK_RESULT(isAlphaMatch(tree, 
                                                          &hashNode->value.a,
                                                          jo, 
                                                          &match));
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
            result = RULES_OK;
            unsigned int *betaList = &tree->nextPool[currentAlpha->betaListOffset];
            for (unsigned int entry = 0; betaList[entry] != 0; ++entry) {
                CHECK_RESULT(handleBetaMessage(tree, 
                                               state, 
                                               &tree->nodePool[betaList[entry]],
                                               jo,
                                               currentMessageOffset));
            }
        }
    }

    return result;
}

static unsigned int handleMessageCore(ruleset *tree,
                                      jsonObject *jo, 
                                      unsigned char actionType,
                                      char **commands,
                                      unsigned int *commandCount,
                                      unsigned int *messageOffset,
                                      unsigned int *stateOffset) {
    stateNode *sidState;
    jsonProperty *sidProperty = &jo->properties[jo->sidIndex];
    jsonProperty *midProperty = &jo->properties[jo->idIndex];

#ifdef _WIN32
    char *sid = (char *)_alloca(sizeof(char)*(sidProperty->valueLength + 1));
#else
    char sid[sidProperty->valueLength + 1];
#endif  
    if (sidProperty->valueOffset) {
        strncpy(sid, jo->content + sidProperty->valueOffset, sidProperty->valueLength);
    } else {
        strncpy(sid, jo->sidBuffer, sidProperty->valueLength);
    }

    sid[sidProperty->valueLength] = '\0';

    #ifdef _WIN32
    char *mid = (char *)_alloca(sizeof(char)*(midProperty->valueLength + 1));
#else
    char mid[midProperty->valueLength + 1];
#endif
    if (midProperty->valueOffset) {
        strncpy(mid, jo->content + midProperty->valueOffset, midProperty->valueLength);
    } else {
        strncpy(mid, jo->idBuffer, midProperty->valueLength);
    }

    mid[midProperty->valueLength] = '\0';
    unsigned char isNewState;
    CHECK_RESULT(ensureStateNode(tree, 
                                 sid,
                                 &isNewState, 
                                 &sidState));

    *stateOffset = sidState->offset;
    if (actionType == ACTION_UPDATE_STATE) {
        if (sidState->factOffset != UNDEFINED_HASH_OFFSET) {
            CHECK_RESULT(handleDeleteMessage(tree,
                                             sidState,
                                             sidState->factOffset));

        }

        CHECK_RESULT(storeMessage(sidState,
                                  mid,
                                  jo,
                                  MESSAGE_TYPE_FACT,
                                  messageOffset));


        sidState->factOffset = *messageOffset;
        CHECK_RESULT(handleAlpha(tree,
                                 sidState,
                                 mid,
                                 jo,
                                 sidState->factOffset,
                                 &tree->nodePool[NODE_M_OFFSET].value.a));

    } else if (actionType == ACTION_RETRACT_FACT || actionType == ACTION_RETRACT_EVENT) {
        CHECK_RESULT(getMessage(sidState,
                                mid,
                                messageOffset));

        CHECK_RESULT(handleDeleteMessage(tree,
                                         sidState,
                                         *messageOffset));

    } else {
        CHECK_RESULT(storeMessage(sidState,
                                  mid,
                                  jo,
                                  (actionType == ACTION_ASSERT_FACT ? MESSAGE_TYPE_FACT : MESSAGE_TYPE_EVENT),
                                  messageOffset));

        unsigned int result = handleAlpha(tree,
                                          sidState,
                                          mid,
                                          jo,
                                          *messageOffset,
                                          &tree->nodePool[NODE_M_OFFSET].value.a);

        if (result == RULES_OK || result == ERR_EVENT_NOT_HANDLED) {
            if (isNewState) {      
    #ifdef _WIN32
                char *stateMessage = (char *)_alloca(sizeof(char)*(50 + sidProperty->valueLength * 2));
                sprintf_s(stateMessage, sizeof(char)*(50 + sidProperty->valueLength * 2), "{ \"sid\":\"%s\", \"id\":\"sid-%s\", \"$s\":1}", sid, sid);
    #else
                char stateMessage[50 + sidProperty->valueLength * 2];
                snprintf(stateMessage, sizeof(char)*(50 + sidProperty->valueLength * 2), "{ \"sid\":\"%s\", \"id\":\"sid-%s\", \"$s\":1}", sid, sid);
    #endif
                
                unsigned int stateMessageOffset;
                unsigned int stateResult = handleMessage(tree,
                                                         stateMessage,  
                                                         ACTION_ASSERT_FACT,
                                                         commands,
                                                         commandCount,
                                                         &stateMessageOffset,
                                                         stateOffset);
                if (stateResult != RULES_OK && stateResult != ERR_EVENT_NOT_HANDLED) {
                    return stateResult;
                }

                sidState->factOffset = stateMessageOffset;

            }
        }

        return result;
    } 

    return RULES_OK;

}

static unsigned int handleMessage(ruleset *tree,
                                  char *message, 
                                  unsigned char actionType, 
                                  char **commands,
                                  unsigned int *commandCount,
                                  unsigned int *messageOffset,
                                  unsigned int *stateOffset) {
    char *next;
    jsonObject jo;
    CHECK_RESULT(constructObject(message,
                                 NULL, 
                                 NULL, 
                                 1,
                                 &jo, 
                                 &next));
    
    return handleMessageCore(tree, 
                             &jo,
                             actionType,
                             commands,
                             commandCount,
                             messageOffset,
                             stateOffset);
}

static unsigned int handleMessages(ruleset *tree, 
                                   unsigned char actionType,
                                   char *messages, 
                                   char **commands,
                                   unsigned int *commandCount,
                                   unsigned int *stateOffset) {
    unsigned int result;
    unsigned int returnResult = RULES_OK;
    unsigned int messageOffset;
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
                           1,
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
                                   &jo, 
                                   actionType, 
                                   commands,
                                   commandCount,
                                   &messageOffset,
                                   stateOffset);
        
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

static unsigned int handleTimers(ruleset *tree, 
                                 char **commands,
                                 unsigned int *commandCount,
                                 unsigned int *stateOffset) {
    return RULES_OK;
}

static unsigned int startHandleMessage(ruleset *tree, 
                                       char *message, 
                                       unsigned char actionType,
                                       unsigned int *stateOffset,
                                       unsigned int *replyCount) {
    char *commands[MAX_COMMAND_COUNT];
    unsigned int commandCount = 0;
    unsigned int messageOffset;
    unsigned int result = handleMessage(tree, 
                                        message, 
                                        actionType, 
                                        commands,
                                        &commandCount,
                                        &messageOffset,
                                        stateOffset);
    if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
        freeCommands(commands, commandCount);
        return result;
    }

    unsigned int batchResult = startNonBlockingBatch(NULL, commands, commandCount, replyCount);
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
    unsigned int stateOffset;
    unsigned int messageOffset;
    unsigned int result = handleMessage(tree, 
                                        message, 
                                        actionType, 
                                        commands,
                                        &commandCount,
                                        &messageOffset,
                                        &stateOffset);
    if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
        freeCommands(commands, commandCount);
        return result;
    }

    unsigned int batchResult = executeBatch(NULL, commands, commandCount);
    if (batchResult != RULES_OK) {
        return batchResult;
    }

    return result;
}

static unsigned int startHandleMessages(ruleset *tree, 
                                        char *messages, 
                                        unsigned char actionType,
                                        unsigned int *stateOffset,
                                        unsigned int *replyCount) {
    char *commands[MAX_COMMAND_COUNT];
    unsigned int commandCount = 0;
    unsigned int result = handleMessages(tree,
                                         actionType,
                                         messages,
                                         commands,
                                         &commandCount,
                                         stateOffset);
    if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
        freeCommands(commands, commandCount);
        return result;
    }

    unsigned int batchResult = startNonBlockingBatch(NULL, commands, commandCount, replyCount);
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
    unsigned int stateOffset;
    unsigned int result = handleMessages(tree,
                                         actionType,
                                         messages,
                                         commands,
                                         &commandCount,
                                         &stateOffset);
    if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
        freeCommands(commands, commandCount);
        return result;
    }

    unsigned int batchResult = executeBatch(NULL, commands, commandCount);
    if (batchResult != RULES_OK) {
        return batchResult;
    }

    return result;
}

unsigned int complete(unsigned int stateOffset, unsigned int replyCount) {
    unsigned int result = completeNonBlockingBatch(NULL, replyCount);
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
                             unsigned int *stateOffset, 
                             unsigned int *replyCount) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    return startHandleMessage(tree, message, ACTION_ASSERT_EVENT, stateOffset, replyCount);
}

unsigned int assertEvents(unsigned int handle, 
                          char *messages) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    return executeHandleMessages(tree, messages, ACTION_ASSERT_EVENT);
}

unsigned int startAssertEvents(unsigned int handle, 
                              char *messages, 
                              unsigned int *stateOffset, 
                              unsigned int *replyCount) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    return startHandleMessages(tree, messages, ACTION_ASSERT_EVENT, stateOffset, replyCount);
}

unsigned int retractEvent(unsigned int handle, char *message) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    return executeHandleMessage(tree, message, ACTION_RETRACT_EVENT);
}

unsigned int startAssertFact(unsigned int handle, 
                             char *message, 
                             unsigned int *stateOffset, 
                             unsigned int *replyCount) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    return startHandleMessage(tree, message, ACTION_ASSERT_FACT, stateOffset, replyCount);
}

unsigned int assertFact(unsigned int handle, char *message) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    return executeHandleMessage(tree, message, ACTION_ASSERT_FACT);
}

unsigned int startAssertFacts(unsigned int handle, 
                              char *messages, 
                              unsigned int *stateOffset, 
                              unsigned int *replyCount) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    return startHandleMessages(tree, messages, ACTION_ASSERT_FACT, stateOffset, replyCount);
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

    return executeHandleMessage(tree, message, ACTION_RETRACT_FACT);
}

unsigned int startRetractFact(unsigned int handle, 
                             char *message, 
                             unsigned int *stateOffset, 
                             unsigned int *replyCount) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    return startHandleMessage(tree, message, ACTION_RETRACT_FACT, stateOffset, replyCount);
}

unsigned int retractFacts(unsigned int handle, 
                          char *messages) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    return executeHandleMessages(tree, messages, ACTION_RETRACT_FACT);
}

unsigned int startRetractFacts(unsigned int handle, 
                              char *messages, 
                              unsigned int *stateOffset, 
                              unsigned int *replyCount) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    return startHandleMessages(tree, messages, ACTION_RETRACT_FACT, stateOffset, replyCount);
}

unsigned int updateState(unsigned int handle, char *sid, char *state) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    char *commands[MAX_COMMAND_COUNT];
    unsigned int commandCount = 0;
    unsigned int stateOffset;
    unsigned int messageOffset;
    unsigned int result = handleMessage(tree, 
                                        state, 
                                        ACTION_UPDATE_STATE,
                                        commands,
                                        &commandCount,
                                        &messageOffset,
                                        &stateOffset);
    if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
        freeCommands(commands, commandCount);
        return result;
    }

    unsigned int batchResult = executeBatch(NULL, commands, commandCount);
    if (batchResult != RULES_OK) {
        return batchResult;
    }

    return result;
}

unsigned int startUpdateState(unsigned int handle, 
                              char *state,
                              unsigned int *stateOffset,
                              unsigned int *replyCount) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    char *commands[MAX_COMMAND_COUNT];
    unsigned int result = RULES_OK;
    unsigned int commandCount = 0;
    unsigned int messageOffset;
    result = handleMessage(tree, 
                           state,
                           ACTION_UPDATE_STATE,
                           commands,
                           &commandCount,
                           &messageOffset,
                           stateOffset);
    if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
        freeCommands(commands, commandCount);
        return result;
    }

    result = startNonBlockingBatch(NULL, commands, commandCount, replyCount);

    //TODO: Durable store
    *replyCount = 1;
    return result;

}

unsigned int assertTimers(unsigned int handle) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    char *commands[MAX_COMMAND_COUNT];
    unsigned int commandCount = 0;
    unsigned int stateOffset;
    unsigned int result = handleTimers(tree, 
                                       commands,
                                       &commandCount,
                                       &stateOffset);
    if (result != RULES_OK) {
        freeCommands(commands, commandCount);
        return result;
    }

    result = executeBatch(NULL, commands, commandCount);
    if (result != RULES_OK && result != ERR_EVENT_OBSERVED) {
        return result;
    }

    return RULES_OK;
}

unsigned int startAction(unsigned int handle, 
                         char **stateFact, 
                         char **messages, 
                         unsigned int *stateOffset) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);
    stateNode *resultState;
    actionStateNode *resultAction;
    unsigned int actionStateIndex;
    unsigned int resultCount;
    unsigned int resultFrameOffset;

    CHECK_RESULT(getNextResult(tree,
                               &resultState, 
                               &actionStateIndex,
                               &resultCount,
                               &resultFrameOffset,
                               &resultAction));

    CHECK_RESULT(serializeResult(tree, 
                                 resultState, 
                                 resultAction, 
                                 resultCount,
                                 &resultState->context.messages));

    CHECK_RESULT(serializeState(resultState, 
                                &resultState->context.stateFact));

   
    resultState->context.actionStateIndex = actionStateIndex;
    resultState->context.resultCount = resultCount;
    resultState->context.resultFrameOffset = resultFrameOffset;
    *stateOffset = resultState->offset;
    *messages  = resultState->context.messages;
    *stateFact = resultState->context.stateFact;
    
    return RULES_OK;
}

static unsigned int deleteCurrentAction(ruleset *tree,
                                        stateNode *state,
                                        unsigned int actionStateIndex,
                                        unsigned int resultCount,
                                        unsigned int resultFrameOffset) {

    for (unsigned int currentCount = 0; currentCount < resultCount; ++currentCount) {
        leftFrameNode *resultFrame;
        frameLocation resultLocation; 
        resultLocation.frameType = ACTION_FRAME;
        resultLocation.nodeIndex = actionStateIndex;
        resultLocation.frameOffset = resultFrameOffset;   
        CHECK_RESULT(getActionFrame(state,
                                    resultLocation,
                                    &resultFrame));
        
        for (int i = 0; i < resultFrame->messageCount; ++i) {
            messageFrame *currentMessageFrame = &resultFrame->messages[resultFrame->reverseIndex[i]]; 
            messageNode *currentMessageNode = MESSAGE_NODE(state, 
                                                           currentMessageFrame->messageNodeOffset);


            if (currentMessageNode->messageType == MESSAGE_TYPE_EVENT) {
                CHECK_RESULT(handleDeleteMessage(tree,
                                                 state, 
                                                 currentMessageFrame->messageNodeOffset));
            }
        }

        resultFrameOffset = resultFrame->nextOffset;
        unsigned int result = deleteDispatchingActionFrame(state, resultLocation);
        if (result != RULES_OK) {
            return result;
        }
    }

    return RULES_OK;
}

static void freeActionContext(stateNode *resultState) {
    if (resultState->context.messages) {
        free(resultState->context.messages);
        resultState->context.messages = NULL;
    }

    if (resultState->context.stateFact) {
        free(resultState->context.stateFact);
        resultState->context.stateFact = NULL;
    }
}

unsigned int completeAndStartAction(unsigned int handle, 
                                    unsigned int expectedReplies,
                                    unsigned int stateOffset, 
                                    char **messages) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);
    stateNode *resultState = STATE_NODE(tree, stateOffset);
    
    CHECK_RESULT(deleteCurrentAction(tree,
                                     resultState, 
                                     resultState->context.actionStateIndex,
                                     resultState->context.resultCount,
                                     resultState->context.resultFrameOffset));

    
    freeActionContext(resultState);

    actionStateNode *resultAction;
    unsigned int actionStateIndex;
    unsigned int resultCount;
    unsigned int resultFrameOffset;

    CHECK_RESULT(getNextResultInState(tree,
                                      resultState,
                                      &actionStateIndex,
                                      &resultCount,
                                      &resultFrameOffset, 
                                      &resultAction));

    CHECK_RESULT(serializeResult(tree, 
                                 resultState, 
                                 resultAction, 
                                 resultCount,
                                 &resultState->context.messages));

    resultState->context.actionStateIndex = actionStateIndex;
    resultState->context.resultCount = resultCount;
    resultState->context.resultFrameOffset = resultFrameOffset;
    *messages  = resultState->context.messages;
    
    return RULES_OK;
}

unsigned int abandonAction(unsigned int handle, unsigned int stateOffset) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);
    stateNode *resultState = STATE_NODE(tree, stateOffset);
    
    freeActionContext(resultState);
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

