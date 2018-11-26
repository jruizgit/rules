
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

#define MAX_STATE_NODES 8
#define MAX_MESSAGE_NODES 8
#define MAX_LEFT_FRAME_NODES 8
#define MAX_RIGHT_FRAME_NODES 8

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

static unsigned int storeIdiom(ruleset *tree, 
                               idiom **newIdiom, 
                               unsigned int *idiomOffset) {

    if (!tree->idiomPool) {
        tree->idiomPool = malloc(sizeof(idiom));
        if (!tree->idiomPool) {
            return ERR_OUT_OF_MEMORY;
        }

        *idiomOffset = 0;
        *newIdiom = &tree->idiomPool[0];
        tree->idiomOffset = 1;
    } else {
        tree->idiomPool = realloc(tree->idiomPool, (tree->idiomOffset + 1) * sizeof(idiom));
        if (!tree->idiomPool) {
            return ERR_OUT_OF_MEMORY;
        }

        *idiomOffset = tree->idiomOffset;
        *newIdiom = &tree->idiomPool[tree->idiomOffset];
        tree->idiomOffset = tree->idiomOffset + 1;        
    }

    return RULES_OK;
}

static unsigned int appendTerm(expression *expr, unsigned int nodeOffset) {
    expr->terms[expr->termsLength] = nodeOffset;
    ++expr->termsLength;
    if (expr->termsLength == MAX_EXPRESSION_TERMS) {
        return ERR_TERM_LIMIT_EXCEEDED;
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

static unsigned int copyValue(ruleset *tree, 
                              jsonValue *right, 
                              char *first, 
                              char *last,
                              unsigned int idiomOffset,
                              reference *ref,
                              unsigned char type) {
    unsigned int result = RULES_OK;
    right->type = type;
    unsigned int leftLength;
    char temp;
    switch(type) {
        case JSON_EVENT_PROPERTY:
        case JSON_EVENT_LOCAL_PROPERTY:
            right->value.property.nameHash = ref->nameHash;
            right->value.property.nameOffset = ref->nameOffset;
            right->value.property.idOffset = ref->idOffset;
            break;
        case JSON_EVENT_IDIOM:
        case JSON_EVENT_LOCAL_IDIOM:
            right->value.idiomOffset = idiomOffset;
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
        case JSON_EVENT_LOCAL_PROPERTY:
            if (right->value.property.nameHash == ref->nameHash &&
                right->value.property.nameOffset == ref->nameOffset &&
                right->value.property.idOffset == ref->idOffset)
                return 1;

            return 0;
        case JSON_EVENT_IDIOM:
        case JSON_EVENT_LOCAL_IDIOM:
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

static unsigned int validateReference(char *rule, unsigned char *referenceType) {
    char *first;
    char *last;
    unsigned int hash;
    unsigned int result;

    result = readNextName(rule, &first, &last, &hash);
    if (result != PARSE_OK) {
        return result;
    }

    *referenceType = JSON_EVENT_PROPERTY;

    if (hash == HASH_M) {
         *referenceType = JSON_EVENT_LOCAL_PROPERTY;
    }

    result = readNextString(last, &first, &last, &hash);
    if (result != PARSE_OK) {
        return result;
    }        

    return PARSE_OK;
}

static unsigned int validateIdiom(char *rule, unsigned char *idiomType) {
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
        return validateReference(rule, idiomType);
    } else {
        result = readNextValue(last, &first, &last, &type);
        if (result != PARSE_OK) {
            return result;
        }
        
        *idiomType = 0;
        result = readNextName(first, &first, &last, &hash);
        while (result == PARSE_OK) {
            unsigned char newIdiomType = 0;
            result = readNextValue(last, &first, &last, &type);
            if (result != PARSE_OK) {
                return result;
            }

            if (type == JSON_OBJECT) {
                result = validateIdiom(first, &newIdiomType);
                if (result != PARSE_OK) {
                    return result;
                }
            }

            if (newIdiomType == JSON_EVENT_PROPERTY || newIdiomType == JSON_EVENT_IDIOM) {
                if (*idiomType == JSON_EVENT_LOCAL_PROPERTY || *idiomType == JSON_EVENT_LOCAL_PROPERTY) {
                    return ERR_UNEXPECTED_TYPE;
                }

                *idiomType = JSON_EVENT_IDIOM;
            }                  

            if (newIdiomType == JSON_EVENT_LOCAL_PROPERTY || newIdiomType == JSON_EVENT_LOCAL_IDIOM) {
                if (*idiomType == JSON_EVENT_PROPERTY || *idiomType == JSON_EVENT_PROPERTY) {
                    return ERR_UNEXPECTED_TYPE;
                }

                *idiomType = JSON_EVENT_IDIOM;
            }

            if (hash != HASH_L && hash != HASH_R) {
                return ERR_UNEXPECTED_NAME;
            }

            result = readNextName(last, &first, &last, &hash);
        }
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

    if (operator == OP_IALL || operator == OP_IANY) {
        result = validateExpression(first);
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
        unsigned char idiomType = 0;
        result = validateIdiom(first, &idiomType);
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
            result = validateExpression(first);
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
    } else if (nextNode->value.a.operator == OP_NEX) {
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

static unsigned int readReference(ruleset *tree, char *rule, unsigned char *idiomType, reference *ref) {
    char *first;
    char *last;
    unsigned int hash;
    unsigned int result;
    *idiomType = JSON_EVENT_PROPERTY;    
    ref->idOffset = 0;
    readNextName(rule, &first, &last, &hash);
    result = storeString(tree, first, &ref->idOffset, last - first); 
    if (result != RULES_OK) {
        return result;
    }  

    if (hash == HASH_M) {
        *idiomType = JSON_EVENT_LOCAL_PROPERTY;
    }

    readNextString(last, &first, &last, &hash);
    ref->nameHash = hash;
    result = storeString(tree, first, &ref->nameOffset, last - first);
    if (result != RULES_OK) {
        return result;
    } 

    return RULES_OK;
}

static unsigned int readIdiom(ruleset *tree, char *rule, unsigned char *idiomType, unsigned int *idiomOffset, reference *ref) {
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
        result = readReference(tree, rule, idiomType, ref);
        if (result != RULES_OK) {
            return result;
        }
    } else {
        idiom *newIdiom = NULL;
        result = storeIdiom(tree, &newIdiom, idiomOffset);
        if (result != RULES_OK) {
            return result;
        }

        *idiomType = JSON_EVENT_IDIOM;  
        newIdiom->operator = operator;
        readNextValue(last, &first, &last, &type);
        result = readNextName(first, &first, &last, &hash);
        while (result == PARSE_OK) {
            unsigned int newIdiomOffset = 0;
            reference newRef;
            readNextValue(last, &first, &last, &type);
            if (type == JSON_OBJECT) {
                result = readIdiom(tree, first, &type, &newIdiomOffset, &newRef);
                if (result != RULES_OK) {
                    return result;
                }
            }
            
            if (type == JSON_EVENT_PROPERTY || type == JSON_EVENT_IDIOM) {
                *idiomType = JSON_EVENT_IDIOM;  
            }

            if (*idiomType != JSON_EVENT_IDIOM && (type == JSON_EVENT_LOCAL_PROPERTY || type == JSON_EVENT_LOCAL_IDIOM)) {
                *idiomType = JSON_EVENT_LOCAL_IDIOM;  
            }

            // newIdiom address might have changed after readIdiom
            newIdiom = &tree->idiomPool[*idiomOffset];
            switch (hash) {
                case HASH_L:
                    copyValue(tree, &newIdiom->left, first, last, newIdiomOffset, &newRef, type);
                    break;
                case HASH_R:
                    copyValue(tree, &newIdiom->right, first, last, newIdiomOffset, &newRef, type);
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
                              expression *expr, 
                              unsigned int *resultOffset) {
    char *first;
    char *last;
    char *firstName;
    char *lastName;
    unsigned char type;
    unsigned int entry;
    unsigned int hash;
    unsigned int idiomOffset;
    unsigned int result;
    reference ref;
    
    readNextName(rule, &firstName, &lastName, &hash);
    readNextValue(lastName, &first, &last, &type);
    if (type == JSON_OBJECT && operator != OP_IALL && operator != OP_IANY) {
        result = readIdiom(tree, first, &type, &idiomOffset, &ref);
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
            if (currentNode->value.a.hash == hash && 
                currentNode->value.a.operator == operator) {
                if (operator != OP_IALL && operator != OP_IANY) {
                    if (compareValue(tree, &currentNode->value.a.right, first, last, &ref, type)) {
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
            if (currentNode->value.a.hash == hash && 
                currentNode->value.a.operator == operator) {
                if (operator != OP_IALL && operator != OP_IANY) {
                    if (compareValue(tree, &currentNode->value.a.right, first, last, &ref, type)) {
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
    newAlpha->value.a.hash = hash;
    newAlpha->value.a.operator = operator;
    if (operator == OP_MT) {
        type = JSON_REGEX;
    }

    if (operator == OP_IMT) {
        type = JSON_IREGEX;
    }

    if (operator == OP_IANY || operator == OP_IALL) {
        newAlpha->value.a.right.type = JSON_NIL;
    } else {
        result = copyValue(tree, &newAlpha->value.a.right, first, last, idiomOffset, &ref, type);
        if (result != RULES_OK) {
            return result;
        }

        if (type == JSON_EVENT_PROPERTY || type == JSON_EVENT_IDIOM) {
            result = appendTerm(expr, *resultOffset);
            if (result != RULES_OK) {
                return result;
            }
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
    newAlpha->value.a.hash = HASH_FORWARD;
    newAlpha->value.a.operator = OP_NEX;

    return RULES_OK;
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
            result = appendTerm(expr, tree->andNodeOffset);
            if (result != RULES_OK) {
                return result;
            }

            readNextValue(last, &first, &last, &type);
            unsigned int previousOffset = 0;
            unsigned int resultOffset = parentOffset;
            result = readNextArrayValue(first, &first, &last, &type);
            while (result == PARSE_OK) {
                result = createAlpha(tree, first, expr, 0, &resultOffset);
                if (result != RULES_OK) {
                    return result;
                }

                previousOffset = resultOffset;
                result = readNextArrayValue(last, &first, &last, &type);   
            }
            *newOffset = previousOffset;

            result = appendTerm(expr, tree->endNodeOffset);
            if (result != RULES_OK) {
                return result;
            }

            if (nextOffset != 0) {
                return linkAlpha(tree, previousOffset, nextOffset);
            }

            return RULES_OK;
        case HASH_OR:
            result = appendTerm(expr, tree->orNodeOffset);
            if (result != RULES_OK) {
                return result;
            }

            result = createForwardAlpha(tree, newOffset);
            if (result != RULES_OK) {
                return result;
            }

            readNextValue(last, &first, &last, &type);
            result = readNextArrayValue(first, &first, &last, &type);
            while (result == PARSE_OK) {
                unsigned int single_offset = parentOffset;
                result = createAlpha(tree, first, expr, 0, &single_offset);
                if (result != RULES_OK) {
                    return result;
                }

                result = linkAlpha(tree, single_offset, *newOffset);
                if (result != RULES_OK) {
                    return result;
                }

                result = readNextArrayValue(last, &first, &last, &type);   
            }

            result = appendTerm(expr, tree->endNodeOffset);
            if (result != RULES_OK) {
                return result;
            }

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
        unsigned int result = findAlpha(tree, parentOffset, operator, first, expr, newOffset);
        if (result != RULES_OK) {
            return result;
        }   

        unsigned int inner_offset = *newOffset;
        readNextName(first, &first, &last, &hash);
        readNextValue(last, &first, &last, &type);
        result = createAlpha(tree, first, expr, nextOffset, &inner_offset);
        if (result != RULES_OK) {
            return result;
        }

        node *newAlpha = &tree->nodePool[inner_offset];
        if (newAlpha->value.a.right.type != JSON_EVENT_PROPERTY && newAlpha->value.a.right.type != JSON_EVENT_IDIOM) {
            return linkAlpha(tree, *newOffset, nextOffset);
        } else {
            // Functions that can execute in client or backend should follow this pattern
            node *oldAlpha = &tree->nodePool[*newOffset];
            idiom *newIdiom = NULL;
            unsigned int idiomOffset = 0;
            result = storeIdiom(tree, &newIdiom, &idiomOffset);
            if (result != RULES_OK) {
                return result;
            }

            oldAlpha->value.a.right.type = JSON_EVENT_IDIOM;
            oldAlpha->value.a.right.value.idiomOffset = idiomOffset;

            newIdiom->operator = newAlpha->value.a.operator;
            newIdiom->right.type = newAlpha->value.a.right.type;
            if (newAlpha->value.a.right.type == JSON_EVENT_IDIOM) {
                newIdiom->right.value.idiomOffset = newAlpha->value.a.right.value.idiomOffset;
            } else {
                newIdiom->right.value.property.nameHash = newAlpha->value.a.right.value.property.nameHash;
                newIdiom->right.value.property.nameOffset = newAlpha->value.a.right.value.property.nameOffset;
                newIdiom->right.value.property.idOffset = newAlpha->value.a.right.value.property.idOffset;
            }
            newIdiom->left.type = JSON_EVENT_PROPERTY;
            newIdiom->left.value.property.nameHash = newAlpha->value.a.hash;
            newIdiom->left.value.property.nameOffset = newAlpha->nameOffset;
            newIdiom->left.value.property.idOffset = 0;

            expr->terms[expr->termsLength - 1] = *newOffset;
            return linkAlpha(tree, *newOffset, nextOffset);
        }
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

        expression *expr;
        newBeta->nameOffset = stringOffset;
        newBeta->type = NODE_BETA_CONNECTOR;
        newBeta->value.b.nextOffset = nextOffset;
        newBeta->value.b.not = (operator == OP_NOT) ? 1 : 0;
        newBeta->value.b.hash = hash;

        if (previousOffset != 0) {
            tree->nodePool[previousOffset].value.b.nextOffset = betaOffset;
        }
        previousOffset = betaOffset;

        result = storeExpression(tree, &expr, &newBeta->value.b.expressionOffset);
        if (result != RULES_OK) {
            return result;
        }

        expr->nameOffset = stringOffset;
        expr->aliasOffset = stringOffset;
        expr->not = (operator == OP_NOT) ? 1 : 0;
        expr->termsLength = 0;
        expr->distinct = (distinct != 0) ? 1 : 0;
        if (operator == OP_NOP || operator == OP_NOT) {
            unsigned int resultOffset = NODE_M_OFFSET;
            readNextValue(last, &first, &last, &type);
            result = createAlpha(tree, first, expr, betaOffset, &resultOffset);
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

static void printIdiom(ruleset *tree, jsonValue *newValue) {
    char *rightProperty;
    char *rightAlias;
    char *valueString;
    idiom *newIdiom;
    switch (newValue->type) {
        case JSON_EVENT_PROPERTY:
            rightProperty = &tree->stringPool[newValue->value.property.nameOffset];
            rightAlias = &tree->stringPool[newValue->value.property.idOffset];
            printf("frame[\"%s\"][\"%s\"]", rightAlias, rightProperty);
            break;
        case JSON_EVENT_LOCAL_PROPERTY:
            rightProperty = &tree->stringPool[newValue->value.property.nameOffset];
            printf("message[\"%s\"]", rightProperty);
            break;
        case JSON_EVENT_LOCAL_IDIOM:
        case JSON_EVENT_IDIOM:
            newIdiom = &tree->idiomPool[newValue->value.idiomOffset];
            printf("(");
            printIdiom(tree, &newIdiom->left);
            switch (newIdiom->operator) {
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
            printIdiom(tree, &newIdiom->right);
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

static void printExpression(ruleset *tree, expression *expr, int level, unsigned int offset) {
    for (int i = 0; i < level; ++ i) {
        printf("    ");
    }

    char *comp = NULL;
    char *compStack[32];
    unsigned char compTop = 0;
    unsigned char first = 1;
    
    for (unsigned short i = 0; i < expr->termsLength; ++i) {
        unsigned int currentNodeOffset = expr->terms[i];
        node *currentNode = &tree->nodePool[currentNodeOffset];
        if (currentNode->value.a.operator == OP_AND || currentNode->value.a.operator == OP_OR) {
            if (first) {
                printf("(");
            } else {
                printf("%s (", comp);
            }
            
            compStack[compTop] = comp;
            ++compTop;
            first = 1;

            if (currentNode->value.a.operator == OP_AND) {
                comp = "and";    
            } else {
                comp = "or";    
            }
            
        } else if (currentNode->value.a.operator == OP_END) {
            --compTop;
            comp = compStack[compTop];
            printf(")");            
        } else if (currentNode->value.a.operator == OP_IALL || currentNode->value.a.operator == OP_IANY) {
            char *leftProperty = &tree->stringPool[currentNode->nameOffset];
            idiom *newIdiom = &tree->idiomPool[currentNode->value.a.right.value.idiomOffset];
            char *op = "";
            switch (newIdiom->operator) {
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
            if (currentNode->value.a.operator == OP_IALL) {
                par = "true";
            } else {
                par = "false";
            }

            if (!first) {
                printf("%s compare_array(message[\"%s\"], ", comp, leftProperty);

            } else {
                if (expr->distinct) {
                    printf("distinct(frame, message) and compare_array(message[\"%s\"], ", leftProperty);
                } else {
                    printf("compare_array(message[\"%s\"], ", leftProperty);
                }
    
                first = 0;   
            }

            printIdiom(tree, &newIdiom->right);
            printf(", %s, %s)\n", op, par);

        } else {
            char *leftProperty = &tree->stringPool[currentNode->nameOffset];
            char *op = "";
            switch (currentNode->value.a.operator) {
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
                if (expr->distinct) {
                    printf("distinct(frame, message) and message[\"%s\"] %s ", leftProperty, op);
                } else {
                    printf("message[\"%s\"] %s ", leftProperty, op);
                }
    
                first = 0;   
            }

            printIdiom(tree, &currentNode->value.a.right);
        }
    }
    printf("\n");
}

static void printActionNode(ruleset *tree, node *actionNode, int level, unsigned int offset) {
    for (int i = 0; i < level; ++ i) {
        printf("    ");
    }

    printf("-> action: name %s, count %d, cap %d, priority %d, joins %d, offset %u\n", 
          &tree->stringPool[actionNode->nameOffset],
          actionNode->value.c.count,
          actionNode->value.c.cap,
          actionNode->value.c.priority,
          actionNode->value.c.joinsLength,
          offset);
}

static void printBetaNode(ruleset *tree, node *betaNode, int level, unsigned int offset) {
    for (int i = 0; i < level; ++ i) {
        printf("    ");
    }

    printf("-> beta: name %s, not %d, offset %u\n", &tree->stringPool[betaNode->nameOffset], betaNode->value.b.not, offset);
    if (betaNode->value.b.expressionOffset) {
        expression *exp = &tree->expressionPool[betaNode->value.b.expressionOffset];
        printExpression(tree, exp, level, betaNode->value.b.expressionOffset);
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

    printf("-> alpha: name %s, operator %x, offset %u\n", &tree->stringPool[alphaNode->nameOffset], alphaNode->value.a.operator, offset);
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
    while (result == PARSE_OK) {
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
#ifdef _WIN32
		char *runtimeActionName = (char *)_alloca(sizeof(char)*(namespaceLength + lastName - firstName + 2));
#else
		char runtimeActionName[namespaceLength + lastName - firstName + 2];
#endif
        
        strncpy(runtimeActionName, firstName, lastName - firstName);
        runtimeActionName[lastName - firstName] = '!';
        strncpy(&runtimeActionName[lastName - firstName + 1], namespace, namespaceLength);
        runtimeActionName[namespaceLength + lastName - firstName + 1] = '\0';
        result = storeString(tree, runtimeActionName, &ruleAction->nameOffset, namespaceLength + lastName - firstName + 1);
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
    tree->idiomPool = NULL;
    tree->idiomOffset = 0;
    tree->regexStateMachinePool = NULL;
    tree->regexStateMachineOffset = 0;
    tree->actionCount = 0;
    tree->bindingsList = NULL;
    memset(tree->stateIndex, 0, MAX_STATE_INDEX_LENGTH * sizeof(unsigned int));
    initStatePool(tree, MAX_STATE_NODES);
    initMessagePool(tree, MAX_MESSAGE_NODES);
    initLeftFramePool(tree, MAX_LEFT_FRAME_NODES);
    initRightFramePool(tree, MAX_RIGHT_FRAME_NODES);

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
    result = storeAlpha(tree, &newNode, &tree->andNodeOffset);
    if (result != RULES_OK) {
        return result;
    }

    newNode->nameOffset = 0;
    newNode->type = NODE_ALPHA;
    newNode->value.a.operator = OP_AND;
    result = storeAlpha(tree, &newNode, &tree->orNodeOffset);
    if (result != RULES_OK) {
        return result;
    }

    newNode->nameOffset = 0;
    newNode->type = NODE_ALPHA;
    newNode->value.a.operator = OP_OR;
    result = storeAlpha(tree, &newNode, &tree->endNodeOffset);
    if (result != RULES_OK) {
        return result;
    }

    newNode->nameOffset = 0;
    newNode->type = NODE_ALPHA;
    newNode->value.a.operator = OP_END;

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
    free(tree->idiomPool);
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
    tree->idiomPool = NULL;
    tree->idiomOffset = 0;
    tree->actionCount = 0;
    tree->bindingsList = NULL;
    memset(tree->stateIndex, 0, MAX_STATE_INDEX_LENGTH * sizeof(unsigned int));
    initStatePool(tree, MAX_STATE_NODES);
    
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


