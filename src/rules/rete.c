
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rules.h"
#include "net.h"
#include "json.h"

#define HASH_WHEN 2090866807 // when
#define HASH_WHEN_ALL 3322641392 // whenAll 
#define HASH_WHEN_ANY 3322641471 // whenAny
#define HASH_WHEN_SOME 2273633803 // whenSome
#define HASH_TO 5863848 // to
#define HASH_LT 193419881 // $lt
#define HASH_LTE 2087888878 // $lte
#define HASH_GT 193419716 // $gt
#define HASH_GTE 2087883433 // $gte
#define HASH_EQ 193419647 // $eq
#define HASH_NEQ 2087890573 // $neq
#define HASH_EX 193419654 // $ex
#define HASH_NEX 2087890580 // $nex
#define HASH_OR 193419978 // $or
#define HASH_AND 2087876700 // $and
#define HASH_S 5861212 // $s
#define HASH_NAME 2090536006 // name 
#define HASH_TIME 2090760340 // time
#define HASH_AT_LEAST 4092918839 // $atLeast
#define HASH_AT_MOST 3247687841 // $atMost
#define MAX_STATE_PROPERTY_TIME 120


typedef struct path {
    unsigned char operator;
    struct path **parents;
    unsigned int parentsLength;
    expression *expressions; 
    unsigned int expressionsLength;
} path;

typedef struct any {
    struct all **nodes;
    unsigned int nodesLength;
} any;

typedef struct all {
    unsigned int *expressions;
    unsigned short expressionsLength;
} all;

static unsigned int validateAlgebra(char *rule);

static unsigned int createBeta(ruleset *tree, 
                               char *rule, 
                               unsigned char operator, 
                               unsigned int nextOffset, 
                               path *nextPath, 
                               unsigned short atLeast,
                               unsigned short atMost,
                               path **outPath);

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

