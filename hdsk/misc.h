int HdskReadClock(apa_ps2time_t *time);
void *AllocMemory(int size);
int HdskRI(unsigned char *id);
int HdskUnlockHdd(int unit);
int HdskCreateEventFlag(void);
int HdskCreateThread(void (*function)(void *arg), int StackSize);
