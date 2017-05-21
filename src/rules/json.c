
#include <stdio.h>
#include <string.h>
#include "rules.h"
#include "json.h"

#define ST_OBJECT_SEEK 0
#define ST_OBJECT_PROP_SEEK 1
#define ST_OBJECT_PROP_NAME 2
#define ST_OBJECT_PROP_PARSE 3
#define ST_OBJECT_PROP_VAL 4

#define ST_ARRAY_SEEK 0
#define ST_ARRAY_VAL_SEEK 1
#define ST_ARRAY_VAL_PARSE 2

#define ST_STRING_SEEK 0
#define ST_STRING_PARSE 1
#define ST_STRING_ESC 2

#define ST_NUMBER_SEEK 0
#define ST_NUMBER_INT 1
#define ST_NUMBER_FRA 2
#define ST_NUMBER_EXP 3

#define IS_WHITESPACE(value) (value == ' ' || value == '\t' || value == '\n' || value == '\t') 

static unsigned int getValue(char *start, char **first, char **last, unsigned char *type);
static unsigned int getObject(char *start, char **first, char **last);
static unsigned int getArray(char *start, char **first, char **last);
static unsigned int getNumber(char *start, char **first, char **last, unsigned char *type);
static unsigned int getString(char *start, char **first, char **last);
static unsigned int getStringAndHash(char *start, char **first, char **last, unsigned int *hash);

unsigned int readNextName(char *start, char **first, char **last, unsigned int *hash) {
    unsigned char state = ST_OBJECT_SEEK;
    while(start[0] != '\0') {
        switch(state) {
            case ST_OBJECT_SEEK:
                if (start[0] == '{') {
                    state = ST_OBJECT_PROP_NAME;
                } else if (!IS_WHITESPACE(start[0])) {
                    state = ST_OBJECT_PROP_SEEK;
                } 
                break;
            case ST_OBJECT_PROP_SEEK:
                if (start[0] == ',') {
                    state = ST_OBJECT_PROP_NAME;
                } else if (start[0] == '}') {
                    *first = start;
                    *last = start;
                    return PARSE_END;
                } else if (!IS_WHITESPACE(start[0])) {
                    return ERR_PARSE_OBJECT;
                }
                break;
            case ST_OBJECT_PROP_NAME: 
                if (!IS_WHITESPACE(start[0])) {
                    return getStringAndHash(start, first, last, hash);
                }
                break;
        }

        ++start;
    }

    return ERR_PARSE_OBJECT;
}

unsigned int readNextValue(char *start, char **first, char **last, unsigned char *type) {
    unsigned char state = ST_OBJECT_PROP_PARSE;
    ++start;
    
    while(start[0] != '\0') {
        switch(state) {
            case ST_OBJECT_PROP_PARSE:
                if (start[0] == ':') {
                    state = ST_OBJECT_PROP_VAL;
                } else if (!IS_WHITESPACE(start[0])) {
                    return ERR_PARSE_OBJECT;
                }
                break;
            case ST_OBJECT_PROP_VAL:
                return getValue(start, first, last, type);
        }
                
        ++start;
    }

    return ERR_PARSE_OBJECT;
}

unsigned int readNextString(char *start, char **first, char **last, unsigned int *hash) {
    unsigned char state = ST_OBJECT_PROP_PARSE;
    ++start;
    while(start[0] != '\0') {
        switch(state) {
            case ST_OBJECT_PROP_PARSE:
                if (start[0] == ':') {
                    state = ST_OBJECT_PROP_VAL;
                } else if (!IS_WHITESPACE(start[0])) {
                    return ERR_PARSE_OBJECT;
                }
                break;
            case ST_OBJECT_PROP_VAL:
                return getStringAndHash(start, first, last, hash);
        }
                
        ++start;
    }

    return ERR_PARSE_OBJECT;
}

unsigned int readNextArrayValue(char *start, char **first, char **last, unsigned char *type) {
    unsigned char state = ST_OBJECT_PROP_PARSE;
    if (start[0] == '[') {
        state = ST_OBJECT_PROP_VAL;
    } 
    ++start;
    while(start[0] != '\0') {
        switch(state) {
            case ST_OBJECT_PROP_PARSE:
                if (start[0] == ',') {
                    state = ST_OBJECT_PROP_VAL;
                } else if (start[0] == ']') {
                    *first = start;
                    *last = start;
                    return PARSE_END;    
                } else if (!IS_WHITESPACE(start[0])) {
                    return ERR_PARSE_OBJECT;
                }
                break;
            case ST_OBJECT_PROP_VAL:
                return getValue(start, first, last, type);
                break;
        }
                
        ++start;
    }

    return ERR_PARSE_OBJECT;
}

