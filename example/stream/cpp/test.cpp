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

class CTestThread : public ZSThread {
public:
  CTestThread(const ZSReader &a_reader)
  : ZSThread((int(*)(ZSThread &)) &CTestThread::Run, 0),
    reader(a_reader)
  { }
  ~CTestThread() {
    std::cout << "Thread exit" << std::endl;
  }

private:
  ZSReader reader;
  static int Run(CTestThread &a_this) {
    std::string a_str;

    while(true) {
      a_this.reader >> a_str;
      if(a_str == "")
        break;
      std::cout << "Thread: \"" << a_str << "\"" << std::endl;
    }
    return 0;
  }
};

int my_main(void *a_data) {
  ZSThread::Handle a_thread_handle;
  ZSWriter a_writer;
  std::pair<ZSReader, ZSWriter> a_pair;

  a_pair = ZSStream::CreatePipe();
  a_thread_handle = (new CTestThread(a_pair.first))->GetHandle();
  a_writer = a_pair.second;

  ZSThread::Execute(a_thread_handle);

  a_writer << "Test\n";
  a_writer << "1\n";
  a_writer << "2\n";
  a_writer << "3\n";
  a_writer << "\n";

  ZSThread::Wait(a_thread_handle);
  return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  return zs__execute(0, my_main);
}
