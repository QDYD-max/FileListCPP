#include <stdio.h>
#include <malloc.h>  
#include <string.h>  
#include <time.h>  

#define BLOCKSIZE 1024  // 磁盘块大小  
#define SIZE 1024000  // 虚拟磁盘空间大小  
#define END 65535  // FAT中的文件结束标志  
#define FREE 0  // 磁盘块、FAT中盘块空闲标志  
#define BUSY 1  //磁盘块占用标志
#define ROOTBLOCKNUM 2  // 根目录区所占盘块数  
#define MAXOPENFILE 10  // 最多同时打开文件个数t  
#define MAXTEXT 10000  

#define FIXEDSIZE 1024     
#define MAGNIFICATION FIXEDSIZE/BLOCKSIZE   //倍率
#define LINE (100 * MAGNIFICATION)    //列
#define ROW SIZE / LINE /BLOCKSIZE   //行
#define ROOTSTART (SIZE /FIXEDSIZE * 2 / BLOCKSIZE )+ 6

#pragma warning(disable:4996)

/* 文件控制块 */
typedef struct FCB
{
    char filename[8];  // 文件名  
    char exname[3];  // 文件扩展名  
    unsigned char attribute;  // 文件属性字段，值为0时表示目录文件，值为1时表示数据文件  
    unsigned short time;  // 文件创建时间  
    unsigned short date;  // 文件创建日期  
    unsigned short first;  // 文件起始盘块号  
    unsigned long length;  // 文件长度  
    char free;  // 表示目录项是否为空，若值为0，表示空，值为1，表示已分配  


}fcb;

/* 文件分配表 */
typedef struct FAT
{
    unsigned short id;  // 磁盘块的状态（END，下一个）  
}fat;


/* 用户打开文件表 */
typedef struct USEROPEN
{
    char filename[8];  // 文件名  
    char exname[3];  // 文件扩展名  
    unsigned char attribute;  // 文件属性字段，值为0时表示目录文件，值为1时表示数据文件  
    unsigned short time;  // 文件创建时间  
    unsigned short date;  // 文件创建日期  
    unsigned short first;  // 文件起始盘块号  
    unsigned long length;  // 文件长度（对数据文件是字节数，对目录文件可以是目录项个数）  
    char free;  // 表示目录项是否为空，若值为0，表示空，值为1，表示已分配  

    unsigned short dirno;  // 相应打开文件的目录项在父目录文件中的盘块号  
    int diroff;  // 相应打开文件的目录项在父目录文件的dirno盘块中的目录项序号  
    char dir[80];  // 相应打开文件所在的目录名，这样方便快速检查出指定文件是否已经打开  
    int father;  // 父目录在打开文件表项的位置  
    int count;  // 读写指针在文件中的位置,文件的总字符数  
    char fcbstate;  // 是否修改了文件的FCB的内容，如果修改了置为1，否则为0  
    char topenfile;  // 表示该用户打开表项是否为空，若值为0，表示为空，否则表示已被某打开文件占据  
}useropen;

/* 引导块 */
typedef struct BLOCK0
{
    char magic[10];  // 文件系统魔数  
    char information[200];  // 存储一些描述信息，如磁盘块大小、磁盘块数量、最多打开文件数等  
    unsigned short root;  // 根目录文件的起始盘块号  
    unsigned char* startblock;  // 虚拟磁盘上数据区开始位置  
}block0;

/*位示图*/
typedef struct MAP
{
    unsigned short int map[ROW][LINE];//位示图
}map;


unsigned char* myvhard;  // 指向虚拟磁盘的起始地址  
useropen openfilelist[MAXOPENFILE];  // 用户打开文件表数组  
int curdir;  // 用户打开文件表中的当前目录所在打开文件表项的位置  
char currentdir[80];  // 记录当前目录的目录名（包括目录的路径）  
unsigned char* startp;  // 记录虚拟磁盘上数据区开始位置  
char myfilename[] = "myfilesys";//文件系统的文件名  

void startsys();  // 进入文件系统  
void my_format();  // 磁盘格式化  
void my_cd(char* dirname);  // 更改当前目录  
void my_mkdir(char* dirname);  // 创建子目录  
void my_rmdir(char* dirname);  // 删除子目录  
void my_ls();  // 显示目录  
void my_create(char* filename);  // 创建文件  
void my_rm(char* filename);  // 删除文件  
int my_open(char* filename);  // 打开文件  
int my_close(int fd);  // 关闭文件  
int my_write(int fd);  // 写文件  
int do_write(int fd, char* text, int len, char wstyle);  // 实际写文件  
int my_read(int fd, int len);  // 读文件  
int do_read(int fd, int len, char* text);  // 实际读文件  
void my_exitsys();  // 退出文件系统  
unsigned short findblock();  // 寻找空闲盘块  
int findopenfile();  // 寻找空闲文件表项  
void restore(); //恢复删除的文件
void synchronization(); //固定时间备份fat2表


