
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
    stateNode *resultState;
    actionStateNode *resultAction;
    char *messages;
    char *stateFact;
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
                                  char *message, 
                                  unsigned char actionType, 
                                  char **commands,
                                  unsigned int *commandCount,
                                  unsigned int *messageOffset,
                                  void **rulesBinding);

static unsigned int reduceExpression(ruleset *tree,
                                     stateNode *state, 
                                     expression *currentExpression,
                                     jsonObject *messageObject,
                                     messageFrame *messageContext,
                                     jsonProperty *targetProperty);

static unsigned int reduceString(char *left, 
                                 unsigned short leftLength, 
                                 char *right, 
                                 unsigned short rightLength,
                                 unsigned char op,
                                 jsonProperty *targetProperty) {
    char rightTemp = right[rightLength];
    right[rightLength] = '\0';        
    char leftTemp = left[leftLength];
    left[leftLength] = '\0';
    int result = strcmp(left, right);
    right[rightLength] = rightTemp;
    left[leftLength] = leftTemp;
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
                                  messageFrame *messageContext,
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
                                        messageContext,
                                        *targetProperty);
            }
        case JSON_IDENTIFIER:
            CHECK_RESULT(getMessageFromFrame(state,
                                             messageContext, 
                                             sourceOperand->value.id.nameHash, 
                                             &messageObject));
            // break omitted intentionally, continue to next case
        case JSON_MESSAGE_IDENTIFIER:
            return getObjectProperty(messageObject,
                                     sourceOperand->value.id.propertyNameHash,
                                     targetProperty);
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
                char *leftString = (char *)_alloca(sizeof(char)*(leftProperty->valueLength + 1));
                sprintf_s(leftString, sizeof(char)*(leftProperty->valueLength + 1), "%ld", leftProperty->value.i);
#else
                char leftString[leftProperty->valueLength + 1];
                snprintf(leftString, sizeof(char)*(leftProperty->valueLength + 1), "%ld", leftProperty->value.i);
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
                char *leftString = (char *)_alloca(sizeof(char)*(leftProperty->valueLength + 1));
                sprintf_s(leftString, sizeof(char)*(leftProperty->valueLength + 1), "%f", leftProperty->value.d);
#else
                char leftString[leftProperty->valueLength + 1];
                snprintf(leftString, sizeof(char)*(leftProperty->valueLength + 1), "%f", leftProperty->value.d);
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
                char *rightString = (char *)_alloca(sizeof(char)*(rightProperty->valueLength + 1));
                sprintf_s(rightString, sizeof(char)*(rightProperty->valueLength + 1), "%ld", rightProperty->value.i);
#else
                char rightString[rightProperty->valueLength + 1];
                snprintf(rightString, sizeof(char)*(rightProperty->valueLength + 1), "%ld", rightProperty->value.i);
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
                char *rightString = (char *)_alloca(sizeof(char)*(rightProperty->valueLength + 1));
                sprintf_s(rightString, sizeof(char)*(rightProperty->valueLength + 1), "%f", rightProperty->value.d);
