
#include "rete.h"
#include "../../deps/hiredis/hiredis.h"

#define HASH_LENGTH 40
#define MAX_MESSAGE_BATCH 64

typedef char functionHash[HASH_LENGTH + 1];

typedef struct binding {
    redisContext *reContext;
    functionHash assertMessageHash;
    functionHash peekActionHash;
    functionHash removeActionHash;
    functionHash partitionHash;
    functionHash timersHash;
    char *sessionHashset;
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

unsigned int assertMessageImmediate(void *rulesBinding, 
                                    char *key, 
                                    char *sid, 
                                    char *message, 
                                    jsonProperty *allProperties,
                                    unsigned int propertiesLength,
                                    unsigned int actionIndex);

unsigned int assertFirstMessage(void *rulesBinding, 
                                char *key, 
                                char *sid, 
                                char *message, 
                                jsonProperty *allProperties,
                                unsigned int propertiesLength);

unsigned int assertMessage(void *rulesBinding, 
                           char *key, 
                           char *sid, 
                           char *message, 
                           jsonProperty *allProperties,
                           unsigned int propertiesLength);

unsigned int assertLastMessage(void *rulesBinding, 
                              char *key, 
                              char *sid, 
                              char *message, 
                              jsonProperty *allProperties,
                              unsigned int propertiesLength,
                              int actionIndex,
                              unsigned int messageCount);

unsigned int peekAction(ruleset *tree, 
                        void **bindingContext, 
                        redisReply **reply);

unsigned int removeAction(void *rulesBinding, 
                          char *sid);

unsigned int assertTimer(void *rulesBinding, 
                         char *key, 
                         char *sid, 
                         char *timer, 
                         jsonProperty *allProperties,
                         unsigned int propertiesLength,
                         unsigned int actionIndex);

unsigned int peekTimers(ruleset *tree, void **bindingContext, redisReply **reply);
unsigned int prepareCommands(void *rulesBinding);
unsigned int rollbackCommands(void *rulesBinding);
unsigned int executeCommands(void *rulesBinding, unsigned int commandCount);
unsigned int registerTimer(void *rulesBinding, unsigned int duration, char *timer);
unsigned int removeTimer(void *rulesBinding, char *timer);
unsigned int deleteBindingsList(ruleset *tree);
unsigned int getSession(void *rulesBinding, char *sid, char **state);
unsigned int storeSession(void *rulesBinding, char *sid, char *state);
unsigned int storeSessionImmediate(void *rulesBinding, char *sid, char *state);


