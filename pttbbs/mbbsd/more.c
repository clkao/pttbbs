/* $Id$ */
#include "bbs.h"

/*
 * more.c
 * a mini pager in 130 lines of code, or stub of the huge pager pmore.
 * Author: Hung-Te Lin (piaip), April 2008.
 *
 * Copyright (c) 2008 Hung-Te Lin <piaip@csie.ntu.edu.tw>
 * All rights reserved.
 * Distributed under BSD license (GPL compatible).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 */

static int 
check_sysop_edit_perm(const char *fpath)
{
    if (!HasUserPerm(PERM_SYSOP) ||
	strcmp(fpath, "etc/ve.hlp") == 0)
	return 0;

#ifdef BN_SECURITY
    if (strcmp(currboard, BN_SECURITY) == 0)
	return 0;
#endif // BN_SECURITY

    return 1;
}

#ifdef USE_PMORE
static int 
common_pmore_key_handler(int ch, void *ctx)
{
    switch(ch)
    {
	// Special service keys
	case 'z':
	    if (!HasUserPerm(PERM_BASIC))
		break;
	    return RET_DOCHESSREPLAY;

#if defined(USE_BBSLUA) && !defined(DISABLE_BBSLUA_IN_PAGER)
	case 'L':
	case 'l':
	    if (!HasUserPerm(PERM_BASIC))
		break;
	    return RET_DOBBSLUA;
#endif
	
	// Query information and file touch
	case 'Q':
	    return RET_DOQUERYINFO;

	case Ctrl('T'):
	    if (!HasUserPerm(PERM_BASIC))
		break;
	    return RET_COPY2TMP;

	case 'E':
	    if (!check_sysop_edit_perm("")) // for early check, skip file name (must check again later)
		break;
	    return RET_DOSYSOPEDIT;

	// Making Response
	case '%':
	case 'X':
	    return RET_DORECOMMEND;

	case 'r': case 'R':
	    return RET_DOREPLY;

	case 'Y': case 'y':
	    return RET_DOREPLYALL;

	// Special Navigation
	case 's':
	    if (!HasUserPerm(PERM_BASIC) ||
		currstat != READING)
		break;
	    return RET_SELECTBRD;
	
	/* ------- SOB THREADED NAVIGATION EXITING KEYS ------- */
	// I'm not sure if these keys are all invented by SOB,
	// but let's honor their names.
	// Kaede, Raw, Izero, woju - you are all TWBBS heroes  
	//                                  -- by piaip, 2008.
	case 'A':
	    return AUTHOR_PREV;
	case 'a':
	    return AUTHOR_NEXT;
	case 'F': case 'f':
	    return READ_NEXT;
	case 'B': case 'b':
	    return READ_PREV;

	/* from Kaede, thread reading */
	case ']':
	case '+':
	    return RELATE_NEXT;
	case '[':
	case '-':
	    return RELATE_PREV;
	case '=':
	    return RELATE_FIRST;
    }

    return DONOTHING;
}

static const char 
*hlp_nav [] = 
{ "【瀏覽指令】", NULL,
    "下篇文章  ", "f",
    "前篇文章  ", "b",
    "同主題下篇", "]  +",
    "同主題前篇", "[  -",
    "同主題首篇", "=",
    "同主題循序", "t",
    "同作者前篇", "A",
    "同作者下篇", "a",
    NULL,
},
*hlp_reply [] = 
{ "【回應指令】", NULL,
    "推薦文章", "% X",
    "回信回文", "r",
    "全部回覆", "y",
    NULL,
},
*hlp_spc [] = 
{ "【特殊指令】", NULL,
    "查詢資訊  ", "Q",
    "存入暫存檔", "^T",
    "切換看板  ", "s",
    "棋局打譜  ", "z",
#if defined(USE_BBSLUA) && !defined(DISABLE_BBSLUA_IN_PAGER)
    "執行BBSLua", "L l",
#endif
    NULL,
};

