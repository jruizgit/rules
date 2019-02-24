
#define HASH_ID 926444256
#define HASH_SID 3593476751
#define UNDEFINED_INDEX 0xFFFFFFFF
#define SID_BUFFER_LENGTH 2
#define ID_BUFFER_LENGTH 22

#define UNDEFINED_HASH_OFFSET 0
#define MAX_OBJECT_PROPERTIES 64
#define MAX_MESSAGE_FRAMES 32
#define MAX_MESSAGE_INDEX_LENGTH 16384
#define MAX_LEFT_FRAME_INDEX_LENGTH 1024
#define MAX_RIGHT_FRAME_INDEX_LENGTH 1024
#define MAX_FRAME_LOCATIONS 128

#define LEFT_FRAME 0
#define RIGHT_FRAME 1
#define ACTION_FRAME 2

#define MESSAGE_NODE(state, offset) &((messageNode *)state->messagePool.content)[offset]

#define RIGHT_FRAME_NODE(state, index, offset) &((rightFrameNode *)state->betaState[index].rightFramePool.content)[offset]

#define LEFT_FRAME_NODE(state, index, offset) &((leftFrameNode *)state->betaState[index].leftFramePool.content)[offset]

#define ACTION_FRAME_NODE(state, index, offset) &((leftFrameNode *)state->actionState[index].resultPool.content)[offset]

#define RESULT_FRAME(actionNode, offset) &((leftFrameNode *)actionNode->resultPool.content)[offset];

#define STATE_NODE(tree, offset) &((stateNode *)((ruleset *)tree)->statePool.content)[offset]


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

typedef struct messageNode {
    unsigned int prevOffset;
    unsigned int nextOffset;
    unsigned int hash;
    unsigned short locationCount;
    frameLocation locations[MAX_FRAME_LOCATIONS];
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
    unsigned short messageCount;
    unsigned short reverseIndex[MAX_MESSAGE_FRAMES];
    messageFrame messages[MAX_MESSAGE_FRAMES];
} leftFrameNode;

typedef struct rightFrameNode {
    unsigned int prevOffset;
    unsigned int nextOffset;
    unsigned int hash;
    unsigned int messageOffset;
} rightFrameNode;

typedef struct pool {
    void *content;
    unsigned int freeOffset;
    unsigned int contentLength;
    unsigned int count;
} pool;

typedef struct actionStateNode {
    pool resultPool;
    unsigned int resultIndex[1];
    unsigned short count;
    unsigned short cap;
} actionStateNode;

typedef struct betaStateNode {
    pool leftFramePool;
    unsigned int leftFrameIndex[MAX_LEFT_FRAME_INDEX_LENGTH];
    pool rightFramePool;
    unsigned int rightFrameIndex[MAX_RIGHT_FRAME_INDEX_LENGTH];
} betaStateNode;

typedef struct stateNode {
    unsigned int prevOffset;
    unsigned int nextOffset;
    unsigned int hash;
    unsigned int bindingIndex;
    unsigned int factOffset;
    pool messagePool;
    unsigned int messageIndex[MAX_MESSAGE_INDEX_LENGTH];
    betaStateNode *betaState;
    actionStateNode *actionState; 
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

unsigned int appendFrameLocation(stateNode *state,
                                 frameLocation location,
                                 unsigned int messageNodeOffset);

unsigned int getMessageFromFrame(stateNode *state,
                                 messageFrame *messages,
                                 unsigned int hash,
                                 jsonObject **message);

unsigned int setMessageInFrame(leftFrameNode *node,
                               unsigned int nameOffset,
                               unsigned int hash, 
                               unsigned int messageNodeOffset);

unsigned int getLeftFrame(stateNode *state,
                          unsigned int index, 
                          unsigned int hash,
                          leftFrameNode **node);

unsigned int setLeftFrame(stateNode *state,
                          unsigned int hash, 
                          frameLocation location);


unsigned int deleteLeftFrame(stateNode *state,
                             frameLocation location);

unsigned int createLeftFrame(stateNode *state,
                            unsigned int index, 
                            unsigned int nameOffset,
                            leftFrameNode *oldNode,                        
                            leftFrameNode **newNode,
                            frameLocation *newLocation);

unsigned int getRightFrame(stateNode *state,
                           unsigned int index, 
                           unsigned int hash,
                           rightFrameNode **node);

unsigned int setRightFrame(stateNode *state,
                           unsigned int hash, 
                           frameLocation location);

unsigned int deleteRightFrame(stateNode *state,
                              frameLocation location);

unsigned int createRightFrame(stateNode *state,
                              unsigned int index,
                              rightFrameNode **node,
                              frameLocation *location);

unsigned int setActionFrame(stateNode *state, 
                            frameLocation location);


unsigned int deleteActionFrame(stateNode *state,
                               frameLocation location);

unsigned int createActionFrame(stateNode *state,
                               unsigned int index,
                               unsigned int nameOffset, 
                               leftFrameNode *oldNode,                        
                               leftFrameNode **newNode,
                               frameLocation *newLocation);

unsigned int deleteLocationFromMessage(stateNode *state,
                                       unsigned int messageNodeOffset,
                                       frameLocation location);

unsigned int deleteMessage(stateNode *state,
                           unsigned int messageNodeOffset);

unsigned int storeMessage(stateNode *state,
                          char *mid,
                          jsonObject *message,
                          unsigned int *valueOffset);

unsigned int ensureStateNode(void *tree, 
                             char *sid, 
                             unsigned char *isNew,
                             stateNode **state);

unsigned int serializeResult(void *tree, 
                             stateNode *state, 
                             actionStateNode *actionNode, 
                             char **result);

unsigned int serializeState(stateNode *state, 
                            char **stateFact);

unsigned int getNextResult(void *tree, 
                           stateNode **resultState, 
                           unsigned int *actionIndex,
                           actionStateNode **resultAction);



