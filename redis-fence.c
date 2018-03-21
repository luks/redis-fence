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

typedef struct r_buffer {
    size_t pos;
    char *buff;
    int cmd_cnt;
    struct r_cmd *cmd;
} r_buff_t;

void cmd_free(struct r_cmd *cmd);
void cmd_print(struct r_cmd *cmd);
void cmd_log(struct r_cmd *cmd);
int cmd_state_len(struct r_cmd *cmd, char *sb);
struct r_buffer *buffer_new(char *buff);
int cmd_parse_recursive(struct r_cmd *cmd, struct r_buffer *pBuff);

#endif

int cmd_state_len(struct r_cmd *cmd, char *sb) {

    int term = 0, first = 0;
    char c;
    size_t i = cmd->pos;
    size_t pos = i;

    while((c = cmd->buff[i]) != '\0') {
        first++;
        pos++;
        switch(c) {
            case '\r':
                term = 1;
                break;
            case '\n':
                if (term) {
                    cmd->pos = pos;
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

struct r_buffer *buffer_new(char *buff) {

    struct r_buffer *r_buff;
    r_buff = calloc(1, sizeof(struct r_buffer));
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

void buffer_free(struct r_buffer *buffer) {

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

int cmd_parse_recursive(struct r_cmd *cmd, struct r_buffer *pBuff) {

    cmd->buff = pBuff->buff+pBuff->pos;

    int i;
    char sb[BUF_SIZE] = {0};
    char *ptr, *ptr1;

    if (cmd_state_len(cmd, sb) != STATE_CONTINUE) {
        printf("\r\rargc format error:%s", sb);
        return false;
    }
    cmd->argc = (size_t) strtol(sb, &ptr, 0);
    cmd->argv  =(char**)calloc(cmd->argc, sizeof(char*));

    for (i = 0; i < cmd->argc; i++) {
        size_t argv_len;
        char *v;
        memset(sb, 0, BUF_SIZE);
        if (cmd_state_len(cmd, sb) != STATE_CONTINUE) {
            printf("\r\rargv's length error, packet:%s", sb);
            return false;
        }
        argv_len = (size_t) strtol(sb, &ptr1, 0);

        if (argv_len < BUF_SIZE) { //todo  <--- this is product of tirednes and stupidity FIX IT!!!!
            v = (char *) malloc(sizeof(char) * argv_len + 1);
            memcpy(v, cmd->buff + (cmd->pos), argv_len);
            v[argv_len] = '\0';
            cmd->argv[i] = v;
        }
        cmd->pos += argv_len + 2;
    }

    pBuff->pos += cmd->pos;
    pBuff->cmd_cnt += 1;
    if((pBuff->buff + pBuff->pos)[0] == *REDIS_STAR) {
        struct r_cmd *tmp = cmd->next = calloc(1, sizeof(struct r_cmd));
        cmd_parse_recursive(tmp, pBuff);
    } else {
        return STATE_FAIL;
    }
}


void buffer_parse(struct r_buffer * buffer) {

    buffer->cmd = calloc(1, sizeof(struct r_cmd));
    cmd_parse_recursive(buffer->cmd, buffer);

}

void cmd_print(struct r_cmd *cmd) {
    while (cmd != NULL) {
        printf("------------------------------------");
        cmd_log(cmd);
        cmd = cmd->next;
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

void test_parser(char * data) {

    struct r_buffer *buffer = buffer_new(data);
    buffer_parse(buffer);
    cmd_print(buffer->cmd);
    printf("\r\n");
    printf("Buffer position: [%d]\n", (int) buffer->pos);
    printf("Commands count: [%d]\n", (int) buffer->cmd_cnt);
    printf("_____________________________________________________\n");
    buffer_free(buffer);
}

int main(int argc, char *argv[]) {

    test_parser("*3\r\n$4\r\nhget\r\n$7\r\nprofile\r\n$10\r\nakul.musis\r\n*3\r\n$4\r\ntegh\r\n$7\r\nprofile\r\n$10\r\nluka.musin\r\n*3\r\n$4\r\ntttt\r\n$7\r\nprofile\r\n$10\r\nluka.musin\r\n");
    test_parser("*3\r\n$4\r\nhget\r\n$7\r\nprofile\r\n$10\r\nakul.musin\r\n");
    test_parser("*3\r\n$4\r\nsrem\r\n$28\r\ntarget:owner(luka.musin).idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nsrem\r\n$18\r\ntarget:realm().idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nsrem\r\n$20\r\ntarget:tag(keks).idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nsrem\r\n$20\r\ntarget:tag(poks).idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nsrem\r\n$21\r\ntarget:hostname().idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nsrem\r\n$26\r\ntarget:template(false).idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nsrem\r\n$10\r\ntarget.idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nhdel\r\n$6\r\ntarget\r\n$5\r\nkoloe\r\n*2\r\n$4\r\nincr\r\n$8\r\nrevision\r\n*1\r\n$4\r\nexec\r\n");
    test_parser("*3\r\n$11\r\nsinterstore\r\n$29\r\ntmp_sinterstore@1521640901:94\r\n$10\r\ntarget.idx\r\n");

}
