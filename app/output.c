#include "output.h"
#include "mixer-interface.h"
#include "sf.h"
#include "utils.h"
#include "xmalloc.h"
#include "list.h"
#include "debug.h"
#include "ui_curses.h"
#include "options.h"
#include "xstrjoin.h"
#include "misc.h"
#include "log.h"
#include "interface.h"
#include "plugins/output-interface.h"

#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <dirent.h>
#include <dlfcn.h>


extern const char* libDir;
extern const char* dataDir;



static const char*  pluginDir;
static LIST_HEAD(op_head);
static OutputPlugin *op = NULL;

/* volume is between 0 and volume_max */
int volume_max = 0;
int volume_l = -1;
int volume_r = -1;

static void add_plugin(OutputPlugin *plugin)
{
	struct list_head *item = op_head.next;

//	while (item != &op_head) {
//		OutputPlugin *o = container_of(item, OutputPlugin, node);
//
//		if (plugin->priority < o->priority)
//			break;
//		item = item->next;
//	}
//
//	/* add before item */
//	list_add_tail(&plugin->node, item);
}

void op_load_plugins(void)
{
	DIR *dir;
	struct dirent *d;

	pluginDir = xstrjoin(libDir, "/op");
	dir = opendir(pluginDir);
	if (dir == NULL) {
		ERROR ("couldn't open directory `%s': %s", pluginDir, strerror(errno));
		return;
	}
	while ((d = (struct dirent *) readdir(dir)) != NULL) {
		char filename[512];
		OutputPlugin *plug;
		void *so, *symptr;
		char *ext;
		bool err = false;

		if (d->d_name[0] == '.') {
            continue;
        }

		ext = strrchr(d->d_name, '.');
		if (ext == NULL) {
            DEBUG ("ignore '%s/%s'", pluginDir, d->d_name);
            continue;
        }

		if (0 != g_strcmp0 (ext, ".so")) {
            DEBUG ("ignore '%s/%s'", pluginDir, d->d_name);
            continue;
        }

		snprintf(filename, sizeof(filename), "%s/%s", pluginDir, d->d_name);

        DEBUG ("load plugin: '%s'", filename);

		so = dlopen(filename, RTLD_NOW);
		if (so == NULL) {
			ERROR ("dlopen '%s': '%s'", filename, dlerror());
			continue;
		}

//		plug = xnew(OutputPlugin, 1);
//
//		plug->pcmOps = dlsym(so, "op_pcm_ops");
//		plug->pcm_options = dlsym(so, "op_pcm_options");
//		symptr = dlsym(so, "op_priority");
//		plug->abi_version_ptr = dlsym(so, "op_abi_version");
//		if (!plug->pcmOps || !plug->pcm_options || !symptr) {
//			ERROR ("'%s': missing symbol", filename);
//			err = true;
//		}
//		STATIC_ASSERT(OUTPUT_ABI_VERSION == 2);
//		if (!plug->abi_version_ptr || (*plug->abi_version_ptr != 1 && *plug->abi_version_ptr != 2)) {
//			ERROR ("%s: incompatible plugin version", filename);
//			err = true;
//		}
//		if (err) {
//			free(plug);
//			dlclose(so);
//			continue;
//		}
//		plug->priority = *(int *)symptr;
//
//		plug->mixerOps = dlsym(so, "op_mixer_ops");
//		plug->mixerOptions = dlsym(so, "op_mixer_options");
//		if (plug->mixerOps == NULL || plug->mixerOptions == NULL) {
//			plug->mixerOps = NULL;
//			plug->mixerOptions = NULL;
//		}
//
//		plug->name = xstrndup(d->d_name, ext - d->d_name);
//		plug->handle = so;
//		plug->pcm_initialized = 0;
//		plug->mixer_initialized = 0;
//		plug->mixerOpen = 0;
//
//		add_plugin(plug);
	}
	closedir(dir);
}

static void init_plugin(OutputPlugin *o)
{
//	if (!o->mixer_initialized && o->mixerOps) {
//		if (o->mixerOps->init() == 0) {
//			d_print("initialized mixer for %s\n", o->name);
//			o->mixer_initialized = 1;
//		} else {
//			d_print("could not initialize mixer `%s'\n", o->name);
//		}
//	}
//	if (!o->pcm_initialized) {
//		if (o->pcmOps->init() == 0) {
//			d_print("initialized pcm for %s\n", o->name);
//			o->pcm_initialized = 1;
//		} else {
//			d_print("could not initialize pcm `%s'\n", o->name);
//		}
//	}
}

