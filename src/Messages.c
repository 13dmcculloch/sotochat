#include <string.h>
#include <stdio.h>

#include "Messages.h"

/* [username]\0/xxxx\0/[message]\0 */
int serialise_message(Message *msg, char *send_buf)
{
    size_t idx = 0;

    /* copy username */
    if(!memcpy(send_buf, msg->username, sizeof(msg->username)))
    {
        fputs("Username copy error\n", stderr);
        return 1;
    }

    idx += sizeof(msg->username);

    /* serialise message length */
    char len_s[7];  /* /max 1024/'\0' */
    sprintf(len_s, "/%04d/", msg->message_len);

    if(!memcpy(&send_buf[idx], len_s, sizeof(len_s)))
    {
        fputs("Len copy error\n", stderr);
        return 1;
    }

    idx += sizeof(len_s);

    /* copy message */
    if(!memcpy(&send_buf[idx], msg->message, sizeof(msg->message)))
    {
        fputs("Message copy error\n", stderr);
        return 1;
    }

    return 0;
}

Message deserialise_message(char *recv_buf)
{
    Message msg;
    size_t idx = 0;

    memcpy(msg.username, recv_buf, sizeof(msg.username));
    idx += sizeof(msg.username);

    sscanf(&recv_buf[idx], "/%04d/", &msg.message_len);
    idx += sizeof("/xxxx/");

    memcpy(msg.message, &recv_buf[idx], sizeof(msg.message));

    if(msg.message_len != strlen(msg.message))
    {
        fputs("Warning: length mismatch.\n", stderr);
    }
    
    return msg;
}
