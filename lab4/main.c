// Programmer: Amariah Del Mar
// lab4

#include <stdio.h>       // for printf()
#include <stdlib.h>      // for exit()
#include <string.h>      // for strcpy(), strcmp(), etc.
#include <libgen.h>      // for basename(), dirname()
#include <fcntl.h>       // for open(), close(), read(), write()

// for stat syscalls
#include <sys/stat.h>
#include <unistd.h>

// for opendir, readdir syscalls
#include <sys/types.h>
#include <dirent.h>

//typedef enum {
//    false = 0,
//    true = 1
//} Bool;



int cpd2d(char *f1, char *f2);
int fileexists(const char *filename);
int myrcp(char *f1, char *f2);
int check_identical_files(const char *f1, const char *f2);
int cpf2f(char *f1, char *f2);


int main(int argc, char *argv[]) {
    if (argc < 3) {
        //print usage and exit
        printf("This program requires two arguments to run. Please run as './a.out file1 file2'\n");
        return 0;
    }
    return myrcp(argv[1], argv[2]);
}

int fileexists(const char *filename) {
    FILE *file;
    if ((file = fopen(filename, "r"))) {
        fclose(file);
        return 1;
    }
    return 0;
}

int myrcp(char *f1, char *f2) {

//    1. stat f1;   if f1 does not exist ==> exit.
    if( !f1 ){
        printf("Error: file1 must be provided.\n");
    }
    if (!fileexists(f1)) {
        printf("Given file to copy does not exist. Please provide an existing file for the first argument.\n");
        return 0;
    }
    struct stat f1_stat, f2_stat;
    stat(f1, &f1_stat);
    stat(f2, &f2_stat);

//   f1 exists: reject if f1 is not REG or LNK file
    if (S_ISREG(f1_stat.st_mode) || S_ISLNK(f1_stat.st_mode)) {
        printf("f1 is reg or link\n");
//    2. stat f2;   reject if f2 exists and is not REG or LNK file
        if (!fileexists(f2) || (S_ISREG(f2_stat.st_mode) || S_ISLNK(f2_stat.st_mode)) ) {
//    3. if (f1 is REG)
            if (S_ISREG(f1_stat.st_mode)) {
//          if (f2 non-exist OR exists and is REG)
//             return cpf2f(f1, f2);
// 	 else // f2 exist and is DIR
//             return cpf2d(f1,f2);
                printf("f1 is reg\n");
                if (!fileexists(f2) || (fileexists(f2) && S_ISREG(f2_stat.st_mode))) {
                    return cpf2f(f1, f2);
                } else if (fileexists(f2) && S_ISDIR(f2_stat.st_mode)) {
                    return cpd2d(f1, f2);
                }
            }
        }
    }
//    4. if (f1 is DIR)
// 	if (f2 exists but not DIR) ==> reject;
//         if (f2 does not exist)     ==> mkdir f2
// 	return cpd2d(f1, f2);

    if (S_ISDIR(f1_stat.st_mode)) {
        if (fileexists(f2) && !S_ISDIR(f2_stat.st_mode)) {
            printf("File exists but is not DIR\n");
            return 0;
        }
        if (!fileexists(f2)) {
            printf("f2 doesn't exist. creating new directory");
            mkdir(f2, f1_stat.st_mode);
        }
        return cpd2d(f1, f2);

    }
}
//
//int check_identical_files(const char *f1, const char *f2) {
//    printf("Checking to see if files %s and %s are identical\n", f1, f2);
//    FILE *fp1, *fp2;
//    int ch1, ch2;
//    fp1 = fopen(f1, "r");
//    fp2 = fopen(f2, "r");
//
//    if (fp1 == NULL) {
//        printf("Cannot open f1 %s for reading \n", f1);
//        exit(1);
//    } else if (fp2 == NULL) {
//        printf("Cannot open f2 %s for reading\nFile may not exist\n", f2);
//        exit(1);
//    }
//    ch1 = getc(fp1);
//    ch2 = getc(fp2);
//
//    while ((ch1 != EOF) && (ch2 != EOF) && (ch1 == ch2)) {
////        printf("%c : %c\n", ch1, ch2);
//        ch1 = getc(fp1);
//        ch2 = getc(fp2);
//
//    }
//
//    if (ch1 == ch2) {
//        printf("Files are identical\n");
//        exit(1);
//    }
//    else if (ch1 != ch2) printf("Files are not identical");
//
//    fclose(fp1);
//    fclose(fp2);
//    return 1;
//}


// cp file to file
int cpf2f(char *f1, char *f2) {

//   1. reject if f1 and f2 are the SAME file
    char buffer[4096];

    struct stat f1_stat, f2_stat;
    lstat(f1, &f1_stat);
    int f2_exists = lstat(f2, &f2_stat);

    if (f1_stat.st_ino == f2_stat.st_ino) {
        printf("These files are the same, there is no need to copy!\n");
        return 1;
    }
//   2. if f1 is LNK and f2 exists: reject
    if (S_ISLNK(f1_stat.st_mode) && !f2_exists){
        printf("f1 %s is lnk and f2 %s exists...\n...REJECTING...\n", f1, f2);
        return 1;
    }
//   3. if f1 is LNK and f2 does not exist: create LNK file f2 SAME as f1
    //TODO: get help
    if (S_ISLNK(f1_stat.st_mode) && !fileexists(f2)) {
        printf("f1 is lnk and f2 doesn't exist\n");
        buffer[readlink(f1, buffer, 4096)] = '\0';
        symlink(buffer, f2);
    }
//   4:
//      open f1 for READ;
//      open f2 for O_WRONLY|O_CREAT|O_TRUNC, mode=mode_of_f1;
//      copy f1 to f2
    int fp1 = open(f1, O_RDONLY);
    int fp2 = open(f2, O_WRONLY | O_CREAT|O_TRUNC, f1_stat.st_mode);

    int len;
    while (len = read(fp1, buffer, 4096)) {
        write(fp2, buffer, len);
    }
    printf("%s copied to %s\n", f1, f2);
    close(fp1);
    close(fp2);
    return 0;
}

int cpd2d(char *f1, char *f2) {
    printf("Copying one directory to another in cpd2d\n");

    struct stat f1_stat, f2_stat;
    lstat(f1, &f1_stat);
    lstat(f2, &f2_stat);
    if (f1_stat.st_ino == f2_stat.st_ino) {
        printf("These directories are the same, there is no need to copy!!\n");
        return 1;
    }

    // recursively cp dir to dir
    struct dirent* entry;

    printf("making dir\n");
    DIR *directory = opendir(f1);
    char realpth1[4096], realpth2[4096];
    realpath(f1, realpth1);
    realpath(f2, realpth2);
    if (!strncmp(realpth1, realpth2, strlen(realpth1))) {
        printf("Cannot copy to a subdirectory of source.\n");
        return 1;
    }
    while(entry = readdir(directory)) {

        printf("entry = readdir(directory)");
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;

        char fullpth1[4096], fullpth2[4096];
        strcat(strcat(strcpy(fullpth1, f1), "/"), entry->d_name);
        strcat(strcat(strcpy(fullpth2, f2), "/"), entry->d_name);

        struct stat path_stat;
        stat(fullpth1, &path_stat);
        if (S_ISDIR(path_stat.st_mode)) {
            printf("fullpth1 is dir\n");
            mkdir(fullpth2, path_stat.st_mode);
            cpd2d(fullpth1, fullpth2);
        } else {
            printf("fullpth2 is file\n");
            cpf2f(fullpth1, fullpth2);
        }
    }
    return 0;
}


