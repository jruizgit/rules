
#include "rete.h"
#include "../../deps/hiredis/hiredis.h"

#define MAX_MESSAGE_BATCH 64

unsigned int djbHash(char* str, unsigned int len);
unsigned int resolveBinding(ruleset *tree, char *sid, void **rulesBinding);
unsigned int assertMessageImmediate(void *rulesBinding, char *key, char *sid, char *mid, char *message, unsigned int actionIndex);
unsigned int assertFirstMessage(void *rulesBinding, char *key, char *sid, char *mid, char *message);
unsigned int assertMessage(void *rulesBinding, char *key, char *sid, char *mid, char *message);
unsigned int assertLastMessage(void *rulesBinding, char *key, char *sid, char *mid, char *message, int actionIndex, unsigned int messageCount);
unsigned int peekAction(ruleset *tree, void **bindingContext, redisReply **reply);
unsigned int assertSession(void *rulesBinding, char *key, char *sid, char *state, unsigned int actionIndex);
unsigned int assertSessionImmediate(void *rulesBinding, char *key, char *sid, char *state, unsigned int actionIndex);
unsigned int negateMessage(void *rulesBinding, char *key, char *sid, char *mid);
unsigned int negateSession(void *rulesBinding, char *key, char *sid);
unsigned int removeAction(void *rulesBinding, char *action);
unsigned int removeMessage(void *rulesBinding, char *mid);
unsigned int prepareCommands(void *rulesBinding);
unsigned int executeCommands(void *rulesBinding, unsigned int commandCount);
unsigned int registerTimer(void *rulesBinding, unsigned int duration, char *timer);
unsigned int deleteBindingsMap(ruleset *tree);

