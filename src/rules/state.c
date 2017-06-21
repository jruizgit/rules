#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "rules.h"
#include "json.h"
#include "net.h"

unsigned int fnv1Hash32(char *str, unsigned int len) {
    unsigned int hash = FNV_32_OFFSET_BASIS;
    for(unsigned int i = 0; i < len; str++, i++) {
        hash ^= (*str);
        hash *= FNV_32_PRIME;
    }
    return hash;
}

static unsigned int evictEntry(ruleset *tree) {
    stateEntry *lruEntry = &tree->state[tree->lruStateOffset];
    unsigned int lruBucket = lruEntry->sidHash % tree->stateBucketsLength;
    unsigned int offset = tree->stateBuckets[lruBucket];
    unsigned int lastOffset = UNDEFINED_HASH_OFFSET;
    unsigned char found = 0;
    while (!found) {
        stateEntry *current = &tree->state[offset];
        if (current->sidHash == lruEntry->sidHash) {
            if (!strcmp(current->sid, lruEntry->sid)) {
                if (lastOffset == UNDEFINED_HASH_OFFSET) {
                    tree->stateBuckets[lruBucket] = current->nextHashOffset;
                } else {
                    tree->state[lastOffset].nextHashOffset = current->nextHashOffset;
                }

                if (current->state) {
                    free(current->state);
                    current->state = NULL;
                }

                free(current->sid);
                current->sid = NULL;
                current->sidHash = 0;
                current->bindingIndex = 0;
                current->lastRefresh = 0;
                current->jo.propertiesLength = 0;
                found = 1;
            }
        }

        lastOffset = offset;
        offset = current->nextHashOffset;
    }

    unsigned int result = tree->lruStateOffset;
    tree->lruStateOffset = lruEntry->nextLruOffset;
    tree->state[tree->lruStateOffset].prevLruOffset = UNDEFINED_HASH_OFFSET;
    return result;
}

static unsigned int addEntry(ruleset *tree, char *sid, unsigned int sidHash) {
    unsigned newOffset;
    if (tree->stateLength == tree->maxStateLength) {
        newOffset = evictEntry(tree);
    } else {
        newOffset = tree->stateLength;
        ++tree->stateLength;
    }

    stateEntry *current = &tree->state[newOffset];
    current->prevLruOffset = UNDEFINED_HASH_OFFSET;
    current->nextLruOffset = UNDEFINED_HASH_OFFSET;
    current->nextHashOffset = UNDEFINED_HASH_OFFSET;
    current->sid = malloc(strlen(sid) + 1);
    strcpy(current->sid, sid);
    current->sidHash = sidHash;
    unsigned int bucket = sidHash % tree->stateBucketsLength;
    unsigned int offset = tree->stateBuckets[bucket];
    if (offset == UNDEFINED_HASH_OFFSET) {
        tree->stateBuckets[bucket] = newOffset;
    }
    else {
        while (1) {
            current = &tree->state[offset];
            if (current->nextHashOffset == UNDEFINED_HASH_OFFSET) {
                current->nextHashOffset = newOffset;
                break;
            }

            offset = current->nextHashOffset;
        }
    }

    return newOffset;
}

static unsigned char ensureEntry(ruleset *tree, char *sid, unsigned int sidHash, stateEntry **result) {
    unsigned int bucket = sidHash % tree->stateBucketsLength;
    unsigned int offset = tree->stateBuckets[bucket];
    stateEntry *current = NULL;
    unsigned char found = 0;
    // find state entry by sid in hash table
    while (offset != UNDEFINED_HASH_OFFSET) {
        current = &tree->state[offset];
        if (current->sidHash == sidHash) {
            if (!strcmp(current->sid, sid)) {
                found = 1;
                break;
            }
        }

        offset = current->nextHashOffset;
    }  

    // create entry if not found
    if (offset == UNDEFINED_HASH_OFFSET) {
        offset = addEntry(tree, sid, sidHash);
        current = &tree->state[offset];
    }

    // remove entry from lru double linked list
    if (current->prevLruOffset != UNDEFINED_HASH_OFFSET) {
        tree->state[current->prevLruOffset].nextLruOffset = current->nextLruOffset;
        if (tree->mruStateOffset == offset) {
            tree->mruStateOffset = current->prevLruOffset;
        }
    }

    if (current->nextLruOffset != UNDEFINED_HASH_OFFSET) {
        tree->state[current->nextLruOffset].prevLruOffset = current->prevLruOffset;
        if (tree->lruStateOffset == offset) {
            tree->lruStateOffset = current->nextLruOffset;
        }
    }

    // attach entry to end of linked list
    current->nextLruOffset = UNDEFINED_HASH_OFFSET;
    current->prevLruOffset = tree->mruStateOffset;
    if (tree->mruStateOffset == UNDEFINED_HASH_OFFSET) {
        tree->lruStateOffset = offset;
    } else {
        tree->state[tree->mruStateOffset].nextLruOffset = offset;
    }
    tree->mruStateOffset = offset;

    *result = current;
    return found;
}

