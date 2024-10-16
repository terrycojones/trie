#ifndef lint
static char *rcsid = "$Header: /home/cogsci1/u4/terry/s/trie/RCS/trie.c,v 2.1 90/04/25 15:50:58 terry Exp $";
#endif


/*
 * $Log:	trie.c,v $
 * Revision 2.1  90/04/25  15:50:58  terry
 * Made +/- be recognised as the start of a READ_INT_TRANS in handle().
 * 
 * Revision 2.0  90/04/25  13:40:45  terry
 * Moved destroy_requests into parse_adjust. all is well for testing
 * with canvas.
 * 
 * Revision 1.14  90/04/24  20:13:22  terry
 * Silly.
 * 
 * Revision 1.12  90/04/24  19:58:18  terry
 * Made rparse() use the new list too..... :-(
 * 
 * Revision 1.11  90/04/24  19:49:58  terry
 * Made rparse use a list instead of a table.
 * 
 * Revision 1.10  90/04/24  18:50:28  jkh
 * added #ifdef TRIE_TESTING to tty_ouput_fd stuff in rparse.
 * 
 * Revision 1.9  90/04/24  16:54:27  jkh
 * Clean-up to make things work with the new stuff. Fun fun fun.
 * 
 * Revision 1.8  90/04/24  15:33:05  terry
 * Made things consistent with the new common.h, put in Local, Export
 * etc stuff. Made parse_add and parse_sub into function calls. Cleaned
 * up #includes stuff and took out BSD stuff etc. 
 * Not tested.
 * 
 * Revision 1.7  90/04/18  17:23:14  terry
 * check in so jordan can take it home.
 * 
 * Revision 1.6  90/04/18  00:00:37  terry
 * Rewrote add_parse completely, split into about 5 functions.
 * Added stuff to do reverse parsing, adding and subtracting. Pretty
 * much all written, strange compiler message though. Need to test rparse().
 * 
 * Revision 1.5  90/04/16  22:48:50  terry
 * Added stuff for the reverse parser.
 * 
 * Revision 1.4  90/04/11  15:25:58  terry
 * Fixed up special lists stuff to be added to transitions ok.
 * changed interface to dispatch to use the new ComBlock in
 * common.h. Changed the stack to use Generic data items. Put
 * checks into parse_add() to watch for ambiguities like %d%d etc.
 * 
 * Revision 1.3  90/04/10  21:08:56  terry
 * Lots and lots of changes. Made %d and %s into real live
 * transitions (that may take a while to follow) moved special
 * information to be associated with a transition and not a
 * state. Rewrote handle() to cope with the new stuff. Wrote
 * handle_special(). All sorts of things. Everything is sort
 * of ok but I need to put the special info onto the transition
 * properly in parse_add().
 * 
 * Revision 1.2  90/03/26  22:24:36  terry
 * Lots of things. Now in a state where things seem to work ok. Need to rewrite
 * parse_add() to make it cleaner. Need to allow special state info
 * to be present in the same place with an identical prefix as long as the
 * special info is the same for both prefixes. Yukko. Apart from that, things
 * seem stable.
 * 
 * Revision 1.1  90/03/14  18:18:13  terry
 * Initial revision
 * 
 *
 */

/*
 * trie.c
 * 
 * This is a dynamic parser of escape sequences. It reads a set of escape
 * sequences, each of which has some request ID(s) associated with it.
 * From this set of sequences it builds a recogniser to recognise these
 * sequences and upon recognition of a sequence, to output the associated
 * request ID(s).  The recogniser can be grown (by adding new sequences) or
 * shrunk (by removing sequences) at any time.
 *
 * It is considered an error for any escape sequence to be a prefix of
 * another.
 * 
 * A trie data structure is maintained and traversed to do the
 * recognition.  Special sequences may be embedded in the escape
 * sequences given to build the recogniser. These begin with a % sign and
 * the following are currently recognised,
 * 
 *     %%      a normal % character
 *     %d      a string of decimal digits will arrive here
 *     %s      a string of characters will arrive here
 *     %{nn}   the number given by the digits nn.
 * 
 *     %+      addition
 *     %-      subtraction
 *     %*      multiplication
 *     %/      integer division
 *     %m      modulus
 * 
 *     %pX     pop the stack into register X    ('a' <= X <= 'z')
 *     %gX     push the value in register X     ('a' <= X <= 'z')
 * 
 * A simple stack machine is used to hold and compute values given
 * by the commands above.
 *
 * If any error is encountered a message is printed to stderr and 
 * exit(1) is called.
 *
 * Terry Jones 
 * (terry@distel.pcs.com  or  ..!{pyramid,unido}!pcsbst!distel!terry
 * March 1990
 *
 */

#include "common.h"
#include "trie.h"
#include "destroy.h"
#include "new.h"

#define STACK_SZ   100
#define BUF_SZ     1024

Local State *zero_state = STATE_NULL;
Local State *current_state = STATE_NULL;

Local int sp;                        /* stack pointer. */
Local Generic stack[STACK_SZ];       /* internal stack. */
#define PARSE_PUSH(x)      push((Generic)(x), stack, &sp, STACK_SZ)
#define PARSE_POP()        pop(stack, &sp)
#define PARSE_INT_POP()    (int)(pop(stack, &sp))

Local ComBlock com_block;
Local int save_pos;
Local int handle_state;

/* The reverse parser request list. */
Local Rparse *rparse_list = RPARSE_NULL;

Import void exit();

/*
 * parse_init()
 *
 * Initialise (or re-initialise) the state of the parser.
 */
Export void
parse_init()
{
    if (zero_state){
	destroy_state(zero_state);
    }
    zero_state = new_state();
    zero_state->transition_list = (Generic)TRANS_NULL;
    zero_state->request_list = REQ_NULL;
    zero_state->final = 0;
    
    handle_init();
    rparse_list = RPARSE_NULL;
    
    return;
}

