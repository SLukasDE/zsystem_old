/*
MIT License

Copyright (c) 2004 Sven Lukas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <zsystem/zslib.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef linux
#include <errno.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include <netinet/in.h>
#define __USE_GNU
#include <sys/sem.h>
#include <sys/socket.h>
#include <sys/un.h> 
#include <sys/wait.h>
#include <sys/poll.h>
#include <sys/mman.h>

#include <linux/unistd.h>
#include <sys/types.h>
//_syscall0(pid_t,gettid)

#else
#include <winsock.h>
#endif
#include <arpa/inet.h>

#define ZS__STREAMTYPE_UNKNOWN 0
#define ZS__STREAMTYPE_INET 1
#define ZS__STREAMTYPE_LOCAL 2
#define ZS__STREAMTYPE_SOCKET 3
#define ZS__STREAMTYPE_PIPE 4

// ************************************************************************************************
// ***  Alle Strukturen definieren                                                              ***
// ************************************************************************************************

// !!!  Message Loop                                                                            !!!

struct zs__messageloop_element {
  struct zs__messageloop_element *next;
  void *data;
};

struct zs__messageloop {
  struct zs__semaphore *mutex;
  struct zs__messageloop_element *first;
  struct zs__messageloop_element *last;
  int aborting;
  void(*response)(void *instance, void *data);
  void *instance;
  zs__thread_handle thread_handle;
  void(*after_abort)(void*);
};

// !!!  Semaphore                                                                               !!!

struct zs__semaphore {
  unsigned char count;
  unsigned int *references;
  #ifdef linux // for Linux
  int sem;
  #else // for Win32
  HANDLE *sem;
  #endif
};

// !!!  Server                                                                                  !!!

struct zs__server {
  void *data;
  void(*on_connect)(struct zs__stream*, void*);
  struct zs__stream *stream_struct;
#ifdef linux
  int socket;
#else
  SOCKET socket;
#endif
};

// !!!  Stream                                                                                  !!!

struct zs__stream {
  unsigned int (*read)(struct zs__stream *a_stream, char *a_buffer, unsigned int a_size);
  unsigned int (*write)(struct zs__stream *a_stream, const char *a_buffer, unsigned int a_size);
  unsigned int (*state)(struct zs__stream *a_stream);
  void(*close_reader)(struct zs__stream*);
  void(*close_writer)(struct zs__stream*);
#ifdef linux
  int(*get_fd_reader)(struct zs__stream *a_stream_struct);
  int(*get_fd_writer)(struct zs__stream *a_stream_struct);
#endif
  int streamtype;
  unsigned char closed;

//  union {
  struct {
//    struct {
      int reader;
      int writer;
//    };
//    struct {
      int socket;
      char *read_buffer;
      unsigned int read_buffer_cur;
      unsigned int read_buffer_length;
      char remote_ipv4_address[4];
      int remote_port;
      char local_ipv4_address[4];
      int local_port;
//    };
  };
};
/*
struct zs__stream_pipe {
  unsigned int (*read)(struct zs__stream *a_stream, char *a_buffer, unsigned int a_size);
  unsigned int (*write)(struct zs__stream *a_stream, const char *a_buffer, unsigned int a_size);
  unsigned int (*state)(struct zs__stream *a_stream);
  void(*close_reader)(struct zs__stream*);
  void(*close_writer)(struct zs__stream*);
#ifdef linux
  int(*get_fd_reader)(struct zs__stream *a_stream_struct);
  int(*get_fd_writer)(struct zs__stream *a_stream_struct);
#endif
  int streamtype;
  unsigned char closed;
  
  int reader;
  int writer;
};

struct zs__stream_socket {
  unsigned int (*read)(struct zs__stream *a_stream, char *a_buffer, unsigned int a_size);
  unsigned int (*write)(struct zs__stream *a_stream, const char *a_buffer, unsigned int a_size);
  unsigned int (*state)(struct zs__stream *a_stream);
  void(*close_reader)(struct zs__stream*);
  void(*close_writer)(struct zs__stream*);
#ifdef linux
  int(*get_fd_reader)(struct zs__stream *a_stream_struct);
  int(*get_fd_writer)(struct zs__stream *a_stream_struct);
#endif
  int streamtype;
  unsigned char closed;

  char remote_ipv4_address[4];
  char local_ipv4_address[4];
  int socket;
};
*/

// !!!  Thread                                                                                  !!!

struct zs__thread {
  void *data;
  int(*run)(void *);
  void(*abort)(void *);
  pthread_mutex_t waiting_threads_mutex;
  pthread_cond_t  waiting_threads_cv;
  unsigned int    waiting_threads;
  zs__thread_handle thread_handle;
};

struct zs__thread_list {
  struct zs__thread *thread;
  struct zs__thread_list *next;
};

// !!!  Wait                                                                                    !!!

struct zs__wait_element {
  unsigned int number;
#ifdef linux
  int fd;
#endif
  struct zs__wait_element *next;
};

struct zs__wait {
  unsigned int max_number;
  unsigned int timer_number;
  long sec;
  long microsec;
  struct zs__wait_element *first;
  struct zs__wait_element *last;
};

// ************************************************************************************************
// ***  Alle lokalen Variablen definieren                                                       ***
// ************************************************************************************************

static struct zs__thread_list *zs__thread_map;
static pthread_mutex_t thread_map_mutex;

// ************************************************************************************************
// ***  Alle Funktionen deklarieren                                                             ***
// ************************************************************************************************

// !!!  Message Loop                                                                            !!!

static int zs__messageloop_run(void *a_data);
static void zs__messageloop_abort(void *a_data);

// EXPORT
struct zs__messageloop* zs__messageloop_create(void *a_instance, void(*a_response)(void *instance, void *data), void(*a_after_abort)(void *instance));
int zs__messageloop_post(struct zs__messageloop *a_messageloop, void *a_data);
zs__thread_handle zs__messageloop_get_thread_handle(struct zs__messageloop *a_messageloop);

// !!!  Semaphore                                                                               !!!

// EXPORT
struct zs__semaphore* zs__semaphore_create(unsigned char a_count);
struct zs__semaphore* zs__semaphore_duplicate(struct zs__semaphore *a_sem);
void zs__semaphore_destroy(struct zs__semaphore *a_sem);
void zs__semaphore_set(struct zs__semaphore *a_sem, unsigned char a_sem_id, int a_value);
int zs__semaphore_get(struct zs__semaphore *a_sem, unsigned char a_sem_id);
int zs__semaphore_op(struct zs__semaphore *a_sem, unsigned char a_sem_id, int a_value, unsigned long a_sec, unsigned long a_nano_sec);
// if operation was successful, 0 will be returend.
// if a timeout occured, -1 will be returned;
// if an error occured, -2 will be returned;


// !!!  Server                                                                                  !!!

// EXPORT
zs__thread_handle zs__server_create_local(const char *a_name, void *a_data, void(*a_on_connect)(struct zs__stream*, void*));
zs__thread_handle zs__server_create_tcp_ipv4(const char *a_ipv4, int a_port, void *a_data, void(*a_on_connect)(struct zs__stream*, void*));
zs__thread_handle zs__server_create_udp_ipv4(const char *a_ipv4, int a_port, void *a_data, void(*a_on_connect)(struct zs__stream*, void*));

// !!!  Stream                                                                                  !!!

static struct zs__stream* zs__stream_create(unsigned int(*)(struct zs__stream*, char*, unsigned int), unsigned int(*)(struct zs__stream*, const char*, unsigned int), unsigned int(*)(struct zs__stream *a_stream), void(*)(struct zs__stream*), void(*)(struct zs__stream*));
static struct zs__stream* zs__stream_create_socket(int a_socket, struct sockaddr*);

// EXPORT
struct zs__stream* zs__stream_create_pipe(void);
struct zs__stream* zs__stream_create_system(int a_reader, int a_writer);
//static struct zs__stream* zs__stream_create_filereader(int a_socket);
//static struct zs__stream* zs__stream_create_tcpip(char a_ip1, char a_ip2, char a_ip3, char a_ip4, int a_port);
struct zs__stream* zs__stream_create_tcp_ipv4(const char *a_src_ipv4, int a_src_port, const char *a_dst_ipv4, int a_dst_port);
struct zs__stream* zs__stream_create_udp_ipv4(const char *a_src_ipv4, int a_src_port, const char *a_dst_ipv4, int a_dst_port);
struct zs__stream* zs__stream_create_local(const char *a_name);
unsigned int zs__stream_read(struct zs__stream *a_stream, char *a_buffer, unsigned int a_size);
unsigned int zs__stream_write(struct zs__stream *a_stream, const char *a_buffer, unsigned int a_size);
unsigned int zs__stream_state(struct zs__stream *a_stream);
void zs__stream_close_reader(struct zs__stream *a_stream);
void zs__stream_close_writer(struct zs__stream *a_stream);
void zs__stream_destroy(struct zs__stream *a_stream);
const char* zs__stream_get_remote_ipv4_address(struct zs__stream *a_stream_struct);
const char* zs__stream_get_local_ipv4_address(struct zs__stream *a_stream_struct);
int zs__stream_get_remote_port(struct zs__stream *a_stream_struct);
int zs__stream_get_local_port(struct zs__stream *a_stream_struct);

// !!!  Process                                                                                 !!!

