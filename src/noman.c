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
#include <sys/types.h>
#include <unistd.h>

#define NOTES_DIR ".noman"

static void usage()
{
        printf(
                "noman [OPTION]... [TOPIC]\n"
                "A command-line tool for accessing personalized notes and cheat sheets instantly.\n\n"
                "Options:\n"
                "  -d dir  specify a custom notes directory\n"
                "  -h      display this help and exit\n"
        );
}

static void view_note(FILE *notes_file)
{
        char buffer[1024];

        while (fgets(buffer, sizeof(buffer), notes_file)) {
                printf("%s", buffer);
        }
}

struct passwd *pws;

int main(int argc, char **argv)
{
        int opt = 0, custom_dir = 0;
        char *dir_s;

        while((opt = getopt(argc, argv, ":d:h")) != -1) {
                switch(opt) {
                case 'd':
                        custom_dir = 1;
                        dir_s = optarg;
                        break;

                case 'h':
                        usage();
                        exit(EXIT_SUCCESS);

                case ':':
                        if (optopt == 'd')
                                fprintf (stderr,
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
                exit(EXIT_FAILURE);
        }

        DIR *dirp;

        FILE *fp;
        char *note_s = argv[optind];

        pws = getpwuid(getuid());
        if (!custom_dir) {
                dir_s = realloc(dir_s, strlen(pws->pw_dir) + 8);
                sprintf(dir_s, "%s/%s", pws->pw_dir, NOTES_DIR);
        }

        char *pattern = malloc(strlen(note_s) + 6);
        sprintf(pattern, "*%s*.md", note_s);

        dirp = opendir(dir_s);
        if (errno == ENOENT) {
                fprintf(stderr, "Error: notes directory does not exist.\n");
                free(pattern);
                exit(EXIT_FAILURE);
        }

        int num_files = 0;
        char **file_list;
        struct dirent *entry;
        while ((entry = readdir(dirp)) != NULL) {
                if (fnmatch(pattern, entry->d_name, 0) == 0) {
                        file_list = realloc(file_list,
                                            sizeof(char *) * (num_files + 1));
                        file_list[num_files] = malloc(strlen(entry->d_name) + 1);
                        strcpy(file_list[num_files], entry->d_name);
                        num_files++;
                }
        }

        closedir(dirp);
        free(pattern);

        if(!num_files) {
                if (!custom_dir)
                        free(dir_s);
                free(file_list);
                exit(EXIT_FAILURE);
        }

        else if (num_files > 1) {
                fprintf(stderr, "Error: multiple notes found.\n");
                exit(EXIT_FAILURE);
        }

        char *path = malloc(strlen(dir_s) + strlen(
                                    file_list[0]) + 2);
        sprintf(path, "%s/%s", dir_s, file_list[0]);

        fp = fopen(path, "r");

        if (fp == NULL) {
                fprintf(stderr, "Error: could not open note file.\n");
                if (!custom_dir)
                        free(dir_s);
                free(file_list);
                exit(EXIT_FAILURE);
        }

        view_note(fp);

        fclose(fp);

        if (!custom_dir)
                free(dir_s);
        for (int i = 0; i < num_files; i++)
                free(file_list[i]);
        free(file_list);
        free(path);

        return EXIT_SUCCESS;
}