#else
                char rightString[rightProperty->valueLength + 1];
                snprintf(rightString, sizeof(char)*(rightProperty->valueLength + 1), "%f", rightProperty->value.d);
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
                                     messageFrame *messageContext,
                                     jsonProperty *targetProperty) {
    jsonProperty leftValue;
    jsonProperty *leftProperty = &leftValue;
    CHECK_RESULT(reduceOperand(tree,
                               state,
                               &currentExpression->left,
                               messageObject,
                               messageContext,
                               &leftProperty));
    
    if (currentExpression->right.type == JSON_REGEX || currentExpression->right.type == JSON_REGEX) {
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
                               messageContext,
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
                                             messageFrame *messageContext,
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
                                                      messageContext,
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
                                                      messageContext,
                                                      i,
                                                      targetProperty));
            } else if (currentExpression->operator == OP_IALL || currentExpression->operator == OP_IANY) {
                
            } else {
                CHECK_RESULT(reduceExpression(tree,
                                              state,
                                              currentExpression,
                                              messageObject,
                                              messageContext,
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

static unsigned int isBetaMatch(ruleset *tree,
                                stateNode *state, 
                                beta *currentBeta, 
                                jsonObject *messageObject,
                                messageFrame *messageContext,
                                unsigned char *propertyMatch) {
    jsonProperty resultProperty;
    unsigned short i = 0;
    CHECK_RESULT(reduceExpressionSequence(tree,
                                         state,
                                         &currentBeta->expressionSequence,
                                         OP_NOP,
                                         messageObject,
                                         messageContext,
                                         &i,
                                         &resultProperty));

    if (resultProperty.type != JSON_BOOL) {
        return ERR_OPERATION_NOT_SUPPORTED;
    }

    *propertyMatch = resultProperty.value.b;
    printf("isBetaMatch %d\n", *propertyMatch);
    return RULES_OK;
}

static unsigned int isAlphaMatch(ruleset *tree,
                                 alpha *currentAlpha,
                                 jsonObject *messageObject,
                                 unsigned char *propertyMatch) {
    *propertyMatch = 0;
    if (currentAlpha->expression.operator == OP_EX) {
        *propertyMatch = 1;
        return RULES_OK;
    }

    jsonProperty resultProperty;
    CHECK_RESULT(reduceExpression(tree,
                                  NULL,
                                  &currentAlpha->expression,
                                  messageObject,
                                  NULL,
                                  &resultProperty));

    if (resultProperty.type != JSON_BOOL) {
        return ERR_OPERATION_NOT_SUPPORTED;
    }

    *propertyMatch = resultProperty.value.b;
    printf("isAlphaMatch %d\n", *propertyMatch);
    return RULES_OK;
}

static void freeCommands(char **commands,
                         unsigned int commandCount) {
    for (unsigned int i = 0; i < commandCount; ++i) {
        free(commands[i]);
    }
}

static unsigned int handleAction(ruleset *tree, 
                                 stateNode *state, 
                                 node *node,
                                 leftFrameNode *frame) {
    printf("handle action %s\n", &tree->stringPool[node->nameOffset]);
    printf("frame\n");
    for (int i = 0; i < MAX_MESSAGE_FRAMES; ++i) {
        if (frame->messages[i].hash) {
            messageNode *node = MESSAGE_NODE(state, frame->messages[i].messageNodeOffset);
            printf("    -> %s\n", node->jo.content);
        }
    }
    return RULES_OK;
}

static unsigned char isDistinct(leftFrameNode *currentFrame, unsigned int currentMessageOffset) {
    for (unsigned short i = 0; i < currentFrame->messageCount; ++i) {
        if (currentFrame->messages[currentFrame->reverseIndex[i]].messageNodeOffset == currentMessageOffset) {
            return 0;
        }
    }

    return 1;
}

static unsigned int handleBeta(ruleset *tree, 
                               stateNode *state,
                               char *mid,
                               jsonObject *messageObject,
                               unsigned int currentMessageOffset,
                               node *betaNode) {    
    printf("handling beta %d message %d expressions %d\n", betaNode->value.b.index, currentMessageOffset, betaNode->value.b.expressionSequence.length);
    node *actionNode = NULL;
    node *currentNode = betaNode;
    leftFrameNode *currentFrame = NULL;
    frameLocation currentFrameLocation;

    while (currentNode != NULL) {
        if (currentNode->type == NODE_ACTION) {
            currentNode = NULL;
            actionNode = currentNode;
        } else {
            node *nextNode = &tree->nodePool[currentNode->value.b.nextOffset];
            if (!currentFrame) {
                if (!currentNode->value.b.expressionSequence.length) {
                    if (nextNode->type == NODE_ACTION) {
                        CHECK_RESULT(createActionFrame(state,
                                                       nextNode->value.c.index,
                                                       nextNode->nameOffset, 
                                                       NULL,
                                                       &currentFrame,
                                                       &currentFrameLocation));
                    } else {
                        CHECK_RESULT(createLeftFrame(state,
                                                     nextNode->value.b.index, 
                                                     nextNode->nameOffset,
                                                     NULL,
                                                     &currentFrame,
                                                     &currentFrameLocation));
                    }
                } else {
                    frameLocation rightFrameLocation;
                    rightFrameNode *rightFrame;
                    CHECK_RESULT(createRightFrame(state,
                                                  currentNode->value.b.index,
                                                  &rightFrame,
                                                  &rightFrameLocation));

                    //TODO get real message hash
                    unsigned int messageHash = fnv1Hash32("1", 1);
                    rightFrame->messageOffset = currentMessageOffset;
                    CHECK_RESULT(appendFrameLocation(state,
                                                     rightFrameLocation,
                                                     currentMessageOffset));

                    CHECK_RESULT(setRightFrame(state,
                                               messageHash,
                                               rightFrameLocation)); 

                    // Find frame for message
                    leftFrameNode *oldFrame = NULL;
                    CHECK_RESULT(getLeftFrame(state,
                                              currentNode->value.b.index,
                                              messageHash,
                                              &oldFrame));

                    unsigned char match = 0;
                    while (!match) {
                        CHECK_RESULT(isBetaMatch(tree,
                                                 state,
                                                 &currentNode->value.b,
                                                 messageObject,
                                                 oldFrame->messages,
                                                 &match));
                        if (match) {
                            if (currentNode->value.b.distinct && 
                                !isDistinct(oldFrame, currentMessageOffset)) {
                                match = 0;
                            }
                
                        }

                        if (!match) {
                            unsigned int oldFrameOffset = oldFrame->nextOffset;
                            if (oldFrameOffset == UNDEFINED_HASH_OFFSET) {
                                return ERR_FRAME_NOT_FOUND;
                            }

                            oldFrame = LEFT_FRAME_NODE(state, 
                                                       betaNode->value.b.index, 
                                                       oldFrameOffset);
                            if (oldFrame->hash != messageHash) {
                                return ERR_FRAME_NOT_FOUND;
                            }
                        }

                    }

                    if (nextNode->type == NODE_ACTION) {
                        CHECK_RESULT(createActionFrame(state,
                                                       nextNode->value.c.index,
                                                       nextNode->nameOffset,
                                                       oldFrame,
                                                       &currentFrame,
                                                       &currentFrameLocation));
                    } else {
                        CHECK_RESULT(createLeftFrame(state,
                                                    nextNode->value.b.index,
                                                    nextNode->nameOffset,
                                                    oldFrame,
                                                    &currentFrame,
                                                    &currentFrameLocation));
                    }
                    
                }

                CHECK_RESULT(setMessageInFrame(currentFrame,
                                               currentNode->nameOffset,
                                               currentNode->value.b.hash,
                                               currentMessageOffset));

                CHECK_RESULT(appendFrameLocation(state,
                                                 currentFrameLocation,
                                                 currentMessageOffset));

            } else {
                //TODO get real frame hash
                unsigned int frameHash = fnv1Hash32("1", 1);
                CHECK_RESULT(setLeftFrame(state,
                                          frameHash,
                                          currentFrameLocation));

                // Find message for frame
                rightFrameNode *rightFrame;
                CHECK_RESULT(getRightFrame(state,
                                           currentNode->value.b.index,
                                           frameHash,
                                           &rightFrame));

                messageNode *rightMessage = MESSAGE_NODE(state, rightFrame->messageOffset); 
                unsigned char match = 0;
                while (!match) {
                    CHECK_RESULT(isBetaMatch(tree,
                                             state,
                                             &currentNode->value.b,
                                             &rightMessage->jo,
                                             currentFrame->messages,
                                             &match));

                    if (match) {
                        if (currentNode->value.b.distinct && 
                            !isDistinct(currentFrame, currentMessageOffset)) {
                            match = 0;
                        }
            
                    }
                    
                    if (!match) {
                        unsigned int rightFrameOffset = rightFrame->nextOffset;
                        if (rightFrameOffset == UNDEFINED_HASH_OFFSET) {
                            return ERR_FRAME_NOT_FOUND;
                        }

                        rightFrame = RIGHT_FRAME_NODE(state,
                                                      currentNode->value.b.index, 
                                                      rightFrameOffset); 
                        if (rightFrame->hash != frameHash) {
                            return ERR_FRAME_NOT_FOUND;
                        }

                        rightMessage = MESSAGE_NODE(state, rightFrame->messageOffset);
                    }
                }                

                if (nextNode->type == NODE_ACTION) {
                    CHECK_RESULT(createActionFrame(state,
                                                   nextNode->value.c.index,
                                                   nextNode->nameOffset,
                                                   currentFrame,
                                                   &currentFrame,
                                                   &currentFrameLocation));
                } else {
                    CHECK_RESULT(createLeftFrame(state,
                                                 nextNode->value.b.index,
                                                 nextNode->nameOffset,
                                                 currentFrame,
                                                 &currentFrame,
                                                 &currentFrameLocation));
                }

                CHECK_RESULT(setMessageInFrame(currentFrame,
                                               currentNode->nameOffset,
                                               currentNode->value.b.hash,
                                               currentMessageOffset));

                CHECK_RESULT(appendFrameLocation(state,
                                                 currentFrameLocation,
                                                 currentMessageOffset));

            }

            currentNode = &tree->nodePool[currentNode->value.b.nextOffset];
        }
    }

    CHECK_RESULT(setActionFrame(state, currentFrameLocation));

    return handleAction(tree, 
                        state, 
                        actionNode,
                        currentFrame);
}


static unsigned int handleAplhaArray(ruleset *tree,
                                     jsonObject *messageObject,
                                     jsonProperty *currentProperty,
                                     alpha *arrayAlpha,
                                     unsigned char *propertyMatch) {
    printf("handle alpha array\n");  
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
                                           last - first));
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
                                                              messageObject,
                                                              currentProperty, 
                                                              &hashNode->value.a,
                                                              propertyMatch));
                            } else {
                                CHECK_RESULT(isAlphaMatch(tree,
                                                          &hashNode->value.a,
                                                          messageObject,
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
    printf("handle alpha\n");                       
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
            unsigned int *betaList = &tree->nextPool[currentAlpha->betaListOffset];
            for (unsigned int entry = 0; betaList[entry] != 0; ++entry) {
                unsigned int bresult = handleBeta(tree, 
                                                  state, 
                                                  mid,
                                                  jo,
                                                  currentMessageOffset,
                                                  &tree->nodePool[betaList[entry]]);
                if (bresult != RULES_OK && bresult != ERR_FRAME_NOT_FOUND) {
                    return bresult;
                }
            }
        }
    }

    return ERR_EVENT_NOT_HANDLED;
}

