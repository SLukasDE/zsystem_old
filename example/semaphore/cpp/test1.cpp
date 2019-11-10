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

#include <iostream>
#include <cstdlib>
#include <zsystem/zsthread.h>
#include <zsystem/zssemaphore.h>
#include "test2.h"
#define COUNT 20

int my_main(void *a_data) {
  ZSSemaphore semaphore(2);
  volatile int i(0);
  zs__thread_handle a_thread_handle;

  CTestThread *a_thread(new CTestThread(i, semaphore, COUNT));
  semaphore.Set(0, 1);
  semaphore.Set(1, 0);
  a_thread_handle = a_thread->GetHandle();
  ZSThread::Execute(a_thread_handle);

  while(i<COUNT) {
    semaphore.Op(0, -1);
    ++i;
    semaphore.Op(1, 1);
  }
  ZSThread::Wait(a_thread_handle);
  std::cout << "Hello, world! " << i << std::endl;

  return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  return zs__execute(0, my_main);
}
