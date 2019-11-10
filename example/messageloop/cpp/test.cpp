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
#include <string>
#include <cstdlib>
#include <zsystem/zsmessageloop.h>

class CTestMessageLoop : public ZSMessageLoop<std::string> {
public:
  CTestMessageLoop()
  : ZSMessageLoop<std::string>()
  { }

protected:
  ~CTestMessageLoop() {
    std::cout << "MessageLoop: exit" << std::endl;
  }
  void Response(std::string &a_str) {
    std::cout << "MessageLoop: \"" << a_str << "\"" << std::endl;
    delete &a_str;
  }
};

int my_main(void *a_data) {
  CTestMessageLoop *a_messageloop;
  zs__thread_handle a_thread_handle;

  a_messageloop = new CTestMessageLoop();
  a_thread_handle = a_messageloop->GetHandle();
  ZSThread::Execute(a_thread_handle);
  for(unsigned int i=0; i<0xffffff; ++i);
  a_messageloop->PostRequest(*(new std::string("Hello")));
//  for(unsigned int i=0; i<0xffffffff; ++i);
  a_messageloop->PostRequest(*(new std::string("world")));
//  for(unsigned int i=0; i<0xffffffff; ++i);
  a_messageloop->PostRequest(*(new std::string("!")));
//  for(unsigned int i=0; i<0xffffffff; ++i);
  ZSThread::Abort(a_thread_handle);
//  for(unsigned int i=0; i<0xffffffff; ++i);
  ZSThread::Wait(a_thread_handle);
  return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  return zs__execute(0, my_main);
}