static unsigned int handleDeleteMessage(stateNode *state,
                                        unsigned int messageOffset) {
    messageNode *node = MESSAGE_NODE(state, messageOffset);
    unsigned int result = deleteMessage(state, messageOffset);
    if (result == ERR_NODE_DELETED) {
        return RULES_OK;
    } else if (result != RULES_OK) {
        return result;
    }

    for (unsigned int i = 0; i < node->locationCount; ++i) {
        leftFrameNode *frame = NULL;

        switch(node->locations[i].frameType) {
            case LEFT_FRAME:
                frame = LEFT_FRAME_NODE(state, 
                                        node->locations[i].nodeIndex,
                                        node->locations[i].frameOffset);
                
                result = deleteLeftFrame(state, node->locations[i]);
                if (result != RULES_OK && result != ERR_NODE_DELETED) {
                    return result;
                }

                break;
            case ACTION_FRAME:
                frame = ACTION_FRAME_NODE(state, 
                                          node->locations[i].nodeIndex,
                                          node->locations[i].frameOffset);

                result = deleteActionFrame(state, node->locations[i]);
                if (result != RULES_OK && result != ERR_NODE_DELETED) {
                    return result;
                }

                break;
            case RIGHT_FRAME:
                result = deleteRightFrame(state, node->locations[i]);
                if (result != RULES_OK && result != ERR_NODE_DELETED) {
                    return result;
                }

                break;
        }

        if (frame != NULL) {
            for (int i = 0; i < frame->messageCount; ++i) {
                messageFrame *currentFrame = &frame->messages[frame->reverseIndex[i]]; 
                CHECK_RESULT(deleteLocationFromMessage(state,
                                                       currentFrame->messageNodeOffset,
                                                       node->locations[i]));
            }
        }
    }

    return RULES_OK;
}

