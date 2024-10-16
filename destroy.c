#ifndef lint
static char *rcsid = "$Header: /user1/terry/s/trie/RCS/destroy.c,v 2.0 90/04/25 13:39:32 terry Exp $";
#endif


/*
 * $Log:	destroy.c,v $
 * Revision 2.0  90/04/25  13:39:32  terry
 * Stable for first real test with canvas.
 * 
 * Revision 1.4  90/04/24  16:53:52  jkh
 * Clean-up to make things work with the new stuff. Fun fun fun.
 * 
 * Revision 1.3  90/04/18  17:24:22  terry
 * check in so jordan can take it home.
 * 
 * Revision 1.2  90/04/18  00:01:45  terry
 * Changes for reverse parsing go in.
 * 
 * Revision 1.1  90/04/16  22:47:08  terry
 * Initial revision
 * 
 *
 */

#include "common.h"
#include "trie.h"
#include "destroy.h"

Import void free();

/*
 * destroy_transition(state, label)
 *
 * Remove the transition 'label' from this state, together with
 * everything it points to (i.e. the resulting state and everything
 * that it points to etc.). We know that the state has a transition
 * list and so there's no need to check that state->transition_list
 * is non-null.
 */
Export void destroy_transition(state, label)
State *state;
int label;
{
    Transition *to_free = TRANS_NULL;

#ifdef TRIE_ARG_CHECKING
    if (!state){
	parse_error("Destroy transition called with null pointer.");
    }
#endif /* TRIE_ARG_CHECKING */
    

    if (((Transition *)state->transition_list)->label == label){
	/* It's the first transition for this state. */
	preserve_current_state(((Transition *)state->transition_list)->state);
	destroy_state(((Transition *)state->transition_list)->state);
	destroy_special(((Transition *)state->transition_list)->special);
	to_free = (Transition *)state->transition_list;
	state->transition_list = (Generic)(((Transition *)state->transition_list)->next);
    }
    else{
	Transition *t = (Transition *)state->transition_list;

	while (t->next){
	    if (t->next->label == label){
		preserve_current_state(t->next->state);
		destroy_state(t->next->state);
		destroy_special(t->next->special);
		to_free = t->next;
		t->next = t->next->next;
		break;
	    }
	    t = t->next;
	}
    }
    
    if (!to_free){
	/* ??? We didn't find the transition! Strange. */
	parse_error("Transition '%c' not found in destroy_transition.", label);
    }
    
    free(to_free);
    
#ifdef TRIE_TESTING
    trans_count--;
#endif
    return;
}

/*
 * destroy_state(state)
 *
 * Remove the state from the trie, together with everything it points at.
 * This is recursive.
 *
 * This should not be used to destroy the zero_state unless
 * you know you are doing that and then set zero_state to STATE_NULL
 * afterwards. (That happens in init_parse()). When we call ourselves
 * we cannot be calling ourselves on the zero_state because we are moving
 * deeper into the trie with each call.
 *
 * destroy_transition() calls us too, but only with a state that is pointed
 * to by a transition - hence not with the zero state either.
 *
 */
Export void destroy_state(state)
State *state;
{
    Transition *t;

#ifdef TRIE_ARG_CHECKING
    if (!state){
	parse_error("Destroy state called with null pointer.");
    }
#endif /* TRIE_ARG_CHECKING */

    /* Free the transition list. */
    t = (Transition *)state->transition_list;

    while (t){
	Transition *next = t->next;
	destroy_transition(state, t->label);
	t = next;
    }
    
    destroy_requests(state->request_list);
    free(state);
#ifdef TRIE_TESTING
    state_count--;
#endif
    return;
}

Export void destroy_requests(r)
Request *r;
{
    /* Free the request list. */

    while (r) {
	Request *next = r->next;
	free(r);
#ifdef TRIE_TESTING
	req_count--;
#endif
	r = next;
    }
    return;
}

Export void destroy_special(s)
Special *s;
{
    /* Free the special list. */

    while (s) {
	Special *next = s->next;
	free(s);
#ifdef TRIE_TESTING
	spec_count--;
#endif
	s = next;
    }
    return;
}
