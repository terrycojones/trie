#ifndef lint
static char *rcsid = "$Header: /user1/terry/s/trie/RCS/new.c,v 2.0 90/04/25 13:40:08 terry Exp $";
#endif


/*
 * $Log:	new.c,v $
 * Revision 2.0  90/04/25  13:40:08  terry
 * Changed new_rparse to actually return an Rparse *...
 * 
 * Revision 1.4  90/04/24  20:15:05  terry
 * silly.
 * 
 * Revision 1.3  90/04/24  16:54:22  jkh
 * Clean-up to make things work with the new stuff. Fun fun fun.
 * 
 * Revision 1.2  90/04/18  17:24:24  terry
 * check in so jordan can take it home.
 * 
 * Revision 1.1  90/04/16  22:47:01  terry
 * Initial revision
 * 
 *
 */

#ifdef TRIE_TESTING
int trans_count = 0;
int state_count = 0;
int spec_count  = 0;
int req_count  = 0;
int rparse_count = 0;
#endif

#include "common.h"
#include "trie.h"

Import char *malloc();

/*
 * new_transition()
 *
 * Return a pointer to a new Transition struct.
 */
Export Transition *new_transition()
{
    Transition *new = (Transition *)malloc((unsigned) sizeof(Transition));

#ifdef TRIE_TESTING
    new->id = trans_count++;
#endif

    if (!new)
	parse_error("Could not make new transition, malloc failed.");
    return new;
}

/*
 * new_state()
 *
 * Return a pointer to a new State struct.
 */
Export State *new_state()
{
    State *new = (State *)malloc((unsigned) sizeof(State));

#ifdef TRIE_TESTING
    new->id = state_count++;
#endif

    if (!new)
	parse_error("Could not make new state, malloc failed.");
    return new;
}

/*
 * new_special()
 *
 * Return a pointer to a new Special struct.
 */
Export Special *new_special()
{
    Special *new = (Special *)malloc((unsigned) sizeof(Special));

#ifdef TRIE_TESTING
    spec_count++;
#endif

    if (!new)
	parse_error("Could not make new special, malloc failed.");
    return new;
}

/*
 * new_request()
 *
 * Return a pointer to a new Request struct.
 */
Export Request *new_request()
{
    Request *new = (Request *)malloc((unsigned) sizeof(Request));

#ifdef TRIE_TESTING
    req_count++;
#endif

    if (!new)
	parse_error("Could not make new request, malloc failed.");
    return new;
}



/*
 * new_rparse()
 *
 * Return a pointer to a new Rparse struct.
 */
Export Rparse *new_rparse()
{
    Rparse *new = (Rparse *)malloc((unsigned) sizeof(Rparse));

#ifdef TRIE_TESTING
    rparse_count++;
#endif

    if (!new)
	parse_error("Could not make new rparse, malloc failed.");
    return new;
}

