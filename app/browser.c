#include "browser.h"

#include "log.h"
#include "file.h"
#include "misc.h"
#include "uchar.h"
#include "options.h"
#include "xmalloc.h"
#include "load_dir.h"
#include "curses-main.h"
#include "graceful-music.h"

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <gio/gio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>

struct window *browser_win;
struct searchable *browser_searchable;
char*                       gBrowserDir;

static LIST_HEAD(browser_head);

static inline void browser_entry_to_iter(struct browser_entry *e, struct iter *iter)
{
	iter->data0 = &browser_head;
	iter->data1 = e;
	iter->data2 = NULL;
}

/* filter out names starting with '.' except '..' */
static int normal_filter(const char *name, const struct stat *s)
{
	if (name[0] == '.') {
		if (name[1] == '.' && name[2] == 0)
			return 1;
		return 0;
	}
	if (S_ISDIR(s->st_mode))
		return 1;
	return gm_is_supported(name);
}

/* filter out '.' */
static int hidden_filter(const char *name, const struct stat *s)
{
	if (name[0] == '.' && name[1] == 0)
		return 0;
	return 1;
}

/* only works for BROWSER_ENTRY_DIR and BROWSER_ENTRY_FILE */
static int entry_cmp(const struct browser_entry *a, const struct browser_entry *b)
{
	if (a->type == BROWSER_ENTRY_DIR) {
		if (b->type == BROWSER_ENTRY_FILE)
			return -1;
		if (!strcmp(a->name, "../"))
			return -1;
		if (!strcmp(b->name, "../"))
			return 1;
		return strcmp(a->name, b->name);
	}
	if (b->type == BROWSER_ENTRY_DIR)
		return 1;
	return strcmp(a->name, b->name);
}

static char *fullname(const char *path, const char *name)
{
	int l1, l2;
	char *full;

	l1 = strlen(path);
	l2 = strlen(name);
	if (path[l1 - 1] == '/')
		l1--;
	full = xnew(char, l1 + 1 + l2 + 1);
	memcpy(full, path, l1);
	full[l1] = '/';
	memcpy(full + l1 + 1, name, l2 + 1);
	return full;
}

static void free_browser_list(void)
{
	struct list_head *item;

	item = browser_head.next;
	while (item != &browser_head) {
		struct list_head *next = item->next;
		struct browser_entry *entry;

		entry = list_entry(item, struct browser_entry, node);
		free(entry);
		item = next;
	}
	list_init(&browser_head);
}

static int add_pl_line(void *data, const char *line)
{
	struct browser_entry *e;
	int name_size = strlen(line) + 1;

	e = xmalloc(sizeof(struct browser_entry) + name_size);
	memcpy(e->name, line, name_size);
	e->type = BROWSER_ENTRY_PLLINE;
	list_add_tail(&e->node, &browser_head);
	return 0;
}

static int do_browser_load(const char* name)
{
	struct stat st;

	if (stat(name, &st)) {
        return -1;
    }

    if (S_ISREG(st.st_mode) && gm_is_playlist(name)) {
		char *buf;
		ssize_t size;

		buf = mmap_file(name, &size);
		if (size == -1)
			return -1;

		free_browser_list();

		if (buf) {
			struct browser_entry *parent_dir_e = xmalloc(sizeof(struct browser_entry) + 4);
			strcpy(parent_dir_e->name, "../");
			parent_dir_e->type = BROWSER_ENTRY_DIR;
			list_add_tail(&parent_dir_e->node, &browser_head);

			gm_playlist_for_each(buf, size, 0, add_pl_line, NULL);
			munmap(buf, size);
		}
    }
    else if (S_ISDIR(st.st_mode)) {
		int (*filter) (const char *, const struct stat *) = normal_filter;
		struct directory dir;
		const char *str;
		int root = !strcmp(name, "/");

		if (show_hidden) {
            filter = hidden_filter;
        }

		if (dir_open(&dir, name)) {
            return -1;
        }

		free_browser_list();
		while ((str = dir_read(&dir))) {
			struct browser_entry *e;
			struct list_head *item;
			int len;

			if (!filter(str, &dir.st)) {
                continue;
            }

			/* ignore .. if we are in the root dir */
			if (root && !strcmp(str, "..")) {
                continue;
            }

			len = (int) strlen (str);
			e = xmalloc(sizeof(struct browser_entry) + len + 2);
			e->type = BROWSER_ENTRY_FILE;
			memcpy(e->name, str, len);
			if (S_ISDIR(dir.st.st_mode)) {
				e->type = BROWSER_ENTRY_DIR;
				e->name[len++] = '/';
			}
			e->name[len] = 0;

			item = browser_head.prev;
			while (item != &browser_head) {
				struct browser_entry *other;

				other = container_of(item, struct browser_entry, node);
				if (entry_cmp(e, other) >= 0)
					break;
				item = item->prev;
			}
			/* add after item */
			list_add(&e->node, item);
		}
		dir_close(&dir);

		/* try to update currect working directory */
		if (chdir(name))
			return -1;
	}
    else {
		errno = ENOTDIR;
		return -1;
	}
	return 0;
}

