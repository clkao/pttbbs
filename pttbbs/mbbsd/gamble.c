/* $Id: gamble.c,v 1.10 2002/06/07 18:47:40 ptt Exp $ */
#include "bbs.h"

#ifndef _BBS_UTIL_C_
#define MAX_ITEM	8	//�̤j �䶵(item) �Ӽ�
#define MAX_ITEM_LEN	30	//�̤j �C�@�䶵�W�r����
#define MAX_SUBJECT_LEN 650	//8*81 = 648 �̤j �D�D���� 

static char betname[MAX_ITEM][MAX_ITEM_LEN];
static int currbid;

int post_msg(char* bname, char* title, char *msg, char* author)
{
    FILE *fp;
    int bid;
    fileheader_t fhdr;
    char genbuf[256];
                
    /* �b bname ���o��s�峹 */
    sprintf(genbuf, "boards/%c/%s", bname[0], bname);
    stampfile(genbuf, &fhdr);
    fp = fopen(genbuf,"w");
    
    if(!fp)
      return -1;
    
    fprintf(fp, "�@��: %s �ݪO: %s\n���D: %s \n",author,bname,title);
    fprintf(fp, "�ɶ�: %s\n", ctime(&now));
                                                
    /* �峹�����e */
    fprintf(fp, "%s", msg);
    fclose(fp);
                                                                        
    /* �N�ɮץ[�J�C�� */
    strcpy(fhdr.title, title);
    strcpy(fhdr.owner, author);
    setbdir(genbuf,bname);
    if(append_record(genbuf, &fhdr, sizeof(fhdr))!=-1)
      if((bid = getbnum(bname)) > 0)
          setbtotal(bid);
    return 0;
}

int post_file(char* bname,  char* title, char *filename, char* author)
{
 int size=dashs(filename);
 char *msg;
 FILE *fp;

 if(size<=0) return -1;
 if(!(fp=fopen(filename,"r")) ) return -1;
 msg= (char *)malloc(size);
 fread(msg,1,size,fp);
 size= post_msg(bname, title, msg, author);
 fclose(fp);
 free(msg);
 return size;
}


static int load_ticket_record(char *direct, int ticket[])
{
   char buf[256];
   int i, total=0;
   FILE *fp;
   sprintf(buf,"%s/"FN_TICKET_RECORD,direct);
   if(!(fp=fopen(buf,"r"))) return 0;
   for(i=0;i<MAX_ITEM && fscanf(fp, "%9d ",&ticket[i]); i++) 
      total=total+ticket[i]; 
   fclose(fp);
   return total;
}

static int show_ticket_data(char *direct, int *price, boardheader_t *bh) {
    int i,count, total = 0, end=0,
        ticket[MAX_ITEM] = {0, 0, 0, 0, 0, 0, 0, 0};
    FILE *fp;
    char genbuf[256];

    clear();
    if (bh)
      {
        sprintf(genbuf,"%s ��L", bh->brdname);
        showtitle(genbuf, BBSNAME);
      }	
    else
        showtitle("Ptt��L", BBSNAME);
    move(2, 0);
    sprintf(genbuf, "%s/"FN_TICKET_ITEMS, direct);
    if(!(fp = fopen(genbuf,"r")))
    {
      prints("\n�ثe�èS���|���L\n");
      sprintf(genbuf, "%s/"FN_TICKET_OUTCOME, direct);
      if(more(genbuf, NA));
      return 0;
    }
    fgets(genbuf,MAX_ITEM_LEN,fp);
    *price=atoi(genbuf);
    for(count=0; fgets(betname[count],MAX_ITEM_LEN,fp)&&count<MAX_ITEM; count++)
	strtok(betname[count],"\r\n");
    fclose(fp);

    prints("\033[32m���W:\033[m 1.�i�ʶR�H�U���P�������m���C�C�i�n�� \033[32m%d\033[m ���C\n"
         "      2.%s\n"
         "      3.�}���ɥu���@�رm������, ���ʶR�ӱm����, �h�i���ʶR���i�Ƨ���"
         "�`����C\n"
         "      4.�C�������Ѩt�Ω�� 5% ���|��%s�C\n\n"
         "\033[32m%s:\033[m", *price,
         bh?"����L�Ѫ��D�t�d�|��åB�M�w�}���ɶ����G, ��������, �@��A��C":
             "�t�ΨC�� 2:00 11:00 16:00 21:00 �}���C",
	 bh?", �䤤 2% �����}�����D":"",
         bh?"���D�ۭq�W�h�λ���":"�e�X���}�����G");


    sprintf(genbuf, "%s/"FN_TICKET, direct);
    if(!dashf(genbuf))
	{
	  sprintf(genbuf, "%s/"FN_TICKET_END, direct);
	  end=1;
	}
    show_file(genbuf, 8, -1, NO_RELOAD);
    move(15,0);
    prints("\033[1;32m�ثe�U�`���p:\033[m\n");

    total=load_ticket_record(direct, ticket);
    
    prints("\033[33m");
    for(i = 0 ; i<count; i++)
      {
	prints("%d.%-8s: %-7d", i+1, betname[i], ticket[i]);
	if(i==3) prints("\n");
      }
    prints("\033[m\n\033[42m �U�`�`���B:\033[31m %d �� \033[m",total*(*price));
    if(end)
     {
       prints("\n��L�w�g����U�`\n");
       return -count;
     }
    return count;
}

