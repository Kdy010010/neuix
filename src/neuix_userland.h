#ifndef NEUIX_USERLAND_H
#define NEUIX_USERLAND_H

#include "neuix_keyboard.h"
#include "neuix_vga.h"
#include "neuix_fs.h"
#include <stdint.h>
#include <stdbool.h>

static char cwd[256] = "/root";

static bool startswith(const char* str, const char* prefix) {
    while (*prefix) {
        if (*str++ != *prefix++) return false;
    }
    return true;
}

static void make_path(const char* input, char* output) {
    if (input[0] == '/') {
        strcpy(output, input);
    } else {
        strcpy(output, cwd);
        strcat(output, "/");
        strcat(output, input);
    }
}

static void login() {
    uint8_t buf[4096];
    uint32_t size;
    if (!fs_read("/root/user/pass.txt", buf, &size)) {
        vga_write("[Error] No user database.\n");
        while (1);
    }
    buf[size] = 0;

    while (1) {
        vga_write("Username: ");
        char username[64];
        getline(username, 64);

        vga_write("Password: ");
        char password[64];
        getline(password, 64);

        char* p = (char*)buf;
        bool success = false;
        while (*p) {
            char* user_start = p;
            char* colon = 0;
            while (*p && *p != '\n') {
                if (*p == ':' && !colon) colon = p;
                p++;
            }
            if (colon) {
                *colon = '\0';
                if (streq(user_start, username)) {
                    char* pass_start = colon + 1;
                    if (*p) *p = '\0';
                    if (streq(pass_start, password)) {
                        success = true;
                        break;
                    }
                }
            }
            if (*p) p++;
        }

        if (success) {
            vga_write("[Login Success]\n");
            return;
        } else {
            vga_write("[Login Failed]\n");
        }
    }
}

