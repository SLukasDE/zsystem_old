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
#include <zsystem/zsstream.h>
#include <zsystem/zswait.h>

class CTestThread : public ZSThread {
public:
  CTestThread(const ZSReader &a_reader)
  : ZSThread((int(*)(ZSThread &)) &CTestThread::Run, 0),
    reader(a_reader)
  { }

private:
  ZSReader reader;
  static int Run(CTestThread &a_this) {
    std::string a_str;
    ZSWait a_wait;
    unsigned int read_event_id;

    read_event_id = a_wait.AddStreamReader(a_this.reader);
    a_wait.AddTimer(1, 0);
    while(true) {
      std::cout << "Thread: Wait..." << std::endl;
      for(unsigned int i=1; a_wait() != read_event_id; ++i)
        std::cout << "Thread: " << i << " Sekunden" << std::endl;
      std::cout << "Thread: I got something..." << std::endl;
      a_this.reader >> a_str;
      if(a_str == "")
        break;
      std::cout << "Thread: \"" << a_str << "\"" << std::endl;
    }
    std::cout << "Thread exit" << std::endl;
    return 0;
  }
};

int my_main(void *a_data) {
  CTestThread *a_thread;
  zs__thread_handle a_thread_handle;
  ZSWriter a_writer;

  {
    std::pair<ZSReader, ZSWriter> a_pair(ZSStream::CreatePipe());
    a_thread = new CTestThread(a_pair.first);
    a_writer = a_pair.second;
  }

  a_thread_handle = a_thread->GetHandle();
  ZSThread::Execute(a_thread_handle);
  for(unsigned int i=0; i<0xffffffff; ++i);
  a_writer << "Hello\n";
  a_writer << "World\n";
  for(unsigned int i=0; i<0xffffffff; ++i);
  a_writer << "!\n";
  a_writer << "\n";

  ZSThread::Wait(a_thread_handle);
  return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  return zs__execute(0, my_main);
}
