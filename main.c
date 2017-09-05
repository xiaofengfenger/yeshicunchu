#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<time.h>


#define PAGESIZE (4)  //页面大小
#define VIRTUAL_MEMORY_SIZE (8*4)  //虚存空间大小（字节）（8个逻辑页，可定义再大一些，如64）
#define ACTUAL_MEMORY_SIZE (4*4)   //内存(实存)大小（字节）（4个物理页，可定义再大一些，如32）
#define PAGE_SUM (VIRTUAL_MEMORY_SIZE/PAGESIZE)  //总虚页数
#define BLOCK_SUM (ACTUAL_MEMORY_SIZE/PAGESIZE)  //总物理块数



#define READABLE 0x01u       //可读标识位
#define WRITABLE 0x02u       //可写标识位
#define EXECUTABLE 0x04u    //可执行标识位
#define BYTE unsigned char    //定义字节类型
typedef enum{
	TRUE=1,FALSE=0
}BOOL;
typedef enum{
	REQUEST_READ=0x01u,REQUEST_WRITE=0x02u,REQUEST_EXECUTE=0x04u}MemoryAccessRequestType;


typedef struct{
	unsigned int pageNum;  //页号
	unsigned int blockNum;  // 页面号
	BOOL effective;  // 有效位    页面是否在内存
	BYTE proType;  // 保护类型
	BOOL modified; //  修改标识
	unsigned long auxAddr;  //  页面对应外存地址
	unsigned long count;  //  页面访问次数
	int order;          //调用顺序，越大越靠后
}PageTableItem,*Ptr_PageTableItem;


typedef struct{
	MemoryAccessRequestType  reqType;
	unsigned long virAddr;
	BYTE value;
}MemoryAccessRequest, *Ptr_MemoryAccessRequest;



int a=1;
PageTableItem  pageTable[PAGE_SUM];// 页表  其中PAGE_SUM为页表项个数。pageTable的第i个页表项对应的页号为i。

BYTE actMem[ACTUAL_MEMORY_SIZE];//模拟内存空间的数组

FILE *ptr_auxMem;//模拟外存空间的文件指针

BOOL blockStatus[BLOCK_SUM];//物理块使用标识

MemoryAccessRequest ptr_memAccReq;//访存请求



void do_init();
void print_pageinfo();
void do_request();
void do_response();
void do_page_fault(Ptr_PageTableItem ptr_pageTabIt);
void do_page_in(Ptr_PageTableItem ptr_pageTabIt,unsigned int blockNum);
void do_page_out(Ptr_PageTableItem ptr_pageTabIt);
void do_LFU(Ptr_PageTableItem ptr_pageTabIt);
void do_FIFO(Ptr_PageTableItem ptr_pageTabIt);



int main(){
    int i;
    do_init();
    print_pageinfo();
    while(1){
        printf("输入1产生访问内存请求!\n");
        printf("输入0退出!\n");
        scanf("%d",&i);
        if(i==1){
            do_request();
            do_response();
            printf("访问内存后页表更新如下：\n");
            print_pageinfo();
        }
        else
            break;
    }
}



void print_pageinfo(){
	int i;
	printf("页号\t页面号\t装入\t修改\t保护\t外存地址   访问次数  写入顺序\n");   //7
	for(i=0;i<PAGE_SUM;i++){
		printf("%d\t%d\t%d\t%d\t",pageTable[i].pageNum,pageTable[i].blockNum,pageTable[i].effective,pageTable[i].modified);  //4
		switch(pageTable[i].proType){
            case READABLE:
                printf("r__\t");
                break;
            case WRITABLE:
                printf("_w_\t");
                break;
            case EXECUTABLE:
                printf("__x\t");
                break;
            case (READABLE | WRITABLE):
                printf("rw_\t");
                break;
            case (READABLE | EXECUTABLE):
                printf("r_x\t");
                break;
            case (WRITABLE | EXECUTABLE):
                printf("_wx\t");
                break;
            case (READABLE|WRITABLE| EXECUTABLE):
                printf("rwx\t");
                break;
		}
		printf("  %d\t\t%d\t%d\n",pageTable[i].auxAddr,pageTable[i].count,pageTable[i].order);
	}
}


