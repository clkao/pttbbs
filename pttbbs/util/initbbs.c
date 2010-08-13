/* $Id$ */
#include "bbs.h"

static void initDir() {
    mkdir("adm", 0755);
    mkdir("boards", 0755);
    mkdir("etc", 0755);
    mkdir("man", 0755);
    mkdir("man/boards", 0755);
    mkdir("out", 0755);
    mkdir("tmp", 0755);
    mkdir("run", 0755);
    mkdir("jobspool", 0755);
}

static void initHome() {
    int i;
    char buf[256];
    
    mkdir("home", 0755);
    strcpy(buf, "home/?");
    for(i = 0; i < 26; i++) {
	buf[5] = 'A' + i;
	mkdir(buf, 0755);
	buf[5] = 'a' + i;
	mkdir(buf, 0755);
#if 0
	/* in current implementation we don't allow 
	 * id as digits so we don't create now. */
	if(i >= 10)
	    continue;
	/* 0~9 */
	buf[5] = '0' + i;
	mkdir(buf, 0755);
#endif
    }
}

static void initBoardsDIR() {
    int i;
    char buf[256];
    
    mkdir("boards", 0755);
    strcpy(buf, "boards/?");
    for(i = 0; i < 26; i++) {
	buf[7] = 'A' + i;
	mkdir(buf, 0755);
	buf[7] = 'a' + i;
	mkdir(buf, 0755);
	if(i >= 10)
	    continue;
	/* 0~9 */
	buf[7] = '0' + i;
	mkdir(buf, 0755);
    }
}

static void initManDIR() {
    int i;
    char buf[256];
    
    mkdir("man", 0755);
    mkdir("man/boards", 0755);
    strcpy(buf, "man/boards/?");
    for(i = 0; i < 26; i++) {
	buf[11] = 'A' + i;
	mkdir(buf, 0755);
	buf[11] = 'a' + i;
	mkdir(buf, 0755);
	if(i >= 10)
	    continue;
	/* 0~9 */
	buf[11] = '0' + i;
	mkdir(buf, 0755);
    }
}

static void initPasswds() {
    int i;
    userec_t u;
    FILE *fp = fopen(".PASSWDS", "w");
    
    memset(&u, 0, sizeof(u));
    if(fp) {
	for(i = 0; i < MAX_USERS; i++)
	    fwrite(&u, sizeof(u), 1, fp);
	fclose(fp);
    }
}

static void newboard(FILE *fp, boardheader_t *b) {
    char buf[256];
    
    fwrite(b, sizeof(boardheader_t), 1, fp);
    sprintf(buf, "boards/%c/%s", b->brdname[0], b->brdname);
    mkdir(buf, 0755);
    sprintf(buf, "man/boards/%c/%s", b->brdname[0], b->brdname);
    mkdir(buf, 0755);
}

static void initBoards() {
    FILE *fp = fopen(".BRD", "w");
    boardheader_t b;
    
    if(fp) {
	memset(&b, 0, sizeof(b));
	
	strcpy(b.brdname, "SYSOP");
	strcpy(b.title, "�T�� �������n!");
	b.brdattr = BRD_POSTMASK | BRD_NOTRAN;
	b.level = 0;
	b.gid = 2;
	newboard(fp, &b);

	strcpy(b.brdname, "1...........");
	strcpy(b.title, ".... �U�����F��  �m�����M�I,�D�H�i�ġn");
	b.brdattr = BRD_GROUPBOARD;
	b.level = PERM_SYSOP;
	b.gid = 1;
	newboard(fp, &b);
	
	strcpy(b.brdname, "junk");
	strcpy(b.title, "�o�q �����C���K���U��");
	b.brdattr = BRD_NOTRAN;
	b.level = PERM_SYSOP;
	b.gid = 2;
	newboard(fp, &b);
	
	strcpy(b.brdname, "Security");
	strcpy(b.title, "�o�q �������t�Φw��");
	b.brdattr = BRD_NOTRAN;
	b.level = PERM_SYSOP;
	b.gid = 2;
	newboard(fp, &b);
	
	strcpy(b.brdname, "2...........");
	strcpy(b.title, ".... �U�����s��     ���i  ����  ���I");
	b.brdattr = BRD_GROUPBOARD;
	b.level = 0;
	b.gid = 1;
	newboard(fp, &b);
	
	strcpy(b.brdname, BN_ALLPOST);
	strcpy(b.title, "�T�� ����O��LOCAL�s�峹");
	b.brdattr = BRD_POSTMASK | BRD_NOTRAN;
	b.level = PERM_SYSOP;
	b.gid = 5;
	newboard(fp, &b);
	
	strcpy(b.brdname, "deleted");
	strcpy(b.title, "�T�� ���귽�^����");
	b.brdattr = BRD_NOTRAN;
	b.level = PERM_BM;
	b.gid = 5;
	newboard(fp, &b);
	
	strcpy(b.brdname, "Note");
	strcpy(b.title, "�T�� ���ʺA�ݪO�κq����Z");
	b.brdattr = BRD_NOTRAN;
	b.level = 0;
	b.gid = 5;
	newboard(fp, &b);
	
	strcpy(b.brdname, "Record");
	strcpy(b.title, "�T�� ���ڭ̪����G");
	b.brdattr = BRD_NOTRAN | BRD_POSTMASK;
	b.level = 0;
	b.gid = 5;
	newboard(fp, &b);
	
	
	strcpy(b.brdname, "WhoAmI");
	strcpy(b.title, "�T�� �������A�q�q�ڬO�֡I");
	b.brdattr = BRD_NOTRAN;
	b.level = 0;
	b.gid = 5;
	newboard(fp, &b);
	
	strcpy(b.brdname, "EditExp");
	strcpy(b.title, "�T�� ���d�����F��Z��");
	b.brdattr = BRD_NOTRAN;
	b.level = 0;
	b.gid = 5;
	newboard(fp, &b);

	strcpy(b.brdname, "ALLHIDPOST");
	strcpy(b.title, "�T�� ����O��LOCAL�s�峹(���O)");
	b.brdattr = BRD_POSTMASK | BRD_HIDE;
	b.level = PERM_SYSOP;
	b.gid = 5;
	newboard(fp, &b);
	
#ifdef BN_DIGEST
	strcpy(b.brdname, BN_DIGEST);
	strcpy(b.title, "��K ��" BBSNAME "��K �n�媺�����a");
	b.brdattr = BRD_NOTRAN | BRD_POSTMASK;
	b.level = PERM_SYSOP;
	b.gid = 5;
	newboard(fp, &b);
#endif

#ifdef BN_FIVECHESS_LOG
	strcpy(b.brdname, BN_FIVECHESS_LOG);
	strcpy(b.title, "���� ��" BBSNAME "���l���� ���W�什������");
	b.brdattr = BRD_NOTRAN | BRD_POSTMASK;
	b.level = PERM_SYSOP;
	b.gid = 5;
	newboard(fp, &b);
#endif

	fclose(fp);
    }
}

