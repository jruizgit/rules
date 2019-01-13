
#define HASH_ID 926444256
#define HASH_SID 3593476751
#define UNDEFINED_INDEX 0xFFFFFFFF
#define SID_BUFFER_LENGTH 2
#define ID_BUFFER_LENGTH 22

#define UNDEFINED_HASH_OFFSET 0
#define MAX_OBJECT_PROPERTIES 64
#define MAX_MESSAGE_FRAMES 32

#define MESSAGE_NODE(tree, offset) &((messageNode *)((ruleset *)tree)->messagePool.content)[offset]

#define RIGHT_FRAME_NODE(tree, offset) &((rightFrameNode *)((ruleset *)tree)->rightFramePool.content)[offset]

#define LEFT_FRAME_NODE(tree, offset) &((leftFrameNode *)((ruleset *)tree)->leftFramePool.content)[offset]

typedef struct jsonProperty {
    unsigned int hash;
    unsigned char type;
    unsigned short parentProperty;
    unsigned short valueOffset;
    unsigned short valueLength;
    unsigned short nameOffset;
    unsigned short nameLength;
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
    unsigned char propertyIndex[MAX_OBJECT_PROPERTIES];
    unsigned char propertiesLength; 
    unsigned int idIndex; 
    unsigned int sidIndex;
    char sidBuffer[SID_BUFFER_LENGTH];
    char idBuffer[ID_BUFFER_LENGTH];
} jsonObject;

typedef struct messageNode {
    unsigned int prevOffset;
    unsigned int nextOffset;
    unsigned int hash;
    jsonObject jo;
} messageNode;

typedef struct stateNode {
    unsigned int prevOffset;
    unsigned int nextOffset;
    unsigned int hash;
    unsigned int bindingIndex;
} stateNode;

typedef struct messageFrame {
    unsigned int hash;
    unsigned int messageNodeOffset;
} messageFrame;

typedef struct leftFrameNode {
    unsigned int prevOffset;
    unsigned int nextOffset;
    unsigned int hash;
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
} pool;

unsigned int fnv1Hash32(char *str, unsigned int len);

unsigned int getObjectProperty(jsonObject *jo, 
                               unsigned int hash, 
                               jsonProperty **property);

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

unsigned int initMessagePool(void *tree);

unsigned int initLeftFramePool(void *tree);

unsigned int initRightFramePool(void *tree);

unsigned int getMessageFromFrame(void *tree,
                                 messageFrame *messages,
                                 unsigned int hash,
                                 jsonObject **message);

unsigned int setMessageInFrame(messageFrame *messages,
                               unsigned int hash, 
                               unsigned int messageNodeOffset);

unsigned int getLeftFrame(void *tree, 
                          unsigned int *index, 
                          unsigned int hash,
                          leftFrameNode **node);

unsigned int setLeftFrame(void *tree, 
                          unsigned int *index, 
                          unsigned int hash, 
                          unsigned int valueOffset);

unsigned int createLeftFrame(void *tree, 
                             unsigned int *valueOffset,
                             leftFrameNode **node);

unsigned int cloneLeftFrame(void *tree, 
                            leftFrameNode *oldNode,                        
                            unsigned int *newValueOffset,
                            leftFrameNode **newNode);

unsigned int getRightFrame(void *tree, 
                           unsigned int *index, 
                           unsigned int hash,
                           rightFrameNode **node);

unsigned int setRightFrame(void *tree, 
                           unsigned int *index, 
                           unsigned int hash, 
                           unsigned int valueOffset);

unsigned int createRightFrame(void *tree, 
                              unsigned int *valueOffset,
                              rightFrameNode **node);

unsigned int storeMessage(void *tree,
                          char *sid,
                          char *mid,
                          jsonObject *message,
                          unsigned int *valueOffset);

