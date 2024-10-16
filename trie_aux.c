

/*
 * AUXILLARY stuff for trie.c
 * #included if TRIE_TESTING is defined.
 *
 * These are all called by try.c
 *
 */

#include "common.h"

/*
 * parse_file(fp)
 *
 * Open and read the contents of 'file', adding or deleting each new
 * line to/from the trie.
 */
void
parse_file(file, what)
char *file;
int what;
{
    FILE *fopen(), *fp;
    int fclose();
    char line[BUF_SZ + 1];

    if (!file){
	parse_error("parse file called with null pointer.");
    }

    if (!(fp = fopen(file, "r"))){
	parse_error("Could not open file '%s'.", file);
    }

    while (fgets(line, BUF_SZ, fp)){
	if (what == PARSE_ADD){
	    parse_add(line);
	}
	else {
	    parse_sub(line);
	}
    }
    
    return;
}


/*
 * exactly_in_trie(s)
 *
 * Search the trie and see if 's' is there. We return the
 * request ID associated with 's'. This will be non-zero if
 * 's' was given (as oppsed to 's' being the prefix of something else).
 * A bit flaky, but it's only a testing function.
 */
Request *
exactly_in_trie(s)
caddr_t s;
{
    State *state = zero_state;

#ifdef TRIE_ARG_CHECKING
    if (!s){
	parse_error("In trie called with null pointer.");
    }
#endif /* TRIE_ARG_CHECKING */

    while (*s){
	if (!(state = follow_transition(state, (int)*s))){
	    return 0;
	}
	s++;
    }
    
    return state->request_list;
}

/*
 * prefix_in_trie(s)
 *
 * Follow 's' through the trie.
 * This tells us how many ways there are to continue on from 's'.
 */
void
prefix_in_trie(s)
caddr_t s;
{
    State *state = zero_state;
    Transition *t;
    /* int count = 0; */

#ifdef TRIE_ARG_CHECKING
    if (!s){
	parse_error("In trie called with null pointer.");
    }
#endif /* TRIE_ARG_CHECKING */

    while (*s){
	int len = 0;
	int ch = next_char(s, &len);
	s += len;
	if (!(state = follow_transition(state, ch))){
	    return;
	}
    }
    
    t = (Transition *)state->transition_list;

    while (t){
	printf("%c (%d times)", t->label, t->valency);
	/* count += t->valency; */
	t = t->next;
    }
    return;
}

/*
 * trie_memory()
 *
 * Print out statistics on memory usage.
 */
void
trie_memory()
{
    int state_size = sizeof(State);
    int trans_size = sizeof(Transition);
    int spec_size = sizeof(Special);
    int req_size = sizeof(Special);
    
    int state_total = state_count * state_size;
    int trans_total = trans_count * trans_size;
    int spec_total = spec_count * spec_size;
    int req_total = req_count * req_size;
    
    printf("%d states allocated @ %d bytes/state = %d bytes.\n", state_count, state_size, state_total);
    printf("%d transitions allocated @ %d bytes/transition = %d bytes.\n", trans_count, trans_size, trans_total);
    printf("%d special transitions allocated @ %d bytes/transition = %d bytes.\n", spec_count, spec_size, spec_total);
    printf("%d requests allocated @ %d bytes/request = %d bytes.\n", req_count, req_size, req_total);
    printf("total allocation is %d bytes\n", state_total + trans_total + spec_total + req_total);
    return;
}


/*
 * dispatch(cb)
 *
 * Print out the fact that we have recognised the escape 
 * sequence with the request ID given.
 */
