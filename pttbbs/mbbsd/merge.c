/* $Id: merge.c 2060 2004-06-11 17:18:06Z Ptt $ */
#define _XOPEN_SOURCE
#define _ISOC99_SOURCE
/* this is a interface provided when we merge BBS */ 
#include "bbs.h"
#include "fpg.h"

int
m_fpg()
{
    char genbuf[256], buf[256], userid[25], passbuf[24], msg[2048]="";
    int count=0, i, isimported=0;
    FILE *fp;
    ACCT man;
    time_t d;

    clear();
    move(1,0);

    outs(
 "    �p����������,\n"
 "      ����骺�ϥΪ��ಾ�ӤH�겣�H�έ��n�H�θ��, �ɦ������w��������.\n"
 "      �p�G�z���ݭn, �Ъ�����[Enter]���}.\n"
 "    -----------------------------------------------------------------\n"
 "    �S�O�m�{:\n"
 "      ���F�b���w��,�z�u���s��T���K�X���~�����|,�Фp�߿�J.\n"
 "      �s��T�����~�z���ܨ��\\��N�|�Q�}�@��ê����q������.\n"
 "      �Ф��n�b�ܨ��L�{�������`�_�u, ��N�_�u�ܥb�~�H�������ϭ�.\n"
	);


   if(search_ulistn(usernum,2)) 
        {vmsg("�еn�X��L����, �H�K�ܨ�����"); return 0;}
   do
   {
    if(!getdata(10,0, "      �p����ID [�^��j�p�g�n�������T]:", userid, 20,
	       DOECHO)) return 0;
    if(bad_user_id(userid)) continue;
    sprintf(genbuf, "/home/bbs/fpg/home/%c/%s.ACT",userid[0], userid);
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
    getdata(11,0, "      �p�����K�X:", passbuf, sizeof(passbuf), 
		  NOECHO);
    if(++count>=3)
    {
          cuser.userlevel |= PERM_VIOLATELAW;
          cuser.vl_count++;
	  passwd_update(usernum, &cuser);
          post_violatelaw(cuser.userid, "[PTTĵ��]", "���դp���b�����~�T��",
		          "�H�k�[��");
          mail_violatelaw(cuser.userid, "[PTTĵ��]", "���դp���b�����~�T��",
		          "�H�k�[��");

          return 0;
    }
   } while(!checkpasswd(man.passwd, passbuf));
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
    int price[10] = {74, 21, 29, 48, 67, 11, 9, 43, 57, 72};
    unsigned lmarket=0;

   reload_money(); 

   for(i=0; i<10; i++)
     lmarket += man.market[i]/(674 / price[i]);
   sprintf(buf, 
           "�z�������� %10d ���⦨ Ptt ���� %9d (�u�f�ײv 155:1), \n"
           "    �Ȧ榳   %10d ���⬰ Ptt ���� %9d (�ײv�� 674:1), \n"
           "    �ᥫ���� %10d ���⬰ Ptt ���� %9d (�ײv�� 674:1), \n"
           "    �즳P��  %10d �פJ��@�� %d\n",
            man.money, man.money/155, 
            man.bank, man.bank/674,
            lmarket*674, lmarket,
            cuser.money, cuser.money + man.money/155 + man.bank/674 + lmarket);
   demoney(man.money/155 + man.bank/674 + lmarket);
   strcat(msg, buf); 
#endif

     i =  cuser.exmailbox + man.mailk + man.keepmail;
     if (i > 1000) i = 1000;
     sprintf(buf, "�z�����H�c�� %d (%dk), �즳 %d �פJ��@�� %d\n", 
	    man.keepmail, man.mailk, cuser.exmailbox, i);
     strcat(msg, buf);
     cuser.exmailbox = i;

     if(man.userlevel & PERM_MAILLIMIT)
      {
       sprintf(buf, "�}�ҫH�c�L�W��\n");
       strcat(msg, buf);
       cuser.userlevel |= PERM_MAILLIMIT;
      }

     if(cuser.firstlogin > man.firstlogin) d = man.firstlogin;
     else  d = cuser.firstlogin;
     sprintf(buf, "�����U��� %s ", Cdatedate(&(man.firstlogin)));
     strcat(msg,buf);
     sprintf(buf, "���b�����U��� %s �N�� ",Cdatedate(&(cuser.firstlogin)));
     strcat(msg,buf);
     sprintf(buf, "�N�� %s\n", Cdatedate(&d) );
     strcat(msg,buf);
     cuser.firstlogin = d;

     if(cuser.numlogins < man.numlogins) i = man.numlogins;
     else i = cuser.numlogins;

     sprintf(buf, "���i������ %d ���b�� %d �N�� %d \n", man.numlogins,
	   cuser.numlogins, i);
     strcat(msg,buf);
     cuser.numlogins = i;

     if(cuser.numposts < man.numposts ) i = man.numposts;
     else i = cuser.numposts;
     sprintf(buf, "���峹���� %d ���b�� %d �N�� %d\n", 
                 man.numposts,cuser.numposts,i);
     strcat(msg,buf);
     cuser.numposts = i;
     outs(msg);
     while(search_ulistn(usernum,2)) 
        {vmsg("�бN���ФW����L�u����! �A�~��");}
     passwd_update(usernum, &cuser);
   }
   sethomeman(genbuf, cuser.userid);
   mkdir(genbuf, 0600);
   sprintf(buf, "tar zxvf home/%c/%s.tgz>/dev/null",
	   userid[0], userid);
   chdir("fpg");
   system(buf);
   chdir(BBSHOME);

   if (getans("�O�_�פJ�ӤH�H�c? (Y/n)")!='n')
    {
	sethomedir(buf, cuser.userid);
	sprintf(genbuf, "fpg/home/bbs/home/%c/%s/.DIR",
		userid[0], userid);
	merge_dir(buf, genbuf, 1);
        strcat(msg, "�פJ�ӤH�H�c\n");
    }
   if(getans("�O�_�פJ�ӤH�H�c��ذ�? (�|�л\\�{���]�w) (y/N)")=='y')
   {
        sprintf(buf,
	   "rm -rd home/%c/%s/man>/dev/null ; mv fpg/home/bbs/home/%c/%s/man home/%c/%s", 
              cuser.userid[0], cuser.userid,
	      userid[0], userid,
	      cuser.userid[0], cuser.userid);
        system(buf);
        strcat(msg, "�פJ�ӤH�H�c��ذ�\n");
   }
   if(getans("�O�_�פJ�n�ͦW��? (�|�л\\�{���]�w, ID�i��O���P�H)? (y/N)")=='y')
   {
       sethomefile(genbuf, cuser.userid, "overrides");
       sprintf(buf, "fpg/home/bbs/home/%c/%s/overrides",userid[0],userid);
       Copy(buf, genbuf);
       strcat(buf, genbuf);
       friend_load(FRIEND_OVERRIDE);
       strcat(msg, "�פJ�n��������\n");
   }
   sprintf(buf, "�b���פJ���i %s -> %s ", userid, cuser.userid);
   post_msg("Security", buf, msg, "[�t�Φw����]");
   sprintf(buf, "fpg/home/bbs/home/%c/%s/PttID", userid[0],userid);
   if((fp = fopen(buf, "w")))
     {
        fprintf(fp, "%s\n", cuser.userid);
        fprintf(fp, "%s", msg);
	fclose(fp);
     }

   vmsg("���߱z�����b���ܨ�..");
   return 0;
}

