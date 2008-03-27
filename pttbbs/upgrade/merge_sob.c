/* $Id: merge.c 3650 2007-12-08 02:37:03Z piaip $ */
#define _XOPEN_SOURCE
#define _ISOC99_SOURCE
/* this is a interface provided when we merge BBS */ 
#include "bbs.h"
#include "fpg.h"

int
m_sob(void)
{
    char genbuf[256], buf[256], userid[25], passbuf[24], msg[2048]="";
    int count=0, i, isimported=0, corrected;
    FILE *fp;
    sobuserec man;
    time4_t d;

    clear();
    move(1,0);

    outs(
 "   �Ъ`�N �o�O�u�������F�y�ϥΪ�!\n"
 "      ���F�y���ϥΪ��ಾ�ӤH�겣�H�έ��n�H�θ��, �ɦ������w��������.\n"
 "      �p�G�z���ݭn, �Ъ����}.\n"
 "    -----------------------------------------------------------------\n"
 "    �S�O�m�{:\n"
 "      ���F�b���w��,�z�u���s��Q���K�X���~�����|,�Фp�߿�J.\n"
 "      �s�򦸿��~�z���ܨ��\\��N�|�Q�}�@��ê����q������.\n"
 "      �Ф��n�b�ܨ��L�{�������`�_�u, ��N�_�u�ܥb�~�H�������ϭ�.\n"
	);

   if(getkey("�O�_�n�~��?(y/N)")!='y') return 0;
   if(search_ulistn(usernum,2)) 
        {vmsg("�еn�X��L����, �H�K�ܨ�����"); return 0;}
   do
   {
    if(!getdata(10,0, "      �F�y��ID [�j�p�g�n�������T]:", userid, 20,
	       DOECHO)) return 0;
    if(bad_user_id(userid)) continue;
    sprintf(genbuf, "sob/passwd/%c/%s.inf",userid[0], userid);
    if(!(fp=fopen(genbuf, "r"))) 
       {
        isimported = 1;
        strcat(genbuf, ".done");
        if(!(fp=fopen(genbuf, "r")))
         {
           vmsg("�d�L���H�Τw�g�פJ�L..�Ъ`�N�j�p�g ");
           isimported = 0;
           continue;
         }
       }
    count = fread(&man, sizeof(man), 1, fp);
    fclose(fp);
   }while(!count);
   count = 0;
   do{
    if(!getdata(11,0, "      �F�y���K�X:", passbuf, sizeof(passbuf), 
		  NOECHO)) return 0;
    if(++count>=10)
    {
          cuser.userlevel |= PERM_VIOLATELAW;
          cuser.vl_count++;
	  passwd_update(usernum, &cuser);
          post_violatelaw(cuser.userid, "[PTTĵ��]", "���ձb�����~�Q��",
		          "�H�k�[��");
          mail_violatelaw(cuser.userid, "[PTTĵ��]", "���ձb�����~�Q��",
		          "�H�k�[��");

          return 0;
    }
    if(!(corrected = checkpasswd(man.passwd, passbuf)))
       vmsg("�K�X���~"); 
   } while(!corrected);
   move(12,0);
   clrtobot();

   if(!isimported)
     {
       if(!dashf(genbuf))  // avoid multi-login
       {
         vmsg("�Ф��n���զh��id��פJ");
         return 0;
       }
       sprintf(buf,"%s.done",genbuf);
       rename(genbuf,buf);
#ifdef MERGEMONEY

   reload_money(); 

   sprintf(buf, 
           "�z���F�y�x�M�� %10d ���⦨ " MONEYNAME " ���� %9d (�ײv 22:1), \n"
           "    �F�y���ߦ� %10d ���⬰ " MONEYNAME " ���� %9d (�ײv 222105:1), \n"
           "    �즳 %10d �פJ��@�� %d\n",
	   (int)man.goldmoney, (int)man.goldmoney/22, 
	   (int)man.silvermoney, (int)man.silvermoney/222105,
	   cuser.money,
	   (int)(cuser.money + man.goldmoney/22 + man.silvermoney/222105));
   demoney(man.goldmoney/22 + man.silvermoney/222105 );
   strcat(msg, buf); 
#endif

     i =  cuser.exmailbox + man.exmailbox + man.exmailboxk/2000;
     if (i > MAX_EXKEEPMAIL) i = MAX_EXKEEPMAIL;
     sprintf(buf, "�z���F�y�H�c�� %d (%dk), �즳 %d �פJ��@�� %d\n", 
	    man.exmailbox, man.exmailboxk, cuser.exmailbox, i);
     strcat(msg, buf);
     cuser.exmailbox = i;

     if(man.userlevel & PERM_MAILLIMIT)
      {
       sprintf(buf, "�}�ҫH�c�L�W��\n");
       strcat(msg, buf);
       cuser.userlevel |= PERM_MAILLIMIT;
      }

     if (cuser.firstlogin > man.firstlogin)
	 d = man.firstlogin;
     else
	 d = cuser.firstlogin;
     cuser.firstlogin = d;

     if (cuser.numlogins < man.numlogins)
	 i = man.numlogins;
     else
	 i = cuser.numlogins;

     sprintf(buf, "�F�y�i������ %d ���b�� %d �N�� %d \n", man.numlogins,
	   cuser.numlogins, i);
     strcat(msg,buf);
     cuser.numlogins = i;

     if (cuser.numposts < man.numposts )
	 i = man.numposts;
     else
	 i = cuser.numposts;
     sprintf(buf, "�F�y�峹���� %d ���b�� %d �N�� %d\n", 
                 man.numposts,cuser.numposts,i);
     strcat(msg,buf);
     cuser.numposts = i;
     outs(msg);
     while (search_ulistn(usernum,2)) 
        {vmsg("�бN���ФW����L�u����! �A�~��");}
     passwd_update(usernum, &cuser);
   }
   sethomeman(genbuf, cuser.userid);
   mkdir(genbuf, 0600);
   sprintf(buf, "tar zxvf %c/%s.tar.gz>/dev/null",
	   userid[0], userid);
   chdir("sob/home");
   system(buf);
   chdir(BBSHOME);

   if (getans("�O�_�פJ�ӤH�H�c? (Y/n)")!='n')
   {
	sethomedir(buf, cuser.userid);
	sprintf(genbuf, "sob/home/%c/%s/.DIR",
		userid[0], userid);
	merge_dir(buf, genbuf, 1);
        strcat(msg, "�פJ�ӤH�H�c\n");
   }
   if(getans("�O�_�פJ�ӤH�H�c��ذ�(�ӤH�@�~��)? (�|�л\\�{���]�w) (y/N)")=='y')
   {
        fileheader_t fh;
        sprintf(buf,
	 "rm -rd home/%c/%s/man>/dev/null ; "
         "mv sob/home/%c/%s/man home/%c/%s>/dev/null;"
         "mv sob/home/%c/%s/gem home/%c/%s/man>/dev/null", 
              cuser.userid[0], cuser.userid,
	      userid[0], userid,
	      cuser.userid[0], cuser.userid,
	      userid[0], userid,
	      cuser.userid[0], cuser.userid);
        system(buf);
        strcat(msg, "�פJ�ӤH�H�c��ذ�(�ӤH�@�~��)\n");
        sprintf(buf,"home/%c/%s/man/gem", cuser.userid[0], cuser.userid);
        if(dashd(buf))
         {
          strcat(fh.title, "�� �ӤH�@�~��");
          strcat(fh.filename, "gem");
          sprintf(fh.owner, cuser.userid);
          sprintf(buf, "home/%c/%s/man/.DIR", cuser.userid[0], cuser.userid);
          append_record(buf, &fh, sizeof(fh));
         }
   }
   if(getans("�O�_�פJ�n�ͦW��? (�|�л\\�{���]�w, ID�i��O���P�H)? (y/N)")=='y')
   {
       sethomefile(genbuf, cuser.userid, "overrides");
       sprintf(buf, "sob/home/%c/%s/overrides",userid[0],userid);
       Copy(buf, genbuf);
       strcat(buf, genbuf);
       friend_load(FRIEND_OVERRIDE);
       strcat(msg, "�פJ�n�ͦW��\n");
   }
   sprintf(buf, "�b���פJ���i %s -> %s ", userid, cuser.userid);
   post_msg(GLOBAL_SECURITY, buf, msg, "[�t�Φw����]");

   vmsg("���߱z�����b���ܨ�..");
   return 0;
}

