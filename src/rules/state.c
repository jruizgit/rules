#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "rules.h"
#include "json.h"
#include "net.h"


#define INIT(type, pool, length) do { \
    pool.content = malloc(length * sizeof(type)); \
    if (!pool.content) { \
        return ERR_OUT_OF_MEMORY; \
    } \
    pool.contentLength = length; \
    for (unsigned int i = 0; i < pool.contentLength; ++ i) { \
        ((type *)pool.content)[i].nextOffset = i + 1; \
        ((type *)pool.content)[i].prevOffset = i - 1; \
    } \
    pool.freeOffset = 0; \
} while(0)

// Index offset by one  in type *current = &pool.content[offset - 1];
#define GET(type, index, max, pool, nodeHash, value) do { \
    unsigned int offset = index[nodeHash % max]; \
    value = NULL; \
    printf("%d\n", offset); \
    while (offset != UNDEFINED_HASH_OFFSET) { \
        type *current = &pool.content[offset - 1]; \
        if (current->hash != nodeHash) { \
            offset = current->nextOffset; \
        } else { \
            value = current; \
            offset = UNDEFINED_HASH_OFFSET; \
        } \
    } \
} while(0)

// Index offset by one in last two lines of macro
#define NEW(type, index, max, pool, nodeHash, value) do { \
    unsigned int valueOffset = pool.freeOffset; \
    value = &pool.content[valueOffset]; \
    if (value->nextOffset == UNDEFINED_HASH_OFFSET) { \
        pool.content = realloc(pool.content, (pool.contentLength * 2) * sizeof(type)); \
        if (!pool.content) { \
            return ERR_OUT_OF_MEMORY; \
        } \
        for (unsigned int i = pool.contentLength; i < pool.contentLength * 2; ++ i) { \
            ((type *)pool.content)[i].nextOffset = i + 1; \
            ((type *)pool.content)[i].prevOffset = i - 1; \
        } \
        value->nextOffset = pool.contentLength; \
        ((type *)pool.content)[pool.contentLength].prevOffset = pool.freeOffset; \
        pool.contentLength *= 2; \
        ((type *)pool.content)[pool.contentLength - 1].nextOffset = UNDEFINED_HASH_OFFSET; \
    } \
    pool.freeOffset = value->nextOffset; \
    value->prevOffset = UNDEFINED_HASH_OFFSET; \
    value->hash = nodeHash; \
    value->nextOffset = index[nodeHash % max] - 1; \
    index[nodeHash % max] = valueOffset + 1; \
} while(0)


#define DELETE(type, index, max, pool, nodeHash) do { \
    unsigned int offset = index[nodeHash % max]; \
    while (offset != UNDEFINED_HASH_OFFSET) { \
        type *current = &pool.content[offset]; \
        if (current->hash != nodeHash) { \
            offset = current->nextOffset; \
        } else { \
            if (current->nextOffset != UNDEFINED_HASH_OFFSET) {\
                ((type *)pool.content)[current->nextOffset].prevOffset = current->prevOffset; \
            } \
            if (current->prevOffset != UNDEFINED_HASH_OFFSET) {\
                ((type *)pool.content)[current->prevOffset].nextOffset = current->nextOffset; \
            } \
            current->prevOffset = UNDEFINED_HASH_OFFSET; \
            current->nextOffset = pool.freeOffset; \
            pool.freeOffset = offset; \
            offset = UNDEFINED_HASH_OFFSET; \
        } \
    } \
} while(0)

unsigned int fnv1Hash32(char *str, unsigned int length) {
    unsigned int hash = FNV_32_OFFSET_BASIS;
    for(unsigned int i = 0; i < length; str++, i++) {
        hash ^= (*str);
        hash *= FNV_32_PRIME;
    }
    return hash;
}

unsigned int initStatePool(void *tree, unsigned int length) {
    INIT(stateNode, ((ruleset*)tree)->statePool, length);
    return RULES_OK;
}

unsigned int initMessagePool(void *tree, unsigned int length) {
    INIT(messageNode, ((ruleset*)tree)->messagePool, length);
    return RULES_OK;
}