/*
 * parse(fd)
 *
 * Read from the file descriptor and parse what you get. When we
 * read 0, return. The file descriptor has O_NDELAY set and
 * so we don't have to worry about hanging. We are also 
 * guaranteed that there will be something to read since we are
 * called by someone else as the result of a select().
 *
 * Call handle() to take care of each character. Look at what it
 * returns and act accordingly. It it tells us that the parse of a
 * full sequence has been done, then reset our save buffer. If it
 * tells us that the parse of a sequence is in progress, save the char
 * in case handle later cannot figure out what to do (and then we
 * flush all the chars in the save buffer). If we run out of save room,
 * flush the buffer ourselves and reset the handler.
 */
Export void
parse(fd)
int fd;
{
    Import int read();
    Local int handle();
    
    char buf[BUF_SZ];
    static char save_buf[BUF_SZ];
    int nread;
    register int i;
    
#ifdef TRIE_ARG_CHECKING
    if (fd < 0){
	parse_error("parse called with negative file descriptor (%d).", fd);
    }
#endif /* TRIE_ARG_CHECKING */

    while ((nread = read(fd, buf, BUF_SZ))){
	if (nread == -1){
	    parse_error("read error in parse.");
	}
    
	for (i = 0; i < nread; i++){
	    switch (handle(buf[i])){
		case RECOGNISED:{
		    /* The handler has recognised it all. */
		    save_pos = 0;
		    break;
		}
		case IN_PROGRESS:{
		    /* The handler took the char but it wasn't the end of a sequence. */
		    if (save_pos == BUF_SZ){
			/* We can't save any more. Get rid of it all as a self-inserts. */
			com_block.buffer = save_buf;
			com_block.nbytes = BUF_SZ;
			com_block.opcode = OP_INSERT;
			dispatch(&com_block);
			save_pos = 0;
			handle_init();
		    }
		    else{
			save_buf[save_pos++] = buf[i];
		    }
		    break;
		}
		case FLUSH:{
		    /* 
		     * The handler is in trouble, flush the previous characters as a self-insert. 
		     * Try to put this character into the save buffer so we only have to call
		     * dispatch() once. If that doesn't work though, we do it separately *after*
		     * sending off the save buffer.
		     */
		    int did_last = 1;
		    if (save_pos != BUF_SZ){
			/* tack on the last seen character. */
			save_buf[save_pos++] = buf[i];
		    }
		    else {
			did_last = 0;
		    }
		    com_block.buffer = save_buf;
		    com_block.nbytes = save_pos;
		    com_block.opcode = OP_INSERT;
		    dispatch(&com_block);
		    if (!did_last){
			/* Put it into the save buf cause that is static, 'buf' is NOT! */
			save_buf[0] = buf[i];
			com_block.buffer = save_buf;
			com_block.nbytes = 1;
			com_block.opcode = OP_INSERT;
			dispatch(&com_block);
		    }
		    save_pos = 0;
		    /* 
		     * We don't have to call handle_init() since the handler told *us*
		     * it was in trouble and so should have reset itself.
		     */
		    break;
		}
		default:{
		    /* Huh? */
		    parse_error("handler returns unrecognised value.");
		}
	    }
	}
    }
    return;
}

/*
 * handle_init()
 *
 * Initialise the state of the thing that actually does the parsing.
 * Set the stack pointer to -1 here so we clear the stack every time 
 * we reset the handler. This makes sense because if anything is left
 * on the stack we wont want it since something went wrong to get us here.
 */
Local void
handle_init()
{
    current_state = zero_state;
    handle_state = HANDLE_NORMAL;
    sp = -1;
    return;
}

#define HANDLE_BUF_SZ 4096

/*
 * handle(ch)
 *
 * Handle the character 'ch'. If we come to the end of an escape
 * sequence, return RECOGNISED. If everything appears ok, return
 * IN_PROGRESS. If we don't figure out what to do with the character,
 * re-initialise ourselves and return FLUSH. The calling routine (parse)
 * will then send the characters since we last returned RECOGNISED as
 * self-insert characters.
 */
Local int
handle(ch)
int ch;
{
    static int value;
    static int negative;
    static Transition *this_trans = TRANS_NULL;
    static char string_space[HANDLE_BUF_SZ];
    static int string_start_pos;
    static int string_pos;

    switch (handle_state){
	    
	case HANDLE_READ_INT:{
	    if (has_transition(this_trans->state, ch)){
		if (negative){
		    value *= -1;
		}
		PARSE_PUSH(value);
		handle_special(this_trans->special);
		value = 0;
		current_state = this_trans->state;
		/* Our state is now normal since %d%d and %d%s are illegal. */
		handle_state = HANDLE_NORMAL;
		/* Call ourselves to deal with the next char. */
		return handle(ch);
	    }
	    
	    /* Is it a digit? If so, keep going. */
	    if (isdigit(ch)){
		value = value * 10 + ch - '0';
		return IN_PROGRESS;
	    }
	    
	    /* We have an error, not a digit and yet no outgoing arc from the state that will be next. */
	    handle_init();
	    return FLUSH;
	    
	    break;
	}

	case HANDLE_READ_STR:{
	    /* Add the char or a 0 if we have finished with it. */
	    if (string_pos == HANDLE_BUF_SZ){
		/* We can't take any more. Give up. */
		handle_init();
		return FLUSH;
	    }
	    if (has_transition(this_trans->state, ch)){
		string_space[string_pos++] = '\0';
		PARSE_PUSH(string_space + string_start_pos);
		handle_special(this_trans->special);
		current_state = this_trans->state;
		/* Our state is now normal since %s%d and %s%s are illegal. */
		handle_state = HANDLE_NORMAL;
		/* Call ourselves to deal with the next char. */
		return handle(ch);
	    }
	    /* There is no transition from our next state with this char, so add it to our string. */
	    string_space[string_pos++] = ch;
	    return IN_PROGRESS;
	    break;
	}

	case HANDLE_NORMAL:{
	    /*
	     * There's a few cases here...
	     *
	     *    1) The state we are in has a transition for the character 'ch'.
	     *       We just follow the transition after executing any special
	     *       instructions associated with it. If the state we reach is a
	     *       final one then we dispatch the requests and return RECOGNISED.
	     *       If the state we reach is not final we return IN_PROGRESS.
	     *
	     *    2) We have a digit or a '-' or '+' and the state we are in has an outgoing 
	     *       READ_INT_TRAN arc. Then we set up the value of 'value',
	     *       change state and return IN_PROGRESS.
	     *
	     *    3) The state has an outgoing READ_STR_TRAN arc. Then we add set up
	     *       a new string with 'ch' as the first character, change state and 
	     *       return IN_PROGRESS.
	     *
	     *    4) If none of the above works, we have an error.
	     *
	     * All these are handled in the above order below. 
	     * The order is IMPORTANT!
	     *
	     * 'this_trans' is used to point to the transition we are following.
	     * this is used up above by READ_STR and READ_INT too. It is also used
	     * to determine the next state after we follow this transition.
	     */

	    if ((this_trans = has_transition(current_state, ch))){
		handle_special(this_trans->special);
		current_state = this_trans->state;
		if (current_state->final){
		    Request *r = current_state->request_list;
		    while (r){
			com_block.opcode = r->request_id;
			dispatch(&com_block);
			r = r->next;
		    }
		    handle_init();
		    /* 
		     * Set ourselves up to overwrite the string, as all the values
		     * in it will be read after we return RECOGNISED.
		     */
		    string_pos = 0;
		    return RECOGNISED;
		}
		return IN_PROGRESS;
	    }
	    else if ((isdigit(ch) || ch == '-' || ch == '+') && 
		     (this_trans = has_transition(current_state, READ_INT_TRAN))){
		value = 0;
		negative = 0;
		if (ch == '-'){
		    negative = 1;
		}
		else if (ch != '+'){
		    value = ch - '0';
		    negative = 0;
		}
		handle_state = HANDLE_READ_INT;
		return IN_PROGRESS;
	    }
	    else if ((this_trans = has_transition(current_state, READ_STR_TRAN))){
		if (string_pos == HANDLE_BUF_SZ){
		    /* We can't handle any more. Give up. */
		    handle_init();
		    return FLUSH;
		}
		string_start_pos = string_pos;
		string_space[string_pos++] = ch;
		handle_state = HANDLE_READ_STR;
		return IN_PROGRESS;
	    }
	    else {
		handle_init();
		return FLUSH;
	    }
	    
	    break;
	}
	
	default:{
	    parse_error("handle() in unknown state.");
	}
    }
    
    /* NOTREACHED */
    parse_error("handle() fell off the end of the world! aaaaahhhh.");
    return FLUSH;   /* Keep gcc quiet.... :-(  */
}

