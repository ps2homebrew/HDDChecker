#include <errno.h>
#include <iomanX.h>
#include <irx.h>
#include <loadcore.h>
#include <stdio.h>
#include <types.h>

#include <sys/stat.h>

#define MODNAME "HDD_logger"
IRX_ID(MODNAME, 1, 1);

#define NUM_TTY	1

static int ttyFds[NUM_TTY];

static int _unmount(int unit);

static int hdlog_init(iop_device_t *fd)
{
	int i;

	for(i=0; i < NUM_TTY; i++)
		ttyFds[i] = -1;

	return 0;
}

static int hdlog_deinit(iop_device_t *fd)
{
	int i;

	for(i=0; i < NUM_TTY; i++)
		_unmount(i);

	return 0;
}

static int hdlog_open(iop_file_t *fd, const char *path, int flags, int mode)
{
	int result;

	if(fd->unit < NUM_TTY && ttyFds[fd->unit] >= 0)
	{
		fd->privdata = (void*)ttyFds[fd->unit];
		result = fd->unit;
	}
	else result = -ENXIO;

	return result;
}

static int hdlog_close(iop_file_t *fd)
{
	return 0;
}

static int hdlog_write(iop_file_t *fd, void *buffer, int size)
{
	if(size == 0)
		return 0;
	else if (size < 0)
		return -1;

	return write((int)fd->privdata, buffer, size);
}

static int hdlog_mount(iop_file_t *f, const char *mountpoint, const char *blockdev, int flags, void *arg, int arglen)
{
	int result, fd;

	if(f->unit < NUM_TTY)
	{
		if(ttyFds[f->unit] < 0)
		{
			printf("HDLOG: mount %s %s %d\n", mountpoint, blockdev, flags);

			if((fd = open(blockdev, flags)) >= 0)
			{
				ttyFds[f->unit] = fd;

				if(f->unit == 0)
				{
					close(0);
					close(1);

					open("tty00:", O_RDWR);
					open("tty00:", O_WRONLY);
				}
			}

			result = fd;
		} else
			result = -EBUSY;
	}
	else
		result = -ENXIO;

	return result;
}

static int _unmount(int unit)
{
	int result;

	if((unit < NUM_TTY) && (ttyFds[unit] >= 0))
	{
		close(ttyFds[unit]);
		ttyFds[unit] = -1;
		result = 0;
	} else
		result = -ENXIO;

	return result;
}

static int hdlog_umount(iop_file_t *fd, const char *mountpoint)
{
	return _unmount(fd->unit);
}

static int hdlog_NulldevFunction(void)
{
	return -1;
}

/* Device driver I/O functions */
static iop_device_ops_t hdlog_functarray={
	&hdlog_init,			/* INIT */
	&hdlog_deinit,			/* DEINIT */
	(void*)&hdlog_NulldevFunction,	/* FORMAT */
	(void*)&hdlog_open,		/* OPEN */
	(void*)&hdlog_close,		/* CLOSE */
	(void*)&hdlog_NulldevFunction,	/* READ */
	(void*)&hdlog_write,		/* WRITE */
	(void*)&hdlog_NulldevFunction,	/* LSEEK */
	(void*)&hdlog_NulldevFunction,	/* IOCTL */
	(void*)&hdlog_NulldevFunction,	/* REMOVE */
	(void*)&hdlog_NulldevFunction,	/* MKDIR */
	(void*)&hdlog_NulldevFunction,	/* RMDIR */
	(void*)&hdlog_NulldevFunction,	/* DOPEN */
	(void*)&hdlog_NulldevFunction,	/* DCLOSE */
	(void*)&hdlog_NulldevFunction,	/* DREAD */
	(void*)&hdlog_NulldevFunction,	/* GETSTAT */
	(void*)&hdlog_NulldevFunction,	/* CHSTAT */
	(void*)&hdlog_NulldevFunction,	/* RENAME */
	(void*)&hdlog_NulldevFunction,	/* CHDIR */
	(void*)&hdlog_NulldevFunction,	/* SYNC */
	&hdlog_mount,			/* MOUNT */
	&hdlog_umount,			/* UMOUNT */
};

static const char hdlog_dev_name[]="tty";

static iop_device_t hdlog_dev={
	hdlog_dev_name,		/* Device name */
	IOP_DT_CHAR|IOP_DT_CONS|IOP_DT_FS|IOP_DT_FSEXT,	/* Device type flag */
	1,			/* Version */
	"HDD logger driver",	/* Description. */
	&hdlog_functarray	/* Device driver function pointer array */
};

/* Entry point */
int _start(int argc, char *argv[])
{
	DelDrv(hdlog_dev_name);
	if(AddDrv(&hdlog_dev) != 0)
	{
		printf("hdlog: unable to register device.\n");
		return MODULE_NO_RESIDENT_END;
	}

	return MODULE_RESIDENT_END;
}
