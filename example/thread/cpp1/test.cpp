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
#include <zsystem/zsthread.h>

class CTestThread : public ZSThread {
public:
  CTestThread(volatile const int &a_i)
  : ZSThread((int(*)(ZSThread &)) &CTestThread::Run, 0),
    i(a_i)
  { }
  ~CTestThread() {
    std::cout << "TestThread waiting..." << std::endl;
    for(int i=0; i<999999999; ++i);
    std::cout << "TestThread finished" << std::endl;
  }

private:
  volatile const int &i;
  static int Run(CTestThread &a_this) {
    int i_1;
    int i_2 = -1;
    std::cout << "TestThread started" << std::endl;
    while(1) {
      i_1 = a_this.i;
      if(i_1 >= 10)
	break;
      if(i_1 == i_2)
	continue;
      std::cout << "Thread: " << i_1 << std::endl;
      i_2 = i_1;
    }
//    while(a_this.i<10)
//      std::cout << "Thread: " << a_this.i << std::endl;
    std::cout << "Thread: " << a_this.i << std::endl;
    return 0;
  }
};

int my_main(void *a_data) {
  volatile int i(0);
  CTestThread *a_thread;
  zs__thread_handle a_thread_handle;

  a_thread = new CTestThread(i);
  a_thread_handle = a_thread->GetHandle();
  std::cout << "Main Thread !" << std::endl;

  ZSThread::Execute(a_thread_handle);
  for(; i<10; ++i)
//    for(int t=0; t<9999999; ++t);
    for(int t=0; t<99999999; ++t);
  std::cout << "Calling ::Wait !" << std::endl;
  ZSThread::Wait(a_thread_handle);
  std::cout << "Hello, world! " << &i << std::endl;

  return 0;
}

int main(int argc, char **argv) {
  return zs__execute(0, my_main);
}