#undef HANDLE_BUF_SZ


/*
 * handle_special(s)
 *
 * Execute the special actions found in s.
 *
 */
Local void
handle_special(s)
Special *s;
{
    while (s){
	switch(s->type){
			
	    case META_PUSH:{
		PARSE_PUSH(s->value);
		break;
	    }
	    case META_ADD:{
		PARSE_PUSH(PARSE_INT_POP() + PARSE_INT_POP());
		break;
	    }
	    case META_SUB:{
		int val = PARSE_INT_POP();
		PARSE_PUSH(PARSE_INT_POP() - val);
		break;
	    }
	    case META_MUL:{
		PARSE_PUSH(PARSE_INT_POP() * PARSE_INT_POP());
		break;
	    }
	    case META_DIV:{
		int val = PARSE_INT_POP();
		PARSE_PUSH(PARSE_INT_POP() / val);
		break;
	    }
	    case META_MOD:{
		int val = PARSE_INT_POP();
		PARSE_PUSH(PARSE_INT_POP() % val);
		break;
	    }
	    case META_POP_TO_REG:{
		com_block.regs[s->value].data = PARSE_POP();
		com_block.regs[s->value].type = CB_INT_TYPE;
		break;
	    }
	    case META_GET:{
		PARSE_PUSH(com_block.regs[s->value].data);
		break;
	    }
	}
	s = s->next;
    }
    return;
}

#define RPARSE_PUSH(x)   push((Generic)(x), rp_stack, &rp_sp, STACK_SZ)
#define RPARSE_POP()     pop(rp_stack, &rp_sp)
#define RPARSE_STR_POP() (char *)(pop(rp_stack, &rp_sp))
#define RPARSE_INT_POP() (int)(pop(rp_stack, &rp_sp))
#define RPARSE_LNG_POP() (long)(pop(rp_stack, &rp_sp))
#define RPARSE_CHR_POP() (char)(pop(rp_stack, &rp_sp))
#define RPARSE_FLT_POP() (float)((long)(pop(rp_stack, &rp_sp)))

/*
 * rparse(blk)
 *
 * Reverse "parse" the string in s and send the result to the FILE *
 * we get from tty_output_fp().
 */
