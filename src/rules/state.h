
#include <time.h> 

#define HASH_ID 926444256
#define HASH_SID 3593476751
#define UNDEFINED_INDEX 0xFFFFFFFF
#define SID_BUFFER_LENGTH 2
#define ID_BUFFER_LENGTH 22

#define MESSAGE_TYPE_EVENT 0
#define MESSAGE_TYPE_FACT 1
#define MESSAGE_TYPE_STATE 2

#define ACTION_ASSERT_FACT 1
#define ACTION_ASSERT_EVENT 2
#define ACTION_RETRACT_FACT 3
#define ACTION_UPDATE_STATE 4

#define UNDEFINED_HASH_OFFSET 0
#define MAX_OBJECT_PROPERTIES 255
#define MAX_MESSAGE_FRAMES 16
#define MAX_MESSAGE_INDEX_LENGTH 512
#define MAX_LEFT_FRAME_INDEX_LENGTH 512
#define MAX_RIGHT_FRAME_INDEX_LENGTH 512
#define MAX_LOCATION_INDEX_LENGTH 16

#define LEFT_FRAME 0
#define RIGHT_FRAME 1
#define A_FRAME 2
#define B_FRAME 3
#define ACTION_FRAME 4

#define STATE_LEASE_TIME 30 

#define MESSAGE_NODE(state, offset) &((messageNode *)state->messagePool.content)[offset]

#define RIGHT_FRAME_NODE(state, index, offset) &((rightFrameNode *)state->betaState[index].rightFramePool.content)[offset]

#define LEFT_FRAME_NODE(state, index, offset) &((leftFrameNode *)state->betaState[index].leftFramePool.content)[offset]

#define A_FRAME_NODE(state, index, offset) &((leftFrameNode *)state->connectorState[index].aFramePool.content)[offset]

#define B_FRAME_NODE(state, index, offset) &((leftFrameNode *)state->connectorState[index].bFramePool.content)[offset]

#define ACTION_FRAME_NODE(state, index, offset) &((leftFrameNode *)state->actionState[index].resultPool.content)[offset]

#define RESULT_FRAME(actionNode, offset) &((leftFrameNode *)actionNode->resultPool.content)[offset];

#define STATE_NODE(tree, offset) &((stateNode *)((ruleset *)tree)->statePool.content)[offset]

#define LOCATION_NODE(message, offset) &((locationNode *)message->locationPool.content)[offset]


// defined in rete.h
struct node;

typedef struct pool {
    void *content;
    unsigned int freeOffset;
    unsigned int contentLength;
    unsigned int count;
} pool;

typedef struct jsonProperty {
    unsigned int hash;
    unsigned char type;
    unsigned short valueOffset;
    unsigned short valueLength;
    union {
        long long i; 
        double d; 
        unsigned char b; 
        char *s;
    } value;
} jsonProperty;

typedef struct jsonObject {
    char *content;
    jsonProperty properties[MAX_OBJECT_PROPERTIES];
    unsigned short propertyIndex[MAX_OBJECT_PROPERTIES];
    unsigned char propertiesLength; 
    unsigned int idIndex; 
    unsigned int sidIndex;
    char sidBuffer[SID_BUFFER_LENGTH];
    char idBuffer[ID_BUFFER_LENGTH];
} jsonObject;

typedef struct frameLocation {
    unsigned char frameType;
    unsigned int nodeIndex;
    unsigned int frameOffset;
} frameLocation;

typedef struct locationNode {
    unsigned int prevOffset;
    unsigned int nextOffset;
    unsigned int hash;
    frameLocation location;
    unsigned char isActive;
} locationNode;

typedef struct messageNode {
    unsigned int prevOffset;
    unsigned int nextOffset;
    unsigned int hash;
    unsigned char isActive;
    unsigned char messageType;
    pool locationPool;
    unsigned int locationIndex[MAX_LOCATION_INDEX_LENGTH * 2];
    jsonObject jo;
} messageNode;

typedef struct messageFrame {
    unsigned int hash;
    unsigned int nameOffset;
    unsigned int messageNodeOffset;
} messageFrame;

typedef struct leftFrameNode {
    unsigned int prevOffset;
    unsigned int nextOffset;
    unsigned int nameOffset;
    unsigned int hash;
    unsigned char isActive;
    unsigned char isDispatching;
    unsigned short messageCount;
    unsigned short reverseIndex[MAX_MESSAGE_FRAMES];
    messageFrame messages[MAX_MESSAGE_FRAMES];
} leftFrameNode;

typedef struct rightFrameNode {
    unsigned int prevOffset;
    unsigned int nextOffset;
    unsigned int hash;
    unsigned char isActive;
    unsigned int messageOffset;
} rightFrameNode;

typedef struct actionStateNode {
    struct node *reteNode;
    pool resultPool;
    unsigned int resultIndex[2];
} actionStateNode;

typedef struct connectorStateNode {
    struct node *reteNode;
    pool aFramePool;
    unsigned int aFrameIndex[2];
    pool bFramePool;
    unsigned int bFrameIndex[2];
} connectorStateNode;

typedef struct betaStateNode {
    struct node *reteNode;
    pool leftFramePool;
    unsigned int leftFrameIndex[MAX_LEFT_FRAME_INDEX_LENGTH * 2];
    pool rightFramePool;
    unsigned int rightFrameIndex[MAX_RIGHT_FRAME_INDEX_LENGTH * 2];
} betaStateNode;

typedef struct actionContext {
    unsigned int actionStateIndex;
    unsigned int resultCount;
    unsigned int resultFrameOffset;
    char *messages;
    char *stateFact;
} actionContext;

