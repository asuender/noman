#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <getopt.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define NOTES_DIR ".noman" /* default notes directory */

static struct option long_options[] = {
        {"dir", required_argument, 0, 'd'},
        {"recursive", no_argument, 0, 'r'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
};

static void usage()
{
        printf(
                "noman [OPTION]... [TOPIC]\n"
                "A command-line tool for accessing personalized notes and cheat sheets instantly.\n\n"
                "Options:\n"
                "  -d, --dir=DIR    specify a custom notes directory\n"
                "  -r, --recursive  search recursively for notes\n"
                "  -h, --help       display this help and exit\n");
}

static int find_note(char *dir_s, char ***file_list,
                     char *pattern, int recursive)
{
        int fc = 0;
        char *d_name;
        unsigned char d_type;
        DIR *dirp = opendir(dir_s);

        struct dirent *entp;
        while ((entp = readdir(dirp)) != NULL) {
                d_name = entp->d_name;
                d_type = entp->d_type;

                if (!strcmp(d_name, ".") || !strcmp(d_name, ".."))
                        continue;

                if (d_type == DT_DIR && recursive) {
                        char *path = malloc(strlen(dir_s) + strlen(d_name) + 2);
                        sprintf(path, "%s%s/", dir_s, d_name);
                        fc += find_note(path, file_list, pattern, recursive);
                        free(path);
                } else if (d_type == DT_REG &&
                           !fnmatch(pattern, d_name, 0)) {
                        *file_list = realloc(*file_list,
                                             sizeof(char *) * (fc + 1));
                        (*file_list)[fc] = malloc(strlen(d_name) + 1);
                        strcpy((*file_list)[fc], d_name);
                        fc++;
                }
        }

        closedir(dirp);
        return fc;
}

static void view_note(FILE *notes_file)
{
        char buffer[1024];

        while (fgets(buffer, sizeof(buffer), notes_file)) {
                printf("%s", buffer);
        }
}

struct passwd *pws;
struct stat st = {0};

int main(int argc, char **argv)
{
        int opt = 0, optidx = 0, custom_dir = 0, recursive = 0;
        char *dir_s;

        while ((opt = getopt_long(argc, argv, ":d:rh", long_options,
                                  &optidx)) != -1) {
                switch (opt) {
                case 'd':
                        if (!strlen(optarg)) {
                                fprintf(stderr,
                                        "Error: Option --dir requires an argument.\n");
                                exit(EXIT_FAILURE);
                        }
                        custom_dir = 1;
                        dir_s = malloc(strlen(optarg) + 2);
                        strcpy(dir_s, optarg);
                        if (optarg[strlen(optarg) - 1] != '/')
                                strcat(dir_s, "/");
                        break;

                case 'r':
                        recursive = 1;
                        break;

                case 'h':
                        usage();
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

        if (optind >= argc) {
                usage();
                fprintf(stderr, "\nError: no topic specified.\n");
                free(dir_s);
                exit(EXIT_FAILURE);
        }

        FILE *fp;

        char *note_s = argv[optind];

        pws = getpwuid(getuid());
        if (!custom_dir) {
                dir_s = realloc(dir_s, strlen(pws->pw_dir) + 8);
                sprintf(dir_s, "%s/%s", pws->pw_dir, NOTES_DIR);
        }

        char *pattern = malloc(strlen(note_s) + 6);
        sprintf(pattern, "*%s*.md", note_s);

        if (stat(dir_s, &st) == -1) {
                fprintf(stderr, "Error: notes directory does not exist.\n");
                free(dir_s);
                free(pattern);
                exit(EXIT_FAILURE);
        }

        char **file_list = NULL;
        int nc = find_note(dir_s, &file_list, pattern, recursive);

        free(pattern);

        if (!nc) {
                fprintf(stderr, "Error: no notes found.\n");
                free(dir_s);
                for (int i = 0; i < nc; i++)
                        free(file_list[i]);
                free(file_list);
                exit(EXIT_FAILURE);
        }

        if (nc > 1) {
                fprintf(stderr, "Error: multiple notes found.\n");
                free(dir_s);
                for (int i = 0; i < nc; i++)
                        free(file_list[i]);
                free(file_list);
                exit(EXIT_FAILURE);
        }

        char *path = malloc(strlen(dir_s) + strlen(
                                    file_list[0]) + 2);
        sprintf(path, "%s/%s", dir_s, file_list[0]);

        fp = fopen(path, "r");
        if (fp == NULL) {
                fprintf(stderr, "Error: could not open note file.\n");
                free(dir_s);
                for (int i = 0; i < nc; i++)
                        free(file_list[i]);
                free(file_list);
                exit(EXIT_FAILURE);
        }

        view_note(fp);

        fclose(fp);

        free(dir_s);
        for (int i = 0; i < nc; i++)
                free(file_list[i]);
        free(file_list);
        free(path);

        return EXIT_SUCCESS;
}