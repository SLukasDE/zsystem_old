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

#ifndef ZS__THREAD
#define ZS__THREAD

#include <zsystem/zslib.h>

class ZSThread {
public:
  typedef zs__thread_handle Handle;
  
  ZSThread(int(*a_run)(ZSThread &), void(*a_abort)(ZSThread &))
  : thread_handle(zs__thread_create(this, ZSThread::InternRun, ZSThread::InternAbort)),
    run(a_run),
    abort(a_abort)
  { }
  
  ZSThread::Handle GetHandle() const {
    return thread_handle;
  }

  static int Main(int argc, char *argv[]);

  static bool Execute(ZSThread::Handle a_thread_handle) { // OK: return true; | FEHLER (z.B. still running): return false;
    return zs__thread_execute(a_thread_handle)!=0;
  }
  static void Abort(ZSThread::Handle a_thread_handle) {
    zs__thread_abort(a_thread_handle);
  }
  static bool Available(ZSThread::Handle a_thread_handle) {
    return zs__thread_available(a_thread_handle)!=0;
  }
  static void Pause(ZSThread::Handle a_thread_handle)  {
    zs__thread_pause(a_thread_handle);
  }
  static void Resume(ZSThread::Handle a_thread_handle) {
    zs__thread_resume(a_thread_handle);
  }

  // Wait darf niemals im Destruktor eines anderen Threads
  // aufgerufen werden -> Deadlock
  static void Wait(ZSThread::Handle a_thread_handle) {
    zs__thread_wait(a_thread_handle);
  }

protected:
  virtual ~ZSThread() { }

private:
  static int InternRun(void *a_thread) {
    int a_back;
    if(((ZSThread*)a_thread)->run)
      a_back = ((ZSThread*)a_thread)->run(*(ZSThread*)a_thread);
    else
      a_back = 0;
    delete (ZSThread*)a_thread;
    return a_back;
  }
  static void InternAbort(void *a_thread) {
    if(a_thread)
      if(((ZSThread*)a_thread)->abort)
        ((ZSThread*)a_thread)->abort(*(ZSThread*)a_thread);
  }

  ZSThread::Handle thread_handle;
  int(*run)(ZSThread &);
  void(*abort)(ZSThread &);
};

#endif /* !ZS__THREAD */