static int browser_load (const char *name)
{
	int rc;

	rc = do_browser_load(name);
	if (rc) {
        LOG_WARNING("load dir '%s' error!", name);
        return rc;
    }

	window_set_contents(browser_win, &browser_head);
	free(gBrowserDir);
	gBrowserDir = g_strdup (name);
	return 0;
}

//static GENERIC_ITER_PREV(browser_get_prev, struct browser_entry, node)
//static GENERIC_ITER_NEXT(browser_get_next, struct browser_entry, node)


static int browser_get_next (GList*)
{
    return 0;
}

static int browser_get_prev (GList*)
{
    return 0;
}

static int browser_search_get_current(void *data, GList* iter)
{
    return 0;
//	return window_get_sel(browser_win, iter);
}

static int browser_search_matches(void *data, GList* iter, const char *text)
{
    return 0;
//	char **words = get_words(text);
//	int matched = 0;
//
//	if (words[0] != NULL) {
//		struct browser_entry *e;
//		int i;
//
//		e = iter_to_browser_entry(iter);
//		for (i = 0; ; i++) {
//			if (words[i] == NULL) {
//				window_set_sel(browser_win, iter);
//				matched = 1;
//				break;
//			}
//			if (u_strcasestr_filename(e->name, words[i]) == NULL)
//				break;
//		}
//	}
//	free_str_array(words);
//	return matched;
}

static const SearchableOptions browser_search_ops = {
	.GetPrev = browser_get_prev,
	.GetNext = browser_get_next,
	.GetCurrent = browser_search_get_current,
	.Matches = browser_search_matches
};

void browser_init(void)
{
	struct iter             iter;
	char                    cwd[1024] = {0};
	g_autofree char*        dir = NULL;

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        dir = g_strdup ("/");
    }
    else {
		dir = g_strdup (cwd);
    }

	if (do_browser_load(dir)) {
		do_browser_load("/");
		gBrowserDir = g_strdup ("/");
	} else {
		gBrowserDir = g_strdup (dir);
	}

	browser_win = window_new(browser_get_prev, browser_get_next);
	window_set_contents(browser_win, &browser_head);
	window_changed(browser_win);

	iter.data0 = &browser_head;
	iter.data1 = NULL;
	iter.data2 = NULL;
	browser_searchable = searchable_new(NULL, &iter, &browser_search_ops);
}

void browser_exit(void)
{
	searchable_free(browser_searchable);
	free_browser_list();
	window_free(browser_win);
	free(gBrowserDir);
}

int browser_chdir(const char *dir)
{
	if (browser_load(dir)) {
	}
	return 0;
}

