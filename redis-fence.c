#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#ifndef _REQUEST_H
#define _REQUEST_H
#define BUF_SIZE (1024*16)
#define REDIS_STAR  "*"
#define REDIS_DOLLAR  "$"
#define REDIS_CR  "\r"
#define REDIS_LF  "\n"

enum {
    STATE_CONTINUE,
    STATE_FAIL
};

typedef struct r_cmd {
    size_t pos;
    char *buff;
    size_t argc;
    char **argv;
    struct r_cmd *next;
} r_cmd_t;

typedef struct r_buff {
    size_t pos;
    char *buff;
    int cmd_cnt;
    struct r_cmd *cmd;
} r_buff_t;

void cmd_free(struct r_cmd *cmd);
void cmd_print(struct r_cmd *n);
void cmd_log(struct r_cmd *cmd);
int cmd_state_len(struct r_cmd *req, char *sb);
int cmd_parse(struct r_cmd *req, struct r_buff *pBuff);

#endif

int cmd_state_len(struct r_cmd *req, char *sb) {

    int term = 0, first = 0;
    char c;
    size_t i = req->pos;
    size_t pos = i;

    while((c = req->buff[i]) != '\0') {
        first++;
        pos++;
        switch(c) {
            case '\r':
                term = 1;
                break;
            case '\n':
                if (term) {
                    req->pos = pos;
                    return STATE_CONTINUE;
                }
                else
                    return STATE_FAIL;
            default:
                if (first == 1) {
                    if (c != *REDIS_DOLLAR && c != *REDIS_STAR && c != *REDIS_CR && c != *REDIS_LF)
                        return STATE_FAIL;
                } else {
                    if (c >= '0' && c <= '9') {
                        *sb++ = c;
                    } else {
                        return STATE_FAIL;
                    }
                }
                break;
        }
        i++;
    }
    return STATE_FAIL;
}

struct r_buff *buffer_new(char *buff) {

    struct r_buff *r_buff;
    r_buff = calloc(1, sizeof(struct r_buff));
    r_buff->buff = buff;
    return r_buff;
}

void cmd_free_all(struct r_cmd *cmd) {

    while (cmd != NULL) {
        struct r_cmd *tmp = cmd;
        cmd = cmd->next;
        cmd_free(tmp);

    }
}

void buffer_free(struct r_buff * buffer) {

    cmd_free_all(buffer->cmd);
    free(buffer);
}

void cmd_free(struct r_cmd *cmd) {

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

int cmd_parse(struct r_cmd *req, struct r_buff *pBuff) {

    int i;
    char sb[BUF_SIZE] = {0};
    char *ptr, *ptr1;

    if (cmd_state_len(req, sb) != STATE_CONTINUE) {
        printf("\r\rargc format error:%s", sb);
        return false;
    }
    req->argc = (size_t) strtol(sb, &ptr, 0);
    req->argv  =(char**)calloc(req->argc, sizeof(char*));

    for (i = 0; i < req->argc; i++) {
        size_t argv_len;
        char *v;
        memset(sb, 0, BUF_SIZE);
        if (cmd_state_len(req, sb) != STATE_CONTINUE) {
            printf("\r\rargv's length error, packet:%s", sb);
            return false;
        }
        argv_len = (size_t) strtol(sb, &ptr1, 0);

        if (argv_len < BUF_SIZE) { //todo  <--- this is product of tirednes and stupidity FIX IT!!!!
            v = (char *) malloc(sizeof(char) * argv_len + 1);
            memcpy(v, req->buff + (req->pos), argv_len);
            v[argv_len] = '\0';
            req->argv[i] = v;
        }
        req->pos += argv_len + 2;
    }

    pBuff->pos += req->pos;
    pBuff->cmd_cnt += 1;

    return (int) ((pBuff->buff + pBuff->pos)[0] == *REDIS_STAR ? STATE_CONTINUE : STATE_FAIL);
}

void buffer_parse(struct r_buff * buffer) {

    buffer->cmd = calloc(1, sizeof(struct r_cmd));
    buffer->cmd->buff = buffer->buff;

    printf("Buffer continue?: [%d]\n", cmd_parse(buffer->cmd, buffer));

    buffer->cmd->next = calloc(1, sizeof(struct r_cmd));
    buffer->cmd->next->buff = buffer->buff+buffer->pos;

    printf("Buffer continue?: [%d]\n", cmd_parse(buffer->cmd->next, buffer));

    buffer->cmd->next->next = calloc(1, sizeof(struct r_cmd));
    buffer->cmd->next->next->buff = buffer->buff+buffer->pos;

    printf("Buffer continue?: [%d]\n", cmd_parse(buffer->cmd->next->next, buffer));

    printf("Buffer position: [%d]\n", (int) buffer->pos);
    printf("Commands count: [%d]\n", (int) buffer->cmd_cnt);

    cmd_print(buffer->cmd);
    buffer_free(buffer);

}

void cmd_print(struct r_cmd *n) {
    while (n != NULL) {
        printf("------------------------------------");
        cmd_log(n);
        n = n->next;
    }
}

void cmd_log(struct r_cmd *cmd) {
    int i;
    if (cmd == NULL)
        return;
    printf("\nargc:<%d>\n", (int) cmd->argc);
    for (i = 0; i < cmd->argc; i++) {
        printf("\nargv[%d]:<%s>\n",i,cmd->argv[i]);
    }
}

int main(int argc, char *argv[]) {

    char * data = "*3\r\n$4\r\nhget\r\n$7\r\nprofile\r\n$10\r\nakul.musis\r\n*3\r\n$4\r\ntegh\r\n$7\r\nprofile\r\n$10\r\nluka.musin\r\n*3\r\n$4\r\ntttt\r\n$7\r\nprofile\r\n$10\r\nluka.musin\r\n";
//    char * data = "*3\r\n$4\r\nhget\r\n$7\r\nprofile\r\n$10\r\nakul.musin\r\n";



    struct r_buff *buffer = buffer_new(data);
    buffer_parse(buffer);

}
