
#include "common.h"

void gotoxy(int x, int y)
{
    printf("\033[%d;%dH", x, y);
}

void init_common(Common * p_common)
{
    init_MessageCont(&(p_common->cont));
    p_common->server_pid = getpid();
}

void init_MessageCont(MessageCont * p_cont)
{
    p_cont->start_idx = 0;
    p_cont->msg_num = 0;
}

void push_MessageCont(MessageCont * p_cont, char * msg)
{
    int idx = (p_cont->start_idx + p_cont->msg_num) % MAX_MSG;
    if(p_cont->start_idx == idx) {
        ++(p_cont->start_idx);
        --(p_cont->msg_num);
    }
    strcpy_cnt(p_cont->msg[idx].msg, msg, MSG_SIZE);
    ++(p_cont->msg_num);
}

void strcpy_cnt(char * dest, char * src, int max_len)
{
    int idx = 0;
    for(char * pch = src; *pch != '\0' && idx < max_len; ++pch, ++idx) {
        dest[idx] = *pch;
    }
    if(idx < max_len) dest[idx] = '\0';
}
