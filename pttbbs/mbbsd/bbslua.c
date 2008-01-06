//////////////////////////////////////////////////////////////////////////
// BBS Lua Project
//
// Author: Hung-Te Lin(piaip), Jan. 2008. 
// <piaip@csie.ntu.edu.tw>
// This source is released in MIT License.
//
// TODO:
//  1. add quick key/val conversion
//  2. add key values (UP/DOWN/...)
//  3. remove i/o libraries [done]
//  4. add system break key (Ctrl-C?)
//////////////////////////////////////////////////////////////////////////

#include "bbs.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

//////////////////////////////////////////////////////////////////////////
// CONST DEFINITION
//////////////////////////////////////////////////////////////////////////

#define BBSLUA_VERSION_MAJOR	(0)
#define BBSLUA_VERSION_MINOR	(1)
#define BBSLUA_VERSION_STR		"0.01"
#define BBSLUA_SIGNATURE		"--#BBSLUA"
#define BBSLUA_EOFSIGNATURE		"--\n"

//////////////////////////////////////////////////////////////////////////
// CONFIGURATION VARIABLES
//////////////////////////////////////////////////////////////////////////
#define BLAPI_PROTO		int

#define BLCONF_EXEC_COUNT	(1000)
#define BLCONF_PEEK_TIME	(0.1)
#define BLCONF_BREAK_KEY	Ctrl('C')

//////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////
static int abortBBSLua = 0;

//////////////////////////////////////////////////////////////////////////
// BBSLUA API IMPLEMENTATION
//////////////////////////////////////////////////////////////////////////

BLAPI_PROTO
bl_getyx(lua_State* L)
{
	int y, x;
	getyx(&y, &x);
	lua_pushinteger(L, y);
	lua_pushinteger(L, x);
	return 2;
}

BLAPI_PROTO
bl_getmaxyx(lua_State* L)
{
	lua_pushinteger(L, t_lines);
	lua_pushinteger(L, t_columns);
	return 2;
}

BLAPI_PROTO
bl_move(lua_State* L)
{
	int n = lua_gettop(L);
	int y = 0, x = 0;
	if (n > 0)
		y = lua_tointeger(L, 1);
	if (n > 1)
		x = lua_tointeger(L, 2);
	move(y, x);
	return 0;
}

BLAPI_PROTO
bl_moverel(lua_State* L)
{
	int n = lua_gettop(L);
	int y = 0, x = 0;
	getyx(&y, &x);
	if (n > 0)
		y += lua_tointeger(L, 1);
	if (n > 1)
		x += lua_tointeger(L, 2);
	move(y, x);
	getyx(&y, &x);
	lua_pushinteger(L, y);
	lua_pushinteger(L, x);
	return 2;
}

BLAPI_PROTO
bl_clear(lua_State* L)
{
	clear();
	return 0;
}

BLAPI_PROTO
bl_clrtoeol(lua_State* L)
{
	clrtoeol();
	return 0;
}

BLAPI_PROTO
bl_clrtobot(lua_State* L)
{
	clrtobot();
	return 0;
}

BLAPI_PROTO
bl_refresh(lua_State* L)
{
	refresh();
	return 0;
}

BLAPI_PROTO
bl_redrawwin(lua_State* L)
{
	redrawwin();
	return 0;
}

BLAPI_PROTO
bl_addstr(lua_State* L)
{
	int n = lua_gettop(L);
	int i = 1;
	for (i = 1; i <= n; i++)
	{
		const char *s = lua_tostring(L, i);
		if(s)
			outs(s);
	}
	return 0;
}

