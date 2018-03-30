#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef _command_H
#define _command_H

struct command {
    char *querybuf;
    int argc;
    char **argv;
    size_t pos;
    size_t failed;
    struct command *next;
};

struct redis {
    size_t count;
    struct command *root;
    char *tail;
    size_t failed;
};

int  command_parse(struct command *cmd);
void command_free(struct command *cmd);
void command_dump(struct command *cmd);
void print_commands(struct command *root);
void free_commands(struct command *root);
struct command *request_parse(char *req_data);

#endif

#define BUF_SIZE (1024*10)

#define REDIS_STAR '*'

enum {
    STATE_CONTINUE,
    STATE_FAIL
};

enum {
    OK,
    FAILED
};

int req_state_len(struct command *cmd, char *sb) {

    int term = 0, first = 0;
    char c;
    int i = (int) cmd->pos;
    int pos = i;

    while((c = cmd->querybuf[i]) != '\0') {
        first++;
        pos++;
        switch(c) {
            case '\r':
                term = 1;
                break;

            case '\n':
                if (term) {
                    cmd->pos = (size_t) pos;
                    return STATE_CONTINUE;
                }
                else
                    return STATE_FAIL;
            default:
                if (first == 1) {
                    if (c != '*' && c != '$')
                        return STATE_FAIL;
                } else {
                    /* the symbol must be numeral */
                    if (c >= '0' && c <= '9')
                        *sb++ = c;
                    else
                        return STATE_FAIL;
                }
                break;
        }
        i++;
    }

    return STATE_FAIL;
}


int command_parse(struct command *cmd) {

    int i;
    char sb[BUF_SIZE] = {0};

    if (req_state_len(cmd, sb) != STATE_CONTINUE) {
        printf("argc format ***ERROR***,packet:%s\n", sb);
        return FAILED;
    }
    cmd->argc = atoi(sb);

    cmd->argv = (char**) calloc((size_t)cmd->argc, sizeof(char*));
    for (i = 0; i < cmd->argc; i++) {
        int argv_len;
        char *v;

        /* parse argv len */
        memset(sb, 0, BUF_SIZE);
        if (req_state_len(cmd, sb) != STATE_CONTINUE) {
            printf("argv's length format ***ERROR***, packet:%s\n", sb);
            return FAILED;
        }
        argv_len = atoi(sb);

        /* get argv */
        v = (char*) malloc(sizeof(char)* argv_len);
        memcpy(v, cmd->querybuf + (cmd->pos), argv_len);
        cmd->argv[i] = v;
        cmd->pos += argv_len + 2;
    }
    return OK;
}


struct command *request_parse(char *req_data) {

    int i = 0;
    char c;
    struct command *root = NULL, **ppCmd = &root;

    while((c = req_data[i]) != '\0') {

        if (c == REDIS_STAR) {
            printf("Star found at position [%d]\n", i);

            *ppCmd = calloc(1, sizeof(struct command));
            (*ppCmd)->querybuf = &req_data[i];

            if(command_parse(*ppCmd) != OK)
                (*ppCmd)->failed = 1;

            /* jump over bytes already consumed by command_parse... */
            if((*ppCmd)->pos > 1)
                i += (int) (*ppCmd)->pos - 1;

            printf("Current parse position [%d]\n", i);
            ppCmd = &(*ppCmd)->next;
        }
        i++;
    }
    /* terminate linked list's last node */
    *ppCmd = NULL;
    /* return linked list's root */
    return root;
}

void print_commands(struct command * root) {
    struct command *ptr = root;
    while (ptr) {
        command_dump(ptr);
        ptr = ptr->next;
    }
}

void free_commands(struct command * root) {
    struct command *ptr = root;
    while (ptr) {
        struct command *tmp = ptr;
        ptr = ptr->next;
        command_free(tmp);
    }
}

void command_dump(struct command *cmd) {
    int i;
    if (cmd == NULL)
        return;

    printf("command-dump--->");
    if(cmd->failed) {

        printf("command-failed--->");
        printf("command-tail--->[%s]\n", cmd->querybuf);

    } else {

        printf("argc:<%d>\n", cmd->argc);
        for (i = 0; i < cmd->argc; i++) {
            printf("argv[%d]:<%s>\n", i, cmd->argv[i]);
        }
    }
    printf("\n");
}

void command_free(struct command *cmd) {
    int i;
    if (cmd) {
        for (i = 0; i < cmd->argc; i++) {
            if (cmd->argv[i])
                free(cmd->argv[i]);
        }
        free(cmd->argv);
        free(cmd);
    }
}


void test_parser(char *data) {

    struct command *root = request_parse(data);
    print_commands(root);
    free_commands(root);
}


int main(int argc, char *argv[]) {

    test_parser("*3\r\n$4\r\nhget\r\n$7\r\nprofile\r\n$10\r\nakul.musis\r\n*3\r\n$4\r\ntegh\r\n$7\r\nprofile\r\n$10\r\nluka.musin\r\n*3\r\n$4\r\ntttt\r\n$7\r\nprofile\r\n$10\r\nluka.musin\r\n");
    test_parser("*3\r\n$4\r\nhget\r\n$7\r\nprofile\r\n$10\r\nakul.musin\r\n");
    test_parser("*3\r\n$4\r\nsrem\r\n$28\r\ntarget:owner(luka.musin).idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nsrem\r\n$18\r\ntarget:realm().idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nsrem\r\n$20\r\ntarget:tag(keks).idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nsrem\r\n$20\r\ntarget:tag(poks).idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nsrem\r\n$21\r\ntarget:hostname().idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nsrem\r\n$26\r\ntarget:template(false).idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nsrem\r\n$10\r\ntarget.idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nhdel\r\n$6\r\ntarget\r\n$5\r\nkoloe\r\n*2\r\n$4\r\nincr\r\n$8\r\nrevision\r\n*1\r\n$4\r\nexec\r\n");
    test_parser("*3\r\n$11\r\nsinterstore\r\n$29\r\ntmp_sinterstore@1521640901:94\r\n$10\r\ntarget.idx\r\n");
    test_parser("*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n");
    test_parser("*2\r\n$4720\r\ndasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\r\n$1\r\n*\r\n");
    test_parser("*2\r\n$4\r\nkeys\r\n$4720\r\ndasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd");
    test_parser("kolon*****\r\n$5aoklo*3\r\n$4\r\nhget\r\n$7\r\nprofile\r\n$10\r\nakul.musis\r\n*3\r\n$4\r\ntegh\r\n$7\r\nprofile\r\n$10\r\nluka.musin\r\n*3\r\n$4\r\ntttt\r\n$7\r\nprofile\r\n$10\r\nluka.musin\r\n");
    test_parser("*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n");
    test_parser("*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n");
    test_parser("*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n*2\r\n$4\r\nkeyl\r\n");
    test_parser("*2\r\n$4\r\nkeys");
    test_parser("2\r\n$4\r\nkeys\r\n$1\r\n\r\n2\r\n$4\r\nkeys\r\n$1\r\n\r\n2\r\n$4\r\nkeyl\r\n");

}
