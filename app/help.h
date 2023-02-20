#ifndef CMUS_HELP_H
#define CMUS_HELP_H

#include "list.h"
#include "keys.h"
#include "window.h"
#include "search.h"
#include "options.h"

struct help_entry {
	struct list_head node;
	enum {
		HE_TEXT,		/* text entries 	*/
		HE_BOUND,		/* bound keys		*/
		HE_UNBOUND,		/* unbound commands	*/
		HE_OPTION,
	} type;
	union {
		const char *text;			/* HE_TEXT	*/
		const struct binding *binding;		/* HE_BOUND	*/
		const struct command *command;		/* HE_UNBOUND	*/
		const Option *option;
	};
};

static inline struct help_entry *iter_to_help_entry(struct iter *iter)
{
	return iter->data1;
}

extern struct window *help_win;
extern struct searchable *help_searchable;

void help_select(void);
void help_toggle(void);
void help_remove(void);

void help_add_bound(const struct binding *bind);
void help_remove_bound(const struct binding *bind);
void help_remove_unbound(struct command *cmd);
void help_add_unbound(struct command *cmd);
void help_add_all_unbound(void);

void help_init(void);
void help_exit(void);

#endif /* HELP_H */
