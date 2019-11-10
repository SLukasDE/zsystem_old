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

#ifndef ZS_LIB_H
#define ZS_LIB_H
#include <pthread.h>

/**
  *@author Sven Lukas
  */

#define ZS__READER_CLOSED 0x0001
#define ZS__READER_EOF    0x0002
#define ZS__WRITER_CLOSED 0x0004

#ifdef linux /* for Linux */
#include <unistd.h>
//typedef __pid_t zs__thread_handle;
typedef pthread_t zs__thread_handle;
typedef __pid_t zs__process_handle;
#elif WIN32 /* for Win32 */
#include <windows.h>
typedef HANDLE zs__thread_handle;
typedef HANDLE zs__process_handle;
#endif

struct zs__messageloop;
struct zs__semaphore;
struct zs__stream;
struct zs__wait;
struct sockaddr;

#ifdef __cplusplus
extern "C" {
#endif

int                     zs__execute(void *data, int(*a_main)(void*));
//void                    zs__wait_for_sigterm(void);

zs__thread_handle       zs__thread_create(void *data, int(*a_run)(void*), void(*a_abort)(void*));
int                     zs__thread_execute(zs__thread_handle a_thread_handle); /* OK: return true; | FEHLER (z.B. still running): return false; */
void                    zs__thread_abort(zs__thread_handle a_thread_handle);
int                     zs__thread_available(zs__thread_handle a_thread_handle);
void                    zs__thread_pause(zs__thread_handle a_thread_handle);
void                    zs__thread_resume(zs__thread_handle a_thread_handle);
void                    zs__thread_wait(zs__thread_handle a_thread_handle); /* Darf niemals im Destruktor eines anderen Threads aufgerufen werden -> Deadlock */

int                     zs__process_execute(const char *a_file, char *const a_argv[], struct zs__stream *a_reader, struct zs__stream *a_writer, zs__process_handle *a_process_handle_ptr, void *a_data, void(*a_on_started)(void*)); /* letztes Argument in argv muss NULL sein. Parameter a_stream_struct darf 0 sein. Sonst sollte es sich um einen mit ZSStream::CreateProcess erstellten Stream handeln, so dass die Ein- und/oder Ausgabe extern steuerbar ist. Dabei kann zuvor oder auch anschliessend jeder Zeit der Reader oder Writer geschlossen werden, falls man entweder nur noch an der Ein- oder Ausgabe interessiert ist. Werden zuvor beide Enden geschlossen, oder handelt es sich um einen anderen Streamtyp als einen Process stream, so wird dies erkannt, wodurch a_stream_struct als 0-Pointer interpretiert wird. */
void                    zs__process_terminate(zs__process_handle a_process_handle);
void                    zs__process_kill(zs__process_handle a_process_handle);
   
struct zs__messageloop* zs__messageloop_create(void *a_instance, void(*a_response)(void *instance, void *data), void(*a_after_abort)(void *instance));
int                     zs__messageloop_post(struct zs__messageloop *a_messageloop, void *a_data);
zs__thread_handle       zs__messageloop_get_thread_handle(struct zs__messageloop *a_messageloop);

struct zs__semaphore*   zs__semaphore_create(unsigned char a_count);
struct zs__semaphore*   zs__semaphore_duplicate(struct zs__semaphore *a_sem);
void                    zs__semaphore_destroy(struct zs__semaphore *a_sem);
void                    zs__semaphore_set(struct zs__semaphore *a_sem, unsigned char a_sem_id, int a_value);
int                     zs__semaphore_get(struct zs__semaphore *a_sem, unsigned char a_sem_id);
int                     zs__semaphore_op(struct zs__semaphore *a_sem, unsigned char a_sem_id, int a_value, unsigned long a_sec, unsigned long a_nano_sec);
// if operation was successful, 0 will be returend.
// if a timeout occured, -1 will be returned;
// if an error occured, -2 will be returned;

zs__thread_handle       zs__server_create_local(const char *a_name, void *a_data, void(*a_on_connect)(struct zs__stream*, void*));
zs__thread_handle       zs__server_create_tcp_ipv4(const char *a_ipv4, int a_port, void *a_data, void(*a_on_connect)(struct zs__stream*, void*));
zs__thread_handle       zs__server_create_udp_ipv4(const char *a_ipv4, int a_port, void *a_data, void(*a_on_connect)(struct zs__stream*, void*));

//  struct zs__stream* (*stream_create)(char(*read)(struct zs__stream*), void(*write)(struct zs__stream*, char), void(*close_reader)(struct zs__stream*), void(*close_writer)(struct zs__stream*), unsigned int data_size);
struct zs__stream*      zs__stream_create_system(int a_reader, int a_writer);
struct zs__stream*      zs__stream_create_pipe(void);
//  struct zs__stream* (*stream_create_filereader)(int a_socket);
struct zs__stream*      zs__stream_create_tcp_ipv4(const char *a_src_ipv4, int a_src_port, const char *a_dst_ipv4, int a_dst_port);
// implementierung von stream_create_udp_ipv4 fehlt noch
struct zs__stream*      zs__stream_create_udp_ipv4(const char *a_src_ipv4, int a_src_port, const char *a_dst_ipv4, int a_dst_port);
  
struct zs__stream*      zs__stream_create_local(const char *a_name);
unsigned int            zs__stream_read(struct zs__stream *a_stream, char *a_buffer, unsigned int a_size);
unsigned int            zs__stream_write(struct zs__stream *a_stream, const char *a_buffer, unsigned int a_size);
unsigned int            zs__stream_state(struct zs__stream *a_stream);
void                    zs__stream_close_reader(struct zs__stream *a_stream);
void                    zs__stream_close_writer(struct zs__stream *a_stream);
void                    zs__stream_destroy(struct zs__stream *a_stream);
const char*             zs__stream_get_remote_ipv4_address(struct zs__stream *a_stream_struct);
const char*             zs__stream_get_local_ipv4_address(struct zs__stream *a_stream_struct);
int                     zs__stream_get_remote_port(struct zs__stream *a_stream_struct);
int                     zs__stream_get_local_port(struct zs__stream *a_stream_struct);

struct zs__wait*        zs__wait_create(void);
void                    zs__wait_destroy(struct zs__wait *a_wait_struct);
unsigned int            zs__wait_do(struct zs__wait *a_wait_struct);
unsigned int            zs__wait_add_timer(struct zs__wait *a_wait_struct, long sec, long microsec);
unsigned int            zs__wait_add_stream_reader(struct zs__wait *a_wait_struct, struct zs__stream *a_stream_struct);

#ifdef __cplusplus
}
#endif

#endif