static stateEntry *getEntry(ruleset *tree, char *sid, unsigned int sidHash) {
    unsigned int bucket = sidHash % tree->stateBucketsLength;
    unsigned int offset = tree->stateBuckets[bucket];
    while (offset != UNDEFINED_HASH_OFFSET) {
        stateEntry *current = &tree->state[offset];
        if (current->sidHash == sidHash) {
            if (!strcmp(current->sid, sid)) {
                return current;
            }
        }

        offset = current->nextHashOffset;
    }  
    
    return NULL;
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
            hash ^= property->name[ii];
            hash *= FNV_64_PRIME;
        }

        unsigned short valueLength = property->valueLength;
        if (property->type != JSON_STRING) {
            ++valueLength;
        }

        for (unsigned short ii = 0; ii < valueLength; ++ii) {
            hash ^= property->valueString[ii];
            hash *= FNV_64_PRIME;
        }
    }

#ifdef _WIN32
    sprintf_s(jo->idBuffer, sizeof(char) * 21, "%020llu", hash); 
#else
    snprintf(jo->idBuffer, sizeof(char) * 21, "%020llu", hash);
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
        property->isMaterial = 1;
        property->valueString = jo->sidBuffer;
        property->valueLength = 1;
        strncpy(property->name, "sid", 3);
        property->nameLength = 3;
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
        property->isMaterial = 1;
        property->valueString = jo->idBuffer;
        property->valueLength = 0;
        strncpy(property->name, "id", 2);
        property->nameLength = 2;
        property->type = JSON_STRING;
        if (generateId) {
            calculateId(jo);
        }
    }

    return RULES_OK;
}