static unsigned int storeJoin(ruleset *tree, 
                              join **newJoin, 
                              unsigned int *joinOffset) {

    if (!tree->joinPool) {
        tree->joinPool = malloc(sizeof(join));
        if (!tree->joinPool) {
            return ERR_OUT_OF_MEMORY;
        }

        *joinOffset = 0;
        *newJoin = &tree->joinPool[0];
        tree->joinOffset = 1;
    } else {
        tree->joinPool = realloc(tree->joinPool, (tree->joinOffset + 1) * sizeof(join));
        if (!tree->joinPool) {
            return ERR_OUT_OF_MEMORY;
        }

        *joinOffset = tree->joinOffset;
        *newJoin = &tree->joinPool[tree->joinOffset];
        tree->joinOffset = tree->joinOffset + 1;        
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

static void copyValue(ruleset *tree, 
                      jsonValue *right, 
                      char *first, 
                      char *last,
                      reference *ref,
                      unsigned char type) {
    right->type = type;
    unsigned int leftLength;
    char temp;
    switch(type) {
        case JSON_EVENT_PROPERTY:
        case JSON_STATE_PROPERTY:
            right->value.property.time = ref->time;
            right->value.property.hash = ref->hash;
            right->value.property.nameOffset = ref->nameOffset;
            right->value.property.idOffset = ref->idOffset;
            break;
        case JSON_STRING:
            leftLength = last - first;
            storeString(tree, first, &right->value.stringOffset, leftLength);
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
            if (leftLength == 5 && strncmp("false", first, 5)) {
                leftb = 0;
            }
            right->value.b = leftb;
            break;
    }
}

static unsigned char compareValue(ruleset *tree, 
                                  jsonValue *right, 
                                  char *first, 
                                  char *last,
                                  reference *ref,
                                  unsigned char type) {
    
    if (right->type != type) {
        return 0;
    }

    unsigned int leftLength;
    char temp;
    switch(type) {
        case JSON_EVENT_PROPERTY:
        case JSON_STATE_PROPERTY:
            if (right->value.property.time == ref->time &&
                right->value.property.hash == ref->hash &&
                right->value.property.nameOffset == ref->nameOffset &&
                right->value.property.idOffset == ref->idOffset)
                return 1;

            return 0;
        case JSON_STRING:
            leftLength = last - first;
            char *rightString = &tree->stringPool[right->value.stringOffset];
            unsigned int rightLength = strlen(rightString);
            return (leftLength == rightLength ? !strncmp(rightString, first, rightLength): 0);
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
            if ((leftLength == 5 && strncmp("false", first, 5)) ||
                (leftLength == 1 && first[0] == '0')) {
                leftb = 0;
            } 
            return (right->value.b == leftb);     
    }

    return 0;
}

static unsigned int validateWindowSize(char *rule) {
    char *first;
    char *last;
    unsigned int hash;
    unsigned char type;
    unsigned int result = readNextName(rule, &first, &last, &hash);
    while (result == PARSE_OK) {
        if (hash == HASH_AT_LEAST || hash == HASH_AT_MOST) {
            result = readNextValue(last, &first, &last, &type);
            if (type != JSON_INT) {
                return ERR_UNEXPECTED_TYPE;
            }
        }

        result = readNextName(last, &first, &last, &hash);
    }

    return PARSE_OK;
}

static unsigned int validateExpression(char *rule) {
    char *first;
    char *last;
    unsigned char type;
    unsigned char operator = OP_NOP;
    unsigned int hash;
    unsigned int result = readNextName(rule, &first, &last, &hash);
    if (result != PARSE_OK) {
        return result;
    }

    // Fast forward $atLeast and $atMost
    while(hash == HASH_AT_LEAST || hash == HASH_AT_MOST) {
        readNextValue(last, &first, &last, &type);
        readNextName(last, &first, &last, &hash);
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
                result = validateExpression(first);
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

    // Validating expression rValue
    result = readNextValue(last, &first, &last, &type);
    if (result != PARSE_OK) {
        return result;
    }

    if (type == JSON_ARRAY) {
        return ERR_UNEXPECTED_TYPE;
    }

    if (type == JSON_OBJECT) {
        result = readNextName(first, &first, &last, &hash);
        if (result != PARSE_OK) {
            return result;
        }
        
        result = readNextValue(last, &first, &last, &type);
        if (result != PARSE_OK) {
            return result;
        }

        if (type != JSON_STRING && type != JSON_OBJECT) {
            return ERR_UNEXPECTED_TYPE;
        }
    }

    return PARSE_OK;
}

static unsigned int validateWrappedExpression(char *rule) {
    char *first;
    char *last;
    unsigned char type;
    unsigned int hash;
    unsigned int result = readNextName(rule, &first, &last, &hash);
    if (result != PARSE_OK) {
        return result;
    }

    // Fast forward $atLeast and $atMost
    while(hash == HASH_AT_LEAST || hash == HASH_AT_MOST) {
        readNextValue(last, &first, &last, &type);
        readNextName(last, &first, &last, &hash);
    }

    unsigned int nameLength = last - first; 
    if (nameLength == 2) {
        if (!strncmp("$s", first, 2) || !strncmp("$m", first, 2)) {
            result = readNextValue(last, &first, &last, &type);
            if (result != PARSE_OK) {
                return result;
            }
            if (type != JSON_OBJECT) {
                return ERR_UNEXPECTED_TYPE;
            }

            return validateExpression(last);
        }
    } else if (nameLength >= 4) { 
        if (!strncmp("$all", last - 4, 4) || !strncmp("$any", last - 4, 4)) {
            result = readNextValue(last, &first, &last, &type);
            if (result != PARSE_OK) {
                return result;
            }
            if (type != JSON_OBJECT) {
                return ERR_UNEXPECTED_TYPE;
            }
            return validateAlgebra(first);
        } 
    } 

    return validateExpression(rule);
}

static unsigned int validateAlgebra(char *rule) {
    char *first;
    char *last;
    unsigned int hash;
    unsigned char type;
    unsigned char reenter = 0;
    unsigned int result = readNextName(rule, &first, &last, &hash);
    while (result == PARSE_OK) {
        // Fast foward $atLeast and $atMost
        while (hash == HASH_AT_LEAST || hash == HASH_AT_MOST) {
            result = readNextValue(last, &first, &last, &type);
            if (result != PARSE_OK) {
                return result;
            }

            result = readNextName(last, &first, &last, &hash);
            if (result != PARSE_OK) {
                return (result == PARSE_END ? PARSE_OK: result);
            }
        }

        unsigned int nameLength = last - first; 
        if (nameLength >= 4) {
            if (!strncmp("$all", last - 4, 4)) {
                nameLength = nameLength - 4;
                reenter = 1;
            } else if (!strncmp("$any", last - 4, 4)) {
                nameLength = nameLength - 4;
                reenter = 1;
            } 
        } 
        
        result = readNextValue(last, &first, &last, &type);
        if (result != PARSE_OK) {
            return result;
        }
        
        result = validateWindowSize(first);
        if (result != PARSE_OK) {
            return result;
        }

        if (!reenter) {
            result = validateWrappedExpression(first);
        }
        else {
            result = validateAlgebra(first);
            reenter = 0;
        }

        if (result != PARSE_OK) {
            return result;
        }

        result = readNextName(last, &first, &last, &hash);
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

        result = readNextName(first, &first, &last, &hash);
        while (result == PARSE_OK) {
            result = readNextValue(last, &first, &last, &type);
            if (result != PARSE_OK) {
                return result;
            }
            
            result = validateWindowSize(first);
            if (result != PARSE_OK) {
                return result;
            }

            switch (hash) {
                case HASH_WHEN:
                    result = validateWrappedExpression(first);
                    break;
                case HASH_WHEN_ANY:
                case HASH_WHEN_ALL:
                    result = validateAlgebra(first);
                    break;
            }

            if (result != RULES_OK && result != PARSE_END) {
                return result;
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
    }
    else if (nextNode->value.a.operator == OP_NEX) {
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
    }
    else {
        result = ensureNextHashset(tree, parentAlpha);
        if (result != RULES_OK) {
            return result;
        }

        unsigned int *parentNext = &tree->nextPool[parentAlpha->value.a.nextOffset];
        unsigned int hash = nextNode->value.a.hash;
        for (entry = hash & HASH_MASK; parentNext[entry] != 0; entry = (entry + 1) % NEXT_BUCKET_LENGTH) {
            if ((entry + 1) % NEXT_BUCKET_LENGTH == (hash & HASH_MASK)) {
                return ERR_RULE_LIMIT_EXCEEDED;
            }
        }

        parentNext[entry] = nextOffset;
    }

    return RULES_OK;
}

static unsigned char readReference(ruleset *tree, char *rule, reference *ref) {
    char *first;
    char *last;
    unsigned char type;
    unsigned int hash;
    unsigned char returnType = JSON_EVENT_PROPERTY;
    
    ref->time = MAX_STATE_PROPERTY_TIME;
    ref->idOffset = 0;
    readNextName(rule, &first, &last, &hash);
    if (hash != HASH_S) {
        storeString(tree, first, &ref->idOffset, last - first);   
        readNextString(last, &first, &last, &hash);
        ref->hash = hash;
        storeString(tree, first, &ref->nameOffset, last - first);
    } else {
        returnType = JSON_STATE_PROPERTY;
        if (readNextString(last, &first, &last, &hash) == PARSE_OK) {
            ref->hash = hash;
            storeString(tree, first, &ref->nameOffset, last - first);
        }
        else {
            readNextValue(last, &first, &last, &type);
            unsigned int result = readNextName(first, &first, &last, &hash);
            while (result == PARSE_OK) {
                switch (hash) {
                    case HASH_NAME:
                        readNextString(last, &first, &last, &hash);
                        ref->hash = hash;
                        storeString(tree, first, &ref->nameOffset, last - first);
                        break;
                    case HASH_TIME:
                        readNextValue(last, &first, &last, &type);
                        char temp = last[1];
                        last[1] = '\0';
                        ref->time = atoi(first);
                        last[1] = temp;
                        break;
                    case HASH_ID:
                        readNextValue(last, &first, &last, &type);
                        if (type == JSON_STRING) {
                            storeString(tree, first, &ref->idOffset, last - first);
                        } else{
                            storeString(tree, first, &ref->idOffset, last - first + 1);
                        }

                        break;
                    default:
                        readNextValue(last, &first, &last, &type);
                        break;
                }

                result = readNextName(last, &first, &last, &hash);
            }
        }
    }

    return returnType;
}

static unsigned int findAlpha(ruleset *tree, 
                              unsigned int parentOffset, 
                              unsigned char operator, 
                              char *rule,
                              expression *expr, 
                              unsigned int *resultOffset) {
    char *first;
    char *last;
    char *firstName;
    char *lastName;
    unsigned char type;
    unsigned int entry;
    unsigned int hash;
    reference ref;
    
    readNextName(rule, &firstName, &lastName, &hash);
    readNextValue(lastName, &first, &last, &type);
    if (type == JSON_OBJECT) {
        type = readReference(tree, first, &ref);
    }

    node *parent = &tree->nodePool[parentOffset];
    unsigned int *parentNext;
    if (parent->value.a.nextOffset) {
        parentNext = &tree->nextPool[parent->value.a.nextOffset];
        for (entry = hash & HASH_MASK; parentNext[entry] != 0; entry = (entry + 1) % NEXT_BUCKET_LENGTH) {
            node *currentNode = &tree->nodePool[parentNext[entry]];
            if (currentNode->value.a.hash == hash && 
                currentNode->value.a.operator == operator) {
                if (compareValue(tree, &currentNode->value.a.right, first, last, &ref, type)) {
                    *resultOffset = parentNext[entry];
                    return RULES_OK;
                }
            }
        }
    }
    
    if (parent->value.a.nextListOffset) {
        parentNext = &tree->nextPool[parent->value.a.nextListOffset];
        for (entry = 0; parentNext[entry] != 0; ++entry) {
            node *currentNode = &tree->nodePool[parentNext[entry]];
            if (currentNode->value.a.hash == hash&& 
                currentNode->value.a.operator == operator) {
                if (compareValue(tree, &currentNode->value.a.right, first, last, &ref, type)) {
                    *resultOffset = parentNext[entry];
                    return RULES_OK;
                }
            }
        }
    }

    unsigned int stringOffset;
    unsigned int result = storeString(tree, firstName, &stringOffset, lastName - firstName);
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
    newAlpha->value.a.hash = hash;
    newAlpha->value.a.operator = operator;
    copyValue(tree, &newAlpha->value.a.right, first, last, &ref, type);
    if (expr && type == JSON_EVENT_PROPERTY) {
        if (expr->termsLength == 0) {
            expr->termsLength = 1;
            expr->t.termsPointer = malloc(sizeof(unsigned int));
            if (!expr->t.termsPointer) {
                return ERR_OUT_OF_MEMORY;
            }

            expr->t.termsPointer[0] = *resultOffset;
        }
        else {
            expr->termsLength = expr->termsLength + 1;
            expr->t.termsPointer = realloc(expr->t.termsPointer, expr->termsLength * sizeof(unsigned int));
            if (!expr->t.termsPointer) {
                return ERR_OUT_OF_MEMORY;
            }

            expr->t.termsPointer[expr->termsLength - 1] = *resultOffset;
        }
    }
    
    return linkAlpha(tree, parentOffset, *resultOffset);
}

static void getWindowSize(char *rule, unsigned short *atLeast, unsigned short *atMost) {
    char *first;
    char *last;
    char temp;
    unsigned int hash;
    unsigned char type;
    unsigned int result = readNextName(rule, &first, &last, &hash);
    while (result == PARSE_OK) {
        readNextValue(last, &first, &last, &type);
        switch (hash) {
            case HASH_AT_LEAST:
                temp = first[last - first + 1];
                first[last - first + 1] = '\0';
                *atLeast = atoi(first);
                first[last - first + 1] = temp;
                break;
            case HASH_AT_MOST:
                temp = first[last - first + 1];
                first[last - first + 1] = '\0';
                *atMost = atoi(first);
                first[last - first + 1] = temp;
                break;
        }

        result = readNextName(last, &first, &last, &hash);
    }
}

static unsigned int createAlpha(ruleset *tree, 
                                char *rule, 
                                expression *expr,
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

    // Fast forward $atLeast and $atMost
    while(hash == HASH_AT_LEAST || hash == HASH_AT_MOST) {
        readNextValue(last, &first, &rule, &type);
        readNextName(rule, &first, &last, &hash);
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
        case HASH_LT:
            operator = OP_LT;
            break;
        case HASH_LTE:
            operator = OP_LTE;
            break;
        case HASH_AND:
            readNextValue(last, &first, &last, &type);
            unsigned int previousOffset = 0;
            unsigned int resultOffset = parentOffset;
            result = readNextArrayValue(first, &first, &last, &type);
            while (result == PARSE_OK) {
                createAlpha(tree, first, expr, 0, &resultOffset);
                previousOffset = resultOffset;
                result = readNextArrayValue(last, &first, &last, &type);   
            }

            return linkAlpha(tree, previousOffset, nextOffset);
        case HASH_OR:
            readNextValue(last, &first, &last, &type);
            result = readNextArrayValue(first, &first, &last, &type);
            while (result == PARSE_OK) {
                unsigned int single_offset = parentOffset;
                createAlpha(tree, first, expr, nextOffset, &single_offset);
                result = readNextArrayValue(last, &first, &last, &type);   
            }

            return RULES_OK;
    }

    if (operator == OP_NOP) {
        operator = OP_EQ;
        first = rule;
    } else {
        readNextValue(last, &first, &last, &type);
    }
    
    if (nextOffset == 0) {
        return findAlpha(tree, parentOffset, operator, first, expr, newOffset);
    } else {
        unsigned int result = findAlpha(tree, parentOffset, operator, first, expr, newOffset);
        if (result != RULES_OK) {
            return result;
        }

        return linkAlpha(tree, *newOffset, nextOffset);
    }
}

static unsigned int createBetaConnector(ruleset *tree, 
                                        char *rule, 
                                        path *betaPath,
                                        unsigned short atLeast,
                                        unsigned short atMost, 
                                        unsigned int nextOffset) {
    char *first;
    char *last;
    unsigned int hash;
    unsigned char type;
    unsigned int result = readNextName(rule, &first, &last, &hash);
    while (result == PARSE_OK) {
        // Fast foward $atLeast and $atMost
        while (hash == HASH_AT_LEAST || hash == HASH_AT_MOST) {
            readNextValue(last, &first, &last, &type);
            result = readNextName(last, &first, &last, &hash);
            if (result != PARSE_OK) {
                return (result == PARSE_END ? RULES_OK: result);
            }
        }

        unsigned int nameLength = last - first; 
        unsigned char operator = OP_NOP;
        if (nameLength >= 4) { 
            if (!strncmp("$all", last - 4, 4)) {
                nameLength = nameLength - 4;
                operator = OP_ALL;
            } else if (!strncmp("$any", last - 4, 4)) {
                nameLength = nameLength - 4;
                operator = OP_ANY;
            }
        }        
        
        unsigned int stringOffset;
        result = storeString(tree, first, &stringOffset, nameLength);
        if (result != RULES_OK) {
            return result;
        }

        node *connector;
        unsigned int connectorOffset;
        result = storeNode(tree, &connector, &connectorOffset);
        if (result != RULES_OK) {
            return result;
        }

        expression *expr;
        connector->nameOffset = stringOffset;
        connector->type = NODE_BETA_CONNECTOR;
        connector->value.b.nextOffset = nextOffset;
        if (betaPath->expressionsLength == 0) {
            betaPath->expressionsLength = 1;
            betaPath->expressions = malloc(sizeof(expression));
            if (!betaPath->expressions) {
                return ERR_OUT_OF_MEMORY;
            }

            expr = betaPath->expressions;
        }
        else {
            betaPath->expressionsLength = betaPath->expressionsLength + 1;
            betaPath->expressions = realloc(betaPath->expressions, betaPath->expressionsLength * sizeof(expression));
            if (!betaPath->expressions) {
                return ERR_OUT_OF_MEMORY;
            }

            expr = &betaPath->expressions[betaPath->expressionsLength - 1];
        }

        expr->nameOffset = stringOffset;
        expr->termsLength = 0;
        expr->t.termsPointer = NULL;
        char *rFirst;
        char *rLast;
        unsigned char rType;
        readNextValue(last, &rFirst, &rLast, &rType);
        expr->atLeast = atLeast;
        expr->atMost = atMost;
        getWindowSize(rFirst, &expr->atLeast, &expr->atMost);
        
        if (operator == OP_NOP) {
            unsigned int  resultOffset = NODE_M_OFFSET;
            if (((last - first) != 2)  || (strncmp("$s", first, 2) && strncmp("$m", first, 2))) {
                readNextValue(last, &first, &last, &type);
                result = createAlpha(tree, first, expr, connectorOffset, &resultOffset);
            } 
            else {
                if (!strncmp("$s", first, 2)) {
                    resultOffset = NODE_S_OFFSET;
                }

                readNextValue(last, &first, &last, &type);
                result = createAlpha(tree, first, expr, connectorOffset, &resultOffset);
            }
        }
        else {
            readNextValue(last, &first, &last, &type);
            result = createBeta(tree, first, operator, connectorOffset, betaPath, atLeast, atMost, NULL);
        }

        if (result != RULES_OK) {
            return result;
        }

        result = readNextName(last, &first, &last, &hash);
    }

    return (result == PARSE_END ? RULES_OK: result);
}

static unsigned int createBeta(ruleset *tree, 
                               char *rule, 
                               unsigned char operator, 
                               unsigned int nextOffset, 
                               path *nextPath, 
                               unsigned short atLeast,
                               unsigned short atMost,
                               path **outPath) {

    path *betaPath = malloc(sizeof(path));
    if (!betaPath) {
        return ERR_OUT_OF_MEMORY;
    }

    betaPath->operator = operator;
    betaPath->expressionsLength = 0;
    betaPath->expressions = NULL;
    betaPath->parents = NULL;
    betaPath->parentsLength = 0;

    if (nextPath) {
        if (nextPath->parentsLength == 0) {
            nextPath->parentsLength = nextPath->expressionsLength;
            nextPath->parents = calloc(nextPath->expressionsLength, sizeof(path*));
            if (!nextPath->parents) {
                return ERR_OUT_OF_MEMORY;
            }

            nextPath->parents[nextPath->expressionsLength - 1] = betaPath;
        } else {
            int lengthDiff = nextPath->expressionsLength - nextPath->parentsLength;
            nextPath->parents = realloc(nextPath->parents, sizeof(path*) * nextPath->expressionsLength);
            if (!nextPath->parents) {
                return ERR_OUT_OF_MEMORY;
            }

            for (int i = 0; i < lengthDiff -1; ++i) {
                nextPath->parents[nextPath->parentsLength + i] = NULL;
            }

            nextPath->parentsLength = nextPath->expressionsLength;
            nextPath->parents[nextPath->parentsLength - 1] = betaPath;
        }
    }

    if (outPath) {
        *outPath = betaPath;
    }

    return createBetaConnector(tree, rule, betaPath, atLeast, atMost, nextOffset);    
}

static unsigned int unwrapAndCreateAlpha(ruleset *tree, 
                                         char *rule, 
                                         unsigned int nextOffset, 
                                         unsigned short atLeast,
                                         unsigned short atMost,
                                         path **outPath) {
    char *first;
    char *last;
    unsigned int hash;
    unsigned int resultOffset;
    unsigned char type;
    readNextName(rule, &first, &last, &hash);

    // Fast forward $atLeast and $atMost
    while(hash == HASH_AT_LEAST || hash == HASH_AT_MOST) {
        readNextValue(last, &first, &rule, &type);
        readNextName(rule, &first, &last, &hash);
    }

    resultOffset = NODE_M_OFFSET;
    *outPath = NULL;
    unsigned int nameLength = last - first; 
    if (nameLength == 2) {
        if (!strncmp("$s", first, 2) || !strncmp("$m", first, 2)) {
            if (!strncmp("$s", first, 2)) {
                resultOffset = NODE_S_OFFSET;
            }

            return createBeta(tree, rule, OP_ANY, nextOffset, NULL, atLeast, atMost, outPath);
        }
    } else if (nameLength >= 4) { 
        if (!strncmp("$all", last - 4, 4)) {
            return createBeta(tree, rule, OP_ALL, nextOffset, NULL, atLeast, atMost, outPath);
        } else if (!strncmp("$any", last - 4, 4)) {
            return createBeta(tree, rule, OP_ANY, nextOffset, NULL, atLeast, atMost, outPath);
        } 
    } 


    path *betaPath = malloc(sizeof(path));
    if (!betaPath) {
        return ERR_OUT_OF_MEMORY;
    }

    betaPath->operator = OP_NOP;
    betaPath->parents = NULL;
    betaPath->parentsLength = 0;
    betaPath->expressionsLength = 1;
    betaPath->expressions = malloc(sizeof(expression));
    if (!betaPath->expressions) {
        return ERR_OUT_OF_MEMORY;
    }

    betaPath->expressions->nameOffset = 0;
    betaPath->expressions->atLeast = atLeast;
    betaPath->expressions->atMost = atMost;
    betaPath->expressions->termsLength = 0;
    betaPath->expressions->t.termsPointer = NULL;

    *outPath = betaPath;
    return createAlpha(tree, rule, NULL, nextOffset, &resultOffset);
}

static unsigned int add(any *right, any *left, any **result) {
    right->nodes = realloc(right->nodes, (right->nodesLength + left->nodesLength) * sizeof(all*));
    if (!right->nodes) {
        return ERR_OUT_OF_MEMORY;
    }
    for (unsigned int i = 0; i < left->nodesLength; ++i) {
        right->nodes[right->nodesLength + i] = left->nodes[i];
    }
    right->nodesLength = right->nodesLength + left->nodesLength;
    free(left->nodes);
    free(left);
    *result = right;
    return RULES_OK;
}

static unsigned int multiply(any *right, any *left, any **result) {
    any *product = malloc(sizeof(any));
    product->nodesLength = right->nodesLength * left->nodesLength;
    product->nodes = malloc(sizeof(all*) * product->nodesLength);

    for (unsigned int i = 0; i < left->nodesLength; ++i) {
        all *leftAll = left->nodes[i];
        for (unsigned int ii = 0; ii < right->nodesLength; ++ii) {
            all *rightAll = right->nodes[ii];
            all *newAll = malloc(sizeof(all));
            if (!newAll) {
                return ERR_OUT_OF_MEMORY;
            }

            newAll->expressions = malloc((rightAll->expressionsLength + leftAll->expressionsLength) * sizeof(unsigned int));
            if (!newAll->expressions) {
                return ERR_OUT_OF_MEMORY;
            }

            for (unsigned int iii = 0; iii < rightAll->expressionsLength; ++iii) {
                newAll->expressions[iii] =  rightAll->expressions[iii];
            }

            for (unsigned int iii = 0; iii < leftAll->expressionsLength; ++iii) {
                newAll->expressions[rightAll->expressionsLength + iii] = leftAll->expressions[iii];
            }

            newAll->expressionsLength = rightAll->expressionsLength + leftAll->expressionsLength;
            product->nodes[i + ii * left->nodesLength] = newAll;
        }

        free(leftAll->expressions);
        free(leftAll);
    }

    for (unsigned int i = 0; i < right->nodesLength; ++i) {
        all *rightAll = right->nodes[i];
        free(rightAll->expressions);
        free(rightAll);
    }

    free(right->nodes);
    free(right);
    free(left->nodes);
    free(left);
    *result = product;
    return RULES_OK;
}

static unsigned int createSingleQuery(ruleset *tree, char *name, expression *expr, any **resultAny) {
    any *newAny = malloc(sizeof(any));
    if (!newAny) {
        return ERR_OUT_OF_MEMORY;
    }

    newAny->nodesLength = 1;
    newAny->nodes = malloc(sizeof(all*));
    if (!newAny->nodes) {
        return ERR_OUT_OF_MEMORY;
    }

    newAny->nodes[0] = malloc(sizeof(all));
    if (!newAny->nodes[0]) {
        return ERR_OUT_OF_MEMORY;
    }

    newAny->nodes[0]->expressionsLength = 1;
    newAny->nodes[0]->expressions = malloc(sizeof(unsigned int));
    if (!newAny->nodes[0]->expressions) {
        return ERR_OUT_OF_MEMORY;
    }

    expression *newExpression;
    unsigned int result = storeExpression(tree, &newExpression, newAny->nodes[0]->expressions);
    if (result != RULES_OK) {
        return result;
    }

    result = storeString(tree, name, &newExpression->nameOffset, strlen(name));
    if (result != RULES_OK) {
        return result;
    }

    newExpression->atLeast = expr->atLeast;
    newExpression->atMost = expr->atMost;

    if (expr->termsLength) {
        result = allocateNext(tree, expr->termsLength, &newExpression->t.termsOffset);
        if (result != RULES_OK) {
            return result;
        }

        for (unsigned short i = 0; i < expr->termsLength; ++i) {
            tree->nextPool[newExpression->t.termsOffset + i] = expr->t.termsPointer[i];
        }

        free(expr->t.termsPointer);
    }

    *resultAny = newAny;
    return RULES_OK;
}

static unsigned int createQueries(ruleset *tree, 
                                  char *postfix, 
                                  path *betaPath, 
                                  any **anyResult) {
    any *currentAny = NULL;
    for (unsigned short i = 0; i < betaPath->expressionsLength; ++ i) {
        unsigned int result;
        expression *expr = &betaPath->expressions[i];
        if (betaPath->operator == OP_NOP) {
            result = createSingleQuery(tree, postfix, expr, anyResult);
            if (result != RULES_OK) {
                return result;
            }

            free(betaPath->expressions);
            free(betaPath);
            return RULES_OK;
        }

        char *name = &tree->stringPool[expr->nameOffset];
        int nameLength = strlen(name);
        char nextPostfix[strlen(postfix) + nameLength + 2];
        strcpy(nextPostfix, name);
        nextPostfix[nameLength] = '!';
        strcpy(&nextPostfix[nameLength + 1], postfix);
        
        any *newAny = NULL;    
        if (i >= betaPath->parentsLength || !betaPath->parents[i]) {
            result = createSingleQuery(tree, nextPostfix, expr, &newAny);
            if (result != RULES_OK) {
                return result;
            }
        }
        else {
            result = createQueries(tree, nextPostfix, betaPath->parents[i], &newAny);
            if (result != RULES_OK) {
                return result;
            }
        }

        if (!currentAny) {
            currentAny = newAny;
        }
        else if (betaPath->operator == OP_ANY) {
            result = add(currentAny, newAny, &currentAny);
            if (result != RULES_OK) {
                return result;
            }
        }
        else if (betaPath->operator == OP_ALL) {
            result = multiply(currentAny, newAny, &currentAny);
            if (result != RULES_OK) {
                return result;
            }
        }
    }

    free(betaPath->expressions);
    free(betaPath->parents);
    free(betaPath);
    *anyResult = currentAny;
    return RULES_OK;
}

static unsigned int fixupQueries(ruleset *tree, unsigned int actionOffset, char* postfix, path *betaPath) {
    node *actionNode = &tree->nodePool[actionOffset];
    char copyPostfix[strlen(postfix) + 1];
    strcpy(copyPostfix, postfix);
    
    any *query;
    unsigned int result = createQueries(tree, copyPostfix, betaPath, &query);
    if (result != RULES_OK) {
        return result;
    }

    actionNode->value.c.joinsLength = query->nodesLength;
    result = allocateNext(tree, query->nodesLength, &actionNode->value.c.joinsOffset);
    if (result != RULES_OK) {
        return result;
    }

    for (unsigned int i = 0; i < query->nodesLength; ++i) {
        all *currentAll = query->nodes[i];

        join *newJoin;
        result = storeJoin(tree, &newJoin, &tree->nextPool[actionNode->value.c.joinsOffset + i]);
        if (result != RULES_OK) {
            return result;
        }
        
        newJoin->expressionsLength = currentAll->expressionsLength;
        result = allocateNext(tree, currentAll->expressionsLength, &newJoin->expressionsOffset);
        if (result != RULES_OK) {
            return result;
        }

        for (unsigned int ii = 0; ii < currentAll->expressionsLength; ++ii) {
            tree->nextPool[newJoin->expressionsOffset + ii] = currentAll->expressions[ii];
        }

        free(currentAll->expressions);
        free(currentAll);
    }
    
    free(query->nodes);
    free(query);
    return RULES_OK;
}

static unsigned int createTree(ruleset *tree, char *rules) {
    char *first;
    char *last;
    char *lastRuleValue;
    char *firstName;
    char *lastName;
    unsigned char type;
    unsigned int hash;
    unsigned int result = readNextName(rules, &firstName, &lastName, &hash);
    while (result == PARSE_OK) {
        path *betaPath = NULL;
        node *ruleAction;
        unsigned int actionOffset;
        result = storeNode(tree, &ruleAction, &actionOffset);
        if (result != RULES_OK) {
            return result;
        }
        
        ruleAction->value.c.index = tree->actionCount;
        ++tree->actionCount;
        ruleAction->type = NODE_ACTION;

        // tree->stringPool can change after storing strings
        // need to resolve namespace every time it is used.
        char *namespace = &tree->stringPool[tree->nameOffset];
        int namespaceLength = strlen(namespace); 
        char runtimeActionName[namespaceLength + lastName - firstName + 2];
        strncpy(runtimeActionName, firstName, lastName - firstName);
        runtimeActionName[lastName - firstName] = '!';
        strncpy(&runtimeActionName[lastName - firstName + 1], namespace, namespaceLength);
        runtimeActionName[namespaceLength + lastName - firstName + 1] = '\0';
        result = storeString(tree, runtimeActionName, &ruleAction->nameOffset, namespaceLength + lastName - firstName + 1);
        if (result != RULES_OK) {
            return result;
        }

        readNextValue(lastName, &first, &lastRuleValue, &type);
        result = readNextName(first, &first, &last, &hash);
        while (result == PARSE_OK) {
            unsigned short atLeast = 1;
            unsigned short atMost = 1;
            readNextValue(last, &first, &last, &type);
            getWindowSize(first, &atLeast, &atMost);
            switch (hash) {
                case HASH_WHEN:
                    result = unwrapAndCreateAlpha(tree, first, actionOffset, atLeast, atMost, &betaPath);
                    break;
                case HASH_WHEN_ANY:
                    result = createBeta(tree, first, OP_ANY, actionOffset, NULL, atLeast, atMost, &betaPath);
                    break;
                case HASH_WHEN_ALL:
                    result = createBeta(tree, first, OP_ALL, actionOffset, NULL, atLeast, atMost, &betaPath);
                    break;
            }
            result = readNextName(last, &first, &last, &hash);
        }

        result = fixupQueries(tree, actionOffset, runtimeActionName, betaPath);
        if (result != RULES_OK) {
            return result;
        }

        result = readNextName(lastRuleValue, &firstName, &lastName, &hash);
    }

    return RULES_OK;
}

unsigned int createRuleset(void **handle, char *name, char *rules, unsigned int stateCaheSize) {
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
    tree->joinPool = NULL;
    tree->joinOffset = 0;
    tree->actionCount = 0;
    tree->bindingsList = NULL;
    tree->stateLength = 0;
    tree->state = calloc(stateCaheSize, sizeof(stateEntry));
    tree->maxStateLength = stateCaheSize;
    tree->stateBucketsLength = stateCaheSize / 4;
    tree->stateBuckets = malloc(tree->stateBucketsLength * sizeof(unsigned int));
    memset(tree->stateBuckets, 0xFF, tree->stateBucketsLength * sizeof(unsigned int));
    tree->lruStateOffset = UNDEFINED_HASH_OFFSET;
    tree->mruStateOffset = UNDEFINED_HASH_OFFSET;

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
    newNode->value.a.operator = OP_TYPE;
    
    result = storeString(tree, "s", &stringOffset, 1);
    if (result != RULES_OK) {
        return result;
    }

    result = storeAlpha(tree, &newNode, &nodeOffset);
    if (result != RULES_OK) {
        return result;
    }

    newNode->nameOffset = stringOffset;
    newNode->type = NODE_ALPHA;
    newNode->value.a.operator = OP_TYPE;
    
    *handle = tree;
    return createTree(tree, rules);
}

unsigned int deleteRuleset(void *handle) {
    ruleset *tree = (ruleset*)(handle);
    deleteBindingsList(tree);
    free(tree->nodePool);
    free(tree->nextPool);
    free(tree->stringPool);
    free(tree->expressionPool);
    free(tree->joinPool);
    free(tree->stateBuckets);
    for (unsigned int i = 0; i < tree->stateLength; ++i) {
        stateEntry *entry = &tree->state[i];
        if (entry->state) {
            free(entry->state);
        }

        if (entry->sid) {
            free(entry->sid);
        }
    }
    free(tree->state);
    free(tree);
    return RULES_OK;
}