void
m_fpg_brd(char *bname, char *fromdir)
{
  char fbname[25], buf[256];
  fileheader_t fh;

  fromdir[0]=0;
  do{

     if(!getdata(20,0, "�p�����O�W [�^��j�p�g�n�������T]:", fbname, 20,
	        DOECHO)) return;
  }
  while(invalid_brdname(fbname));

  sprintf(buf, "fpg/boards/%s.inf", fbname);
  if(!dashf(buf))
  {
       vmsg("�L���ݪO");
       return;
  }
  chdir("fpg");
  sprintf(buf, "tar zxf boards/%s.tgz >/dev/null",fbname);
  system(buf);
  sprintf(buf, "tar zxf boards/%s.man.tgz >/dev/null", fbname);
  system(buf);
  chdir(BBSHOME);
  sprintf(buf, "mv fpg/home/bbs/man/boards/%s man/boards/%c/%s", fbname,
	    bname[0], bname);
  system(buf);
  sprintf(fh.title, "�� %s ��ذ�", fbname);
  sprintf(fh.filename, fbname);
  sprintf(fh.owner, cuser.userid);
  sprintf(buf, "man/boards/%c/%s/.DIR", bname[0], bname);
  append_record(buf, &fh, sizeof(fh));
  sprintf(fromdir, "fpg/home/bbs/boards/%s/.DIR", fbname);
  vmsg("�Y�N�פJ %s �����..�����ݭn�@�I�ɶ�",fbname);
}
