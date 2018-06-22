typedef struct
{
	u32 totalLBA;
	u32 partitionMaxSize;
	int format;
	int status;
} hdd_device_t;

struct hdskBitmap{
	struct hdskBitmap *next;	//0x00
	struct hdskBitmap *prev;	//0x04
	u32 start;			//0x08
	u32 length;			//0x0C
	u32 type;			//0x10
};

#define HDSK_BITMAP_SIZE	0x4001
