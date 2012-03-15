#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/file.h>
#include <sys/sysinfo.h>
#include <sys/epoll.h>
#include <signal.h>
//#include <linux/inotify.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/inotify.h>
struct queue_struct
{
  int capacity;
  int front;
  int rear;
  int size;
  void **array;
};
struct queue_struct;
typedef struct queue_struct *queue_t;
void queue_make_empty (queue_t q);
int queue_empty (queue_t q)
{
    return q->size == 0;
}
int queue_full (queue_t q)
{
    return q->size == q->capacity;
}
queue_t queue_create (int num_elements)
{
    queue_t q;
    q = malloc (sizeof (struct queue_struct));
    if (q == NULL)
      exit (-1);
    q->array = malloc(sizeof (void *) * num_elements);
    if (q->array == NULL)
      exit (-1);
    q->capacity = num_elements;
    queue_make_empty (q);
    return q;
}
void queue_destroy (queue_t q)
{
    if (q != NULL)
    {
        if (q->array)
	  free (q->array);
        free (q);
    }
}
void queue_make_empty (queue_t q)
{
    q->size = 0;
    q->front = 1;
    q->rear = 0;
}
static int next_position (int v, queue_t q)
{
    if (++v == q->capacity) {
        v = 0;
    }
    return v;
}
void queue_enqueue (void *d, queue_t q)
{
    if (queue_full (q))
    {
        return;
    }
    q->size++;
    q->rear = next_position (q->rear, q);
    q->array[q->rear] = d;
}
void *queue_front (queue_t q)
{
    if (!queue_empty(q))
      return q->array [q->front];
    return NULL;
}
void queue_dequeue (queue_t q)
{
    if (!queue_empty (q))
    {
        q->size--;
        q->front = next_position (q->front, q);
    }
}
/* This method does the dirty work of determining what happened,
   then allows us to act appropriately
*/
void handle_event (struct inotify_event *event)
{
    /* If the event was associated with a filename, we will store it here */
    char * cur_event_filename = NULL;
    /* This is the watch descriptor the event occurred on */
    int cur_event_wd = event->wd;
    if (event->len)
    {
        cur_event_filename = event->name;
    }
    printf("FILENAME=%s\n", cur_event_filename);
    printf("watch descriptor=%d\n",cur_event_wd);
    printf("\n");
    /* Perform event dependent handler routines */
    /* The mask is the magic that tells us what file operation occurred */
    switch (event->mask)
    {
	/* File was accessed */
    case IN_ACCESS:
      printf("ACCESS EVENT OCCURRED: File \"%s\" on WD #%i\n",
	     cur_event_filename, cur_event_wd);
      break;
      /* File was modified */
    case IN_MODIFY:
      printf("MODIFY EVENT OCCURRED: File \"%s\" on WD #%i\n",
	     cur_event_filename, cur_event_wd);
      break;
      /* File changed attributes */
    case IN_ATTRIB:
      printf("ATTRIB EVENT OCCURRED: File \"%s\" on WD #%i\n",
	     cur_event_filename, cur_event_wd);
      break;
      /* File was closed */
    case IN_CLOSE:
      printf("CLOSE EVENT OCCURRED: File \"%s\" on WD #%i\n",
	     cur_event_filename, cur_event_wd);
      break;
      /* File was opened */
    case IN_OPEN:
      printf("OPEN EVENT OCCURRED: File \"%s\" on WD #%i\n",
	     cur_event_filename, cur_event_wd);
      break;
      /* File was moved from X */
    case IN_MOVED_FROM:
      printf("MOVE_FROM EVENT OCCURRED: File \"%s\" on WD #%i\n",
	     cur_event_filename, cur_event_wd);
      break;
      /* File was moved to X */
    case IN_MOVED_TO:
      printf("MOVE_TO EVENT OCCURRED: File \"%s\" on WD #%i\n",
	     cur_event_filename, cur_event_wd);
      break;
      /* Watched entry was deleted */
    case IN_DELETE_SELF:
      printf("DELETE_SELF EVENT OCCURRED: File \"%s\" on WD #%i\n",
	     cur_event_filename, cur_event_wd);
      break;
      /* Backing FS was unmounted */
    case IN_UNMOUNT:
      printf("UNMOUNT EVENT OCCURRED: File \"%s\" on WD #%i\n",
	     cur_event_filename, cur_event_wd);
      break;
      /* Too many FS events were received without reading them
	 some event notifications were potentially lost. */
    case IN_Q_OVERFLOW:
      printf("Warning: AN OVERFLOW EVENT OCCURRED: \n");
      break;
    case IN_IGNORED:
      printf("IGNORED EVENT OCCURRED: \n");
      break;
      /* Some unknown message received */
    default:
      printf ("UNKNOWN EVENT OCCURRED for file \"%s\" on WD #%i\n",
	      cur_event_filename, cur_event_wd);
      break;
    }
}
void handle_events (queue_t q)
{
    struct inotify_event *event;
    while (!queue_empty (q))
    {
        event = queue_front (q);
        queue_dequeue (q);
        handle_event (event);
        free (event);
    }
}
int event_check (int fd)
{
    struct timeval timeout;
    int r;
    fd_set rfds;
    timeout.tv_sec = 4;
    timeout.tv_usec = 0;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    r = select (fd+1, &rfds, NULL, NULL, &timeout);
    return r;
}
int read_events (queue_t q, int fd)
{
    char buffer[16384];
    size_t buffer_i;
    struct inotify_event *pevent, *event;
    ssize_t r;
    size_t event_size;
    int count = 0;
    r = read (fd, buffer, 16384);
    if (r <= 0)
      return r;
    buffer_i = 0;
    while (buffer_i < r)
    {
        /* Parse events and queue them ! */
        pevent = (struct inotify_event *)&buffer[buffer_i];
        event_size = sizeof(struct inotify_event) + pevent->len;
        event = malloc(event_size);
        memmove(event, pevent, event_size);
        queue_enqueue(event, q);
        buffer_i += event_size;
        count++;
    }
    return count;
}
int process_inotify_events (queue_t q, int fd)
{
    while (1)
    {
        if (!queue_empty(q))
        {
            handle_events (q);
        }
        if (event_check(fd) > 0)
        {
            int r;
            r = read_events (q, fd);
            if (r < 0)
	      break;
        }
    }
    return 0;
}
int main()
{
    int fd;
    queue_t q;
    int wd;
    q = queue_create (128);
    fd=inotify_init();
    wd = inotify_add_watch (fd, "file", IN_MODIFY);
    printf("add watch: %d\n", wd);
    process_inotify_events (q,fd);
    queue_destroy (q);
    close(fd);
    return 0;
}
