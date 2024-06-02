#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "cachelab.h"

struct line {
    unsigned long valid:1;
    long index:63;
    struct line * next;
};

struct group {
    struct line * root;
    int num;
};

int main(int argc, char ** argv)
{
    int s;
    int E;
    int b;
    const char * t;
    int c;
    int verbose = 0;
    while ((c = getopt(argc, argv, "s:E:b:t:v")) != -1) {
        switch (c) {
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
                t = optarg;
                break;
            case 'v':
                verbose = 1;
                break;
        }
    }

    if(verbose) {
        printf("s = %d, E = %d, b = %d, t = %s\n", s, E, b, t);
    }

    FILE *fptr;
    fptr = fopen(t, "r");
    char buff[255];

    int mem_size = sizeof(struct group) * (1 << s);
    struct group * cache = (struct group*)malloc(mem_size);
    memset(cache, 0, mem_size);

    int hits_count = 0;
    int misses_count = 0;
    int evictions_count = 0;

    while(fgets(buff, 255, fptr)) {

        int loop_num = 1;

        char *p = buff;
        while(p[0] == ' ') {
            ++p;
        }
        if(p[0] == 'I') {
            continue;
            // Modify means read and then write, so do it twice
        } else if(p[0] == 'M') {
            loop_num = 2;
        }
        if(verbose)
        {
            printf("%c ", p[0]);
        }
        do {
            ++p;
        } while(p[0] == ' ');

        long address;
        int size;

        sscanf(p, "%lx,%x", &address, &size);
        if(verbose)
        {
            printf("%lx,%x", address, size);
        }

        for(int i = 0; i < loop_num; ++i) {

            long last_aligned_address = -1;

            for(int offset = 0; offset < size; ++offset) {
                long aligned_address = (address + offset) >> b;
                if(aligned_address == last_aligned_address) {
                    continue;
                }
                last_aligned_address = aligned_address;

                int index = aligned_address & ((1 << s) - 1);

                struct line * found = NULL;
                struct group * g = &cache[index];
                {
                    struct line * l = g->root;
                    while(l) {
                        if(l->valid && l->index == aligned_address) {
                            found = l;
                            break;
                        }
                        l = l->next;
                    }
                }
                if(found) {
                    hits_count ++;
                    // Make the latest visited at first
                    if(found != g->root) {
                        struct line * found_next = found->next;
                        found->next = g->root;
                        g->root = found;
                        struct line * l = found->next;
                        while(l) {
                            if(l->next == found) {
                                l->next= found_next;
                                break;
                            }
                            l = l->next;
                        }
                    }
                    if(verbose) {
                        printf(" hit");
                    }
                } else {
                    misses_count ++;
                    if(verbose) {
                        printf(" miss");
                    }

                    struct line * l = g->root;
                    struct line * n = malloc(sizeof(struct line));
                    memset(n, 0, sizeof(struct line));
                    n->index = aligned_address;
                    n->valid = 1;
                    n->next = l;
                    g->root = n;
                    if(g->num == E) {
                        // evict last one
                        while(l->next) {
                            l = l->next;
                            n = n->next;
                        }
                        n->next = NULL;
                        free(l);
                        evictions_count ++;
                        if(verbose) {
                            printf(" eviction");
                        }
                    } else {
                        // just insert at first
                        g->num ++;
                    }
                } 
            }
        }
        if(verbose)  printf("\n");
    }


    fclose(fptr);
    printSummary(hits_count, misses_count, evictions_count);
    return 0;
}