static void append_ticket_record(char *direct, int ch, int n, int count) {
    FILE *fp;
    int ticket[8] = {0,0,0,0,0,0,0,0}, i;
    char genbuf[256];
    sprintf(genbuf, "%s/"FN_TICKET_USER, direct);
    
    if((fp = fopen(genbuf,"a"))) {
	fprintf(fp, "%s %d %d\n", cuser.userid, ch, n);
	fclose(fp);
    }
    load_ticket_record(direct, ticket);
    ticket[ch] += n;
    sprintf(genbuf, "%s/" FN_TICKET_RECORD, direct);
    if((fp = fopen(genbuf,"w"))) {
        for(i=0; i<count; i++)
           fprintf(fp,"%d ", ticket[i]);
	fclose(fp);
    }
}

#define lockreturn0(unmode, state) if(lockutmpmode(unmode, state)) return 0
int ticket(int bid)
{
    int ch, n, price, count, end=0;
    char path[128],fn_ticket[128];
    boardheader_t *bh=NULL; 
    
    if(bid)
      {
       bh=getbcache(bid);
       setbpath(path, bh->brdname);   
       setbfile(fn_ticket, bh->brdname, FN_TICKET);   
       currbid = bid;
      }
    else
       strcpy(path,"etc/");

    lockreturn0(TICKET, LOCK_MULTI);
    while(1) {
	count=show_ticket_data(path, &price, bh);
        if(count<=0) 
        {
	   pressanykey();
           break;
        } 
        move(20, 0);
        reload_money();
	prints("\033[44m��: %-10d  \033[m\n\033[1m�п�ܭn�ʶR������(1~%d)"
        "[Q:���}]\033[m:", cuser.money,count);
	ch = igetch();
	/*-- 
	  Tim011127
	  ���F����CS���D ���O�o���٤��৹���ѨM�o���D,        
	  �Yuser�q�L�ˬd�U�h, ��n���D�}��, �٬O�|�y��user���o������
	  �ܦ��i��]��U����L�������h, �]�ܦ��i��Q���D�s�}��L�ɬ~��
	  ���L�o��ܤ֥i�H���쪺�O, ���h�u�|���@����ƬO����
	--*/
        if(bid && !dashf(fn_ticket))
        {
           move(b_lines-1,0);
           prints("�z!! �@����...���D�w�g����U�`�F ������P");
	   pressanykey();
	   break;
        }

        if(ch=='q' || ch == 'Q')
	    break;
        ch-='1';
	if(end || ch >= count || ch < 0)
	    continue;
        n=0;
        ch_buyitem(price, "etc/buyticket", &n);
	if(n > 0)
		append_ticket_record(path,ch,n,count);
      }	
    unlockutmpmode();
    return 0;
}

