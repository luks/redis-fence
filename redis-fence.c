#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef _REQUEST_H
#define _REQUEST_H

struct request{
    char *querybuf;
    int argc;
    char **argv;
    size_t pos;
};

struct request *request_new(char *querybuf);
int  request_parse(struct request *request);
void request_free(struct request *request);
void request_dump(struct request *request);

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

struct request *request_new(char *querybuf) {
    struct request *req;
    req=calloc(1, sizeof(struct request));
    req->querybuf = querybuf;
    return req;
}


int req_state_len(struct request *req,char *sb) {
    int term=0, first=0;
    char c;
    int i = req->pos;
    int pos=i;

    while((c = req->querybuf[i]) != '\0') {
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
                    /* the first symbol is not '*' or '$'*/
                    if (c != '$' && c != '*' && c != '\r' && c != '\n')
                        return STATE_FAIL;
                } else {
                    /* the symbol must be numeral*/
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


int request_parse(struct request *req) {
    int i;
    char sb[BUF_SIZE] = {0};

    if (req_state_len(req, sb) != STATE_CONTINUE) {
        fprintf(stderr, "argc format ***ERROR***,packet:%s\n", sb);
        return FAILED;
    }
    req->argc = atoi(sb);

    req->argv  =(char**)calloc(req->argc, sizeof(char*));
    for (i = 0; i < req->argc; i++) {
        int argv_len;
        char *v;

        /*parse argv len*/
        memset(sb, 0, BUF_SIZE);
        if (req_state_len(req, sb) != STATE_CONTINUE) {
            fprintf(stderr, "argv's length format ***ERROR***,packet:%s\n", sb);
            return FAILED;
        }
        argv_len=atoi(sb);

        /*get argv*/
        v=(char*)malloc(sizeof(char) * argv_len);
        memcpy(v, req->querybuf+(req->pos), argv_len);
        req->argv[i] = v;
        req->pos += argv_len+2;
    }
    return OK;
}


int request_multiple_parse(char *data) {

    int i = 0;

    char c;

    while((c = data[i]) != '\0') {

        if (c == REDIS_STAR) {

            printf("Star found at position [%d]\n", i);

            struct request *req = request_new(&data[i]);

            if(request_parse(req) != OK) {
                printf("Error parse request at position [%d] \n", i);
            };

            // jump over bytes already parsed
            if(req->pos > 1) {
                i += (int) req->pos - 1;
            }

            request_dump(req);
            request_free(req);

        }
        printf("Current position [%d]\n", i);
        i++;
    }

}


void request_dump(struct request *req) {
    int i;
    if (req == NULL)
        return;

    printf("request-dump--->");
    //printf("request-tail--->[%s]", req->querybuf);
    printf("argc:<%d>\n", req->argc);
    for (i = 0; i < req->argc; i++) {
        printf("argv[%d]:<%s>\n",i,req->argv[i]);
    }
    printf("\n");
}

void request_free(struct request *req) {
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
    //struct request *buffer = request_new(data);
    request_multiple_parse(data);
    //request_dump(buffer);
    //request_free(buffer);

}


int main(int argc, char *argv[]) {



//    test_parser("*3\r\n$4\r\nhget\r\n$7\r\nprofile\r\n$10\r\nakul.musis\r\n*3\r\n$4\r\ntegh\r\n$7\r\nprofile\r\n$10\r\nluka.musin\r\n*3\r\n$4\r\ntttt\r\n$7\r\nprofile\r\n$10\r\nluka.musin\r\n");
//    test_parser("*3\r\n$4\r\nhget\r\n$7\r\nprofile\r\n$10\r\nakul.musin\r\n");
//    test_parser("*3\r\n$4\r\nsrem\r\n$28\r\ntarget:owner(luka.musin).idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nsrem\r\n$18\r\ntarget:realm().idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nsrem\r\n$20\r\ntarget:tag(keks).idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nsrem\r\n$20\r\ntarget:tag(poks).idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nsrem\r\n$21\r\ntarget:hostname().idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nsrem\r\n$26\r\ntarget:template(false).idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nsrem\r\n$10\r\ntarget.idx\r\n$5\r\nkoloe\r\n*3\r\n$4\r\nhdel\r\n$6\r\ntarget\r\n$5\r\nkoloe\r\n*2\r\n$4\r\nincr\r\n$8\r\nrevision\r\n*1\r\n$4\r\nexec\r\n");
//    test_parser("*3\r\n$11\r\nsinterstore\r\n$29\r\ntmp_sinterstore@1521640901:94\r\n$10\r\ntarget.idx\r\n");
//    test_parser("*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n");
//    test_parser("*2\r\n$4720\r\ndasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\r\n$1\r\n*\r\n");
//    test_parser("*2\r\n$4\r\nkeys\r\n$4720\r\ndasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddasdadasdasdddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd");
    test_parser("kolon*****\r\n$5aoklo*3\r\n$4\r\nhget\r\n$7\r\nprofile\r\n$10\r\nakul.musis\r\n*3\r\n$4\r\ntegh\r\n$7\r\nprofile\r\n$10\r\nluka.musin\r\n*3\r\n$4\r\ntttt\r\n$7\r\nprofile\r\n$10\r\nluka.musin\r\n");
    //test_parser("*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n");
   // test_parser("*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n");
   // test_parser("*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n*2\r\n$4\r\nkey");



}
