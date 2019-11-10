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

#ifndef ZS__WAIT
#define ZS__WAIT

#include <zsystem/zslib.h>
#include <zsystem/zsstream.h>

class ZSWait {
public:
  ZSWait()
  : wait_struct(0)
  { }
  ~ZSWait() {
    if(wait_struct)
      zs__wait_destroy(wait_struct);
  }
  unsigned int AddTimer(long sec, long microsec) {
    if(!wait_struct)
      wait_struct = zs__wait_create();
    return zs__wait_add_timer(wait_struct, sec, microsec);
  }
  unsigned int AddStreamReader(const ZSReader &a_stream_reader) {
    if(!a_stream_reader.stream)
      return 0;
    if(!wait_struct)
      wait_struct = zs__wait_create();
    return zs__wait_add_stream_reader(wait_struct, a_stream_reader.stream->stream_struct);
  }
  unsigned int operator()() {
    if(wait_struct)
      return zs__wait_do(wait_struct);
    return 0;
  }
private:
  struct zs__wait *wait_struct;
};

#endif /* !SLSTD__WAIT */
