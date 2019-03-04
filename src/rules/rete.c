
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "rules.h"
#include "net.h"
#include "json.h"
#include "regex.h"

#define _PRINT 1
#define HASH_ALL 321211332 // all
#define HASH_ANY 740945997 // any
#define HASH_PRI 1450887882 // pri
#define HASH_COUNT 967958004 // count
#define HASH_CAP 41178555 // cap
#define HASH_DIST 1281379241 // dist
#define HASH_LT 542787579 // $lt
#define HASH_LTE 2350824890 // $lte
#define HASH_GT 507407960 // $gt
#define HASH_GTE 3645344263 // $gte
#define HASH_EQ 256037865 // $eq
#define HASH_NEQ 2488026869 // $neq
#define HASH_MT 576092554 // $mt
#define HASH_IMT 1472564215 // $imt
#define HASH_IALL 3261766877 // $iall
#define HASH_IANY 3379504400 // $iany
#define HASH_EX 373481198 // $ex
#define HASH_NEX 2605470202 // $nex
#define HASH_OR 340911698 // $or
#define HASH_AND 3746487396 // $and
#define HASH_M 1690058490 // $m
#define HASH_NAME 2369371622 // name 
#define HASH_ADD 4081054038 // $add
#define HASH_SUB 1040718071 // $sub
#define HASH_MUL 304370403 // $mul
#define HASH_DIV 130502 // $div
#define HASH_L 1706836109 // $l
#define HASH_R 1203507539 //$r
#define HASH_FORWARD 739185624 // $forward

#define MAX_ACTIONS 4096

#define GET_EXPRESSION(exprs, expr) do { \
    expr = &exprs->expressions[exprs->length]; \
    ++exprs->length; \
    if (exprs->length == MAX_SEQUENCE_EXPRESSIONS) { \
        return ERR_EXPRESSION_LIMIT_EXCEEDED; \
    } \
} while(0)

#define APPEND_EXPRESSION(exprs, op) do { \
    exprs->expressions[exprs->length].operator = op; \
    ++exprs->length; \
    if (exprs->length == MAX_SEQUENCE_EXPRESSIONS) { \
        return ERR_EXPRESSION_LIMIT_EXCEEDED; \
    } \
} while(0)


unsigned int firstEmptyEntry = 1;
unsigned int lastEmptyEntry = MAX_HANDLES -1;
char entriesInitialized = 0;

static unsigned int validateAlgebra(char *rule);

static unsigned int storeString(ruleset *tree, 
                                char *newString, 
                                unsigned int *stringOffset, 
                                unsigned int length) {

    unsigned int newStringLength = length + 1;
    if (!tree->stringPool) {
        tree->stringPool = malloc(newStringLength * sizeof(char));
        if (!tree->stringPool) {
            return ERR_OUT_OF_MEMORY;
        }

        *stringOffset = 0;
        tree->stringPoolLength = newStringLength;
    } else {
        tree->stringPool = realloc(tree->stringPool, (tree->stringPoolLength + newStringLength) * sizeof(char));
        if (!tree->stringPool) {
            return ERR_OUT_OF_MEMORY;
        }

        *stringOffset = tree->stringPoolLength;
        tree->stringPoolLength = tree->stringPoolLength + newStringLength;
    }

    strncpy(tree->stringPool + *stringOffset, newString, length);
    tree->stringPool[tree->stringPoolLength - 1] = '\0';
    return RULES_OK;
}

static unsigned int storeExpression(ruleset *tree, 
                               expression **newExpression, 
                               unsigned int *expressionOffset) {

    if (!tree->expressionPool) {
        tree->expressionPool = malloc(sizeof(expression));
        if (!tree->expressionPool) {
            return ERR_OUT_OF_MEMORY;
        }

        *expressionOffset = 0;
        *newExpression = &tree->expressionPool[0];
        tree->expressionOffset = 1;
    } else {
        tree->expressionPool = realloc(tree->expressionPool, (tree->expressionOffset + 1) * sizeof(expression));
        if (!tree->expressionPool) {
            return ERR_OUT_OF_MEMORY;
        }

        *expressionOffset = tree->expressionOffset;
        *newExpression = &tree->expressionPool[tree->expressionOffset];
        tree->expressionOffset = tree->expressionOffset + 1;        
    }

    return RULES_OK;
}

static unsigned int storeNode(ruleset *tree, 
                              node **newNode, 
                              unsigned int *nodeOffset) {

    if (!tree->nodePool) {
        tree->nodePool = malloc(sizeof(node));
        if (!tree->nodePool) {
            return ERR_OUT_OF_MEMORY;
        }

        *nodeOffset = 0;
        *newNode = &tree->nodePool[0];
        tree->nodeOffset = 1;
    } else {
        tree->nodePool = realloc(tree->nodePool, (tree->nodeOffset + 1) * sizeof(node));
        if (!tree->nodePool) {
            return ERR_OUT_OF_MEMORY;
        }

        *nodeOffset = tree->nodeOffset;
        *newNode = &tree->nodePool[tree->nodeOffset];
        tree->nodeOffset = tree->nodeOffset + 1;        
    }

    return RULES_OK;
}

static unsigned int storeAlpha(ruleset *tree, 
                               node **newNode, 
                               unsigned int *nodeOffset) {

    unsigned int result = storeNode(tree, newNode, nodeOffset);
    if (result != RULES_OK) {
        return result;
    }

    (*newNode)->value.a.nextListOffset = 0;
    (*newNode)->value.a.betaListOffset = 0;
    (*newNode)->value.a.nextOffset = 0;
    return RULES_OK;
}

static unsigned int allocateNext(ruleset *tree, 
                                 unsigned int length, 
                                 unsigned int *offset) {

    if (!tree->nextPool) {
        tree->nextPool = malloc((length + 1) * sizeof(unsigned int));
        if (!tree->nextPool) {
            return ERR_OUT_OF_MEMORY;
        }

        memset(tree->nextPool, 0, (length + 1) * sizeof(unsigned int));
        *offset = 1;
        tree->nextOffset = length + 1;
    } else {
        tree->nextPool = realloc(tree->nextPool, (tree->nextOffset + length) * sizeof(unsigned int));
        if (!tree->nextPool) {
            return ERR_OUT_OF_MEMORY;
        }

        memset(&tree->nextPool[tree->nextOffset], 0, length * sizeof(unsigned int));
        *offset = tree->nextOffset;
        tree->nextOffset = tree->nextOffset + length;
    }

    return RULES_OK;
}


static unsigned int ensureNextHashset(ruleset *tree, node *newNode) {
    if (!newNode->value.a.nextOffset) {
        return allocateNext(tree, NEXT_BUCKET_LENGTH, &newNode->value.a.nextOffset);
    }

    return RULES_OK;
}

