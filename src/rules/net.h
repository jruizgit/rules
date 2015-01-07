
#include "rete.h"
#include "../../deps/hiredis/hiredis.h"
#include "../../deps/hiredis/sds.h"

#define HASH_LENGTH 40
#define MAX_MESSAGE_BATCH 64

typedef char functionHash[HASH_LENGTH + 1];

typedef struct binding {
    redisContext *reContext;
    functionHash peekActionHash;
    functionHash removeActionHash;
    functionHash partitionHash;
    functionHash timersHash;
    char *sessionHashset;
    char *factsHashset;
    char *timersSortedset;
    functionHash *hashArray;
    unsigned int hashArrayLength;
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
                               char *key, 
                               char *sid, 
                               char *message, 
                               jsonProperty *allProperties,
                               unsigned int propertiesLength,
                               unsigned int actionIndex,
                               unsigned char assertFact,
                               char **command);

unsigned int formatStoreSession(void *rulesBinding, 
                                char *sid, 
                                char *state, 
                                unsigned char tryExists,
                                char **command);

unsigned int formatStoreSessionFactId(void *rulesBinding, 
                                      char *sid, 
                                      char *mid,
                                      unsigned char tryExists, 
                                      char **command);

unsigned int formatRemoveTimer(void *rulesBinding, 
                               char *timer,
                               char **command);

unsigned int formatRemoveAction(void *rulesBinding, 
                                char *sid, 
                                char **command);

unsigned int formatRetractMessage(void *rulesBinding, 
                                  char *sid, 
                                  char *mid,
                                  char **command);

unsigned int executeBatch(void *rulesBinding,
                          char **commands,
                          unsigned short commandCount);

unsigned int retractMessage(void *rulesBinding, 
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

unsigned int deleteBindingsList(ruleset *tree);

unsigned int getSession(void *rulesBinding, 
                        char *sid, 
                        char **state);