void do_init(){
	int i,j;
	srand(time(NULL));
	for(i=0; i<PAGE_SUM; i++){
		pageTable[i].pageNum=i;
		pageTable[i].effective=FALSE;
		pageTable[i].modified=FALSE;
		pageTable[i].count=0;
		switch(rand()%7){
			case 0:
				pageTable[i].proType=READABLE;
				break;
			case 1:
				pageTable[i].proType=WRITABLE;
				break;
			case 2:
				pageTable[i].proType=EXECUTABLE;
				break;
			case 3:
				pageTable[i].proType=READABLE | WRITABLE;
				break;
			case 4:
				pageTable[i].proType=READABLE | EXECUTABLE;
				break;
			case 5:
				pageTable[i].proType=WRITABLE | EXECUTABLE;
				break;
			case 6:
				pageTable[i].proType=READABLE|WRITABLE| EXECUTABLE;
				break;
			default:
				break;
		}
		pageTable[i].auxAddr = i*PAGESIZE;//逻辑页的外存地址
		pageTable[i].blockNum=-1;
		pageTable[i].order=0;
	}
	for (j=0; j<BLOCK_SUM; j++){//都不装入
			blockStatus[j]=FALSE;
	}




	/*if ( !(ptr_auxMem = fopen("VirtualMem", "wb+")) ) {
        printf("cuowu");
        exit(0);
    }
    BYTE buffer[PAGESIZE];
    for(j=0; j<PAGE_SUM; j++) {
        for (i=0; i<PAGESIZE; i++) {
            buffer[i] = rand()%0xFFu;
        }
        fwrite(buffer,sizeof(BYTE), PAGESIZE, ptr_auxMem);
    }
    fclose(ptr_auxMem);*/
}


void do_request(){      //初始化访存请求
	ptr_memAccReq.virAddr=rand()%VIRTUAL_MEMORY_SIZE;
	ptr_memAccReq.value=rand();
	switch(ptr_memAccReq.virAddr%3){
		case 0:
		ptr_memAccReq.reqType=REQUEST_READ;
		printf("产生请求:\n 类型：读取  地址为%d\t\n",ptr_memAccReq.virAddr);
		break;
		case 1:
		ptr_memAccReq.reqType=REQUEST_WRITE;
		printf("产生请求:\n 类型：写入   地址为%d\t值为：%d\n",ptr_memAccReq.virAddr,ptr_memAccReq.value);
		break;
		case 2:
		ptr_memAccReq.reqType=REQUEST_EXECUTE;
		printf("产生请求:\n 类型：执行  地址为%d\t\n",ptr_memAccReq.virAddr);
		break;
		default :
		break;
	}
}


void do_response(){
	Ptr_PageTableItem ptr_pageTabIt;
	unsigned int pageNum,offAddr;
	unsigned int actAddr;
	if(ptr_memAccReq.virAddr<0||ptr_memAccReq.virAddr>=VIRTUAL_MEMORY_SIZE){
		printf("地址错误\n");
		return;
	}
	pageNum =ptr_memAccReq.virAddr/PAGESIZE;   //页号取整
	offAddr=ptr_memAccReq.virAddr%PAGESIZE;      //页内偏移取余
	printf("页号：%u\t  页内偏移：%u\n\n",pageNum,offAddr);
	ptr_pageTabIt=&pageTable[pageNum];
	if(!ptr_pageTabIt->effective){         //是否在内存
		do_page_fault(ptr_pageTabIt);  //处理缺页中断
	}
	else{
		printf("页面在内存当中，不需要调页\n");
	}
	actAddr=ptr_pageTabIt->blockNum*PAGESIZE+offAddr;
	printf("在内存中实际地址为：%u\n",actAddr);
	switch(ptr_memAccReq.reqType){
		case REQUEST_READ:
		ptr_pageTabIt->count++;     //访问次数加一
		ptr_pageTabIt->order=a;
		a++;
		if(!(ptr_memAccReq.reqType&READABLE)){  //访问请求是不是可读
			printf("不允许进行读操作\n\n");
			return;
		}
		printf("读取操作成功\n值为:%x\n\n",actMem[actAddr]);
		break;
		case REQUEST_WRITE:
		ptr_pageTabIt->count++;   //访问次数加一
		ptr_pageTabIt->order=a;
		a++;
		if(!(ptr_memAccReq.reqType&WRITABLE)){  //访问请求是不是可写
			printf("不允许进行写操作\n\n");
			return;
		}
		printf("写入操作成功\n\n");
		actMem[actAddr]=ptr_memAccReq.value;
		break;
		case REQUEST_EXECUTE:
		ptr_pageTabIt->count++;   //访问次数加一
		ptr_pageTabIt->order=a;
		a++;
		if(!(ptr_memAccReq.reqType&EXECUTABLE)){  //访问请求是不是可执行
			printf("不允许进行运行行操作\n\n");
			return;
		}
		printf("运行操作成功\n");
		break;
		default:
		printf("error");
	}
}



