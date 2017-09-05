#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<time.h>


#define PAGESIZE (4)  //ҳ���С
#define VIRTUAL_MEMORY_SIZE (8*4)  //���ռ��С���ֽڣ���8���߼�ҳ���ɶ����ٴ�һЩ����64��
#define ACTUAL_MEMORY_SIZE (4*4)   //�ڴ�(ʵ��)��С���ֽڣ���4������ҳ���ɶ����ٴ�һЩ����32��
#define PAGE_SUM (VIRTUAL_MEMORY_SIZE/PAGESIZE)  //����ҳ��
#define BLOCK_SUM (ACTUAL_MEMORY_SIZE/PAGESIZE)  //���������



#define READABLE 0x01u       //�ɶ���ʶλ
#define WRITABLE 0x02u       //��д��ʶλ
#define EXECUTABLE 0x04u    //��ִ�б�ʶλ
#define BYTE unsigned char    //�����ֽ�����
typedef enum{
	TRUE=1,FALSE=0
}BOOL;
typedef enum{
	REQUEST_READ=0x01u,REQUEST_WRITE=0x02u,REQUEST_EXECUTE=0x04u}MemoryAccessRequestType;


typedef struct{
	unsigned int pageNum;  //ҳ��
	unsigned int blockNum;  // ҳ���
	BOOL effective;  // ��Чλ    ҳ���Ƿ����ڴ�
	BYTE proType;  // ��������
	BOOL modified; //  �޸ı�ʶ
	unsigned long auxAddr;  //  ҳ���Ӧ����ַ
	unsigned long count;  //  ҳ����ʴ���
	int order;          //����˳��Խ��Խ����
}PageTableItem,*Ptr_PageTableItem;


typedef struct{
	MemoryAccessRequestType  reqType;
	unsigned long virAddr;
	BYTE value;
}MemoryAccessRequest, *Ptr_MemoryAccessRequest;



int a=1;
PageTableItem  pageTable[PAGE_SUM];// ҳ��  ����PAGE_SUMΪҳ���������pageTable�ĵ�i��ҳ�����Ӧ��ҳ��Ϊi��

BYTE actMem[ACTUAL_MEMORY_SIZE];//ģ���ڴ�ռ������

FILE *ptr_auxMem;//ģ�����ռ���ļ�ָ��

BOOL blockStatus[BLOCK_SUM];//�����ʹ�ñ�ʶ

MemoryAccessRequest ptr_memAccReq;//�ô�����



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
        printf("����1���������ڴ�����!\n");
        printf("����0�˳�!\n");
        scanf("%d",&i);
        if(i==1){
            do_request();
            do_response();
            printf("�����ڴ��ҳ��������£�\n");
            print_pageinfo();
        }
        else
            break;
    }
}