#ifndef PMHLPATTR_NORMAL
#define PMHLPATTR_NORMAL      ANSI_COLOR(0)
#define PMHLPATTR_NORMAL_KEY  ANSI_COLOR(0;1;36)
#define PMHLPATTR_HEADER      ANSI_COLOR(0;1;32)
#endif

// TODO make this help renderer into vtuikit or other location someday
static int 
common_pmore_help_handler(int y, void *ctx)
{
    // simply show ptt special function keys
    int i;
    const char ** p[3] = { hlp_nav, hlp_reply, hlp_spc };
    const int  cols[3] = { 29, 29, 20 },    // columns
               desc[3] = { 14, 11, 13 };    // desc width
    move(y+1, 0);
    // render help page
    while (*p[0] || *p[1] || *p[2])
    {
        y++;
        for ( i = 0; i < 3; i++ )
        {
            const char *dstr = "", *kstr = "";
            if (*p[i]) {
                dstr = *p[i]++; kstr = *p[i]++;
            }
            if (!kstr)
                prints( PMHLPATTR_HEADER "%-*s", cols[i], dstr);
            else
                prints( PMHLPATTR_NORMAL " %-*s"
                        PMHLPATTR_NORMAL_KEY "%-*s",
                        desc[i], dstr, cols[i]-desc[i]-1, kstr);
        }
        outs("\n");
    }
    PRESSANYKEY();
    return 0;
}

/* use new pager: piaip's more. */
int 
more(const char *fpath, int promptend)
{
    int r = pmore2(fpath, promptend,
	    (void*) fpath,
	    common_pmore_key_handler, 
	    common_pmore_help_handler);

    // post processing
    switch(r)
    {

	case RET_DOSYSOPEDIT:
	    r = FULLUPDATE;
	    if (!check_sysop_edit_perm(fpath))
		break;
	    log_filef("log/security", LOG_CREAT,
		    "%u %s %d %s admin edit file=%s\n", 
		    (int)now, Cdate(&now), getpid(), cuser.userid, fpath);
	    veditfile(fpath);
	    break;

	case RET_COPY2TMP:
	    r = FULLUPDATE;
	    if (HasUserPerm(PERM_BASIC))
	    {
		char buf[PATHLEN];
		getdata(b_lines - 1, 0, "把這篇文章收入到暫存檔？[y/N] ",
			buf, 4, LCECHO);
		if (buf[0] != 'y')
		    break;
		setuserfile(buf, ask_tmpbuf(b_lines - 1));
		Copy(fpath, buf);
	    }
	    break;

	case RET_SELECTBRD:
	    r = FULLUPDATE;
	    if (currstat == READING)
		Select();
	    break;

	case RET_DOCHESSREPLAY:
	    r = FULLUPDATE;
	    if (HasUserPerm(PERM_BASIC))
		ChessReplayGame(fpath);
	    break;

#if defined(USE_BBSLUA) && !defined(DISABLE_BBSLUA_IN_PAGER)
	case RET_DOBBSLUA:
	    r = FULLUPDATE;
	    // check permission again
	    if (HasUserPerm(PERM_BASIC)) 
		bbslua(fpath);
	    break;
#endif
    }

    return r;
}

#else  // !USE_PMORE