static unsigned int ensureNextList(ruleset *tree, node *newNode) {
    if (!newNode->value.a.nextListOffset) {
        return allocateNext(tree, NEXT_LIST_LENGTH, &newNode->value.a.nextListOffset);
    }

    return RULES_OK;
}

static unsigned int ensureBetaList(ruleset *tree, node *newNode) {
    if (!newNode->value.a.betaListOffset) {
        return allocateNext(tree, BETA_LIST_LENGTH, &newNode->value.a.betaListOffset);
    }

    return RULES_OK;
}

static void copyOperand(operand *op,
                        operand *target) {
    target->type = op->type;
    switch(op->type) {
        case JSON_IDENTIFIER:
        case JSON_MESSAGE_IDENTIFIER:
            target->value.id.propertyNameHash = op->value.id.propertyNameHash;
            target->value.id.propertyNameOffset = op->value.id.propertyNameOffset;
            target->value.id.nameOffset = op->value.id.nameOffset;
            target->value.id.nameHash = op->value.id.nameHash;
            break;
        case JSON_EXPRESSION:
        case JSON_MESSAGE_EXPRESSION:
            target->value.expressionOffset = op->value.expressionOffset;
            break;
        case JSON_STRING:
            target->value.stringOffset = op->value.stringOffset;
            break;
        case JSON_INT:
            target->value.i = op->value.i;
            break;
        case JSON_DOUBLE:
            target->value.d = op->value.d;
            break;
        case JSON_BOOL:
            target->value.b = op->value.b;    
            break;
        case JSON_REGEX:
        case JSON_IREGEX:
            target->value.regex.stringOffset = op->value.regex.stringOffset;
            target->value.regex.vocabularyLength = op->value.regex.vocabularyLength;
            target->value.regex.statesLength = op->value.regex.statesLength;
            target->value.regex.stateMachineOffset = op->value.regex.stateMachineOffset;
            break;
    }
}

static void copyExpression(expression *expr, expression *target) {
    target->operator = expr->operator;
    copyOperand(&expr->right, &target->right);
    copyOperand(&expr->left, &target->left);
}

static unsigned int copyValue(ruleset *tree, 
                              operand *right, 
                              char *first, 
                              char *last,
                              unsigned int expressionOffset,
                              identifier *id,
                              unsigned char type) {
    unsigned int result = RULES_OK;
    right->type = type;
    unsigned int leftLength;
    char temp;
    switch(type) {
        case JSON_IDENTIFIER:
        case JSON_MESSAGE_IDENTIFIER:
            right->value.id.propertyNameHash = id->propertyNameHash;
            right->value.id.propertyNameOffset = id->propertyNameOffset;
            right->value.id.nameOffset = id->nameOffset;
            right->value.id.nameHash = id->nameHash;
            break;
        case JSON_EXPRESSION:
        case JSON_MESSAGE_EXPRESSION:
            right->value.expressionOffset = expressionOffset;
            break;
        case JSON_STRING:
            leftLength = last - first;
            result = storeString(tree, first, &right->value.stringOffset, leftLength);
            break;
        case JSON_INT:
            temp = last[1];
            last[1] = '\0';
            right->value.i = atol(first);
            last[1] = temp;
            break;
        case JSON_DOUBLE:
            temp = last[1];
            last[1] = '\0';
            right->value.d = atof(first);
            last[1] = temp;
            break;
        case JSON_BOOL:
            leftLength = last - first + 1;
            unsigned char leftb = 1;
            if (leftLength == 5 && strncmp("false", first, 5) == 0) {
                leftb = 0;
            }
            right->value.b = leftb;
            break;
        case JSON_REGEX:
        case JSON_IREGEX:
            leftLength = last - first;
            result = storeString(tree, first, &right->value.regex.stringOffset, leftLength);
            if (result != RULES_OK) {
                return result;
            }

            result = compileRegex(tree, 
                                  first, 
                                  last,
                                  (type == JSON_REGEX) ? 0 : 1,
                                  &right->value.regex.vocabularyLength,
                                  &right->value.regex.statesLength,
                                  &right->value.regex.stateMachineOffset);
            break;
    }

    return result;
}

static unsigned char compareValue(ruleset *tree, 
                                  operand *right, 
                                  char *first, 
                                  char *last,
                                  identifier *id,
                                  unsigned char type) {
    
    if (right->type != type) {
        return 0;
    }

    unsigned int leftLength;
    char temp;
    switch(type) {
        case JSON_IDENTIFIER:
        case JSON_MESSAGE_IDENTIFIER:
            if (right->value.id.propertyNameHash == id->propertyNameHash &&
                right->value.id.propertyNameOffset == id->propertyNameOffset &&
                right->value.id.nameHash == id->nameHash &&
                right->value.id.nameOffset == id->nameOffset)
                return 1;

            return 0;
        case JSON_EXPRESSION:
        case JSON_MESSAGE_EXPRESSION:
            return 0;
        case JSON_STRING:
            {
                char *rightString;
                if (right->type == JSON_REGEX || right->type == JSON_IREGEX) {
                    rightString = &tree->stringPool[right->value.regex.stringOffset];
                } else {
                    rightString = &tree->stringPool[right->value.stringOffset];
                }

                leftLength = last - first;
                unsigned int rightLength = strlen(rightString);
                return (leftLength == rightLength ? !strncmp(rightString, first, rightLength): 0);
            }
        case JSON_INT:
            temp = last[1];
            last[1] = '\0';
            long lefti = atol(first);
            last[1] = temp;
            return (right->value.i == lefti);
        case JSON_DOUBLE:
            temp = last[1];
            last[1] = '\0';
            double leftd = atof(first);
            last[1] = temp;
            return (right->value.d == leftd);
        case JSON_BOOL:
            leftLength = last - first + 1;
            unsigned char leftb = 1;
            if (leftLength == 5 && strncmp("false", first, 5) == 0) {
                leftb = 0;
            } 
            return (right->value.b == leftb);     
    }

    return 0;
}

static unsigned int validateSetting(unsigned int settingHash, char *rule, unsigned char targetType) {
    char *first;
    char *last;
    unsigned int hash;
    unsigned char type;
    unsigned int result = readNextName(rule, &first, &last, &hash);
    while (result == PARSE_OK) {
        if (hash == settingHash) {
            result = readNextValue(last, &first, &last, &type);
            if (type != targetType) {
                return ERR_UNEXPECTED_TYPE;
            }

            return PARSE_OK;
        }

        result = readNextName(last, &first, &last, &hash);
    }

    return ERR_SETTING_NOT_FOUND;
}

