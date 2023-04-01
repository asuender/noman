#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#if defined(__arm__) || defined(__i386__)
#define _FILE_OFFSET_BITS 64
#endif
#if defined(__APPLE__)
#define _DARWIN_C_SOURCE
#endif

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <getopt.h>
#include <limits.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __APPLE__
#include <sys/dirent.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PROGRAM_NAME  "noman"
#define VERSION       "0.1.0"

#define DEF_NOTES_DIR ".noman" /* default notes directory */

static struct option long_options[] =
{
    {"dir", required_argument, 0, 'd'},
    {"recursive", no_argument, 0, 'r'},
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'v'},
    {0, 0, 0, 0}
};

static void usage(int status)
{
    printf(
        "noman [OPTION]... [TOPIC]\n"
        "A command-line tool for accessing personalized notes and cheat sheets instantly.\n\n"
        "Options:\n"
        "  -d, --dir=DIR    specify a custom notes directory\n"
        "  -r, --recursive  search recursively for notes\n"
        "  -h, --help       display this help and exit\n"
        "  -v, --version    output version information and exit\n");

    exit(status);
}

static void search_note(char *dir_s, char ***file_list,
                        char *pattern, int *fc, int recursive)
{
    char *d_name;
    char d_type;
    char sdir_s[PATH_MAX];

    DIR *dirp = opendir(dir_s);
    struct dirent *entp;

    if (dirp == NULL)
        return;

    while((entp = readdir(dirp)) != NULL)
    {
        d_name = entp->d_name;
        d_type = entp->d_type;

        if (!strcmp(d_name, ".") || !strcmp(d_name, ".."))
            continue;

        if (d_type == DT_DIR && recursive)
        {
            sprintf(sdir_s, "%s/%s", dir_s, d_name);
            search_note(sdir_s, file_list, pattern, fc, recursive);
        }

        else if(d_type == DT_REG && !fnmatch(pattern, d_name, 0))
        {
            *file_list = realloc(*file_list,
                                 sizeof(char *) * (*fc + 1));
            (*file_list)[*fc] = malloc(strlen(dir_s) + strlen(
                                           d_name) + 2);
            sprintf((*file_list)[*fc], "%s/%s", dir_s, d_name);
            (*fc)++;
        }
    }

    closedir(dirp);
}

static void view_note(FILE *notes_file)
{
    char buffer[1024];

    while (fgets(buffer, sizeof(buffer), notes_file))
    {
        printf("%s", buffer);
    }
}

struct passwd *pws;
struct stat st = {0};

int main(int argc, char **argv)
{
    int opt = 0, optidx = 0, custom_dir = 0, recursive = 0;
    char dir_s[PATH_MAX], path[PATH_MAX];

    while ((opt = getopt_long(argc, argv, ":d:rhv",
                              long_options,
                              &optidx)) != -1)
    {
        switch (opt)
        {
        case 'd':
            if (!strlen(optarg))
            {
                fprintf(stderr,
                        "Error: Option --dir requires an argument.\n");
                exit(EXIT_FAILURE);
            }
            custom_dir = 1;
            strcpy(dir_s, optarg);
            break;

        case 'r':
            recursive = 1;
            break;

        case 'h':
            usage(EXIT_SUCCESS);

        case 'v':
            printf("%s\n", VERSION);
            exit(EXIT_SUCCESS);

        case ':':
            if (optopt == 'd')
                fprintf(stderr,
                        "Error: Option -%c requires an argument.\n",
                        optopt);
            exit(EXIT_FAILURE);

        case '?':
            fprintf(stderr, "Error: unknown option '-%c'.\n", optopt);
            exit(EXIT_FAILURE);

        default:
            break;
        }
    }

    if (optind >= argc)
        usage(EXIT_FAILURE);

    FILE *fp;

    char *note_s = argv[optind];
    char pattern[strlen(note_s) + 6];

    pws = getpwuid(getuid());
    if (!custom_dir)
    {
        sprintf(dir_s, "%s/%s", pws->pw_dir, DEF_NOTES_DIR);
    }

    if (stat(dir_s, &st) == -1)
    {
        /* TODO: change error messages */
        perror(dir_s);
        exit(EXIT_FAILURE);
    }

    char **file_list = NULL;
    int nc = 0;
    sprintf(pattern, "*%s*.md", note_s);

    search_note(dir_s, &file_list, pattern, &nc, recursive);

    if (!nc)
    {
        fprintf(stderr, "Error: no notes found.\n");
        for (int i = 0; i < nc; i++)
            free(file_list[i]);
        exit(EXIT_FAILURE);
    }

    if (nc > 1)
    {
        fprintf(stderr, "Error: multiple notes found.\n");
        for (int i = 0; i < nc; i++)
            free(file_list[i]);
        free(file_list);
        exit(EXIT_FAILURE);
    }

    strcpy(path, file_list[0]);

    fp = fopen(path, "r");
    if (fp == NULL)
    {
        /* TODO: change error messages */
        perror("Error: could not open note file");
        for (int i = 0; i < nc; i++)
            free(file_list[i]);
        free(file_list);
        exit(EXIT_FAILURE);
    }

    view_note(fp);

    fclose(fp);

    for (int i = 0; i < nc; i++)
        free(file_list[i]);
    free(file_list);

    return EXIT_SUCCESS;
}