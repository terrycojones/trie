/* $Header: /user1/terry/s/trie/RCS/trie.h,v 2.0 90/04/25 13:41:27 terry Exp $ */

/*
 * $Log:	trie.h,v $
 * Revision 2.0  90/04/25  13:41:27  terry
 * Stable for first real test with canvas.
 * 
 * Revision 1.10  90/04/24  20:05:35  terry
 * Fixed silliness.
 * 
 * Revision 1.9  90/04/24  19:50:16  terry
 * Made rparse use a list instead of a table.
 * 
 * Revision 1.8  90/04/24  16:54:35  jkh
 * Clean-up to make things work with the new stuff. Fun fun fun.
 * 
 * Revision 1.7  90/04/24  15:34:15  terry
 * Lots of cleanup for integration with the rest of this stuff.
 * Moved definitions of entry points into common.h removed parse_add
 * and parse_sub macros.
 * 
 * Revision 1.6  90/04/18  17:24:26  terry
 * check in so jordan can take it home.
 * 
 * Revision 1.5  90/04/18  00:01:33  terry
 * Changes for reverse parsing go in.
 * 
 * Revision 1.4  90/04/16  22:49:08  terry
 * Added stuff for the reverse parser.
 * 
 * Revision 1.3  90/04/10  21:12:39  terry
 * Changes to move the special info to a transition instead
 * of a state. added new handle states and the two new 
 * special transition types.
 * 
 * Revision 1.2  90/03/26  22:26:10  terry
 * Things are stable pretty much. This goes with trie.c revision 1.2.
 * The interface stuff needs to be properly defined and added.
 * 
 * Revision 1.1  90/03/14  18:18:15  terry
 * Initial revision
 * 
 *
 */

/* A request ID node. Just holds an opcode. */
typedef struct _request {
    int request_id;
    struct _request* next;
} Request;

/* A special action node. Holds the action type and an argument. */
typedef struct _Special {
    int type;
    int value;
    struct _Special *next;
} Special;

/* A state in the trie. */
typedef struct {
    int final;
    Generic transition_list;       /* This will point to a Transition node. */
    Request *request_list;
#ifdef TRIE_TESTING
    int id;
#endif
} State;

/* A transition in the trie. */
typedef struct _Transition {
    int label;
    State *state;
    Special *special;
    struct _Transition *next;
    int valency;
#ifdef TRIE_TESTING
    int id;
#endif
} Transition;

/* A reverse parser list element. */
typedef struct _Rparse {
    int id;
    String s;
    int length;
    struct _Rparse *next;
} Rparse;

#define STATE_NULL            (State *)0
#define TRANS_NULL            (Transition *)0
#define SPEC_NULL             (Special *)0
#define REQ_NULL              (Request *)0
#define RPARSE_NULL           (Rparse *)0

/* Special 'characters' for the %d and %s transitions. */
/* These values must be outside the range of normal chars. */
#define READ_INT_TRAN         666
#define READ_STR_TRAN         999

/* Actions for the special transitions. */
#define META_READ_INT         0
#define META_READ_STR         1
#define META_PUSH             2
#define META_ADD              3
#define META_SUB              4
#define META_MUL              5
#define META_DIV              6
#define META_MOD              7
#define META_POP_TO_REG       8
#define META_GET              9

/* Return values for handle(). */
#define RECOGNISED            0
#define IN_PROGRESS           1
#define FLUSH                 2

/* Internal states for handle(). */
#define HANDLE_NORMAL         0
#define HANDLE_READ_INT       1
#define HANDLE_READ_STR       2

/* Stuff for communication between parse_add, parse_sub and parse_adjust. */
#define PARSE_ADD             0
#define PARSE_SUB             1

Import Request *add_request();
Import Request *get_request();
Import Request *get_request_list();
Import State *add_transition();
Import State *follow_transition();
Import Transition *has_transition();
Import String pop();
Import char *read_int();
Import int next_char();
Import int requests_negative();
Import int requests_positive();
Import int same_requests();
Import void add_string();
Import void adjust_valencies();
Import void delete_string();
Import void emit();
Import void handle_init();
Import void handle_special();
Import void parse_adjust();
Import void parse_error();
Import void preserve_current_state();
Import void push();
Import void reverse_add();
Import void reverse_sub();
Import Rparse *rparse_find();

#ifdef TRIE_TESTING
#ifdef MALLOC_TEST
#define malloc Malloc
#define free Free
Import char *Malloc();
Import void Free();
#endif /* MALLOC_TEST */
#include <stdio.h>
Import Request *exactly_in_trie();
Import int req_count;
Import int spec_count;
Import int state_count;
Import int trans_count;
Import int transition_count();
Import void parse_file();
#define delete_file(file)     parse_file((file), PARSE_SUB)
#define add_file(file)        parse_file((file), PARSE_ADD)
Import void prefix_in_trie();
Import void print_requests();
Import void print_reverse();
Import void print_special();
Import void print_state();
Import void print_transition();
Import void print_trie();
Import void trie_memory();
#endif
