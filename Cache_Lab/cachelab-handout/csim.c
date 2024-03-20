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
	int lastUse;
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
Rtn_CacheSts updateCache(Cache_S *cache, Rtn_CacheSrt srt, unsigned int opt_Adr);
Rtn_CacheSts modifyData(Cache_S *cache, Rtn_CacheSrt srt, unsigned int opt_Adr);

void debug(Rtn_CacheSrt srt, Rtn_FileSrt fileSrt){
	printf("Set:%u, E:%u, b:%u, fileName:%s, opt_C:%c, opt_Adr:0x%x(%d)\t",
			srt.ng_s,srt.nr_E,srt.nb_b,srt.fileName,
			fileSrt.opt_C,fileSrt.opt_Adr,fileSrt.opt_Adr);
	Cache_E cache_E = transfromAdr(srt, fileSrt.opt_Adr);
	printf("cache address:%x,%x\t",cache_E.signBits,cache_E.setBits);
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
		cache[i].cePtr = (Cache_E *)malloc(sizeof(Cache_E)*srt.nr_E);
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
		debug(srt,opts);
		switch (opts.opt_C)
		{
		case 'L':
		case 'S':
			if (DETAIL_V){
				for (int i = 1; i < strlen(line); i++){
					if (line[i] != '\n'){
						printf("%c",line[i]);
					}
				}
				printf(" ");
			}
			status = updateCache(cache,srt,opts.opt_Adr);
			break;
		case 'M':
			if (DETAIL_V){
				for (int i = 1; i < strlen(line); i++){
					if (line[i] != '\n'){
						printf("%c",line[i]);
					}
				}
				printf(" ");
			}
			status = modifyData(cache,srt,opts.opt_Adr);
			break;
		case 'I':
			continue;
			break;
		default:
			printf("Error in read file %s,%c is not have\n",srt.fileName, opts.opt_C);
			exit(1);
			break;
		}
		ans_hit += status.nh_hit;
		ans_miss += status.nm_miss;
		ans_evic += status.ne_evic;
		if (DETAIL_V){
			printf("\n");
		}
	} 

    printSummary(ans_hit, ans_miss, ans_evic);
	
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
	
	for (int i = 0; i < 3; i++){
		if (line[i] != ' '){
			fileSrt.opt_C = line[i];
		}
	}
	
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
	cache_E.signBits = opt_Adr >> srt.ng_s;
	return cache_E;
}


/*idx is not to update, but other need*/
void lastUseAdd(Cache_E *cePtr, int idx, int nr_E){
	for (int i = 0; i < nr_E; i++){
		if (i==idx){
			continue;
		}
		cePtr[i].lastUse++;
	}
	return ;
}
/*oh, I have miss, so I need load data to my cache
 *And I use LRU to evict cache row 
*/
void missToGetData(Cache_S *cache, Rtn_CacheSts *status, Rtn_CacheSrt srt, Cache_E cache_E){
	Cache_E *cePtr = cache[cache_E.setBits].cePtr;
	for (int i = 0; i < srt.nr_E; i++){
		if (cePtr[i].validBits == 0){
			/*I can directly exchange this empty block */
			cePtr[i].validBits = 1;
			cePtr[i].signBits = cache_E.signBits;
			cePtr[i].lastUse = 0;
			/*update other lastUse*/
			lastUseAdd(cePtr, i, srt.nr_E);
			return ;
		}
	}
	/*otherwise, it means there are no empty blocks, and the cache is full*/
	if (DETAIL_V){
		printf("eviction ");
	}
	
	status->ne_evic++;
	int lastUse = cePtr[0].lastUse;
	int cnt = 0;
	for (int i = 0; i < srt.nr_E; i++){
		if (lastUse <= cePtr[i].lastUse){
			lastUse = cePtr[i].lastUse;
			cnt = i;
		}
	}
	cePtr[cnt].validBits = 1;
	cePtr[cnt].signBits = cache_E.signBits;
	cePtr[cnt].lastUse = 0;
	lastUseAdd(cePtr, cnt, srt.nr_E);
	return ;
}

Rtn_CacheSts updateCache(Cache_S *cache, Rtn_CacheSrt srt, unsigned int opt_Adr){
	Rtn_CacheSts status;
	status.nh_hit=0;
	status.nm_miss=0;
	status.ne_evic=0;
	Cache_E cache_E=transfromAdr(srt, opt_Adr);
	/*Then I need check have corresponding cache in variable cache
	 *cache[cache_E.setBits] is the corresponding row and I need find validBits==1 
	 *and signBits==cache[cache_E.setBits].cePtr[i].signBits
	*/
	Cache_E *cePtr = cache[cache_E.setBits].cePtr;
	int haveHit = 0;
	for (int i = 0; i < srt.nr_E; i++){
		if (cePtr[i].validBits == 1 && cePtr[i].signBits == cache_E.signBits){
			if (DETAIL_V){
				printf("hit ");
			}
			/*oh what f*k, I forget update this lastUse!!!
			* ******   *     *  * * *
			* ****** *   *   *      *
			* ******   *     *      *
			*/ 
			cePtr[i].lastUse = 0;
			lastUseAdd(cePtr, i, srt.nr_E);

			status.nh_hit++;
			haveHit = 1;
			break;
		}
	}
	if (!haveHit){
		if (DETAIL_V){
				printf("miss ");
		}
		status.nm_miss++;
		missToGetData(cache, &status, srt, cache_E);
	}
	return status;
}

Rtn_CacheSts modifyData(Cache_S *cache, Rtn_CacheSrt srt, unsigned int opt_Adr){
	Rtn_CacheSts status1 = updateCache(cache, srt, opt_Adr);
	Rtn_CacheSts status2 = updateCache(cache, srt, opt_Adr);
	status1.nh_hit += status2.nh_hit;
	status1.nm_miss += status2.nm_miss;
	status1.ne_evic += status2.ne_evic;
	return status1;
}