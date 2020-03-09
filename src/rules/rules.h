
#define RULES_OK 0
#define ERR_EVENT_NOT_HANDLED 1
#define ERR_EVENT_OBSERVED 2
#define ERR_EVENT_DEFERRED 3

#define ERR_OUT_OF_MEMORY 101
#define ERR_UNEXPECTED_TYPE 102
#define ERR_IDENTIFIER_LENGTH 103  
#define ERR_UNEXPECTED_VALUE 104 
#define ERR_UNEXPECTED_NAME 105 
#define ERR_RULE_LIMIT_EXCEEDED 106 
#define ERR_RULE_EXISTS_LIMIT_EXCEEDED 107 
#define ERR_RULE_BETA_LIMIT_EXCEEDED 108
#define ERR_RULE_WITHOUT_QUALIFIER 109
#define ERR_SETTING_NOT_FOUND 110
#define ERR_INVALID_HANDLE 111
#define ERR_HANDLE_LIMIT_EXCEEDED 112
#define ERR_EXPRESSION_LIMIT_EXCEEDED 113
#define ERR_PARSE_VALUE 201
#define ERR_PARSE_STRING 202
#define ERR_PARSE_NUMBER 203
#define ERR_PARSE_OBJECT 204
#define ERR_PARSE_ARRAY 205
#define ERR_EVENT_MAX_PROPERTIES 302
#define ERR_MAX_STACK_SIZE 303
#define ERR_MAX_MESSAGES_IN_FRAME 304
#define ERR_MESSAGE_NOT_FOUND 305
#define ERR_LOCATION_NOT_FOUND 306
#define ERR_NODE_DELETED 307
#define ERR_NODE_DISPATCHING 308
#define ERR_SID_NOT_FOUND 309
#define ERR_NO_ACTION_AVAILABLE 310
#define ERR_PROPERTY_NOT_FOUND 311
#define ERR_OPERATION_NOT_SUPPORTED 312
#define ERR_PARSE_REGEX 501
#define ERR_REGEX_MAX_TRANSITIONS 502
#define ERR_REGEX_MAX_STATES 503
#define ERR_REGEX_QUEUE_FULL 504
#define ERR_REGEX_LIST_FULL 505
#define ERR_REGEX_SET_FULL 506
#define ERR_REGEX_CONFLICT 507

#define NEXT_BUCKET_LENGTH 512
#define NEXT_LIST_LENGTH 128
#define BETA_LIST_LENGTH 256
#define HASH_MASK 0x1F

#define QUEUE_ASSERT_FACT 1
#define QUEUE_ASSERT_EVENT 2
#define QUEUE_RETRACT_FACT 3

#define MAX_HANDLES 131072


#ifdef __cplusplus
extern "C" {
#endif


typedef struct handleEntry {
    void *content;
    unsigned int nextEmptyEntry;
} handleEntry;

handleEntry handleEntries[MAX_HANDLES];
extern unsigned int firstEmptyEntry;
extern unsigned int lastEmptyEntry;
extern char entriesInitialized;

#define CHECK_RESULT(func) do { \
    unsigned int result = func; \
    if (result != RULES_OK) { \
        return result; \
    } \
} while(0)

#define INITIALIZE_ENTRIES do { \
    if (!entriesInitialized) { \
        for (unsigned int i = 0; i < lastEmptyEntry; ++i) { \
            handleEntries[i].content = NULL; \
            handleEntries[i].nextEmptyEntry = i + 1; \
        } \
        handleEntries[lastEmptyEntry].nextEmptyEntry = 0; \
        entriesInitialized = 1; \
    } \
} while(0)

#define RESOLVE_HANDLE(h, cRef) do { \
    if (h < 1 || h > MAX_HANDLES - 1) return ERR_INVALID_HANDLE; \
    if (!handleEntries[h].content) return ERR_INVALID_HANDLE; \
    *cRef = handleEntries[h].content; \
} while(0)

#define CREATE_HANDLE(c, hRef) do { \
    if (firstEmptyEntry == 0) return ERR_HANDLE_LIMIT_EXCEEDED; \
    handleEntries[firstEmptyEntry].content = c; \
    *hRef = firstEmptyEntry; \
    firstEmptyEntry = handleEntries[firstEmptyEntry].nextEmptyEntry; \
} while(0)

