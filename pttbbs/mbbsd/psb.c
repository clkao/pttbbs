/* $Id$ */
#include "bbs.h"

// Panty & Stocking Browser
//
// A generic framework for displaying pre-generated data by a simplified
// page-view user interface.
//
// Author: Hung-Te Lin (piaip)
// --------------------------------------------------------------------------
// Copyright (c) 2010 Hung-Te Lin <piaip@csie.ntu.edu.tw>
// All rights reserved.
// Distributed under BSD license (GPL compatible).
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
// --------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////
// Constant
#define PSB_EOF (-1)
#define PSB_NA  (-2)
#define PSB_NOP (-3)

///////////////////////////////////////////////////////////////////////////
// Data Structure
typedef struct {
    int curr, total, header_lines, footer_lines;
    int key;
    void *ctx;
    int (*header)(void *ctx); 
    int (*footer)(void *ctx); 
    int (*renderer)(int i, int curr, int total, int rows, void *ctx);
    int (*cursor)(int y, int curr, void *ctx);
    int (*input_processor)(int key, int curr, int total, int rows, void *ctx);
} PSB_CTX;

static int
psb_default_header(void *ctx) {
    vs_hdr2bar("Panty & Stocking Browser", BBSNAME);
    return 0;
}

static int
psb_default_footer(void *ctx) {
    vs_footer(" PSB 1.0 ",
              " (��/��/PgUp/PgDn/0-9)Move (Enter/��)Select \t(q/��)Quit");
    return 0;
}

static int
psb_default_renderer(int i, int curr, int total, int rows, void *ctx) {
    prints("   %s(Demo) %5d / %5d Item\n", (i == curr) ? "*" : " ", i, total);
    return 0;
}

static int
psb_default_cursor(int y, int curr, void * ctx) {
    outs("=>");
    // cursor_show(y, 0);
    return 0;
}

static int
psb_default_input_processor(int key, int curr, int total, int rows, void *ctx) {
    switch(key) {
        case 'q':
        case KEY_LEFT:
            return PSB_EOF;

        case KEY_HOME:
        case '0':
            return 0;

        case KEY_END:  
        case '$':
            return total-1;

        case KEY_PGUP:
        case Ctrl('B'):
        case 'N':
            if (curr / rows > 0)
                return curr - rows;
            return 0;

        case KEY_PGDN:
        case Ctrl('F'):
        case 'P':
            if (curr + rows < total)
                return curr + rows;
            return total - 1;

        case KEY_UP: 
        case Ctrl('P'):
        case 'p':
        case 'k':
            return (curr > 0) ? curr-1 : curr;

        case KEY_DOWN:
        case Ctrl('N'):
        case 'n':
        case 'j':
            return (curr + 1 < total) ? curr + 1 : curr;

        default:
            if (key >= '0' && key <= '9') {
                int newval = search_num(key, total);
                if (newval >= 0 && newval < total)
                    return newval;
                return curr;
            }
            break;
    }
    return  PSB_NA;
}

static void
psb_init_defaults(PSB_CTX *psbctx) {
    // pre-setup
    assert(psbctx);
    if (!psbctx->header)
        psbctx->header = psb_default_header;
    if (!psbctx->footer)
        psbctx->footer = psb_default_footer;
    if (!psbctx->renderer)
        psbctx->renderer = psb_default_renderer;
    if (!psbctx->cursor)
        psbctx->cursor = psb_default_cursor;
    if (!psbctx->footer)
        psbctx->footer = psb_default_footer;

    assert(psbctx->curr >= 0 &&
           psbctx->total >= 0 &&
           psbctx->curr < psbctx->total);
    assert(psbctx->header_lines > 0 &&
           psbctx->footer_lines);
}