unsigned int initLeftFramePool(void *tree, unsigned int length) {
    INIT(leftFrameNode, ((ruleset*)tree)->leftFramePool, length);
    return RULES_OK;
}

unsigned int initRightFramePool(void *tree, unsigned int length) {
    INIT(rightFrameNode, ((ruleset*)tree)->rightFramePool, length);
    return RULES_OK;
}


static void insertSortProperties(jsonObject *jo, jsonProperty **properties) {
    for (unsigned short i = 1; i < jo->propertiesLength; ++i) {
        unsigned short ii = i; 
        while (properties[ii]->hash < properties[ii - 1]->hash) {
            jsonProperty *temp = properties[ii];
            properties[ii] = properties[ii - 1];
            properties[ii - 1] = temp;
            --ii;
        }
    }
}

static void radixSortProperties(jsonObject *jo, jsonProperty **properties) {
    unsigned char counts[43];
    memset(counts, 0, 43 * sizeof(char));

    for (unsigned char i = 0; i < jo->propertiesLength; ++i) {
        unsigned char mostSignificant = jo->properties[i].hash / 100000000;
        ++counts[mostSignificant];
    }

    unsigned char previousCount = 0;
    for (unsigned char i = 0; i < 43; ++i) {
        unsigned char nextCount = counts[i] + previousCount;
        counts[i] = previousCount;
        previousCount = nextCount;
    }

    for (unsigned char i = 0; i < jo->propertiesLength; ++i) {
        unsigned char mostSignificant = jo->properties[i].hash / 100000000;
        properties[counts[mostSignificant]] = &jo->properties[i];
        ++counts[mostSignificant];
    }
}

static void calculateId(jsonObject *jo) {

#ifdef _WIN32
    jsonProperty **properties = (jsonProperty *)_alloca(sizeof(jsonProperty *) * (jo->propertiesLength));
#else
    jsonProperty *properties[jo->propertiesLength];
#endif

    radixSortProperties(jo, properties);
    insertSortProperties(jo, properties);

    unsigned long long hash = FNV_64_OFFSET_BASIS;
    for (unsigned short i = 0; i < jo->propertiesLength; ++i) {
        jsonProperty *property = properties[i];
        for (unsigned short ii = 0; ii < property->nameLength; ++ii) {
            hash ^= jo->content[property->nameOffset + ii];
            hash *= FNV_64_PRIME;
        }

        unsigned short valueLength = property->valueLength;
        if (property->type != JSON_STRING) {
            ++valueLength;
        }

        for (unsigned short ii = 0; ii < valueLength; ++ii) {
            hash ^= jo->content[property->valueOffset + ii];
            hash *= FNV_64_PRIME;
        }
    }

#ifdef _WIN32
    sprintf_s(jo->idBuffer, sizeof(char) * ID_BUFFER_LENGTH, "$%020llu", hash); 
#else
    snprintf(jo->idBuffer, sizeof(char) * ID_BUFFER_LENGTH, "$%020llu", hash);
#endif
    jo->properties[jo->idIndex].valueLength = 20;
}

static unsigned int fixupIds(jsonObject *jo, char generateId) {
    jsonProperty *property;
    // id and sid are coerced to strings
    // to avoid unnecessary conversions
    if (jo->sidIndex != UNDEFINED_INDEX && jo->properties[jo->sidIndex].type != JSON_NIL) {
        //coerce value to string
        property = &jo->properties[jo->sidIndex];
        if (property->type != JSON_STRING) {
            ++property->valueLength;
        }

        property->type = JSON_STRING;
    } else {
        property = &jo->properties[jo->propertiesLength];
        jo->sidIndex = jo->propertiesLength;
        ++jo->propertiesLength;
        if (jo->propertiesLength == MAX_OBJECT_PROPERTIES) {
            return ERR_EVENT_MAX_PROPERTIES;
        }

        strncpy(jo->sidBuffer, "0", 1);
        property->hash = HASH_SID;
        property->valueOffset = 0;
        property->valueLength = 1;
        property->nameLength = 0;
        property->nameOffset = 0;
        property->type = JSON_STRING;
    } 

    if (jo->idIndex != UNDEFINED_INDEX && jo->properties[jo->idIndex].type != JSON_NIL) {
        //coerce value to string
        property = &jo->properties[jo->idIndex];
        if (property->type != JSON_STRING) {
            ++property->valueLength;
        }

        property->type = JSON_STRING;
    } else {
        property = &jo->properties[jo->propertiesLength];
        jo->idIndex = jo->propertiesLength;
        ++jo->propertiesLength;
        if (jo->propertiesLength == MAX_OBJECT_PROPERTIES) {
            return ERR_EVENT_MAX_PROPERTIES;
        }

        jo->idBuffer[0] = 0;
        property->hash = HASH_ID;
        property->valueOffset = 0;
        property->valueLength = 0;
        property->nameLength = 0;
        property->nameOffset = 0;
        property->type = JSON_STRING;
        if (generateId) {
            calculateId(jo);
        }
    }

    return RULES_OK;
}