void do_page_fault(Ptr_PageTableItem ptr_pageTabIt){
    int i;
	printf("页面不在内存中\n产生缺页中断，开始进行调页。。\n");
	for(i=0;i<BLOCK_SUM;i++){
		if(!blockStatus[i]){    //有空闲页面
			printf("有空闲页面\n");
			do_page_in(ptr_pageTabIt,i);
			ptr_pageTabIt->blockNum=i;
			ptr_pageTabIt->effective=TRUE;
			ptr_pageTabIt->modified=FALSE;
			ptr_pageTabIt->count=0;
			blockStatus[i]=TRUE;
			return;
		}
	}
	printf("无空闲页面，请选择页面置换方式\n");
	printf("1.LFU\t2.FIFO\n");
	scanf("%d",&i);
	switch(i){
		case 1:
            do_LFU(ptr_pageTabIt);
            break;
		case 2:
            do_FIFO(ptr_pageTabIt);
            break;
        default:
             printf("choose error");

	}
}

void do_LFU(Ptr_PageTableItem ptr_pageTabIt){
	unsigned int i,j,min,page;
	printf("开始LFU页面置换\n");
	for(i=0,min=0xFFFFFFFF,page=0;i<PAGE_SUM;i++){
		if(pageTable[i].count<min && pageTable[i].effective==TRUE){
			min=pageTable[i].count;
			j=pageTable[i].blockNum;   //获得要置换的物理块号
			page=i;
		}
	}
	printf("选择页面%u进行置换\n",j);
	if(pageTable[page].modified){
		printf("页面有修改，写回外存\n");
		do_page_out(&pageTable[page]);
	}
	pageTable[page].effective=FALSE;
	pageTable[page].count=0;
	pageTable[page].modified=FALSE;
	pageTable[page].blockNum=0;
	do_page_in(ptr_pageTabIt,j);
	ptr_pageTabIt->blockNum=j;    //修改调入页的页表项
	ptr_pageTabIt->effective=TRUE;
	ptr_pageTabIt->count=1;
	printf("页面置换成功\n");
}


void do_FIFO(Ptr_PageTableItem ptr_pageTabIt){
    unsigned int i,j,min,page;
    printf("开始进行FIFO页面置换算法");
    for(i=0,min=0xFFFFFFFF,page=0;i<PAGE_SUM;i++){
		if(pageTable[i].order<min && pageTable[i].effective==TRUE){
			min=pageTable[i].order;
			j=pageTable[i].blockNum;   //获得要置换的物理块号
			page=i;
		}
	}
    printf("选择页面%u进行置换\n",j);
	if(pageTable[page].modified){
		printf("页面有修改，写回外存\n");
		do_page_out(&pageTable[page]);
	}
	pageTable[page].effective=FALSE;
	pageTable[page].count=0;
	pageTable[page].modified=FALSE;
	pageTable[page].blockNum=0;
	do_page_in(ptr_pageTabIt,j);
	ptr_pageTabIt->blockNum=j;    //修改调入页的页表项
	ptr_pageTabIt->effective=TRUE;
	ptr_pageTabIt->count=1;
	printf("页面置换成功\n");
}




void do_page_in(Ptr_PageTableItem ptr_pageTabIt,unsigned int blockNum){
	int readNum;
	if(!(ptr_auxMem=fopen("VirtualMem","rb"))){
        printf("in_open file error");
        exit(0);
	}
	if(fseek(ptr_auxMem, ptr_pageTabIt->auxAddr, SEEK_SET)){
        printf("in_fseek file error");
        exit(0);
	}
	if(!(readNum=fread(actMem+blockNum*PAGESIZE, sizeof(BYTE), PAGESIZE, ptr_auxMem))){
        printf("in_read error");
        exit(0);
	}
	fclose(ptr_auxMem);
}

void do_page_out(Ptr_PageTableItem ptr_pageTabIt){
    int writeNum;
	if(!(ptr_auxMem=fopen("VirtualMem","r+"))){
        printf("out_open file error");
        return;
	}
	if(fseek(ptr_auxMem, ptr_pageTabIt->auxAddr, SEEK_SET)){
        printf("out_fseek file error");
        return;
	}
	if(!(writeNum=fwrite(actMem + ptr_pageTabIt->blockNum * PAGESIZE, sizeof(BYTE), PAGESIZE, ptr_auxMem))){
        printf("in_write error");
        return;
	}
	fclose(ptr_auxMem);
}