typedef struct stateNode {
    char *sid;
    time_t lockExpireTime;
    unsigned int offset;
    unsigned int prevOffset;
    unsigned int nextOffset;
    unsigned int factOffset;
    unsigned int hash;
    unsigned char isActive;
    pool messagePool;
    unsigned int messageIndex[MAX_MESSAGE_INDEX_LENGTH * 2];
    betaStateNode *betaState;
    actionStateNode *actionState; 
    connectorStateNode *connectorState;
    actionContext context;
} stateNode;


unsigned int fnv1Hash32(char *str, unsigned int len);

unsigned int getObjectProperty(jsonObject *jo, 
                               unsigned int hash, 
                               jsonProperty **property);

unsigned int setObjectProperty(jsonObject *jo, 
                               unsigned int hash, 
                               unsigned char type, 
                               unsigned short valueOffset, 
                               unsigned short valueLength);

unsigned int constructObject(char *root,
                             char *parentName, 
                             char *object,
                             char generateId,
                             jsonObject *jo,
                             char **next);

unsigned int resolveBinding(void *tree, 
                            char *sid, 
                            void **rulesBinding);


unsigned int getHash(char *sid, char *key);

unsigned int initStatePool(void *tree);

unsigned int addFrameLocation(stateNode *state,
                              frameLocation location,
                              unsigned int messageNodeOffset);

unsigned int deleteFrameLocation(stateNode *state,
                                 unsigned int messageNodeOffset,
                                 frameLocation location);


unsigned int deleteMessageFromFrame(unsigned int messageNodeOffset, 
                                    leftFrameNode *frame);

unsigned int getMessageFromFrame(stateNode *state,
                                 messageFrame *messages,
                                 unsigned int hash,
                                 jsonObject **message);

unsigned int setMessageInFrame(leftFrameNode *node,
                               unsigned int nameOffset,
                               unsigned int hash, 
                               unsigned int messageNodeOffset);

unsigned int getLastLeftFrame(stateNode *state,
                              unsigned int index, 
                              unsigned int hash,
                              frameLocation *location,
                              leftFrameNode **node);

unsigned int setLeftFrame(stateNode *state,
                          unsigned int hash, 
                          frameLocation location);


unsigned int deleteLeftFrame(stateNode *state,
                             frameLocation location);

unsigned int createLeftFrame(stateNode *state,
                             struct node *reteNode,
                             leftFrameNode *oldNode,                        
                             leftFrameNode **newNode,
                             frameLocation *newLocation);

unsigned int getLastConnectorFrame(stateNode *state,
                                   unsigned int frameType,
                                   unsigned int index, 
                                   unsigned int *valueOffset,
                                   leftFrameNode **node);

unsigned int setConnectorFrame(stateNode *state, 
                               unsigned int frameType,
                               frameLocation location);


unsigned int deleteConnectorFrame(stateNode *state,
                                  unsigned int frameType,
                                  frameLocation location);

unsigned int createConnectorFrame(stateNode *state,
                                  unsigned int frameType,
                                  struct node *reteNode,
                                  leftFrameNode *oldNode,                        
                                  leftFrameNode **newNode,
                                  frameLocation *newLocation);

unsigned int getLastRightFrame(stateNode *state,
                           unsigned int index, 
                           unsigned int hash,
                           rightFrameNode **node);

unsigned int setRightFrame(stateNode *state,
                           unsigned int hash, 
                           frameLocation location);

unsigned int deleteRightFrame(stateNode *state,
                              frameLocation location);

unsigned int createRightFrame(stateNode *state,
                              struct node *reteNode,
                              rightFrameNode **node,
                              frameLocation *location);

unsigned int getActionFrame(stateNode *state,
                            frameLocation resultLocation,
                            leftFrameNode **resultNode);

unsigned int setActionFrame(stateNode *state, 
                            frameLocation location);


unsigned int deleteActionFrame(stateNode *state,
                               frameLocation location);

unsigned int deleteDispatchingActionFrame(stateNode *state,
                                          frameLocation location);

unsigned int createActionFrame(stateNode *state,
                               struct node *reteNode,
                               leftFrameNode *oldNode,                        
                               leftFrameNode **newNode,
                               frameLocation *newLocation);

unsigned int deleteMessage(void *tree,
                           stateNode *state,
                           char *mid,
                           unsigned int messageNodeOffset);

unsigned int getMessage(stateNode *state,
                        char *mid,
                        unsigned int *valueOffset);

unsigned int storeMessage(void *tree,
                          stateNode *state,
                          char *mid,
                          jsonObject *message,
                          unsigned char messageType,
                          unsigned char sideEffect,
                          unsigned int *valueOffset);

unsigned int getStateNode(void *tree, 
                          char *sid, 
                          stateNode **state);

unsigned int createStateNode(void *tree, 
                             char *sid, 
                             stateNode **state);

unsigned int deleteStateNode(void *tree, 
                             stateNode *state);

unsigned int serializeResult(void *tree, 
                             stateNode *state, 
                             actionStateNode *actionNode, 
                             unsigned int count,
                             char **result);

unsigned int serializeState(stateNode *state, 
                            char **stateFact);

unsigned int getNextResultInState(void *tree, 
                                  time_t currentTime,
                                  stateNode *state,
                                  unsigned int *actionStateIndex,
                                  unsigned int *resultCount,
                                  unsigned int *resultFrameOffset, 
                                  actionStateNode **resultAction);

unsigned int getNextResult(void *tree, 
                           time_t currentTime,
                           stateNode **resultState, 
                           unsigned int *actionStateIndex,
                           unsigned int *resultCount,
                           unsigned int *resultFrameOffset, 
                           actionStateNode **resultAction);



