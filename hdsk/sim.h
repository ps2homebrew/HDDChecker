void hdskSimGetFreeSectors(s32 device, struct hdskStat *stat, apa_device_t *deviceinfo);
void hdskSimMovePartition(struct hdskBitmap *dest, struct hdskBitmap *start);
struct hdskBitmap *hdskSimFindEmptyPartition(u32 size);
struct hdskBitmap *hdskSimFindLastUsedPartition(u32 size, u32 start, int mode);
struct hdskBitmap *hdskSimFindLastUsedBlock(u32 size, u32 start, int mode);
void hdskSimMovePartitionsBlock(struct hdskBitmap *dest, struct hdskBitmap *src);
