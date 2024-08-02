/*
 * Copyright: (c) 2006-2007, 2008 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	vsh.h
 * Purpose:	A very tiny shell skeleton.
 * History:
 *	Apr 30, 2007	jetmotor	Create
 */
 
#ifndef __VSH_H__
#define __VSH_H__


/********************************************************
 *		Notice: debug xsh or not?		*
 ********************************************************/
#define XSH_DEBUG


#include "sys.h"
#include "rbtree.h"
#include "term.h"
#include <string.h>


/**
 * view_t - Shell commands will be grouped by views. That is to say, the shell may have
 *	several views, each of which have several commands.
 *
 * @rb:		used as rb_tree link because views are organized as a rb_tree
 * @name:	view's name, used as key when search views in shell's view tree
 * @prompt:	view's prompt
 * @cmd_tr:	view's commands rb_tree
 */
typedef struct view_s{
	struct rb_node rb;

	char		*name;
	char		*prompt;
	rb_tree_t	cmd_tr;
}view_t;


#define VIEWTAB_ENTRY(name, prompt) \
	{RB_NODE_INIT(), #name, prompt, RB_TREE_INIT()}

extern view_t g_viewtab[];
extern uint32_t nr_viewtab;


#define PARAM_NONE	0x00
/* properties for values of parameters
 */
#define PARAM_INTEGER	0x01
#define PARAM_NUMBER	0x02
#define PARAM_STRING	0x04
#define PARAM_MAC	0x08
#define PARAM_IP	0x10
#define PARAM_LIST	0x20
#define PARAM_OPTIONAL	0x40
#define PARAM_TOGGLE	0x80
#define PARAM_RANGE	0x100
#define PARAM_SUB	0x200
/* types for parameters
 */
#define PARAM_OPT	0x10000
#define PARAM_FLG	0x20000
#define PARAM_ARG	0x40000

#define PARAM_DONE	0x08000000

typedef struct param_s{
	uint32_t	type;
	char		*name;
	char		*help;
	char		*ref;
	struct list_head next;
	struct param_s	*list;
	struct param_s  *n;
}param_t;

#define param_info(param, info) do { \
        if ((param)->type & PARAM_TOGGLE) \
                (info) = (param)->help; \
        else if ((param)->type & PARAM_INTEGER) \
                (info) = "<int>"; \
        else if ((param)->type & PARAM_NUMBER) \
                (info) = "<num>"; \
        else if ((param)->type & PARAM_MAC) \
                (info) = "<mac>"; \
        else if ((param)->type & PARAM_IP) \
                (info) = "<ip>"; \
        else if ((param)->type & PARAM_STRING) \
                (info) = "<str>"; \
        else if ((param)->type & PARAM_RANGE) \
                (info) = "<range>"; \
} while (0)

#define PARAM_EMPTY		{PARAM_NONE, (char *)0, (char *)0, (char *)0, {0,0}, (param_t *)0, (param_t *)0}
extern param_t cmd_none_param_tab[];


typedef struct cmd_desc_s{
	char		*help;
	param_t		*param_tab;
	param_t		*list;
	param_t		*n;
}cmd_desc_t;

#define CMD_DESC_ENTRY(name, h, t)	cmd_desc_t name = {h, t, 0, 0}


struct command_s;
struct xsh_s;
/**
 * action_fp - Function pointer of command action.
 */
typedef void (*action_fp)(struct command_s *cmd, struct xsh_s *xsh);

/**
 * command_t - Each command corresponds to an action that an operator could type on a CLI
 *	interface to accomplish some operation. Each also has a current view and a next
 *	view. After invoking some command's action, shell's view may change from current
 *	view into the next view, such as `exit' etc..
 *
 * @rb:		used as rb_tree link because commands are organized as a rb_tree
 * @link:	matched commands link when shell perform tab expansion
 * @name:	command's name, used as key when search commands in view's command tree
 * @len:	length of name
 * @action:	command action function, invoked when shell matches this command
 */
typedef struct command_s{
	struct rb_node rb;
	struct list_head link;
	
	char		*name;
	char		*suffix;
	uint32_t	len;

	action_fp	action;
	cmd_desc_t	*desc;
}command_t;