int openticket(int bid) {
    char path[128],buf[256],outcome[128];
    int i, money=0, count, bet, price, total = 0, ticket[8]={0,0,0,0,0,0,0,0};
    boardheader_t *bh=getbcache(bid);
    FILE  *fp, *fp1;

    setbpath(path, bh->brdname); 
    count=-show_ticket_data(path, &price, bh);
    if(count==0)
      {
         setbfile(buf,bh->brdname,FN_TICKET_END);
         unlink(buf); //Ptt: ��bug
         return 0;
      }
    lockreturn0(TICKET, LOCK_MULTI);
    do
    {
     do
     {
       getdata(20, 0,
         "\033[1m��ܤ��������X(0:���}�� 99:�����h��)\033[m:", buf, 3, LCECHO);
       bet=atoi(buf);
       move(0,0);
       clrtoeol();
     } while( (bet<0 || bet>count) && bet!=99);
     if(bet==0) 
        {unlockutmpmode(); return 0;}
     getdata(21, 0, "\033[1m�A���T�{��J���X\033[m:", buf, 3, LCECHO);
    }while(bet!=atoi(buf));

    bet -= 1; //�ন�x�}��index

    total=load_ticket_record(path, ticket);
    setbfile(buf,bh->brdname,FN_TICKET_END);
    if(!(fp1 = fopen(buf,"r")))
      {
         unlockutmpmode();
         return 0; 
      }
        // �٨S�}���������� �u�nmv�@���N�n 
    if(bet!=98)
     {
      money=total*price;
      demoney(money*0.02);
      mail_redenvelop("[������Y]", cuser.userid, money*0.02, 'n');
      money = ticket[bet] ?  money*0.95/ticket[bet]:9999999; 
     }
    else
     {
      vice(price*10, "��L�h������O");
      money=price;
     }
    setbfile(outcome,bh->brdname,FN_TICKET_OUTCOME);
    if((fp = fopen(outcome, "w")))
    {  
      fprintf(fp,"��L����\n");
      while(fgets(buf,256,fp1))
         {
	   buf[255]=0;
	   fprintf(fp,"%s",buf);
         }
      fprintf(fp,"�U�`���p\n");

      fprintf(fp, "\033[33m");
      for(i = 0 ; i<count; i++)
      {
        fprintf(fp, "%d.%-8s: %-7d",i+1,betname[i], ticket[i]);
        if(i==3) fprintf(fp,"\n");
      }
      fprintf(fp, "\033[m\n");
      
      if(bet!=98)
       {                                  
        fprintf(fp, "\n\n�}���ɶ��G %s \n\n"
             "�}�����G�G %s \n\n"
             "�Ҧ����B�G %d �� \n"
             "������ҡG %d�i/%d�i  (%f)\n"
             "�C�i�����m���i�o %d �T�޹� \n\n",
             Cdatelite(&now), betname[bet], total*price, ticket[bet], total,
             (float) ticket[bet] / total, money);
             
        fprintf(fp, "%s ��L�}�X:%s �Ҧ����B:%d �� ����/�i:%d �� ���v:%1.2f\n",
              Cdatelite(&now), betname[bet], total*price, money,
              total? (float)ticket[bet] / total:0);
       }
      else 
        fprintf(fp, "\n\n��L�����h���G %s \n\n",  Cdatelite(&now));
              
    }
    fclose(fp1); 

    setbfile(buf, bh->brdname, FN_TICKET_END);
    unlink(buf);
    if(fork())
      {  // Ptt ��fork������`�_�u�~��
        move(22,0);
    prints("�t�αN��y��۰ʧ⤤�����G���G��ݪO �Y�ѥ[�̦h�|�ݭn�X�����ɶ�..");
        pressanykey();
        unlockutmpmode();
        return 0;
      }
    close(0);
    close(1);
    /*
      �H�U�O�����ʧ@
    */
    setbfile(buf, bh->brdname, FN_TICKET_USER);
    if ((bet==98 || ticket[bet]) && (fp1 = fopen(buf, "r")))  
    {
        int mybet, uid;
        char userid[IDLEN];
        
        while (fscanf(fp1, "%s %d %d\n", userid, &mybet, &i) != EOF)
        {
           if (bet==98 )
           {
                fprintf(fp,"%s �R�F %d �i %s, �h�^ %d �T�޹�\n"
                       ,userid, i, betname[mybet], money);
                sprintf(buf, "%s ����h��! $ %d", bh->brdname,  money * i);
           }
           else if (mybet == bet)
           {
                fprintf(fp,"���� %s �R�F%d �i %s, ��o %d �T�޹�\n"
                       ,userid, i, betname[mybet], money);
                sprintf(buf, "%s ������! $ %d", bh->brdname,  money * i);
           }
           if((uid=getuser(userid))==0) continue;
           deumoney(uid, money * i);
           mail_id(userid, buf, "etc/ticket.win", "Ptt���");
        }   
        fclose(fp1);
    }
    fclose(fp);

    if(bet!=98)
      sprintf(buf, "[���i] %s ��L�}��", bh->brdname);
    else
      sprintf(buf, "[���i] %s ��L����", bh->brdname);
    post_file(bh->brdname, buf, outcome, "[�䯫]");
    post_file("Record", buf+7, outcome, "[�������l]");

    setbfile(buf, bh->brdname, FN_TICKET_RECORD); 
    unlink(buf);
    setbfile(buf, bh->brdname, FN_TICKET_USER); 
    unlink(buf);
    exit(1);
    return 0;
}

int ticket_main() {
    ticket(0);
    return 0;
}
#endif
