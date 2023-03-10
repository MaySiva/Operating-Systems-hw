#ifndef MESSAGE_SLOT_H_
#define MESSAGE_SLOT_H_

#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUMBER,1,unsigned int)
#define MAX_MESSAGE_SIZE 128
#define MAJOR_NUMBER 235
#define MESSAGE_SLOT "message_slot"
#define MAX_OF_CHANNEL 1048576



struct channel
{

    unsigned long channel_id;
    int last_msg_len;
    char *last_message_data; 
    struct channel *next;

};
typedef struct channel CHANNEL;
typedef CHANNEL *CHANNEL_LINK; 

struct list_of_channels
{
    CHANNEL *head;
    int curr_num_of_channels;

};
typedef struct list_of_channels LIST_OF_CHANNELS;


#endif
