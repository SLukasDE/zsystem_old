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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <zsystem/zslib.h>

void messageloop_response(void *a_instance, void *a_data) {
  int *i;
  i = (int*) a_data;
  printf("Thread: %d\n", *i);
  free(a_data);
}

void messageloop_after_abort(void *a_instance) {
  printf("Thread: after abort\n");
}

int my_main(void *data) {
  struct zs__messageloop *a_messageloop;
  zs__thread_handle a_thread_handle;
  int *i_ptr;
  int i;

  a_messageloop = zs__messageloop_create(0, &messageloop_response, &messageloop_after_abort);
  a_thread_handle = zs__messageloop_get_thread_handle(a_messageloop);
  zs__thread_execute(a_thread_handle);
  for(i=0; i<10; ++i) {
    i_ptr = malloc(sizeof(int));
    *i_ptr = i;
    zs__messageloop_post(a_messageloop, i_ptr);
  }
  zs__thread_abort(a_thread_handle);
  zs__thread_wait(a_thread_handle);
  printf("Hello, world!\n");

  return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  return zs__execute(0, my_main);
//  return zs__execute(my_main);
}
