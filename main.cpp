#include <stdio.h>
#include <malloc.h>  
#include <string.h>  
#include <time.h>  

#define BLOCKSIZE 1024  // ���̿��С  
#define SIZE 1024000  // ������̿ռ��С  
#define END 65535  // FAT�е��ļ�������־  
#define FREE 0  // ���̿顢FAT���̿���б�־  
#define BUSY 1  //���̿�ռ�ñ�־
#define ROOTBLOCKNUM 2  // ��Ŀ¼����ռ�̿���  
#define MAXOPENFILE 10  // ���ͬʱ���ļ�����t  
#define MAXTEXT 10000  

#define FIXEDSIZE 1024     
#define MAGNIFICATION FIXEDSIZE/BLOCKSIZE   //����
#define LINE (100 * MAGNIFICATION)    //��
#define ROW SIZE / LINE /BLOCKSIZE   //��
#define ROOTSTART (SIZE /FIXEDSIZE * 2 / BLOCKSIZE )+ 6

#pragma warning(disable:4996)

/* �ļ����ƿ� */
typedef struct FCB
{
    char filename[8];  // �ļ���  
    char exname[3];  // �ļ���չ��  
    unsigned char attribute;  // �ļ������ֶΣ�ֵΪ0ʱ��ʾĿ¼�ļ���ֵΪ1ʱ��ʾ�����ļ�  
    unsigned short time;  // �ļ�����ʱ��  
    unsigned short date;  // �ļ���������  
    unsigned short first;  // �ļ���ʼ�̿��  
    unsigned long length;  // �ļ�����  
    char free;  // ��ʾĿ¼���Ƿ�Ϊ�գ���ֵΪ0����ʾ�գ�ֵΪ1����ʾ�ѷ���  


}fcb;

/* �ļ������ */
typedef struct FAT
{
    unsigned short id;  // ���̿��״̬��END����һ����  
}fat;


/* �û����ļ��� */
typedef struct USEROPEN
{
    char filename[8];  // �ļ���  
    char exname[3];  // �ļ���չ��  
    unsigned char attribute;  // �ļ������ֶΣ�ֵΪ0ʱ��ʾĿ¼�ļ���ֵΪ1ʱ��ʾ�����ļ�  
    unsigned short time;  // �ļ�����ʱ��  
    unsigned short date;  // �ļ���������  
    unsigned short first;  // �ļ���ʼ�̿��  
    unsigned long length;  // �ļ����ȣ��������ļ����ֽ�������Ŀ¼�ļ�������Ŀ¼�������  
    char free;  // ��ʾĿ¼���Ƿ�Ϊ�գ���ֵΪ0����ʾ�գ�ֵΪ1����ʾ�ѷ���  

    unsigned short dirno;  // ��Ӧ���ļ���Ŀ¼���ڸ�Ŀ¼�ļ��е��̿��  
    int diroff;  // ��Ӧ���ļ���Ŀ¼���ڸ�Ŀ¼�ļ���dirno�̿��е�Ŀ¼�����  
    char dir[80];  // ��Ӧ���ļ����ڵ�Ŀ¼��������������ټ���ָ���ļ��Ƿ��Ѿ���  
    int father;  // ��Ŀ¼�ڴ��ļ������λ��  
    int count;  // ��дָ�����ļ��е�λ��,�ļ������ַ���  
    char fcbstate;  // �Ƿ��޸����ļ���FCB�����ݣ�����޸�����Ϊ1������Ϊ0  
    char topenfile;  // ��ʾ���û��򿪱����Ƿ�Ϊ�գ���ֵΪ0����ʾΪ�գ������ʾ�ѱ�ĳ���ļ�ռ��  
}useropen;

/* ������ */
typedef struct BLOCK0
{
    char magic[10];  // �ļ�ϵͳħ��  
    char information[200];  // �洢һЩ������Ϣ������̿��С�����̿������������ļ�����  
    unsigned short root;  // ��Ŀ¼�ļ�����ʼ�̿��  
    unsigned char* startblock;  // �����������������ʼλ��  
}block0;

/*λʾͼ*/
typedef struct MAP
{
    unsigned short int map[ROW][LINE];//λʾͼ
}map;


unsigned char* myvhard;  // ָ��������̵���ʼ��ַ  
useropen openfilelist[MAXOPENFILE];  // �û����ļ�������  
int curdir;  // �û����ļ����еĵ�ǰĿ¼���ڴ��ļ������λ��  
char currentdir[80];  // ��¼��ǰĿ¼��Ŀ¼��������Ŀ¼��·����  
unsigned char* startp;  // ��¼�����������������ʼλ��  
char myfilename[] = "myfilesys";//�ļ�ϵͳ���ļ���  

