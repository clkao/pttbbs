/* �إߩҦ��ݪO��ذϪ��s�� */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include "config.h"
#include "pttstruct.h"
#include "proto.h"
#define GROUPROOT BBSHOME"/man/group"

extern bcache_t *brdshm;   
extern boardheader_t *bcache;
extern void resolve_boards();
extern int numboards;

int cmpboardclass(boardheader_t **brd, boardheader_t **tmp) {
     return (strncmp((*brd)->title, (*tmp)->title, 4)<<8)+
           strcasecmp((*brd)->brdname, (*tmp)->brdname);
} 
void buildchilds(int level,char *path,int gid)
{
    char newpath[512];
    int i,count=0;
    boardheader_t *ptr, *selected[512];
    fileheader_t item;
    
    for(i=0; i<numboards;  i++)
    {
        ptr =&brdshm->bcache[i];
	if(
           ptr->gid != gid ||
	   (ptr->brdattr&(BRD_BAD | BRD_HIDE))!=0
	   ||
	   (ptr->level && !(ptr->brdattr & BRD_POSTMASK)))
	    continue;
        selected[count++]=ptr;
    }       
    qsort(&selected, count, sizeof(boardheader_t *),
             cmpboardclass); 
    for(i=0;i<count;i++)
    {
        ptr=selected[i];
        printf("%*.*s+-%-14s %-s \n",level*2,level*2,"| | | | | | | | |",
	       ptr->brdname, ptr->title);

        if(ptr->brdattr & BRD_GROUPBOARD){
	    sprintf(newpath,"%s/%s",path,ptr->brdname);
	    mkdir(newpath,0766);
	    buildchilds(level+1,newpath,ptr-bcache+1); 
	}
	else{
	    sprintf(newpath,"/bin/ln -s "BBSHOME"/man/boards/%c/%s %s/%s",
		    /* maybe something wrong with this             ^^^^^ */
		    ptr->brdname[0], ptr->brdname,path,ptr->brdname);
	    system(newpath);
	}
        strcpy(item.owner,ptr->BM);
	strtok(item.owner,"/");
        sprintf(item.title,"%-13.13s %-32.32s", level?ptr->brdname:"", ptr->title+7);
        item.filemode =  0 ;
        sprintf(item.filename,ptr->brdname);
        sprintf(newpath,"%s/.DIR",path);
	append_record(newpath, &item, sizeof(item));
    }
}


int main()
{ 
    char path[512];
    setsid();
    strcpy(path,GROUPROOT);
    system("rm -rf "GROUPROOT);
    mkdir(GROUPROOT,0766);
    resolve_boards();
    buildchilds(0,path,1);
    return 0;
}