void print_pageinfo(){
	int i;
	printf("ҳ��\tҳ���\tװ��\t�޸�\t����\t����ַ   ���ʴ���  д��˳��\n");   //7
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
		pageTable[i].auxAddr = i*PAGESIZE;//�߼�ҳ������ַ
		pageTable[i].blockNum=-1;
		pageTable[i].order=0;
	}
	for (j=0; j<BLOCK_SUM; j++){//����װ��
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


void do_request(){      //��ʼ���ô�����
	ptr_memAccReq.virAddr=rand()%VIRTUAL_MEMORY_SIZE;
	ptr_memAccReq.value=rand();
	switch(ptr_memAccReq.virAddr%3){
		case 0:
		ptr_memAccReq.reqType=REQUEST_READ;
		printf("��������:\n ���ͣ���ȡ  ��ַΪ%d\t\n",ptr_memAccReq.virAddr);
		break;
		case 1:
		ptr_memAccReq.reqType=REQUEST_WRITE;
		printf("��������:\n ���ͣ�д��   ��ַΪ%d\tֵΪ��%d\n",ptr_memAccReq.virAddr,ptr_memAccReq.value);
		break;
		case 2:
		ptr_memAccReq.reqType=REQUEST_EXECUTE;
		printf("��������:\n ���ͣ�ִ��  ��ַΪ%d\t\n",ptr_memAccReq.virAddr);
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
		printf("��ַ����\n");
		return;
	}
	pageNum =ptr_memAccReq.virAddr/PAGESIZE;   //ҳ��ȡ��
	offAddr=ptr_memAccReq.virAddr%PAGESIZE;      //ҳ��ƫ��ȡ��
	printf("ҳ�ţ�%u\t  ҳ��ƫ�ƣ�%u\n\n",pageNum,offAddr);
	ptr_pageTabIt=&pageTable[pageNum];
	if(!ptr_pageTabIt->effective){         //�Ƿ����ڴ�
		do_page_fault(ptr_pageTabIt);  //����ȱҳ�ж�
	}
	else{
		printf("ҳ�����ڴ浱�У�����Ҫ��ҳ\n");
	}
	actAddr=ptr_pageTabIt->blockNum*PAGESIZE+offAddr;
	printf("���ڴ���ʵ�ʵ�ַΪ��%u\n",actAddr);
	switch(ptr_memAccReq.reqType){
		case REQUEST_READ:
		ptr_pageTabIt->count++;     //���ʴ�����һ
		ptr_pageTabIt->order=a;
		a++;
		if(!(ptr_memAccReq.reqType&READABLE)){  //���������ǲ��ǿɶ�
			printf("��������ж�����\n\n");
			return;
		}
		printf("��ȡ�����ɹ�\nֵΪ:%x\n\n",actMem[actAddr]);
		break;
		case REQUEST_WRITE:
		ptr_pageTabIt->count++;   //���ʴ�����һ
		ptr_pageTabIt->order=a;
		a++;
		if(!(ptr_memAccReq.reqType&WRITABLE)){  //���������ǲ��ǿ�д
			printf("���������д����\n\n");
			return;
		}
		printf("д������ɹ�\n\n");
		actMem[actAddr]=ptr_memAccReq.value;
		break;
		case REQUEST_EXECUTE:
		ptr_pageTabIt->count++;   //���ʴ�����һ
		ptr_pageTabIt->order=a;
		a++;
		if(!(ptr_memAccReq.reqType&EXECUTABLE)){  //���������ǲ��ǿ�ִ��
			printf("��������������в���\n\n");
			return;
		}
		printf("���в����ɹ�\n");
		break;
		default:
		printf("error");
	}
}



void do_page_fault(Ptr_PageTableItem ptr_pageTabIt){
    int i;
	printf("ҳ�治���ڴ���\n����ȱҳ�жϣ���ʼ���е�ҳ����\n");
	for(i=0;i<BLOCK_SUM;i++){
		if(!blockStatus[i]){    //�п���ҳ��
			printf("�п���ҳ��\n");
			do_page_in(ptr_pageTabIt,i);
			ptr_pageTabIt->blockNum=i;
			ptr_pageTabIt->effective=TRUE;
			ptr_pageTabIt->modified=FALSE;
			ptr_pageTabIt->count=0;
			blockStatus[i]=TRUE;
			return;
		}
	}
	printf("�޿���ҳ�棬��ѡ��ҳ���û���ʽ\n");
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
	printf("��ʼLFUҳ���û�\n");
	for(i=0,min=0xFFFFFFFF,page=0;i<PAGE_SUM;i++){
		if(pageTable[i].count<min && pageTable[i].effective==TRUE){
			min=pageTable[i].count;
			j=pageTable[i].blockNum;   //���Ҫ�û���������
			page=i;
		}
	}
	printf("ѡ��ҳ��%u�����û�\n",j);
	if(pageTable[page].modified){
		printf("ҳ�����޸ģ�д�����\n");
		do_page_out(&pageTable[page]);
	}
	pageTable[page].effective=FALSE;
	pageTable[page].count=0;
	pageTable[page].modified=FALSE;
	pageTable[page].blockNum=0;
	do_page_in(ptr_pageTabIt,j);
	ptr_pageTabIt->blockNum=j;    //�޸ĵ���ҳ��ҳ����
	ptr_pageTabIt->effective=TRUE;
	ptr_pageTabIt->count=1;
	printf("ҳ���û��ɹ�\n");
}


void do_FIFO(Ptr_PageTableItem ptr_pageTabIt){
    unsigned int i,j,min,page;
    printf("��ʼ����FIFOҳ���û��㷨");
    for(i=0,min=0xFFFFFFFF,page=0;i<PAGE_SUM;i++){
		if(pageTable[i].order<min && pageTable[i].effective==TRUE){
			min=pageTable[i].order;
			j=pageTable[i].blockNum;   //���Ҫ�û���������
			page=i;
		}
	}
    printf("ѡ��ҳ��%u�����û�\n",j);
	if(pageTable[page].modified){
		printf("ҳ�����޸ģ�д�����\n");
		do_page_out(&pageTable[page]);
	}
	pageTable[page].effective=FALSE;
	pageTable[page].count=0;
	pageTable[page].modified=FALSE;
	pageTable[page].blockNum=0;
	do_page_in(ptr_pageTabIt,j);
	ptr_pageTabIt->blockNum=j;    //�޸ĵ���ҳ��ҳ����
	ptr_pageTabIt->effective=TRUE;
	ptr_pageTabIt->count=1;
	printf("ҳ���û��ɹ�\n");
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
