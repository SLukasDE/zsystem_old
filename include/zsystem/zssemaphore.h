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

#ifndef ZS__SEMAPHORE
#define ZS__SEMAPHORE

#include <zsystem/zslib.h>

class ZSSemaphore {
public:
  ZSSemaphore(unsigned char a_count)
  : semaphore_struct(zs__semaphore_create(a_count))
  { }
  ZSSemaphore(const ZSSemaphore &a_semaphore)
  : semaphore_struct(zs__semaphore_duplicate(a_semaphore.semaphore_struct))
  { }
  ~ZSSemaphore() {
    zs__semaphore_destroy(semaphore_struct);
  }

  void Set(unsigned char a_sem_id, int value) {
    zs__semaphore_set(semaphore_struct, a_sem_id, value);
  }
  int Get(unsigned char a_sem_id) {
    return zs__semaphore_get(semaphore_struct, a_sem_id);
  }
  int Op(unsigned char a_sem_id, int value, unsigned long a_sec = 0, unsigned long a_nano_sec = 0) {
    return zs__semaphore_op(semaphore_struct, a_sem_id, value, a_sec, a_nano_sec);
  }

private:
  zs__semaphore *semaphore_struct;
};

#endif /* !SLSTD__SEMAPHORE */