Export void
rparse(blk)
ComBlock *blk;
{
    Rparse *r;
    register int id;
    register char *s;
    Generic rp_stack[STACK_SZ];
    int rp_sp = 0;
#ifdef TRIE_TESTING
    FILE *fp = stdout;
#else
    FILE *fp = tty_output_fp();
#endif


    if (!blk){
	parse_error("rparse called with NULL argument.");
    }
    
    id = blk->opcode;
    
    if (id < 0){
	parse_error("rparse called with illegal opcode (%d).", id);
    }
    
    if (id == OP_INSERT /* Zero */){
	blk->buffer[blk->nbytes] = '\0';
	fprintf(fp, "%s", blk->buffer);
	return;
    }
    
    if (!(r = rparse_find(id))){
	parse_error("rparse called with opcode %d which does not exist.", id);
    }

    s = r->s;
    
    if (!s){
	parse_error("rparse called with opcode (%d) that has no corresponding output string.", id);
    }
    
    while (*s){
	switch (*s){
	    case '%':{
		switch (*(s + 1)){
		    case 'g':{
			int reg = *(s + 2) - 'a';
			
			if (reg < 0 || reg >= CB_NREGS){
			    parse_error("rparse found illegal register (%c) in string (%s) for opcode %d", 
					*(s + 2), r->s, id);
			}
			RPARSE_PUSH(blk->regs[reg].data);
			s += 3;
			break;
		    }
		    case 'p':{
			int reg = *(s + 2) - 'a';
			
			if (reg < 0 || reg >= CB_NREGS){
			    parse_error("rparse found illegal register (%c) in string (%s) for opcode %d", 
					*(s + 2), r->s, id);
			}

			blk->regs[reg].data = RPARSE_POP();
			blk->regs[reg].type = CB_INT_TYPE;
			s += 3;
			break;
		    }
		    case '%':{
			fputc('%', fp);
			s += 2;
			break;
		    }
		    case '{':{
			int val;
			s += 2;
			s = read_int(s, &val);
			
			if (*s != '}'){
			    parse_error("missing '}' found by rparse after '{%d' for opcode %d", val, id);
			}
			RPARSE_PUSH(val);
			s++;
			break;
		    }
		    case '+':{
			RPARSE_PUSH(RPARSE_INT_POP() + RPARSE_INT_POP());
			s += 2;
			break;
		    }
		    case '-':{
			int val = RPARSE_INT_POP();
			RPARSE_PUSH(RPARSE_INT_POP() - val);
			s += 2;
			break;
		    }
		    case '*':{
			RPARSE_PUSH(RPARSE_INT_POP() * RPARSE_INT_POP());
			s += 2;
			break;
		    }
		    case '/':{
			int val = RPARSE_INT_POP();
			RPARSE_PUSH(RPARSE_INT_POP() / val);
			s += 2;
			break;
		    }
		    case 'm':{
			int val = RPARSE_INT_POP();
			RPARSE_PUSH(RPARSE_INT_POP() % val);
			s += 2;
			break;
		    }
		    case 'l':
		    case 'h':{
			/* Long integer. */
			switch (*(s + 2)){
			    case 'o':
			    case 'u':
			    case 'X':
			    case 'x':
			    case 'i':
			    case 'd':{
				fprintf(fp, "%d", RPARSE_LNG_POP());
				s += 3;
			    }
			    case '\0':{
				parse_error("unterminated %%%c action in reverse parse sequence (%s)", 
					    *(s + 1), r->s);
			    }
			    default:{
				parse_error("unrecognized %%%c%c action in reverse parse sequence (%s)", 
					    *(s + 1), *(s + 2), r->s);
			    }
			}
			s += 3;
		    }
		    case 'o':
		    case 'u':
		    case 'X':
		    case 'x':
		    case 'i':
		    case 'd':{
			fprintf(fp, "%d", RPARSE_INT_POP());
			s += 2;
			break;
		    }
		    case 'e':
		    case 'E':
		    case 'f':{
			fprintf(fp, "%f", RPARSE_FLT_POP());
			s += 2;
			break;
		    }
		    case 'c':{
			fprintf(fp, "%c", RPARSE_CHR_POP());
			s += 2;
			break;
		    }
		    case 's':{
			fprintf(fp, "%s", RPARSE_STR_POP());
			s += 2;
			break;
		    }
		    case '\0':{
			parse_error("unterminated %% action in reverse parse sequence (%s)", r->s);
		    }
		    default:{
			parse_error("unrecognized %%%c action in reverse parse sequence (%s)", *(s + 1), r->s);
		    }
		}
		break;
	    }

	    default:{
		fputc(*s, fp);
		s++;
		break;
	    }
	}
    }
    return;
}
#undef RPARSE_PUSH
#undef RPARSE_POP
#undef RPARSE_STR_POP
#undef RPARSE_INT_POP
#undef RPARSE_LNG_POP
#undef RPARSE_CHR_POP
#undef RPARSE_FLT_POP


/*
 * add_transition(state, label, special)
 *
 * Add an outgoing arc labelled 'label' to the state 'state' with
 * the special action list 'special'.
 * If the arc already exists then we just return whatever state
 * it points to. 
 * If not, we create a new transition and a new
 * state for it to point to, and fill in the blanks. Then return
 * the new state. The new transition is added at the front of
 * the transition list for this state (it doesn't matter where
 * we put it because later we do a linear serch to find it anyway.)
 *
 * We have an error condition if the state is a final state, since
 * then the string we are adding has a prefix which is another string.
 */
Local State *
add_transition(state, label, special)
State *state;
int label;
Special *special;
{
    Transition *t;
    
    if (!state){
	/* The first ever transition, create the zero_state. */
	state = zero_state = current_state = new_state();
	state->transition_list = (Generic)TRANS_NULL;
	state->request_list = REQ_NULL;
	state->final = 0;
    }
    else if (state->final){
	/* Error! (one is a prefix of another). */
	parse_error("add_transition found an escape sequence which is the prefix of another.");
    }

    t = (Transition *)state->transition_list;

    /* Walk the transition list and see if the transition exists. */
    while (t){
	if (t->label == label){
	    /* Unless both have no special info, we probably have a problem. */
	    /* Could check to see that special lists are identical. later. */
	    if (!(!special && !t->special)){
		parse_error("special action inconsistency.");
	    }
	    
	    /* 
	     * Add to the valency since another string is being added that goes
	     * through this transition.
	     */
	    t->valency++;
	    return t->state;
	}
	t = t->next;
    }
    
    /* Create and add the new transition. */
    t = new_transition();
    t->next = (Transition *)state->transition_list;
    t->label = label;
    t->special = special;
    t->valency = 1;
    t->state = new_state();
    t->state->final = 0;
    t->state->transition_list = (Generic)TRANS_NULL;
    t->state->request_list = REQ_NULL;
    state->transition_list = (Generic)t;
    
    return t->state;
}

/*
 * add_special(list, type, value)
 *
 * Add a special action transition to the list 'list'. When we encounter
 * this state while actually parsing, the special action will be
 * taken in preference to any ordinary outgoing transition. The
 * special actions are given in the specification of the escape
 * sequences - things like %* and %pX etc etc.
 *
 * We must add the new special action at the END of the list of special
 * actions. This is because we want to execute them in
 * the correct order when we are parsing.
 *
 * Return the head of the list.
 *
 */
Local Special *
add_special(list, type, value)
Special *list;
int type;
int value;
{
    Special *head;

    if (list){
	/* Walk to the end of the list and there create a new special entry. */
	head = list;
	while (list->next){
	    list = list->next;
	}
	list->next = new_special();
	list = list->next;
    }
    else{
	/* Create the list. */
	head = list = new_special();
    }
    
    /* Fill in the blanks. */
    list->type = type;
    list->value = value;
    list->next = SPEC_NULL;
    return head;
}

/*
 * has_transition(state, label)
 *
 * Return a pointer to the transition if the state 'state' has a label 'label'
 * and (Transition *)0 otherwise.
 */
