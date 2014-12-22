#include "state.h"

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
#define OP_TYPE 0x0B

#define NODE_ALPHA 0
#define NODE_BETA_CONNECTOR 1
#define NODE_ACTION 2
#define NODE_M_OFFSET 0
#define NODE_S_OFFSET 1

typedef struct reference {
    unsigned int hash;
    unsigned int nameOffset;
    unsigned int idOffset;
    unsigned int time;
} reference;

typedef struct jsonValue {
    unsigned char type;
    union { 
        long i; 
        double d; 
        unsigned char b; 
        unsigned int stringOffset;
        reference property;
    } value;
} jsonValue;

typedef struct expression {
    unsigned int nameOffset;
    unsigned short atLeast;
    unsigned short atMost;
    unsigned short termsLength;
    union {
        unsigned int termsOffset; 
        unsigned int *termsPointer;
    } t;
} expression;

typedef struct join {
    unsigned int expressionsOffset;
    unsigned short expressionsLength;
} join;

typedef struct alpha {
    unsigned int hash;
    unsigned char operator;
    unsigned int betaListOffset;
    unsigned int nextListOffset;
    unsigned int nextOffset;
    jsonValue right;
} alpha;

typedef struct betaConnector {
    unsigned int nextOffset;
} betaConnector;

typedef struct action {
    unsigned int index;
    unsigned int joinsOffset;
    unsigned short joinsLength;
} action;

typedef struct node {
    unsigned int nameOffset;
    unsigned char type;
    union { 
        alpha a; 
        betaConnector b; 
        action c; 
    } value;
} node;

typedef struct ruleset {
    unsigned int nameOffset;
    node *nodePool;
    unsigned int nodeOffset;
    unsigned int *nextPool;
    unsigned int nextOffset;
    char *stringPool;
    unsigned int stringPoolLength; 
    expression *expressionPool;
    unsigned int expressionOffset;
    join *joinPool;
    unsigned int joinOffset;
    unsigned int actionCount;
    void *bindingsList;
    unsigned int *stateBuckets;
    unsigned int stateBucketsLength;
    stateEntry *state; 
    unsigned int maxStateLength;
    unsigned int stateLength;
    unsigned int lruStateOffset;
    unsigned int mruStateOffset;
} ruleset;