#define CMDTAB_ENTRY(nm, action, desc) \
	{RB_NODE_INIT(), {0,0}, #nm, 0, 0, action, desc}


/**
 * histent_t - A place to record characters that term receives after each carriage return stroke 
 *	occurs. Notice that the buffer in hisent_t is of the same length with buffer in term_t.
 *
 * @link:	list link because all the histent_t are linked as a list
 * @line:	the buffer
 * @line_size:	actual data length in the buffer
 */
typedef struct histent_s{
	struct list_head link;
	
	char line[TTY_BUF_SIZE_MAX];
	int32_t line_size;
}histent_t;


#define NR_HISTENT_MAX		8
#define NR_ARG_MAX		32
#define TASK_STK_SIZE_XSH	512


/**
 * xsh_t - `xsh' is a CLI interface through which an operator could perform operating & management
 *	activities. Currently, xsh supports different views of commands, emacs-like line edition,
 *	tab expansion and help infomation invokation. Although the implementaion is quite a little
 *	ugly, xsh does work very well anyway.
 *
 * @root:	root view, the first view that xsh uses after starting
 * @curr:	current working view
 * @term:	the term that xsh uses
 * @prompt:
 * @line:
 * @line_size:
 * @opts:	points to the command options if it does have one
 * @exec_pid:
 * @exec:	the resolved exact command that xsh find in its current view
 * @argc:	number of arguments
 * @argv:	arrary of arguments, including command name
 * @arg2desc:
 * @match:	list head for all possible commands when xsh try to perform tab expansion
 * @nr_match:	number of match commands
 * @histent:
 * @hbeg:
 * @hend:
 * @hist:
 */
typedef struct xsh_s{
	view_t *root;
	view_t *curr;

	tty_t *term;
	char prompt[TTY_BUF_SIZE_MAX];
	int32_t prompt_len;
	char session[TTY_BUF_SIZE_MAX/2];
	int32_t session_len;

	int32_t max_x, max_y;
	char line[TTY_BUF_SIZE_MAX];
	int32_t line_size;
	char *opts;
	
	int32_t nr_exec_ent;
	struct list_head exec_ent_list;

	int32_t _argc;
	command_t *exec;
	int32_t argc;
	char *argv[NR_ARG_MAX+1];
	param_t *arg2param[NR_ARG_MAX+1];
	char *arg2tog[NR_ARG_MAX+1];

	struct list_head match;
	uint32_t nr_match;

	histent_t histent[NR_HISTENT_MAX];
	struct list_head *hbeg;
	struct list_head *hend;
	struct list_head *hist;

	uint32_t task_xsh_stk[TASK_STK_SIZE_XSH];
}xsh_t;

extern xsh_t g_vsh;


#define XSH_PANIC()     sys_panic("<XSH panic> %s : %d\n", __func__, __LINE__)


extern void vsh_init(tty_t *term);
extern void task_xsh(void *arg);

extern void xsh_push_hist(xsh_t *xsh, char *s);
extern int32_t xsh_resolve_cmd(xsh_t *xsh);
extern void xsh_update_view(xsh_t *xsh, view_t *view);
extern void xsh_update_session(xsh_t *xsh, char *session);
extern char * xsh_getopt(xsh_t *xsh);
extern char * xsh_getopt_val(char *arg, char *opt);

extern void xsh_dokey_tab(void *context);
extern void xsh_dokey_help(void *context);
extern void xsh_dokey_previous(void *context);
extern void xsh_dokey_next(void *context);

extern int32_t command_cmp_ws(struct rb_node *n, void *key);
extern int32_t strprefix(char *s, char *t);
extern void print_arrow(int32_t len, xsh_t *xsh);

#define VIEW_TABLE_DEFINE_BEG() view_t g_viewtab[] = {
#define VIEW_TABLE_DEFINE_END() }; \
	uint32_t nr_viewtab = sizeof(g_viewtab) / sizeof(g_viewtab[0]);

#define CMD_TABLE_DEFINE_BEG() command_t g_cmdtab[] = {
#define CMD_TABLE_DEFINE_END() }; \
	uint32_t nr_cmdtab = sizeof(g_cmdtab)/sizeof(g_cmdtab[0]);

extern command_t g_cmdtab[];
extern uint32_t nr_cmdtab;

/**
 * return OK
 * return ERROR
 */
extern int32_t vsh_add_cmd(command_t *cmd);

#endif	/* end of __VSH_H__ */