Local Transition *
has_transition(state, label)
State *state;
int label;
{
    Transition *t;
    
    if (!state){
	return TRANS_NULL;
    }
    
    t = (Transition *)state->transition_list;

    while (t){
	if (t->label == label){
	    return t;
	}
	t = t->next;
    }
    
    return TRANS_NULL;
}


/*
 * follow_transition(state, label)
 *
 * Follow the transition marked 'label' leading out from this state.
 * Return the new state or else 0 if there is no such outgoing 
 * transition.
 */
Local State *
follow_transition(state, label)
State *state;
int label;
{
    Transition *t;
    
    if (!state){
	return STATE_NULL;
    }
    
    t = (Transition *)state->transition_list;

    while (t){
	if (t->label == label){
	    return t->state;
	}
	t = t->next;
    }
    
    return STATE_NULL;
}

/*
 * delete_string(s)
 *
 * Removed the string 's' from the trie. We walk the trie until
 * we find the point at which a transition has valency 1 and then we
 * snip off that arm, starting at the previous state.
 * Reduce the valency of each transition we follow since we are
 * removing an instance of that character.
 */
Local void
delete_string(s)
String s;
{
    State *state = zero_state;
    int chop_char = '\0';
    State *chop_state = STATE_NULL;
    Transition *t;
    
#ifdef TRIE_ARG_CHECKING
    if (!s){
	parse_error("Delete string called with null pointer.");
    }
#endif /* TRIE_ARG_CHECKING */
    
    if (!state){
	parse_error("Delete string called with no zero state.");
    }
    
    while (*s){
	int len = 0;
	int ch = next_char(s, &len);
	
	s += len;
	
	if (!(t = has_transition(state, ch))){
	    parse_error("Delete string called with a string not in the trie.");
	}
	
	t->valency--;

	/* Remember where we are if this is the branch point for this string. */
	if (!t->valency && chop_state == STATE_NULL){
	    chop_state = state;
	    chop_char = ch;
	}
	state = t->state;
    }
    
    if (!state->final){
	parse_error("Delete string called with a string in the trie that does not end in a final state.");
    }

    if (!chop_state){
	parse_error("Delete string could not find chop state!.");
    }
    
    /* 
     * Now that we have verified the string is there and that it terminates in
     * a final state, chop off the branch that leads to it.
     */
    destroy_transition(chop_state, chop_char);
    return;
}

/*
 * next_char(s)
 *
 * Return the next character from s that corresponds to a transition in the
 * trie. This is a normal character unles we have one of %d, %s or %% which
 * correspond to the 'chars' READ_INT_TRAN, READ_STR_TRAN and '%'. Other %
 * metachars are skipped because they do not correspond to any input.
 *
 * We also return the number of chars the next 'char' takes up. We are
 * recursive, and add to 'len' each time. For example, called on the string
 * "%d%paX" we would call ourselves again (twice) and eventually return
 * 'X' with *len = 6. make sense? So our caller should set *len to 0 in the
 * beginning or all bets are off.
 */
Local int
next_char(s, len)
String s;
int *len;
{
#ifdef TRIE_ARG_CHECKING
    if (!s || !len){
	parse_error("Next_char() called with null pointer.");
    }
#endif /* TRIE_ARG_CHECKING */

    switch (*s){
	case '%':{
	    switch (*(s + 1)){
		case 'd':{
		    (*len) += 2;
		    return READ_INT_TRAN;
		}
		case 's':{
		    (*len) += 2;
		    return READ_STR_TRAN;
		}
		case '%':{
		    (*len) += 2;
		    return '%';
		}
		case 'p':
		case 'g':{
		    /* Need the register name and a following character at least. */
		    if (*(s + 2) == '\0' || *(s + 3) == '\0'){
			parse_error("Next_char() called with strange string.");
		    }
		    (*len) += 3;
		    return next_char(s + 3, len);
		}
		
		case '{':{
		    s += 2;
		    if (*s == '-' || *s == '+'){
			(*len)++;
			s++;
		    }

		    while (isdigit(*s)){
			(*len)++;
			s++;
		    }
		    if (*s != '}'){
			parse_error("Next_char() called with string that has \"%%{\", but no closing }.");
		    }
		    (*len) += 3;   /* One each for %, {, and }. */
		    if (*(s + 1) == '\0'){
			parse_error("Next_char() called with strange string.");
		    }
		    return next_char(s + 1, len);
		}
		
		case 'm':
		case '+':
		case '-':
		case '/':
		case '*':{
		    /* Need at least one more character. */
		    if (*(s + 2) == '\0'){
			parse_error("Next_char() called with strange string.");
		    }
		    (*len) += 2;
		    return next_char(s + 2, len);
		}
		
		default:{
		    /* Strange... */
		    parse_error("Next_char() called with string containing unknown %%%c action.", *(s + 1));
		    return 0; /* Keep gnu cc happy :-( */
		}
	    }
	}

	default:{
	    (*len)++;
	    return (int)(*s);
	}
    }
    
    parse_error("Next_char() fell off the end of the earth.....thud.");
    return 0; /* Keep the compiler quiet. */
}


Export void
parse_sub(s)
String s;
{
#ifdef TRIE_ARG_CHECKING
    if (!s){
	parse_error("parse_sub() called with NULL.");
    }
#endif

    parse_adjust(s, PARSE_SUB);
    return;
}

Export void
parse_add(s)
String s;
{
#ifdef TRIE_ARG_CHECKING
    if (!s){
	parse_error("parse_add() called with NULL.");
    }
#endif

    parse_adjust(s, PARSE_ADD);
    return;
}

/*
 * parse_adjust(s, what)
 *
 * Add or subtract the escape sequences in 's' to or from the trie. The format of s is:
 * 
 *  s = "[white]ID[,ID[,ID]...]<esc_seq>[white]ID[,ID[,ID]...]<esc_seq>..."
 *
 * where ID is a string of digits giving the request id that should
 * be returned when the escape sequence esc_seq is recognized. The
 * angle brackets are used to delimit the sequence. If esc_seq
 * contains a '>' then it will be escaped by a '\' character.
 * [white] indicates optional white space consisting of ' ', '\t' and '\n'.
 *
 */