void startsys();  // �����ļ�ϵͳ  
void my_format();  // ���̸�ʽ��  
void my_cd(char* dirname);  // ���ĵ�ǰĿ¼  
void my_mkdir(char* dirname);  // ������Ŀ¼  
void my_rmdir(char* dirname);  // ɾ����Ŀ¼  
void my_ls();  // ��ʾĿ¼  
void my_create(char* filename);  // �����ļ�  
void my_rm(char* filename);  // ɾ���ļ�  
int my_open(char* filename);  // ���ļ�  
int my_close(int fd);  // �ر��ļ�  
int my_write(int fd);  // д�ļ�  
int do_write(int fd, char* text, int len, char wstyle);  // ʵ��д�ļ�  
int my_read(int fd, int len);  // ���ļ�  
int do_read(int fd, int len, char* text);  // ʵ�ʶ��ļ�  
void my_exitsys();  // �˳��ļ�ϵͳ  
unsigned short findblock();  // Ѱ�ҿ����̿�  
int findopenfile();  // Ѱ�ҿ����ļ�����  
void restore(); //�ָ�ɾ�����ļ�
void synchronization(); //�̶�ʱ�䱸��fat2��


void startsys()
{
    FILE* fp;
    unsigned char buf[SIZE];
    fcb* root;
    int i;
    myvhard = (unsigned char*)malloc(SIZE);//����������̿ռ�  
    memset(myvhard, 0, SIZE);//��myvhard��ǰSIZE���ֽ��� 0 �滻������ myvhard  
    if ((fp = fopen(myfilename, "r")) != NULL)
    {
        fread(buf, SIZE, 1, fp);//���������ļ���ȡ��������  
        fclose(fp);//�رմ򿪵��ļ�������������д���ļ����ͷ�ϵͳ�ṩ�ļ���Դ  
        if (strcmp(((block0*)buf)->magic, "10101010"))//�жϿ�ʼ��8���ֽ������Ƿ�Ϊ�ļ�ϵͳħ��  
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
    root = (fcb*)(myvhard + ROOTSTART * BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
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
    fat1 = (fat*)(myvhard + BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
    fat2 = (fat*)(myvhard + 3 * BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
    wst = (map*)(myvhard + 5 * BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
    root = (fcb*)(myvhard + ROOTSTART * BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
    strcpy(blk0->magic, "10101010");
    strcpy(blk0->information, "My FileSystem Ver 1.0 \n Blocksize=1KB Whole size=1000KB Blocknum=1000 RootBlocknum=2\n");
    blk0->root = ROOTSTART * MAGNIFICATION;//@@���̿��С�޸ĵ���ز���
    blk0->startblock = (unsigned char*)root;
    for (i = 0; i < blk0->root; i++)//@@���̿��С�޸ĵ���ز���
    {//blk0��fat1��fat2��wst�̿�ռ��
        wst->map[ i / LINE ][ i % LINE ] = BUSY;
        fat1->id = END;
        fat2->id = END;
        fat1++;
        fat2++;
    }


    for (i = blk0->root;i < (ROOTSTART+1) * MAGNIFICATION; i++) //@@���̿��С�޸ĵ���ز���
    {//root�µ�"."ռ��ROOTSTART * (FIXEDSIZE / BLOCKSIZE)��
        fat1->id = i + 1;
        fat2->id = i + 1;
        wst->map[i / LINE][i % LINE] = BUSY;
        fat1++;
        fat2++;
    }
    fat1->id = (ROOTSTART + 1) * MAGNIFICATION;
    fat2->id = (ROOTSTART + 1) * MAGNIFICATION;//ָ��root�µ�".."���ڵĿ�


    //fat1->id = 6;//root�µ�"."
    //fat2->id = 6;
    //blk0->map[0][5] = END;
    //fat1++;
    //fat2++;
    for (i = (ROOTSTART+1) * MAGNIFICATION;i < (ROOTSTART+2) * MAGNIFICATION; i++) //@@���̿��С�޸ĵ���ز���
    {//root�µ�".."ռ��(ROOTSTART+1) * (FIXEDSIZE / BLOCKSIZE)��
        fat1->id = i + 1;
        fat2->id = i + 1;
        wst->map[i / LINE][i % LINE] = BUSY;
        fat1++;
        fat2++;
    }
    fat1->id = END;
    fat2->id = END;//root���ڵĿ����

    //fat1->id = END;//root�µ�".."
    //fat2->id = END;
    //blk0->map[0][6] = END;
    //fat1++;
    //fat2++;
    for (i = (ROOTSTART + 2) * MAGNIFICATION; i < SIZE / BLOCKSIZE; i++)//@@���̿��С�޸ĵ���ز���
    {//��ʼ������������п�����
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

    fat1 = (fat*)(myvhard + BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
    fat2 = (fat*)(myvhard + 3 * BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
    wst = (map*)(myvhard + 5 * BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
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
    dir = strtok(dirname, "\\");//�ֽ��ַ���Ϊһ���ַ�����dirnameΪҪ�ֽ���ַ�����"\\"Ϊ�ָ����ַ���  
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
    fat1 = (fat*)(myvhard + BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
    fat2 = (fat*)(myvhard + 3 * BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
    wst = (map*)(myvhard + 5 * BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
    openfilelist[curdir].count = 0;
    rbn = do_read(curdir, openfilelist[curdir].length, text);
    fcbptr = (fcb*)text;
    for (i = 0;i < rbn / sizeof(fcb); i++)//�ڵ�ǰĿ¼���ң��Ƿ�������Ŀ¼  
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

   
    blkno = findblock();//Ѱ�ҿ����̿�  
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

    fd = my_open(dirname);//������Ŀ¼��'.','..'Ŀ¼  
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
    fat1 = (fat*)(myvhard + BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
    fat2 = (fat*)(myvhard + 3 * BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
    wst = (map*)(myvhard + 5 * BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
    if (strcmp(dirname, ".") == 0 || strcmp(dirname, "..") == 0)
    {
        printf("Error,can't remove this directory.\n");
        return;
    }
    openfilelist[curdir].count = 0;
    rbn = do_read(curdir, openfilelist[curdir].length, text);
    fcbptr = (fcb*)text;
    for (i = 0; i < rbn / sizeof(fcb); i++)//����Ҫɾ����Ŀ¼  
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
    for (j = 0; j < rbn2 / sizeof(fcb); j++)//�ж�Ҫɾ��Ŀ¼�Ƿ�Ϊ��  
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
    fat1 = (fat*)(myvhard + BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
    fat2 = (fat*)(myvhard + 3 * BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
    wst = (map*)(myvhard + 5 * BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
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
    fat1 = (fat*)(myvhard + BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
    fat2 = (fat*)(myvhard + 3 * BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
    wst = (map*)(myvhard + 5 * BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
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
    fat1 = (fat*)(myvhard + BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
    fat2 = (fat*)(myvhard + 3 * BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
    wst = (map*)(myvhard + 5 * BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
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
    fat1 = (fat*)(myvhard + BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
    fat2 = (fat*)(myvhard + 3 * BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
    wst = (map*)(myvhard + 5 * BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
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
    while (blkoff >= BLOCKSIZE)//�ҵ���Ŀ¼�Ľ����̿�
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
        blkptr = (unsigned char*)(myvhard + blkno * BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
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
    fat1 = (fat*)(myvhard + BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
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
        blkptr = (unsigned char*)(myvhard + blkno * BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
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
    wst = (map*)(myvhard + 5 * BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
    for (i = ROOTSTART * MAGNIFICATION; i < SIZE / BLOCKSIZE; i++)//@@���̿��С�޸ĵ���ز���
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

    fat1 = (fat*)(myvhard + BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
    fat2 = (fat*)(myvhard + 3 * BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
    wst = (map*)(myvhard + 5 * BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
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
    fat1 = (fat*)(myvhard + BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
    fat2 = (fat*)(myvhard + 3 * BLOCKSIZE * MAGNIFICATION);//@@���̿��С�޸ĵ���ز���
    printf("*********************File System V1.0*******************************\n\n");
    printf("������\t\t�������\t\t����˵��\n\n");
    printf("cd\t\tĿ¼��(·����)\t\t�л���ǰĿ¼��ָ��Ŀ¼\n");
    printf("mkdir\t\tĿ¼��\t\t\t�ڵ�ǰĿ¼������Ŀ¼\n");
    printf("rmdir\t\tĿ¼��\t\t\t�ڵ�ǰĿ¼ɾ��ָ��Ŀ¼\n");
    printf("ls\t\t��\t\t\t��ʾ��ǰĿ¼�µ�Ŀ¼���ļ�\n");
    printf("create\t\t�ļ���\t\t\t�ڵ�ǰĿ¼�´���ָ���ļ�\n");
    printf("rm\t\t�ļ���\t\t\t�ڵ�ǰĿ¼��ɾ��ָ���ļ�\n");
    printf("open\t\t�ļ���\t\t\t�ڵ�ǰĿ¼�´�ָ���ļ�\n");
    printf("write\t\t��\t\t\t�ڴ��ļ�״̬�£�д���ļ�\n");
    printf("read\t\t��\t\t\t�ڴ��ļ�״̬�£���ȡ���ļ�\n");
    printf("close\t\t��\t\t\t�ڴ��ļ�״̬�£��رո��ļ�\n");
    printf("restore\t\t��\t\t\t�ع���֮ǰ��״̬\n");
    printf("exit\t\t��\t\t\t�˳�ϵͳ\n\n");
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
                    if (strchr(sp, '.'))//����sp��'.'�״γ��ֵ�λ��  
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