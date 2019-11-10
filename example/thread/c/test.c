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

int thread1(void *a_data) {
  zs__thread_handle a_thread_handle = *(zs__thread_handle*) a_data;

  printf("Thread 1: Waiting for thread 2\n");
  zs__thread_wait(a_thread_handle);
  printf("Thread 1: Done\n");
  return 0;
}

int thread2(void *a_data) {
  int j = 0;
  volatile int *i;
  i = (int*) a_data;
  while(j<10) {
    if(*i == j)
      continue;
    j = *i;
    printf("Thread 2: %d\n", j);
  }
  return 0;
}

int my_main(void *a_data)
{
  volatile int i;
  int t;
//  zs__thread_handle a_thread_handle1;
  zs__thread_handle a_thread_handle2;

  i=0;
  a_thread_handle2 = zs__thread_create((void*)&i, thread2, 0);
  zs__thread_execute(a_thread_handle2);

//  a_thread_handle1 = zs__thread_create((void*)&a_thread_handle2, thread1, 0);
//  zs__thread_execute(a_thread_handle1);

  for(; i<10; ++i)
    for(t=0; t<9999999; ++t);

//  zs__thread_wait(a_thread_handle1);
  printf("A\n");
  for(; i<20; ++i) {
    for(t=0; t<9999999; ++t);
    printf(".\n");
  }
  printf("B\n");
  zs__thread_wait(a_thread_handle2);
  printf("Hello, world! %p\n", &i);

  return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  return zs__execute(0, my_main);
}
