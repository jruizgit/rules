
#define ID_HASH 5863474
#define SID_HASH 193505797
#define UNDEFINED_INDEX 0xFFFFFFFF
#define MAX_STATE_ENTRIES 1024
#define MAX_CONFIG_PROPERTIES 256
#define MAX_STATE_PROPERTIES 128

typedef struct jsonProperty {
    unsigned int hash;
    unsigned char type;
    char *firstValue;
    char *lastValue;
    unsigned char isMaterial;
    union { 
        long i; 
        double d; 
        unsigned char b; 
    } value;
} jsonProperty;

typedef struct stateEntry {
    unsigned int sidHash;
    unsigned int bindingIndex;
    unsigned int lastRefresh;
    unsigned int propertiesLength;
    jsonProperty properties[MAX_CONFIG_PROPERTIES];
    char *state;
} stateEntry;

unsigned int constructObject(char *parentName, 
                             char *object,
                             char createHashtable,
                             unsigned int maxProperties,
                             jsonProperty *properties, 
                             unsigned int *propertiesLength, 
                             unsigned int *midIndex, 
                             unsigned int *sidIndex,
                             char **next);
void rehydrateProperty(jsonProperty *property);
unsigned int resolveBinding(void *tree, char *sid, void **rulesBinding);
unsigned int refreshGlobalState(void *tree);
unsigned int refreshState(void *tree, char *sid, void **rulesBinding);
unsigned int fetchGlobalStateProperty(void *tree, 
                                       unsigned int propertyHash,
                                       unsigned int maxTime,
                                       unsigned char ignoreStaleState,
                                       jsonProperty **property);
unsigned int fetchStateProperty(void *tree,
                                      char *sid, 
                                      unsigned int propertyHash, 
                                      unsigned int maxTime, 
                                      unsigned char ignoreStaleState,
                                      jsonProperty **property);