unsigned int constructObject(char *root,
                             char *parentName, 
                             char *object,
                             char layout,
                             char generateId,
                             jsonObject *jo,
                             char **next) {
    char *firstName;
    char *lastName;
    char *first;
    char *last;
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
            switch (layout) {
                case JSON_OBJECT_SEQUENCED:
                    property = &jo->properties[jo->propertiesLength];
                    if (hash == HASH_ID) {
                        jo->idIndex = jo->propertiesLength;
                    } else if (hash == HASH_SID) {
                        jo->sidIndex = jo->propertiesLength;
                    }
                    break;
                case JSON_OBJECT_HASHED: 
                {
                    unsigned int candidate = hash % MAX_OBJECT_PROPERTIES;
                    while (jo->properties[candidate].type != 0) {
                        candidate = (candidate + 1) % MAX_OBJECT_PROPERTIES;
                    }

                    if (hash == HASH_ID) {
                        jo->idIndex = candidate;
                    } else if (hash == HASH_SID) {
                        jo->sidIndex = candidate;
                    }

                    property = &jo->properties[candidate];
                }
                break;
            }
            
            ++jo->propertiesLength;
            if (jo->propertiesLength == MAX_OBJECT_PROPERTIES) {
                return ERR_EVENT_MAX_PROPERTIES;
            }

            property->isMaterial = 0;
            property->valueString = first;
            property->valueLength = last - first;
            property->type = type;
        }
        
        if (!parentName) {
            int nameLength = lastName - firstName;
            if (nameLength > MAX_NAME_LENGTH) {
                return ERR_MAX_PROPERTY_NAME_LENGTH;
            }

            if (type != JSON_OBJECT) {
                strncpy(property->name, firstName, nameLength);
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
                                         layout,
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
            if (fullNameLength > MAX_NAME_LENGTH) {
                return ERR_MAX_PROPERTY_NAME_LENGTH;
            }

            if (type != JSON_OBJECT) {
                strncpy(property->name, parentName, parentNameLength);
                property->name[parentNameLength] = '.';
                strncpy(&property->name[parentNameLength + 1], firstName, nameLength);
                property->nameLength = fullNameLength;
                property->hash = fnv1Hash32(property->name, fullNameLength);
            } else {
#ifdef _WIN32
                char *fullName = (char *)_alloca(sizeof(char)*(fullNameLength + 1));
#else
                char fullName[fullNameLength + 1];
#endif
                strncpy(fullName, parentName, parentNameLength);
                fullName[parentNameLength] = '.';
                strncpy(&fullName[parentNameLength + 1], firstName, nameLength);
                fullName[fullNameLength] = '\0';
                result = constructObject(root,
                                         fullName, 
                                         first, 
                                         layout,
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

void rehydrateProperty(jsonProperty *property, char *state) {
    if (!property->isMaterial) {
        unsigned short propertyLength = property->valueLength + 1;
        char *propertyFirst = property->valueString;
        unsigned char propertyType = property->type;
        unsigned char b = 0;
        char temp;

        switch(propertyType) {
            case JSON_INT:
                temp = propertyFirst[propertyLength];
                propertyFirst[propertyLength] = '\0';
                property->value.i = atol(propertyFirst);
                propertyFirst[propertyLength] = temp;
                break;
            case JSON_DOUBLE:
                temp = propertyFirst[propertyLength];
                propertyFirst[propertyLength] = '\0';
                property->value.d = atof(propertyFirst);
                propertyFirst[propertyLength] = temp;
                break;
            case JSON_BOOL:
                if (propertyLength == 4 && strncmp("true", propertyFirst, 4) == 0) {
                    b = 1;
                }

                property->value.b = b;
                break;
        }

        property->isMaterial = 1;
    }
}

static unsigned int resolveBindingAndEntry(ruleset *tree, 
                                          char *sid, 
                                          stateEntry **entry,
                                          void **rulesBinding) {   
    unsigned int sidHash = fnv1Hash32(sid, strlen(sid));
    if (!ensureEntry(tree, sid, sidHash, entry)) {
        unsigned int result = getBindingIndex(tree, sidHash, &(*entry)->bindingIndex);
        if (result != RULES_OK) {
            return result;
        }
    }
    
    bindingsList *list = tree->bindingsList;
    *rulesBinding = &list->bindings[(*entry)->bindingIndex];
    return RULES_OK;
}

unsigned int resolveBinding(void *handle, 
                            char *sid, 
                            void **rulesBinding) {  
    stateEntry *entry = NULL;
    return resolveBindingAndEntry(handle, sid, &entry, rulesBinding);
}

unsigned int refreshState(void *handle, 
                          char *sid) {
    unsigned int result;
    stateEntry *entry = NULL;  
    void *rulesBinding;
    result = resolveBindingAndEntry(handle, sid, &entry, &rulesBinding);
    if (result != RULES_OK) {
        return result;
    }

    if (entry->state) {
        free(entry->state);
        entry->state = NULL;
    }

    result = getSession(rulesBinding, sid, &entry->state);
    if (result != RULES_OK) {
        if (result == ERR_NEW_SESSION) {
            entry->lastRefresh = time(NULL);    
        }
        return result;
    }

    memset(entry->jo.properties, 0, MAX_OBJECT_PROPERTIES * sizeof(jsonProperty));
    entry->jo.propertiesLength = 0;
    char *next;
    result =  constructObject(entry->state,
                             NULL, 
                             NULL, 
                             JSON_OBJECT_HASHED, 
                             0,
                             &entry->jo, 
                             &next);
    if (result != RULES_OK) {
        return result;
    }

    entry->lastRefresh = time(NULL);
    return RULES_OK;
}

unsigned int fetchStateProperty(void *tree,
                                char *sid, 
                                unsigned int propertyHash, 
                                unsigned int maxTime, 
                                unsigned char ignoreStaleState,
                                char **state,
                                jsonProperty **property) {
    unsigned int sidHash = fnv1Hash32(sid, strlen(sid));
    stateEntry *entry = getEntry(tree, sid, sidHash);
    if (entry == NULL || entry->lastRefresh == 0) {
        return ERR_STATE_NOT_LOADED;
    }
    
    if (!ignoreStaleState && (time(NULL) - entry->lastRefresh > maxTime)) {
        return ERR_STALE_STATE;
    }

    unsigned int propertyIndex = propertyHash % MAX_OBJECT_PROPERTIES;
    jsonProperty *result = &entry->jo.properties[propertyIndex];
    while (result->type != 0 && result->hash != propertyHash) {
        propertyIndex = (propertyIndex + 1) % MAX_OBJECT_PROPERTIES;  
        result = &entry->jo.properties[propertyIndex];   
    }

    if (!result->type) {
        return ERR_PROPERTY_NOT_FOUND;
    }

    *state = entry->state;
    rehydrateProperty(result, *state);
    *property = result;
    return RULES_OK;    
}

unsigned int getState(void *handle, char *sid, char **state) {
    void *rulesBinding = NULL;
    if (!sid) {
        sid = "0";
    }

    unsigned int result = resolveBinding(handle, sid, &rulesBinding);
    if (result != RULES_OK) {
      return result;
    }

    return getSession(rulesBinding, sid, state);
}

unsigned int getStateVersion(void *handle, char *sid, unsigned long *stateVersion) {
    void *rulesBinding = NULL;
    unsigned int result = resolveBinding(handle, sid, &rulesBinding);
    if (result != RULES_OK) {
      return result;
    }

    return getSessionVersion(rulesBinding, sid, stateVersion);
}


unsigned int deleteState(void *handle, char *sid) {
    if (!sid) {
        sid = "0";
    }

    void *rulesBinding = NULL;
    unsigned int result = resolveBinding(handle, sid, &rulesBinding);
    if (result != RULES_OK) {
      return result;
    }

    unsigned int sidHash = fnv1Hash32(sid, strlen(sid));
    return deleteSession(handle, rulesBinding, sid, sidHash);
}