static unsigned int validateIdentifier(char *rule, unsigned char *identifierType) {
    char *first;
    char *last;
    unsigned int hash;
    unsigned int result;

    result = readNextName(rule, &first, &last, &hash);
    if (result != PARSE_OK) {
        return result;
    }

    *identifierType = JSON_IDENTIFIER;

    if (hash == HASH_M) {
         *identifierType = JSON_MESSAGE_IDENTIFIER;
    }

    result = readNextString(last, &first, &last, &hash);
    if (result != PARSE_OK) {
        return result;
    }        

    return PARSE_OK;
}

static unsigned int validateExpression(char *rule, unsigned char *expressionType) {
    char *first;
    char *last;
    unsigned char type;
    unsigned int hash;
    unsigned int result;
    
    result = readNextName(rule, &first, &last, &hash);
    if (result != PARSE_OK) {
        return result;
    }

    if (hash != HASH_ADD && hash != HASH_SUB &&
        hash != HASH_MUL && hash != HASH_DIV) {
        return validateIdentifier(rule, expressionType);
    } else {
        result = readNextValue(last, &first, &last, &type);
        if (result != PARSE_OK) {
            return result;
        }
        
        *expressionType = 0;
        result = readNextName(first, &first, &last, &hash);
        while (result == PARSE_OK) {
            unsigned char newExpressionType = 0;
            result = readNextValue(last, &first, &last, &type);
            if (result != PARSE_OK) {
                return result;
            }

            if (type == JSON_OBJECT) {
                result = validateExpression(first, &newExpressionType);
                if (result != PARSE_OK) {
                    return result;
                }
            }

            if (newExpressionType == JSON_IDENTIFIER || newExpressionType == JSON_EXPRESSION) {
                if (*expressionType == JSON_MESSAGE_IDENTIFIER) {
                    return ERR_UNEXPECTED_TYPE;
                }

                *expressionType = JSON_EXPRESSION;
            }                  

            if (newExpressionType == JSON_MESSAGE_IDENTIFIER || newExpressionType == JSON_MESSAGE_EXPRESSION) {
                if (*expressionType == JSON_IDENTIFIER) {
                    return ERR_UNEXPECTED_TYPE;
                }

                *expressionType = JSON_EXPRESSION;
            }

            if (hash != HASH_L && hash != HASH_R) {
                return ERR_UNEXPECTED_NAME;
            }

            result = readNextName(last, &first, &last, &hash);
        }
    }

    return PARSE_OK;
}


static unsigned int validateExpressionSequence(char *rule) {
    char *first;
    char *last;
    unsigned char type;
    unsigned char operator = OP_NOP;
    unsigned int hash;
    unsigned int result = readNextName(rule, &first, &last, &hash);
    if (result != PARSE_OK) {
        return result;
    }

    switch (hash) {
        case HASH_EQ:
            operator = OP_EQ;
            break;
        case HASH_NEQ:
            operator = OP_NEQ;
            break;
        case HASH_GT:
            operator = OP_GT;
            break;
        case HASH_GTE:
            operator = OP_GTE;
            break;
        case HASH_EX:
            operator = OP_EX;
            break;
        case HASH_NEX:
            operator = OP_NEX;
            break;
        case HASH_MT:
            operator = OP_MT;
            break;
        case HASH_IMT:
            operator = OP_IMT;
            break;
        case HASH_IALL:
            operator = OP_IALL;
            break;
        case HASH_IANY:
            operator = OP_IANY;
            break;
        case HASH_LT:
            operator = OP_LT;
            break;
        case HASH_LTE:
            operator = OP_LTE;
            break;
        case HASH_AND:
        case HASH_OR:
            result = readNextValue(last, &first, &last, &type);
            if (type != JSON_ARRAY) {
                return ERR_UNEXPECTED_TYPE;
            }

            result = readNextArrayValue(first, &first, &last, &type);
            while (result == PARSE_OK) {
                result = validateExpressionSequence(first);
                if (result != PARSE_OK) {
                    return result;
                }

                result = readNextArrayValue(last, &first, &last, &type);   
            }

            return (result == PARSE_END ? RULES_OK: result);
    }

    if (operator == OP_NOP) {
        operator = OP_EQ;
        first = rule;
    } else {
        result = readNextValue(last, &first, &last, &type);
        if (result != PARSE_OK) {
            return result;
        }
        if (type != JSON_OBJECT) {
            return ERR_UNEXPECTED_TYPE;
        }
    }
    
    result = readNextName(first, &first, &last, &hash);
    if (result != PARSE_OK) {
        return result;
    }

    // Validating expressionSequence rValue
    result = readNextValue(last, &first, &last, &type);
    if (result != PARSE_OK) {
        return result;
    }

    if (operator == OP_IALL || operator == OP_IANY) {
        result = validateExpressionSequence(first);
        return result;
    }

    if (operator == OP_MT || operator == OP_IMT) {
        if (type != JSON_STRING) {
            return ERR_UNEXPECTED_TYPE; 
        }  

        result = validateRegex(first, last);
        if (result != PARSE_OK) {
            return result;
        }
    }

    if (type == JSON_ARRAY) {
        return ERR_UNEXPECTED_TYPE;
    }

    if (type == JSON_OBJECT) {
        unsigned char expressionType = 0;
        result = validateExpression(first, &expressionType);
        if (result != PARSE_OK) {
            return result;
        }
    }

    return PARSE_OK;
}

static unsigned int validateAlgebra(char *rule) {
    char *first;
    char *last;
    char *lastArrayValue;
    unsigned int hash;
    unsigned char type;
    unsigned char reenter = 0;
    unsigned int result = readNextArrayValue(rule, &first, &lastArrayValue, &type);
    while (result == PARSE_OK) {
        result = readNextName(first, &first, &last, &hash);
        unsigned int nameLength = last - first; 
        if (nameLength >= 4) {
            if (!strncmp("$all", last - 4, 4)) {
                nameLength = nameLength - 4;
                reenter = 1;
            } else if (!strncmp("$any", last - 4, 4)) {
                nameLength = nameLength - 4;
                reenter = 1;
            } else if (!strncmp("$not", last - 4, 4)) {
                nameLength = nameLength - 4;    
            }

            if (nameLength == 0) {
                return ERR_RULE_WITHOUT_QUALIFIER;
            } 
        } 
        
        result = readNextValue(last, &first, &last, &type);
        if (result != PARSE_OK) {
            return result;
        }
        
        if (!reenter) {
            result = validateExpressionSequence(first);
        }
        else {
            result = validateAlgebra(first);
            reenter = 0;
        }

        if (result != PARSE_OK) {
            return result;
        }

        result = readNextArrayValue(lastArrayValue, &first, &lastArrayValue, &type);
    }

    return (result == PARSE_END ? PARSE_OK: result);
}

