/* Message struct and functions
Todo:
    - TLV encode everything?

Douglas McCulloch, October 2023
*/

#ifndef MESSAGE_H
#define MESSAGE_H

#define USERNAME_LEN 32
#define MESSAGE_LEN 1024

#define MESSAGE_BUF_LEN MESSAGE_LEN + USERNAME_LEN + 10

typedef struct
{
    char username[USERNAME_LEN];
    int message_len;  /* actual message length */
    char message[MESSAGE_LEN];
} Message;

/* Message serialisation functions */
extern int serialise_message(Message *msg, char *send_buf);
extern Message deserialise_message(char *recv_buf);

#endif