void startsys()
{
    FILE* fp;
    unsigned char buf[SIZE];
    fcb* root;
    int i;
    myvhard = (unsigned char*)malloc(SIZE);//申请虚拟磁盘空间  
    memset(myvhard, 0, SIZE);//将myvhard中前SIZE个字节用 0 替换并返回 myvhard  
    if ((fp = fopen(myfilename, "r")) != NULL)
    {
        fread(buf, SIZE, 1, fp);//将二进制文件读取到缓冲区  
        fclose(fp);//关闭打开的文件，缓冲区数据写入文件，释放系统提供文件资源  
        if (strcmp(((block0*)buf)->magic, "10101010"))//判断开始的8个字节内容是否为文件系统魔数  
        {
            printf("myfilesys is not exist,begin to creat the file...\n");
            my_format();
        }
        else
        {
            for (i = 0; i < SIZE; i++)
                myvhard[i] = buf[i];
        }
    }
    else
    {
        printf("myfilesys is not exist,begin to creat the file...\n");
        my_format();
    }
    root = (fcb*)(myvhard + ROOTSTART * BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    strcpy(openfilelist[0].filename, root->filename);
    strcpy(openfilelist[0].exname, root->exname);
    openfilelist[0].attribute = root->attribute;
    openfilelist[0].time = root->time;
    openfilelist[0].date = root->date;
    openfilelist[0].first = root->first;
    openfilelist[0].length = root->length;
    openfilelist[0].free = root->free;
    openfilelist[0].dirno = ROOTSTART;
    openfilelist[0].diroff = 0;
    strcpy(openfilelist[0].dir, "\\root\\");
    openfilelist[0].father = 0;
    openfilelist[0].count = 0;
    openfilelist[0].fcbstate = 0;
    openfilelist[0].topenfile = 1;
    for (i = 1; i < MAXOPENFILE; i++)
        openfilelist[i].topenfile = 0;
    curdir = 0;
    strcpy(currentdir, "\\root\\");
    startp = ((block0*)myvhard)->startblock;
}
void my_format()
{
    FILE* fp;
    fat* fat1, * fat2;
    map* wst;
    block0* blk0;
    time_t now;
    struct tm* nowtime;
    fcb* root;
    int i;
    blk0 = (block0*)myvhard;
    fat1 = (fat*)(myvhard + BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    fat2 = (fat*)(myvhard + 3 * BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    wst = (map*)(myvhard + 5 * BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    root = (fcb*)(myvhard + ROOTSTART * BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    strcpy(blk0->magic, "10101010");
    strcpy(blk0->information, "My FileSystem Ver 1.0 \n Blocksize=1KB Whole size=1000KB Blocknum=1000 RootBlocknum=2\n");
    blk0->root = ROOTSTART * MAGNIFICATION;//@@磁盘块大小修改的相关操作
    blk0->startblock = (unsigned char*)root;
    for (i = 0; i < blk0->root; i++)//@@磁盘块大小修改的相关操作
    {//blk0、fat1、fat2、wst盘块占用
        wst->map[ i / LINE ][ i % LINE ] = BUSY;
        fat1->id = END;
        fat2->id = END;
        fat1++;
        fat2++;
    }


    for (i = blk0->root;i < (ROOTSTART+1) * MAGNIFICATION; i++) //@@磁盘块大小修改的相关操作
    {//root下的"."占用ROOTSTART * (FIXEDSIZE / BLOCKSIZE)块
        fat1->id = i + 1;
        fat2->id = i + 1;
        wst->map[i / LINE][i % LINE] = BUSY;
        fat1++;
        fat2++;
    }
    fat1->id = (ROOTSTART + 1) * MAGNIFICATION;
    fat2->id = (ROOTSTART + 1) * MAGNIFICATION;//指向root下的".."所在的块


    //fat1->id = 6;//root下的"."
    //fat2->id = 6;
    //blk0->map[0][5] = END;
    //fat1++;
    //fat2++;
    for (i = (ROOTSTART+1) * MAGNIFICATION;i < (ROOTSTART+2) * MAGNIFICATION; i++) //@@磁盘块大小修改的相关操作
    {//root下的".."占用(ROOTSTART+1) * (FIXEDSIZE / BLOCKSIZE)块
        fat1->id = i + 1;
        fat2->id = i + 1;
        wst->map[i / LINE][i % LINE] = BUSY;
        fat1++;
        fat2++;
    }
    fat1->id = END;
    fat2->id = END;//root所在的块结束

    //fat1->id = END;//root下的".."
    //fat2->id = END;
    //blk0->map[0][6] = END;
    //fat1++;
    //fat2++;
    for (i = (ROOTSTART + 2) * MAGNIFICATION; i < SIZE / BLOCKSIZE; i++)//@@磁盘块大小修改的相关操作
    {//初始化块以外的所有块置闲
        wst->map[i / LINE][i % LINE] = FREE;
        //fat1->id = FREE;
        //fat2->id = FREE;
        //fat1++;
        //fat2++;
    }

    now = time(NULL);
    nowtime = localtime(&now);
    strcpy(root->filename, ".");
    strcpy(root->exname, "");
    root->attribute = 0x28;
    root->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
    root->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
    root->first = ROOTSTART;
    root->length = 2 * sizeof(fcb);
    root->free = 1;
    root++;
    //root = root + BLOCKSIZE * MAGNIFICATION;
    now = time(NULL);
    nowtime = localtime(&now);
    strcpy(root->filename, "..");
    strcpy(root->exname, "");
    root->attribute = 0x28;
    root->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
    root->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
    root->first = ROOTSTART;
    root->length = 2 * sizeof(fcb);
    root->free = 1;
    fp = fopen(myfilename, "w");
    fwrite(myvhard, SIZE, 1, fp);
    fclose(fp);
}

void restore()
{
    fat *fat1, *fat2,*fatprt1,* fatprt2;
    fcb* fcbptr;
    map* wst;
    char* fname, * exname, text[MAXTEXT];
    int i,rbn;

    fat1 = (fat*)(myvhard + BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    fat2 = (fat*)(myvhard + 3 * BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    wst = (map*)(myvhard + 5 * BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    fatprt1 = fat1;
    fatprt2 = fat2;
    for (int i = 0;i < SIZE / BLOCKSIZE;i++)
    {
        fatprt1->id = fatprt2->id;
        fatprt1++;
        fatprt2++;
    }
    openfilelist[curdir].count = 0;
    rbn = do_read(curdir, openfilelist[curdir].length, text);
    fcbptr = (fcb*)text;
    for (i = 0; i < rbn / sizeof(fcb); i++)
    {
        if (fcbptr->free == 0)
        {
            fcbptr->free = 1;
            openfilelist[curdir].count = i * sizeof(fcb);
            do_write(curdir, (char*)fcbptr, sizeof(fcb), 2);
        }
        fcbptr++;
    }
    for (int i = 0;i < SIZE / BLOCKSIZE;i++)
    {
        fatprt1 = fat1;
        if (fatprt1->id != 0)
        {
            wst->map[i / LINE][i % LINE] = BUSY;
        }
        fatprt1++;
    }
}

void my_cd(char* dirname)
{
    char* dir;
    int fd;
    dir = strtok(dirname, "\\");//分解字符串为一组字符串。dirname为要分解的字符串，"\\"为分隔符字符串  
    if (strcmp(dir, "..") == 0)
    {
        if (curdir)
            curdir = my_close(curdir);
        return;
    }
    else if (strcmp(dir, "root") == 0)
    {
        while (curdir)
            curdir = my_close(curdir);
        dir = strtok(NULL, "\\");
    }
    else if (strcmp(dir, ".") == 0)
    {
        return;
    }

    while (dir)
    {
        fd = my_open(dir);
        if (fd != -1)
            curdir = fd;
        else
            return;
        dir = strtok(NULL, "\\");
    }
}
void my_mkdir(char* dirname)
{
    fcb* fcbptr;
    block0* blk0;
    map* wst;
    fat* fat1, * fat2;
    time_t now;
    struct tm* nowtime;
    char text[MAXTEXT];
    unsigned short blkno;
    int rbn, fd, i;
    blk0 = (block0*)myvhard;
    fat1 = (fat*)(myvhard + BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    fat2 = (fat*)(myvhard + 3 * BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    wst = (map*)(myvhard + 5 * BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    openfilelist[curdir].count = 0;
    rbn = do_read(curdir, openfilelist[curdir].length, text);
    fcbptr = (fcb*)text;
    for (i = 0;i < rbn / sizeof(fcb); i++)//在当前目录下找，是否有重名目录  
    {
        if (strcmp(fcbptr->filename, dirname) == 0 && strcmp(fcbptr->exname, "") == 0)
        {
            printf("Error,the dirname is already exist!\n");
            return;
        }
        fcbptr++;
    }
    fcbptr = (fcb*)text;
    for (i = 0; i < rbn / sizeof(fcb); i++)
    {
        if (fcbptr->free == 0)
            break;
        fcbptr++;
    }

   
    blkno = findblock();//寻找空闲盘块  
    if (blkno == -1)
        return;
    (fat1 + blkno)->id = END;
    (fat2 + blkno)->id = END;
    wst->map[blkno / LINE][blkno % LINE] = BUSY;
    now = time(NULL);
    nowtime = localtime(&now);
    strcpy(fcbptr->filename, dirname);
    strcpy(fcbptr->exname, "");
    fcbptr->attribute = 0x30;
    fcbptr->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
    fcbptr->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
    fcbptr->first = blkno;
    fcbptr->length = 2 * sizeof(fcb);
    fcbptr->free = 1;
    openfilelist[curdir].count = i * sizeof(fcb);
    do_write(curdir, (char*)fcbptr, sizeof(fcb), 2);

    fd = my_open(dirname);//建立新目录的'.','..'目录  
    if (fd == -1)
        return;
    fcbptr = (fcb*)malloc(sizeof(fcb));
    now = time(NULL);
    nowtime = localtime(&now);
    strcpy(fcbptr->filename, ".");
    strcpy(fcbptr->exname, "");
    fcbptr->attribute = 0x28;
    fcbptr->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
    fcbptr->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
    fcbptr->first = blkno;
    fcbptr->length = 2 * sizeof(fcb);
    fcbptr->free = 1;
    do_write(fd, (char*)fcbptr, sizeof(fcb), 2);
    now = time(NULL);
    nowtime = localtime(&now);
    strcpy(fcbptr->filename, "..");
    strcpy(fcbptr->exname, "");
    fcbptr->attribute = 0x28;
    fcbptr->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
    fcbptr->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
    fcbptr->first = blkno;
    fcbptr->length = 2 * sizeof(fcb);
    fcbptr->free = 1;
    do_write(fd, (char*)fcbptr, sizeof(fcb), 2);
    free(fcbptr);
    my_close(fd);

    fcbptr = (fcb*)text;
    fcbptr->length = openfilelist[curdir].length;
    openfilelist[curdir].count = 0;
    do_write(curdir, (char*)fcbptr, sizeof(fcb), 2);
    openfilelist[curdir].fcbstate = 1;
}

void my_rmdir(char* dirname)
{
    fcb* fcbptr, * fcbptr2;
    fat* fat1, * fat2, * fatptr1, * fatptr2;
    char text[MAXTEXT], text2[MAXTEXT];
    unsigned short blkno;
    int rbn, rbn2, fd, i, j;
    block0* blk0;
    map* wst;
    blk0 = (block0*)myvhard;
    fat1 = (fat*)(myvhard + BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    fat2 = (fat*)(myvhard + 3 * BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    wst = (map*)(myvhard + 5 * BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    if (strcmp(dirname, ".") == 0 || strcmp(dirname, "..") == 0)
    {
        printf("Error,can't remove this directory.\n");
        return;
    }
    openfilelist[curdir].count = 0;
    rbn = do_read(curdir, openfilelist[curdir].length, text);
    fcbptr = (fcb*)text;
    for (i = 0; i < rbn / sizeof(fcb); i++)//查找要删除的目录  
    {
        if (strcmp(fcbptr->filename, dirname) == 0 && strcmp(fcbptr->exname, "") == 0)
            break;
        fcbptr++;
    }
    if (i == rbn / sizeof(fcb))
    {
        printf("Error,the directory is not exist.\n");
        return;
    }
    fd = my_open(dirname);
    rbn2 = do_read(fd, openfilelist[fd].length, text2);
    fcbptr2 = (fcb*)text2;
    for (j = 0; j < rbn2 / sizeof(fcb); j++)//判断要删除目录是否为空  
    {
        if (strcmp(fcbptr2->filename, ".") && strcmp(fcbptr2->filename, "..") && strcmp(fcbptr2->filename, ""))
        {
            my_close(fd);
            printf("Error,the directory is not empty.\n");
            return;
        }
        fcbptr2++;
    }
    blkno = openfilelist[fd].first;
    while (blkno != END)
    {
        fatptr1 = fat1 + blkno;
        fatptr2 = fat2 + blkno;
        blkno = fatptr1->id;
        wst->map[blkno / LINE][blkno % LINE] = FREE;
        //fatptr1->id = FREE;
        //fatptr2->id = FREE;
    }
    my_close(fd);
    strcpy(fcbptr->filename, "");
    fcbptr->free = 0;
    openfilelist[curdir].count = i * sizeof(fcb);
    do_write(curdir, (char*)fcbptr, sizeof(fcb), 2);
    openfilelist[curdir].fcbstate = 1;
}
void my_ls()
{
    fcb* fcbptr;
    char text[MAXTEXT];
    int rbn, i;
    openfilelist[curdir].count = 0;
    rbn = do_read(curdir, openfilelist[curdir].length, text);
    fcbptr = (fcb*)text;
    for (i = 0; i < rbn / sizeof(fcb); i++)
    {
        if (fcbptr->free)
        {
            if (fcbptr->attribute & 0x20)
                printf("%s\\\t\t<DIR>\t\t%d/%d/%d\t%02d:%02d:%02d\n", fcbptr->filename, (fcbptr->date >> 9) + 1980, (fcbptr->date >> 5) & 0x000f, fcbptr->date & 0x001f, fcbptr->time >> 11, (fcbptr->time >> 5) & 0x003f, fcbptr->time & 0x001f * 2);
            else
                printf("%s.%s\t\t%dB\t\t%d/%d/%d\t%02d:%02d:%02d\t\n", fcbptr->filename, fcbptr->exname, (int)(fcbptr->length), (fcbptr->date >> 9) + 1980, (fcbptr->date >> 5) & 0x000f, fcbptr->date & 0x1f, fcbptr->time >> 11, (fcbptr->time >> 5) & 0x3f, fcbptr->time & 0x1f * 2);
        }
        fcbptr++;
    }
}
void my_create(char* filename)
{
    fcb* fcbptr;
    fat* fat1, * fat2;
    char* fname, * exname, text[MAXTEXT];
    unsigned short blkno;
    int rbn, i;
    time_t now;
    struct tm* nowtime;
    block0* blk0;
    map* wst;
    blk0 = (block0*)myvhard;
    fat1 = (fat*)(myvhard + BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    fat2 = (fat*)(myvhard + 3 * BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    wst = (map*)(myvhard + 5 * BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    fname = strtok(filename, ".");
    exname = strtok(NULL, ".");
    if (strcmp(fname, "") == 0)
    {
        printf("Error,creating file must have a right name.\n");
        return;
    }
    if (!exname)
    {
        printf("Error,creating file must have a extern name.\n");
        return;
    }
    if (strlen(exname)>3)
    {
        printf("Error,the extern name is too long.\n");
        return;
    }
    openfilelist[curdir].count = 0;
    rbn = do_read(curdir, openfilelist[curdir].length, text);
    fcbptr = (fcb*)text;
    for (i = 0; i < rbn / sizeof(fcb); i++)
    {
        if (strcmp(fcbptr->filename, fname) == 0 && strcmp(fcbptr->exname, exname) == 0)
        {
            printf("Error,the filename is already exist!\n");
            return;
        }
        fcbptr++;
    }
    fcbptr = (fcb*)text;
    for (i = 0; i < rbn / sizeof(fcb); i++)
    {
        if (fcbptr->free == 0)
            break;
        fcbptr++;
    }
    blkno = findblock();
    if (blkno == -1)
        return;
    (fat1 + blkno)->id = END;
    (fat2 + blkno)->id = END;
    wst->map[blkno / LINE][blkno % LINE] = BUSY;

    now = time(NULL);
    nowtime = localtime(&now);
    strcpy(fcbptr->filename, fname);
    strcpy(fcbptr->exname, exname);
    fcbptr->attribute = 0x00;
    fcbptr->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
    fcbptr->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
    fcbptr->first = blkno;
    fcbptr->length = 0;
    fcbptr->free = 1;
    openfilelist[curdir].count = i * sizeof(fcb);
    do_write(curdir, (char*)fcbptr, sizeof(fcb), 2);
    fcbptr = (fcb*)text;
    fcbptr->length = openfilelist[curdir].length;
    openfilelist[curdir].count = 0;
    do_write(curdir, (char*)fcbptr, sizeof(fcb), 2);
    openfilelist[curdir].fcbstate = 1;
}
void my_rm(char* filename)
{
    fcb* fcbptr;
    fat* fat1, * fat2, * fatptr1, * fatptr2;
    char* fname, * exname, text[MAXTEXT];
    unsigned short blkno;
    int rbn, i;
    block0* blk0;
    map* wst;
    blk0 = (block0*)myvhard;
    fat1 = (fat*)(myvhard + BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    fat2 = (fat*)(myvhard + 3 * BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    wst = (map*)(myvhard + 5 * BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    fname = strtok(filename, ".");
    exname = strtok(NULL, ".");
    if (strcmp(fname, "") == 0)
    {
        printf("Error,removing file must have a right name.\n");
        return;
    }
    if (!exname)
    {
        printf("Error,removing file must have a extern name.\n");
        return;
    }
    openfilelist[curdir].count = 0;
    rbn = do_read(curdir, openfilelist[curdir].length, text);
    fcbptr = (fcb*)text;
    for (i = 0; i < rbn / sizeof(fcb); i++)
    {
        if (strcmp(fcbptr->filename, fname) == 0 && strcmp(fcbptr->exname, exname) == 0)
            break;
        fcbptr++;
    }
    if (i == rbn / sizeof(fcb))
    {
        printf("Error,the file is not exist.\n");
        return;
    }
    openfilelist[curdir].count = 0;
    rbn = do_read(curdir, openfilelist[curdir].length, text);
    fcbptr = (fcb*)text;
    for (i = 0; i < rbn / sizeof(fcb); i++)
    {
        if (strcmp(fcbptr->filename, fname) == 0 && strcmp(fcbptr->exname, exname) == 0)
            break;
        fcbptr++;
    }
    if (i == rbn / sizeof(fcb))
    {
        printf("Error,the file is not exist.\n");
        return;
    }
    blkno = fcbptr->first;
    while (blkno != END)
    {
        fatptr1 = fat1 + blkno;
        //fatptr2 = fat2 + blkno;
        blkno = fatptr1->id;
        wst->map[blkno / LINE][blkno % LINE] = FREE;
        fatptr1->id = FREE;
        //fatptr2->id = FREE;
    }
    //strcpy(fcbptr->filename, "");
    fcbptr->free = 0;
    openfilelist[curdir].count = i * sizeof(fcb);
    do_write(curdir, (char*)fcbptr, sizeof(fcb), 2);
    openfilelist[curdir].fcbstate = 1;
}

int my_open(char* filename)
{
    fcb* fcbptr;
    char* fname, exname[4], * str, text[MAXTEXT];
    int rbn, fd, i;
    fname = strtok(filename, ".");
    str = strtok(NULL, ".");
    if (str)
        strcpy(exname, str);
    else
        strcpy(exname, "");
    for (i = 0; i < MAXOPENFILE; i++)
    {
        if (strcmp(openfilelist[i].filename, fname) == 0 && strcmp(openfilelist[i].exname, exname) == 0 && i != curdir)
        {
            printf("Error,the file is already open.\n");
            return curdir;
        }
    }
    openfilelist[curdir].count = 0;
    rbn = do_read(curdir, openfilelist[curdir].length, text);
    fcbptr = (fcb*)text;
    for (i = 0; i < rbn / sizeof(fcb); i++)
    {
        if (strcmp(fcbptr->filename, fname) == 0 && strcmp(fcbptr->exname, exname) == 0)
            break;
        fcbptr++;
    }
    if (i == rbn / sizeof(fcb))
    {
        printf("Error,the file is not exist.\n");
        return curdir;
    }
    fd = findopenfile();
    if (fd == -1)
        return curdir;
    strcpy(openfilelist[fd].filename, fcbptr->filename);
    strcpy(openfilelist[fd].exname, fcbptr->exname);
    openfilelist[fd].attribute = fcbptr->attribute;
    openfilelist[fd].time = fcbptr->time;
    openfilelist[fd].date = fcbptr->date;
    openfilelist[fd].first = fcbptr->first;
    openfilelist[fd].length = fcbptr->length;
    openfilelist[fd].free = fcbptr->free;
    openfilelist[fd].dirno = openfilelist[curdir].first;
    openfilelist[fd].diroff = i;
    strcpy(openfilelist[fd].dir, openfilelist[curdir].dir);
    strcat(openfilelist[fd].dir, filename);
    if (fcbptr->attribute & 0x20)
        strcat(openfilelist[fd].dir, "\\");
    openfilelist[fd].father = curdir;
    openfilelist[fd].count = 0;
    openfilelist[fd].fcbstate = 0;
    openfilelist[fd].topenfile = 1;
    return fd;
}
int my_close(int fd)
{
    fcb* fcbptr;
    int father = openfilelist[fd].father;
    if (fd < 0 || fd >= MAXOPENFILE)
    {
        printf("Error,the file is not exist.\n");
        return -1;
    }
    if (openfilelist[fd].fcbstate)
    {
        fcbptr = (fcb*)malloc(sizeof(fcb));
        strcpy(fcbptr->filename, openfilelist[fd].filename);
        strcpy(fcbptr->exname, openfilelist[fd].exname);
        fcbptr->attribute = openfilelist[fd].attribute;
        fcbptr->time = openfilelist[fd].time;
        fcbptr->date = openfilelist[fd].date;
        fcbptr->first = openfilelist[fd].first;
        fcbptr->length = openfilelist[fd].length;
        fcbptr->free = openfilelist[fd].free;
        father = openfilelist[fd].father;
        openfilelist[father].count = openfilelist[fd].diroff * sizeof(fcb);
        do_write(father, (char*)fcbptr, sizeof(fcb), 2);
        free(fcbptr);
        openfilelist[fd].fcbstate = 0;
    }
    strcpy(openfilelist[fd].filename, "");
    strcpy(openfilelist[fd].exname, "");
    openfilelist[fd].topenfile = 0;
    return father;
}
int my_write(int fd)
{
    fat* fat1, * fat2, * fatptr1, * fatptr2;
    int wstyle, len, ll, tmp;
    char text[MAXTEXT];
    unsigned short blkno;
    block0* blk0;
    map* wst;
    blk0 = (block0*)myvhard;
    fat1 = (fat*)(myvhard + BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    fat2 = (fat*)(myvhard + 3 * BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    wst = (map*)(myvhard + 5 * BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    if (fd < 0 || fd >= MAXOPENFILE)
    {
        printf("The file is not exist!\n");
        return -1;
    }
    while (1)
    {
        printf("Please enter the number of write style:\n1.cut write\t2.cover write\t3.add write\n");
        scanf("%d", &wstyle);
        if (wstyle > 0 && wstyle < 4)
            break;
        printf("Input Error!");
    }
    getchar();
    switch (wstyle)
    {
    case 1:
        blkno = openfilelist[fd].first;
        fatptr1 = fat1 + blkno;
        fatptr2 = fat2 + blkno;
        blkno = fatptr1->id;
        fatptr1->id = END;
        fatptr2->id = END;
        while (blkno != END)
        {
            fatptr1 = fat1 + blkno;
            fatptr2 = fat2 + blkno;
            blkno = fatptr1->id;
            wst->map[blkno / LINE][blkno % LINE] = FREE;
            //fatptr1->id = FREE;
            //fatptr2->id = FREE;
        }
        openfilelist[fd].count = 0;
        openfilelist[fd].length = 0;
        break;
    case 2:
        openfilelist[fd].count = 0;
        break;
    case 3:
        openfilelist[fd].count = openfilelist[fd].length;
        break;
    default:
        break;
    }
    ll = 0;
    printf("please input write data(end with Ctrl+Z):\n");
    while (gets_s(text))
    {
        len = strlen(text);
        text[len++] = '\n';
        text[len] = '\0';
        tmp = do_write(fd, text, len, wstyle);
        if (tmp != -1)
            ll += tmp;
        if (tmp < len)
        {
            printf("Wirte Error!");
            break;
        }
    }
    return ll;
}

int do_write(int fd, char* text, int len, char wstyle)
{
    fat* fat1, * fat2, * fatptr1, * fatptr2;
    unsigned char* buf, * blkptr;
    unsigned short blkno, blkoff;
    int i, ll;
    block0* blk0;
    map* wst;
    blk0 = (block0*)myvhard;
    fat1 = (fat*)(myvhard + BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    fat2 = (fat*)(myvhard + 3 * BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    wst = (map*)(myvhard + 5 * BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    buf = (unsigned char*)malloc(BLOCKSIZE);
    if (buf == NULL)
    {
        printf("malloc failed!\n");
        return -1;
    }
    blkno = openfilelist[fd].first;
    blkoff = openfilelist[fd].count;
    fatptr1 = fat1 + blkno;
    fatptr2 = fat2 + blkno;
    while (blkoff >= BLOCKSIZE)//找到打开目录的结束盘块
    {
        blkno = fatptr1->id;
        if (blkno == END)
        {
            blkno = findblock();
            if (blkno == -1)
            {
                free(buf);
                return -1;
            }
            fatptr1->id = blkno;
            fatptr2->id = blkno;
            fatptr1 = fat1 + blkno;
            fatptr2 = fat2 + blkno;
            wst->map[blkno / LINE][blkno % LINE] = BUSY;
            fatptr1->id = END;
            fatptr2->id = END;
        }
        else
        {
            fatptr1 = fat1 + blkno;
            fatptr2 = fat2 + blkno;
        }
        blkoff = blkoff - BLOCKSIZE;
    }

    ll = 0;
    while (ll < len)
    {
        blkptr = (unsigned char*)(myvhard + blkno * BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
        for (i = 0; i < BLOCKSIZE; i++)
            buf[i] = blkptr[i];
        for (;blkoff < BLOCKSIZE; blkoff++)
        {
            buf[blkoff] = text[ll++];
            openfilelist[fd].count++;
            if (ll == len)
                break;
        }
        for (i = 0; i < BLOCKSIZE; i++)
            blkptr[i] = buf[i];
        if (ll < len)
        {
            blkno = fatptr1->id;
            if (blkno == END)
            {
                blkno = findblock();
                if (blkno == -1)
                    break;
                fatptr1->id = blkno;
                fatptr2->id = blkno;
                fatptr1 = fat1 + blkno;
                fatptr2 = fat2 + blkno;
                fatptr1->id = END;
                fatptr2->id = END;
                wst->map[blkno / LINE][blkno % LINE] = BUSY;
            }
            else
            {
                fatptr1 = fat1 + blkno;
                fatptr2 = fat2 + blkno;
            }
            blkoff = 0;
        }
    }
    if (openfilelist[fd].count > openfilelist[fd].length)
        openfilelist[fd].length = openfilelist[fd].count;
    openfilelist[fd].fcbstate = 1;
    free(buf);
    return ll;
}
int my_read(int fd, int len)
{
    char text[MAXTEXT];
    int ll;
    if (fd < 0 || fd >= MAXOPENFILE)
    {
        printf("The File is not exist!\n");
        return -1;
    }
    openfilelist[fd].count = 0;
    ll = do_read(fd, len, text);
    if (ll != -1)
        printf("%s", text);
    else
        printf("Read Error!\n");
    return ll;
}
int do_read(int fd, int len, char* text)
{
    fat* fat1, * fatptr;
    unsigned char* buf, * blkptr;
    unsigned short blkno, blkoff;
    int i, ll;
    fat1 = (fat*)(myvhard + BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    buf = (unsigned char*)malloc(BLOCKSIZE);
    if (buf == NULL)
    {
        printf("malloc failed!\n");
        return -1;
    }
    blkno = openfilelist[fd].first;
    blkoff = openfilelist[fd].count;
    if (blkoff >= openfilelist[fd].length)
    {
        puts("Read out of range!");
        free(buf);
        return -1;
    }
    fatptr = fat1 + blkno;
    while (blkoff >= BLOCKSIZE)
    {
        blkno = fatptr->id;
        blkoff = blkoff - BLOCKSIZE;
        fatptr = fat1 + blkno;
    }
    ll = 0;
    while (ll < len)
    {
        blkptr = (unsigned char*)(myvhard + blkno * BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
        for (i = 0; i < BLOCKSIZE; i++)
            buf[i] = blkptr[i];
        for (; blkoff < BLOCKSIZE; blkoff++)
        {
            text[ll++] = buf[blkoff];
            openfilelist[fd].count++;
            if (ll == len || openfilelist[fd].count == openfilelist[fd].length)
                break;
        }
        if (ll < len && openfilelist[fd].count != openfilelist[fd].length)
        {
            blkno = fatptr->id;
            if (blkno == END)
                break;
            blkoff = 0;
            fatptr = fat1 + blkno;
        }
    }
    text[ll] = '\0';
    free(buf);
    return ll;
}
void my_exitsys()
{
    FILE* fp;
    while (curdir)
        curdir = my_close(curdir);
    fp = fopen(myfilename, "w");
    fwrite(myvhard, SIZE, 1, fp);
    fclose(fp);
    free(myvhard);
}
unsigned short findblock()
{
    unsigned short i;
    //fat* fat1, * fatptr;
    //block0* blk0;
    //blk0 = (block0*)myvhard;
    //fat1 = (fat*)(myvhard + BLOCKSIZE);
    map* wst;
    wst = (map*)(myvhard + 5 * BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    for (i = ROOTSTART * MAGNIFICATION; i < SIZE / BLOCKSIZE; i++)//@@磁盘块大小修改的相关操作
    {
        if (wst->map[i / LINE][i % LINE] == FREE)
            return i;
    }
    printf("Error,Can't find free block!\n");
    return -1;
}

int findopenfile()
{
    int i;
    for (i = 0; i < MAXTEXT; i++)
    {
        if (openfilelist[i].topenfile == 0)
            return i;
    }
    printf("Error,open too many files!\n");
    return -1;
}

void synchronization()
{
    fat* fat1, * fat2, * fatprt1, * fatprt2;
    fcb* fcbptr;
    map* wst;

    fat1 = (fat*)(myvhard + BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    fat2 = (fat*)(myvhard + 3 * BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    wst = (map*)(myvhard + 5 * BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    fatprt1 = fat1;
    fatprt2 = fat2;
    for (int i = 0;i < SIZE / BLOCKSIZE;i++)
    {
        fatprt2->id = fatprt1->id;
        fatprt1++;
        fatprt2++;
    }
}

int main()
{
    time_t now;
    struct tm* nowtime;
    int time1, time2;
    fat *fat1, *fat2,*faptr1,* faptr2;
    unsigned short num1, num2;

    char cmd[15][10] = { "cd", "mkdir", "rmdir", "ls", "create", "rm", "open", "close", "write", "read", "exit","restore" };
    char s[30], * sp;
    int cmdn, flag = 1, i;
    now = time(NULL);
    nowtime = localtime(&now);
    time2 = nowtime->tm_hour;
    time1 = nowtime->tm_hour;
    startsys();
    fat1 = (fat*)(myvhard + BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    fat2 = (fat*)(myvhard + 3 * BLOCKSIZE * MAGNIFICATION);//@@磁盘块大小修改的相关操作
    printf("*********************File System V1.0*******************************\n\n");
    printf("命令名\t\t命令参数\t\t命令说明\n\n");
    printf("cd\t\t目录名(路径名)\t\t切换当前目录到指定目录\n");
    printf("mkdir\t\t目录名\t\t\t在当前目录创建新目录\n");
    printf("rmdir\t\t目录名\t\t\t在当前目录删除指定目录\n");
    printf("ls\t\t无\t\t\t显示当前目录下的目录和文件\n");
    printf("create\t\t文件名\t\t\t在当前目录下创建指定文件\n");
    printf("rm\t\t文件名\t\t\t在当前目录下删除指定文件\n");
    printf("open\t\t文件名\t\t\t在当前目录下打开指定文件\n");
    printf("write\t\t无\t\t\t在打开文件状态下，写该文件\n");
    printf("read\t\t无\t\t\t在打开文件状态下，读取该文件\n");
    printf("close\t\t无\t\t\t在打开文件状态下，关闭该文件\n");
    printf("restore\t\t无\t\t\t回滚到之前的状态\n");
    printf("exit\t\t无\t\t\t退出系统\n\n");
    printf("*********************************************************************\n\n");
    while (flag)
    {
        time1 = nowtime->tm_hour;
        if (time2- time1 >= 1)
        {
            synchronization();
           // printf("%d", nowtime->tm_sec);
        }
        printf("%s>", openfilelist[curdir].dir);
        gets_s(s);
        time2 = nowtime->tm_hour;
        cmdn = -1;
        if (strcmp(s, ""))
        {
            sp = strtok(s, " ");
            for (i = 0; i < 15; i++)
            {
                if (strcmp(sp, cmd[i]) == 0)
                {
                    cmdn = i;
                    break;
                }
            }
            //          printf("%d\n", cmdn);  
            switch (cmdn)
            {
            case 0:
                sp = strtok(NULL, " ");
                if (sp && (openfilelist[curdir].attribute & 0x20))
                    my_cd(sp);
                else
                    printf("Please input the right command.\n");
                break;
            case 1:
                sp = strtok(NULL, " ");
                if (sp && (openfilelist[curdir].attribute & 0x20))
                    my_mkdir(sp);
                else
                    printf("Please input the right command.\n");
                break;
            case 2:
                sp = strtok(NULL, " ");
                if (sp && (openfilelist[curdir].attribute & 0x20))
                    my_rmdir(sp);
                else
                    printf("Please input the right command.\n");
                break;
            case 3:
                if (openfilelist[curdir].attribute & 0x20)
                {
                    my_ls();
                    faptr1 = fat1;
                    faptr2 = fat2;
                    /*printf("fat1\n");
                    for (int i = 0;i < 20;i++)
                    {
                        printf("%d\n", faptr1->id);
                        faptr1++;
                    }
                    printf("fat2\n");
                    for (int i = 0;i < 20;i++)
                    {
                        printf("%d\n", faptr2->id);
                        faptr2++;
                    }*/
                }
                else
                    printf("Please input the right command.\n");
                break;
            case 4:
                sp = strtok(NULL, " ");
                if (sp && (openfilelist[curdir].attribute & 0x20))
                    my_create(sp);
                else
                    printf("Please input the right command.\n");
                break;
            case 5:
                sp = strtok(NULL, " ");
                if (sp && (openfilelist[curdir].attribute & 0x20))
                    my_rm(sp);
                else
                    printf("Please input the right command.\n");
                break;
            case 6:
                sp = strtok(NULL, " ");
                if (sp && (openfilelist[curdir].attribute & 0x20))
                {
                    if (strchr(sp, '.'))//查找sp中'.'首次出现的位置  
                        curdir = my_open(sp);
                    else
                        printf("the openfile should have exname.\n");
                }
                else
                    printf("Please input the right command.\n");
                break;
            case 7:
                if (!(openfilelist[curdir].attribute & 0x20))
                    curdir = my_close(curdir);
                else
                    printf("No files opened.\n");
                break;
            case 8:
                if (!(openfilelist[curdir].attribute & 0x20))
                    my_write(curdir);
                else
                    printf("No files opened.\n");
                break;
            case 9:
                if (!(openfilelist[curdir].attribute & 0x20))
                    my_read(curdir, openfilelist[curdir].length);
                else
                    printf("No files opened.\n");
                break;
            case 10:
                if (openfilelist[curdir].attribute & 0x20)
                {
                    my_exitsys();
                    flag = 0;
                }
                else
                    printf("Please input the right command.\n");
                break;
            case 11:
                if (openfilelist[curdir].attribute & 0x20)
                {
                    restore();
                }
                else
                    printf("Please input the right command.\n");
                break;
            default:
                printf("Please input the right command.\n");
                break;
            }
        }
    }
    return 0;
}