void
bl_k2s(lua_State* L, int v)
{
	if (v <= 0)
		lua_pushnil(L);
	else if (v == KEY_TAB)
		lua_pushstring(L, "TAB");
	else if (v == '\b' || v == 0x7F)
		lua_pushstring(L, "BS");
	else if (v == '\n' || v == '\r' || v == Ctrl('M'))
		lua_pushstring(L, "ENTER");
	else if (v < ' ')
		lua_pushfstring(L, "^%c", v-1+'A');
	else if (v < 0x100)
		lua_pushfstring(L, "%c", v);
	else if (v >= KEY_F1 && v <= KEY_F12)
		lua_pushfstring(L, "F%d", v - KEY_F1 +1);
	else switch(v)
	{
		case KEY_UP:	lua_pushstring(L, "UP");	break;
		case KEY_DOWN:	lua_pushstring(L, "DOWN");	break;
		case KEY_RIGHT: lua_pushstring(L, "RIGHT"); break;
		case KEY_LEFT:	lua_pushstring(L, "LEFT");	break;
		case KEY_HOME:	lua_pushstring(L, "HOME");	break;
		case KEY_END:	lua_pushstring(L, "END");	break;
		case KEY_INS:	lua_pushstring(L, "INS");	break;
		case KEY_DEL:	lua_pushstring(L, "DEL");	break;
		case KEY_PGUP:	lua_pushstring(L, "PGUP");	break;
		case KEY_PGDN:	lua_pushstring(L, "PGDN");	break;
		default:		lua_pushnil(L);				break;
	}
}

BLAPI_PROTO
bl_getch(lua_State* L)
{
	int c = igetch();
	if (c == BLCONF_BREAK_KEY)
	{
		abortBBSLua = 1;
		return lua_yield(L, 0);
	}
	bl_k2s(L, c);
	return 1;
}


BLAPI_PROTO
bl_getstr(lua_State* L)
{
	int y, x;
	char buf[PATHLEN] = "";
	int len = 2, echo = 1;
	int n = lua_gettop(L);

	if (n > 0)
		len = lua_tointeger(L, 1);
	if (n > 1)
		echo = lua_tointeger(L, 2);

	if (len < 2)
		len = 2;
	if (len >= sizeof(buf))
		len = sizeof(buf)-1;

	// TODO process Ctrl-C here
	getyx(&y, &x);
	len = getdata(y, x, NULL, buf, len, echo);
	if (len)
		lua_pushstring(L, buf);
	return len ? 1 : 0;
}

BLAPI_PROTO
bl_kbhit(lua_State *L)
{
	int n = lua_gettop(L);
	double f = 0.1f;
	
	if (n > 0)
		f = (double)lua_tonumber(L, 1);

	if (f < 0.1f)
		f = 0.1f;
	if (f > 10*60)
		f = 10*60;

	if (num_in_buf() || wait_input(f, 0))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}

BLAPI_PROTO
bl_pause(lua_State* L)
{
	int n = lua_gettop(L);
	const char *s = NULL;
	if (n > 0)
		s = lua_tostring(L, 1);

	n = vmsg(s);
	if (n == BLCONF_BREAK_KEY)
	{
		abortBBSLua = 1;
		return lua_yield(L, 0);
	}
	lua_pushinteger(L, n);
	return 1;
}

BLAPI_PROTO
bl_title(lua_State* L)
{
	int n = lua_gettop(L);
	const char *s = NULL;
	if (n > 0)
		s = lua_tostring(L, 1);
	if (s == NULL)
		return 0;

	stand_title(s);
	return 0;
}

BLAPI_PROTO
bl_ansi_color(lua_State *L)
{
	char buf[PATHLEN] = ESC_STR "[";
	char *p = buf + strlen(buf);
	int i = 1;
	int n = lua_gettop(L);
	if (n >= 10) n = 10;
	for (i = 1; i <= n; i++)
	{
		if (i > 1) *p++ = ';';
		sprintf(p, "%d", (int)lua_tointeger(L, i));
		p += strlen(p);
	}
	*p++ = 'm';
	*p   = 0;
	lua_pushstring(L, buf);
	return 1;
}

BLAPI_PROTO
bl_attrset(lua_State *L)
{
	char buf[PATHLEN] = ESC_STR "[";
	char *p = buf + strlen(buf);
	int i = 1;
	int n = lua_gettop(L);
	for (i = 1; i <= n; i++)
	{
		sprintf(p, "%dm",(int)lua_tointeger(L, i)); 
		outs(buf);
	}
	return 0;
}

BLAPI_PROTO
bl_time(lua_State *L)
{
	syncnow();
	lua_pushinteger(L, now);
	return 1;
}

BLAPI_PROTO
bl_ctime(lua_State *L)
{
	syncnow();
	lua_pushstring(L, ctime4(&now));
	return 1;
}

