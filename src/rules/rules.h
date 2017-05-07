
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
#define NEXT_LIST_LENGTH 32
#define BETA_LIST_LENGTH 16
#define HASH_MASK 0x1F

#define QUEUE_ASSERT_FACT 1
#define QUEUE_ASSERT_EVENT 2
#define QUEUE_RETRACT_FACT 3

#ifdef __cplusplus
extern "C" {
#endif

unsigned int createRuleset(void **handle, 
                           char *name, 
                           char *rules, 
                           unsigned int stateCaheSize);

unsigned int deleteRuleset(void *handle);

unsigned int createClient(void **handle, 
                          char *name,
                          unsigned int stateCaheSize);

unsigned int deleteClient(void *handle);

unsigned int bindRuleset(void *handle, 
                         char *host, 
                         unsigned int port, 
                         char *password,
                         unsigned char db);

unsigned int complete(void *rulesBinding, 
                      unsigned int replyCount);

unsigned int assertEvent(void *handle, 
                         char *message);

unsigned int startAssertEvent(void *handle, 
                             char *message, 
                             void **rulesBinding, 
                             unsigned int *replyCount);

unsigned int assertEvents(void *handle, 
                          char *messages);

unsigned int startAssertEvents(void *handle, 
                              char *messages, 
                              void **rulesBinding, 
                              unsigned int *replyCount);

unsigned int retractEvent(void *handle, 
                          char *message);

unsigned int startAssertFact(void *handle, 
                             char *message, 
                             void **rulesBinding, 
                             unsigned int *replyCount);

unsigned int assertFact(void *handle, 
                        char *message);

unsigned int startAssertFacts(void *handle, 
                              char *messages, 
                              void **rulesBinding, 
                              unsigned int *replyCount);

unsigned int assertFacts(void *handle, 
                         char *messages);

unsigned int retractFact(void *handle, 
                         char *message);

unsigned int startRetractFact(void *handle, 
                             char *message, 
                             void **rulesBinding, 
                             unsigned int *replyCount);

unsigned int retractFacts(void *handle, 
                          char *messages);

unsigned int startRetractFacts(void *handle, 
                              char *messages, 
                              void **rulesBinding, 
                              unsigned int *replyCount);

unsigned int startUpdateState(void *handle, 
                              void *actionHandle, 
                              char *state,
                              void **rulesBinding,
                              unsigned int *replyCount);

unsigned int assertState(void *handle,
                         char *sid, 
                         char *state);

unsigned int startAction(void *handle, 
                         char **state, 
                         char **messages, 
                         void **actionHandle,
                         void **actionBinding);

unsigned int completeAction(void *handle, 
                            void *actionHandle, 
                            char *state);

unsigned int completeAndStartAction(void *handle, 
                                    unsigned int expectedReplies,
                                    void *actionHandle, 
                                    char **messages);

unsigned int abandonAction(void *handle, 
                           void *actionHandle);

unsigned int startTimer(void *handle, 
                        char *sid, 
                        unsigned int duration, 
                        char manualReset,
                        char *timer);

unsigned int cancelTimer(void *handle, 
                         char *sid, 
                         char *timerName);

unsigned int queueMessage(void *handle,
                          unsigned int queueAction, 
                          char *sid, 
                          char *destination, 
                          char *message);

unsigned int assertTimers(void *handle);

unsigned int getState(void *handle, 
                      char *sid, 
                      char **state);

unsigned int deleteState(void *handle, 
                         char *sid);

unsigned int renewActionLease(void *handle, 
                              char *sid);

#ifdef _WIN32
int asprintf(char** ret, char* format, ...);
#endif

#ifdef __cplusplus
}
#endif