void browser_up(void)
{
	char *new, *ptr, *pos;
	struct browser_entry *e;
	int len;

	if (strcmp(gBrowserDir, "/") == 0)
		return;

	ptr = strrchr(gBrowserDir, '/');
	if (ptr == gBrowserDir) {
		new = xstrdup("/");
	} else {
		new = xstrndup(gBrowserDir, ptr - gBrowserDir);
	}

	/* remember old position */
	ptr++;
	len = strlen(ptr);
	pos = xstrdup(ptr);

	errno = 0;
	if (browser_load(new)) {
		if (errno == ENOENT) {
			free(pos);
			free(gBrowserDir);
			gBrowserDir = new;
			browser_up();
			return;
		}
		error_msg("could not open directory '%s': %s\n", new, strerror(errno));
		free(new);
		free(pos);
		return;
	}
	free(new);

	/* select old position */
	list_for_each_entry(e, &browser_head, node) {
		if (strncmp(e->name, pos, len) == 0 &&
		    (e->name[len] == '/' || e->name[len] == '\0')) {
			struct iter iter;

			browser_entry_to_iter(e, &iter);
			window_set_sel(browser_win, &iter);
			break;
		}
	}
	free(pos);
}

static void browser_cd(const char *dir)
{
	char *new;
	int len;

	if (strcmp(dir, "../") == 0) {
		browser_up();
		return;
	}

	new = fullname(gBrowserDir, dir);
	len = strlen(new);
	if (new[len - 1] == '/')
		new[len - 1] = 0;
	if (browser_load(new))
		error_msg("could not open directory '%s': %s\n", dir, strerror(errno));
	free(new);
}

static void browser_cd_playlist(const char *filename)
{
	if (browser_load(filename))
		error_msg("could not read playlist '%s': %s\n", filename, strerror(errno));
}

void browser_enter(void)
{
	struct browser_entry *e;
	struct iter sel;
	int len;

	if (!window_get_sel(browser_win, &sel))
		return;
	e = iter_to_browser_entry(&sel);
	len = strlen(e->name);
	if (len == 0)
		return;
	if (e->type == BROWSER_ENTRY_DIR) {
		browser_cd(e->name);
	} else {
		if (e->type == BROWSER_ENTRY_PLLINE) {
			gm_play_file(e->name);
		} else {
			char *filename;

			filename = fullname(gBrowserDir, e->name);
			if (gm_is_playlist(filename)) {
				browser_cd_playlist(filename);
			} else {
				gm_play_file(filename);
			}
			free(filename);
		}
	}
}

char *browser_get_sel(void)
{
	struct browser_entry *e;
	struct iter sel;

	if (!window_get_sel(browser_win, &sel))
		return NULL;

	e = iter_to_browser_entry(&sel);
	if (e->type == BROWSER_ENTRY_PLLINE)
		return xstrdup(e->name);

	return fullname(gBrowserDir, e->name);
}

void browser_delete(void)
{
	struct browser_entry *e;
	struct iter sel;
	int len;

	if (!window_get_sel(browser_win, &sel))
		return;
	e = iter_to_browser_entry(&sel);
	len = strlen(e->name);
	if (len == 0)
		return;
	if (e->type == BROWSER_ENTRY_FILE) {
		char *name;

		name = fullname(gBrowserDir, e->name);
		if (yes_no_query("Delete file '%s'? [y/N]", e->name) == UI_QUERY_ANSWER_YES) {
			if (unlink(name) == -1) {
				error_msg("deleting '%s': %s", e->name, strerror(errno));
			} else {
				window_row_vanishes(browser_win, &sel);
				list_del(&e->node);
				free(e);
			}
		}
		free(name);
	}
}

void browser_reload(void)
{
	char *tmp = g_strdup(gBrowserDir);
	char *sel = NULL;
	struct iter iter;
	struct browser_entry *e;

	/* remember selection */
	if (window_get_sel(browser_win, &iter)) {
		e = iter_to_browser_entry(&iter);
		sel = xstrdup(e->name);
	}

	/* have to use tmp  */
	if (browser_load(tmp)) {
		error_msg("could not update contents '%s': %s\n", tmp, strerror(errno));
		free(tmp);
		free(sel);
		return;
	}

	if (sel) {
		/* set selection */
		list_for_each_entry(e, &browser_head, node) {
			if (strcmp(e->name, sel) == 0) {
				browser_entry_to_iter(e, &iter);
				window_set_sel(browser_win, &iter);
				break;
			}
		}
	}

	free(tmp);
	free(sel);
}
