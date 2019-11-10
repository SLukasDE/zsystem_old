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

/**
  *@author Sven Lukas
  */

#ifndef ZS__MESSAGELOOP_H
#define ZS__MESSAGELOOP_H

#include <zsystem/zslib.h>
#include <zsystem/zsthread.h>

template<typename Message>
class ZSMessageLoop {
public:
  ZSMessageLoop()
  : messageloop(zs__messageloop_create(this, &ZSMessageLoop<Message>::InternResponse, &ZSMessageLoop<Message>::InternAfterAbort))
  { }

  void PostRequest(Message &a_message) {
    zs__messageloop_post(messageloop, &a_message);
  }
  
  ZSThread::Handle GetHandle() {
    return zs__messageloop_get_thread_handle(messageloop);
  }
  
  
protected:
  virtual ~ZSMessageLoop() { }
  virtual void Response(Message &a_message) = 0;

private:
  static void InternResponse(void *a_instance, void *a_data) {
    ((ZSMessageLoop<Message>*) a_instance)->Response(*((Message*) a_data));
  }
  static void InternAfterAbort(void *a_instance) {
    delete ((ZSMessageLoop<Message>*) a_instance);
  }
  zs__messageloop *messageloop;
};
#endif
