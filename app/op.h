#ifndef CMUS_OP_H
#define CMUS_OP_H

#include "sf.h"
#include "channelmap.h"

#ifndef __GNUC__
#include <fcntl.h>
#endif

#define OP_ABI_VERSION 2

enum {
	/* no error */
	OP_ERROR_SUCCESS,
	/* system error (error code in errno) */
	OP_ERROR_ERRNO,
	/* no such plugin */
	OP_ERROR_NO_PLUGIN,
	/* plugin not initialized */
	OP_ERROR_NOT_INITIALIZED,
	/* function not supported */
	OP_ERROR_NOT_SUPPORTED,
	/* mixer not open */
	OP_ERROR_NOT_OPEN,
	/* plugin does not support the sample format */
	OP_ERROR_SAMPLE_FORMAT,
	/* plugin does not have this option */
	OP_ERROR_NOT_OPTION,
	/*  */
	OP_ERROR_INTERNAL
};

struct output_plugin_ops {
	int (*init)(void);
	int (*exit)(void);
	int (*open)(sample_format_t sf, const channel_position_t *channel_map);
	int (*close)(void);
	int (*drop)(void);
	int (*write)(const char *buffer, int count);
	int (*buffer_space)(void);

	/* these can be NULL */
	int (*pause)(void);
	int (*unpause)(void);

};

#define OPT(prefix, name) { #name, prefix ## _set_ ## name, \
	prefix ## _get_ ## name }

struct output_plugin_opt {
	const char *name;
	int (*set)(const char *val);
	int (*get)(char **val);
};

/* symbols exported by plugin */
extern const struct output_plugin_ops op_pcm_ops;
extern const struct output_plugin_opt op_pcm_options[];
extern const int op_priority;
extern const unsigned op_abi_version;

#endif
