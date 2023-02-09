#ifndef CMUS_EXPR_H
#define CMUS_EXPR_H

#include "track_info.h"
#include "list.h"

enum { OP_LT, OP_LE, OP_EQ, OP_GE, OP_GT, OP_NE };
#define NR_OPS (OP_NE + 1)

enum expr_type {
	EXPR_AND,
	EXPR_OR,

	EXPR_NOT,

	EXPR_STR,
	EXPR_INT,
	EXPR_ID,
	EXPR_BOOL
};
#define NR_EXPRS (EXPR_BOOL + 1)

struct expr {
	struct expr *left, *right, *parent;
	enum expr_type type;
	char *key;
	union {
		struct {
			struct list_head glob_head;
			enum {
				SOP_EQ = OP_EQ,
				SOP_NE = OP_NE
			} op;
		} estr;
		struct {
			int val;
			enum {
				IOP_LT = OP_LT,
				IOP_LE = OP_LE,
				IOP_EQ = OP_EQ,
				IOP_GE = OP_GE,
				IOP_GT = OP_GT,
				IOP_NE = OP_NE
			} op;
		} eint;
		struct {
			char* key;
			enum {
				KOP_LT = OP_LT,
				KOP_LE = OP_LE,
				KOP_EQ = OP_EQ,
				KOP_GE = OP_GE,
				KOP_GT = OP_GT,
				KOP_NE = OP_NE
			} op;
		} eid;
	};
};

struct expr *expr_parse(const char *str);
struct expr* expr_parse_i(const char *str, const char *err_msg, int check_short);
int expr_check_leaves(struct expr **exprp, const char *(*get_filter)(const char *name));
int expr_op_to_bool(int res, int op);
int expr_eval(struct expr *expr, struct track_info *ti);
void expr_free(struct expr *expr);
const char *expr_error(void);
int expr_is_short(const char *str);

unsigned int expr_get_match_type(struct expr *expr);
/* "harmless" expressions will reduce filter results when adding characters at the beginning/end */
int expr_is_harmless(const struct expr *expr);

#endif