static unsigned int getValue(char *start, char **first, char **last, unsigned char *type) {
    *type = JSON_OBJECT;
    while(start[0] != '\0') {   
        if (start[0] == '"' || start[0] == '\'') {
            *type = JSON_STRING;
            return getString(start, first, last);
        } else if ((start[0] >= '0' && start[0] <= '9') || start[0] == '-') {
            return getNumber(start, first, last, type);
        } else if (start[0] == 'f') {
            if (strncmp(start, "false", 5) == 0) {
                *first = start;
                *last = start + 4;
                *type = JSON_BOOL;
                return PARSE_OK;
            } else {
                return ERR_PARSE_STRING;
            }
        } else if (start[0] == 't') {
            if (strncmp(start, "true", 4) == 0) {
                *first = start;
                *last = start + 3;
                *type = JSON_BOOL;
                return PARSE_OK;
            } else {
                return ERR_PARSE_STRING;
            }
        } else if (start[0] == 'N') {
            if (strncmp(start, "NaN", 3) == 0) {
                *first = start;
                *last = start + 2;
                *type = JSON_NIL;
                return PARSE_OK;
            } else {
                return ERR_PARSE_STRING;
            }
        } else if (start[0] == 'n') {
            if (strncmp(start, "null", 4) == 0) {
                *first = start;
                *last = start + 3;
                *type = JSON_NIL;
                return PARSE_OK;
            } else {
                return ERR_PARSE_STRING;
            }
        } else if (start[0] == '{') {
            *type = JSON_OBJECT;
            unsigned int result = getObject(start, first, last);
            return result; 
        } else if (start[0] == '[') {
            *type = JSON_ARRAY;
            return getArray(start, first, last);
        } else if (!IS_WHITESPACE(start[0])) {
            return ERR_PARSE_STRING;
        } 

        ++start;
    }

    return ERR_PARSE_VALUE;
}

static unsigned int getObject(char *start, char **first, char **last) {
    unsigned char state = ST_OBJECT_SEEK;
    char *dummy;
    unsigned char dummyType;
    unsigned int result;
    while(start[0] != '\0') {
        switch(state) {
            case ST_OBJECT_SEEK:
                if (start[0] == '{') {
                    state = ST_OBJECT_PROP_NAME;
                    *first = start;
                } else if (!IS_WHITESPACE(start[0])) {
                    return ERR_PARSE_OBJECT;
                }
                break;
            case ST_OBJECT_PROP_SEEK:
                if (start[0] == '}') {
                    *last = start;
                    return PARSE_OK;
                } else if (start[0] == ',') {
                    state = ST_OBJECT_PROP_NAME;
                } else if (!IS_WHITESPACE(start[0])) {
                    return ERR_PARSE_OBJECT;
                }
                break;
            case ST_OBJECT_PROP_NAME: 
                if (start[0] == '}') {
                    *last = start;
                    return PARSE_OK;
                } else if (!IS_WHITESPACE(start[0])) {
                    result = getString(start, &dummy , &start); 
                    if (result != PARSE_OK) {
                        return result;
                    } else {
                        state = ST_OBJECT_PROP_PARSE;
                    }
                }
                break;
            case ST_OBJECT_PROP_PARSE:
                if (start[0] == ':') {
                    state = ST_OBJECT_PROP_VAL;
                } else if (!IS_WHITESPACE(start[0])) {
                    return ERR_PARSE_OBJECT;
                }
                break;
            case ST_OBJECT_PROP_VAL:
                result = getValue(start, &dummy , &start, &dummyType);
                if (result != PARSE_OK) {
                    return result;
                } else {
                    state = ST_OBJECT_PROP_SEEK;
                }
                break;
        }
                
        ++start;
    }

    return ERR_PARSE_OBJECT;
}

