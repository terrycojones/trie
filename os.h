/* $Header$ */

/*
 * This file is part of the PCS xterm+ program.
 *
 * Copyright (c) 1990 PCS Computer Systeme, GmbH.
 * All rights reserved.
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF PCS Computer Systeme, GmbH
 * The copyright notice above does not evidence any actual or intended
 * publication of such source code.
 *
 */

/*
 * Operating system specific interface.
 *
 * Author: Jordan K. Hubbard and Digital Equipment Corporation
 * Date: March 20th, 1990.
 * Description: This file contains (or includes) all OS specific portions
 *		of xterm+. Make any OS specific changes to (and only to)
 *		this file if you absolutely have to, but you probably won't
 *		even want to look at it. I just LOVE all the different Unix's
 *		we have out there these days. Wish we had more.
 *
 * Revision History:
 *
 * $Log$
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>

/* Our generic type */
typedef caddr_t Generic;

#if defined(SYSV) && !defined(PCS)
#define	killpg(x,sig)	kill(-x,sig)
#ifndef CRAY
#define	dup2(fd1,fd2)	((fd1 == fd2) ? fd1 : \
				(close(fd2), fcntl(fd1, F_DUPFD, fd2)))
#endif
#endif	/* !SYSV || PCS */

#ifdef SIGTSTP
#include <sys/wait.h>
#ifdef hpux
#include <sys/bsdtty.h>
#endif
#endif

#ifdef att
#define USE_USG_PTYS
#else
#define USE_HANDSHAKE
#endif

#include <sys/ioctl.h>

#ifdef SYSV
#include <sys/termio.h>
#ifdef USE_USG_PTYS			/* AT&T SYSV has no ptyio.h */
#include <sys/stream.h>			/* get typedef used in ptem.h */
#include <sys/ptem.h>			/* get struct winsize */
#include <sys/stropts.h>		/* for I_PUSH */
#include <poll.h>			/* for POLLIN */
#endif
#include <sys/stat.h>
#define USE_SYSV_TERMIO
#define USE_SYSV_SIGNALS
#define	USE_SYSV_PGRP
#define USE_SYSV_ENVVARS		/* COLUMNS/LINES vs. TERMCAP */
/*
 * now get system-specific includes
 */
#ifdef CRAY
#define HAS_UTMP_UT_HOST
#define HAS_BSD_GROUPS
#endif
#ifdef macII
#define HAS_UTMP_UT_HOST
#define HAS_BSD_GROUPS
#include <sys/ttychars.h>
#undef USE_SYSV_ENVVARS
#undef FIOCLEX
#undef FIONCLEX
#define setpgrp2 setpgrp
#include <sgtty.h>
#include <sys/resource.h>
#endif
#ifdef hpux
#define HAS_BSD_GROUPS
#include <sys/ptyio.h>
#endif
#endif /* SYSV */

#ifndef SYSV				/* BSD systems */
#include <sgtty.h>
#include <sys/resource.h>
#define HAS_UTMP_UT_HOST
#define HAS_BSD_GROUPS
#endif	/* !SYSV */

#include <stdio.h>
#include <errno.h>
#include <setjmp.h>

#ifdef hpux
#include <sys/utsname.h>
#endif /* hpux */

#ifdef apollo
#define ttyslot() 1
#endif /* apollo */

#include <utmp.h>
#ifdef LASTLOG
#include <lastlog.h>
#endif
#include <sys/param.h>	/* for NOFILE */

#ifdef  PUCC_PTYD
#include <local/openpty.h>
int	Ptyfd;
#endif /* PUCC_PTYD */

#ifndef UTMP_FILENAME
#define UTMP_FILENAME "/etc/utmp"
#endif
#ifndef LASTLOG_FILENAME
#define LASTLOG_FILENAME "/usr/adm/lastlog"  /* only on BSD systems */
#endif
#ifndef WTMP_FILENAME
#if defined(SYSV)
#define WTMP_FILENAME "/etc/wtmp"
#else
#define WTMP_FILENAME "/usr/adm/wtmp"
#endif
#endif

#ifndef PTYDEV
#ifdef hpux
#define	PTYDEV		"/dev/ptym/ptyxx"
#else	/* !hpux */
#define	PTYDEV		"/dev/ptyxx"
#endif	/* !hpux */
#endif	/* !PTYDEV */

#ifndef TTYDEV
#ifdef hpux
#define TTYDEV		"/dev/pty/ttyxx"
#else	/* !hpux */
#define	TTYDEV		"/dev/ttyxx"
#endif	/* !hpux */
#endif	/* !TTYDEV */

#ifndef PTYCHAR1
#ifdef hpux
#define PTYCHAR1	"zyxwvutsrqp"
#else	/* !hpux */
#ifdef PCS
#define PTYCHAR1        "qrstuvw"
#else   /* !hpux && !PCS */
#define	PTYCHAR1	"pqrstuvwxyz"
#endif  /* !PCS */
#endif	/* !hpux */
#endif	/* !PTYCHAR1 */

#ifndef PTYCHAR2
#ifdef hpux
#define	PTYCHAR2	"fedcba9876543210"
#else	/* !hpux */
#define	PTYCHAR2	"0123456789abcdef"
#endif	/* !hpux */
#endif	/* !PTYCHAR2 */