static unsigned int validateRuleset(char *rules) {
    char *first;
    char *last;
    char *firstName;
    char *lastName;
    char *lastRuleValue;
    unsigned char type;
    unsigned int hash;
    unsigned int result;
    result = readNextName(rules, &firstName, &lastName, &hash);
    while (result == PARSE_OK) {
        result = readNextValue(lastName, &first, &lastRuleValue, &type);
        if (result != PARSE_OK) {
            return result;
        }
        if (type != JSON_OBJECT) {
            return ERR_UNEXPECTED_TYPE;
        }

        unsigned int countResult = validateSetting(HASH_COUNT, first, JSON_INT);
        if (countResult != PARSE_OK && countResult != ERR_SETTING_NOT_FOUND) {
            return countResult;
        }

        unsigned int capResult = validateSetting(HASH_CAP, first, JSON_INT);
        if (capResult != PARSE_OK && capResult != ERR_SETTING_NOT_FOUND) {
            return capResult;
        }

        if (countResult == PARSE_OK && capResult == PARSE_OK) {
            return ERR_UNEXPECTED_NAME;
        }

        result = validateSetting(HASH_PRI, first, JSON_INT);
        if (result != PARSE_OK && result != ERR_SETTING_NOT_FOUND) {
            return result;
        }

        result = validateSetting(HASH_DIST, first, JSON_INT);
        if (result != PARSE_OK && result != ERR_SETTING_NOT_FOUND) {
            return result;
        }

        result = readNextName(first, &first, &last, &hash);
        while (result == PARSE_OK) {
            result = readNextValue(last, &first, &last, &type);
            if (result != PARSE_OK) {
                return result;
            }
            
            if (hash == HASH_ALL || hash == HASH_ANY) {
                result = validateAlgebra(first);
                if (result != RULES_OK && result != PARSE_END) {
                    return result;
                }
            } else if (hash != HASH_COUNT && hash != HASH_PRI && hash != HASH_CAP && hash != HASH_DIST) {
                return ERR_UNEXPECTED_NAME;
            }

            result = readNextName(last, &first, &last, &hash);
        }

        result = readNextName(lastRuleValue, &firstName, &lastName, &hash);
    }

    return (result == PARSE_END ? PARSE_OK: result);
}

static unsigned int linkAlpha(ruleset *tree, 
                              unsigned int parentOffset, 
                              unsigned int nextOffset) {
    unsigned int result;
    unsigned int entry;
    node *parentAlpha = &tree->nodePool[parentOffset];
    node *nextNode = &tree->nodePool[nextOffset];
    if (nextNode->type != NODE_ALPHA) {
        result = ensureBetaList(tree, parentAlpha);
        if (result != RULES_OK) {
            return result;
        }

        unsigned int *parentBetaList = &tree->nextPool[parentAlpha->value.a.betaListOffset];
        for (entry = 0; parentBetaList[entry] != 0; ++entry) {
            if (entry == BETA_LIST_LENGTH) {
                return ERR_RULE_BETA_LIMIT_EXCEEDED;
            }
        }

        parentBetaList[entry] = nextOffset;
    } else if (nextNode->value.a.expression.operator == OP_NEX) {
        result = ensureNextList(tree, parentAlpha);
        if (result != RULES_OK) {
            return result;
        }

        unsigned int *parentNextList = &tree->nextPool[parentAlpha->value.a.nextListOffset];
        unsigned int entry;
        for (entry = 0; parentNextList[entry] != 0; ++entry) {
            if (entry == NEXT_LIST_LENGTH) {
                return ERR_RULE_EXISTS_LIMIT_EXCEEDED;
            }
        }

        parentNextList[entry] = nextOffset;
    } else {
        result = ensureNextHashset(tree, parentAlpha);
        if (result != RULES_OK) {
            return result;
        }

        unsigned int *parentNext = &tree->nextPool[parentAlpha->value.a.nextOffset];
        unsigned int hash = nextNode->value.a.expression.left.value.id.propertyNameHash;
        for (entry = hash & HASH_MASK; parentNext[entry] != 0; entry = (entry + 1) % NEXT_BUCKET_LENGTH) {
            if ((entry + 1) % NEXT_BUCKET_LENGTH == (hash & HASH_MASK)) {
                return ERR_RULE_LIMIT_EXCEEDED;
            }
        }

        parentNext[entry] = nextOffset;
    }

    return RULES_OK;
}

static unsigned int readIdentifier(ruleset *tree, char *rule, unsigned char *expressionType, identifier *id) {
    char *first;
    char *last;
    unsigned int hash;
    unsigned int result;
    *expressionType = JSON_IDENTIFIER;    
    id->nameOffset = 0;
    readNextName(rule, &first, &last, &hash);
    id->nameHash = hash;
    result = storeString(tree, first, &id->nameOffset, last - first); 
    if (result != RULES_OK) {
        return result;
    }  

    if (hash == HASH_M) {
        *expressionType = JSON_MESSAGE_IDENTIFIER;
    }

    readNextString(last, &first, &last, &hash);
    id->propertyNameHash = hash;
    result = storeString(tree, first, &id->propertyNameOffset, last - first);
    if (result != RULES_OK) {
        return result;
    } 

    return RULES_OK;
}

static unsigned int readExpression(ruleset *tree, char *rule, unsigned char *expressionType, unsigned int *expressionOffset, identifier *id) {
    char *first;
    char *last;
    unsigned char type = 0;
    unsigned char operator = OP_NOP;
    unsigned int hash;
    unsigned int result;
    
    readNextName(rule, &first, &last, &hash);
    switch (hash) {
        case HASH_ADD:
            operator = OP_ADD;
            break;
        case HASH_SUB:
            operator = OP_SUB;
            break;
        case HASH_MUL:
            operator = OP_MUL;
            break;
        case HASH_DIV:
            operator = OP_DIV;
            break;
    }

    if (operator == OP_NOP) {
        result = readIdentifier(tree, rule, expressionType, id);
        if (result != RULES_OK) {
            return result;
        }
    } else {
        expression *newExpression = NULL;
        result = storeExpression(tree, &newExpression, expressionOffset);
        if (result != RULES_OK) {
            return result;
        }

        *expressionType = JSON_EXPRESSION;  
        newExpression->operator = operator;
        readNextValue(last, &first, &last, &type);
        result = readNextName(first, &first, &last, &hash);
        while (result == PARSE_OK) {
            unsigned int newExpressionOffset = 0;
            identifier newRef;
            readNextValue(last, &first, &last, &type);
            if (type == JSON_OBJECT) {
                result = readExpression(tree, first, &type, &newExpressionOffset, &newRef);
                if (result != RULES_OK) {
                    return result;
                }
            }
            
            if (type == JSON_IDENTIFIER || type == JSON_EXPRESSION) {
                *expressionType = JSON_EXPRESSION;  
            }

            if (*expressionType != JSON_EXPRESSION && (type == JSON_MESSAGE_IDENTIFIER || type == JSON_MESSAGE_EXPRESSION)) {
                *expressionType = JSON_MESSAGE_EXPRESSION;  
            }

            // newExpression address might have changed after readExpression
            newExpression = &tree->expressionPool[*expressionOffset];
            switch (hash) {
                case HASH_L:
                    copyValue(tree, &newExpression->left, first, last, newExpressionOffset, &newRef, type);
                    break;
                case HASH_R:
                    copyValue(tree, &newExpression->right, first, last, newExpressionOffset, &newRef, type);
                    break;
            }

            result = readNextName(last, &first, &last, &hash);
        }
    }

    return RULES_OK;
}

