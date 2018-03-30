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

int  command_parse(struct command *command);
void command_free(struct command *command);
void command_dump(struct command *command);
void print_commands(struct command * root);
void free_commands(struct command * root);
struct command *request_parse(char *reqbuff);

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

int req_state_len(struct command *req,char *sb) {

    int term = 0, first = 0;
    char c;
    int i = (int) req->pos;
    int pos = i;

    while((c = req->querybuf[i]) != '\0') {
        first++;
        pos++;
        switch(c) {
            case '\r':
                term = 1;
                break;

            case '\n':
                if (term) {
                    req->pos = (size_t) pos;
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


int command_parse(struct command *req) {

    int i;
    char sb[BUF_SIZE] = {0};

    if (req_state_len(req, sb) != STATE_CONTINUE) {
        //fprintf(stderr, "argc format ***ERROR***,packet:%s\n", sb);
        printf("argc format ***ERROR***,packet:%s\n", sb);
        return FAILED;
    }
    req->argc = atoi(sb);

    req->argv = (char**) calloc((size_t)req->argc, sizeof(char*));
    for (i = 0; i < req->argc; i++) {
        int argv_len;
        char *v;

        /* parse argv len */
        memset(sb, 0, BUF_SIZE);
        if (req_state_len(req, sb) != STATE_CONTINUE) {
            //fprintf(stderr, "argv's length format ***ERROR***, packet:%s\n", sb);
            printf("argv's length format ***ERROR***, packet:%s\n", sb);
            return FAILED;
        }
        argv_len = atoi(sb);

        /* get argv */
        v = (char*) malloc(sizeof(char)* argv_len);
        memcpy(v, req->querybuf + (req->pos), argv_len);
        req->argv[i] = v;
        req->pos += argv_len + 2;
    }
    return OK;
}


struct command *request_parse(char *request) {

    int i = 0;
    char c;
    struct command *root = NULL, **ppReq = &root;

    while((c = request[i]) != '\0') {

        if (c == REDIS_STAR) {
            printf("Star found at position [%d]\n", i);

            *ppReq = calloc(1, sizeof(struct command));
            (*ppReq)->querybuf = &request[i];

            if(command_parse(*ppReq) != OK)
                (*ppReq)->failed = 1;

            /* jump over bytes already consumed by command_parse... */
            if((*ppReq)->pos > 1)
                i += (int) (*ppReq)->pos - 1;

            printf("Current parse position [%d]\n", i);
            ppReq = &(*ppReq)->next;
        }
        i++;
    }
    /* terminate linked list's last node */
    *ppReq = NULL;
    /* return linked list root */
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

void command_dump(struct command *req) {
    int i;
    if (req == NULL)
        return;

    printf("command-dump--->");
    if(req->failed) {

        printf("command-failed--->");
        printf("command-tail--->[%s]\n", req->querybuf);

    } else {

        printf("argc:<%d>\n", req->argc);
        for (i = 0; i < req->argc; i++) {
            printf("argv[%d]:<%s>\n", i, req->argv[i]);
        }
    }
    printf("\n");
}

void command_free(struct command *req) {
    int i;
    if (req) {
        for (i = 0; i < req->argc; i++) {
            if (req->argv[i])
                free(req->argv[i]);
        }
        free(req->argv);
        free(req);
    }
}


void test_parser(char * data) {

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
