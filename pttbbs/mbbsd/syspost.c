/* $Id: syspost.c,v 1.6 2002/05/13 03:20:04 ptt Exp $ */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include "config.h"
#include "pttstruct.h"
#include "perm.h"
#include "common.h"
#include "proto.h"

extern char *str_permid[];
extern userec_t cuser;
extern time_t now;
void post_change_perm(int oldperm, int newperm, char *sysopid, char *userid) {
    FILE *fp;
    fileheader_t fhdr;
    char genbuf[200], reason[30];
    int i, flag=0;
    
    strcpy(genbuf, "boards/S/Security");
    stampfile(genbuf, &fhdr);
    if(!(fp = fopen(genbuf,"w")))
	return;
    
    fprintf(fp, "�@��: [�t�Φw����] �ݪO: Security\n"
	    "���D: [���w���i] �����ק��v�����i\n"
	    "�ɶ�: %s\n", ctime(&now));
    for(i = 5; i < NUMPERMS; i++) {
        if(((oldperm >> i) & 1) != ((newperm >> i) & 1)) {
            fprintf (fp, "   ����\033[1;32m%s%s%s%s\033[m���v��\n",
		     sysopid,
		     (((oldperm >> i) & 1) ? "\033[1;33m����":"\033[1;33m�}��"),
		     userid, str_permid[i]);
            flag++;
        }
    }
    
    if(flag) {
	clrtobot();
	clear();
	while(!getdata_str(5, 0, "�п�J�z�ѥH�ܭt�d�G",
			   reason, sizeof(reason), DOECHO, "�ݪ����D:"));
	fprintf(fp, "\n   \033[1;37m����%s�ק��v���z�ѬO�G%s\033[m",
		cuser.userid, reason);
	fclose(fp);
	
	sprintf(fhdr.title, "[���w���i] ����%s�ק�%s�v�����i",
		cuser.userid, userid);
	strcpy(fhdr.owner, "[�t�Φw����]");
	append_record("boards/S/Security/.DIR", &fhdr, sizeof(fhdr));
    }
}

void post_violatelaw(char* crime, char* police, char* reason, char* result){
    char genbuf[200];
    fileheader_t fhdr;
    FILE *fp;            
    strcpy(genbuf, "boards/S/Security");
    stampfile(genbuf, &fhdr);
    if(!(fp = fopen(genbuf,"w")))
        return;
    fprintf(fp, "�@��: [Ptt�k�|] �ݪO: Security\n"
	    "���D: [���i] %-20s �H�k�P�M���i\n"
	    "�ɶ�: %s\n"
	    "\033[1;32m%s\033[m�P�M�G\n     \033[1;32m%s\033[m"
	    "�]\033[1;35m%s\033[m�欰�A\n�H�ϥ������W�A�B�H\033[1;35m%s\033[m�A�S�����i",
	    crime, ctime(&now), police, crime, reason, result);
    fclose(fp);
    sprintf(fhdr.title, "[���i] %-20s �H�k�P�M���i", crime);
    strcpy(fhdr.owner, "[Ptt�k�|]");
    append_record("boards/S/Security/.DIR", &fhdr, sizeof(fhdr));
    
    strcpy(genbuf, "boards/V/ViolateLaw");
    stampfile(genbuf, &fhdr);
    if(!(fp = fopen(genbuf,"w")))
        return;
    fprintf(fp, "�@��: [Ptt�k�|] �ݪO: ViolateLaw\n"
	    "���D: [���i] %-20s �H�k�P�M���i\n"
	    "�ɶ�: %s\n"
	    "\033[1;32m%s\033[m�P�M�G\n     \033[1;32m%s\033[m"
	    "�]\033[1;35m%s\033[m�欰�A\n�H�ϥ������W�A�B�H\033[1;35m%s\033[m�A�S�����i",
	    crime, ctime(&now), police, crime, reason, result);
    fclose(fp);
    sprintf(fhdr.title, "[���i] %-20s �H�k�P�M���i", crime);
    strcpy(fhdr.owner, "[Ptt�k�|]");
    
    append_record("boards/V/ViolateLaw/.DIR", &fhdr, sizeof(fhdr));
                                 
}

void post_newboard(char* bgroup, char* bname, char* bms){
    char genbuf[256], title[128];
    sprintf(title, "[�s������] %s", bname);
    sprintf(genbuf, "%s �}�F�@�ӷs�� %s : %s\n\n�s�����D�� %s\n\n����*^_^*\n",
	     cuser.userid, bname, bgroup, bms);
    post_msg("Record", title,  genbuf, "[�t��]");  
}

