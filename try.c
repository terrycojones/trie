#ifndef lint
static char *rcsid = "$Header: /user1/terry/s/trie/RCS/try.c,v 2.0 90/04/25 13:42:03 terry Exp $";
#endif


/*
 * $Log:	try.c,v $
 * Revision 2.0  90/04/25  13:42:03  terry
 * Stable for first real test with canvas.
 * 
 * Revision 1.7  90/04/24  18:50:12  jkh
 * trash.
 * 
 * Revision 1.6  90/04/24  16:54:45  jkh
 * Clean-up to make things work with the new stuff. Fun fun fun.
 * 
 * Revision 1.5  90/04/18  17:24:30  terry
 * check in so jordan can take it home.
 * 
 * Revision 1.4  90/04/18  00:01:48  terry
 * Changes for reverse parsing go in.
 * 
 * Revision 1.3  90/04/10  21:13:27  terry
 * added 'w' for writing out the trie (which calls
 * trie_print()) etc etc.
 * 
 * Revision 1.2  90/03/26  22:27:58  terry
 * goes with trie.c revision 1.2. all seems well except for the problem
 * of identiacl prefixes with special info in the same place.
 * 
 * Revision 1.1  90/03/14  18:18:17  terry
 * Initial revision
 * 
 *
 */

#include "common.h"
#include "trie.h"

Local char *myname;
Local ComBlock rp_blk;

Export int main(argc, argv)
int argc;
char **argv;
{
    Local void prompt();
    Local void help();
    Import void exit();
    Import int open();
    Import int close();
    Import void parse_init();
    char line[1024];
    
    myname = *argv;
    
    parse_init();
    prompt();
    
    while (gets(line)){
	
	int action = line[0];
	char *key;
	
	key = line + 1;
	while (*key == ' ' || *key == '\t'){
	    key++;
	}

	switch (action){
	    case 'a':{
		parse_add(key);
		break;
	    }
	    case 'c':{
		register int i;
		for (i = 0; i < CB_NREGS; i++){
		    rp_blk.regs[i].data = (Generic)0;
		    rp_blk.regs[i].type = CB_INT_TYPE;
		} 
		break;
	    }
	    case 'd':{
		parse_sub(key);
		break;
	    }
	    case 'e':{
		Request *r = exactly_in_trie(key);
		if (r){
		    printf("request ids are ");
		    while (r){
			printf("%d ", r->request_id);
			r = r->next;
		    }
		    printf("\n");
		}
		else{
		    printf("no\n");
		}
		break;
	    }
	    case 'h':{
		help();
		break;
	    }
	    case 'i':{
		parse_init();
		break;
	    }
	    case 'l':{
		char *tmp = key + 1;
		int val = 0;
		int reg = *key - 'a';
		while (*tmp == ' ' || *tmp == '\t'){
		    tmp++;
		}
		while (isdigit(*tmp)){
		    val = val * 10 + *tmp - '0';
		    tmp++;
		}
		rp_blk.regs[reg].data = (Generic)val;
		rp_blk.regs[reg].type = CB_INT_TYPE;
		break;
	    }
	    case 'm':{
		trie_memory();
		break;
	    }
	    case 'p':{
		Import void parse();
		int d;
		
		d = open(key, O_RDONLY);

		if (d == -1){
		    /* Put the string into a file and then open it. */
		    char *f = "..gobbledygook";
		    FILE *fp = fopen(f, "w");
		    if (!fp){
			printf("could not fopen %s\n", f);
			break;
		    }
		    fprintf(fp, "%s", key);
		    if (fclose(fp) == EOF){
			parse_error("Could not fclose %s.", f);
		    }
		    d = open(f, O_RDONLY);
		    if (d == -1){
			parse_error("could not re-open %s", f);
		    }
		}
		
		printf("parsing %s\n", key);
		parse(d);
		close(d);
		break;
	    }
	    case 'P':{
		prefix_in_trie(key);
		break;
	    }
	    case '+':{
		add_file(key);
		printf("added %s\n", key);
		break;
	    }
	    case '-':{
		delete_file(key);
		printf("deleted %s\n", key);
		break;
	    }
	    case 'r':{
		print_reverse();
		break;
	    }
	    case 'R':{
		Import void print_regs();
		print_regs(key, 0);
		break;
	    }
	    case 's':{
		Import void print_stack();
		print_stack();
		break;
	    }
	    case 'u':{
		Import int atoi();
		rp_blk.opcode = atoi(key);
		rparse(&rp_blk);
		printf("\n");
		break;
	    }
	    case 'U':{
		Import void print_regs();
		print_regs(key, &rp_blk);
		break;
	    }
	    case 'w':{
		/* write out the trie contents. */
		FILE *d;
		if (!*key){
		    print_trie(stdout);
		    break;
		}
		
		d = fopen(key, "w");
		if (!d){
		    printf("could not fopen '%s'\n", key);
		    break;
		}
		print_trie(d);
		printf("wrote '%s'\n", key);
		if (fclose(d) == EOF){
		    fprintf(stderr, "%s: Could not fclose '%s'.\n", myname,
			    key);
		    exit(1);
		}
		break;
	    }
	    case 'x':
	    case 'q':{
		exit(0);
	    }
	    case '\0':{
		break;
	    }
	    case '?':{
		help();
		break;
	    }
	    default:{
		printf("?\n");
		break;
	    }
	}
	prompt();
    }
    return 0;
}

Local void prompt()
{
    printf("trie ==> ");
}

Local void help()
{
    printf("\
    a ID<string>  add <string> to the parser\n\
    c             clear the rparse common block registers.\n\
    d ID<string>  delete <string> from the parser.\n\
    e <string>    is the string exactly in the trie.\n\
    help          this help.
    i             initialise parser.\n\
    l x y         let register x have value y for rparse\n\
    m             show memory usage.\n\
    p <file>      parse the contents of <file> or use <file> as a string if it does not exist.\n\
    P <string>    is string a prefix of a sequence?.\n\
    + <file>      add the sequences in file to the parser.\n\
    - <file>      remove the sequences in file from the parser.\n\
    q             quit.\n\
    r             show the contents of the reverse parse table.\n\
    R             print the registers in the common block.\n\
    s             print the internal stack.\n\
    U regs        print some registers in the rparse comblock.\n\
    u opcode      unparse (i.e. rparse) opcode.
    w [<file>]    dump the trie in a readable form.\n\
    x             exit.\n\
    ?             this help.\n\
");
    
    return;
}
