#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "cachelab.h"

#define MAX_STRLENTH 100

int DETAIL_V = 0;

/*define cache row,cache block is not used in this program*/
typedef struct {
	time_t lastUse;
	unsigned int setBits;
	unsigned int validBits;
	unsigned int signBits;
} Cache_E;

/*define cache group*/
typedef struct {
	Cache_E *cePtr;
} Cache_S;

typedef struct {
	unsigned int ng_s;
	unsigned int nr_E;
	unsigned int nb_b;
	char *fileName;
} Rtn_CacheSrt;

typedef struct {
	char opt_C;
	unsigned int opt_Adr;
} Rtn_FileSrt;

typedef struct {
	unsigned int nh_hit;
	unsigned int nm_miss;
	unsigned int ne_evic;
} Rtn_CacheSts;

Rtn_CacheSrt parseCommandLine(int argc, char *argv[]);
Rtn_FileSrt parseFileLine(char *line);
Cache_E transfromAdr(Rtn_CacheSrt srt,unsigned int opt_Adr);
Rtn_CacheSts executeLoadData(Cache_S *cache, Rtn_CacheSrt srt, unsigned int opt_Adr);
Rtn_CacheSts excuteSaveData(Cache_S *cache, Rtn_CacheSrt srt, unsigned int opt_Adr);
Rtn_CacheSts excuteModifyData(Cache_S *cache, Rtn_CacheSrt srt, unsigned int opt_Adr);

void debug(Rtn_CacheSrt srt, Rtn_FileSrt fileSrt){
	printf("Set:%u, E:%u, b:%u, fileName:%s, opt_C:%c, opt_Adr:0x%x(%d)\t",
			srt.ng_s,srt.nr_E,srt.nb_b,srt.fileName,
			fileSrt.opt_C,fileSrt.opt_Adr,fileSrt.opt_Adr);
	Cache_E cache_E = transfromAdr(srt, fileSrt.opt_Adr);
	printf("cache address:%u,%u\t",cache_E.validBits,cache_E.setBits);
	printf("\n");
	return ;
}

void command_help(char *argv0){
	printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv0);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\n");
    printf("Examples:\n");
    printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv0);
    printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv0);

	return ;
}


int main(int argc, char *argv[])
{
	Rtn_CacheSrt srt = parseCommandLine(argc,argv);

	unsigned int ng_S = pow(2,srt.ng_s);
	/*define cache group array*/
	Cache_S *cache = (Cache_S *)malloc(sizeof(Cache_S)*ng_S);
	/*define cache row in all cache group*/
	for (int i = 0; i < ng_S; i++){
		cache[i].cePtr = (Cache_E *)malloc(sizeof(Cache_S)*srt.nr_E);
		for (int j = 0; j < srt.nr_E; j++){
			cache[i].cePtr[j].validBits=0;
		}
	}

	FILE *file;
	char line[MAX_STRLENTH];
	file = fopen(srt.fileName,"r");
	if (file == NULL){
		perror("file open fail");
		return 1;
	}

	unsigned int ans_hit = 0;
	unsigned int ans_miss = 0;
	unsigned int ans_evic = 0;
	Rtn_CacheSts status;
	while (fgets(line,sizeof(line),file) != NULL){
		Rtn_FileSrt opts = parseFileLine(line);
		//debug(srt,opts);
		switch (opts.opt_C)
		{
		case 'L':
			status = executeLoadData(cache,srt,opts.opt_Adr);
			break;
		case 'S':
			status = excuteSaveData(cache,srt,opts.opt_Adr);
			break;
		case 'M':
			status = excuteModifyData(cache,srt,opts.opt_Adr);
			break;
		default:
			printf("Error in read file %s\n",srt.fileName);
			exit(1);
			break;
		}
		ans_hit += status.nh_hit;
		ans_miss += status.nm_miss;
		ans_evic += status.ne_evic;
	} 

    //printSummary(ans_hit, ans_miss, ans_evic);
	
	/*free memory from malloc*/
	for (int i = 0; i < ng_S; i++){
		free(cache[i].cePtr);
	}
	free(cache);
    return 0;
}

Rtn_CacheSrt parseCommandLine(int argc, char *argv[]){
	const char *optstr = "hvs:E:b:t:";
	char ch;
	Rtn_CacheSrt srt;
	while ((ch = getopt(argc,argv,optstr)) != -1){
		switch (ch)
		{
		case 'h':
			command_help(argv[0]);
			exit(0);
			break;
		case 'v':
			DETAIL_V = 1;
			break;
		case 's':
			srt.ng_s = atoi(optarg);
			break;
		case 'E':
			srt.nr_E = atoi(optarg);
			break;
		case 'b':
			srt.nb_b = atoi(optarg);
			break;
		case 't':
			srt.fileName = optarg;
			break;
		case '?':
			command_help(argv[0]);
			exit(1);
			break;
		}
	}
	return srt;
}
Rtn_FileSrt parseFileLine(char *line){
	Rtn_FileSrt fileSrt;
	
	fileSrt.opt_C = line[1];

	unsigned int len = 0;
	for (int i = 3; i < strlen(line); i++){
		if (line[i] != ','){
			len++;
		}
		else {
			break;
		}
	}

	char *adrPtr=(char *)malloc(len+1);
	strncpy(adrPtr,line+3,len);
	adrPtr[len]='\0';

	sscanf(adrPtr,"%x",&fileSrt.opt_Adr);
	free(adrPtr);

	return fileSrt;
}

Cache_E transfromAdr(Rtn_CacheSrt srt,unsigned int opt_Adr){
	Cache_E cache_E;
	/*to get validBits, I need remove b bit in Adr(opt_Adr>>b)
	 *then get set bit from bottom(opt_Adr%pow(2,s))
	*/
	opt_Adr >>= srt.nb_b;
	unsigned int t = pow(2,srt.ng_s);
	cache_E.setBits = opt_Adr%t;
	cache_E.validBits = opt_Adr >> srt.ng_s;
	return cache_E;
}

Rtn_CacheSts executeLoadData(Cache_S *cache, Rtn_CacheSrt srt, unsigned int opt_Adr){
	Rtn_CacheSts status;
	status.nh_hit=0;
	status.nm_miss=0;
	status.ne_evic=0;
	//Cache_E cache_E=transfromAdr(srt, opt_Adr);
	return status;
}
Rtn_CacheSts excuteSaveData(Cache_S *cache, Rtn_CacheSrt srt, unsigned int opt_Adr){
	Rtn_CacheSts status;
	status.nh_hit=0;
	status.nm_miss=0;
	status.ne_evic=0;
	return status;
}
Rtn_CacheSts excuteModifyData(Cache_S *cache, Rtn_CacheSrt srt, unsigned int opt_Adr){
	Rtn_CacheSts status;
	status.nh_hit=0;
	status.nm_miss=0;
	status.ne_evic=0;
	return status;
}