Local void
parse_adjust(s, what)
String s;
int what;
{
    char *orig;
    char *str;
    char *new;
    Request *request_list;
    Import char *malloc();
    Import void free();

    if (!(str = (char *)malloc(strlen(s) + 1))){
	parse_error("could not malloc in parse_add()");
    }
    
    orig = str;
    strcpy(str, s);

    while ((request_list = get_request(str, &str, &new))){
	
	/* 
	 * str now point to the start of the sequence, (after the '<') and is NUL terminated.
	 * new now points to the first char after the '>'
	 * We check to make sure all the request IDs were of the same sign,
	 * and then add or subtract the list according to what we find in what.
	 */
	
	/* If all are positive, act on the parser. */
	if (requests_positive(request_list)){
	    if (what == PARSE_ADD){
		add_string(str, request_list);
	    }
	    else if (what == PARSE_SUB){
		delete_string(str, request_list);
	    }
	    else {
		parse_error("parse_add() called with unknown action (%d).", what);
	    }
	    /* Don't destroy the requests list - some state is using it! */
	}
	/* If all are negative, act on the reverse parser. */
	else if (requests_negative(request_list)){
	    if (what == PARSE_ADD){
		reverse_add(str, request_list);
	    }
	    else if (what == PARSE_SUB){
		reverse_sub(request_list);
	    }
	    else {
		parse_error("parse_add() called with unknown action (%d).", what);
	    }
	    /* Destroy the request list, no-one is using it now. */
	    destroy_requests(request_list);
	}
	else {
	    parse_error("positive and negative IDs found for the same escape sequence '%s' in parse_add()", str);
	}

	str = new;
    }
    
    free(orig);
    return;
}


/*
 * get_request(s, start, end)
 *
 * read the next ID,ID,ID<sequence> string.
 * return a list of the request IDs, set *start to point to
 * the char after the <, set the > char to be a '\0' and *end to
 * point after it.
 *
 */
Local Request *
get_request(s, start, end)
String s;
char **start;
char **end;
{
    register char *left_angle_bracket;
    register char *right_angle_bracket;

#ifdef TRIE_ARG_CHECKING
    if (!s){
	parse_error("get_request called with null string.");
    }
#endif

    if (!(left_angle_bracket = strchr(s, '<'))){
	return REQ_NULL;
    }
    
    *left_angle_bracket = '\0';
    *start = right_angle_bracket = left_angle_bracket + 1;
    
    do {
	if (!(right_angle_bracket = strchr(right_angle_bracket, '>'))){
	    return REQ_NULL;
	}
    } while (*(right_angle_bracket - 1) == '\\');
    
    *right_angle_bracket = '\0';
    *end = right_angle_bracket + 1;
    
    return get_request_list(s);
}


/*
 * get_request_list(s)
 *
 * Read the request list ID,ID,ID... froms and form it into
 * a list of Request structs. Use strtok to make things easy.
 * 
 */
Local Request *
get_request_list(s)
String s;
{
    Import char *strtok();
    Request *r = REQ_NULL;
    char *id;

#ifdef TRIE_ARG_CHECKING
    if (!s){
	parse_error("get_request_list called with null string.");
    }
#endif

#define SEP_STR " \t\n,"
    id = strtok(s, SEP_STR);

    while (id){
	int n;
	id = read_int(id, &n);
	if (*id != '\0'){
	    parse_error("get_request_list found unexpected char '%c' in request ID list", *id);
	}
	r = add_request(n, r);
	id = strtok(NULL, SEP_STR);
    }
    
    return r;
}
#undef SEP_STR

/*
 *
 * add_string(s, requests)
 *
 * add the NUL terminated string s to the trie. When we get to the end make
 * the state we have reached a final state and tack on the request list in
 * requests.
 *
 * Read s and watch for \> as well as the special action designators
 * such as %d and %p etc.
 *
 * Special action designators fall into two classes:
 *
 *        1) %d, %s and %%
 *        2) all other % actions.
 *
 * The first three (%d, %s and %%) will correspond to some input when we are parsing.
 * The others all call for actions to be taken on the stack and registers.
 * The first three are considered transitions in their own right, the others
 * are attached to a transition and are executed before that transition is 
 * followed. For this reason we need to ensure that the % sequences are 
 * always followed by a normal (non-%) character. This is a little obscure 
 * i'm afraid - i don't have too much time to document this stuff. Let alone
 * get the bloody thing working...
 *
 */

#define remember(x) \
    if (ipos == BUF_SZ) parse_error("sequence too long in add_string"); else inserted[ipos++] = (x)

