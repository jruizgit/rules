
#define HASH_ID 926444256
#define HASH_SID 3593476751
#define UNDEFINED_INDEX 0xFFFFFFFF
#define SID_BUFFER_LENGTH 2
#define ID_BUFFER_LENGTH 22

#define MESSAGE_TYPE_EVENT 0
#define MESSAGE_TYPE_FACT 1

#define UNDEFINED_HASH_OFFSET 0
#define MAX_OBJECT_PROPERTIES 32
#define MAX_MESSAGE_FRAMES 16
#define MAX_MESSAGE_INDEX_LENGTH 512
#define MAX_LEFT_FRAME_INDEX_LENGTH 512
#define MAX_RIGHT_FRAME_INDEX_LENGTH 512
#define MAX_LOCATION_INDEX_LENGTH 16

#define LEFT_FRAME 0
#define RIGHT_FRAME 1
#define ACTION_FRAME 2


#define MESSAGE_NODE(state, offset) &((messageNode *)state->messagePool.content)[offset]

#define RIGHT_FRAME_NODE(state, index, offset) &((rightFrameNode *)state->betaState[index].rightFramePool.content)[offset]

#define LEFT_FRAME_NODE(state, index, offset) &((leftFrameNode *)state->betaState[index].leftFramePool.content)[offset]

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
        long i; 
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
    unsigned int offset;
    unsigned int prevOffset;
    unsigned int nextOffset;
    unsigned int factOffset;
    unsigned int hash;
    unsigned char isActive;
    unsigned int bindingIndex;
    pool messagePool;
    unsigned int messageIndex[MAX_MESSAGE_INDEX_LENGTH * 2];
    betaStateNode *betaState;
    actionStateNode *actionState; 
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

unsigned int deleteMessage(stateNode *state,
                           unsigned int messageNodeOffset);

unsigned int getMessage(stateNode *state,
                        char *mid,
                        unsigned int *valueOffset);

unsigned int storeMessage(stateNode *state,
                          char *mid,
                          jsonObject *message,
                          unsigned char messageType,
                          unsigned int *valueOffset);

unsigned int ensureStateNode(void *tree, 
                             char *sid, 
                             unsigned char *isNew,
                             stateNode **state);

unsigned int serializeResult(void *tree, 
                             stateNode *state, 
                             actionStateNode *actionNode, 
                             unsigned int count,
                             char **result);

unsigned int serializeState(stateNode *state, 
                            char **stateFact);

unsigned int getNextResultInState(void *tree, 
                                  stateNode *state,
                                  unsigned int *actionStateIndex,
                                  unsigned int *resultCount,
                                  unsigned int *resultFrameOffset, 
                                  actionStateNode **resultAction);

unsigned int getNextResult(void *tree, 
                           stateNode **resultState, 
                           unsigned int *actionStateIndex,
                           unsigned int *resultCount,
                           unsigned int *resultFrameOffset, 
                           actionStateNode **resultAction);