static unsigned int findAlpha(ruleset *tree, 
                              unsigned int parentOffset, 
                              unsigned char operator, 
                              char *rule,
                              expressionSequence *exprs, 
                              unsigned int *resultOffset) {
    char *first;
    char *last;
    char *firstName;
    char *lastName;
    unsigned char type;
    unsigned int entry;
    unsigned int hash;
    unsigned int expressionOffset;
    unsigned int result;
    identifier id;
    
    readNextName(rule, &firstName, &lastName, &hash);
    readNextValue(lastName, &first, &last, &type);
    if (type == JSON_OBJECT && operator != OP_IALL && operator != OP_IANY) {
        result = readExpression(tree, first, &type, &expressionOffset, &id);
        if (result != RULES_OK) {
            return result;
        }
    }

    node *parent = &tree->nodePool[parentOffset];
    unsigned int *parentNext;
    if (parent->value.a.nextOffset) {
        parentNext = &tree->nextPool[parent->value.a.nextOffset];
        for (entry = hash & HASH_MASK; parentNext[entry] != 0; entry = (entry + 1) % NEXT_BUCKET_LENGTH) {
            node *currentNode = &tree->nodePool[parentNext[entry]];
            if (currentNode->value.a.expression.left.value.id.propertyNameHash == hash && 
                currentNode->value.a.expression.operator == operator) {
                if (operator != OP_IALL && operator != OP_IANY) {
                    if (compareValue(tree, &currentNode->value.a.expression.right, first, last, &id, type)) {
                        *resultOffset = parentNext[entry];
                        return RULES_OK;
                    }
                }
            }
        }
    }
    
    if (parent->value.a.nextListOffset) {
        parentNext = &tree->nextPool[parent->value.a.nextListOffset];
        for (entry = 0; parentNext[entry] != 0; ++entry) {
            node *currentNode = &tree->nodePool[parentNext[entry]];
            if (currentNode->value.a.expression.left.value.id.propertyNameHash == hash && 
                currentNode->value.a.expression.operator == operator) {
                if (operator != OP_IALL && operator != OP_IANY) {
                    if (compareValue(tree, &currentNode->value.a.expression.right, first, last, &id, type)) {
                        *resultOffset = parentNext[entry];
                        return RULES_OK;
                    }
                }
            }
        }
    }

    unsigned int stringOffset;
    result = storeString(tree, firstName, &stringOffset, lastName - firstName);
    if (result != RULES_OK) {
        return result;
    }

    node *newAlpha;
    result = storeAlpha(tree, &newAlpha, resultOffset);
    if (result != RULES_OK) {
        return result;
    }

    newAlpha->nameOffset = stringOffset;
    newAlpha->type = NODE_ALPHA;
    newAlpha->value.a.expression.left.type = JSON_MESSAGE_IDENTIFIER;
    newAlpha->value.a.expression.left.value.id.propertyNameHash = hash;
    newAlpha->value.a.expression.left.value.id.propertyNameOffset = newAlpha->nameOffset;
    newAlpha->value.a.expression.left.value.id.nameOffset = 0;
    newAlpha->value.a.expression.left.value.id.nameHash = 0;
    newAlpha->value.a.expression.operator = operator;
    if (operator == OP_MT) {
        type = JSON_REGEX;
    }

    if (operator == OP_IMT) {
        type = JSON_IREGEX;
    }

    if (operator == OP_IANY || operator == OP_IALL) {
        newAlpha->value.a.expression.right.type = JSON_NIL;
    } else {
        result = copyValue(tree, &newAlpha->value.a.expression.right, first, last, expressionOffset, &id, type);
        if (result != RULES_OK) {
            return result;
        }

        if (type == JSON_IDENTIFIER || type == JSON_EXPRESSION) {
            expression *expr;
            GET_EXPRESSION(exprs, expr);
            copyExpression(&newAlpha->value.a.expression, expr);
        } 
    }

    return linkAlpha(tree, parentOffset, *resultOffset);
}

static void getSetting(unsigned int settingHash, char *rule, unsigned short *value) {
    char *first;
    char *last;
    char temp;
    unsigned int hash;
    unsigned char type;
    unsigned int result = readNextName(rule, &first, &last, &hash);
    while (result == PARSE_OK) {
        readNextValue(last, &first, &last, &type);
        if (hash == settingHash) {
            temp = first[last - first + 1];
            first[last - first + 1] = '\0';
            *value = atoi(first);
            first[last - first + 1] = temp;
            break;   
        }
        result = readNextName(last, &first, &last, &hash);
    }
}

static unsigned int createForwardAlpha(ruleset *tree,
                                       unsigned int *newOffset) {
    node *newAlpha;
    unsigned int result = storeAlpha(tree, &newAlpha, newOffset);
    if (result != RULES_OK) {
        return result;
    }

    newAlpha->nameOffset = 0;
    newAlpha->type = NODE_ALPHA;
    newAlpha->value.a.expression.left.type = JSON_MESSAGE_IDENTIFIER;
    newAlpha->value.a.expression.left.value.id.propertyNameOffset = 0;
    newAlpha->value.a.expression.left.value.id.propertyNameHash = HASH_FORWARD;
    newAlpha->value.a.expression.left.value.id.nameOffset = 0;
    newAlpha->value.a.expression.left.value.id.nameHash = 0;
    newAlpha->value.a.expression.operator = OP_NEX;

    return RULES_OK;
}