void
dispatch(cb)
ComBlock *cb;
{
    void print_regs();
    void print_stack();
    
    printf("recognised ID %2d = ", cb->opcode);
    
    switch (cb->opcode){
	case OP_INSERT:{
	    register int i; 
	    printf("self_insert %d char%s: '", cb->nbytes, cb->nbytes != 1 ? "s" : "");
	    for (i = 0; i < cb->nbytes; i++){
		fputc(cb->buffer[i], stdout);
	    } 
	    printf("'\n");
	    break;
	}
	case OP_MOVE_ABS:{
	    printf("OP_MOVE_ABS : ");
	    print_regs("xy", &com_block);
	    break;
	}
	case OP_MOVE_REL:{
	    printf("OP_MOVE_REL : ");
	    print_regs("xy", &com_block);
	    break;
	}
	case OP_MOVE_ABS_COLUMN:{
	    printf("OP_MOVE_ABS_COLUMN : ");
	    print_regs("x", &com_block);
	    break;
	}
	case OP_MOVE_ABS_ROW:{
	    printf("OP_MOVE_ABS_ROW : ");
	    print_regs("y", &com_block);
	    break;
	}
	case OP_MOVE_REL_COLUMN:{
	    printf("OP_MOVE_REL_COLUMN : ");
	    print_regs("x", &com_block);
	    break;
	}
	case OP_MOVE_REL_ROW:{
	    printf("OP_MOVE_REL_ROW : ");
	    print_regs("y", &com_block);
	    break;
	}
	case OP_INSERT_MODE:{
	    printf("OP_INSERT_MODE : ");
	    printf("\n");
	    break;
	}
	case OP_OVERWRITE_MODE:{
	    printf("OP_OVERWRITE_MODE : ");
	    printf("\n");
	    break;
	}
	case OP_DELETE_CHARS:{
	    printf("OP_DELETE_CHARS : ");
	    print_regs("a", &com_block);
	    break;
	}
	case OP_DELETE_TO_EOL:{
	    printf("OP_DELETE_TO_EOL : ");
	    printf("\n");
	    break;
	}
	case OP_DELETE_LINES:{
	    printf("OP_DELETE_LINES : ");
	    print_regs("a", &com_block);
	    break;
	}
	case OP_DELETE_TO_EOSCR:{
	    printf("OP_DELETE_TO_EOSCR : ");
	    printf("\n");
	    break;
	}
	case OP_INSERT_BLANKS:{
	    printf("OP_INSERT_BLANKS : ");
	    print_regs("a", &com_block);
	    break;
	}
	case OP_INSERT_LINES:{
	    printf("OP_INSERT_LINES : ");
	    print_regs("a", &com_block);
	    break;
	}
	case OP_CLEAR_SCREEN:{
	    printf("OP_CLEAR_SCREEN : ");
	    printf("\n");
	    break;
	}
	case OP_SET_SCROLL_REGION:{
	    printf("OP_SET_SCROLL_REGION : ");
	    print_regs("ab", &com_block);
	    break;
	}
	case OP_MOVE_REL_ROW_SCROLLED:{
	    printf("OP_MOVE_REL_ROW_SCROLLED : ");
	    print_regs("y", &com_block);
	    break;
	}

	default:{
	    printf("(unknown): ", cb->opcode);
	    print_regs("abcxyz", &com_block);
	    break;
	}
    }
    return;
}


/*
 * print_regs(regs, cb)
 *
 * Print the contents of some registers.
 */
void
print_regs(regs, cb)
char *regs;
ComBlock *cb;
{
    register int i; 

    if (!regs)
	return;

    if (!cb)
	 cb = &com_block;

    for (i = 'a'; i <= 'z'; i++){
	if (strchr(regs, i)){
	    switch (cb->regs[i - 'a'].type){
		case CB_INT_TYPE:{
		    printf("  %c=%d", i, (int)(cb->regs[i - 'a'].data));
		break;
		}
		case CB_CHAR_TYPE:{
		    printf("  %c=%c", i, (char)(cb->regs[i - 'a'].data));
		    break;
		}
		case CB_STR_TYPE:{
		    printf("  %c=%s", i, (char *)(cb->regs[i - 'a'].data));
		    break;
		}
		default:{
		    fprintf(stderr, "strnge TYPE (%d) in com block register %d\n", cb->regs[i - 'a'].type, i - 'a');
		    exit(1); 
		}
	    }
	} 
    }
    printf("\n");

    return;
}


/*
 * print_stack()
 *
 * Print the contents of the stack.
 */
void
print_stack()
{
    register int i = sp;

    if (i >= 0){
	printf("stack contents : ");
	while (i >= 0){
	    printf("%d ", (int)(stack[i--]));
	} 
	printf("\n");
    }
    else{
	printf("empty stack\n");
    }
}


/* Stuff for the printing of the trie. */

int indent_level;
char *indent = "  ";
#define in_print {register int i; for (i = 0; i < indent_level; i++) fprintf(fp, indent);} fprintf

/*
 * print_trie()
 *
 * Print out the contents of the trie.
 *
 */
void
print_trie(fp)
FILE *fp;
{
    if (!zero_state){
	printf("empty trie.\n");
	return;
    }
    indent_level = -1;
    print_state(fp, zero_state);
    return;
}


/*
 * print_state(fp, s)
 *
 * Print out the contents of the state.
 * We call ourselves indirectly by calling print_transition for
 * each transition we have.
 *
 */