static unsigned int handleMessageCore(ruleset *tree,
                                      jsonObject *jo, 
                                      unsigned char actionType,
                                      char **commands,
                                      unsigned int *commandCount,
                                      unsigned int *messageOffset,
                                      void **rulesBinding) {
    stateNode *sidState;
    jsonProperty *sidProperty = &jo->properties[jo->sidIndex];
    jsonProperty *midProperty = &jo->properties[jo->idIndex];

    printf("handling message core %s, %u, %u\n", jo->content, sidProperty->valueOffset, sidProperty->valueLength);
    
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

    if (actionType == ACTION_RETRACT_FACT) {
        CHECK_RESULT(getMessage(sidState,
                                mid,
                                messageOffset));

        CHECK_RESULT(handleDeleteMessage(sidState,
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
                sprintf_s(stateMessage, sizeof(char)*(50 + sidProperty->valueLength * 2), "{ \"sid\":\"%s\", \"mid\":\"sid-%s\", \"$s\":1}", sid, sid);
    #else
                char stateMessage[50 + sidProperty->valueLength * 2];
                snprintf(stateMessage, sizeof(char)*(50 + sidProperty->valueLength * 2), "{ \"sid\":\"%s\", \"mid\":\"sid-%s\", \"$s\":1}", sid, sid);
    #endif
                printf("asserting new state %s\n", stateMessage); 

                unsigned int stateMessageOffset;
                unsigned int stateResult = handleMessage(tree,
                                                         stateMessage,  
                                                         MESSAGE_TYPE_FACT,
                                                         commands,
                                                         commandCount,
                                                         &stateMessageOffset,
                                                         rulesBinding);
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
                                  void **rulesBinding) {
    char *next;
    jsonObject jo;
    CHECK_RESULT(constructObject(message,
                                 NULL, 
                                 NULL, 
                                 actionType == ACTION_ASSERT_FACT || 
                                 actionType == ACTION_RETRACT_FACT || 
                                 actionType == ACTION_REMOVE_FACT,
                                 &jo, 
                                 &next));
    
    return handleMessageCore(tree, 
                             &jo,
                             actionType,
                             commands,
                             commandCount,
                             messageOffset,
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
                                   &jo, 
                                   actionType, 
                                   commands,
                                   commandCount,
                                   &messageOffset,
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
        unsigned int messageOffset;
        result = handleMessage(tree, 
                               reply->element[i]->str + 2, 
                               action,
                               commands, 
                               commandCount, 
                               &messageOffset,
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
    printf("startHandleMessage\n");
    char *commands[MAX_COMMAND_COUNT];
    unsigned int commandCount = 0;
    unsigned int messageOffset;
    unsigned int result = handleMessage(tree, 
                                        message, 
                                        actionType, 
                                        commands,
                                        &commandCount,
                                        &messageOffset,
                                        rulesBinding);
    if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
        freeCommands(commands, commandCount);
        return result;
    }

    unsigned int batchResult = startNonBlockingBatch(*rulesBinding, commands, commandCount, replyCount);
    if (batchResult != RULES_OK) {
        return batchResult;
    }

    printf("done\n");
    return result;
}

static unsigned int executeHandleMessage(ruleset *tree, 
                                         char *message, 
                                         unsigned char actionType) {
    printf("executeHandleMessage\n");
    char *commands[MAX_COMMAND_COUNT];
    unsigned int commandCount = 0;
    void *rulesBinding = NULL;
    unsigned int messageOffset;
    unsigned int result = handleMessage(tree, 
                                        message, 
                                        actionType, 
                                        commands,
                                        &commandCount,
                                        &messageOffset,
                                        &rulesBinding);
    if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
        freeCommands(commands, commandCount);
        return result;
    }

    unsigned int batchResult = executeBatch(rulesBinding, commands, commandCount);
    if (batchResult != RULES_OK) {
        return batchResult;
    }

    printf("done\n");
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
    unsigned int messageOffset;
    unsigned int result = handleMessage(tree, 
                                        state, 
                                        ACTION_ASSERT_FACT,
                                        commands,
                                        &commandCount,
                                        &messageOffset,
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
    unsigned int messageOffset;
    result = handleMessage(tree, 
                           state,
                           ACTION_ASSERT_FACT,
                           commands,
                           &commandCount,
                           &messageOffset,
                           rulesBinding);
    if (result != RULES_OK && result != ERR_EVENT_NOT_HANDLED) {
        //reply object should be freed by the app during abandonAction
        freeCommands(commands, commandCount);
        return result;
    }

    result = startNonBlockingBatch(*rulesBinding, commands, commandCount, replyCount);

    //TODO: Durable store
    *replyCount = 1;
    *rulesBinding = 1;
    return result;

}

unsigned int startAction(unsigned int handle, 
                         char **stateFact, 
                         char **messages, 
                         void **actionHandle,
                         void **actionBinding) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);
    
    unsigned int actionStateIndex;
    actionStateNode *resultAction;
    stateNode *resultState;

    CHECK_RESULT(getNextResult(tree,
                               &resultState, 
                               &actionStateIndex,
                               &resultAction));

    CHECK_RESULT(serializeResult(tree, 
                                 resultState, 
                                 resultAction, 
                                 messages));


    CHECK_RESULT(serializeState(resultState, 
                                stateFact));    
    

    actionContext *context = malloc(sizeof(actionContext));
    if (!context) {
        return ERR_OUT_OF_MEMORY;
    }
    
    *actionHandle = context;
    *actionBinding = NULL;
    context->resultState = resultState;
    context->resultAction = resultAction;
    context->messages = *messages;
    context->stateFact = *stateFact;

    //TODO: Durable store
    *actionBinding = 1;
    printf("starting action %s, %s\n", *stateFact, *messages);

    return RULES_OK;
}

unsigned int completeAction(unsigned int handle, 
                            void *actionHandle, 
                            char *state) {

    printf("completing action\n");
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);
    actionContext *context = (actionContext*)actionHandle;

    leftFrameNode *resultFrame = RESULT_FRAME(context->resultAction, 
                                              context->resultAction->resultIndex[0]);
    for (int i = 0; i < resultFrame->messageCount; ++i) {
        messageFrame *currentMessageFrame = &resultFrame->messages[resultFrame->reverseIndex[i]]; 
        CHECK_RESULT(handleDeleteMessage(context->resultState, 
                                         currentMessageFrame->messageNodeOffset));
    }

    
    char *commands[MAX_COMMAND_COUNT];
    unsigned int commandCount = 0;
    void *rulesBinding = NULL;
    unsigned int messageOffset;
    unsigned int result = handleMessage(tree, 
                                        state,
                                        ACTION_ASSERT_FACT,
                                        commands,
                                        &commandCount,
                                        &messageOffset,
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


    free(context->messages);
    free(context->stateFact);
    free(context);
    printf("completed action\n");
    return RULES_OK;
}

unsigned int completeAndStartAction(unsigned int handle, 
                                    unsigned int expectedReplies,
                                    void *actionHandle, 
                                    char **messages) {
    printf("completing and starting action\n");
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);
    actionContext *context = (actionContext*)actionHandle;

    leftFrameNode *resultFrame = RESULT_FRAME(context->resultAction, 
                                              context->resultAction->resultIndex[0]);
    for (int i = 0; i < resultFrame->messageCount; ++i) {
        messageFrame *currentMessageFrame = &resultFrame->messages[resultFrame->reverseIndex[i]]; 
        messageNode *node = MESSAGE_NODE(context->resultState, currentMessageFrame->messageNodeOffset);

        if (node->messageType == MESSAGE_TYPE_EVENT) {
            CHECK_RESULT(handleDeleteMessage(context->resultState, 
                                             currentMessageFrame->messageNodeOffset));
        }
    }

    *messages = NULL;
    free(context->messages);
    free(context->stateFact);
    free(context);
    printf("done completing and starting action\n");
    return ERR_NO_ACTION_AVAILABLE;
}

unsigned int abandonAction(unsigned int handle, void *actionHandle) {
    printf("abandoning action\n");

    actionContext *context = (actionContext*)actionHandle;
    free(context->messages);
    free(context->stateFact);
    free(context);
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