static unsigned int createAlpha(ruleset *tree, 
                                char *rule, 
                                expressionSequence *exprs,
                                unsigned int nextOffset,
                                unsigned int *newOffset) {
    char *first;
    char *last;
    unsigned char type;
    unsigned char operator = OP_NOP;
    unsigned int hash;
    unsigned int result;
    unsigned int parentOffset = *newOffset;
    readNextName(rule, &first, &last, &hash);
    switch (hash) {
        case HASH_EQ:
            operator = OP_EQ;
            break;
        case HASH_NEQ:
            operator = OP_NEQ;
            break;
        case HASH_GT:
            operator = OP_GT;
            break;
        case HASH_GTE:
            operator = OP_GTE;
            break;
        case HASH_EX:
            operator = OP_EX;
            break;
        case HASH_NEX:
            operator = OP_NEX;
            break;
        case HASH_MT:
            operator = OP_MT;
            break;
        case HASH_IMT:
            operator = OP_IMT;
            break;
        case HASH_IALL:
            operator = OP_IALL;
            break;
        case HASH_IANY:
            operator = OP_IANY;
            break;
        case HASH_LT:
            operator = OP_LT;
            break;
        case HASH_LTE:
            operator = OP_LTE;
            break;
        case HASH_AND:
            APPEND_EXPRESSION(exprs, OP_AND);
            readNextValue(last, &first, &last, &type);
            unsigned int previousOffset = 0;
            unsigned int resultOffset = parentOffset;
            result = readNextArrayValue(first, &first, &last, &type);
            while (result == PARSE_OK) {
                result = createAlpha(tree, first, exprs, 0, &resultOffset);
                if (result != RULES_OK) {
                    return result;
                }

                previousOffset = resultOffset;
                result = readNextArrayValue(last, &first, &last, &type);   
            }
            *newOffset = previousOffset;

            APPEND_EXPRESSION(exprs, OP_END);
            if (nextOffset != 0) {
                return linkAlpha(tree, previousOffset, nextOffset);
            }

            return RULES_OK;
        case HASH_OR:
            APPEND_EXPRESSION(exprs, OP_OR);
            result = createForwardAlpha(tree, newOffset);
            if (result != RULES_OK) {
                return result;
            }

            readNextValue(last, &first, &last, &type);
            result = readNextArrayValue(first, &first, &last, &type);
            while (result == PARSE_OK) {
                unsigned int single_offset = parentOffset;
                result = createAlpha(tree, first, exprs, 0, &single_offset);
                if (result != RULES_OK) {
                    return result;
                }

                result = linkAlpha(tree, single_offset, *newOffset);
                if (result != RULES_OK) {
                    return result;
                }

                result = readNextArrayValue(last, &first, &last, &type);   
            }

            APPEND_EXPRESSION(exprs, OP_END);
            if (nextOffset != 0) {
                return linkAlpha(tree, *newOffset, nextOffset);
            }

            return RULES_OK;
    }

    if (operator == OP_NOP) {
        operator = OP_EQ;
        first = rule;
    } else {
        readNextValue(last, &first, &last, &type);
    }
    
    if (operator == OP_IANY || operator == OP_IALL) {
        unsigned int result = findAlpha(tree, parentOffset, operator, first, exprs, newOffset);
        if (result != RULES_OK) {
            return result;
        }   

        unsigned int inner_offset = *newOffset;
        readNextName(first, &first, &last, &hash);
        readNextValue(last, &first, &last, &type);
        result = createAlpha(tree, first, exprs, nextOffset, &inner_offset);
        if (result != RULES_OK) {
            return result;
        }

        node *newAlpha = &tree->nodePool[inner_offset];
        if (newAlpha->value.a.expression.right.type != JSON_IDENTIFIER && newAlpha->value.a.expression.right.type != JSON_EXPRESSION) {
            return linkAlpha(tree, *newOffset, nextOffset);
        } else {
            // Functions that can execute in client or backend should follow this pattern
            node *oldAlpha = &tree->nodePool[*newOffset];
            expression *newExpression = NULL;
            unsigned int expressionOffset = 0;
            result = storeExpression(tree, &newExpression, &expressionOffset);
            if (result != RULES_OK) {
                return result;
            }

            oldAlpha->value.a.expression.right.type = JSON_EXPRESSION;
            oldAlpha->value.a.expression.right.value.expressionOffset = expressionOffset;
            copyExpression(&newAlpha->value.a.expression, newExpression);

            expression *betaExpression;
            GET_EXPRESSION(exprs, betaExpression);
            copyExpression(newExpression, betaExpression);
            return linkAlpha(tree, *newOffset, nextOffset);
        }
    }

    if (nextOffset == 0) {
        return findAlpha(tree, parentOffset, operator, first, exprs, newOffset);
    } else {
        unsigned int result = findAlpha(tree, parentOffset, operator, first, exprs, newOffset);
        if (result != RULES_OK) {
            return result;
        }

        return linkAlpha(tree, *newOffset, nextOffset);
    }
}

static unsigned int createBeta(ruleset *tree, 
                               char *rule,
                               unsigned short distinct,
                               unsigned int nextOffset) {
    char *first;
    char *last;
    char *lastArrayValue;
    unsigned int hash;
    unsigned int previousOffset = 0;
    unsigned char type; 
    unsigned int result = readNextArrayValue(rule, &first, &lastArrayValue, &type);
    while (result == PARSE_OK) {
        readNextName(first, &first, &last, &hash);
        unsigned int nameLength = last - first; 
        unsigned char operator = OP_NOP;
        if (nameLength >= 4) { 
            if (!strncmp("$all", last - 4, 4)) {
                operator = OP_ALL;
            } else if (!strncmp("$any", last - 4, 4)) {
                operator = OP_ANY;
            } else if (!strncmp("$not", last - 4, 4)) {
                operator = OP_NOT;
            }

            if (operator == OP_ALL || operator == OP_ANY || operator == OP_NOT) {
                nameLength = nameLength - 4;
                hash = fnv1Hash32(first, nameLength);
            }
        }        
        
        unsigned int stringOffset;
        result = storeString(tree, first, &stringOffset, nameLength);
        if (result != RULES_OK) {
            return result;
        }

        node *newBeta;
        unsigned int betaOffset;
        result = storeNode(tree, &newBeta, &betaOffset);
        if (result != RULES_OK) {
            return result;
        }

        newBeta->nameOffset = stringOffset;
        newBeta->type = NODE_BETA_CONNECTOR;
        newBeta->value.b.nextOffset = nextOffset;
        newBeta->value.b.not = (operator == OP_NOT) ? 1 : 0;
        newBeta->value.b.distinct = (distinct != 0) ? 1 : 0;
        newBeta->value.b.hash = hash;
        newBeta->value.b.index = tree->betaCount;
        ++tree->betaCount;
        
        if (previousOffset != 0) {
            tree->nodePool[previousOffset].value.b.nextOffset = betaOffset;
        }
        previousOffset = betaOffset;

        newBeta->value.b.expressionSequence.nameOffset = stringOffset;
        newBeta->value.b.expressionSequence.aliasOffset = stringOffset;
        newBeta->value.b.expressionSequence.not = (operator == OP_NOT) ? 1 : 0;
        newBeta->value.b.expressionSequence.length = 0;
        if (operator == OP_NOP || operator == OP_NOT) {
            unsigned int resultOffset = NODE_M_OFFSET;
            readNextValue(last, &first, &last, &type);
            result = createAlpha(tree, first, &newBeta->value.b.expressionSequence, betaOffset, &resultOffset);
        }
        else {
            readNextValue(last, &first, &last, &type);
            result = createBeta(tree, first, distinct, betaOffset);
        }

        if (result != RULES_OK) {
            return result;
        }

        result = readNextArrayValue(lastArrayValue, &first, &lastArrayValue, &type);
    }

    return (result == PARSE_END ? RULES_OK: result);
}

