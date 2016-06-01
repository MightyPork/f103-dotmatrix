#include "main.h"

#include "com_fileio.h"
#include "com_iface.h"
#include "utils/str_utils.h"

// Holding fields for ifaces
ComIface *debug_iface = NULL;
ComIface *data_iface = NULL;
ComIface *gamepad_iface = NULL;


// --- File descriptor names ------------------------------

struct name_fd {
	const char *name;
	const int fd;
};

#define NAME_FD_MAP_LEN 1

/** pre-assigned file descriptors for names */
static const struct name_fd name_fd_map[NAME_FD_MAP_LEN] = {
	{FNAME_DLNK, FD_DLNK}
};


// --- Syscalls -------------------------------------------

/**
 * @brief Write to a file by file descriptor.
 *
 * @param fd  : open file descriptor
 * @param buf : data to write
 * @param len : buffer size
 * @return number of written bytes
 */
int _write(int fd, const char *buf, int len)
{
	switch (fd) {
		case FD_STDOUT:	return (int)com_tx_block(debug_iface, buf, (size_t)len);
		case FD_STDERR:	return (int)com_tx_block(debug_iface, buf, (size_t)len);
		case FD_DLNK: return (int)com_tx_block(data_iface, buf, (size_t)len);
		default:
			return 0;
	}
}


// For some reason, reading does not work.
#if 0
/**
 * @brief Read from a file descriptor
 *
 * @param fd  : file descriptor
 * @param buf : destination buffer
 * @param len : number of bytes to read
 * @return actual number of read bytes
 */
int _read(int fd, char *buf, int len)
{
	switch (fd) {
		case FD_STDIN: return com_rx_block(debug_iface, buf, len);
		case FD_ESP: return com_rx_block(esp_iface, buf, len);
		default:
			return 0;
	}
}
#endif


/**
 * @brief Open a file by name.
 *
 * This stub is called by newlib when fopen is used.
 * It returns a pre-assigned file descriptor based
 * on the name.
 *
 * @note
 * stdout, stderr, stdin are open implicitly
 *
 * @param name  : file name
 * @param flags : file flags (ignored)
 * @return file descriptor
 */
int _open(const char *name, int flags, ...)
{
	(void)flags;

	for (int  i = 0; i < NAME_FD_MAP_LEN; i++) {
		if (streq(name_fd_map[i].name, name)) {
			return name_fd_map[i].fd;
		}
	}

	return -1;
}