void op_exit_plugins(void)
{
	OutputPlugin *o;

//	list_for_each_entry(o, &op_head, node) {
//		if (o->mixer_initialized && o->mixerOps)
//			o->mixerOps->exit();
//		if (o->pcm_initialized)
//			o->pcmOps->exit();
//	}
}

void mixer_close(void)
{
//	volume_max = 0;
//	if (op && op->mixerOpen) {
//		BUG_ON(op->mixerOps == NULL);
//		op->mixerOps->Close();
//		op->mixerOpen = 0;
//	}
}

void mixer_open(void)
{
	if (op == NULL)
		return;

//	BUG_ON(op->mixerOpen);
//	if (op->mixerOps && op->mixer_initialized) {
//		int rc;
//
//		rc = op->mixerOps->Open(&volume_max);
//		if (rc == 0) {
//			op->mixerOpen = 1;
//			mixer_read_volume();
//		} else {
//			volume_max = 0;
//		}
//	}
}

static int select_plugin(OutputPlugin *o)
{
	/* try to initialize if not initialized yet */
	init_plugin(o);

//	if (!o->pcm_initialized)
//		return -OUTPUT_ERROR_NOT_INITIALIZED;
//	op = o;
//	return 0;
}

int op_select(const char *name)
{
	OutputPlugin *o;

//	list_for_each_entry(o, &op_head, node) {
//		if (strcasecmp(name, o->name) == 0)
//			return select_plugin(o);
//	}
	return -OUTPUT_ERROR_NO_PLUGIN;
}

int op_select_any(void)
{
	OutputPlugin *o;
	int rc = -OUTPUT_ERROR_NO_PLUGIN;
	sample_format_t sf = sf_channels(2) | sf_rate(44100) | sf_bits(16) | sf_signed(1);

//	list_for_each_entry(o, &op_head, node) {
//		rc = select_plugin(o);
//		if (rc != 0)
//			continue;
//		rc = o->pcmOps->Open(sf, NULL);
//		if (rc == 0) {
//			o->pcmOps->Close();
//			break;
//		}
//	}
	return rc;
}

int op_open(sample_format_t sf, const ChannelPosition* channel_map)
{
	if (op == NULL)
		return -OUTPUT_ERROR_NOT_INITIALIZED;
	return op->pcmOps->Open(sf, channel_map);
}

int op_drop(void)
{
	if (op->pcmOps->Drop == NULL)
		return -OUTPUT_ERROR_NOT_SUPPORTED;
	return op->pcmOps->Drop();
}

int op_close(void)
{
	return op->pcmOps->Close();
}

int op_write(const char *buffer, int count)
{
	return op->pcmOps->Write(buffer, count);
}

int op_pause(void)
{
	if (op->pcmOps->Pause == NULL)
		return 0;
	return op->pcmOps->Pause();
}

int op_unpause(void)
{
	if (op->pcmOps->Unpause == NULL)
		return 0;
	return op->pcmOps->Unpause();
}

int op_buffer_space(void)
{
	return op->pcmOps->BufferSpace();
}

int mixer_set_volume(int left, int right)
{
	if (op == NULL)
		return -OUTPUT_ERROR_NOT_INITIALIZED;
	if (!op->mixerOpen)
		return -OUTPUT_ERROR_NOT_OPEN;
	return op->mixerOps->SetVolume(left, right);
}

int mixer_read_volume(void)
{
	if (op == NULL)
		return -OUTPUT_ERROR_NOT_INITIALIZED;
	if (!op->mixerOpen)
		return -OUTPUT_ERROR_NOT_OPEN;
	return op->mixerOps->GetVolume(&volume_l, &volume_r);
}

int mixer_get_fds(int what, int *fds)
{
	if (op == NULL)
		return -OUTPUT_ERROR_NOT_INITIALIZED;
	if (!op->mixerOpen)
		return -OUTPUT_ERROR_NOT_OPEN;
	switch (op->abiVersion) {
	case 1:
		if (!op->mixerOps->getFds.abi1)
			return -OUTPUT_ERROR_NOT_SUPPORTED;
		if (what != MIXER_FDS_VOLUME)
			return 0;
		return op->mixerOps->getFds.abi1(fds);
	default:
		if (!op->mixerOps->getFds.abi2)
			return -OUTPUT_ERROR_NOT_SUPPORTED;
		return op->mixerOps->getFds.abi2(what, fds);
	}
}