Local void
add_string(s, requests)
String s;
Request *requests;
{
    State *state = zero_state;
    Special *special_list = SPEC_NULL;
    char inserted[BUF_SZ + 1];
    int ipos = 0;
    
    int need_normal = 0; /* True if we need at least one more 'normal' char. after seeing a % action. */
    int need_sep = 0;    /* True if we need a separator (a normal char) e.g. catch illegal %d%d */
    int seen_bs = 0;     /* True if we have just seen a backslash. */
    int seen_pc = 0;     /* True if we have just seen a percent sign. */

#ifdef TRIE_ARG_CHECKING
    if (!s || !requests){
	parse_error("parse_add called with null argument.");
    }
#endif

    while (*s){
	
	if (seen_bs){
	    /* '>' is the only meta character. */
	    if (*s != '>'){
		state = add_transition(state, '\\', special_list);
		remember('\\');
		special_list = SPEC_NULL;
	    }
	    state = add_transition(state, *s, special_list);
	    remember(*s);
	    special_list = SPEC_NULL;
	    seen_bs = 0;
	    need_normal = 0;
	    s++;
	    continue;
	}

	/* Did we just see a % sign? */
	if (seen_pc){
	    switch (*s){
		case '%':{
		    /* A normal percent sign. */
		    state = add_transition(state, '%', special_list);
		    remember('%');
		    special_list = SPEC_NULL;
		    need_normal = 0;
		    need_sep = 0;
		    break;
		}
		case 'd':{
		    /* An integer. */
		    if (need_sep){
			parse_error("%%d found immediately after earlier %%s or %%d in add_string");
		    }
		    else {
			need_sep = 1;
		    }
		    state = add_transition(state, READ_INT_TRAN, special_list);
		    remember(READ_INT_TRAN);
		    special_list = SPEC_NULL;
		    break;
		}
		case 's':{
		    /* An integer. */
		    if (need_sep){
			parse_error("%%s found immediately after earlier %%s or %%d in add_string");
		    }
		    else {
			need_sep = 1;
		    }
		    state = add_transition(state, READ_STR_TRAN, special_list);
		    remember(READ_STR_TRAN);
		    special_list = SPEC_NULL;
		    break;
		}
		case '{':{
		    /* The start of a literal number. e.g. %{15} */
		    int literal = 0;
		    
		    s = read_int(s + 1, &literal);
		    
		    if (*s != '}'){
			parse_error("Missing '}' following %{%d push for request in add_string", literal);
		    }
		    
		    special_list = add_special(special_list, META_PUSH, literal);
		    break;
		}
		case '+':{
		    /* Add */
		    special_list = add_special(special_list, META_ADD, 0);
		    break;
		}
		case '-':{
		    /* Subtract */
		    special_list = add_special(special_list, META_SUB, 0);
		    break;
		}
		case '*':{
		    /* Multiply */
		    special_list = add_special(special_list, META_MUL, 0);
		    break;
		}
		case '/':{
		    /* Divide */
		    special_list = add_special(special_list, META_DIV, 0);
		    break;
		}
		case 'm':{
		    /* Modulus */
		    special_list = add_special(special_list, META_MOD, 0);
		    break;
		}
		case 'p':{
		    /* Pop into X (X given by next char). */
		    register int reg;
		    
		    s++;
		    reg = *s - 'a';
		    if (reg < 0 || reg >= CB_NREGS){
			parse_error("Invalid register '%c' following pop in request in add_string", *s);
		    }
		    special_list = add_special(special_list, META_POP_TO_REG, reg);
		    break;
		}
		case 'g':{
		    /* Get from X and push. (X given by next char). */
		    register int reg;
		    
		    s++;
		    reg = *s - 'a';
		    if (reg < 0 || reg >= CB_NREGS){
			parse_error("Invalid register '%c' following pop in request in add_string", *s);
		    }
		    special_list = add_special(special_list, META_GET, reg);
		    break;
		}
		default:{
		    /* Unrecognised - complain. */
		    parse_error("Warning: non-meta-character '%c' following '%%' in add_string.", *s);
		}
	    }
	    seen_pc = 0;
	    s++;
	    continue;
	}
	
	/* A normal character, or the start of a special sequence. */
	switch (*s){
	    case '\\':{
		seen_bs = 1;
		break;
	    }
	    case '%':{
		seen_pc = 1;
		need_normal = 1;
		break;
	    }
	    default:{
		/* A plain old character, add it. */
		state = add_transition(state, *s, special_list);
		remember(*s);
		special_list = SPEC_NULL;
		need_normal = 0;
		need_sep = 0;
		break;
	    }
	}
	s++;
    }
    
    /*
     * Do a couple of sanity checks and then add the requests info to the state.
     * Add the request ID list to the state.
     * Make sure the state was not already a final state or
     * the prefix of some other state.
     */
    
    /* We still need a normal char! complain. */
    if (need_normal){
	parse_error("add_string() found an escape sequence ending with a %% action.");
    }
    
    /* Prefix error. */
    if (state->transition_list){
	parse_error("add_string found an escape sequence which is the prefix of another.");
    }
    
    /* If this is already a final state and the request lists are different, complain. */
    if (state->final){
	if (!same_requests(state->request_list, requests)){
	    parse_error("add_string() found identical escape sequences with differing request ID lists.");
	}
	else {
	    /* Throw one set away. Below we assign request_list to state->request_list, so all's well. */
	    destroy_requests(state->request_list);
	    
	    /*
	     * We have to adjust all the valency counts for this string now. We 
	     * thought we were adding a new string but it turned out it was an
	     * old one... so all the valencies along the way are now one too high.
	     * We saved the actual inserted transitions in 'inserted', so there's no
	     * need to re-parse the string or anything.
	     */
	    inserted[ipos] = '\0';
	    adjust_valencies(inserted);
	}
    }
    
    state->final = 1;
    state->request_list = requests;
    state->transition_list = (Generic)TRANS_NULL;
    
/* #ifdef TRIE_TESTING
    {
	Request *r = requests;
	printf("Added new state with request IDs = ");
	while (r){
	    printf("%d ", r->request_id);
	    r = r->next;
	}
	printf("\n");
    }
#endif */

    return;
}

/*
 * add_request(r, request_list)
 *
 * Add a new request node to the end of the request list.
 * Return the new list. Watch out for a 0 argument - in which we
 * create the list.
 *
 * Also catch the illegal request OP_INSERT
 */
Local Request *
add_request(r, request_list)
int r;
Request *request_list;
{
    Request *tmp;

    if (r == OP_INSERT){
	parse_error("Illegal request ID = %d (reserved for OP_INSERT).", r);
    }

    if (!request_list){
	request_list = new_request();
	request_list->request_id = r;
	request_list->next = REQ_NULL;
	return request_list;
    }
    
    tmp = request_list;

    while (tmp->next){
	tmp = tmp->next;
    }
    
    tmp->next = new_request();
    tmp = tmp->next;
    tmp->request_id = r;
    tmp->next = REQ_NULL;
    return request_list;
}

/*
 * push(x)
 *
 * Push x onto the internal stack.
 */
Local void
push(x, stack, sp, stack_size)
Generic x;
Generic *stack;
int *sp;
int stack_size;
{
    (*sp)++;
    
    if (*sp >= stack_size){
	parse_error("cannot push, stack size limit exceeded!");
    }
    
    stack[*sp] = x;
    return;
}


/*
 * pop()
 *
 * Return the value on the top of the internal stack.
 */
Local Generic
pop(stack, sp)
Generic *stack;
int *sp;
{
    if (*sp < 0){
	parse_error("cannot pop, empty stack!");
    }
    
    return stack[(*sp)--];
}