#ifdef _PRINT

static void printExpression(ruleset *tree, operand *newValue) {
    char *rightProperty;
    char *rightAlias;
    char *valueString;
    expression *newExpression;
    switch (newValue->type) {
        case JSON_IDENTIFIER:
            rightProperty = &tree->stringPool[newValue->value.id.propertyNameOffset];
            rightAlias = &tree->stringPool[newValue->value.id.nameOffset];
            printf("frame[\"%s\"][\"%s\"]", rightAlias, rightProperty);
            break;
        case JSON_MESSAGE_IDENTIFIER:
            rightProperty = &tree->stringPool[newValue->value.id.propertyNameOffset];
            printf("message[\"%s\"]", rightProperty);
            break;
        case JSON_MESSAGE_EXPRESSION:
        case JSON_EXPRESSION:
            newExpression = &tree->expressionPool[newValue->value.expressionOffset];
            printf("(");
            printExpression(tree, &newExpression->left);
            switch (newExpression->operator) {
                case OP_ADD:
                    printf("+");
                    break;
                case OP_SUB:
                    printf("-");
                    break;
                case OP_MUL:
                    printf("*");
                    break;
                case OP_DIV:
                    printf("/");
                    break;
            }
            printExpression(tree, &newExpression->right);
            printf(")");
            break;
        case JSON_STRING:
            valueString = &tree->stringPool[newValue->value.stringOffset];
            printf("%s", valueString);
            break;
        case JSON_INT:
            printf("%ld", newValue->value.i);
            break;
        case JSON_DOUBLE:
            printf("%g", newValue->value.d);
            break;
        case JSON_BOOL:
            if (newValue->value.b == 0) {
                printf("false");
            }
            else {
                printf("true");
            }
            break;
    }
}

static void printExpressionSequence(ruleset *tree, expressionSequence *exprs, int level) {
    for (int i = 0; i < level; ++ i) {
        printf("    ");
    }

    char *comp = NULL;
    char *compStack[32];
    unsigned char compTop = 0;
    unsigned char first = 1;
    
    for (unsigned short i = 0; i < exprs->length; ++i) {
        expression *currentExpression = &exprs->expressions[i];
        if (currentExpression->operator == OP_AND || currentExpression->operator == OP_OR) {
            if (first) {
                printf("(");
            } else {
                printf("%s (", comp);
            }
            
            compStack[compTop] = comp;
            ++compTop;
            first = 1;

            if (currentExpression->operator == OP_AND) {
                comp = "and";    
            } else {
                comp = "or";    
            }
            
        } else if (currentExpression->operator == OP_END) {
            --compTop;
            comp = compStack[compTop];
            printf(")");            
        } else if (currentExpression->operator == OP_IALL || currentExpression->operator == OP_IANY) {
            char *leftProperty = &tree->stringPool[currentExpression->left.value.id.propertyNameOffset];
            expression *newExpression = &tree->expressionPool[currentExpression->right.value.expressionOffset];
            char *op = "";
            switch (newExpression->operator) {
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
                    op = "!=";
                    break;
            }

            char *par = "";
            if (currentExpression->operator == OP_IALL) {
                par = "true";
            } else {
                par = "false";
            }

            if (!first) {
                printf("%s compare_array(message[\"%s\"], ", comp, leftProperty);

            } else {
                printf("compare_array(message[\"%s\"], ", leftProperty);
                first = 0;   
            }

            printExpression(tree, &newExpression->right);
            printf(", %s, %s)\n", op, par);

        } else {
            char *leftProperty = &tree->stringPool[currentExpression->left.value.id.propertyNameOffset];
            char *op = "";
            switch (currentExpression->operator) {
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
                    op = "!=";
                    break;
            }

            if (!first) {
                printf(" %s message[\"%s\"] %s ", comp, leftProperty, op);
            } else {
                printf("message[\"%s\"] %s ", leftProperty, op);
    
                first = 0;   
            }

            printExpression(tree, &currentExpression->right);
        }
    }
    printf("\n");
}

static void printActionNode(ruleset *tree, node *actionNode, int level, unsigned int offset) {
    for (int i = 0; i < level; ++ i) {
        printf("    ");
    }

    printf("-> action: name %s, count %d, cap %d, priority %d, offset %u\n", 
          &tree->stringPool[actionNode->nameOffset],
          actionNode->value.c.count,
          actionNode->value.c.cap,
          actionNode->value.c.priority,
          offset);
}

static void printBetaNode(ruleset *tree, node *betaNode, int level, unsigned int offset) {
    for (int i = 0; i < level; ++ i) {
        printf("    ");
    }

    printf("-> beta: name %s, not %d, distinct %d, index %d, offset %u\n", &tree->stringPool[betaNode->nameOffset], betaNode->value.b.not, betaNode->value.b.distinct, betaNode->value.b.index, offset);
    if (betaNode->value.b.expressionSequence.length != 0) {
        printExpressionSequence(tree, &betaNode->value.b.expressionSequence, level);
    }
    node *currentNode = &tree->nodePool[betaNode->value.b.nextOffset];
    if (currentNode->type == NODE_ACTION) {
        printActionNode(tree, currentNode, level + 1, betaNode->value.b.nextOffset);
    } else {
        printBetaNode(tree, currentNode, level + 1, betaNode->value.b.nextOffset);
    }
}

static void printAlphaNode(ruleset *tree, node *alphaNode, int level, unsigned int offset) {
    for (int i = 0; i < level; ++ i) {
        printf("    ");
    }

    printf("-> alpha: name %s, operator %x, offset %u\n", &tree->stringPool[alphaNode->nameOffset], alphaNode->value.a.expression.operator, offset);
    if (alphaNode->value.a.nextOffset) {
        unsigned int *nextHashset = &tree->nextPool[alphaNode->value.a.nextOffset];
        for (unsigned int entry = 0; entry < NEXT_BUCKET_LENGTH; ++entry) { 
            if (nextHashset[entry]) {
                printAlphaNode(tree, &tree->nodePool[nextHashset[entry]], level + 1, nextHashset[entry]);  
            }
        }  
    }

    if (alphaNode->value.a.nextListOffset) {
        unsigned int *nextList = &tree->nextPool[alphaNode->value.a.nextListOffset];
        for (unsigned int entry = 0; nextList[entry] != 0; ++entry) {
            printAlphaNode(tree, &tree->nodePool[nextList[entry]], level + 1, nextList[entry]);
        }
    }

    if (alphaNode->value.a.betaListOffset) {
        unsigned int *betaList = &tree->nextPool[alphaNode->value.a.betaListOffset];
        for (unsigned int entry = 0; betaList[entry] != 0; ++entry) {
            printBetaNode(tree, &tree->nodePool[betaList[entry]], level + 1, betaList[entry]);
        }
    }
}

