
#define RULES_OK 0
#define ERR_OUT_OF_MEMORY 1
#define ERR_UNEXPECTED_TYPE 2
#define ERR_IDENTIFIER_LENGTH 3  
#define ERR_UNEXPECTED_VALUE 4 
#define ERR_UNEXPECTED_NAME 5 
#define ERR_RULE_LIMIT_EXCEEDED 6 
#define ERR_RULE_EXISTS_LIMIT_EXCEEDED 7 
#define ERR_RULE_BETA_LIMIT_EXCEEDED 8
#define ERR_RULE_WITHOUT_QUALIFIER 9
#define ERR_SETTING_NOT_FOUND 10
#define ERR_INVALID_HANDLE 11
#define ERR_HANDLE_LIMIT_EXCEEDED 12
#define ERR_PARSE_VALUE 101
#define ERR_PARSE_STRING 102
#define ERR_PARSE_NUMBER 103
#define ERR_PARSE_OBJECT 104
#define ERR_PARSE_ARRAY 105
#define ERR_EVENT_NOT_HANDLED 201
#define ERR_EVENT_MAX_PROPERTIES 202
#define ERR_MAX_STACK_SIZE 203
#define ERR_NO_ID_DEFINED 204
#define ERR_INVALID_ID 205
#define ERR_PARSE_PATH 206
#define ERR_MAX_NODE_RESULTS 207
#define ERR_MAX_RESULT_NODES 208
#define ERR_MAX_COMMAND_COUNT 209
#define ERR_MAX_ADD_COUNT 210
#define ERR_MAX_EVAL_COUNT 211
#define ERR_EVENT_OBSERVED 212
#define ERR_CONNECT_REDIS 301
#define ERR_REDIS_ERROR 302
#define ERR_NO_ACTION_AVAILABLE 303
#define ERR_NO_TIMERS_AVAILABLE 304
#define ERR_NEW_SESSION 305 
#define ERR_TRY_AGAIN 306
#define ERR_STATE_CACHE_FULL 401
#define ERR_BINDING_NOT_MAPPED 402
#define ERR_STATE_NOT_LOADED 403
#define ERR_STALE_STATE 404
#define ERR_PROPERTY_NOT_FOUND 405
#define ERR_MAX_PROPERTY_NAME_LENGTH 406
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
                           char *rules, 
                           unsigned int stateCaheSize);

unsigned int deleteRuleset(unsigned int handle);

unsigned int createClient(unsigned int *handle, 
                          char *name,
                          unsigned int stateCaheSize);

unsigned int deleteClient(unsigned int handle);

unsigned int bindRuleset(unsigned int handle, 
                         char *host, 
                         unsigned int port, 
                         char *password,
                         unsigned char db);

unsigned int complete(void *rulesBinding, 
                      unsigned int replyCount);

unsigned int assertEvent(unsigned int handle, 
                         char *message);

unsigned int startAssertEvent(unsigned int handle, 
                             char *message, 
                             void **rulesBinding, 
                             unsigned int *replyCount);

unsigned int assertEvents(unsigned int handle, 
                          char *messages);

unsigned int startAssertEvents(unsigned int handle, 
                              char *messages, 
                              void **rulesBinding, 
                              unsigned int *replyCount);

unsigned int retractEvent(unsigned int handle, 
                          char *message);

unsigned int startAssertFact(unsigned int handle, 
                             char *message, 
                             void **rulesBinding, 
                             unsigned int *replyCount);

unsigned int assertFact(unsigned int handle, 
                        char *message);

unsigned int startAssertFacts(unsigned int handle, 
                              char *messages, 
                              void **rulesBinding, 
                              unsigned int *replyCount);

unsigned int assertFacts(unsigned int handle, 
                         char *messages);

unsigned int retractFact(unsigned int handle, 
                         char *message);

unsigned int startRetractFact(unsigned int handle, 
                             char *message, 
                             void **rulesBinding, 
                             unsigned int *replyCount);

unsigned int retractFacts(unsigned int handle, 
                          char *messages);

unsigned int startRetractFacts(unsigned int handle, 
                              char *messages, 
                              void **rulesBinding, 
                              unsigned int *replyCount);

unsigned int startUpdateState(unsigned int handle, 
                              void *actionHandle, 
                              char *state,
                              void **rulesBinding,
                              unsigned int *replyCount);

unsigned int assertState(unsigned int handle,
                         char *sid, 
                         char *state);

unsigned int startAction(unsigned int handle, 
                         char **state, 
                         char **messages, 
                         void **actionHandle,
                         void **actionBinding);

unsigned int completeAction(unsigned int handle, 
                            void *actionHandle, 
                            char *state);

unsigned int completeAndStartAction(unsigned int handle, 
                                    unsigned int expectedReplies,
                                    void *actionHandle, 
                                    char **messages);

unsigned int abandonAction(unsigned int handle, 
                           void *actionHandle);

unsigned int startTimer(unsigned int handle, 
                        char *sid, 
                        unsigned int duration, 
                        char manualReset,
                        char *timer);

unsigned int cancelTimer(unsigned int handle, 
                         char *sid, 
                         char *timerName);

unsigned int queueMessage(unsigned int handle,
                          unsigned int queueAction, 
                          char *sid, 
                          char *destination, 
                          char *message);

unsigned int assertTimers(unsigned int handle);

unsigned int getState(unsigned int handle, 
                      char *sid, 
                      char **state);

unsigned int deleteState(unsigned int handle, 
                         char *sid);

unsigned int renewActionLease(unsigned int handle, 
                              char *sid);

#ifdef _WIN32
int asprintf(char** ret, char* format, ...);
#endif

#ifdef __cplusplus
}
#endif
