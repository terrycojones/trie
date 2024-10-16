#ifndef _XP_COMMON_H
#define _XP_COMMON_H
/* $Header$ */

/*
 * This file is part of the PCS xterm+ program.
 *
 * Copyright (c) 1990 PCS Computer Systeme, GmbH.
 * All rights reserved.
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF PCS Computer Systeme, GbmH
 * The copyright notice above does not evidence any actual or intended
 * publication of such source code.
 *
 */

/*
 * Common definitions for PCS xterm+.
 *
 * Author: Jordan K. Hubbard, Michael Elbel and Terrence Jones
 * Date: March 20th, 1990.
 * Description: This file is included by all portions of xterm+. OS or site
 *		specific changes should not be made here. Those go in os.h
 *		and config.h
 *
 * Revision History:
 *
 * $Log$
 *
 */

#include "os.h"
#include "config.h"

#include <X11/Intrinsic.h>

/**************************************************************
 * Various helper macros/typedefs to make the code more clear *
 **************************************************************/

#define Local	static
#define Import	extern
#define Export


/*********************************************************
 * Types and macros for various internal data structures *
 *********************************************************/

/* The number of registers in a common block. (Should always be 26) */
#define CB_NREGS            26

/* Types for register contents. */
#define CB_INT_TYPE         0
#define CB_STR_TYPE         1
#define CB_CHAR_TYPE        2

/* A register */
typedef struct _reg {
     int type;			/* data type */
     Generic data;		/* data ptr */
} Register;

#define reg_size		(sizeof(struct _reg))
#define reg_type(reg)		((reg).type)
#define reg_data(reg)		((reg).data)

/* A communications block */
typedef struct _comblock {
     int opcode;		/* operation code */
     char *buffer;		/* data buffer */
     int nbytes;		/* data buffer size */
     Register regs[CB_NREGS];	/* register array */
} ComBlock, *ComBlockPtr;

/* Macros to access a comblock */
#define cb_size			(sizeof(struct _comblock))
#define cb_opcode(x)		(((ComBlockPtr)(x))->opcode)
#define cb_buffer(x)		(((ComBlockPtr)(x))->buffer)
#define cb_nbytes(x)		(((ComBlockPtr)(x))->nbytes)
#define cb_reg(x, r)		(((ComBlockPtr)(x))->regs[(r) - 'A'])

/* shorthand */
#define cb_reg_type(x, r)	(reg_type(cb_reg((x), (r))))
#define cb_reg_data(x, r)	(reg_data(cb_reg((x), (r))))

/* for the pty routines */
#define PTY_IN	0
#define PTY_OUT	1

typedef struct _ptydesc {
     String name;	/* name of pty chosen */
     int pid;		/* process ID of associated process */
     int pgrp;		/* process group of associated process */
     FILE *master[2];	/* master side */
     FILE *slave[2];	/* slave side */
} PtyDesc, *PtyDescPtr;

/* macros to access a ptydesc */
#define pty_name(pty)		(((PtrDescPtr)(pty))->name)
#define pty_pid(pty)		(((PtrDescPtr)(pty))->pid)
#define pty_pgrp(pty)		(((PtrDescPtr)(pty))->pgrp)
#define pty_master(pty)		(((PtrDescPtr)(pty))->master)
#define pty_slave(pty)		(((PtrDescPtr)(pty))->slave)

/* Use these on master/slaves (I.E.: pty_input_side(ptr_master(foo))) */
#define pty_input_side(fp)	((fp)[PTY_IN])
#define pty_output_side(fp)	((fp)[PTY_OUT])

/*
 * Operation codes
 */

#define OP_START		0
/* basic character insertion */
#define OP_INSERT		(OP_START + 0)
/* movement */
#define OP_MOVE_ABS		(OP_START + 1)	/* Col/Line in X/Y */
#define OP_MOVE_REL		(OP_START + 2)	/* Col/Line in X/Y */
#define OP_MOVE_ABS_COLUMN	(OP_START + 3)	/* Col in X */
#define OP_MOVE_ABS_ROW		(OP_START + 4)	/* Line in Y */
#define OP_MOVE_REL_COLUMN	(OP_START + 5)	/* Delta Col in X */
#define OP_MOVE_REL_ROW		(OP_START + 6)	/* Delta Line in Y */
/* write modes */
#define OP_INSERT_MODE		(OP_START + 7)	/* no args */
#define OP_OVERWRITE_MODE	(OP_START + 8)	/* no args */
/* deleting */
#define OP_DELETE_CHARS		(OP_START + 9)	/* A is count */
#define OP_DELETE_TO_EOL	(OP_START + 10)	/* no args */
#define OP_DELETE_LINES		(OP_START + 11)	/* XXX count arg in? XXX */
#define OP_DELETE_TO_EOSCR	(OP_START + 12)	/* no args */
/* inserting */
#define OP_INSERT_BLANKS	(OP_START + 13)	/* XXX count arg in? XXX */
#define OP_INSERT_LINES		(OP_START + 14)	/* XXX count arg in? XXX */
/* erase screen */
#define OP_CLEAR_SCREEN		(OP_START + 15)	/* no args */
/* define the scrolling region */
#define OP_SET_SCROLL_REGION	(OP_START + 16) /* Start Line in A, End in B */
/* more cursor moving */
#define OP_MOVE_REL_ROW_SCROLLED (OP_START + 17) /* e.g. for LF, delta in Y */
/* last OP */
#define OP_END			(OP_START + 17)

/* number of op's */
#define OP_NUM (OP_END - OP_START + 1)


/**********************************************
 * External function and variable definitions *
 **********************************************/

Import int Debug;

/* Misc routines */
Import void fatal(), warn(), debug();

/* The dispatcher - called by the parser when an esc seq has been parsed. */
Import void dispatch();
Import void parse();
Import void rparse();
Import void parse_add();
Import void parse_sub();
Import void parse_init();

#endif /* _XP_COMMON_H */
