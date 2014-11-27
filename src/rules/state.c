#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "rules.h"
#include "json.h"
#include "net.h"

static unsigned int djbHash(char *str, unsigned int len) {
   unsigned int hash = 5381;
   unsigned int i = 0;

   for(i = 0; i < len; str++, i++) {
      hash = ((hash << 5) + hash) + (*str);
   }

   return hash;
}

static stateEntry *getEntry(ruleset *tree, char *sid, unsigned int *sidHash) {
    *sidHash = djbHash(sid, strlen(sid));
    unsigned int candidateIndex = *sidHash % MAX_STATE_ENTRIES;
    stateEntry *candidate = &tree->stateMap[candidateIndex];
    if (candidate->sidHash && candidate->sidHash != *sidHash) {
        candidateIndex = (candidateIndex + 1) % MAX_STATE_ENTRIES;
        candidate = &tree->stateMap[candidateIndex];
    }

    return candidate;
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
                char newParent[nameLength + 1];
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
            char fullName[fullNameLength + 1];
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
            if (hash == ID_HASH) {
                *midIndex = *propertiesLength;
            } else if (hash == SID_HASH) {
                *sidIndex = *propertiesLength;
            }
        } else {
            unsigned int candidate = hash % maxProperties;
            while (properties[candidate].type != 0) {
                candidate = (candidate + 1) % maxProperties;
            }

            if (hash == ID_HASH) {
                *midIndex = candidate;
            } else if (hash == SID_HASH) {
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
        property->firstValue = first;
        property->lastValue = last;
        property->type = type;
        *next = last;
        result = readNextName(last, &firstName, &lastName, &hash);
    }
 
    return (result == PARSE_END ? RULES_OK: result);
}

void rehydrateProperty(jsonProperty *property) {
    char *propertyLast = property->lastValue;
    char *propertyFirst = property->firstValue;
    unsigned char propertyType = property->type;
    char temp;

    if (!property->isMaterial) {
        switch(propertyType) {
            case JSON_INT:
                ++propertyLast;
                temp = propertyLast[0];
                propertyLast[0] = '\0';
                property->value.i = atol(propertyFirst);
                propertyLast[0] = temp;
                break;
            case JSON_DOUBLE:
                ++propertyLast;
                temp = propertyLast[0];
                propertyLast[0] = '\0';
                property->value.i = atof(propertyFirst);
                propertyLast[0] = temp;
                break;
            case JSON_BOOL:
                ++propertyLast;
                unsigned int leftLength = propertyLast - propertyFirst;
                unsigned char b = 1;
                if (leftLength == 5 && strncmp("false", propertyFirst, 5)) {
                    b = 0;
                }
                property->value.b = b;
                break;
        }

        property->isMaterial = 1;
    }
}

unsigned int resolveBinding(void *tree, 
                            char *sid, 
                            void **rulesBinding) {

    
    unsigned int sidHash;
    ruleset *handle = (ruleset*)tree; 
    stateEntry *entry = getEntry(tree, sid, &sidHash);
    if (entry->sidHash == 0) {
        if (handle->stateMapLength >= MAX_STATE_ENTRIES - 1) {
            return ERR_STATE_CACHE_FULL;
        }

        ++handle->stateMapLength;
        unsigned int result = getBindingIndex(tree, sidHash, &entry->bindingIndex);
        if (result != RULES_OK) {
            return result;
        }

        entry->sidHash = sidHash;
    }
    
    bindingsList *list = handle->bindingsList;
    *rulesBinding = &list->bindings[entry->bindingIndex];
    return RULES_OK;
}

unsigned int refreshState(void *tree, char *sid) {

    unsigned int sidHash;   
    stateEntry *entry = getEntry(tree, sid, &sidHash);
    if (entry->sidHash == 0) {
        return ERR_BINDING_NOT_MAPPED;
    }

    unsigned int result = getState(tree, sid, &entry->state);
    if (result != RULES_OK) {
        return result;
    }

    memset(entry->properties, 0, MAX_STATE_PROPERTIES * sizeof(jsonProperty));

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

unsigned int fetchProperty(void *tree,
                           char *sid, 
                           unsigned int propertyHash, 
                           unsigned int maxTime, 
                           unsigned char ignoreStaleState,
                           jsonProperty **property) {

    unsigned int sidHash;   
    stateEntry *entry = getEntry(tree, sid, &sidHash);
    if (entry->sidHash == 0) {
        return ERR_BINDING_NOT_MAPPED;
    }
    
    if (entry->propertiesLength == 0) {
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

    rehydrateProperty(result);
    *property = result;
    return RULES_OK;    
}