static void initMan() {
    FILE *fp;
    fileheader_t f;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    
    memset(&f, 0, sizeof(f));
    strcpy(f.owner, "SYSOP");
    sprintf(f.date, "%2d/%02d", tm->tm_mon + 1, tm->tm_mday);
    f.multi.money = 0;
    f.filemode = 0;
    
    if((fp = fopen("man/boards/N/Note/.DIR", "w"))) {
	strcpy(f.filename, "SONGBOOK");
	strcpy(f.title, "�� �i�I �q �q ���j");
	fwrite(&f, sizeof(f), 1, fp);
	mkdir("man/boards/N/Note/SONGBOOK", 0755);
	
	strcpy(f.filename, "SONGO");
	strcpy(f.title, "�� <�I�q> �ʺA�ݪO");
	fwrite(&f, sizeof(f), 1, fp);
	mkdir("man/boards/N/Note/SONGO", 0755);
	
	strcpy(f.filename, "SYS");
	strcpy(f.title, "�� <�t��> �ʺA�ݪO");
	fwrite(&f, sizeof(f), 1, fp);
	mkdir("man/boards/N/Note/SYS", 0755);
	
	strcpy(f.filename, "AD");
	strcpy(f.title, "�� <�s�i> �ʺA�ݪO");
	fwrite(&f, sizeof(f), 1, fp);
	mkdir("man/boards/N/Note/AD", 0755);
	
	strcpy(f.filename, "NEWS");
	strcpy(f.title, "�� <�s�D> �ʺA�ݪO");
	fwrite(&f, sizeof(f), 1, fp);
	mkdir("man/boards/N/Note/NEWS", 0755);
	
	fclose(fp);
    }
    
}

static void initSymLink() {
    symlink(BBSHOME "/man/boards/N/Note/SONGBOOK", BBSHOME "/etc/SONGBOOK");
    symlink(BBSHOME "/man/boards/N/Note/SONGO", BBSHOME "/etc/SONGO");
    symlink(BBSHOME "/man/boards/E/EditExp", BBSHOME "/etc/editexp");
}

static void initHistory() {
    FILE *fp = fopen("etc/history.data", "w");
    
    if(fp) {
	fprintf(fp, "0 0 0 0");
	fclose(fp);
    }
}

int main(int argc, char **argv)
{
    if( argc != 2 || strcmp(argv[1], "-DoIt") != 0 ){
	fprintf(stderr,
		"ĵ�i!  initbbs�u�Φb�u�Ĥ@���w�ˡv���ɭ�.\n"
		"�Y�z�����x�w�g�W�u,  initbbs�N�|�}�a���즳���!\n\n"
		"�N�� BBS �w�˦b " BBSHOME "\n\n"
		"�T�w�n����, �Шϥ� initbbs -DoIt\n");
	return 1;
    }

    if(chdir(BBSHOME)) {
	perror(BBSHOME);
	exit(1);
    }
    
    initDir();
    initHome();
    initBoardsDIR();
    initManDIR();
    initPasswds();
    initBoards();
    initMan();
    initSymLink();
    initHistory();
    
    return 0;
}
