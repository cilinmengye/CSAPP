#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>


void opt_example1(int argc, char** argv)
{
    const char* optstr = "a:";
    char ch;
    while ( (ch = getopt(argc, argv, optstr)) != -1 )
    {
        switch (ch) {
            case 'a':
                printf("have option: -a\n");
                printf("the argument of -a is %s\n", optarg);
                break;
        }
    }
}

void help_and_die(char* argv0)
{
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
 
    exit(0);
}
void opt_example2(int argc, char** argv)
{
    const char* optstr = "s::";
    char ch;
    while ( (ch = getopt(argc, argv, optstr)) != -1 )
    {
        switch (ch) {
            case 's':
                printf("have option: -s\n");
                printf("the argument of -s is %s\n", optarg);
                break;
        }
    }
}
void opt_example8(int argc, char *argv[])
{
    const char* optstr = "hvs::E::b::t::";
    char ch;
 
    bool verbose = false;
    int s = -1;
    int E = -1;
    int b = -1;
    int t = -1;
    while( (ch = getopt(argc, argv, optstr)) != -1 )
    {
        switch (ch)
        {
        case 'h':
            help_and_die(argv[0]);
            exit(0);
            break;
        case 'v':
            verbose = true;
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
            t = atoi(optarg);
            break;
        case '?':
            help_and_die(argv[0]);
            break;
        }
    }

    printf("%d,%d,%d\n",s,E,b);
 
    if (optind == 1)
    {
        printf("%s: Missing required command line argument\n", argv[0]);
        help_and_die(argv[0]);
    }
}

int main(int argc, char** argv)
{
    //printf("opterr = %d\n", opterr);
 
    //opt_example1(argc, argv);
    //opt_example2(argc, argv);
    //opt_example3(argc, argv);
    //opt_example4(argc, argv);
    //opt_example5(argc, argv);
    //opt_example6(argc, argv);
    //opt_example7(argc, argv);
    opt_example8(argc, argv);
 
    return 0;
}