// minimore: a mini pager in exactly 130 lines
#define PAGER_MAXLINES (2048)
int more(const char *fpath, int promptend)
{
    FILE *fp = fopen(fpath, "rt");
    int  lineno = 0, lines = 0, oldlineno = -1;
    int  i = 0, abort = 0, showall = 0, colorize = 0;
    int  lpos[PAGER_MAXLINES] = {0}; // line position
    char buf [ANSILINELEN];

    if (!fp) return -1; 
    clear();

    if (promptend == NA) {	    // quick print one page
	for (i = 0; i < t_lines-1; i++)
	    if (!fgets(buf, sizeof(buf), fp)) 
		break; 
	    else 
		outs(buf);
	fclose(fp); 
	return 0;
    }
    // YEA mode: pre-read
    while (lines < PAGER_MAXLINES-1 && 
	   fgets(buf, sizeof(buf), fp) != NULL)
	lpos[++lines] = ftell(fp);
    rewind(fp);

    while (!abort) 
    {
	if (oldlineno != lineno)    // seek and print
	{ 
	    clear(); 
	    showall = 0; 
	    oldlineno = lineno;
	    fseek(fp, lpos[lineno], SEEK_SET);

	    for (i = 0, buf[0] = 0; i < t_lines-1; i++, buf[0] = 0) 
	    {
		if (!showall) 
		{
		    fgets(buf, sizeof(buf), fp);
		    if (lineno + i == 0 && 
			(strncmp(buf, STR_AUTHOR1, strlen(STR_AUTHOR1))==0 ||
			 strncmp(buf, STR_AUTHOR2, strlen(STR_AUTHOR2))==0))
			colorize = 1;
		}

		if (!buf[0]) 
		{
		    outs("\n");
		    showall = 1;
		} else {
		    // dirty code to render heeader
		    if (colorize && lineno+i < 4 && *buf && 
			    *buf != '\n' && strchr(buf, ':'))
		    {
			char *q1 = strchr(buf, ':');
			int    l = t_columns - 2 - strlen(buf);
			char *q2 = strstr(buf, STR_POST1);

			chomp(buf); 
			if (q2 == NULL) q2 = strstr(buf, STR_POST2);
			if (q2)	    { *(q2-1) = 0; q2 = strchr(q2, ':'); }
			else q2 = q1;

			*q1++ = 0;	*q2++ = 0; 
			if (q1 == q2)	 q2 = NULL;

			outs(ANSI_COLOR(34;47) " ");
			outs(buf); outs(" " ANSI_REVERSE); 
			outs(q1);  prints("%*s", l, ""); q1 += strlen(q1);

			if (q2) {
			    outs(ANSI_COLOR(0;34;47) " ");  outs(q1+1);
			    outs(" " ANSI_REVERSE);	    outs(q2);
			}
			outs(ANSI_RESET"\n");
		    } else 
			outs(buf);
		}
	    }
	    if (lineno + i >= lines) 
		showall = 1;

	    // print prompt bar
	    snprintf(buf, sizeof(buf),
		    "  瀏覽 P.%d  ", 1 + (lineno / (t_lines-2)));
	    vs_footer(buf, 
	    " (→↓[PgUp][PgDn][Home][End])游標移動\t(←/q)結束");
	}
	// process key
	switch(vkey()) {
	    case KEY_UP: case 'k': case Ctrl('P'):
		if (lineno == 0) abort = READ_PREV;
		lineno--;		    
		break;

	    case KEY_PGUP: case Ctrl('B'):
		if (lineno == 0) abort = READ_PREV;
		lineno -= t_lines-2;	    
		break;

	    case KEY_PGDN: case Ctrl('F'): case ' ':
	    case KEY_RIGHT:
		if (showall) abort = READ_NEXT;
		lineno += t_lines-2;	    
		break;

	    case KEY_DOWN: case 'j': case Ctrl('N'):
		if (showall) abort = READ_NEXT;
		lineno++;		    
		break;
	    case KEY_HOME: case Ctrl('A'):
		lineno = 0;		    
		break;
	    case KEY_END: case Ctrl('E'):
		lineno = lines - (t_lines-1); 
		break;
	    case KEY_LEFT: case 'q':
		abort = FULLUPDATE;		    
		break;

	    case 'b':
		abort = READ_PREV;
		break;
	    case 'f':
		abort = READ_NEXT;
		break;
	}
	if (lineno + (t_lines-1) >= lines)
	    lineno = lines-(t_lines-1);
	if (lineno < 0)
	    lineno = 0;
    }
    fclose(fp);
    return abort > 0 ? abort : 0;
}

#endif // !USE_PMORE
