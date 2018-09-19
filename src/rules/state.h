
#define HASH_ID 926444256
#define HASH_SID 3593476751
#define UNDEFINED_INDEX 0xFFFFFFFF
#define MAX_NAME_LENGTH 256
#define SID_BUFFER_LENGTH 2
#define ID_BUFFER_LENGTH 22
#define UNDEFINED_HASH_OFFSET 0xFFFFFFFF

#define JSON_OBJECT_SEQUENCED 1
#define JSON_OBJECT_HASHED 2

#define MAX_OBJECT_PROPERTIES 128

typedef struct jsonProperty {
    unsigned int hash;
    unsigned char type;
    unsigned char isMaterial;
    char *valueString;
    unsigned short valueLength;
    char name[MAX_NAME_LENGTH];
    unsigned short nameLength;
    union {
        long i; 
        double d; 
        unsigned char b; 
    } value;
} jsonProperty;

typedef struct jsonObject {
    jsonProperty properties[MAX_OBJECT_PROPERTIES];
    unsigned char propertiesLength; 
    unsigned int idIndex; 
    unsigned int sidIndex;
    char sidBuffer[SID_BUFFER_LENGTH];
    char idBuffer[ID_BUFFER_LENGTH];
} jsonObject;

typedef struct stateEntry {
    unsigned int nextHashOffset;
    unsigned int nextLruOffset;
    unsigned int prevLruOffset;
    unsigned int sidHash;
    unsigned int bindingIndex;
    unsigned int lastRefresh;
    char *state;
    char *sid;
    jsonObject jo;
} stateEntry;

unsigned int fnv1Hash32(char *str, unsigned int len);

void rehydrateProperty(jsonProperty *property, char *state);

unsigned int refreshState(void *tree, char *sid);

unsigned int constructObject(char *root,
                             char *parentName, 
                             char *object,
                             char layout,
                             char generateId,
                             jsonObject *jo,
                             char **next);

unsigned int resolveBinding(void *tree, 
                            char *sid, 
                            void **rulesBinding);

unsigned int fetchStateProperty(void *tree,
                                char *sid, 
                                unsigned int propertyHash, 
                                unsigned int maxTime, 
                                unsigned char ignoreStaleState,
                                char **state,
                                jsonProperty **property);

unsigned int getStateVersion(void *tree, 
                             char *sid, 
                             unsigned long *stateVersion);