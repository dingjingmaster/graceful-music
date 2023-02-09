#ifndef CMUS_RBTREE_H
#define CMUS_RBTREE_H

#include "compiler.h" /* container_of */
#include <stddef.h>

struct rb_node
{
	unsigned long  rb_parent_color;
#define	RB_RED		0
#define	RB_BLACK	1
	struct rb_node *rb_right;
	struct rb_node *rb_left;
} __attribute__((aligned(sizeof(long))));
	/* The alignment might seem pointless, but allegedly CRIS needs it */

struct rb_root
{
	struct rb_node *rb_node;
};


#define rb_parent(r)   ((struct rb_node *)((r)->rb_parent_color & ~3))
#define rb_color(r)   ((r)->rb_parent_color & 1)
#define rb_is_red(r)   (!rb_color(r))
#define rb_is_black(r) rb_color(r)
#define rb_set_red(r)  do { (r)->rb_parent_color &= ~1; } while (0)
#define rb_set_black(r)  do { (r)->rb_parent_color |= 1; } while (0)

static inline void rb_set_parent(struct rb_node *rb, struct rb_node *p)
{
	rb->rb_parent_color = (rb->rb_parent_color & 3) | (unsigned long)p;
}
static inline void rb_set_color(struct rb_node *rb, int color)
{
	rb->rb_parent_color = (rb->rb_parent_color & ~1) | color;
}

#define RB_ROOT	(struct rb_root) { NULL, }
#define	rb_entry(ptr, type, member) container_of(ptr, type, member)

#define RB_EMPTY_ROOT(root)	((root)->rb_node == NULL)
#define RB_EMPTY_NODE(node)	(rb_parent(node) == node)
#define RB_CLEAR_NODE(node)	(rb_set_parent(node, node))

void rb_insert_color(struct rb_node *, struct rb_root *);
void rb_erase(struct rb_node *, struct rb_root *);

/* Find logical next and previous nodes in a tree */
struct rb_node *rb_next(const struct rb_node *);
struct rb_node *rb_prev(const struct rb_node *);
struct rb_node *rb_first(const struct rb_root *);
struct rb_node *rb_last(const struct rb_root *);

/* Fast replacement of a single node without remove/rebalance/add/rebalance */
void rb_replace_node(struct rb_node *victim, struct rb_node *new,
			struct rb_root *root);

static inline void rb_link_node(struct rb_node * node, struct rb_node * parent,
				struct rb_node ** rb_link)
{
	node->rb_parent_color = (unsigned long)parent;
	node->rb_left = node->rb_right = NULL;

	*rb_link = node;
}


/* Cmus extensions */

static inline void rb_root_init(struct rb_root *root)
{
	root->rb_node = NULL;
}

static inline int rb_root_empty(struct rb_root *root)
{
	return RB_EMPTY_ROOT(root);
}

/**
 * rb_for_each        -       iterate over a rbtree
 * @pos:        the &struct rb_node to use as a loop counter.
 * @root:       the root for your rbtree.
 */
#define rb_for_each(pos, root) \
	for (pos = rb_first(root); pos; pos = rb_next(pos))

/**
 * rb_for_each_prev   -       iterate over a rbtree backwards
 * @pos:        the &struct rb_node to use as a loop counter.
 * @root:       the root for your rbtree.
 */
#define rb_for_each_prev(pos, root) \
	for (pos = rb_last(root); pos; pos = rb_prev(pos))

/**
 * rb_for_each_safe   -       iterate over a rbtree safe against removal of rbtree node
 * @pos:        the &struct rb_node to use as a loop counter.
 * @n:          another &struct rb_node to use as temporary storage
 * @root:       the root for your rbtree.
 */
#define rb_for_each_safe(pos, n, root) \
	for (pos = rb_first(root), n = pos ? rb_next(pos) : NULL; pos; \
		pos = n, n = pos ? rb_next(pos) : NULL)

/**
 * rb_for_each_entry        -       iterate over a rbtree of given type
 * @pos:        the &struct rb_node to use as a loop counter.
 * @t:          the type * to use as a loop counter.
 * @root:       the root for your rbtree.
 * @member:	the name of the rb_node-struct within the struct.
 */
#define rb_for_each_entry(t, pos, root, member) \
	for (pos = rb_first(root), t = pos ? rb_entry(pos, __typeof__(*t), member) : NULL; \
		pos; pos = rb_next(pos), t = pos ? rb_entry(pos, __typeof__(*t), member) : NULL)

/**
 * rb_for_each_entry_reverse        -       iterate backwards over a rbtree of given type
 * @pos:        the &struct rb_node to use as a loop counter.
 * @t:          the type * to use as a loop counter.
 * @root:       the root for your rbtree.
 * @member:	the name of the rb_node-struct within the struct.
 */
#define rb_for_each_entry_reverse(t, pos, root, member) \
	for (pos = rb_last(root), t = pos ? rb_entry(pos, __typeof__(*t), member) : NULL; \
		pos; pos = rb_prev(pos), t = pos ? rb_entry(pos, __typeof__(*t), member) : NULL)

#endif	/* _LINUX_RBTREE_H */
