
#include "rete.h"
#ifdef _WIN32
#include "../../deps/hiredis_win/hiredis.h"
#include "../../deps/hiredis_win/sds.h"
#else
#include "../../deps/hiredis/hiredis.h"
#include "../../deps/hiredis/sds.h"
#endif

#define HASH_LENGTH 40
#define MAX_MESSAGE_BATCH 64

#define ACTION_ASSERT_FACT 1
#define ACTION_ASSERT_EVENT 2
#define ACTION_RETRACT_FACT 3
#define ACTION_RETRACT_EVENT 4
#define ACTION_ADD_FACT 5
#define ACTION_ADD_EVENT 6
#define ACTION_REMOVE_FACT 7
#define ACTION_REMOVE_EVENT 8

typedef char functionHash[HASH_LENGTH + 1];

typedef struct binding {
    redisContext *reContext;
    functionHash evalMessageHash;
    functionHash addMessageHash;
    functionHash peekActionHash;
    functionHash removeActionHash;
    functionHash partitionHash;
    functionHash timersHash;
    functionHash updateActionHash;
    functionHash deleteSessionHash;
    char *sessionHashset;
    char *factsHashset;
    char *eventsHashset;
    char *timersSortedset;
} binding;

typedef struct bindingsList {
    binding *bindings;
    unsigned int bindingsLength;
    unsigned int lastBinding;
    unsigned int lastTimersBinding;
} bindingsList;

unsigned int getBindingIndex(ruleset *tree, 
                             unsigned int sidHash, 
                             unsigned int *bindingIndex);

unsigned int formatEvalMessage(void *rulesBinding, 
                               char *sid, 
                               char *mid,
                               char *message, 
                               jsonProperty *allProperties,
                               unsigned int propertiesLength,
                               unsigned char actionType,
                               char **keys,
                               unsigned int keysLength,
                               char **command);

unsigned int formatStoreMessage(void *rulesBinding, 
                                char *sid, 
                                char *message, 
                                jsonProperty *allProperties,
                                unsigned int propertiesLength,
                                unsigned char storeFact,
                                char **keys,
                                unsigned int keysLength,
                                char **command);

unsigned int formatStoreSession(void *rulesBinding, 
                                char *sid, 
                                char *state, 
                                unsigned char tryExists,
                                char **command);

unsigned int formatStoreSessionFact(void *rulesBinding, 
                                    char *sid, 
                                    char *message,
                                    unsigned char tryExists, 
                                    char **command);

unsigned int formatRemoveTimer(void *rulesBinding, 
                               char *timer,
                               char **command);

unsigned int formatRemoveAction(void *rulesBinding, 
                                char *sid, 
                                char **command);

unsigned int formatRemoveMessage(void *rulesBinding, 
                                 char *sid, 
                                 char *mid,
                                 unsigned char removeFact,
                                 char **command);

unsigned int formatPeekAction(void *rulesBinding,
                              char *sid,
                              char **command);

unsigned int executeBatch(void *rulesBinding,
                          char **commands,
                          unsigned int commandCount);

unsigned int executeBatchWithReply(void *rulesBinding,
                                   unsigned int expectedReplies,
                                   char **commands,
                                   unsigned int commandCount,
                                   redisReply **lastReply);

unsigned int startNonBlockingBatch(void *rulesBinding,
                                   char **commands,
                                   unsigned int commandCount,
                                   unsigned int *replyCount); 

unsigned int completeNonBlockingBatch(void *rulesBinding,
                                      unsigned int replyCount);

unsigned int removeMessage(void *rulesBinding, 
                            char *sid, 
                            char *mid);

unsigned int peekAction(ruleset *tree, 
                        void **bindingContext, 
                        redisReply **reply);

unsigned int peekTimers(ruleset *tree, 
                        void **bindingContext, 
                        redisReply **reply);

unsigned int registerTimer(void *rulesBinding, 
                           unsigned int duration, 
                           char *timer);

unsigned int removeTimer(void *rulesBinding, 
                         char *timer);

unsigned int registerMessage(void *rulesBinding, 
                             unsigned int queueAction,
                             char *destination, 
                             char *message);

unsigned int deleteBindingsList(ruleset *tree);

unsigned int getSession(void *rulesBinding, 
                        char *sid, 
                        char **state);

unsigned int deleteSession(void *rulesBinding, 
                           char *sid);

unsigned int updateAction(void *rulesBinding, 
                          char *sid);

