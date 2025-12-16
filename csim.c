#include "cachelab.h"
#include <stdbool.h>

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
void update(int i, int op_s, int op_tag) // op_s 是 操作对象所属的组索引 ， 下层的块只能 存进某个特定的 set ， 而LRU是针对 set 内部的eviction
{ 
    cache->line[op_s][i].valid=1;
    cache->line[op_s][i].tag = op_tag;  //要求传入高位tag位，
    for(int k = 0; k < cache->E; k++)
        if(cache->line[op_s][k].valid==1)
            cache->line[op_s][k].time_tamp++;
    cache->line[op_s][i].time_tamp = 0;
}

int find_LRU(int op_s)
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
    return max_index ;
}
int main()
{
    printSummary(0, 0, 0);
    return 0;
}
