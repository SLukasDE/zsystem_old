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
#include "test2.h"

CTestThread::CTestThread(volatile int &a_i, ZSSemaphore &a_semaphore, int a_max)
: ZSThread((int(*)(ZSThread &)) &CTestThread::Run, 0),
  i(a_i),
  semaphore(a_semaphore),
  max(a_max)
{ }

int CTestThread::Run(CTestThread &a_this) {
  while(true) {
    a_this.semaphore.Op(1, -1);
    std::cout << "Thread: " << a_this.i << std::endl;
    if(a_this.i >= a_this.max)
      break;
    a_this.semaphore.Op(0, 1);
  }
  return 0;
}