void
print_state(fp, s)
FILE *fp;
State *s;
{
    if (!s){
	fprintf(stderr, "print_state called with NULL!\n");
	exit(1); 
    }

    if (s->transition_list && s->final){
	in_print(fp, "oops! state %d has a transition list and is also a final state!\n", s->id);
	exit(1); 
    }
    indent_level++;
    
    in_print(fp, "STATE %d\n", s->id);
    
    if (s->transition_list){
	int valency;
	int count = 0;
	Transition *t = (Transition *)s->transition_list;
	
	/* Print the number and label of each transition. */
	valency = transition_count(s);
	in_print(fp, "%d transition%s:\n", valency, valency > 1 ? "s" : "");
	while (t){
	    count++;
	    switch (t->label){
		case READ_INT_TRAN:{
		    in_print(fp, "READ_INT : transition %d\n", t->id);
		    break;
		}
		case READ_STR_TRAN:{
		    in_print(fp, "READ_STR : transition %d\n", t->id);
		    break;
		}
		default:{
		    in_print(fp, "read '%c' : follow transition %d\n", t->label, t->id);
		    break;
		}
	    }
	    t = t->next;
	}
	fprintf(fp, "\n");
	if (count != valency){
	    in_print(fp, "oops! state %d has valency %d but only %d transitions!\n", s->id, valency, count);
	    exit(1); 
	}
	
	t = (Transition *)s->transition_list;
	while (t){
	    print_transition(fp, t);
	    t = t->next;
	}
    }
    else if (s->final) {
	in_print(fp, "a final state\n");
	in_print(fp, "request IDs are : ");
	print_requests(fp, s->request_list);
	fprintf(fp, "\n");
    }
    else {
	in_print(fp, "not a final state, but has no transitions.\n");
    }
    
    indent_level--;
    return;
}

/*
 * print_transition(fp, t)
 *
 * Print out the contents of the transition.
 * We call ourselves indirectly by calling print_transition for
 * each transition we have.
 *
 */
void
print_transition(fp, t)
FILE *fp;
Transition *t;
{
    if (!t){
	fprintf(stderr, "print_transition called with NULL!\n");
	exit(1); 
    }
    
    indent_level++;

    in_print(fp, "TRANSITION %d\n", t->id);

    switch (t->label){
	case READ_INT_TRAN:{
	    in_print(fp, "label is READ_INT\n");
	    break;
	}
	case READ_STR_TRAN:{
	    in_print(fp, "label is READ_STR\n");
	    break;
	}
	default:{
	    in_print(fp, "label is '%c'\n", t->label);
	    break;
	}
    }
    in_print(fp, "This transition leads to state %d\n", t->state->id);
    
    if (t->special){
	in_print(fp, "Special actions for this transition.\n");
	print_special(fp, t->special);
    }
    else {
	in_print(fp, "No special information associated with this transition.\n");
    }
    
    in_print(fp, "\n");
    print_state(fp, t->state);
    indent_level--;
    return;
}

/*
 * print_special(fp, s)
 *
 * Print out the special info.
 *
 */
void
print_special(fp, s)
FILE *fp;
Special *s;
{
    while (s){
	switch (s->type){
	    case META_PUSH:{
		in_print(fp, "push %d\n", s->value);
		break;
	    }
	    case META_ADD:{
		in_print(fp, "Add top two stack elements\n");
		break;
	    }
	    case META_SUB:{
		in_print(fp, "Subtract top two stack elements\n");
		break;
	    }
	    case META_MUL:{
		in_print(fp, "Multiply top two stack elements\n");
		break;
	    }
	    case META_DIV:{
		in_print(fp, "Divide top two stack elements\n");
		break;
	    }
	    case META_MOD:{
		in_print(fp, "Modulus top two stack elements\n");
		break;
	    }
	    case META_POP_TO_REG:{
		in_print(fp, "Pop into register %c\n", 'a' + s->value);
		break;
	    }
	    case META_GET:{
		in_print(fp, "Get from register %c\n", 'a' + s->value);
		break;
	    }
	}
	s = s->next;
    }
    return;
}


/*
 * print_requests(fp, r)
 *
 * Print out the requests info.
 *
 */
void
print_requests(fp, r)
FILE *fp;
Request *r;
{
    while (r){
	fprintf(fp, "%d ", r->request_id);
	r = r->next;
    }
    fprintf(fp, "\n");
    return;
}

#undef in_print


/*
 * print_reverse()
 *
 * Print out the contents of the reverse parse list.
 *
 */
void
print_reverse()
{
    Rparse *r = rparse_list;
    
    if (!r){
	printf("Reverse list is empty.\n");
	return;
    }

    while (r){
	printf("ID %d = '%s'\n", r->id, r->s);
	r = r->next;
    } 
    return;
}


/*
 * transition_count(s)
 *
 * Return the total number of transition arcs that leave state s.
 */
int
transition_count(s)
State *s;
{
    register int count = 0;
    register Transition *t;
    
    if (!s){
	return 0;
    }
    
    t = (Transition *)s->transition_list;
    
    while (t){
	count++;
	t = t->next;
    }
    
    return count;
}

#ifdef MALLOC_TEST
char *
Malloc(n)
int n;
{
#undef malloc
    Import char *malloc();
    char *s = malloc(n);

    if (!s){
	parse_error(stderr, "could not malloc %d in Malloc.", n);
    }
    
    fprintf(stderr, "malloc %2d bytes returns %#x\n", n, s);
    fflush(stderr);
    return s;
#define malloc Malloc
}

void
Free(s)
char *s;
{
#undef free
    Import void free();
    fprintf(stderr, "free %#x\n", s);
    fflush(stderr);
    free(s);
#define free Free
}
#endif