BLAPI_PROTO
bl_userid(lua_State *L)
{
	lua_pushstring(L, cuser.userid);
	return 1;
}

//////////////////////////////////////////////////////////////////////////
// BBSLUA LIBRARY
//////////////////////////////////////////////////////////////////////////

static const struct luaL_reg lib_bbslua [] = {
	/* curses output */
	{ "getyx",		bl_getyx },
	{ "getmaxyx",	bl_getmaxyx },
	{ "move",		bl_move },
	{ "moverel",	bl_moverel },
	{ "clear",		bl_clear },
	{ "clrtoeol",	bl_clrtoeol },
	{ "clrtobot",	bl_clrtobot },
	{ "refresh",	bl_refresh },
	{ "redrawwin",	bl_redrawwin },
	{ "addstr",		bl_addstr },
	{ "print",		bl_addstr },
	{ "outs",		bl_addstr },
	/* input */
	{ "getch",		bl_getch },
	{ "getdata",	bl_getstr },
	{ "getstr",		bl_getstr },
	{ "kbhit",		bl_kbhit },
	/* BBS utilities */
	{ "pause",		bl_pause },
	{ "title",		bl_title },
	{ "userid",		bl_userid },
	/* time */
	{ "time",		bl_time },
	{ "now",		bl_time },
	{ "ctime",		bl_ctime },
	/* ANSI helpers */
	{ "ANSI_COLOR",	bl_ansi_color },
	{ "color",		bl_attrset },
	{ "attrset",	bl_attrset },
	{ NULL, NULL},
};

static const luaL_Reg mylualibs[] = {
  {"", luaopen_base},

  // {LUA_LOADLIBNAME, luaopen_package},
  {LUA_TABLIBNAME, luaopen_table},
  // {LUA_IOLIBNAME, luaopen_io},
  // {LUA_OSLIBNAME, luaopen_os},
  {LUA_STRLIBNAME, luaopen_string},
  {LUA_MATHLIBNAME, luaopen_math},
  // {LUA_DBLIBNAME, luaopen_debug},

  {NULL, NULL}
};


LUALIB_API void myluaL_openlibs (lua_State *L) {
  const luaL_Reg *lib = mylualibs;
  for (; lib->func; lib++) {
    lua_pushcfunction(L, lib->func);
    lua_pushstring(L, lib->name);
    lua_call(L, 1, 0);
  }
}

static void
bbsluaRegConst(lua_State *L, const char *globName)
{
	// section
	lua_getglobal(L, globName);
	lua_pushstring(L, "ESC"); lua_pushstring(L, ESC_STR);
	lua_settable(L, -3);

	lua_getglobal(L, globName);
	lua_pushstring(L, "ANSI_RESET"); lua_pushstring(L, ANSI_RESET);
	lua_settable(L, -3);

	// global
	lua_pushcfunction(L, bl_addstr);
	lua_setglobal(L, "print");
}

static void
bbsluaHook(lua_State *L, lua_Debug* ar)
{
	// vmsg("bbslua HOOK!");
	if (ar->event == LUA_HOOKCOUNT)
		lua_yield(L, 0);
}

static char * 
bbslua_attach(const char *fpath, int *plen)
{
    struct stat st;
	int fd = open(fpath, O_RDONLY, 0600);
	char *buf = NULL;

	*plen = 0;

	if (fd < 0) return buf;
    if (fstat(fd, &st) || ((*plen = st.st_size) < 1) || S_ISDIR(st.st_mode))
    {
		close(fd);
		return buf;
    }
	*plen = *plen +1;

    buf = mmap(NULL, *plen, PROT_READ, MAP_SHARED, fd, 0);
	close(fd);

	if (buf == NULL || buf == MAP_FAILED) 
	{
		*plen = 0;
		return  NULL;
	}

    madvise(buf, *plen, MADV_SEQUENTIAL);
	return buf;
}

static void
bbslua_detach(char *p, int len)
{
	munmap(p, len);
}