// EXPORT
int zs__process_execute(const char *a_file, char *const a_argv[], struct zs__stream *a_reader, struct zs__stream *a_writer, zs__process_handle *a_process_handle_ptr, void *a_data, void(*a_on_started)(void*));
void zs__process_terminate(zs__process_handle a_process_handle);
void zs__process_kill(zs__process_handle a_process_handle);


// !!!  Thread                                                                                  !!!

#ifdef linux // for Linux
static void* zs__thread_start(struct zs__thread *a_thread);
#else // for Win32
static DWORD WINAPI zs__thread_start(struct zs__thread *a_thread);
#endif

// EXPORT
zs__thread_handle zs__thread_create(void *data, int(*a_run)(void*), void(*a_abort)(void*));
int zs__thread_execute(zs__thread_handle a_thread_handle); /* OK: return true; | FEHLER (z.B. still running): return false; */
void zs__thread_abort(zs__thread_handle a_thread_handle);
int zs__thread_available(zs__thread_handle a_thread_handle);
void zs__thread_pause(zs__thread_handle a_thread_handle);
void zs__thread_resume(zs__thread_handle a_thread_handle);

/* Darf niemals im Destruktor eines anderen Threads */
/* aufgerufen werden -> Deadlock */
void zs__thread_wait(zs__thread_handle a_thread_handle);


// !!!  Wait                                                                                    !!!

static unsigned int zs__wait_add_stream_reader_fd(struct zs__wait *a_wait_struct, int a_fd);

// EXPORT
struct zs__wait* zs__wait_create(void);
void zs__wait_destroy(struct zs__wait *a_wait_struct);
unsigned int zs__wait_do(struct zs__wait *a_wait_struct);
unsigned int zs__wait_add_timer(struct zs__wait *a_wait_struct, long sec, long microsec);
unsigned int zs__wait_add_stream_reader(struct zs__wait *a_wait_struct, struct zs__stream *a_stream_struct);

#define PTHREAD_IMPL
#ifdef PTHREAD_IMPL
static volatile unsigned int threads_running = 0;
static pthread_mutex_t threads_running_mutex;
static pthread_cond_t threads_running_cv;
#endif

// ************************************************************************************************
// ***  Alle Funktionen implementieren                                                          ***
// ************************************************************************************************

int zs__execute(void *a_data, int(*a_main)(void*)) {
  /******************************************
   ** Initialisieren                       **
   ******************************************/

  threads_running = 0;
  pthread_mutex_init(&threads_running_mutex, NULL);
  pthread_cond_init (&threads_running_cv, NULL);

  zs__thread_map = 0;
  pthread_mutex_init(&thread_map_mutex, NULL);

  zs__thread_execute(zs__thread_create(a_data, a_main, 0));

  // Wait for last thread finished
  pthread_mutex_lock(&threads_running_mutex);
  pthread_cond_wait(&threads_running_cv, &threads_running_mutex);
  pthread_mutex_unlock(&threads_running_mutex);
  pthread_mutex_destroy(&threads_running_mutex);
  pthread_cond_destroy(&threads_running_cv);

  pthread_mutex_destroy(&thread_map_mutex);

  return 0;
}

// ************************************************************************************************
// ***  M E S S A G E   L O O P                                                                 ***
// ************************************************************************************************

// EXPORT
struct zs__messageloop* zs__messageloop_create(void *a_instance, void(*a_response)(void *instance, void *data), void(*a_after_abort)(void *a_instance)) {
  struct zs__messageloop *a_messageloop;
  
  a_messageloop = malloc(sizeof(struct zs__messageloop));
  a_messageloop->mutex = zs__semaphore_create(2);
  zs__semaphore_set(a_messageloop->mutex, 0, 1);
  zs__semaphore_set(a_messageloop->mutex, 1, 0);
  a_messageloop->first = 0;
  a_messageloop->last = 0;
  a_messageloop->aborting = 0;
  a_messageloop->response = a_response;
  a_messageloop->instance = a_instance;
  a_messageloop->thread_handle = zs__thread_create(a_messageloop, &zs__messageloop_run, &zs__messageloop_abort);
  a_messageloop->after_abort = a_after_abort;
  return a_messageloop;
}

// EXPORT
zs__thread_handle zs__messageloop_get_thread_handle(struct zs__messageloop *a_messageloop) {
  return a_messageloop->thread_handle;
}

// EXPORT
int zs__messageloop_post(struct zs__messageloop *a_messageloop, void *a_data) {
  struct zs__messageloop_element *a_element;
  int back;
  
  if(a_messageloop == 0)
    return 0;
  // sperre Request-Queue
  zs__semaphore_op(a_messageloop->mutex, 0, -1, 0, 0);
  if(a_messageloop->aborting)
    back = 0;
  else {
    //f�ge Request ein
    a_element = malloc(sizeof(struct zs__messageloop_element));
    a_element->data = a_data;
    a_element->next = 0;
    if(a_messageloop->last)
      a_messageloop->last->next = a_element;
    else
      a_messageloop->first = a_element;
    a_messageloop->last = a_element;
    // gibt an, dass ein neuer Request anliegt
    zs__semaphore_op(a_messageloop->mutex, 1, 1, 0, 0);
    back = -1;
  }
  // gebe Request-Queue frei
  zs__semaphore_op(a_messageloop->mutex, 0, 1, 0, 0);
  return back;
}

static void zs__messageloop_abort(void *a_data) {
  struct zs__messageloop *a_messageloop;
  
  a_messageloop = (struct zs__messageloop *) a_data;
  if(a_messageloop == 0)
    return;
  // sperre Request-Queue
  zs__semaphore_op(a_messageloop->mutex, 0, -1, 0, 0);
  if(a_messageloop->aborting == 0) {
    a_messageloop->aborting = -1;
    // gibt an, dass ein neuer Request anliegt
    zs__semaphore_op(a_messageloop->mutex, 1, 1, 0, 0);
  }
  // gebe Request-Queue frei
  zs__semaphore_op(a_messageloop->mutex, 0, 1, 0, 0);
}

static int zs__messageloop_run(void *a_data) {
  struct zs__messageloop_element *a_element;
  struct zs__messageloop *a_messageloop;
  
  a_messageloop = (struct zs__messageloop *) a_data;
  
  while(-1) {
    
    // warte, bis ein Request anliegt
    zs__semaphore_op(a_messageloop->mutex, 1, -1, 0, 0);
    // sperre Request-Queue
    zs__semaphore_op(a_messageloop->mutex, 0, -1, 0, 0);
    
    a_element = a_messageloop->first;
    if(a_element) {
      a_messageloop->first = a_element->next;
      if(a_messageloop->first == 0)
        a_messageloop->last = 0;
    }
    
    // gebe Request-Queue frei
    zs__semaphore_op(a_messageloop->mutex, 0, 1, 0, 0);
    
    if(a_element) {
      a_messageloop->response(a_messageloop->instance, a_element->data);
      free(a_element);
    }
    else
      break;
  }
  if(a_messageloop->after_abort)
    a_messageloop->after_abort(a_messageloop->instance);
  zs__semaphore_destroy(a_messageloop->mutex);
  free(a_messageloop);
  return 0;
}

// ************************************************************************************************
// ***  S E M A P H O R E                                                                       ***
// ************************************************************************************************

// EXPORT
struct zs__semaphore* zs__semaphore_create(unsigned char a_count) {
  struct zs__semaphore *a_sem = malloc(sizeof(struct zs__semaphore));
  