extern int soft_vol;

static void option_error(int rc)
{
	char *msg = op_get_error_msg(rc, "setting option");
	error_msg("%s", msg);
	free(msg);
}

static void set_dsp_option(void *data, const char *val)
{
	const OutputPluginOpt *o = data;
	int rc;

	rc = o->Set (val);
	if (rc)
		option_error(rc);
}

static bool option_of_current_mixer(const MixerPluginOpt* opt)
{
	const MixerPluginOpt* mpo;

	if (!op)
		return false;
	for (mpo = op->mixerOptions; mpo && mpo->name; mpo++) {
		if (mpo == opt)
			return true;
	}
	return false;
}

static void set_mixer_option(void *data, const char *val)
{
	const MixerPluginOpt* o = data;
	int rc;

	rc = o->Set (val);
	if (rc) {
		option_error(rc);
	} else if (option_of_current_mixer(o)) {
		/* option of the current op was set
		 * try to reopen the mixer */
		mixer_close();
		if (!soft_vol)
			mixer_open();
	}
}

static void get_dsp_option(void *data, char *buf, size_t size)
{
	const OutputPluginOpt *o = data;
	char *val = NULL;

	o->Get(&val);
	if (val) {
		strscpy(buf, val, size);
		free(val);
	}
}

static void get_mixer_option(void *data, char *buf, size_t size)
{
	const MixerPluginOpt* o = data;
	char *val = NULL;

	o->Get(&val);
	if (val) {
		strscpy(buf, val, size);
		free(val);
	}
}

void op_add_options(void)
{
	OutputPlugin *o;
	const OutputPluginOpt* opo;
	const MixerPluginOpt* mpo;
	char key[64];

//	list_for_each_entry(o, &op_head, node) {
//		for (opo = o->pcm_options; opo->name; opo++) {
//			snprintf(key, sizeof(key), "dsp.%s.%s", o->name,
//					opo->name);
//			option_add(xstrdup(key), opo, get_dsp_option,
//					set_dsp_option, NULL, 0);
//		}
//		for (mpo = o->mixerOptions; mpo && mpo->name; mpo++) {
//			snprintf(key, sizeof(key), "mixer.%s.%s", o->name,
//					mpo->name);
//			option_add(xstrdup(key), mpo, get_mixer_option,
//					set_mixer_option, NULL, 0);
//		}
//	}
}

char *op_get_error_msg(int rc, const char *arg)
{
	char buffer[1024];

	switch (-rc) {
	case OUTPUT_ERROR_ERRNO:
		snprintf(buffer, sizeof(buffer), "%s: %s", arg, strerror(errno));
		break;
	case OUTPUT_ERROR_NO_PLUGIN:
		snprintf(buffer, sizeof(buffer),
				"%s: no such plugin", arg);
		break;
	case OUTPUT_ERROR_NOT_INITIALIZED:
		snprintf(buffer, sizeof(buffer),
				"%s: couldn't initialize required output plugin", arg);
		break;
	case OUTPUT_ERROR_NOT_SUPPORTED:
		snprintf(buffer, sizeof(buffer),
				"%s: function not supported", arg);
		break;
	case OUTPUT_ERROR_NOT_OPEN:
		snprintf(buffer, sizeof(buffer),
				"%s: mixer is not open", arg);
		break;
	case OUTPUT_ERROR_SAMPLE_FORMAT:
		snprintf(buffer, sizeof(buffer),
				"%s: sample format not supported", arg);
		break;
	case OUTPUT_ERROR_NOT_OPTION:
		snprintf(buffer, sizeof(buffer),
				"%s: no such option", arg);
		break;
	case OUTPUT_ERROR_INTERNAL:
		snprintf(buffer, sizeof(buffer), "%s: internal error", arg);
		break;
	case OUTPUT_ERROR_SUCCESS:
	default:
		snprintf(buffer, sizeof(buffer),
				"%s: this is not an error (%d), this is a bug",
				arg, rc);
		break;
	}
	return xstrdup(buffer);
}

void op_dump_plugins(void)
{
	OutputPlugin *o;

	printf("\nOutput Plugins: %s\n", pluginDir);
//	list_for_each_entry(o, &op_head, node) {
//		printf("  %s\n", o->name);
//	}
}

const char *op_get_current(void)
{
	if (op)
		return op->name;
	return NULL;
}