int
bbslua_detect_range(char **pbs, char **pbe)
{
	int szsig = strlen(BBSLUA_SIGNATURE),
		szeofsig = strlen(BBSLUA_EOFSIGNATURE);
	char *bs, *be, *ps, *pe;

	bs = ps = *pbs;
	be = pe = *pbe;

	// find start
	while (ps + szsig < pe)
	{
		if (strncmp(ps, BBSLUA_SIGNATURE, szsig) == 0)
			break;
		// else, skip to next line
		while (ps + szsig < pe && *ps++ != '\n');
	}

	if (!(ps + szsig < pe))
		return 0;

	*pbs = ps;
	*pbe = be;

	// find tail by SIGNATURE
	pe = ps + 1;
	while (pe + szsig < be)
	{
		if (strncmp(pe, BBSLUA_SIGNATURE, szsig) == 0)
			break;
		// else, skip to next line
		while (pe + szsig < be && *pe++ != '\n');
	}

	if (pe + szsig < be)
	{
		// found sig, end at such line.
		pe--;
		*pbe = pe;
	} else {
		// search by eof-sig
		pe = be - szeofsig-2;
		while (pe > ps)
		{
			if (pe+2 + szeofsig < be &&
					strncmp(pe+2, BBSLUA_EOFSIGNATURE, szeofsig) == 0)
				break;
			while (pe > ps && *pe-- != '\n');
		}
		if (pe > ps)
			*pbe = pe+2;
	}

	// prevent trailing zeros
	pe = *pbe;
	while (pe > ps && !*pe)
		pe--;
	*pbe = pe;

	return 1;
}

int
bbslua(const char *fpath)
{
	int r = 0;
	lua_State *L = lua_open();
	char *bs, *ps, *pe;
	int sz = 0;
	unsigned int prevmode = getutmpmode();

	// detect file
	bs = bbslua_attach(fpath, &sz);
	if (!bs)
		return 0;
	ps = bs;
	pe = ps + sz;

	if(!bbslua_detect_range(&ps, &pe))
	{
		// not detected
		bbslua_detach(bs, sz);
		return 0;
	}

	// load file
	abortBBSLua = 0;
	myluaL_openlibs(L);
	luaL_openlib(L,   "bbs", lib_bbslua, 0);
	bbsluaRegConst(L, "bbs");
	r = luaL_loadbuffer(L, ps, pe-ps, "BBS-Lua");
	
	// unmap
	bbslua_detach(bs, sz);

	if (r != 0)
	{
		const char *errmsg = lua_tostring(L, -1);
		move(b_lines-3, 0); clrtobot();
		outs("\n");
		outs(errmsg);
		vmsg("BBS-Lua ���~: �гq���@�̭ץ��{���X�C");
		lua_close(L);
		return 0;
	}

	// prompt user
	grayout(0, b_lines, GRAYOUT_DARK);
	move(b_lines-2, 0); clrtobot();
	prints("\n" ANSI_COLOR(1;33;41) "%-*s" ANSI_RESET,
			t_columns-1,
			"�Ы����N��}�l���� BBS-Lua �{���C���椤�i�H�ɫ��U Ctrl-C �j��_�C"
			);

	setutmpmode(UMODE_BBSLUA);
	vmsg(" BBS-Lua v" BBSLUA_VERSION_STR " (" __DATE__ " " __TIME__")");

	// ready for running
	clear();
	lua_sethook(L, bbsluaHook, LUA_MASKCOUNT, BLCONF_EXEC_COUNT );

	while (!abortBBSLua && (r = lua_resume(L, 0)) == LUA_YIELD)
	{
		if (input_isfull())
			drop_input();

		// refresh();

		// check if input key is system break key.
		if (peek_input(BLCONF_PEEK_TIME, BLCONF_BREAK_KEY))
		{
			drop_input();
			abortBBSLua = 1;
			break;
		}
	}

	if (r != 0)
	{
		const char *errmsg = lua_tostring(L, -1);
		move(b_lines-3, 0); clrtobot();
		outs("\n");
		outs(errmsg);
	}

	lua_close(L);

	grayout(0, b_lines, GRAYOUT_DARK);
	move(b_lines, 0); clrtoeol();
	vmsgf("BBS-Lua ���浲��%s�C", 
			abortBBSLua ? " (�ϥΪ̤��_)" : r ? " (�{�����~)" : "");
	clear();
	setutmpmode(prevmode);

	return 0;
}


// vim:ts=4:sw=4
