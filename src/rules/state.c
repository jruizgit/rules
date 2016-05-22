#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "rules.h"
#include "json.h"
#include "net.h"

unsigned int djbHash(char *str, unsigned int len) {
   unsigned int hash = 5381;
   unsigned int i = 0;

   for(i = 0; i < len; str++, i++) {
      hash = ((hash << 5) + hash) + (*str);
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
                current->propertiesLength = 0;
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

unsigned int constructObject(char *parentName, 
                             char *object,
                             char createHashtable,
                             unsigned int maxProperties,
                             jsonProperty *properties, 
                             unsigned int *propertiesLength, 
                             unsigned int *midIndex, 
                             unsigned int *sidIndex,
                             char **next) {
    char *firstName;
    char *lastName;
    char *first;
    char *last;
    unsigned char type;
    unsigned int hash;
    int parentNameLength = (parentName ? strlen(parentName): 0);
    unsigned int result = readNextName(object, &firstName, &lastName, &hash);
    while (result == PARSE_OK) {
        result = readNextValue(lastName, &first, &last, &type);
        if (result != PARSE_OK) {
            return result;
        }
        
        if (!parentName) {
            if (type == JSON_OBJECT) {
                int nameLength = lastName - firstName;
#ifdef _WIN32
				char *newParent = (char *)_alloca(sizeof(char)*(nameLength + 1));
#else
				char newParent[nameLength + 1];
#endif
                strncpy(newParent, firstName, nameLength);
                newParent[nameLength] = '\0';
                return constructObject(newParent, 
                                       first, 
                                       createHashtable, 
                                       maxProperties, 
                                       properties, 
                                       propertiesLength, 
                                       midIndex, 
                                       sidIndex, 
                                       next);
            }
        } else {
            int nameLength = lastName - firstName;
            int fullNameLength = nameLength + parentNameLength + 1;
#ifdef _WIN32
			char *fullName = (char *)_alloca(sizeof(char)*(fullNameLength + 1));
#else
			char fullName[fullNameLength + 1];
#endif
            strncpy(fullName, firstName, nameLength);
            fullName[nameLength] = '.';
            strncpy(&fullName[nameLength + 1], parentName, parentNameLength);
            fullName[fullNameLength] = '\0';
            hash = djbHash(fullName, fullNameLength);
            if (type == JSON_OBJECT) {
                return constructObject(fullName, 
                                       first, 
                                       createHashtable, 
                                       maxProperties, 
                                       properties, 
                                       propertiesLength, 
                                       midIndex, 
                                       sidIndex, 
                                       next);
            }
        }

        jsonProperty *property = NULL;
        if (!createHashtable) {
            property = &properties[*propertiesLength];
            if (hash == HASH_ID) {
                *midIndex = *propertiesLength;
            } else if (hash == HASH_SID) {
                *sidIndex = *propertiesLength;
            }
        } else {
            unsigned int candidate = hash % maxProperties;
            while (properties[candidate].type != 0) {
                candidate = (candidate + 1) % maxProperties;
            }

            if (hash == HASH_ID) {
                *midIndex = candidate;
            } else if (hash == HASH_SID) {
                *sidIndex = candidate;
            }

            property = &properties[candidate];
        } 

        

        *propertiesLength = *propertiesLength + 1;
        if (*propertiesLength == maxProperties) {
            return ERR_EVENT_MAX_PROPERTIES;
        }
        
        property->isMaterial = 0;
        property->hash = hash;
        property->valueOffset = first - object;
        property->valueLength = last - first;
        property->nameOffset = firstName - object;
        property->nameLength = lastName - firstName;
        property->type = type;
        *next = last;
        result = readNextName(last, &firstName, &lastName, &hash);
    }
 
    return (result == PARSE_END ? RULES_OK: result);
}

void rehydrateProperty(jsonProperty *property, char *state) {
    // ID and SID are treated as strings regardless of type
    // to avoid unnecessary conversions
    if (!property->isMaterial && property->hash != HASH_ID && property->hash != HASH_SID) {
        unsigned short propertyLength = property->valueLength + 1;
        char *propertyFirst = state + property->valueOffset;
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
    unsigned int sidHash = djbHash(sid, strlen(sid));
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

    memset(entry->properties, 0, MAX_STATE_PROPERTIES * sizeof(jsonProperty));
    entry->propertiesLength = 0;
    char *next;
    unsigned int midIndex = UNDEFINED_INDEX;
    unsigned int sidIndex = UNDEFINED_INDEX;
    result =  constructObject(NULL, 
                             entry->state, 
                             1, 
                             MAX_STATE_PROPERTIES, 
                             entry->properties, 
                             &entry->propertiesLength, 
                             &midIndex, 
                             &sidIndex, 
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
    unsigned int sidHash = djbHash(sid, strlen(sid));
    stateEntry *entry = getEntry(tree, sid, sidHash);
    if (entry == NULL || entry->lastRefresh == 0) {
        return ERR_STATE_NOT_LOADED;
    }
    
    if (!ignoreStaleState && (time(NULL) - entry->lastRefresh > maxTime)) {
        return ERR_STALE_STATE;
    }

    unsigned int propertyIndex = propertyHash % MAX_STATE_PROPERTIES;
    jsonProperty *result = &entry->properties[propertyIndex];
    while (result->type != 0 && result->hash != propertyHash) {
        propertyIndex = (propertyIndex + 1) % MAX_STATE_PROPERTIES;  
        result = &entry->properties[propertyIndex];   
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
    unsigned int result = resolveBinding(handle, sid, &rulesBinding);
    if (result != RULES_OK) {
      return result;
    }

    return getSession(rulesBinding, sid, state);
}

unsigned int deleteState(void *handle, char *sid) {
    void *rulesBinding = NULL;
    unsigned int result = resolveBinding(handle, sid, &rulesBinding);
    if (result != RULES_OK) {
      return result;
    }

    return deleteSession(rulesBinding, sid);
}
