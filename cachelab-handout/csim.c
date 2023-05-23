/* ################################# 
 * Author:Chenxu "Arcueid" Han
 * ID: 10225101440
 * ################################# */

#include "cachelab.h"
#include<stdio.h>
#include<stdlib.h>
#include<getopt.h>
#include<unistd.h>
#include<string.h>

int countHits = 0;
int countMisses = 0;
int countEvictions = 0;
int s, E, b;
int verboseFlag = 0;
int isModifying = 0;

typedef struct cacheLine{
    int bitValid;
    unsigned long tag;
    int time;
} cacheLine, *cacheSet, **cache;

void helpUsage(void){
    printf("Usage: ./csim [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
    printf("-h: Print help info.\n");
    printf("-v: Print verbose info.\n");
    printf("-s <num>: Number of setIndex bits.\n");
    printf("-E <num>: Number of cache lines per set.\n");
    printf("-b <num>: Number of block offset bits.\n");
    printf("-t <file>: Path of trace file.\n");
}

cache allocateCache(int s, int E, int b){
    int setNum = 1<<s;
    cache myCache;
    myCache = (cache)malloc(setNum*sizeof(cacheSet));
    for(int i=0;i<setNum;i++){
	myCache[i]=(cacheSet)malloc(E*sizeof(cacheLine));
    }
    for(int i=0;i<setNum;i++){
	for(int j=0;j<E;j++){
	    myCache[i][j].bitValid = 0;
	    myCache[i][j].tag = -1lu;
	    myCache[i][j].time = 0;
	}
    }
    return myCache;
}

void loadOrStore(unsigned long address, char op, int size, cache myCache){
    int newSetIndex = (address>>b) % (1<<s);
    unsigned long newTag = address>>(s+b);
    if(verboseFlag && !isModifying) printf("%c %lx,%d", op, address, size);
    for(int i=0;i<E;i++){
	if(myCache[newSetIndex][i].tag==newTag && myCache[newSetIndex][i].bitValid==1){
	    if(verboseFlag) printf(" hit");
	    countHits++;
	    myCache[newSetIndex][i].time = 0;
	    return;
	}
    }

    countMisses++;
    if(verboseFlag) printf(" miss");
    for(int i=0;i<E;i++){
	if(myCache[newSetIndex][i].bitValid==0){
	    myCache[newSetIndex][i].bitValid = 1;
	    myCache[newSetIndex][i].tag = newTag;
	    myCache[newSetIndex][i].time = 0;
	    return;
	}
    }
    
    int evictLine=0;
    countEvictions++;
    if(verboseFlag) printf(" eviction");
    for(int i=0;i<E;i++){
	if(myCache[newSetIndex][i].time > myCache[newSetIndex][evictLine].time){
	    evictLine = i;
	}
    }
    myCache[newSetIndex][evictLine].tag = newTag;
    myCache[newSetIndex][evictLine].time = 0;
    return;
}

void modify(unsigned long address, int size, cache myCache){
    if(verboseFlag) printf("M %lx,%d", address, size);
    isModifying = 1;
    loadOrStore(address, 'L', size, myCache);
    loadOrStore(address, 'S', size, myCache);
    isModifying = 0;
    return;
}

void updateTime(cache myCache){
    int setNum = 1<<s;
    for(int i=0;i<setNum;i++){
	for(int j=0;j<E;j++){
	    if(myCache[i][j].bitValid == 1) myCache[i][j].time++;
	}
    }
    return;
}

void handleTrace(FILE *fp, cache myCache){
    char opt;
    unsigned long int addr;
    int size;
    while(fscanf(fp, "%c %lx,%d", &opt, &addr, &size) > 0){
	switch(opt){
	    case 'I':
		break;
	    case 'M':
		modify(addr, size, myCache);
		printf("\n");
		break;
	    case 'L':
		loadOrStore(addr, 'L', size, myCache);
		printf("\n");
		break;
	    case 'S':
		loadOrStore(addr, 'S', size, myCache);
		printf("\n");
		break;
	}
        updateTime(myCache);
    }
    return;
}

int main(int argc, char *argv[])
{
    FILE *fp=NULL;
    int opt;
    int helpUsageFlag=0;
    while((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1){
	switch(opt){
	    case 'h':
		helpUsageFlag=1;
		break;
	    case 'v':
		verboseFlag=1;
		break;
	    case 's':
		s = atoi(optarg);
		break;
	    case 'E':
		E = atoi(optarg);
		break;
	    case 'b':
		b = atoi(optarg);
		break;
	    case 't':
		fp = fopen(optarg, "r");
		break;
	}		
    }
    if(helpUsageFlag==1 || s<=0 || E<=0 || b<=0 || fp==NULL){
	helpUsage();
	return 0;
    }
    cache myCache = allocateCache(s, E, b);
    handleTrace(fp, myCache);
    for(int i=0;i<(1<<s);i++) free(myCache[i]);
    free(myCache);
    fclose(fp);
    printSummary(countHits, countMisses, countEvictions);
    return 0;
}