static void userland() {
    while (1) {
        vga_write(cwd);
        vga_write("> ");

        char cmdline[256];
        getline(cmdline, 256);

        if (strcmp(cmdline, "ls") == 0) {
            FileNode* node = fs_root;
            while (node) {
                vga_write(node->path);
                vga_write(" [");
                vga_write(type_to_str(node->type));
                vga_write("] ");
                char sizebuf[16];
                int si = 0;
                uint32_t temp = node->size;
                char rev[16];
                int ri = 0;
                if (temp == 0) rev[ri++] = '0';
                while (temp) {
                    rev[ri++] = '0' + (temp % 10);
                    temp /= 10;
                }
                for (int i = ri-1; i >= 0; i--) sizebuf[si++] = rev[i];
                sizebuf[si] = 0;
                vga_write(sizebuf);
                vga_write(" bytes\n");
                node = node->next;
            }
        }
        else if (strcmp(cmdline, "help") == 0) {
            vga_write("Commands:\n");
            vga_write("ls, format, cat <file>, touch <file>, mkdir <dir>, rm <path>, mv <old> <new>, cp <src> <dst>, cd <dir>, cd .., run <bin>, ed <file>, stat <file>, help\n");
        }
        else if (strcmp(cmdline, "format") == 0) {
            FileNode* node = fs_root;
            while (node) {
                FileNode* next = node->next;
                if (node->content) free(node->content);
                free(node);
                node = next;
            }
            fs_root = NULL;
            fs_save();
            fs_create("/root", TYPE_DIR, NULL, 0);
            vga_write("[Disk formatted]\n");
        }
        else if (startswith(cmdline, "cat ")) {
            char path[256];
            make_path(cmdline + 4, path);
            uint8_t buf[4096];
            uint32_t size;
            if (fs_read(path, buf, &size)) {
                buf[size] = 0;
                vga_write((char*)buf);
                vga_write("\n");
            } else {
                vga_write("[Not Found]\n");
            }
        }
        else if (startswith(cmdline, "ed ")) {
            char path[256];
            make_path(cmdline + 3, path);
            vga_write("Enter new content. End with a single line containing only '.':\n");
            uint8_t newcontent[4096];
            int idx = 0;
            while (1) {
                char line[256];
                getline(line, 256);
                if (strcmp(line, ".") == 0) break;
                int len = strlen(line);
                for (int i = 0; i < len; i++) newcontent[idx++] = line[i];
                newcontent[idx++] = '\n';
            }
            fs_create(path, TYPE_FILE, newcontent, idx);
            vga_write("[File edited]\n");
        }
        else if (startswith(cmdline, "stat ")) {
            char path[256];
            make_path(cmdline + 5, path);
            FileNode* node = fs_root;
            while (node) {
                if (streq(node->path, path)) {
                    vga_write("Path: "); vga_write(node->path); vga_write("\n");
                    vga_write("Type: "); vga_write(type_to_str(node->type)); vga_write("\n");
                    vga_write("Size: ");
                    char sizebuf[16];
                    int si = 0;
                    uint32_t temp = node->size;
                    char rev[16];
                    int ri = 0;
                    if (temp == 0) rev[ri++] = '0';
                    while (temp) {
                        rev[ri++] = '0' + (temp % 10);
                        temp /= 10;
                    }
                    for (int i = ri-1; i >= 0; i--) sizebuf[si++] = rev[i];
                    sizebuf[si] = 0;
                    vga_write(sizebuf);
                    vga_write(" bytes\n");
                    break;
                }
                node = node->next;
            }
        }
        else if (startswith(cmdline, "touch ")) {
            char path[256];
            make_path(cmdline + 6, path);
            uint8_t empty[1] = {0};
            fs_create(path, TYPE_FILE, empty, 0);
            vga_write("[Created]\n");
        }
        else if (startswith(cmdline, "mkdir ")) {
            char path[256];
            make_path(cmdline + 6, path);
            fs_create(path, TYPE_DIR, NULL, 0);
            vga_write("[Directory Created]\n");
        }
        else if (startswith(cmdline, "rm ")) {
            char path[256];
            make_path(cmdline + 3, path);
            if (fs_delete(path)) {
                vga_write("[Deleted]\n");
            } else {
                vga_write("[Delete Failed]\n");
            }
        }
        else if (startswith(cmdline, "mv ")) {
            char oldpath[256], newpath[256];
            const char* rest = cmdline + 3;
            int i = 0;
            while (*rest && *rest != ' ' && i < 255) oldpath[i++] = *rest++;
            oldpath[i] = 0;
            while (*rest == ' ') rest++;
            make_path(oldpath, oldpath);
            make_path(rest, newpath);

            if (fs_move(oldpath, newpath)) {
                vga_write("[Moved]\n");
            } else {
                vga_write("[Move Failed]\n");
            }
        }
        else if (startswith(cmdline, "cp ")) {
            char srcpath[256], destpath[256];
            const char* rest = cmdline + 3;
            int i = 0;
            while (*rest && *rest != ' ' && i < 255) srcpath[i++] = *rest++;
            srcpath[i] = 0;
            while (*rest == ' ') rest++;
            make_path(srcpath, srcpath);
            make_path(rest, destpath);

            if (fs_copy(srcpath, destpath)) {
                vga_write("[Copied]\n");
            } else {
                vga_write("[Copy Failed]\n");
            }
        }
        else if (strcmp(cmdline, "cd ..") == 0) {
            int len = strlen(cwd);
            if (len > 1) {
                while (len > 0 && cwd[len-1] != '/') {
                    cwd[len-1] = 0;
                    len--;
                }
                if (len > 1) cwd[len-1] = 0;
            }
            vga_write("[Moved Up]\n");
        }
        else if (startswith(cmdline, "cd ")) {
            const char* path = cmdline + 3;
            if (path[0] == '/') {
                strcpy(cwd, path);
            } else {
                strcat(cwd, "/");
                strcat(cwd, path);
            }
            vga_write("[Changed Directory]\n");
        }
        else if (startswith(cmdline, "run ")) {
            char path[256];
            make_path(cmdline + 4, path);
            uint8_t bin[65536];
            uint32_t size;
            if (fs_read(path, bin, &size)) {
                run_binary(bin, size);
            } else {
                vga_write("[Binary Not Found]\n");
            }
        }
        else {
            vga_write("[Unknown Command]\n");
        }
    }
}

#endif // NEUIX_USERLAND_H