#define DELETE_HANDLE(h) do { \
    if (h < 1 || h > MAX_HANDLES - 1) return ERR_INVALID_HANDLE; \
    if (!handleEntries[h].content) return ERR_INVALID_HANDLE; \
    handleEntries[h].content = NULL; \
    handleEntries[h].nextEmptyEntry = 0; \
    handleEntries[lastEmptyEntry].nextEmptyEntry = h; \
    lastEmptyEntry = h; \
    if (!firstEmptyEntry) firstEmptyEntry = lastEmptyEntry; \
} while(0)

unsigned int createRuleset(unsigned int *handle, 
                           char *name, 
                           char *rules);

unsigned int setStoreMessageCallback(unsigned int handle, 
                                     void *context, 
                                     unsigned int (*callback)(void *, char *, char *, char *, unsigned char, char *));

unsigned int setDeleteMessageCallback(unsigned int handle, 
                                      void *context,
                                      unsigned int (*callback)(void *, char *, char *, char *));

unsigned int setQueueMessageCallback(unsigned int handle, 
                                     void *context, 
                                     unsigned int (*callback)(void *, char *, char *, unsigned char, char *));

unsigned int setGetQueuedMessagesCallback(unsigned int handle, 
                                          void *context, 
                                          unsigned int (*callback)(void *, char *, char *));

unsigned int completeGetQueuedMessages(unsigned int handle,
                                       char *sid,
                                       char *queuedMessages);

unsigned int setGetIdleStateCallback(unsigned int handle, 
                                     void *context, 
                                     unsigned int (*callback)(void *, char *));

unsigned int completeGetIdleState(unsigned int handle, 
                                  char *sid, 
                                  char *storedMessages);

unsigned int deleteRuleset(unsigned int handle);

unsigned int createClient(unsigned int *handle, 
                          char *name);

unsigned int deleteClient(unsigned int handle);

unsigned int bindRuleset(unsigned int handle, 
                         char *host, 
                         unsigned int port, 
                         char *password,
                         unsigned char db);

unsigned int assertEvent(unsigned int handle, 
                         char *message,
                          unsigned int *stateOffset);

unsigned int assertEvents(unsigned int handle, 
                          char *messages,
                          unsigned int *stateOffset);

unsigned int assertFact(unsigned int handle, 
                        char *message,
                          unsigned int *stateOffset);

unsigned int assertFacts(unsigned int handle, 
                         char *messages,
                          unsigned int *stateOffset);

unsigned int retractFact(unsigned int handle, 
                         char *message,
                          unsigned int *stateOffset);

unsigned int retractFacts(unsigned int handle, 
                          char *messages,
                          unsigned int *stateOffset);

unsigned int updateState(unsigned int handle,
                         char *state,
                         unsigned int *stateOffset);

unsigned int startAction(unsigned int handle, 
                         char **stateFact, 
                         char **messages, 
                         unsigned int *stateOffset);

unsigned int startActionForState(unsigned int handle, 
                                 unsigned int stateOffset,
                                 char **stateFact,
                                 char **messages);

unsigned int completeAndStartAction(unsigned int handle, 
                                    unsigned int stateOffset, 
                                    char **messages);

unsigned int abandonAction(unsigned int handle, 
                           unsigned int stateOffset);

unsigned int startTimer(unsigned int handle, 
                        char *sid, 
                        unsigned int duration, 
                        char manualReset,
                        char *timer);

unsigned int cancelTimer(unsigned int handle, 
                         char *sid, 
                         char *timerName);

unsigned int assertTimers(unsigned int handle);

unsigned int getState(unsigned int handle, 
                      char *sid, 
                      char **state);

unsigned int deleteState(unsigned int handle, 
                         char *sid);

unsigned int renewActionLease(unsigned int handle, 
                              char *sid);

unsigned int getEvents(unsigned int handle, 
                       char *sid, 
                       char **messages);

unsigned int getFacts(unsigned int handle, 
                      char *sid, 
                      char **messages);

#ifdef _WIN32
int asprintf(char** ret, char* format, ...);
#endif

#ifdef __cplusplus
}
#endif
