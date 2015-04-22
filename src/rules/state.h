
#define HASH_ID 5863474
#define HASH_SID 193505797
#define UNDEFINED_INDEX 0xFFFFFFFF
#define MAX_STATE_PROPERTIES 64
#define UNDEFINED_HASH_OFFSET 0xFFFFFFFF

typedef struct jsonProperty {
    unsigned int hash;
    unsigned char type;
    unsigned char isMaterial;
    unsigned short valueOffset;
    unsigned short valueLength;
    unsigned short nameOffset;
    unsigned short nameLength;
    union {
        long i; 
        double d; 
        unsigned char b; 
    } value;
} jsonProperty;

typedef struct stateEntry {
    unsigned int nextHashOffset;
    unsigned int nextLruOffset;
    unsigned int prevLruOffset;
    unsigned int sidHash;
    unsigned int bindingIndex;
    unsigned int lastRefresh;
    unsigned int propertiesLength;
    jsonProperty properties[MAX_STATE_PROPERTIES];
    char *state;
    char *sid;
} stateEntry;

unsigned int djbHash(char *str, unsigned int len);
void rehydrateProperty(jsonProperty *property, char *state);
unsigned int refreshState(void *tree, char *sid);
unsigned int constructObject(char *parentName, 
                             char *object,
                             char createHashtable,
                             unsigned int maxProperties,
                             jsonProperty *properties, 
                             unsigned int *propertiesLength, 
                             unsigned int *midIndex, 
                             unsigned int *sidIndex,
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
