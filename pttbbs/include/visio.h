// $Id$
#ifndef _VISIO_H
#define _VISIO_H

/*
 * visio.c
 * High-level virtual screen input output control
 */

#include "bbs.h"
#include "ansi.h"   // we need it.
#include <limits.h>

// THEME DEFINITION ----------------------------------------------------
#define VCLR_HEADER		ANSI_COLOR(1;37;46)	// was: TITLE_COLOR
#define VCLR_HEADER_MID		ANSI_COLOR(1;33;46)
#define VCLR_HEADER_RIGHT	ANSI_COLOR(1;37;46)
#define VCLR_HDR		ANSI_COLOR(1;37;46)
#define VCLR_FOOTER_CAPTION     ANSI_COLOR(0;34;46)
#define VCLR_FOOTER             ANSI_COLOR(0;30;47)
#define VCLR_FOOTER_QUOTE       ANSI_COLOR(0;31;47)
#define VCLR_MSG_FLOAT		ANSI_COLOR(1;33;46)
#define VCLR_MSG		ANSI_COLOR(1;36;44)
#define VCLR_PAUSE_PAD		ANSI_COLOR(1;34;44)
#define VCLR_PAUSE		ANSI_COLOR(1;37;44)

#define VMSG_PAUSE		" 請按任意鍵繼續 "
#define VMSG_PAUSE_PAD		"▄"
#define VMSG_MSG_FLOAT		" [按任意鍵繼續]"
#define VMSG_MSG_PREFIX		" ◆ "
#define VMSG_HDR_PREFIX		"【 "
#define VMSG_HDR_POSTFIX	" 】"

// CONSTANT DEFINITION -------------------------------------------------
#define VCOL_ALIGN_LEFT		(0)
#define VCOL_ALIGN_RIGHT	(1)
// #define VCOL_ALIGN_MIDDLE	(2)
#define VCOL_MAXW		(INT16_MAX)

// DATATYPE DEFINITION -------------------------------------------------
typedef void *	VREFSCR;
typedef long	VREFCUR;

typedef short	VCOLW;
typedef struct {
    char *caption;
    char *attr;	    // default attribute
    VCOLW minw;	    // minimal width
    VCOLW maxw;	    // max width
    char has_ansi;  // support ANSI escape sequence
    char align;	    // alignment
    char usewhole;  // draw entire column, not leaving borders
    char flag;	    // reserved
} VCOL;

// API DEFINITION ----------------------------------------------------
// int  vans(char *prompt);	// prompt at bottom and return y/n in lower case.
// void vs_bar(char *title);    // like stand_title
void vpad   (int n, const char *pattern);
 int vgety  (void);
void vbarf  (const char *s, ...)  GCC_CHECK_FORMAT(1,2);
void vbarlr (const char *l, const char *r);
int  vmsgf  (const char *fmt,...) GCC_CHECK_FORMAT(1,2);
int  vmsg   (const char *msg);
int  vansf  (const char *fmt,...) GCC_CHECK_FORMAT(1,2);
int  vans   (const char *msg);
void vshowmsg(const char *msg);

void prints(const char *fmt, ...) GCC_CHECK_FORMAT(1,2);
void mvouts(int y, int x, const char *str);

// vs_*: formatted and themed virtual screen layout
// you cannot use ANSI escapes in these APIs.
void vs_header	(const char *title,   const char *mid, const char *right);	// vs_head, showtitle
void vs_hdr	(const char *title);						// vs_bar,  stand_title
void vs_footer	(const char *caption, const char *prompt);

// columned output
void vs_cols_layout (const VCOL* cols, VCOLW *ws, int n);
void vs_cols_hdr    (const VCOL* cols, const VCOLW *ws, int n);
void vs_cols	    (const VCOL* cols, const VCOLW *ws, int n, ...);

// compatible macros
#define stand_title vs_hdr

// VREF API:
VREFSCR	vscr_save   (void);
void	vscr_restore(VREFSCR);
VREFCUR vcur_save   (void);
void	vcur_restore(VREFCUR);

#endif // _VISIO_H
