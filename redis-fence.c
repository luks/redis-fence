#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#ifndef _REQUEST_H
#define _REQUEST_H
#define BUF_SIZE (1024*16)
#define REDIS_STAR  "*"
#define REDIS_DOLLAR  "$"
#define REDIS_CR  "\r"
#define REDIS_LF  "\n"

enum {
    STATE_START,
    STATE_CONTINUE,
    STATE_FAIL,
    ABORT
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
    size_t len;
    size_t start;
    char * tail;
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
bool buffer_parse(struct r_buffer * buffer);
struct r_buffer *buffer_new(char *buff);
void test_parser(char * data);
void buffer_free(struct r_buffer *buffer);
#endif

void cmd_print(struct r_cmd *cmd) {
    while (cmd != NULL) {
        printf("\r\n");
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

bool find_command(struct r_cmd *cmd) {

    if((cmd->buff + cmd->pos) != NULL) {
        if (cmd->buff[cmd->pos] == *REDIS_STAR) {
            return true;
        }
    }
    return false;
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

    cmd->buff = pBuff->buff + pBuff->pos;

    int i;
    char sb[BUF_SIZE] = {0};
    char *ptr, *ptr1;

    if(!find_command(cmd)) {

        while(cmd->pos < pBuff->len) {
            cmd->pos++;
            if (find_command(cmd)) {
                pBuff->start = cmd->pos;
                if (cmd_state_len(cmd, sb) == STATE_CONTINUE) {
                    break;
                }
            }
        }
    } else {
        cmd_state_len(cmd, sb);
    }

    cmd->argc = (size_t) strtol(sb, &ptr, 0);
    cmd->argv  =(char**)calloc(cmd->argc, sizeof(char*));
    bool parse_failed = false;
    for (i = 0; i < cmd->argc; i++) {
        size_t argv_len;
        char *v;
        memset(sb, 0, BUF_SIZE);

        if (cmd_state_len(cmd, sb) != STATE_CONTINUE) {
            pBuff->tail = cmd->buff;
            parse_failed = true;
            continue;
        }
        argv_len = (size_t) strtol(sb, &ptr1, 0);

        v = (char *) malloc(sizeof(char) * argv_len + 1);
        memcpy(v, cmd->buff + (cmd->pos), argv_len);
        v[argv_len] = '\0';

        cmd->argv[i] = v;
        cmd->pos += argv_len + 2;
    }

    pBuff->pos += cmd->pos;
    if(!parse_failed) {
        pBuff->cmd_cnt += 1;
    }

    if((pBuff->buff + pBuff->pos)[0] == *REDIS_STAR) {
        struct r_cmd *tmp = cmd->next = calloc(1, sizeof(struct r_cmd));
        cmd_parse_recursive(tmp, pBuff);
    }
}

struct r_buffer *buffer_new(char *buff) {

    struct r_buffer *r_buff;
    r_buff = calloc(1, sizeof(struct r_buffer));
    r_buff->buff = buff;
    r_buff->len = strlen(buff);
    return r_buff;
}

bool buffer_parse(struct r_buffer * buffer) {

    buffer->cmd = calloc(1, sizeof(struct r_cmd));
    if(cmd_parse_recursive(buffer->cmd, buffer) == ABORT) {
        return false;
    }
    return true;
}

int state_machine(struct r_cmd *cmd, char *sb) {

    int term = 0, first = 0;
    char c;
    size_t i = 0;
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
                    if (c != *REDIS_DOLLAR &&
                        c != *REDIS_STAR &&
                        c != *REDIS_CR &&
                        c != *REDIS_LF)
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

int test_command(struct r_cmd *cmd) {

    char sb[BUF_SIZE] = {0};

    printf( "Buffer will be sent to state machine\n");

    if(state_machine(cmd, sb) == STATE_CONTINUE) {
        // we expect that first state will be digit
        if(isdigit(*sb)) {
            printf( "So far so good [%s]\n",  sb);
            printf( "Buffer will be processed\n");
        }
    } else {
        printf("Not so good [%s]\n", sb);
    }

}


int inspect_buffer(struct r_buffer *pBuff) {

    char  c;

    int i = 0;

    while((c = pBuff->buff[i]) != '\0') {

        if (c == *REDIS_STAR) {

            printf("Star found at position [%d]\n", i);

            pBuff->cmd = calloc(1, sizeof(struct r_cmd));
            pBuff->cmd->buff = &pBuff->buff[i];

            printf( "Buffer that will be sent to test method\n");
            test_command(pBuff->cmd);

        }
        i++;
    }
}

void test_parser(char * data) {

    struct r_buffer *buffer = buffer_new(data);
    inspect_buffer(buffer);
    buffer_free(buffer);
}


int main(int argc, char *argv[]) {
//    test_parser("*3\r\n$4\r\nhget\r\n$7\r\nprofile\r\n$10\r\nakul.musis\r\n*3\r\n$4\r\ntegh\r\n$7\r\nprofile\r\n$10\r\nluka.musin\r\n*3\r\n$4\r\ntttt\r\n$7\r\nprofile\r\n$10\r\nluka.musin\r\n");
//    test_parser("*3\r\n$4\r\nhget\r\n$7\r\nprofile\r\n$10\r\nakul.musin\r\n");
//    test_parser("*3\r\n$4\r\nsrem\r\n$28\r\ntarget:owner(luka.musin).idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nsrem\r\n$18\r\ntarget:realm().idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nsrem\r\n$20\r\ntarget:tag(keks).idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nsrem\r\n$20\r\ntarget:tag(poks).idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nsrem\r\n$21\r\ntarget:hostname().idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nsrem\r\n$26\r\ntarget:template(false).idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nsrem\r\n$10\r\ntarget.idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nhdel\r\n$6\r\ntarget\r\n$5\r\nkoloe\r\n*2\r\n$4\r\nincr\r\n$8\r\nrevision\r\n*1\r\n$4\r\nexec\r\n");
//    test_parser("*3\r\n$11\r\nsinterstore\r\n$29\r\ntmp_sinterstore@1521640901:94\r\n$10\r\ntarget.idx\r\n");
//    test_parser("*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n");
//    test_parser("*2\r\n$4720\r\ndasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\r\n$1\r\n*\r\n");
//    test_parser("*2\r\n$4\r\nkeys\r\n$4720\r\ndasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd");
//    test_parser("*3\r\n$4\r\nhget\r\n$7\r\nprofile\r\n$10\r\nakul.musis\r\n*3\r\n$4\r\ntegh\r\n$7\r\nprofile\r\n$10\r\nluka.musin\r\n*3\r\n$4\r\ntttt\r\n$7\r\nprofile\r\n$10\r\nluka.musin\r\n");
    //test_parser("*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n");
    //test_parser("*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n");
    //test_parser("\r\nss\rn\rnss*ssss*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n");
    test_parser("\r\nss\rn\rnss*ssss*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n*2\r\n$1\r\n$4\r\n234");
}
