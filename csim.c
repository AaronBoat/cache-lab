#include "cachelab.h"
#include <stdbool.h>
#include <stdlib.h>   
#include <stdio.h>   
#include <getopt.h>   
#include <string.h> 
typedef struct cache_line
{
    bool valid;     //有效位
    unsigned long tag;       //标记位
    int time_tamp; //时间戳
} Cache_line;

typedef struct cache_
{
    int S,E,B;
    Cache_line **line; 
} Cache; //整个cache 的结构 包含 S 个 set ， 每个 set 有 E 个缓存行， 每行是一个 B 位的块

Cache  *cache; //实例化，一个指向 某个 Cache 的指针

int hit_count = 0, miss_count = 0, eviction_count = 0; // 记录冲突不命中、缓存不命中
int verbose = 0;                                       //是否打印详细信息
char t[1000];

void Init_Cache(int s, int E, int b)
{
    int S = 1 << s; //set个数： 2 ^ s
    int B = 1 << b; // 块的大小： 2 ^ b
    cache = (Cache *)malloc(sizeof(Cache)); //实际cache 实例化， 分配 一个结构体所需要的空间 在堆上 
    // ( ) 已经free
    cache->S = S;
    cache->E = E;
    cache->B = B;
    cache->line = (Cache_line **)malloc(sizeof(Cache_line *) * S); //二维数组外层初始化： S 个 set，每个set 将是一组 cacheline
    for (int i = 0; i < S; i++)
    {
        cache->line[i] = (Cache_line *)malloc(sizeof(Cache_line) * E); // 分配每个 set 里面 的 E 个 cacheline
        for (int j = 0; j < E; j++)
        {
            cache->line[i][j].valid = 0; //初始时，高速缓存是空的
            cache->line[i][j].tag = -1; 
            cache->line[i][j].time_tamp = 0;
        }
    
    }
}
void free_Cache()
{
    int S = cache->S;
    for (int i = 0; i < S; i++)
    {
        free(cache->line[i]);
    }
    free(cache->line);
    free(cache);
}
void update(int i, int op_s, int op_tag) // op_s 是 操作对象所属的组索引 ， 下层的块只能 存进某个特定的 set ， 而LRU是针对 set 内部的eviction
{ 
    cache->line[op_s][i].valid=1;   //标记为有效
    cache->line[op_s][i].tag = op_tag;  //要求传入高位tag位，
    for(int k = 0; k < cache->E; k++)
        if(cache->line[op_s][k].valid==1)
            cache->line[op_s][k].time_tamp++;   //每次更新所有时间戳++
    cache->line[op_s][i].time_tamp = 0; //把自身时间戳重新置零
}

int find_LRU(int op_s)  //在同一个区块中寻找
{
    int max_index = 0;
    int max_time_tamp = 0;
    for(int i = 0 ; i < cache->E ; i++)
    {
        if(cache->line[op_s][i].time_tamp > max_time_tamp)
        {
            max_index = i;
            max_time_tamp = cache->line[op_s][i].time_tamp ;
        }
    }
    return max_index ; //找出最大时间戳对应的目录
}

int is_full(int op_s)   //判断set 是否已经填满
{
    for (int i = 0; i < cache->E; i++)
    {
        if (cache->line[op_s][i].valid == 0)
            return i;   //冷不命中 -> 在第一个空的 line 填入即可
    }
    return -1;  //容量不命中 -> 需要 eviction
}

int get_index(int op_s, int op_tag)
{
    for (int i = 0; i < cache->E; i++)
    {
        if (cache->line[op_s][i].valid && cache->line[op_s][i].tag == op_tag)   //判断命中逻辑
            return i;
    }
    return -1;  //miss逻辑
}

void update_info(int op_tag, int op_s)  //更新缓存状态并且输出三种命中情况
{
    int index = get_index(op_s, op_tag);
    if (index == -1)
    {
        miss_count++;
        if (verbose)
            printf("miss ");
        int i = is_full(op_s);
        if(i==-1){
            eviction_count++;
            if(verbose) printf("eviction");
            i = find_LRU(op_s);
        }
        update(i,op_s,op_tag);
    }
    else{
        hit_count++;
        if(verbose)
            printf("hit");
        update(index,op_s,op_tag);    
    }
}

void get_trace(int s, int E, int b)
{
    FILE *pFile;
    pFile = fopen(t, "r");
    if (pFile == NULL)
    {
        exit(-1);
    }
    char identifier;
    unsigned address;
    int size;
    // Reading lines like " M 20,1" or "L 19,3"
    while (fscanf(pFile, " %c %x,%d", &identifier, &address, &size) > 0) // I读不进来,忽略---size没啥用
    {
        //想办法先得到标记位和组序号
        int op_tag = address >> (s + b);
        int op_s = (address >> b) & ((unsigned)(-1) >> (8 * sizeof(unsigned) - s));
        switch (identifier)
        {
        case 'M': //一次存储一次加载
            update_info(op_tag, op_s);
            update_info(op_tag, op_s);
            break;
        case 'L':
            update_info(op_tag, op_s);
            break;
        case 'S':
            update_info(op_tag, op_s);
            break;
        }
    }
    fclose(pFile);
}
void print_help()
{
    printf("** A Cache Simulator by Deconx\n");
    printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options:\n");
    printf("-h         Print this help message.\n");
    printf("-v         Optional verbose flag.\n");
    printf("-s <num>   Number of set index bits.\n");
    printf("-E <num>   Number of lines per set.\n");
    printf("-b <num>   Number of block offset bits.\n");
    printf("-t <file>  Trace file.\n\n\n");
    printf("Examples:\n");
    printf("linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

int main(int argc, char *argv[])    
{
    char opt;
    int s, E, b;
    /*
     * s:S=2^s是组的个数
     * E:每组中有多少行
     * b:B=2^b每个缓冲块的字节数
     */
    while (-1 != (opt = getopt(argc, argv, "hvs:E:b:t:")))  //主程序读入命令行参数
    {
        switch (opt)
        {
        case 'h':
            print_help();
            exit(0);
        case 'v':
            verbose = 1;
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
            strcpy(t, optarg);
            break;
        default:
            print_help();
            exit(-1);
        }
    }
    Init_Cache(s, E, b); //初始化一个cache
    get_trace(s, E, b);
    free_Cache();
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
