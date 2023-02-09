#ifndef CMUS_MIXER_H
#define CMUS_MIXER_H

#ifndef __GNUC__
#include <fcntl.h>
#endif

#define NR_MIXER_FDS 4

enum {
    /* volume changes */
    MIXER_FDS_VOLUME,
    /* output changes */
    MIXER_FDS_OUTPUT
};

struct mixer_plugin_ops {
	int (*init)(void);
	int (*exit)(void);
	int (*open)(int *volume_max);
	int (*close)(void);
	union {
	    int (*abi_1)(int *fds); // MIXER_FDS_VOLUME
	    int (*abi_2)(int what, int *fds);
	} get_fds;
	int (*set_volume)(int l, int r);
	int (*get_volume)(int *l, int *r);
};

struct mixer_plugin_opt {
	const char *name;
	int (*set)(const char *val);
	int (*get)(char **val);
};

/* symbols exported by plugin */
extern const struct mixer_plugin_ops op_mixer_ops;
extern const struct mixer_plugin_opt op_mixer_options[];

#endif
