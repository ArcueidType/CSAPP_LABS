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

int count_Hits = 0;
int count_Misses = 0;
int count_Evictions = 0;
int s, E, b;

typedef struct cacheLine{
    int bitValid;
    int tag;
    int time;
} cacheLine;

//void helpUsage(void);

cacheLine **allocateCache(int s, int E, int b){
    int setNum = 1<<s;
    int blockNum = 1<<b;
    cacheLine **cache;
    cache = (cacheLine **)malloc(setNum*sizeof(cacheLine *));
    for(int i=0;i<E;i++){
	cache[i]=(cacheLine *)malloc(blockNum*sizeof(cacheLine));
    }
    for(int i=0;i<setNum;i++){
	for(int j=0;j<E;j++){
	    cache[i][j].bitValid = 0;
	    cache[i][j].tag = -1;
	    cache[i][j].time = 0;
	}
    }
    return cache;
}

void update(unsigned long address, cacheLine **cache);

void updateTime(cacheLine **cache);

void handleTrace(FILE *fp, cacheLine **cache){
    char opt;
    unsigned long int addr;
    int size;
    while(fscanf(fp, "%c %lux,%d", &opt, &addr, &size) > 0){
	switch(opt){
	    case 'I':
		break;
	    case 'M':
		update(addr, cache);
		update(addr, cache);
		break;
	    case 'L':
		update(addr, cache);
		break;
	    case 'S':
		update(addr, cache);
		break;
	}
        updateTime(cache);
    }
    return;
}

int main(int argc, char *argv[])
{
    FILE *fp=NULL;
    int opt;
    while((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1){
	switch(opt){
	    case 'h':
		//helpUsage();
		break;
	    case 'v':
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
    cacheLine **cache = allocateCache(s, E, b);
    return 0;
}