Local void reverse_add(sequence, requests)
String sequence;
Request *requests;
{
     Import char *malloc();
     Import void free();
     Import Rparse *new_rparse();

    /*
     * Add the sequence to the reverse parser for the IDs in requests.
     * The IDs are all negative.
     */
     
    Request *req;
    register int slen;

#ifdef TRIE_ARG_CHECKING
    if (!requests || !sequence){
	parse_error("reverse_add() called with bad pointer.");
    }
#endif
    
    req = requests;
    slen = strlen(sequence);

    while (req){
	register int id = req->request_id * -1;
	Rparse *r;

	if (id <= 0){
	    parse_error("reverse_add() called with illegal request id = %d.", id);
	}

	if ((r = rparse_find(id))){
	    if (slen > r->length){
		/* Have to malloc... */
		free(r->s);
		if (!(r->s = (String)malloc(slen + 1))){
		    parse_error("could not malloc in reverse_add()");
		}
		r->length = slen;
	    }
	}
	else {
	    /* Make a new Rparse and adjust the rparse_list ptr. */
	    r = new_rparse();
	    if (!(r->s = (String)malloc(slen + 1))){
		parse_error("could not malloc in reverse_add()");
	    }
	    r->id = id;
	    r->length = slen;
	    r->next = rparse_list;
	    rparse_list = r;
	}
	strcpy(r->s, sequence);
	req = req->next;
    }
    
    return;
}


/*
 * rparse_find(x)
 *
 * Walk the rparse_list and try to find a reverse parse ID the same as x.
 *
 */
Local Rparse *
rparse_find(x)
int x;
{
    Rparse *r;

    if (!rparse_list){
	return RPARSE_NULL;
    }
    
    r = rparse_list;
    
    while (r){
	if (r->id == x){
	    return r;
	}
	r = r->next;
    }
    
    return RPARSE_NULL;
}

/*
 * same_requests(a, b)
 *
 * Return 1 or 0 according to whether the request lists a and b are identical.
 */
Local int
same_requests(a, b)
Request *a;
Request *b;
{
#ifdef TRIE_ARG_CHECKING
    if (!a || !b){
	parse_error("same_requests called with bad pointer.");
    }
#endif

    while (a && b){
	if (a->request_id != b->request_id){
	    return 0;
	}
	a = a->next;
	b = b->next;
    }
    
    return (!a && !b) ? 1 : 0;
}

/*
 * requests_positive(r)
 *
 * Return 1 or 0 according to whether all the requests in the list are
 * positive.
 */
Local int
requests_positive(r)
Request *r;
{
#ifdef TRIE_ARG_CHECKING
    if (!r){
	parse_error("requests_positive called with bad pointer.");
    }
#endif
    while (r){
	if (r->request_id <= 0){
	    return 0;
	}
	r = r->next;
    }
    return 1;
}

/*
 * requests_negative(r)
 *
 * Return 1 or 0 according to whether all the requests in the list are
 * negative.
 */
Local int
requests_negative(r)
Request *r;
{
#ifdef TRIE_ARG_CHECKING
    if (!r){
	parse_error("requests_negative called with bad pointer.");
    }
#endif
    while (r){
	if (r->request_id >= 0){
	    return 0;
	}
	r = r->next;
    }
    return 1;
}

/*
 * reverse_sub(requests)
 *
 * Remove request list requests from the reverse parse list.
 */
Local void
reverse_sub(requests)
Request *requests;
{
     Import void free();

#ifdef TRIE_ARG_CHECKING
    if (!requests){
	parse_error("reverse_sub called with bad pointer.");
    }
#endif

    while (requests){
	Rparse *r;
	Rparse *to_free = RPARSE_NULL;
	int n = requests->request_id * -1;
	
	if (n <= 0){
	    parse_error("request_sub called with illegal argument (%d).", n);
	}
	
	if (!(r = rparse_list)){
	    return;
	}

	/* Check the first one, else walk, looking one ahead so as to snip pointers properly. */
	if (r->id == n){
	    to_free = r;
	    free(r->s);
	    rparse_list = r->next;
	}
	else {
	    while (r->next){
		if (r->next->id == n){
		    to_free = r->next;
		    free(r->next->s);
		    r->next = r->next->next;
		    break;
		}
		r = r->next;
	    }
	}
	
	/* Free it if we found it. */
	if (to_free != RPARSE_NULL){
	    free(to_free);
	}
	requests = requests->next;
    }
    
    return;
}

/*
 * preserve_current_state(state)
 *
 * Check to see if the passed state is the current state and if it is,
 * set the current state back to the zero-state. There's not much else 
 * we could sensibly do. This is called by destroy_transition (who
 * calls destroy_state) who is called by delete_string.
 */
Export void
preserve_current_state(state)
State *state;
{
    if (current_state == state){
	if (current_state == zero_state){
	    parse_error("Tried to kill the zero_state!");
	}
	current_state = zero_state;
    }
    return;
}

/*
 * read_int(s, x)
 *
 * Read a (possibly signed) integer starting at s, put the value
 * into *x and return the address of the next char.
 */
Local char *
read_int(s, x)
register String s;
int *x;
{
    register int val = 0;
    int negative = 0;

    if (!s || !x){
	parse_error("read_int called with NULL argument.");
    }

    if (*s == '-'){
	negative = 1;
	s++;
    }
    else if (*s == '+'){
	s++;
    }
    
    while (isdigit(*s)){
	val = val * 10 + *s - '0';
	s++;
    }
    
    if (negative){
	val *= -1;
    }
    
    *x = val;
    return s;
}

/*
 * adjust_valencies(s)
 *
 * Follow the string s through the trie and decrement the valency of
 * each transition that is followed.
 */
void
Local adjust_valencies(s)
String s;
{
    State *state = zero_state;

#ifdef TRIE_ARG_CHECKING
    if (!s){
	parse_error("adjust_valencies called with NULL argument.");
    }
#endif

    while (*s){
	Transition *t = has_transition(state, *s);
	
	if (!t){
	    parse_error("adjust valencies could not find outgoing arc.");
	}
	
	t->valency--;
	state = t->state;
	s++;
    }
    
    /* Sanity check. */
    if (!state->final){
	parse_error("adjust_valencies wound up in non-final state!");
    }
    
    return;
}

#ifdef TRIE_TESTING
#include "trie_aux.c"
#endif