int
psb_main(PSB_CTX *psbctx)
{
    psb_init_defaults(psbctx);

    while (1) {
        int i;
        int rows = t_lines - psbctx->header_lines - psbctx->footer_lines;
        int base;

        assert(rows > 0);
        base = psbctx->curr / rows * rows;
        clear();
        SOLVE_ANSI_CACHE();
        psbctx->header(psbctx->ctx);
        for (i = 0; i < rows; i++) {
            move(psbctx->header_lines + i, 0);
            SOLVE_ANSI_CACHE();
            if (base + i < psbctx->total)
                psbctx->renderer(base + i, psbctx->curr, psbctx->total, 
                                 rows, psbctx->ctx);
        }
        move(t_lines - psbctx->footer_lines, 0);
        SOLVE_ANSI_CACHE();
        psbctx->footer(psbctx->ctx);
        i = psbctx->header_lines + psbctx->curr - base;
        move(i, 0);
        psbctx->cursor(i, psbctx->curr, psbctx->ctx);
        psbctx->key = vkey();

        i = PSB_NA;
        if (psbctx->input_processor)
            i = psbctx->input_processor(psbctx->key, psbctx->curr,
                                        psbctx->total, rows, psbctx->ctx);
        if (i == PSB_NA)
            i = psb_default_input_processor(psbctx->key, psbctx->curr,
                                            psbctx->total, rows, psbctx->ctx);
        if (i == PSB_EOF)
            break;
        if (i == PSB_NOP)
            continue;

        if (i >=0 && i < psbctx->total)
            psbctx->curr = i;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////
// View Edit History

typedef struct {
    const char *subject;
    const char *filebase;
} pveh_ctx;

static int
pveh_header(void *ctx) {
    pveh_ctx *cx = (pveh_ctx*) ctx;
    vs_hdr2barf(" �i�˵��峹�s����v�j \t %s", cx->subject);
    move(1, 0);
    outs("�Ъ`�N���t�Τ��|�ä[�O�d�Ҧ����s����v�C");
    outs("\n");
    return 0;
}

static int
pveh_footer(void *ctx) {
    vs_footer(" �s����v ",
              " (��/��/PgUp/PgDn/0-9)���� (Enter/r/��)��� \t(q/��)���X");
    return 0;
}

static int
pveh_cursor(int y, int curr, void *ctx) {
    // (y, 0) before drawing
#ifdef USE_PFTERM
    outs("��\b");
#else
    cursor_show(y, 0);
#endif
    return 0;
}

static int
pveh_renderer(int i, int curr, int total, int rows, void *ctx) {
    const char *subject = "";
    char fname[PATHLEN];
    time4_t ftime = 0;
    pveh_ctx *cx = (pveh_ctx*) ctx;

    snprintf(fname, sizeof(fname), "%s.%03d", cx->filebase, i);
    ftime = dasht(fname);
    if (ftime != -1)
        subject = Cdate(&ftime);
    else
        subject = "(�O���w�L�O�d����/�w�M��)";

    prints("   %s%s ����: #%08d    ",
           (i == curr) ? ANSI_COLOR(1;41;37) : "",
           (ftime == -1) ? ANSI_COLOR(1;30) : "",
           i + 1);
    prints(" �ɶ�: %-47s" ANSI_RESET "\n", subject);
    return 0;
}

static int
pveh_input_processor(int key, int curr, int total, int rows, void *ctx) {
    char fname[PATHLEN];
    pveh_ctx *cx = (pveh_ctx*) ctx;

    switch (key) {
        case KEY_ENTER:
        case KEY_RIGHT:
        case 'r':
            snprintf(fname, sizeof(fname), "%s.%03d", cx->filebase, curr);
            more(fname, YEA);
            return PSB_NOP;
    }
    return PSB_NA;
}

int
psb_view_edit_history(const char *base, const char *subject, int max_hist) {
    pveh_ctx pvehctx = {
        .subject = subject,
        .filebase = base,
    };
    PSB_CTX ctx = {
        .curr = 0,
        .total = max_hist,
        .header_lines = 3,
        .footer_lines = 2,
        .ctx = (void*)&pvehctx,
        .header = pveh_header,
        .footer = pveh_footer,
        .renderer = pveh_renderer,
        .cursor = pveh_cursor,
        .input_processor = pveh_input_processor,
    };

    // warning screen!
    static char is_first_enter_pveh = 1;
    if (is_first_enter_pveh) {
        is_first_enter_pveh = 0;
        clear();
        move(2, 0);
        outs(
"  �w��ϥΤ峹�s����v�s���t��!\n\n"
"  �����z: (1) ���t�Ω|�b����ʶ}�񤤡A���襼�ӷ|�M�w�O�_�~�򴣨Ѧ��\\��C\n\n"
"          (2) �Ҧ�����ƶȨѰѦҡA���褣�O�Ҧ��B�����㪺�q�ϰO���C\n\n"
"          (3) �Ҧ�����Ƴ��i�ण�w���Ѩt�βM�����C\n"
"              �L�s����v����N��S���s��L�A�]�i��O�Q�M���F\n\n"
"   Mini FAQ:\n\n"
"   Q: ��ˤ~�|�����v�O�� (�W�[������)?\n"
"   A: �b�t�Χ�s��C���ϥ� E �s��峹�æs�ɴN�|���O���C���夣�|�W�[�O��������\n\n"
"   Q: �q�`���v�|�O�d�h�[?\n"
"   A: ���b�������A�γ\\�|�O�@�g��@�Ӥ�H�W\n\n"
"   Q: �ɮ׳Q�R�F�]�i�H�ݾ��v��?\n"
"   A: �����٦b�ݪO�W�N�i�H(������<����w�Q�R��>��)�A���b deleted �ݪO�O�L�Ī�\n"
            );
        pressanykey();
    }
    

    psb_main(&ctx);
    return 0;
}

///////////////////////////////////////////////////////////////////////////
// Admin Edit

// still ���i...
#define MAX_PAE_ENTRIES (256)

typedef struct {
    char *descs[MAX_PAE_ENTRIES];
    char *files[MAX_PAE_ENTRIES];
} pae_ctx;

static int
pae_header(void *ctx) {
    vs_hdr2bar(" �i�t���ɮסj ", "  �s��t���ɮ�");
    outs("�п���n�s�誺�ɮ׫�� Enter �}�l�ק�\n");
    vbarf(ANSI_REVERSE
         "%5s %-36s%-30s", "�s��", "�W  ��", "��  �W");
    return 0;
}

static int
pae_footer(void *ctx) {
    vs_footer(" �s��t���ɮ� ",
              " (����/0-9)���� (Enter/e/r/��)�s�� (DEL/d)�R�� \t(q/��)���X");
    return 0;
}

static int
pae_renderer(int i, int curr, int total, int rows, void *ctx) {
    pae_ctx *cx = (pae_ctx*) ctx;
    prints("  %3d %s%s%-36.36s " ANSI_COLOR(1;37) "%-30.30s" ANSI_RESET "\n", 
            i+1,
            (i == curr) ? ANSI_COLOR(41) : "",
            dashf(cx->files[i]) ? ANSI_COLOR(1;36) : ANSI_COLOR(1;30),
            cx->descs[i], cx->files[i]);
    return 0;
}

static int
pae_cursor(int y, int curr, void *ctx) {
    cursor_show(y, 0);
    return 0;
}

static int
pae_input_processor(int key, int curr, int total, int rows, void *ctx) {
    int result;
    pae_ctx *cx = (pae_ctx*) ctx;

    switch(key) {
        case KEY_DEL:
        case 'd':
            if (vansf("�T�w�n�R�� %s �ܡH (y/N) ", cx->descs[curr]) == 'y')
                unlink(cx->files[curr]);
            vmsgf("�t���ɮ�[%s]: %s", cx->files[curr],
                  !dashf(cx->files[curr]) ?  "�R�����\\ " : "���R��");
            return PSB_NOP;

        case KEY_ENTER:
        case KEY_RIGHT:
        case 'r':
        case 'e':
        case 'E':
            result = veditfile(cx->files[curr]);
            // log file change
            if (result != EDIT_ABORTED)
            {
                log_filef("log/etc_edit.log",
                          LOG_CREAT,
                          "%s %s %s # %s\n",
                          Cdate(&now),
                          cuser.userid,
                          cx->files[curr],
                          cx->descs[curr]);
            }
            vmsgf("�t���ɮ�[%s]: %s",
                  cx->files[curr],
                  (result == EDIT_ABORTED) ?  "������" : "��s����");
            return PSB_NOP;
    }
    return PSB_NA;
}

int
psb_admin_edit() {
    int i;
    char buf[PATHLEN*2];
    FILE *fp;
    pae_ctx paectx = { {0}, };
    PSB_CTX ctx = {
        .curr = 0,
        .total = 0,
        .header_lines = 4,
        .footer_lines = 2,
        .ctx = (void*)&paectx,
        .header = pae_header,
        .footer = pae_footer,
        .renderer = pae_renderer,
        .cursor = pae_cursor,
        .input_processor = pae_input_processor,
    };

    fp = fopen(FN_CONF_EDITABLE, "rt");
    if (!fp) {
	// you can find a sample in sample/etc/editable
	vmsgf("���]�w�i�s���ɮצC��[%s]�A�Ь��t�ί����C", FN_CONF_EDITABLE);
	return 0;
    }

    // load the editable file.
    // format: filename [ \t]* description
    while (ctx.total < MAX_PAE_ENTRIES &&
           fgets(buf, sizeof(buf), fp)) {
        char *k = buf, *v = buf;
        if (!*buf || strchr("#./ \t\n\r", *buf))
            continue;

        // change \t to ' '.
        while (*v) if (*v++ == '\t') *(v-1) = ' ';
        v = strchr(buf, ' ');
        if (v == NULL)
            continue;

        // see if someone is trying to crack
        k = strstr(buf, "..");
        if (k && k < v)
            continue;

	// reject anything outside etc/ folder.
        if (strncmp(buf, "etc/", strlen("etc/")) != 0)
            continue;

        // adjust spaces
        chomp(buf);
        k = buf; *v++ = 0;
        while (*v == ' ') v++;
        trim(k);
        trim(v);

        // add into context
        paectx.files[ctx.total] = strdup(k);
        paectx.descs[ctx.total] = strdup(v);
        ctx.total++;
    }

    psb_main(&ctx);

    for (i = 0; i < ctx.total; i++) {
        free(paectx.files[i]);
        free(paectx.descs[i]);
    }
    return 0;
}