  a_sem->count = a_count;
  a_sem->references = malloc(sizeof(unsigned int));
  *(a_sem->references) = 1;
  #ifdef linux // for linux
  a_sem->sem = (a_count ? semget(IPC_PRIVATE, a_count, IPC_CREAT|0666) : 0);
//  a_sem->sem = (a_count ? semget(IPC_PRIVATE, a_count, IPC_CREAT|IPC_EXCL|0600) : 0);
  #else // for win32
  sem(a_count ? 0 : malloc(sizeof(HANDLE)*a_count);
  unsigned char i;
  for(i=0; i<a_count; ++i)
    sem[i] = CreateSemaphore(0, 0, 100, 0);
  #endif
  return a_sem;
}

// EXPORT
struct zs__semaphore* zs__semaphore_duplicate(struct zs__semaphore *old_sem) {
  struct zs__semaphore *a_sem = malloc(sizeof(struct zs__semaphore));
  a_sem->count = old_sem->count;
  a_sem->references = old_sem->references;
  a_sem->sem = old_sem->sem;
  ++(*a_sem->references);
  return a_sem;
}

// EXPORT
void zs__semaphore_destroy(struct zs__semaphore *a_sem) {
  --(*a_sem->references);
  if(*a_sem->references == 0) {
    #ifdef linux // for linux
    semctl(a_sem->sem, 0, IPC_RMID, 0);
    #else // for win32
    for(unsigned char i=0; i<a_sem->count; ++i) {
      CloseHandle(a_sem->sem[i]);
    }
    free(a_sem->sem);
    #endif
    free(a_sem->references);
  }
  free(a_sem);
}

// EXPORT
void zs__semaphore_set(struct zs__semaphore *a_sem, unsigned char a_sem_id, int a_value) {
  if(a_sem_id >= a_sem->count)
    return;
  #ifdef linux
  semctl(a_sem->sem, a_sem_id, SETVAL, a_value);
  #else
  //win32?
  #endif
}

// EXPORT
int zs__semaphore_get(struct zs__semaphore *a_sem, unsigned char a_sem_id) {
  if(a_sem_id >= a_sem->count)
    return 0;
  #ifdef linux
  return semctl(a_sem->sem, a_sem_id, GETVAL);
  #else
  //win32?
  #endif
}

// EXPORT
int zs__semaphore_op(struct zs__semaphore *a_sem_old, unsigned char a_sem_id, int a_value, unsigned long a_sec, unsigned long a_nano_sec) {
  struct zs__semaphore *a_sem;
  struct timespec a_timeout_value;
  struct timespec *a_timeout_ptr;
  int a_timeout;
#ifdef linux
  struct sembuf sb;
  int last_errno = 0;
#endif

  a_sem = zs__semaphore_duplicate(a_sem_old);
  if(a_sem_id >= a_sem->count || a_value == 0) {
    zs__semaphore_destroy(a_sem);
    return -2;
  }
  
  if(a_sec == 0 && a_nano_sec == 0)
    a_timeout_ptr = 0;
  else {
    a_timeout_ptr = &a_timeout_value;
    a_timeout_ptr->tv_sec = a_sec;
    a_timeout_ptr->tv_nsec = a_nano_sec;
//    printf("SemWait: %d, %d\n", (int) a_sec, (int) a_nano_sec);
  }
  a_timeout = 0;
  
#ifdef linux
  sb.sem_num = a_sem_id;
  sb.sem_op = a_value;
  sb.sem_flg = 0;
  
  while(a_timeout == 0 && semtimedop(a_sem->sem, &sb, 1, 0) == -1) {
//  while(a_timeout == 0 && semtimedop(a_sem->sem, &sb, 1, a_timeout_ptr) == -1) {
//  while(a_timeout == 0 && semtimedop(a_sem->sem, &sb, 1, a_sec == 0 && a_nano_sec == 0 ? 0 : &a_timeout_value) == -1) {
//  while(a_timeout == 0 && semop(a_sem->sem, &sb, 1) == -1) {
/*
  while(a_timeout == 0) {
    if(a_sec == 0 && a_nano_sec == 0) {
      if(semop(a_sem->sem, &sb, 1) == 0)
        break;
    }
    else {
      printf("SemWait: %d, %d\n", (int) a_sec, (int) a_nano_sec);
      if(semtimedop(a_sem->sem, &sb, 1, &a_timeout_value) == 0) 
      break;
    }
*/
    if(errno != last_errno) {
      switch(errno) {
        case EINTR:
          break;
        case EAGAIN:
          a_timeout = -1;
          break;
        default:
          perror("(zs__semaphore_op): ");
          last_errno = errno;
          break;
      }
    }
  }
#else
  //win32?
  //if(a_value > 0) {
  //  ReleaseSemaphore(a_sem.sem[a_sem_id], 1, 0);
  //}
  //else {
  //  WaitForSingleObject(a_sem.sem[a_sem_id], INFINITE);
  //}
#endif
  zs__semaphore_destroy(a_sem);
  
  return a_timeout;
}

// ************************************************************************************************
// ***  S E R V E R                                                                             ***
// ************************************************************************************************

static unsigned int zs__server_read_buffer(struct zs__stream *a_stream, char *a_buffer, unsigned int a_size);
static unsigned int zs__server_write_dgram(struct zs__stream *a_stream, const char *a_buffer, unsigned int a_size);
static void zs__server_close_reader_buffer(struct zs__stream *a_stream_struct); // will not realy close the socket
static void zs__server_close_writer(struct zs__stream *a_stream_struct);
static unsigned int zs__server_state_dgram(struct zs__stream *a_stream);
#ifdef linux
static int zs__server_get_fd_dgram(struct zs__stream *a_stream_struct);
#endif

static void zs__server_abort(void *a_data) {
  struct zs__server *a_server;
  char a_buffer;
  
  a_buffer = 0;
  a_server = (struct zs__server*) a_data;
  while(zs__stream_write(a_server->stream_struct, &a_buffer, 1) == 0);
}

static int zs__server_run(void *a_data) {
  struct zs__server *a_server;
  struct zs__wait *a_wait_struct;
  unsigned int a_abort_val;

  a_server = (struct zs__server*) a_data;
  a_wait_struct = zs__wait_create();
  zs__wait_add_stream_reader_fd(a_wait_struct, a_server->socket);
  a_abort_val = zs__wait_add_stream_reader(a_wait_struct, a_server->stream_struct);
  while(a_abort_val != zs__wait_do(a_wait_struct)) {
    struct sockaddr a_remote_host;
    socklen_t a_sin_size = sizeof(struct sockaddr);
    int a_socket;
    
    if((a_socket = accept(a_server->socket, &a_remote_host, &a_sin_size)) == -1) {
      perror("(zs__server_run) ");
      continue;
    }
    a_server->on_connect(zs__stream_create_socket(a_socket, &a_remote_host), a_server->data);
  }
  zs__wait_destroy(a_wait_struct);
  zs__stream_destroy(a_server->stream_struct);
  free(a_server);
  return 0;
}

static int zs__server_run_dgram(void *a_data) {
  struct zs__server *a_server;
  struct zs__wait *a_wait_struct;
  unsigned int a_abort_val;

  a_server = (struct zs__server*) a_data;
  a_wait_struct = zs__wait_create();
  zs__wait_add_stream_reader_fd(a_wait_struct, a_server->socket);
  a_abort_val = zs__wait_add_stream_reader(a_wait_struct, a_server->stream_struct);
  while(a_abort_val != zs__wait_do(a_wait_struct)) {
    struct sockaddr a_remote_host;
//    struct sockaddr a_local_host;
    socklen_t a_sockaddr_len;
    struct zs__stream *a_stream;
//    int a_socket;
    char a_buffer[2048];
    ssize_t a_read_length;

    a_sockaddr_len = sizeof(struct sockaddr);
    a_read_length = recvfrom(a_server->socket, a_buffer, 2048, 0, &a_remote_host, &a_sockaddr_len);
    if(a_read_length <= 0) {
      perror("(zs__server_run_dgram) ");
      continue;
    }
    a_sockaddr_len = sizeof(struct sockaddr);
    
    a_stream = zs__stream_create(&zs__server_read_buffer, &zs__server_write_dgram, &zs__server_state_dgram, &zs__server_close_reader_buffer, &zs__server_close_writer);
#ifdef linux
    a_stream->get_fd_reader = &zs__server_get_fd_dgram;
    a_stream->get_fd_writer = &zs__server_get_fd_dgram;
#endif
    a_stream->read_buffer_cur = 0;
    if(a_read_length) {
      a_stream->read_buffer = malloc(a_read_length);
      memcpy(a_stream->read_buffer, a_buffer, a_read_length);
      a_stream->read_buffer_length = a_read_length;
    }
    else {
      a_stream->read_buffer = 0;
      a_stream->read_buffer_length = 0;
    }
    a_stream->streamtype = ZS__STREAMTYPE_INET;
    memcpy(a_stream->remote_ipv4_address, &((struct sockaddr_in*)&a_remote_host)->sin_addr.s_addr, 4);
    a_stream->remote_port = ntohs(((struct sockaddr_in*)&a_remote_host)->sin_port);
    //a_stream_struct->remote_host = malloc(sizeof(struct sockaddr_in));
    //memcpy(a_stream_struct->remote_host, a_remote_host, sizeof(struct sockaddr_in));
    a_stream->socket = a_server->socket;
    a_stream->closed = 0x00;
    a_server->on_connect(a_stream, a_server->data);
  }
  zs__wait_destroy(a_wait_struct);
  zs__stream_destroy(a_server->stream_struct);
  free(a_server);
  return 0;
}

static unsigned int zs__server_read_buffer(struct zs__stream *a_stream, char *a_buffer, unsigned int a_size) {
  if(a_size > a_stream->read_buffer_length - a_stream->read_buffer_cur)
    a_size = a_stream->read_buffer_length - a_stream->read_buffer_cur;

  memcpy(a_buffer, &a_stream->read_buffer[a_stream->read_buffer_cur], a_size);
  a_stream->read_buffer_cur += a_size;
  if(a_stream->read_buffer_cur == a_stream->read_buffer_length)
    zs__server_close_reader_buffer(a_stream);
  return a_size;
}

static unsigned int zs__server_write_dgram(struct zs__stream *a_stream, const char *a_buffer, unsigned int a_size) {
  ssize_t a_back;
  struct sockaddr_in a_remote_host;
//  socklen_t a_remote_host_len;
  
//  a_remote_host_len = sizeof(struct sockaddr_in);
  a_remote_host.sin_family = AF_INET;
  a_remote_host.sin_port = htons(a_stream->remote_port);
  ((unsigned char*)&a_remote_host.sin_addr)[0] = a_stream->remote_ipv4_address[0];
  ((unsigned char*)&a_remote_host.sin_addr)[1] = a_stream->remote_ipv4_address[1];
  ((unsigned char*)&a_remote_host.sin_addr)[2] = a_stream->remote_ipv4_address[2];
  ((unsigned char*)&a_remote_host.sin_addr)[3] = a_stream->remote_ipv4_address[3];
  a_back = sendto(a_stream->socket, a_buffer, a_size, 0, (struct sockaddr*) &a_remote_host, sizeof(struct sockaddr_in));
  return a_back < 0 ? 0 : a_back;
}

static unsigned int zs__server_state_dgram(struct zs__stream *a_stream) {
  unsigned int a_back;
  struct pollfd a_pollfd;
  
  if(!a_stream)
    return ZS__READER_CLOSED | ZS__WRITER_CLOSED;
  
  a_back = 0;
  
  if(a_stream->closed & 0x01)
    a_back |= ZS__READER_CLOSED;
  if(a_stream->closed & 0x02)
    a_back |= ZS__WRITER_CLOSED;
  if(!(a_stream->closed & 0x03)) {
    a_pollfd.fd = a_stream->socket;
    a_pollfd.events = POLLERR | POLLHUP | POLLNVAL;
    a_pollfd.revents = 0;
    poll(&a_pollfd, 1, 0);
    if(a_pollfd.revents & (POLLERR | POLLNVAL | POLLHUP))
      a_back |= ZS__WRITER_CLOSED;
  }
  return a_back;
}

static void zs__server_close_reader_buffer(struct zs__stream *a_stream_struct) {
  if((a_stream_struct->closed & 0x01) == 0) {
    a_stream_struct->closed = a_stream_struct->closed | 0x01;
    if(a_stream_struct->read_buffer) {
      free(a_stream_struct->read_buffer);
      a_stream_struct->read_buffer = 0;
      a_stream_struct->read_buffer_cur = 0;
      a_stream_struct->read_buffer_length = 0;
    }
  }
}

static void zs__server_close_writer(struct zs__stream *a_stream_struct) {
  if((a_stream_struct->closed & 0x02) == 0)
    a_stream_struct->closed = a_stream_struct->closed | 0x02;
}

#ifdef linux
static int zs__server_get_fd_dgram(struct zs__stream *a_stream_struct) {
  return a_stream_struct->socket;
}
#endif

// EXPORT
zs__thread_handle zs__server_create_local(const char *a_name, void *a_data, void(*a_on_connect)(struct zs__stream*, void*)) {
  struct zs__server *a_server;
  struct sockaddr_un a_sockaddr;
#ifdef linux
  int a_socket;
#else
#endif

  // Unverbundenen Socket erzeugen
  if((a_socket = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    perror("(zs__server_create_local) ");
    return -1;
  }
  unlink(a_name);
//  fcntl(sock, F_SETFL, O_NONBLOCK);
  a_sockaddr.sun_family      = AF_UNIX;
  strcpy(a_sockaddr.sun_path, a_name);
  // Adresse mit Socket binden
  if (bind(a_socket, (struct sockaddr*) &a_sockaddr, sizeof(struct sockaddr_un)) < 0) {
    perror("(zs__server_create_local) ");
    return -1;
  }

//unbind muss am Ende noch ausgefuehrt werden, damit die Datei wieder verschwindet.
  
  // Neue Verbindungen annehmen
  if(listen(a_socket, SOMAXCONN) == -1) {
    perror("(zs__server_create_local) ");
    return -1;
  }
  a_server = malloc(sizeof(struct zs__server));
  a_server->data = a_data;
  a_server->on_connect = a_on_connect;
  a_server->stream_struct = zs__stream_create_pipe();
  a_server->socket = a_socket;
  return zs__thread_create(a_server, &zs__server_run, &zs__server_abort);
}

// EXPORT
zs__thread_handle zs__server_create_tcp_ipv4(const char *a_ipv4, int a_port, void *a_data, void(*a_on_connect)(struct zs__stream*, void*)) {
  struct zs__server *a_server;
  struct sockaddr_in a_sockaddr;
#ifdef linux
  int a_socket;
#else
  SOCKET a_socket;
  WSADATA wsaData;

  if(WSAStartup(0x0101, &wsaData)) {
    return -1;
  }
  if(wsaData.wVersion != 0x0101) {
    return -1;
  }
#endif
  // Unverbundenen Socket erzeugen
  if((a_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
    perror("(zs__server_create_tcpip) ");
    return -1;
  }
//  fcntl(sock, F_SETFL, O_NONBLOCK);
  int opt = 1;
  if(setsockopt(a_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) == -1) {
    perror("(zs__server_create_tcpip) ");
    return -1;
  }
  a_sockaddr.sin_family      = AF_INET;
  a_sockaddr.sin_port        = htons(a_port);
  if(a_ipv4) {
    ((unsigned char*)&a_sockaddr.sin_addr)[0] = a_ipv4[0];
    ((unsigned char*)&a_sockaddr.sin_addr)[1] = a_ipv4[1];
    ((unsigned char*)&a_sockaddr.sin_addr)[2] = a_ipv4[2];
    ((unsigned char*)&a_sockaddr.sin_addr)[3] = a_ipv4[3];
  }
  else
    a_sockaddr.sin_addr.s_addr = INADDR_ANY;
  // Adresse mit Socket binden
  if (bind(a_socket, (struct sockaddr*) &a_sockaddr, sizeof(struct sockaddr_in)) == -1) {
    perror("(zs__server_create_tcpip) ");
    return -1;
  }
  // Neue Verbindungen annehmen
  if(listen(a_socket, SOMAXCONN) == -1) {
    perror("(zs__server_create_tcpip) ");
    return -1;
  }
  a_server = malloc(sizeof(struct zs__server));
  a_server->data = a_data;
  a_server->on_connect = a_on_connect;
  
  // ???
  a_server->stream_struct = zs__stream_create_pipe();
  
  a_server->socket = a_socket;
  return zs__thread_create(a_server, &zs__server_run, &zs__server_abort);
}

// EXPORT
zs__thread_handle zs__server_create_udp_ipv4(const char *a_ipv4, int a_port, void *a_data, void(*a_on_connect)(struct zs__stream*, void*)) {
  struct zs__server *a_server;
  struct sockaddr_in a_sockaddr;
#ifdef linux
  int a_socket;
#else
  SOCKET a_socket;
  WSADATA wsaData;

  if(WSAStartup(0x0101, &wsaData)) {
    return -1;
  }
  if(wsaData.wVersion != 0x0101) {
    return -1;
  }
#endif
  // Unverbundenen Socket erzeugen
  if((a_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
    perror("(zs__server_create_udp_ipv4) ");
    return -1;
  }
//  fcntl(sock, F_SETFL, O_NONBLOCK);
  int opt = 1;
  if(setsockopt(a_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) == -1) {
    perror("(zs__server_create_udp_ipv4) ");
    return -1;
  }
  a_sockaddr.sin_family      = AF_INET;
  a_sockaddr.sin_port        = htons(a_port);
  if(a_ipv4) {
    ((unsigned char*)&a_sockaddr.sin_addr)[0] = a_ipv4[0];
    ((unsigned char*)&a_sockaddr.sin_addr)[1] = a_ipv4[1];
    ((unsigned char*)&a_sockaddr.sin_addr)[2] = a_ipv4[2];
    ((unsigned char*)&a_sockaddr.sin_addr)[3] = a_ipv4[3];
  }
  else
    a_sockaddr.sin_addr.s_addr = INADDR_ANY;
  // Adresse mit Socket binden
  if (bind(a_socket, (struct sockaddr*) &a_sockaddr, sizeof(struct sockaddr_in)) == -1) {
    perror("(zs__server_create_udp_ipv4) ");
    return -1;
  }
  // Neue Verbindungen annehmen
//  if(listen(a_socket, SOMAXCONN) == -1) {
//    perror("(zs__server_create_udp_ipv4) ");
//    return -1;
//  }
  a_server = malloc(sizeof(struct zs__server));
  a_server->data = a_data;
  a_server->on_connect = a_on_connect;
  
  // ???
  a_server->stream_struct = zs__stream_create_pipe();
  
  a_server->socket = a_socket;
  return zs__thread_create(a_server, &zs__server_run_dgram, &zs__server_abort);
}

// ************************************************************************************************
// ***  S T R E A M                                                                             ***
// ************************************************************************************************

static unsigned int zs__stream_read_pipe(struct zs__stream *a_stream, char *a_buffer, unsigned int a_size);
static unsigned int zs__stream_write_pipe(struct zs__stream *a_stream, const char *a_buffer, unsigned int a_size);
static unsigned int zs__stream_state_pipe(struct zs__stream *a_stream);
static void zs__stream_close_reader_pipe(struct zs__stream *a_stream_struct);
static void zs__stream_close_writer_pipe(struct zs__stream *a_stream_struct);
#ifdef linux
static int zs__stream_get_fd_reader_pipe(struct zs__stream *a_stream_struct);
static int zs__stream_get_fd_writer_pipe(struct zs__stream *a_stream_struct);
#endif

static unsigned int zs__stream_read_socket(struct zs__stream *a_stream, char *a_buffer, unsigned int a_size);
static unsigned int zs__stream_write_socket(struct zs__stream *a_stream, const char *a_buffer, unsigned int a_size);
static unsigned int zs__stream_state_socket(struct zs__stream *a_stream);
static void zs__stream_close_reader_socket(struct zs__stream *a_stream_struct);
static void zs__stream_close_writer_socket(struct zs__stream *a_stream_struct);
#ifdef linux
static int zs__stream_get_fd_socket(struct zs__stream *a_stream_struct);
#endif

static struct zs__stream* zs__stream_create(unsigned int(*read)(struct zs__stream*, char*, unsigned int), unsigned int (*write)(struct zs__stream*, const char*, unsigned int), unsigned int(*state)(struct zs__stream *a_stream), void(*close_reader)(struct zs__stream*), void(*close_writer)(struct zs__stream*)) {
  struct zs__stream *a_stream_struct;

  a_stream_struct = malloc(sizeof(struct zs__stream));
  memset(a_stream_struct, 0, sizeof(struct zs__stream));

  a_stream_struct->read = read;
  a_stream_struct->write = write;
  a_stream_struct->state = state;
  a_stream_struct->close_reader = close_reader;
  a_stream_struct->close_writer = close_writer;

//  a_stream_struct->data = &((char*)a_stream_struct)[sizeof(struct zs__stream)];
//  if(data)
//    *data = a_stream_struct->data;
  return a_stream_struct;
}

// EXPORT
struct zs__stream* zs__stream_create_pipe() {
  struct zs__stream *a_stream_struct;
#ifdef linux
  int tmp_pipe[2];

  pipe(tmp_pipe);
#endif
  a_stream_struct = zs__stream_create(&zs__stream_read_pipe, &zs__stream_write_pipe, &zs__stream_state_pipe, &zs__stream_close_reader_pipe, &zs__stream_close_writer_pipe);
#ifdef linux
  a_stream_struct->get_fd_reader = &zs__stream_get_fd_reader_pipe;
  a_stream_struct->get_fd_writer = &zs__stream_get_fd_writer_pipe;
  a_stream_struct->reader   = tmp_pipe[0];
  a_stream_struct->writer   = tmp_pipe[1];
#endif
  
  a_stream_struct->streamtype = ZS__STREAMTYPE_PIPE;
  a_stream_struct->closed = 0x00;
  
  return a_stream_struct;
}

/* zs__stream_create_socket ist nur eine hilfsprozedur, wobei a_remote_host niemals 0 sein darf ! */
static struct zs__stream* zs__stream_create_socket(int a_socket, struct sockaddr *a_remote_host) {
  struct zs__stream *a_stream_struct;
//  struct zs__stream_data_socket *data;

  a_stream_struct = zs__stream_create(&zs__stream_read_socket, &zs__stream_write_socket, &zs__stream_state_socket, &zs__stream_close_reader_socket, &zs__stream_close_writer_socket);

#ifdef linux
  a_stream_struct->get_fd_reader = &zs__stream_get_fd_socket;
  a_stream_struct->get_fd_writer = &zs__stream_get_fd_socket;
#endif
  if(a_remote_host == 0) { /* dieser Fall darf neimals vorkommen! Zur sicherheit wird er trotzdem behandelt. */
    a_stream_struct->streamtype = ZS__STREAMTYPE_SOCKET;
    memset(a_stream_struct->remote_ipv4_address, 0, 4);
  }
  else {
    if(a_remote_host->sa_family == AF_INET) { // ((struct sockaddr_in*) a_remote_host)
      a_stream_struct->streamtype = ZS__STREAMTYPE_INET;
      memcpy(a_stream_struct->remote_ipv4_address, &((struct sockaddr_in*)a_remote_host)->sin_addr.s_addr, 4);
    }
    else {
      if(a_remote_host->sa_family == AF_UNIX) // ((struct sockaddr_un*) a_remote_host)
        a_stream_struct->streamtype = ZS__STREAMTYPE_LOCAL;
      else
        a_stream_struct->streamtype = ZS__STREAMTYPE_SOCKET;
      memset(a_stream_struct->remote_ipv4_address, 0, 4);
    }
    //a_stream_struct->remote_host = malloc(sizeof(struct sockaddr_in));
    //memcpy(a_stream_struct->remote_host, a_remote_host, sizeof(struct sockaddr_in));
  }
  a_stream_struct->socket = a_socket;
  a_stream_struct->closed = 0x00;
  return a_stream_struct;
}

// EXPORT
struct zs__stream* zs__stream_create_tcp_ipv4(const char *a_src_ipv4, int a_src_port, const char *a_dst_ipv4, int a_dst_port) {
//static struct zs__stream* zs__stream_create_tcpip(char a_ip1, char a_ip2, char a_ip3, char a_ip4, int a_port) {
  int a_socket;
  struct sockaddr_in a_sockaddr;

  if(!a_dst_ipv4)
    return 0;

  // Unverbundenen Socket erzeugen
  if((a_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    perror("(zs__stream_create_tcp_ipv4) ");
    return 0;
  }

  if(a_src_ipv4 || a_src_port) {
    a_sockaddr.sin_family = AF_INET;
    a_sockaddr.sin_port = htons(a_src_port);
    if(a_src_ipv4) {
      ((unsigned char*)&a_sockaddr.sin_addr)[0] = a_src_ipv4[0];
      ((unsigned char*)&a_sockaddr.sin_addr)[1] = a_src_ipv4[1];
      ((unsigned char*)&a_sockaddr.sin_addr)[2] = a_src_ipv4[2];
      ((unsigned char*)&a_sockaddr.sin_addr)[3] = a_src_ipv4[3];
    }
    else
      a_sockaddr.sin_addr.s_addr = INADDR_ANY;
    
    // Adresse mit Socket binden
    if (bind(a_socket, (struct sockaddr*) &a_sockaddr, sizeof(struct sockaddr_in)) == -1) {
      perror("(zs__stream_create_tcp_ipv4) ");
      close(a_socket);
      return 0;
    }
  }
    
  a_sockaddr.sin_family = AF_INET;
  a_sockaddr.sin_port = htons(a_dst_port);
  ((unsigned char*)&a_sockaddr.sin_addr)[0] = a_dst_ipv4[0];
  ((unsigned char*)&a_sockaddr.sin_addr)[1] = a_dst_ipv4[1];
  ((unsigned char*)&a_sockaddr.sin_addr)[2] = a_dst_ipv4[2];
  ((unsigned char*)&a_sockaddr.sin_addr)[3] = a_dst_ipv4[3];

  if(connect(a_socket, (const struct sockaddr*) &a_sockaddr, sizeof(struct sockaddr_in)) == -1) {
    perror("(zs__stream_create_tcp_ipv4) ");
    close(a_socket);
    return 0;
  }
  return zs__stream_create_socket(a_socket, (struct sockaddr*) &a_sockaddr);
}

// EXPORT
struct zs__stream* zs__stream_create_udp_ipv4(const char *a_src_ipv4, int a_src_port, const char *a_dst_ipv4, int a_dst_port) {
  int a_socket;
  struct sockaddr_in a_sockaddr;

  if(!a_dst_ipv4)
    return 0;

  // Unverbundenen Socket erzeugen
  if((a_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    perror("(zs__stream_create_udp_ipv4) ");
    return 0;
  }

  if(a_src_ipv4 || a_src_port) {
    a_sockaddr.sin_family = AF_INET;
    a_sockaddr.sin_port = htons(a_src_port);
    if(a_src_ipv4) {
      ((unsigned char*)&a_sockaddr.sin_addr)[0] = a_src_ipv4[0];
      ((unsigned char*)&a_sockaddr.sin_addr)[1] = a_src_ipv4[1];
      ((unsigned char*)&a_sockaddr.sin_addr)[2] = a_src_ipv4[2];
      ((unsigned char*)&a_sockaddr.sin_addr)[3] = a_src_ipv4[3];
    }
    else
      a_sockaddr.sin_addr.s_addr = INADDR_ANY;
    
    // Adresse mit Socket binden
    if (bind(a_socket, (struct sockaddr*) &a_sockaddr, sizeof(struct sockaddr_in)) == -1) {
      perror("(zs__stream_create_udp_ipv4) ");
      close(a_socket);
      return 0;
    }
  }
    
  a_sockaddr.sin_family = AF_INET;
  a_sockaddr.sin_port = htons(a_dst_port);
  ((unsigned char*)&a_sockaddr.sin_addr)[0] = a_dst_ipv4[0];
  ((unsigned char*)&a_sockaddr.sin_addr)[1] = a_dst_ipv4[1];
  ((unsigned char*)&a_sockaddr.sin_addr)[2] = a_dst_ipv4[2];
  ((unsigned char*)&a_sockaddr.sin_addr)[3] = a_dst_ipv4[3];

  if(connect(a_socket, (const struct sockaddr*) &a_sockaddr, sizeof(struct sockaddr_in)) == -1) {
    perror("(zs__stream_create_udp_ipv4) ");
    close(a_socket);
    return 0;
  }
  return zs__stream_create_socket(a_socket, (struct sockaddr*) &a_sockaddr);
}

// EXPORT
struct zs__stream* zs__stream_create_local(const char *a_name) {
  int a_socket;
  struct sockaddr_un a_sockaddr;
//  char *a_logincode;

  // Unverbundenen Socket erzeugen
  if((a_socket = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    perror("(zs__stream_create_local) ");
    return 0;
  }
  
  a_sockaddr.sun_family = AF_UNIX;
  strcpy(a_sockaddr.sun_path, a_name);

  if(connect(a_socket, (const struct sockaddr*) &a_sockaddr, sizeof(struct sockaddr_un)) == -1) {
    perror("(zs__stream_create_local) ");
    return 0;
  }
  return zs__stream_create_socket(a_socket, (struct sockaddr*) &a_sockaddr);
}

// EXPORT
struct zs__stream* zs__stream_create_system(int a_reader, int a_writer) {
  struct zs__stream *a_stream_struct;
  
  if(a_reader == -1 && a_writer == -1)
    return 0;
  
  if(a_reader == a_writer) {
    a_stream_struct = zs__stream_create(&zs__stream_read_pipe, &zs__stream_write_pipe, &zs__stream_state_socket, &zs__stream_close_reader_socket, &zs__stream_close_writer_socket);
    a_stream_struct->streamtype = ZS__STREAMTYPE_SOCKET;
    memset(a_stream_struct->remote_ipv4_address, 0, 4);
    a_stream_struct->closed = 0x00;
    a_stream_struct->socket = a_writer;
  }
  else {

    a_stream_struct = zs__stream_create(&zs__stream_read_pipe, &zs__stream_write_pipe, &zs__stream_state_pipe, &zs__stream_close_reader_pipe, &zs__stream_close_writer_pipe);
    a_stream_struct->streamtype = ZS__STREAMTYPE_PIPE;
    if(a_reader == -1)
      a_stream_struct->closed = 0x01;
    else if(a_writer == -1)
      a_stream_struct->closed = 0x02;
    else
      a_stream_struct->closed = 0x00;
  }
#ifdef linux
  a_stream_struct->get_fd_reader = &zs__stream_get_fd_reader_pipe;
  a_stream_struct->get_fd_writer = &zs__stream_get_fd_writer_pipe;
#endif
  a_stream_struct->reader = a_reader;
  a_stream_struct->writer = a_writer;
  
  return a_stream_struct;
}
/*
static struct zs__stream* zs__stream_create_filereader(int a_fd) {
  struct zs__stream_pipe *a_stream_struct;

  a_stream_struct = (struct zs__stream_pipe*) zs__stream_create(&zs__stream_read_pipe, &zs__stream_write_pipe, &zs__stream_state_pipe, &zs__stream_close_reader_pipe, &zs__stream_close_writer_pipe, sizeof(struct zs__stream_pipe));
#ifdef linux
  a_stream_struct->get_fd_reader = &zs__stream_get_fd_reader_pipe;
  a_stream_struct->get_fd_writer = &zs__stream_get_fd_writer_pipe;
  a_stream_struct->reader = a_fd;
  a_stream_struct->writer = -1;
#endif
  a_stream_struct->streamtype = ZS__STREAMTYPE_PIPE;
  a_stream_struct->closed = 0x02;
  
  return (struct zs__stream*) a_stream_struct;
}
*/
static unsigned int zs__stream_read_pipe(struct zs__stream *a_stream, char *a_buffer, unsigned int a_size) {
  ssize_t a_back;

  a_back = read(a_stream->reader, a_buffer, a_size);
  if(a_back <= 0) {
    printf("zs__stream_read_pipe: %s\n", strerror(errno));
    return 0;
  }
  return a_back;
}

static unsigned int zs__stream_write_pipe(struct zs__stream *a_stream, const char *a_buffer, unsigned int a_size) {
  ssize_t a_back;
  
  a_back = write(a_stream->writer, a_buffer, a_size);
  if(a_back < 0) {
    printf("zs__stream_write_pipe: %s\n", strerror(errno));
    return 0;
  }
  return a_back;
}

static unsigned int zs__stream_state_pipe(struct zs__stream *a_stream) {
  unsigned int a_back;
  struct pollfd a_pollfd;
  
  if(!a_stream)
    return ZS__READER_CLOSED | ZS__WRITER_CLOSED;
  
  a_back = 0;
  
  if(a_stream->closed & 0x01)
    a_back |= ZS__READER_CLOSED;
  else {
    a_pollfd.fd = a_stream->reader;
    a_pollfd.events = POLLERR | POLLHUP | POLLNVAL | POLLIN | POLLPRI;
//    a_pollfd.events = POLLERR | POLLHUP | POLLNVAL;
    a_pollfd.revents = 0;
    poll(&a_pollfd, 1, 0);
    if(a_pollfd.revents & (POLLERR | POLLHUP | POLLNVAL))
//    if(a_pollfd.revents)
      a_back |= ZS__READER_CLOSED;
    else if(!(a_pollfd.revents & (POLLIN | POLLPRI)))
      a_back |= ZS__READER_EOF;
  }
  
  if(a_stream->closed & 0x02)
    a_back |= ZS__WRITER_CLOSED;
  else {
    a_pollfd.fd = a_stream->writer;
    a_pollfd.events = POLLERR | POLLHUP | POLLNVAL;
    a_pollfd.revents = 0;
    poll(&a_pollfd, 1, 0);
    if(a_pollfd.revents & (POLLERR | POLLHUP | POLLNVAL))
      a_back |= ZS__WRITER_CLOSED;
  }
  
  return a_back;
}

static void zs__stream_close_reader_pipe(struct zs__stream *a_stream_struct) {
  if((a_stream_struct->closed & 0x01) == 0) {
    a_stream_struct->closed = a_stream_struct->closed | 0x01;
    close(a_stream_struct->reader);
  }
}

static void zs__stream_close_writer_pipe(struct zs__stream *a_stream_struct) {
  if((a_stream_struct->closed & 0x02) == 0) {
    a_stream_struct->closed = a_stream_struct->closed | 0x02;
    close(a_stream_struct->writer);
  }
}

#ifdef linux
static int zs__stream_get_fd_reader_pipe(struct zs__stream *a_stream_struct) {
  return a_stream_struct->reader;
}

static int zs__stream_get_fd_writer_pipe(struct zs__stream *a_stream_struct) {
  return a_stream_struct->writer;
}
#endif

static unsigned int zs__stream_read_socket(struct zs__stream *a_stream, char *a_buffer, unsigned int a_size) {
  int last_errno = 0;
  int a_back;

  while((a_back = recv(a_stream->socket, a_buffer, a_size, MSG_WAITALL)) == -1) {
    if(errno != last_errno) {
      switch(errno) {
        case EINTR:
          break;
        default:
//          perror("(zs__stream_read_socket): ");
          return 0;
          last_errno = errno;
          break;
      }
    }
  }
  return a_back;
}

static unsigned int zs__stream_write_socket(struct zs__stream *a_stream, const char *a_buffer, unsigned int a_size) {
  ssize_t a_back;
  
/*
  if(a_stream->streamtype == ZS__STREAMTYPE_INET) {
    const char *a_ipv4;
    struct sockaddr_in a_sockaddr;

    a_ipv4 = zs__stream_get_local_ipv4_address(a_stream);
    printf("Bind: %d.%d.%d.%d:%d\n", (((int)a_ipv4[0]) & 0xff), (((int)a_ipv4[1]) & 0xff), (((int)a_ipv4[2]) & 0xff), (((int)a_ipv4[3]) & 0xff), a_stream->local_port);
    a_sockaddr.sin_family = AF_INET;
    a_sockaddr.sin_port = htons(a_stream->local_port);
    ((unsigned char*)&a_sockaddr.sin_addr)[0] = a_ipv4[0];
    ((unsigned char*)&a_sockaddr.sin_addr)[1] = a_ipv4[1];
    ((unsigned char*)&a_sockaddr.sin_addr)[2] = a_ipv4[2];
    ((unsigned char*)&a_sockaddr.sin_addr)[3] = a_ipv4[3];
    
    // Adresse mit Socket binden
    if (bind(a_stream->socket, (struct sockaddr*) &a_sockaddr, sizeof(struct sockaddr_in)) == -1) {
      perror("(zs__stream_create_udp_ipv4) ");
    }
  }
*/  
  
  a_back = send(a_stream->socket, a_buffer, a_size, 0);
  return a_back < 0 ? 0 : a_back;
}

static unsigned int zs__stream_state_socket(struct zs__stream *a_stream) {
  unsigned int a_back;
  struct pollfd a_pollfd;
  
  if(!a_stream)
    return ZS__READER_CLOSED | ZS__WRITER_CLOSED;
  
  a_back = 0;
  
  if(a_stream->closed & 0x01)
    a_back |= ZS__READER_CLOSED;
  if(a_stream->closed & 0x02)
    a_back |= ZS__WRITER_CLOSED;
  if(!(a_stream->closed & 0x03)) {
    a_pollfd.fd = a_stream->socket;
    a_pollfd.events = POLLERR | POLLHUP | POLLNVAL | POLLIN | POLLPRI;
    a_pollfd.revents = 0;
    poll(&a_pollfd, 1, 0);
    if(a_pollfd.revents & (POLLERR | POLLNVAL))
      a_back |= ZS__READER_CLOSED | ZS__WRITER_CLOSED;
    else if(a_pollfd.revents & POLLHUP) {
      a_back |= ZS__WRITER_CLOSED;
//      if(!(a_pollfd.revents & (POLLIN | POLLPRI))) {
      // Wenn die Gegenseite den Socket geschlossen hat, wird trotzdem Pollin angezeigt (getestet!).
      // Darum verzichte ich hier auf das Abfragen auf POLLIN, um trotzdem anzuzeigen, dass der Socket zu ist.
      // Andererseits f�hrt dies auch dazu, dass zuvor noch von der Gegenseite verschickte Daten die nun hier anliegen
      // nicht mehr gelesen werden, weil ZS__READER_CLOSE zu frueh gesetzt wird (ebenfalls getestet!).
      if(!(a_pollfd.revents & POLLPRI)) {
        a_back |= ZS__READER_CLOSED;
      }
    }
    if(!(a_pollfd.revents & (POLLIN | POLLPRI)))
      a_back |= ZS__READER_EOF;
  }
  return a_back;
}

static void zs__stream_close_reader_socket(struct zs__stream *a_stream_struct) {
  if((a_stream_struct->closed & 0x01) == 0) {
    a_stream_struct->closed = a_stream_struct->closed | 0x01;
    if((a_stream_struct->closed & 0x03) == 0x03)
      close(a_stream_struct->socket);
  }
}

static void zs__stream_close_writer_socket(struct zs__stream *a_stream_struct) {
  if((a_stream_struct->closed & 0x02) == 0) {
    a_stream_struct->closed = a_stream_struct->closed | 0x02;
    if((a_stream_struct->closed & 0x03) == 0x03)
      close(a_stream_struct->socket);
  }
}

#ifdef linux
static int zs__stream_get_fd_socket(struct zs__stream *a_stream_struct) {
  return a_stream_struct->socket;
}
#endif

// EXPORT
unsigned int zs__stream_read(struct zs__stream *a_stream, char *a_buffer, unsigned int a_size) {
  if(a_stream && a_size > 0)
    return a_stream->read(a_stream, a_buffer, a_size);
  return 0;
}

// EXPORT
unsigned int zs__stream_write(struct zs__stream *a_stream, const char *a_buffer, unsigned int a_size) {
  if(a_stream && a_size > 0)
    return a_stream->write(a_stream, a_buffer, a_size);
  return 0;
}

// EXPORT
unsigned int zs__stream_state(struct zs__stream *a_stream) {
  return a_stream->state(a_stream);
}

// EXPORT
void zs__stream_close_reader(struct zs__stream *a_stream_struct) {
  a_stream_struct->close_reader(a_stream_struct);
}

// EXPORT
void zs__stream_close_writer(struct zs__stream *a_stream_struct) {
  a_stream_struct->close_writer(a_stream_struct);
}

// EXPORT
void zs__stream_destroy(struct zs__stream *a_stream_struct) {
  a_stream_struct->close_reader(a_stream_struct);
  a_stream_struct->close_writer(a_stream_struct);
  free(a_stream_struct);
}

// EXPORT
const char* zs__stream_get_remote_ipv4_address(struct zs__stream *a_stream_struct) {
  if(a_stream_struct->streamtype == ZS__STREAMTYPE_INET)
    return a_stream_struct->remote_ipv4_address;
  return 0;
}

// EXPORT
const char* zs__stream_get_local_ipv4_address(struct zs__stream *a_stream) {
  struct sockaddr_in a_sockaddr;
  socklen_t a_socklen;
  
  if(a_stream->streamtype != ZS__STREAMTYPE_INET)
    return 0;
  a_socklen = sizeof(struct sockaddr_in);
  if(getsockname(a_stream->socket, (struct sockaddr*) &a_sockaddr, &a_socklen) != 0)
    return 0;
  memcpy(a_stream->local_ipv4_address, &a_sockaddr.sin_addr, 4);
  return a_stream->local_ipv4_address;
}

// EXPORT
int zs__stream_get_remote_port(struct zs__stream *a_stream_struct) {
  if(a_stream_struct->streamtype == ZS__STREAMTYPE_INET)
    return a_stream_struct->remote_port;
  return 0;
}

// EXPORT
int zs__stream_get_local_port(struct zs__stream *a_stream) {
  struct sockaddr_in a_sockaddr;
  socklen_t a_socklen;
  
  if(a_stream->streamtype != ZS__STREAMTYPE_INET)
    return 0;
  a_socklen = sizeof(struct sockaddr_in);
  if(getsockname(a_stream->socket, (struct sockaddr*) &a_sockaddr, &a_socklen) != 0)
    return 0;
  return ntohs(a_sockaddr.sin_port);
}

// ************************************************************************************************
// ***  P R O C E S S                                                                           ***
// ************************************************************************************************

// EXPORT
int zs__process_execute(const char *a_file, char *const a_argv[], struct zs__stream *a_reader, struct zs__stream *a_writer, zs__process_handle *a_process_handle, void *a_data, void(*a_on_started)(void*)) {
  zs__process_handle aPID;
    
  if(a_reader == a_writer)
    a_reader = a_writer = 0;
  
  if((aPID = fork()) == 0) {
    int fdlimit;
    int fd;
    
    fdlimit = sysconf(_SC_OPEN_MAX);

    for(fd=0; fd<fdlimit; ++fd) {
      if(a_reader)
        if(a_reader->streamtype == ZS__STREAMTYPE_PIPE)
          if((a_reader->closed & 0x03) == 0 && a_reader->writer == fd)
            continue;
      if(a_writer)
        if(a_writer->streamtype == ZS__STREAMTYPE_PIPE)
          if((a_writer->closed & 0x03) == 0 && a_writer->reader == fd)
            continue;
      close(fd);
    }
//    daemon(0, 0);
    if(a_reader)
      if(a_reader->streamtype == ZS__STREAMTYPE_PIPE)
        if((a_reader->closed & 0x03) == 0)
          dup2(a_reader->writer, STDOUT_FILENO);
    if(a_writer)
      if(a_writer->streamtype == ZS__STREAMTYPE_PIPE)
        if((a_writer->closed & 0x03) == 0)
          dup2(a_writer->reader, STDIN_FILENO);
//    signal(SIGINT, SIG_IGN);
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    execv(a_file, a_argv);
  }
  else if(aPID != -1) {
    int a_exit_code;
    
    if(a_process_handle)
      *a_process_handle = aPID;
    if(a_reader)
      if(a_reader->streamtype == ZS__STREAMTYPE_PIPE)
        a_reader->close_writer(a_reader);
    if(a_writer)
      if(a_writer->streamtype == ZS__STREAMTYPE_PIPE)
        a_writer->close_reader(a_writer);
    if(a_on_started)
      a_on_started(a_data);
    waitpid(aPID, &a_exit_code, 0);
//    if(waitpid(aPID, &a_exit_code, 0) == -1)
//      zs__threadmanager_move_from_map_to_queue(aPID);
    return a_exit_code;
  }
  if(a_process_handle)
    *a_process_handle = 0;
  if(a_on_started)
    a_on_started(a_data);
  return -1;
}

// EXPORT
void zs__process_terminate(zs__process_handle a_process_handle) {
  kill(a_process_handle, SIGTERM);
}

// EXPORT
void zs__process_kill(zs__process_handle a_process_handle) {
  kill(a_process_handle, SIGKILL);
}

// ************************************************************************************************
// ***  T H R E A D                                                                             ***
// ************************************************************************************************
static struct zs__thread_list* zs__thread_map_find(zs__thread_handle a_thread_handle);
static void zs__thread_map_remove(struct zs__thread_list *a_entry);

// EXPORT
zs__thread_handle zs__thread_create(void *a_data, int(*a_run)(void*), void(*a_abort)(void*)) {
  struct zs__thread *a_thread;
  struct zs__thread_list *a_list;

  pthread_mutex_lock(&threads_running_mutex);
  ++threads_running;
  pthread_mutex_unlock(&threads_running_mutex);

  a_thread = malloc(sizeof(struct zs__thread));
  a_thread->data = a_data;
  a_thread->run = a_run;
  a_thread->abort = a_abort;
  a_thread->waiting_threads = 0;
  pthread_mutex_init(&a_thread->waiting_threads_mutex, NULL);
  pthread_mutex_lock(&a_thread->waiting_threads_mutex);
  pthread_cond_init (&a_thread->waiting_threads_cv, NULL);


  /*
   * Insert Handle2Thread
   */
  
  a_list = malloc(sizeof(struct zs__thread_list));
  a_list->thread = a_thread;
  
  // Look ThreadMap
  pthread_mutex_lock(&thread_map_mutex);

  a_list->next = zs__thread_map;
  zs__thread_map = a_list;

  // Unlook ThreadMap
  pthread_mutex_unlock(&thread_map_mutex);

#ifdef linux
  pthread_create(&a_thread->thread_handle, NULL, (void*(*)(void*)) zs__thread_start, a_thread);
#else
  DWORD thread_id;
  a_thread->thread_handle = CreateThread(0, 0, (int(*)(void*)) &zs__thread_start, a_thead, 0, &thread_id);
#endif

  return a_thread->thread_handle;
}

static void zs__thread_cleanup(struct zs__thread *a_thread)
{
  struct zs__thread_list *iter;

  pthread_mutex_lock(&thread_map_mutex);
  iter = zs__thread_map_find(a_thread->thread_handle);
  if(iter) {
    zs__thread_map_remove(iter);
  }
  pthread_mutex_unlock(&thread_map_mutex);

  /*
   * check whether this is the last thread.
   * signal this condition to the master thread if true.
   */
  pthread_mutex_lock(&threads_running_mutex);
  --threads_running;
  if(threads_running == 0) {
    /* signal condition: no more thread running */
    pthread_cond_signal(&threads_running_cv);
  }
  pthread_mutex_unlock(&threads_running_mutex);

  /*
   * check whether there are other threads waiting.
   * signal this condition to these threads if true.
   */
  pthread_mutex_lock(&a_thread->waiting_threads_mutex);
  if(a_thread->waiting_threads > 0) {
    /* signal condition: threads finished */
    pthread_cond_broadcast(&a_thread->waiting_threads_cv);
  }
  pthread_mutex_unlock(&a_thread->waiting_threads_mutex);
  
  pthread_mutex_destroy(&a_thread->waiting_threads_mutex);
  pthread_cond_destroy(&a_thread->waiting_threads_cv);
  free(a_thread);
}

#ifdef linux
static void* zs__thread_start(struct zs__thread *a_thread) {
#else
static DWORD WINAPI zs__thread_start(struct zs__thread *a_thread) {
#endif
  
  pthread_cleanup_push((void(*)(void*)) zs__thread_cleanup, a_thread);

  // Halt starting
  pthread_mutex_lock(&a_thread->waiting_threads_mutex);
  pthread_mutex_unlock(&a_thread->waiting_threads_mutex);

  if(a_thread->run) {
    a_thread->run(a_thread->data);
  }

  pthread_exit(NULL);
  pthread_cleanup_pop(0);

  return 0;
}

// EXPORT
int zs__thread_execute(zs__thread_handle a_thread_handle) { // OK: return true; | FEHLER (z.B. still running): return false;
  struct zs__thread_list *iter;
  int a_back;

  // Look ThreadMap
  pthread_mutex_lock(&thread_map_mutex);
  
  iter = zs__thread_map_find(a_thread_handle);
  if(!iter)
    a_back = 0;
  else {
    // Resume starting
    pthread_mutex_unlock(&iter->thread->waiting_threads_mutex);
    a_back = -1;
  }
  
  // Unlook ThreadMap
  pthread_mutex_unlock(&thread_map_mutex);
  
  return a_back;
}

// EXPORT
void zs__thread_abort(zs__thread_handle a_thread_handle) {
  struct zs__thread_list *iter;

  // Look ThreadMap
  pthread_mutex_lock(&thread_map_mutex);

  iter = zs__thread_map_find(a_thread_handle);
  if(iter) {
      if(iter->thread->abort) {
        iter->thread->abort(iter->thread->data);
      }
      else {
        pthread_cancel(a_thread_handle);
      }
  }
//  else
//    printf("Thread not found!\n");
  
  // Unlook ThreadMap
  pthread_mutex_unlock(&thread_map_mutex);
}

// EXPORT
void zs__thread_wait(zs__thread_handle a_thread_handle) {
  struct zs__thread_list *iter;
  pthread_mutex_t *a_mutex;
  pthread_cond_t *a_cv;

  // Look ThreadMap
  pthread_mutex_lock(&thread_map_mutex);

  iter = zs__thread_map_find(a_thread_handle);
  if(iter) {
    a_mutex = &iter->thread->waiting_threads_mutex;
    a_cv = &iter->thread->waiting_threads_cv;
    pthread_mutex_lock(a_mutex);
    ++(iter->thread->waiting_threads);
  }

  // Unlook ThreadMap
  pthread_mutex_unlock(&thread_map_mutex);

  if(iter) {
    pthread_cond_wait(a_cv, a_mutex);
    pthread_mutex_unlock(a_mutex);
  }
}

// EXPORT
int zs__thread_available(zs__thread_handle a_thread_handle) {
  struct zs__thread_list *iter;

  // Look ThreadMap
  pthread_mutex_lock(&thread_map_mutex);

  iter = zs__thread_map_find(a_thread_handle);

  // Unlook ThreadMap
  pthread_mutex_unlock(&thread_map_mutex);

  if(iter == 0)
    return 0;
  return -1;
}

// EXPORT
void zs__thread_pause(zs__thread_handle a_thread_handle) {
/*  // Look ThreadMap
  zs__semaphore_op(zs__thread_map_sem, 2, -1, 0, 0);

  if(zs__thread_map_find(a_thread_handle)) {
*/
#ifdef linux
    kill(a_thread_handle, SIGSTOP);
#endif
/*  }

  // Unlook ThreadMap
  zs__semaphore_op(zs__thread_map_sem, 2, 1, 0, 0);
*/}

// EXPORT
void zs__thread_resume(zs__thread_handle a_thread_handle) {
/*  // Look ThreadMap
  zs__semaphore_op(zs__thread_map_sem, 2, -1, 0, 0);

  if(zs__thread_map_find(a_thread_handle)) {
*/
#ifdef linux
    kill(a_thread_handle, SIGCONT);
#endif
/*  }

  // Unlook ThreadMap
  zs__semaphore_op(zs__thread_map_sem, 2, 1, 0, 0);
*/}

static struct zs__thread_list* zs__thread_map_find(zs__thread_handle a_thread_handle) {
	struct zs__thread_list *a_list;

	for(a_list=zs__thread_map; a_list; a_list=a_list->next)
		if(a_list->thread)
			if(a_list->thread->thread_handle == a_thread_handle)
				break;

	return a_list;
}

static void zs__thread_map_remove(struct zs__thread_list *a_entry)
{
	struct zs__thread_list *a_list;

	if(zs__thread_map == a_entry) {
		zs__thread_map = zs__thread_map->next;
		free(a_entry);
		return;
	}

	for(a_list=zs__thread_map; a_list; a_list=a_list->next) {
		if(a_list->next != a_entry) {
			a_list->next = a_entry->next;
			free(a_entry);
			return;
		}
	}
	
	// entry not found
}
#if 0
// ******************************************
// ** Deinitialisieren                     **
// ******************************************
  {
    struct zs__thread_list *a_list;

    while(zs__thread_map) {
      a_list = zs__thread_map;
      zs__thread_map = zs__thread_map->next;
      free(a_list);
    }
}
#endif

// ************************************************************************************************
// ***  W A I T                                                                                 ***
// ************************************************************************************************

// EXPORT
struct zs__wait* zs__wait_create(void) {
  struct zs__wait *a_wait_struct;
  
  a_wait_struct = malloc(sizeof(struct zs__wait));
  a_wait_struct->max_number = 0;
  a_wait_struct->timer_number = 0;
  a_wait_struct->first = 0;
  a_wait_struct->last = 0;
  return a_wait_struct;
}

// EXPORT
void zs__wait_destroy(struct zs__wait *a_wait_struct) {
  struct zs__wait_element *a_element1;
  struct zs__wait_element *a_element2;
  a_element1 = a_wait_struct->first;
  while(a_element1) {
    a_element2 = a_element1->next;
    free(a_element1);
    a_element1 = a_element2;
  }
  free(a_wait_struct);
}

// EXPORT
unsigned int zs__wait_do(struct zs__wait *a_wait_struct) {
  int a_max_filediscriptor;
  struct zs__wait_element *a_element;
  struct timeval tv;
  struct timeval *tv_ptr;
  fd_set rfds;
  fd_set wfds;
  unsigned int back;
  int last_errno = 0;
  
  if(!a_wait_struct)
    return 0;
  
  // System-Wartestruktur erstellen
  a_max_filediscriptor = -1;
  FD_ZERO(&rfds);
  FD_ZERO(&wfds);
  for(a_element = a_wait_struct->first; a_element; a_element = a_element->next) {
    FD_SET(a_element->fd, &rfds);
    if(a_max_filediscriptor < a_element->fd)
      a_max_filediscriptor = a_element->fd;
  }
  
  // Warten
  if(a_wait_struct->timer_number > 0) {
    tv.tv_sec = a_wait_struct->sec;
    tv.tv_usec = a_wait_struct->microsec;
    tv_ptr = &tv;
//    printf("Timewait mit %d:%d\n", (int) tv.tv_sec, (int) tv.tv_usec);
  }
  else
    tv_ptr = 0;
  
  while(select(a_max_filediscriptor+1, &rfds, &wfds, 0, tv_ptr) == -1) {
    if(errno != last_errno) {
      switch(errno) {
        case EINTR:
          break;
        default:
          last_errno = errno;
          perror("zs__wait_do: ");
//          FD_ZERO(&rfds);
//          FD_ZERO(&wfds);
//          return 0;
          break;
      }
    }
  }
//  if(select(a_max_filediscriptor+1, &rfds, &wfds, 0, tv_ptr) == -1) {
//    perror("zs__wait_do: ");
//    FD_ZERO(&rfds);
//    FD_ZERO(&wfds);
//    return 0;
//  }
  
  // Ausl�ser ermitteln und System-Wartestruktur l�schen
  back = 0;
  for(a_element = a_wait_struct->first; a_element!=0 && back==0; a_element = a_element->next) {
    if(FD_ISSET(a_element->fd, &rfds))
      back = a_element->number;
  }
  if(back == 0 && a_wait_struct->timer_number > 0)
    back = a_wait_struct->timer_number;
  FD_ZERO(&rfds);
  FD_ZERO(&wfds);
  return back;
}

// EXPORT
unsigned int zs__wait_add_timer(struct zs__wait *a_wait_struct, long sec, long microsec) {
  ++a_wait_struct->max_number;
  if(a_wait_struct->timer_number==0
  ||(a_wait_struct->timer_number>0&&(sec<a_wait_struct->sec
                                   ||(sec==a_wait_struct->sec && microsec<a_wait_struct->microsec)))) {
    a_wait_struct->timer_number = a_wait_struct->max_number;
    a_wait_struct->sec = sec;
    a_wait_struct->microsec = microsec;
  }
  return a_wait_struct->max_number;
}

static unsigned int zs__wait_add_stream_reader_fd(struct zs__wait *a_wait_struct, int a_fd) {
  struct zs__wait_element *a_element;
  
  ++a_wait_struct->max_number;
  a_element = malloc(sizeof(struct zs__wait_element));
  a_element->number = a_wait_struct->max_number;
  a_element->fd = a_fd;
  a_element->next = 0;
  
  if(a_wait_struct->last)
    a_wait_struct->last->next = a_element;
  else
    a_wait_struct->first = a_element;
  a_wait_struct->last = a_element;
  return a_wait_struct->max_number;
}

// EXPORT
unsigned int zs__wait_add_stream_reader(struct zs__wait *a_wait_struct, struct zs__stream *a_stream_struct) {
  if(a_stream_struct->get_fd_reader)
    return zs__wait_add_stream_reader_fd(a_wait_struct, a_stream_struct->get_fd_reader(a_stream_struct));
  return 0;
}

//#include <mcheck.h>
