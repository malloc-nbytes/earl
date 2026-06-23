#include "io.h"
#include "array.h"
#include "compatibility.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <unistd.h>

int
file_exists(const char *fp)
{
        FILE *f;

        if ((f = fopen(fp, "r")) != NULL) {
                fclose(f);
                return 1;
        }

        return 0;
}

int
create_file(const char *fp,
            int         force_overwrite)
{
        FILE *f;

        if (!force_overwrite && file_exists(fp))
                return 1;

        if (!(f = fopen(fp, "w")))
                return 0;

        fclose(f);
        return 1;
}

int
write_file(const char *fp,
           const char *content)
{
        FILE *f;

        if (!(f = fopen(fp, "w")))
                return 0;

        (void)fprintf(f, "%s", content);
        fclose(f);
        return 1;
}

int
is_dir(const char *path)
{
        struct stat path_stat;

        if (lstat(path, &path_stat) != 0)
                return 0;

        return S_ISDIR(path_stat.st_mode);
}

char *
load_file(const char *path)
{
        FILE   *f;
        char   *buf;
        long int  size;

        if ((f = fopen(path, "rb")) == NULL)
                return NULL;

        fseek(f, 0, SEEK_END);
        size = ftell(f);
        fseek(f, 0, SEEK_SET);

        buf = (char *)malloc((size_t)(size + 1));
        size_t _ = fread(buf, 1, (size_t)size, f);
        (void)_;

        fclose(f);

        buf[size] = '\0';

        return buf;
}

cstr_ar
lsdir(const char *dir)
{
        DIR           *dp;
        struct dirent *entry;
        cstr_ar        files;

        files = array_empty(cstr_ar);

        if (!(dp = opendir(dir)))
                return files;

        while ((entry = readdir(dp)))
                array_append(files, strdup(entry->d_name));

        closedir(dp);
        return files;
}

const char *
gethome(void)
{
        static char buf[1024] = {0};
        const char *home = getenv("HOME");

        if (home != NULL && home[0] != '\0') {
                size_t len = strlen(home);
                if (len >= sizeof(buf)) {
                        errno = ENAMETOOLONG;
                        return NULL;
                }
                memcpy(buf, home, len + 1);
                return buf;
        }

        struct passwd *pw = getpwuid(getuid());
        if (pw != NULL && pw->pw_dir != NULL && pw->pw_dir[0] != '\0') {
                size_t len = strlen(pw->pw_dir);
                if (len >= sizeof(buf)) {
                        errno = ENAMETOOLONG;
                        return NULL;
                }
                memcpy(buf, pw->pw_dir, len + 1);
                return buf;
        }

        /* Failure */
        errno = ENOENT;
        return NULL;
}

char *
get_realpath(const char *fp)
{
        char *result = NULL;

        if (!fp || !*fp)
                return NULL;

        if (fp[0] == '~') {
                struct passwd *pw = getpwuid(getuid());
                if (!pw)
                        return NULL;

                size_t home_len = strlen(pw->pw_dir);
                size_t fp_len = strlen(fp);
                result = malloc(home_len + fp_len);
                if (!result) { return NULL; }

                strcpy(result, pw->pw_dir);
                strcat(result, fp + 1); // Skip the ~
        } else {
                result = strdup(fp);
                if (!result)
                        return NULL;
        }

        char *absolute = realpath(result, NULL);
        free(result);

        if (!absolute) {
                return NULL;
        }

        return absolute;
}

const char *
get_basename(const char *name)
{
        for (int i = (int)strlen(name)-2; i >= 0; --i) {
                if (name[i] == '/')
                        return name+i+1;
        }
        return name;
}
