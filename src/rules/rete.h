#include "state.h"

// #define _PRINT 1

#define OP_NOP 0
#define OP_LT 0x01
#define OP_LTE 0x02
#define OP_GT 0x03
#define OP_GTE 0x04
#define OP_EQ 0x05
#define OP_NEQ 0x06
#define OP_EX 0x07
#define OP_NEX 0x08
#define OP_ALL 0x09
#define OP_ANY 0x0A
#define OP_OR 0x0B
#define OP_AND 0x0C
#define OP_END 0x0D
#define OP_ADD 0x0E
#define OP_SUB 0x0F
#define OP_MUL 0x10
#define OP_DIV 0x11
#define OP_TYPE 0x12
#define OP_NOT 0x13
#define OP_MT 0x14
#define OP_IMT 0x15
#define OP_IALL 0x16
#define OP_IANY 0x17

#define NODE_ALPHA 0
#define NODE_BETA 1
#define NODE_CONNECTOR 2
#define NODE_ACTION 3

#define GATE_AND 0
#define GATE_OR 1

#define NODE_M_OFFSET 0

#define MAX_STATE_INDEX_LENGTH 1024
#define MAX_SEQUENCE_EXPRESSIONS 32

typedef struct identifier {
    unsigned int propertyNameHash;
    unsigned int propertyNameOffset;
    unsigned int nameOffset;
    unsigned int nameHash;
} identifier;

typedef struct regexReference {
    unsigned int stringOffset;
    unsigned int stateMachineOffset;
    unsigned short statesLength;
    unsigned short vocabularyLength;
} regexReference;

typedef struct operand {
    unsigned char type;
    union { 
        long i; 
        double d; 
        unsigned char b; 
        unsigned int stringOffset;
        unsigned int expressionOffset;
        regexReference regex;
        identifier id;
    } value;
} operand;

typedef struct expression {
    unsigned char operator;
    operand left;
    operand right; 
} expression;

typedef struct expressionSequence {
    unsigned int nameOffset;
    unsigned int aliasOffset;
    unsigned short length;
    unsigned char not;
    expression expressions[MAX_SEQUENCE_EXPRESSIONS];
} expressionSequence;

typedef struct alpha {
    expression expression;
    unsigned int stringOffset;
    unsigned int betaListOffset;
    unsigned int nextListOffset;
    unsigned int arrayListOffset;
    unsigned int arrayOffset;
    unsigned int nextOffset;
} alpha;

typedef struct beta {
    unsigned int index;
    expressionSequence expressionSequence;
    unsigned int hash;
    unsigned int nextOffset;
    unsigned int aOffset;
    unsigned int bOffset;
    unsigned char distinct;
    unsigned char not;
    unsigned char gateType;
    unsigned char isFirst;
} beta;

typedef struct action {
    unsigned int index;
    unsigned short count;
    unsigned short cap;
    unsigned short priority;
} action;

typedef struct node {
    unsigned int nameOffset;
    unsigned char type;
    union { 
        alpha a; 
        beta b; 
        action c; 
    } value;
} node;

typedef struct ruleset {
    unsigned int nameOffset;
    unsigned int actionCount;
    unsigned int betaCount;
    unsigned int connectorCount;
    void *bindingsList;
    
    node *nodePool;
    unsigned int nodeOffset;
    
    unsigned int *nextPool;
    unsigned int nextOffset;
    
    char *stringPool;
    unsigned int stringPoolLength; 
    
    expression *expressionPool;
    unsigned int expressionOffset;
    
    char *regexStateMachinePool;
    unsigned int regexStateMachineOffset;
    
    pool statePool;
    unsigned int stateIndex[MAX_STATE_INDEX_LENGTH * 2];
    unsigned int reverseStateIndex[MAX_STATE_INDEX_LENGTH];
    unsigned int currentStateIndex;


    unsigned int (*storeMessageCallback)(void *, char *, char *, char *, unsigned char, char *);
    void *storeMessageCallbackContext;
    
    unsigned int (*deleteMessageCallback)(void *, char *, char *, char *);
    void *deleteMessageCallbackContext;
    
    unsigned int (*queueMessageCallback)(void *, char *, char *, unsigned char, char *);
    void *queueMessageCallbackContext;
    
    unsigned int (*getQueuedMessagesCallback)(void *, char *, char *);
    void *getQueuedMessagesCallbackContext;
    
    unsigned int (*getIdleStateCallback)(void *, char *);
    void *getIdleStateCallbackContext;


} ruleset;

#ifdef _PRINT

void printSimpleExpression(ruleset *tree, 
                           expression *currentExpression, 
                           unsigned char first, 
                           char *comp);

void printExpressionSequence(ruleset *tree, 
                             expressionSequence *exprs, 
                             int level);


#endif