void
m_sob_brd(char *bname, char *fromdir)
{
  char fbname[25], buf[256];
  fileheader_t fh;

  fromdir[0]=0;
  do{

     if(!getdata(20,0, "SOB���O�W [�^��j�p�g�n�������T]:", fbname, 20,
	        DOECHO)) return;
  }
  while((invalid_brdname(fbname)&1));

  sprintf(buf, "sob/man/%s.tar.gz", fbname);
  if(!dashf(buf))
  {
       vmsg("�L���ݪO");
       return;
  }
  chdir(BBSHOME"/sob/boards");
  sprintf(buf, "tar zxf %s.tar.gz >/dev/null",fbname);
  system(buf);
  chdir(BBSHOME"/sob/man");
  sprintf(buf, "tar zxf %s.tar.gz >/dev/null", fbname);
  system(buf);
  chdir(BBSHOME);
  sprintf(buf, "mv sob/man/%s man/boards/%c/%s", fbname,
	    bname[0], bname);
  system(buf);
  sprintf(fh.title, "�� %s ��ذ�", fbname);
  sprintf(fh.filename, fbname);
  sprintf(fh.owner, cuser.userid);
  sprintf(buf, "man/boards/%c/%s/.DIR", bname[0], bname);
  append_record(buf, &fh, sizeof(fh));
  sprintf(fromdir, "sob/boards/%s/.DIR", fbname);
  vmsgf("�Y�N�פJ %s �O���..�����ݭn�@�I�ɶ�",fbname);
}
