

#define PARSE_OK 0
#define PARSE_END 100

#define JSON_STRING 0x01
#define JSON_INT 0x02
#define JSON_DOUBLE 0x03
#define JSON_BOOL 0x04
#define JSON_ARRAY 0x05
#define JSON_OBJECT 0x06
#define JSON_STATE_PROPERTY 0x07

unsigned int readNextName(char *start, char **first, char **last, unsigned int *hash);
unsigned int readNextValue(char *start, char **first, char **last, unsigned char *type);
unsigned int readNextArrayValue(char *start, char **first, char **last, unsigned char *type);
unsigned int readNextString(char *start, char **first, char **last, unsigned int *hash);