unsigned int getObjectProperty(jsonObject *jo, unsigned int hash, jsonProperty **property) {
    unsigned int candidate = hash % MAX_OBJECT_PROPERTIES;
    for (unsigned short i = 0; i < MAX_OBJECT_PROPERTIES; ++i) {
        unsigned char index = jo->propertyIndex[(candidate + i) % MAX_OBJECT_PROPERTIES];
        if (index == 0) {
            break;
        }

        // Property index offset by 1
        if (jo->properties[index - 1].hash == hash) {
            *property = &jo->properties[index - 1];   
            return RULES_OK;
        }
    }
    return ERR_PROPERTY_NOT_FOUND;
}

unsigned int constructObject(char *root,
                             char *parentName, 
                             char *object,
                             char generateId,
                             jsonObject *jo,
                             char **next) {
    char *firstName;
    char *lastName;
    char *first;
    char *last;
    char temp;
    unsigned char type;
    unsigned int hash;
    int parentNameLength;
    if (parentName) {
        parentNameLength = strlen(parentName);
    } else {
        parentNameLength = 0;
        jo->idIndex = UNDEFINED_INDEX;
        jo->sidIndex = UNDEFINED_INDEX;
        jo->propertiesLength = 0;
        jo->content = root;
        memset(jo->propertyIndex, 0, MAX_OBJECT_PROPERTIES * sizeof(unsigned char));
    }

    object = (object ? object : root);
    unsigned int result = readNextName(object, &firstName, &lastName, &hash);
    while (result == PARSE_OK) {
        result = readNextValue(lastName, &first, &last, &type);
        if (result != PARSE_OK) {
            return result;
        }

        jsonProperty *property = NULL;
        if (type != JSON_OBJECT) {
            unsigned int candidate = hash % MAX_OBJECT_PROPERTIES;
            while (jo->propertyIndex[candidate] != 0) {
                candidate = (candidate + 1) % MAX_OBJECT_PROPERTIES;
            }

            // Property index offset by 1
            jo->propertyIndex[candidate] = jo->propertiesLength + 1;
            property = &jo->properties[jo->propertiesLength]; 
            ++jo->propertiesLength;
            if (jo->propertiesLength == MAX_OBJECT_PROPERTIES) {
                return ERR_EVENT_MAX_PROPERTIES;
            } 

            if (!parentName) {
                if (hash == HASH_ID) {
                    jo->idIndex = candidate;
                } else if (hash == HASH_SID) {
                    jo->sidIndex = candidate;
                }
            }

            property->valueOffset = first - root;
            property->valueLength = last - first + 1;
            property->type = type;

            switch(type) {
                case JSON_INT:
                    temp = first[property->valueLength];
                    first[property->valueLength] = '\0';
                    property->value.i = atol(first);
                    first[property->valueLength] = temp;
                    break;
                case JSON_DOUBLE:
                    temp = first[property->valueLength];
                    first[property->valueLength] = '\0';
                    property->value.d = atof(first);
                    first[property->valueLength] = temp;
                    break;
                case JSON_BOOL:
                    if (property->valueLength == 4 && strncmp("true", first, 4) == 0) {
                        property->value.b = 1;
                    } else {
                        property->value.b = 0;
                    }

                    break;
                case JSON_STRING:
                    property->value.s = first;
                    property->valueLength = property->valueLength - 1;
                    break;
            }

        }
        
        if (!parentName) {
            int nameLength = lastName - firstName;
            if (type != JSON_OBJECT) {
                property->nameOffset = firstName - root;
                property->nameLength = nameLength;
                property->hash = hash;
            } else {            
#ifdef _WIN32
                char *newParent = (char *)_alloca(sizeof(char)*(nameLength + 1));
#else
                char newParent[nameLength + 1];
#endif
                strncpy(newParent, firstName, nameLength);
                newParent[nameLength] = '\0';
                result = constructObject(root,
                                         newParent, 
                                         first,
                                         0, 
                                         jo,
                                         next);
                if (result != RULES_OK) {
                    return result;
                }
            }
        } else {
            int nameLength = lastName - firstName;
            int fullNameLength = nameLength + parentNameLength + 1;
#ifdef _WIN32
            char *fullName = (char *)_alloca(sizeof(char)*(fullNameLength + 1));
#else
            char fullName[fullNameLength + 1];
#endif
            strncpy(fullName, parentName, parentNameLength);
            fullName[parentNameLength] = '.';
            strncpy(&fullName[parentNameLength + 1], firstName, nameLength);
            fullName[fullNameLength] = '\0';
            if (type != JSON_OBJECT) {
                property->nameOffset = firstName - root;
                property->nameLength = nameLength;
                property->hash = fnv1Hash32(fullName, fullNameLength);
            } else {

                result = constructObject(root,
                                         fullName, 
                                         first,
                                         0, 
                                         jo, 
                                         next);
                if (result != RULES_OK) {
                    return result;
                }
            }
        }
        
        *next = last;
        result = readNextName(last, &firstName, &lastName, &hash);
    }
 
    if (!parentName) {
        int idResult = fixupIds(jo, generateId);
        if (idResult != RULES_OK) {
            return idResult;
        }
    }

    return (result == PARSE_END ? RULES_OK: result);
}

