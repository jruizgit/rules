
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rules.h"
#include "net.h"
#include "json.h"

#define HASH_RUN 193505114 // run
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

typedef struct path {
    unsigned char operator;
    struct path **parents;
    unsigned int parentsLength;
    unsigned int namesLength;
    unsigned int *namesOffsets;
} path;

typedef struct any {
    struct all **nodes;
    unsigned int nodesLength;
} any;

typedef struct all {
    char **names;
    unsigned int namesLength;
} all;

static unsigned int createBeta(ruleset *tree, 
                               char *rule, 
                               unsigned char operator, 
                               unsigned int nextOffset, 
                               path *nextPath, 
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

static unsigned int storeQuery(ruleset *tree, 
                               char **query, 
                               unsigned int *queryOffset, 
                               unsigned int lineCount) {

    if (!tree->queryPool) {
        tree->queryPool = malloc(lineCount * sizeof(unsigned int));
        if (!tree->queryPool) {
            return ERR_OUT_OF_MEMORY;
        }

        *queryOffset = 0;
        tree->queryOffset = lineCount;
    } else {
        tree->queryPool = realloc(tree->queryPool, (tree->queryOffset + lineCount) * sizeof(unsigned int));
        if (!tree->queryPool) {
            return ERR_OUT_OF_MEMORY;
        }

        *queryOffset = tree->queryOffset;
        tree->queryOffset = tree->queryOffset + lineCount;
    }

    for (unsigned int i = 0; i < lineCount; ++i) {
        unsigned int stringOffset;
        unsigned int result = storeString(tree, query[i], &stringOffset, strlen(query[i]));
        if (result != RULES_OK) {
            return result;
        }
        tree->queryPool[*queryOffset + i] = stringOffset;
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
                      char* last, 
                      unsigned char type) {

    right->type = type;
    unsigned int leftLength;
    char temp;
    switch(type) {
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
                                  char* last, 
                                  unsigned char type) {
    
    if (right->type != type) {
        return 0;
    }

    unsigned int leftLength;
    char temp;
    switch(type) {
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

    result = readNextValue(last, &first, &last, &type);
    if (result != PARSE_OK) {
        return result;
    }

    if (type == JSON_ARRAY || type == JSON_OBJECT) {
        return ERR_UNEXPECTED_TYPE;
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

    if ((last - first) == 2) {
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
        unsigned int nameLength = last - first; 
        if (nameLength >= 4) { 
            if (!strncmp("$all", last - 3, 4)) {
                nameLength = nameLength - 4;
                reenter = 1;
            } else if (!strncmp("$any", last - 3, 4)) {
                nameLength = nameLength - 4;
                reenter = 1;
            } 
        } 
        
        result = readNextValue(last, &first, &last, &type);
        if (result != PARSE_OK) {
            return result;
        }
        if (type != JSON_OBJECT) {
            return ERR_UNEXPECTED_TYPE;
        }

        if (!reenter) {
            result = validateWrappedExpression(first);
        }
        else {
            result = validateAlgebra(first);
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

            switch (hash) {
                case HASH_WHEN:
                case HASH_WHEN_SOME:
                    result = validateWrappedExpression(first);
                    break;
                case HASH_WHEN_ANY:
                case HASH_WHEN_ALL:
                    result = validateAlgebra(first);
                    break;
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

static unsigned int findAlpha(ruleset *tree, 
                              unsigned int parentOffset, 
                              unsigned char operator, 
                              char *rule, 
                              unsigned int *resultOffset) {
    char *first;
    char *last;
    char *firstName;
    char *lastName;
    unsigned int hash;
    unsigned char type;
    unsigned int entry;
    
    readNextName(rule, &firstName, &lastName, &hash);
    readNextValue(lastName, &first, &last, &type);
    node *parent = &tree->nodePool[parentOffset];
    unsigned int *parentNext;
    if (parent->value.a.nextOffset) {
        parentNext = &tree->nextPool[parent->value.a.nextOffset];
        for (entry = hash & HASH_MASK; parentNext[entry] != 0; entry = (entry + 1) % NEXT_BUCKET_LENGTH) {
            node *currentNode = &tree->nodePool[parentNext[entry]];
            if (currentNode->value.a.hash == hash && 
                currentNode->value.a.operator == operator) {
                if (compareValue(tree, &currentNode->value.a.right, first, last, type)) {
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
                if (compareValue(tree, &currentNode->value.a.right, first, last, type)) {
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
    copyValue(tree, &newAlpha->value.a.right, first, last, type);
    return linkAlpha(tree, parentOffset, *resultOffset);
}

static unsigned int createAlpha(ruleset *tree, 
                                unsigned int *newOffset, 
                                char *rule, 
                                unsigned int nextOffset) {
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
                createAlpha(tree, &resultOffset, first, 0);
                previousOffset = resultOffset;
                result = readNextArrayValue(last, &first, &last, &type);   
            }

            return linkAlpha(tree, previousOffset, nextOffset);
        case HASH_OR:
            readNextValue(last, &first, &last, &type);
            result = readNextArrayValue(first, &first, &last, &type);
            while (result == PARSE_OK) {
                createAlpha(tree, &parentOffset, first, nextOffset);
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
        return findAlpha(tree, parentOffset, operator, first, newOffset);
    } else {
        unsigned int result = findAlpha(tree, parentOffset, operator, first, newOffset);
        if (result != RULES_OK) {
            return result;
        }

        return linkAlpha(tree, *newOffset, nextOffset);
    }
}

static unsigned int createBetaConnector(ruleset *tree, 
                                        char *rule, 
                                        path *betaPath, 
                                        unsigned int nextOffset) {
    char *first;
    char *last;
    unsigned int hash;
    unsigned char type;
    unsigned int result = readNextName(rule, &first, &last, &hash);
    while (result == PARSE_OK) {
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
        if (nameLength >= 5 && !strncmp("$some", last - 5, 5)) {
            nameLength = nameLength - 4;
            char temp = first[nameLength - 1];
            first[nameLength - 1] = '+';
            result = storeString(tree, first, &stringOffset, nameLength);
            if (result != RULES_OK) {
                return result;
            }

            first[nameLength - 1] = temp;
        } else {
            result = storeString(tree, first, &stringOffset, nameLength);
            if (result != RULES_OK) {
                return result;
            }
        }

        node *connector;
        unsigned int connectorOffset;
        result = storeNode(tree, &connector, &connectorOffset);
        if (result != RULES_OK) {
            return result;
        }

        connector->nameOffset = stringOffset;
        connector->type = NODE_BETA_CONNECTOR;
        connector->value.b.nextOffset = nextOffset;
        if (betaPath->namesLength == 0) {
            betaPath->namesLength = 1;
            betaPath->namesOffsets = malloc(sizeof(unsigned int));
            if (!betaPath->namesOffsets) {
                return ERR_OUT_OF_MEMORY;
            }

            betaPath->namesOffsets[0] = stringOffset;
        }
        else {
            betaPath->namesLength = betaPath->namesLength + 1;
            betaPath->namesOffsets = realloc(betaPath->namesOffsets, betaPath->namesLength * sizeof(unsigned int));
            if (!betaPath->namesOffsets) {
                return ERR_OUT_OF_MEMORY;
            }

            betaPath->namesOffsets[betaPath->namesLength - 1] = stringOffset;
        }

        if (operator == OP_NOP) {
            unsigned int  resultOffset = NODE_M_OFFSET;
            if (((last - first) != 2)  || (strncmp("$s", first, 2) && strncmp("$m", first, 2))) {
                readNextValue(last, &first, &last, &type);
                result = createAlpha(tree, &resultOffset, first, connectorOffset);
            } 
            else {
                if (!strncmp("$s", first, 2)) {
                    resultOffset = NODE_S_OFFSET;
                }

                readNextValue(last, &first, &last, &type);
                result = createAlpha(tree, &resultOffset, first, connectorOffset);
            }
        }
        else {
            readNextValue(last, &first, &last, &type);
            result = createBeta(tree, first, operator, connectorOffset, betaPath, NULL);
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
                               path **outPath) {

    path *betaPath = malloc(sizeof(path));
    if (!betaPath) {
        return ERR_OUT_OF_MEMORY;
    }

    betaPath->operator = operator;
    betaPath->namesLength = 0;
    betaPath->namesOffsets = NULL;
    betaPath->parents = NULL;
    betaPath->parentsLength = 0;

    if (nextPath) {
        if (nextPath->parentsLength == 0) {
            nextPath->parentsLength = nextPath->namesLength;
            nextPath->parents = calloc(nextPath->namesLength, sizeof(path*));
            if (!nextPath->parents) {
                return ERR_OUT_OF_MEMORY;
            }

            nextPath->parents[nextPath->namesLength - 1] = betaPath;
        } else {
            int lengthDiff = nextPath->namesLength - nextPath->parentsLength;
            nextPath->parents = realloc(nextPath->parents, sizeof(path*) * nextPath->namesLength);
            if (!nextPath->parents) {
                return ERR_OUT_OF_MEMORY;
            }

            for (int i = 0; i < lengthDiff -1; ++i) {
                nextPath->parents[nextPath->parentsLength + i] = NULL;
            }

            nextPath->parentsLength = nextPath->namesLength;
            nextPath->parents[nextPath->parentsLength - 1] = betaPath;
        }
    }

    if (outPath) {
        *outPath = betaPath;
    }

    return createBetaConnector(tree, rule, betaPath, nextOffset);    
}

static unsigned int unwrapAndCreateAlpha(ruleset *tree, 
                                         char *rule, 
                                         unsigned int nextOffset, 
                                         path **outPath) {
    char *first;
    char *last;
    unsigned int hash;
    unsigned int resultOffset;

    readNextName(rule, &first, &last, &hash);
    resultOffset = NODE_M_OFFSET;
    *outPath = NULL;
    unsigned int nameLength = last - first; 
    if (nameLength == 2) {
        if (!strncmp("$s", first, 2) || !strncmp("$m", first, 2)) {
            if (!strncmp("$s", first, 2)) {
                resultOffset = NODE_S_OFFSET;
            }

            return createBeta(tree, rule, OP_ANY, nextOffset, NULL, outPath);
        }
    } else if (nameLength >= 4) { 
        if (!strncmp("$all", last - 4, 4)) {
            return createBeta(tree, rule, OP_ALL, nextOffset, NULL, outPath);
        } else if (!strncmp("$any", last - 4, 4)) {
            return createBeta(tree, rule, OP_ANY, nextOffset, NULL, outPath);
        } 
    } 

    return createAlpha(tree, &resultOffset, rule, nextOffset);
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

            newAll->names = malloc((rightAll->namesLength + leftAll->namesLength) * sizeof(char*));
            if (!newAll->names) {
                return ERR_OUT_OF_MEMORY;
            }

            for (unsigned int iii = 0; iii < rightAll->namesLength; ++iii) {
                newAll->names[iii] = malloc(sizeof(char) * (strlen(rightAll->names[iii]) + 1));
                if (!newAll->names[iii]) {
                    return ERR_OUT_OF_MEMORY;
                }

                strcpy(newAll->names[iii], rightAll->names[iii]);
            }

            for (unsigned int iii = 0; iii < leftAll->namesLength; ++iii) {
                newAll->names[rightAll->namesLength + iii] = malloc(sizeof(char) * (strlen(leftAll->names[iii]) + 1));
                if (!newAll->names[rightAll->namesLength + iii]) {
                    return ERR_OUT_OF_MEMORY;
                }
                
                strcpy(newAll->names[rightAll->namesLength + iii], leftAll->names[iii]);
            }

            newAll->namesLength = rightAll->namesLength + leftAll->namesLength;
            product->nodes[i + ii * left->nodesLength] = newAll;
        }

        for (unsigned int ii = 0; ii < leftAll->namesLength; ++ii) {
            free(leftAll->names[ii]);
        }
        free(leftAll->names);
        free(leftAll);
    }

    for (unsigned int i = 0; i < right->nodesLength; ++i) {
        all *rightAll = right->nodes[i];
        for (unsigned int ii = 0; ii < rightAll->namesLength; ++ii) {
            free(rightAll->names[ii]);
        }

        free(rightAll->names);
        free(rightAll);
    }

    free(right->nodes);
    free(right);
    free(left->nodes);
    free(left);
    *result = product;
    return RULES_OK;
}

static unsigned int createSingleQuery(char *name, any **result) {
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

    newAny->nodes[0]->namesLength = 1;
    newAny->nodes[0]->names = malloc(sizeof(char*));
    if (!newAny->nodes[0]->names) {
        return ERR_OUT_OF_MEMORY;
    }

    newAny->nodes[0]->names[0] = malloc(sizeof(char) * strlen(name) + 1);
    if (!newAny->nodes[0]->names[0]) {
        return ERR_OUT_OF_MEMORY;
    }

    strcpy(newAny->nodes[0]->names[0], name);
    *result = newAny;
    return RULES_OK;
}

static unsigned int createQueries(ruleset *tree, 
                                  char *postfix, 
                                  path *betaPath, 
                                  any **result) {
    if (!betaPath) {
        return createSingleQuery(postfix, result);
    }

    any *currentAny = NULL;
    for (unsigned int i = 0; i < betaPath->namesLength; ++ i) {
        unsigned int result;
        char *name = &tree->stringPool[betaPath->namesOffsets[i]];
        int nameLength = strlen(name);
        char nextPostfix[strlen(postfix) + nameLength + 2];
        strcpy(nextPostfix, name);
        nextPostfix[nameLength] = '!';
        strcpy(&nextPostfix[nameLength + 1], postfix);
        
        any *newAny = NULL;    
        if (i >= betaPath->parentsLength || !betaPath->parents[i]) {
            result = createSingleQuery(nextPostfix, &newAny);
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

    free(betaPath->namesOffsets);
    free(betaPath->parents);
    free(betaPath);
    *result = currentAny;
    return RULES_OK;
}

static unsigned int fixupQueries(ruleset *tree, unsigned int actionOffset, path *betaPath) {
    node *actionNode = &tree->nodePool[actionOffset];
    char *postfix = &tree->stringPool[actionNode->nameOffset];
    char copyPostfix[strlen(postfix) + 1];
    strcpy(copyPostfix, postfix);
    
    any *query;
    unsigned int result = createQueries(tree, copyPostfix, betaPath, &query);
    if (result != RULES_OK) {
        return result;
    }

    char *queryStrings[query->nodesLength];
    for (unsigned int i = 0; i < query->nodesLength; ++i) {
        all *currentAll = query->nodes[i];
        unsigned int queryLength = 1;
        for (unsigned int ii = 0; ii < currentAll->namesLength; ++ii) {
            queryLength = queryLength + strlen(currentAll->names[ii]) + 1;
        }

        queryStrings[i] = malloc(sizeof(char) * queryLength);
        char *current = queryStrings[i];
        for (unsigned int ii = 0; ii < currentAll->namesLength; ++ii) {
            int nameLength = strlen(currentAll->names[ii]);
            strncpy(current, currentAll->names[ii], nameLength);
            current[nameLength] = ' ';
            current = &current[nameLength + 1];
            free(currentAll->names[ii]);
        }        

        current[0] = '\0';
        free(currentAll->names);
        free(currentAll);
    }
    
    result = storeQuery(tree, queryStrings, &actionNode->value.c.queryOffset, query->nodesLength);
    if (result != RULES_OK) {
        return result;
    }
    
    for (unsigned int i = 0; i < query->nodesLength; ++i) {
        free(queryStrings[i]);
    }
    actionNode->value.c.queryLength = query->nodesLength;
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
        char actionName[namespaceLength + lastName - firstName + 2];
        strncpy(actionName, firstName, lastName - firstName);
        
        readNextValue(lastName, &first, &lastRuleValue, &type);
        result = readNextName(first, &first, &last, &hash);
        while (result == PARSE_OK) {
            readNextValue(last, &first, &last, &type);
            switch (hash) {
                case HASH_RUN:
                    break;
                case HASH_WHEN:
                    result = unwrapAndCreateAlpha(tree, first, actionOffset, &betaPath);
                    break;
                case HASH_WHEN_SOME:
                    actionName[lastName - firstName] = '+';
                    ++lastName; 
                    result = unwrapAndCreateAlpha(tree, first, actionOffset, &betaPath);
                    break;
                case HASH_WHEN_ANY:
                    result = createBeta(tree, first, OP_ANY, actionOffset, NULL, &betaPath);
                    break;
                case HASH_WHEN_ALL:
                    result = createBeta(tree, first, OP_ALL, actionOffset, NULL, &betaPath);
                    break;
            }
            result = readNextName(last, &first, &last, &hash);
        }

        // tree->stringPool can change after storing strings
        // need to resolve namespace every time it is used.
        namespace = &tree->stringPool[tree->nameOffset];
        ruleAction = &tree->nodePool[actionOffset];
        actionName[lastName - firstName] = '!'; 
        strncpy(&actionName[lastName - firstName + 1], namespace, namespaceLength);
        result = storeString(tree, actionName, &ruleAction->nameOffset, namespaceLength + lastName - firstName + 1);
        if (result != RULES_OK) {
            return result;
        }
        
        result = fixupQueries(tree, actionOffset, betaPath);
        if (result != RULES_OK) {
            return result;
        }

        result = readNextName(lastRuleValue, &firstName, &lastName, &hash);
    }

    return RULES_OK;
}

unsigned int createRuleset(void **handle, char *name, char *rules) {
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
    tree->queryPool = NULL;
    tree->queryOffset = 0;
    tree->actionCount = 0;
    tree->bindingsMap = NULL;

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
    deleteBindingsMap(tree);
    free(tree->nodePool);
    free(tree->nextPool);
    free(tree->stringPool);
    free(tree->queryPool);
    free(tree);
    return RULES_OK;
}



