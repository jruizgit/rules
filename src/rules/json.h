

#define PARSE_OK 0
#define PARSE_END 100

#define JSON_STRING 0x01
#define JSON_INT 0x02
#define JSON_DOUBLE 0x03
#define JSON_BOOL 0x04
#define JSON_ARRAY 0x05
#define JSON_OBJECT 0x06
#define JSON_NIL 0x07
#define JSON_STATE_PROPERTY 0x08
#define JSON_EVENT_PROPERTY 0x09
#define JSON_EVENT_LOCAL_PROPERTY 0x0A
#define JSON_STATE_IDIOM 0x0B
#define JSON_EVENT_IDIOM 0x0C
#define JSON_EVENT_LOCAL_IDIOM 0x0D
#define JSON_REGEX 0x0E
#define JSON_IREGEX 0x0F

#define FNV_32_OFFSET_BASIS 0x811c9dc5
#define FNV_32_PRIME 16777619

#define FNV_64_OFFSET_BASIS 0xcbf29ce484222325
#define FNV_64_PRIME 1099511628211

unsigned int readNextName(char *start, char **first, char **last, unsigned int *hash);
unsigned int readNextValue(char *start, char **first, char **last, unsigned char *type);
unsigned int readNextArrayValue(char *start, char **first, char **last, unsigned char *type);
unsigned int readNextString(char *start, char **first, char **last, unsigned int *hash);