#endif

static unsigned int createTree(ruleset *tree, char *rules) {
    char *first;
    char *last;
    char *lastRuleValue;
    char *firstName;
    char *lastName;
    unsigned char type;
    unsigned int hash;
    unsigned int result = readNextName(rules, &firstName, &lastName, &hash);
    node *ruleActions[MAX_ACTIONS];
    while (result == PARSE_OK) {
        node *ruleAction;
        unsigned int actionOffset;
        result = storeNode(tree, &ruleAction, &actionOffset);
        if (result != RULES_OK) {
            return result;
        }
        
        if (tree->actionCount == MAX_ACTIONS) {
            return ERR_RULE_LIMIT_EXCEEDED;
        }

        ruleAction->value.c.index = tree->actionCount;
        ruleActions[tree->actionCount] = ruleAction;
        ++tree->actionCount;
        ruleAction->type = NODE_ACTION;
        
        // tree->stringPool can change after storing strings
        // need to resolve namespace every time it is used.
        result = storeString(tree, firstName, &ruleAction->nameOffset, lastName - firstName);
        if (result != RULES_OK) {
            return result;
        }

        readNextValue(lastName, &first, &lastRuleValue, &type);
        ruleAction->value.c.priority = 0;
        ruleAction->value.c.count = 0;
        ruleAction->value.c.cap = 0;
        getSetting(HASH_PRI, first, &ruleAction->value.c.priority);
        getSetting(HASH_COUNT, first, &ruleAction->value.c.count);
        getSetting(HASH_CAP, first, &ruleAction->value.c.cap);
        if (!ruleAction->value.c.count && !ruleAction->value.c.cap) {
            ruleAction->value.c.count = 1;
        }

        //Ensure action index is assigned based on priority
        unsigned int currentIndex = ruleAction->value.c.index;
        while (currentIndex) {
            if (ruleActions[currentIndex]->value.c.priority >= ruleActions[currentIndex - 1]->value.c.priority) {
                break;
            } else {
                node *tempAction = ruleActions[currentIndex];
                ruleActions[currentIndex] = ruleActions[currentIndex - 1];
                ruleActions[currentIndex]->value.c.index = currentIndex;
                ruleActions[currentIndex - 1] = tempAction;
                ruleActions[currentIndex - 1]->value.c.index = currentIndex - 1;  
                --currentIndex; 
            }
        }

        unsigned short distinct = 1;
        getSetting(HASH_DIST, first, &distinct);
        result = readNextName(first, &first, &last, &hash);
        while (result == PARSE_OK) {
            readNextValue(last, &first, &last, &type);
            result = createBeta(tree, first, distinct, actionOffset);        
            if (result != RULES_OK) {
                return result;
            }
            
            result = readNextName(last, &first, &last, &hash);
        }

        result = readNextName(lastRuleValue, &firstName, &lastName, &hash);
    }

#ifdef _PRINT
    printAlphaNode(tree, &tree->nodePool[NODE_M_OFFSET], 0, NODE_M_OFFSET);
#endif

    return RULES_OK;
}

unsigned int createRuleset(unsigned int *handle, char *name, char *rules) {
    INITIALIZE_ENTRIES;

#ifdef _PRINT
    printf("%lu\n", sizeof(jsonObject));
    printf("%s\n", rules);
#endif

    node *newNode;
    unsigned int stringOffset;
    unsigned int nodeOffset;
    unsigned int result = validateRuleset(rules);
    if (result != PARSE_OK) {
        return result;
    }

    ruleset *tree = malloc(sizeof(ruleset));
    if (!tree) {
        return ERR_OUT_OF_MEMORY;
    }
    
    tree->stringPool = NULL;
    tree->stringPoolLength = 0;
    tree->nodePool = NULL;
    tree->nodeOffset = 0;
    tree->nextPool = NULL;
    tree->nextOffset = 0;
    tree->expressionPool = NULL;
    tree->expressionOffset = 0;
    tree->regexStateMachinePool = NULL;
    tree->regexStateMachineOffset = 0;
    tree->betaCount = 0;
    tree->actionCount = 0;
    tree->bindingsList = NULL;
    tree->currentStateIndex = 0;
    memset(tree->stateIndex, 0, MAX_STATE_INDEX_LENGTH * sizeof(unsigned int));
    memset(tree->reverseStateIndex, 0, MAX_STATE_INDEX_LENGTH * sizeof(unsigned int));
    initStatePool(tree);
    
    result = storeString(tree, name, &tree->nameOffset, strlen(name));
    if (result != RULES_OK) {
        return result;
    }

    result = storeString(tree, "m", &stringOffset, 1);
    if (result != RULES_OK) {
        return result;
    }

    result = storeAlpha(tree, &newNode, &nodeOffset);
    if (result != RULES_OK) {
        return result;
    }

    newNode->nameOffset = stringOffset;
    newNode->type = NODE_ALPHA;
    newNode->value.a.expression.operator = OP_TYPE;
    // will use random numbers for state stored event mids
    srand(time(NULL));

    CREATE_HANDLE(tree, handle);
    return createTree(tree, rules);
}

unsigned int deleteRuleset(unsigned int handle) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    deleteBindingsList(tree);
    free(tree->nodePool);
    free(tree->nextPool);
    free(tree->stringPool);
    free(tree->expressionPool);
    free(tree->statePool.content);
    free(tree);
    DELETE_HANDLE(handle);
    return RULES_OK;
}

unsigned int createClient(unsigned int *handle, char *name) {
    INITIALIZE_ENTRIES;

    ruleset *tree = malloc(sizeof(ruleset));
    if (!tree) {
        return ERR_OUT_OF_MEMORY;
    }
    
    tree->stringPool = NULL;
    tree->stringPoolLength = 0;
    tree->nodePool = NULL;
    tree->nodeOffset = 0;
    tree->nextPool = NULL;
    tree->nextOffset = 0;
    tree->expressionPool = NULL;
    tree->expressionOffset = 0;
    tree->actionCount = 0;
    tree->betaCount = 0;
    tree->bindingsList = NULL;
    memset(tree->stateIndex, 0, MAX_STATE_INDEX_LENGTH * sizeof(unsigned int));
    initStatePool(tree);
    
    unsigned int result = storeString(tree, name, &tree->nameOffset, strlen(name));
    if (result != RULES_OK) {
        return result;
    }

    CREATE_HANDLE(tree, handle);
    return RULES_OK;
}

unsigned int deleteClient(unsigned int handle) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);
    
    deleteBindingsList(tree);
    free(tree->stringPool);
    free(tree->statePool.content);
    free(tree);
    DELETE_HANDLE(handle);
    return RULES_OK;
}