unsigned int resolveBinding(void *tree, 
                            char *sid, 
                            void **rulesBinding) {  
    printf("resolveBinding\n");
    unsigned int sidHash = fnv1Hash32(sid, strlen(sid));
    stateNode *node;
    GET(stateNode, ((ruleset*)tree)->stateIndex, MAX_STATE_INDEX_LENGTH, ((ruleset*)tree)->statePool, sidHash, node);
    if (!node) {
        printf("newNode\n");
        NEW(stateNode, ((ruleset*)tree)->stateIndex, MAX_STATE_INDEX_LENGTH, ((ruleset*)tree)->statePool, sidHash, node);
        unsigned int result = getBindingIndex(tree, sidHash, &node->bindingIndex);
        if (result != RULES_OK) {
            return result;
        }
    }
    
    bindingsList *list = ((ruleset*)tree)->bindingsList;
    *rulesBinding = &list->bindings[node->bindingIndex];
    return RULES_OK;
}

unsigned int getState(unsigned int handle, char *sid, char **state) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    void *rulesBinding = NULL;
    if (!sid) {
        sid = "0";
    }

    unsigned int result = resolveBinding(tree, sid, &rulesBinding);
    if (result != RULES_OK) {
      return result;
    }

    return getSession(rulesBinding, sid, state);
}

unsigned int getStateVersion(void *tree, char *sid, unsigned long *stateVersion) {
    void *rulesBinding = NULL;
    unsigned int result = resolveBinding(tree, sid, &rulesBinding);
    if (result != RULES_OK) {
      return result;
    }

    return getSessionVersion(rulesBinding, sid, stateVersion);
}


unsigned int deleteState(unsigned int handle, char *sid) {
    ruleset *tree;
    RESOLVE_HANDLE(handle, &tree);

    if (!sid) {
        sid = "0";
    }

    void *rulesBinding = NULL;
    unsigned int result = resolveBinding(tree, sid, &rulesBinding);
    if (result != RULES_OK) {
      return result;
    }

    unsigned int sidHash = fnv1Hash32(sid, strlen(sid));
    result = deleteSession(tree, rulesBinding, sid, sidHash);
    if (result != RULES_OK) {
      return result;
    }

    DELETE(stateNode, tree->stateIndex, MAX_STATE_INDEX_LENGTH, tree->statePool, sidHash);
    return RULES_OK;
}