static unsigned int getArray(char *start, char** first, char **last) {
    unsigned char state = ST_ARRAY_SEEK;
    char *dummy;
    unsigned char dummyType;
    unsigned int result;
    while(start[0] != '\0') {
        switch(state) {
            case ST_ARRAY_SEEK:
                if (start[0] == '[') {
                    state = ST_ARRAY_VAL_PARSE;
                    *first = start;
                } else if (!IS_WHITESPACE(start[0])) {
                    return ERR_PARSE_ARRAY;
                }
                break;
            case ST_ARRAY_VAL_SEEK:
                if (start[0] == ']') {
                    *last = start;
                    return PARSE_OK;
                }
                else if (start[0] == ',') {
                    state = ST_ARRAY_VAL_PARSE;
                } else if (!IS_WHITESPACE(start[0])) {
                    return ERR_PARSE_ARRAY;
                }
                break;
            case ST_ARRAY_VAL_PARSE: 
                if (start[0] == ']') {
                    *last = start;
                    return PARSE_OK;
                } else if (!IS_WHITESPACE(start[0])) {
                    result = getValue(start, &dummy , &start, &dummyType); 
                    if (result != PARSE_OK) {
                        return result;
                    } else {
                        state = ST_ARRAY_VAL_SEEK;
                    }
                }
                break;
        }

        ++start;
    }
    
    return ERR_PARSE_ARRAY;
}

static unsigned int getNumber(char *start, char **first, char **last, unsigned char *type) {
    unsigned char state = ST_NUMBER_SEEK;
    *type = JSON_INT;
    while(start[0] != '\0') {
        switch(state) {
            case ST_NUMBER_SEEK:
                if ((start[0] >= '0' && start[0] <= '9') || start[0] == '-') {
                    state = ST_NUMBER_INT;
                    *first = start;
                } else if (!IS_WHITESPACE(start[0])) {
                    return ERR_PARSE_NUMBER;
                }
                break;
            case ST_NUMBER_INT:
                if (start[0] == '.') {
                    state = ST_NUMBER_FRA;
                    *type = JSON_DOUBLE;
                } else if (start[0] < '0' || start[0] > '9') {
                    *last = start - 1;
                    return PARSE_OK;
                }
                break;
            case ST_NUMBER_FRA:
                if (start[0] == 'e' || start[0] == 'E') {
                    state = ST_NUMBER_EXP;
                    if (start[1] == '+' || start[1] == '-') {
                        ++ start;
                    }
                } else if (start[0] < '0' || start[0] > '9') {
                    *last = start - 1;
                    return PARSE_OK;
                }
                break;
            case ST_NUMBER_EXP:
                if (start[0] < '0' || start[0] > '9') {
                    *last = start - 1;
                    return PARSE_OK;
                }
                break;
        }

        ++start;
    }

    return ERR_PARSE_NUMBER;
}

static unsigned int getString(char *start, char **first, char **last) {
    unsigned char state = ST_STRING_SEEK;
    char delimiter = '\0';
    while(start[0] != '\0') {
        switch (state) {
            case ST_STRING_SEEK:
                if (start[0] == '"' || start[0] == '\'') {
                    state = ST_STRING_PARSE;
                    delimiter = start[0];
                    *first = start + 1;
                }
                else if (!IS_WHITESPACE(start[0])) {
                    return ERR_PARSE_STRING;
                }
                break;
            case ST_STRING_PARSE:
                if (start[0] == delimiter) {
                    *last = start;
                    return PARSE_OK;
                } else if (start[0] == '\\') {
                    state = ST_STRING_ESC;  
                }
                break;
            case ST_STRING_ESC:
                state = ST_STRING_PARSE;
                break;
        }

        ++ start;
    }

    return ERR_PARSE_STRING;
}

static unsigned int getStringAndHash(char *start, char **first, char **last, unsigned int *hash) {
    unsigned char state = ST_STRING_SEEK;
    char delimiter = '\0';
    unsigned int currentHash = FNV_32_OFFSET_BASIS;
    while(start[0] != '\0') {
        switch (state) {
            case ST_STRING_SEEK:
                if (start[0] == '"' || start[0] == '\'') {
                    state = ST_STRING_PARSE;
                    delimiter = start[0];
                    *first = start + 1;
                }
                else if (!IS_WHITESPACE(start[0])) {
                    return ERR_PARSE_STRING;
                }
                break;
            case ST_STRING_PARSE:
                if (start[0] == delimiter) {
                    *last = start;
                    *hash = currentHash;
                    return PARSE_OK;
                } else if (start[0] == '\\') {
                    state = ST_STRING_ESC;  
                }

                currentHash ^= (*start);
                currentHash *= FNV_32_PRIME;
                break;
            case ST_STRING_ESC:
                state = ST_STRING_PARSE;

                currentHash ^= (*start);
                currentHash *= FNV_32_PRIME;
                break;
        }

        ++ start;
    }

    return ERR_PARSE_STRING;
}

