#pragma once

#include "com_iface.h"

/** Debug USART iface */
extern ComIface *debug_iface;

/** ESP8266 com iface */
extern ComIface *data_iface;

/** Do-nothing iface */
extern ComIface *com_iface_noop;


/** File descriptors for use with built-in "files" */
enum {
	FD_STDIN  = 0,
	FD_STDOUT = 1,
	FD_STDERR = 2,
	FD_DLNK = 3,
};

#define FNAME_DLNK "dlnk"
