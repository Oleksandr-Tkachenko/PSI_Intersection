/* 
 * File:   main.c
 * Author: Oleksandr Tkachenko <oleksandr.tkachenko at crisp-da.de>
 *
 * Created on May 5, 2016, 12:52 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>

#include "psi_structures.h"
#include "psi_bucketed_intersection.h"

int parse_argv(int argc, char** argv, PSI_INTERSECTION_CTX* ctx);

/*
 * 
 */
int main(int argc, char** argv) {
    if (argc == 1) {
        printf("PSI Intersection\n");
        return (EXIT_FAILURE);
    }
    PSI_INTERSECTION_CTX ctx[1];
    ctx->result = NULL;
    parse_argv(argc, argv, ctx);
    psi_bucketed_intersection(ctx);
    return (EXIT_SUCCESS);
}

int parse_argv(int argc, char** argv, PSI_INTERSECTION_CTX* ctx) {
    int index, c;
    opterr = 0;
    while ((c = getopt(argc, argv, "l:p:e:n:q:r:s:t:a:b:")) != -1)
        switch (c) {
            case 'p':
                strncpy(ctx->path_result, optarg, 128);
                break;
            case 'e':
                ctx->element_size = atoi(optarg);
                break;
            case 'n':
                ctx->bucket_n = atoi(optarg);
                break;
            case 'l':
                ctx->lookup = TRUE;
                strncpy(ctx->path_lookup, optarg, 128);
                break;
            case 'q':
                ctx->queue_buffer_size = atoi(optarg);
                break;
            case 'r':
                ctx->read_buffer_size = atoi(optarg);
                break;
            case 's':
                strncpy(ctx->path_folder_buckets, optarg, 128);
                break;
            case 't':
                ctx->threads = atoi(optarg);
                break;
            case 'a':
                strncpy(ctx->path_a, optarg, 128);
                break;
            case 'b':
                strncpy(ctx->path_b, optarg, 128);
                break;
            case '?':
                if (isprint(optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr,
                        "Unknown option character `\\x%x'.\n",
                        optopt);
                exit(EXIT_FAILURE);
            default:
                abort();
        }

    for (index = optind; index < argc; index++)
        printf("Non-option argument %s\n", argv[index]);

    return 0;
}
