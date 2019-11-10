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

#ifndef ZS__STREAM
#define ZS__STREAM

#include <zsystem/zslib.h>
#include <string>
#include <string.h>
#include <stdio.h>
//#include <iostream>

class ZSWait;
class ZSProcess;
class ZSStream;
class ZSReader;
class ZSWriter;


class ZSStream {
public:
friend class ZSReader;
friend class ZSWriter;
friend class ZSWait;
friend class ZSProcess;
  
  static inline std::pair<ZSReader, ZSWriter> Create(struct zs__stream *a_stream_struct);
  static inline std::pair<ZSReader, ZSWriter> CreatePipe();
  static inline std::pair<ZSReader, ZSWriter> CreateSystem(int a_reader, int a_writer);
  static inline std::pair<ZSReader, ZSWriter> CreateTCP_IPv4(const char *a_src_ipv4, int a_src_port, const char a_dst_ipv4[4], int a_dst_port);
  static inline std::pair<ZSReader, ZSWriter> CreateUDP_IPv4(const char *a_src_ipv4, int a_src_port, const char a_dst_ipv4[4], int a_dst_port);
//  static inline std::pair<ZSReader, ZSWriter> CreateTCP_IPv4(const char a_ipv4[4], int a_port);
  static inline std::pair<ZSReader, ZSWriter> CreateLocal(const std::string &a_name);

private:
  ZSStream(zs__stream *a_stream_struct)
  : stream_struct(a_stream_struct),
    reader_count(0),
    writer_count(0)
  { }
  virtual ~ZSStream() {
    if(stream_struct)
      zs__stream_destroy(stream_struct);
  }
  void RegisterReader() {
    ++reader_count;
  }
  void RegisterWriter() {
    ++writer_count;
  }
  bool UnregisterReader() {
    --reader_count;
    if(reader_count == 0 && stream_struct)
      zs__stream_close_reader(stream_struct);
    return (reader_count == 0 && writer_count == 0);
  }
  bool UnregisterWriter() {
    --writer_count;
    if(writer_count == 0 && stream_struct)
      zs__stream_close_writer(stream_struct);
    return (reader_count == 0 && writer_count == 0);
  }
  unsigned int Read(char *a_buffer, unsigned int a_size) {
    if(stream_struct)
      return zs__stream_read(stream_struct, a_buffer, a_size);
    return 0;
  }
  unsigned int Write(const char *a_buffer, unsigned int a_size) {
    if(stream_struct)
      return zs__stream_write(stream_struct, a_buffer, a_size);
    return 0;
  }
  const char* GetRemoteIPv4Address() {
    return stream_struct ? zs__stream_get_remote_ipv4_address(stream_struct) : 0;
  }
  const char* GetLocalIPv4Address() {
    return stream_struct ? zs__stream_get_local_ipv4_address(stream_struct) : 0;
  }
  int GetRemotePort() {
    return stream_struct ? zs__stream_get_remote_port(stream_struct) : 0;
  }
  int GetLocalPort() {
    return stream_struct ? zs__stream_get_local_port(stream_struct) : 0;
  }
  unsigned int GetState() {
    if(stream_struct)
      return zs__stream_state(stream_struct);
    return 0;
  }
  zs__stream *stream_struct;
  volatile unsigned int reader_count;
  volatile unsigned int writer_count;
};

class ZSReader {
friend class ZSWait;
friend class ZSProcess;
public:
  ZSReader()
  : stream(0)
  { }
  ZSReader(ZSStream &a_stream)
  : stream(&a_stream)
  {
    stream->RegisterReader();
  }
  ZSReader(const ZSReader &a_stream_reader)
  : stream(a_stream_reader.stream)
  {
    if(stream)
      stream->RegisterReader();
  }
  ~ZSReader() {
    if(stream)
      if(stream->UnregisterReader())
        delete stream;
  }
  ZSReader& operator=(const ZSReader &a_reader) {
    if(a_reader.stream == stream)
      return *this;
    if(stream)
      if(stream->UnregisterReader())
        delete stream;
    stream = a_reader.stream;
    if(stream)
      stream->RegisterReader();
    return *this;
  }
  const char* GetRemoteIPv4Address() {
    return stream ? stream->GetRemoteIPv4Address() : 0;
  }
  const char* GetLocalIPv4Address() {
    return stream ? stream->GetLocalIPv4Address() : 0;
  }
  int GetRemotePort() {
    return stream ? stream->GetRemotePort() : 0;
  }
  int GetLocalPort() {
    return stream ? stream->GetLocalPort() : 0;
  }
  bool IsClosed() {
    if(stream)
      return stream->GetState() & ZS__READER_CLOSED;
    return true;
  }
  bool IsEof() {
    if(stream)
      return stream->GetState() & ZS__READER_EOF;
    return true;
  }
  unsigned int Read(char *a_buffer, unsigned int a_size) {
    if(IsClosed())
      return 0;
    return stream->Read(a_buffer, a_size);
  }
  ZSReader& operator>>(char &a_char) {
    while(!IsClosed())
      if(stream->Read(&a_char, 1) == 1)
        break;
    return *this;
  }
  ZSReader& operator>>(std::string &a_string) {
    char a_char;
   
    a_string = "";
    while(!IsClosed()) {
      if(stream->Read(&a_char, 1) == 1)
        if(a_char == '\n')
          break;
//        else if(a_char == '\r')
//          break;
        else
          a_string += a_char;
    }
    return *this;
  }
  ZSReader& operator>>(int &a_value) {
    char a_char;
    std::string a_string;
    
    while(!IsClosed())
      if(stream->Read(&a_char, 1) == 1)
        if(a_char == '\n')
          break;
        else
          a_string += a_char;
    sscanf(a_string.c_str(), "%d", &a_value);
    return *this;
  }
  ZSReader& operator>>(unsigned int &a_value) {
    char a_char;
    std::string a_string;
   
    while(!IsClosed())
      if(stream->Read(&a_char, 1) == 1)
        if(a_char == '\n')
          break;
        else
          a_string += a_char;
    sscanf(a_string.c_str(), "%u", &a_value);
    return *this;
  }
  ZSReader& operator>>(bool &a_value) {
    char a_char;
    std::string a_string;
    
    while(!IsClosed())
      if(stream->Read(&a_char, 1) == 1)
        if(a_char == '\n')
          break;
        else
          a_string += a_char;
    a_value = (a_string == "true");
    return *this;
  }
private:
  ZSStream *stream;
};

class ZSWriter {
friend class ZSProcess;
public:
  ZSWriter()
  : stream(0)
  { }
  ZSWriter(ZSStream &a_stream)
  : stream(&a_stream)
  {
    if(stream)
      stream->RegisterWriter();
  }
  ZSWriter(const ZSWriter &a_stream_writer)
  : stream(a_stream_writer.stream)
  {
    if(stream)
      stream->RegisterWriter();
  }
  ~ZSWriter() {
    if(stream)
      if(stream->UnregisterWriter())
        delete stream;
  }

  ZSWriter& operator = (const ZSWriter &a_writer) {
    if(a_writer.stream == stream)
      return *this;
    if(stream)
      if(stream->UnregisterWriter())
        delete stream;
    stream = a_writer.stream;
    if(stream)
      stream->RegisterWriter();
    return *this;
  }
  const char* GetRemoteIPv4Address() {
    return stream ? stream->GetRemoteIPv4Address() : 0;
  }
  const char* GetLocalIPv4Address() {
    return stream ? stream->GetLocalIPv4Address() : 0;
  }
  int GetRemotePort() {
    return stream ? stream->GetRemotePort() : 0;
  }
  int GetLocalPort() {
    return stream ? stream->GetLocalPort() : 0;
  }
  bool IsClosed() {
    if(stream)
      return stream->GetState() & ZS__WRITER_CLOSED;
    return true;
  }
  unsigned int Write(const char *a_buffer, unsigned int a_size) {
    if(IsClosed())
      return 0;
    return stream->Write(a_buffer, a_size);
  }
  ZSWriter& operator << (char a_char) {
    while(!IsClosed())
      if(stream->Write(&a_char, 1) == 1)
        break;
    return *this;
  }
  ZSWriter& operator << (const char *a_string) {
    unsigned int a_length;
    unsigned int a_back;
      
    a_length = strlen(a_string);
    while(!IsClosed() && a_length > 0) {
      a_back = stream->Write(a_string, a_length);
      a_length -= a_back;
      a_string = &a_string[a_back];
    }
    return *this;
  }
  ZSWriter& operator << (const std::string &a_string) {
    unsigned int a_length;
    unsigned int a_back;
    const char *a_data;
      
    a_data = a_string.data();
    a_length = a_string.size();
    while(!IsClosed() && a_length > 0) {
      a_back = stream->Write(a_data, a_length);
      a_length -= a_back;
      a_data = &a_data[a_back];
    }
    return *this;
  }
  ZSWriter& operator << (int a_value) {
    char a_buffer[1024];
    unsigned int a_length;
    unsigned int a_back;
    const char *a_data(a_buffer);
      
    sprintf(a_buffer, "%d", a_value);
    a_length = strlen(a_buffer);
    while(!IsClosed() && a_length > 0) {
      a_back = stream->Write(a_data, a_length);
      a_length -= a_back;
      a_data = &a_data[a_back];
    }
    return *this;
  }
  ZSWriter& operator << (unsigned int a_value) {
    char a_buffer[1024];
    unsigned int a_length;
    unsigned int a_back;
    const char *a_data(a_buffer);
      
    sprintf(a_buffer, "%u", a_value);
    a_length = strlen(a_buffer);
    while(!IsClosed() && a_length > 0) {
      a_back = stream->Write(a_data, a_length);
      a_length -= a_back;
      a_data = &a_data[a_back];
    }
    return *this;
  }
  ZSWriter& operator << (double a_value) {
    char a_buffer[1024];
    unsigned int a_length;
    unsigned int a_back;
    const char *a_data(a_buffer);
      
    sprintf(a_buffer, "%lf", a_value);
    a_length = strlen(a_buffer);
    while(!IsClosed() && a_length > 0) {
      a_back = stream->Write(a_data, a_length);
      a_length -= a_back;
      a_data = &a_data[a_back];
    }
    return *this;
  }
  ZSWriter& operator << (bool a_value) {
    const char *a_data;
    unsigned int a_length;
    unsigned int a_back;
      
    if(a_value) {
      a_data = "true";
      a_length = 4;
    }
    else {
      a_data = "false";
      a_length = 5;
    }
    while(!IsClosed() && a_length > 0) {
      a_back = stream->Write(a_data, a_length);
      a_length -= a_back;
      a_data = &a_data[a_back];
    }
    return *this;
  }
  ZSWriter& operator << (const void *a_pointer) {
    char a_buffer[1024];
    unsigned int a_length;
    unsigned int a_back;
    const char *a_data(a_buffer);
      
    sprintf(a_buffer, "%p", a_pointer);
    a_length = strlen(a_buffer);
    while(!IsClosed() && a_length > 0) {
      a_back = stream->Write(a_data, a_length);
      a_length -= a_back;
      a_data = &a_data[a_back];
    }
    return *this;
  }
private:
  ZSStream *stream;
};

inline std::pair<ZSReader, ZSWriter> ZSStream::Create(struct zs__stream *a_stream_struct) {
  if(a_stream_struct) {
    ZSStream *a_stream(new ZSStream(a_stream_struct));
    return std::make_pair(ZSReader(*a_stream), ZSWriter(*a_stream));
  }
  return std::make_pair(ZSReader(), ZSWriter());
}

inline std::pair<ZSReader, ZSWriter> ZSStream::CreatePipe() {
  zs__stream *a_stream_struct;

  a_stream_struct = zs__stream_create_pipe();
  if(a_stream_struct) {
    ZSStream *a_stream(new ZSStream(a_stream_struct));
    return std::make_pair(ZSReader(*a_stream), ZSWriter(*a_stream));
  }
  return std::make_pair(ZSReader(), ZSWriter());
}

inline std::pair<ZSReader, ZSWriter> ZSStream::CreateSystem(int a_reader, int a_writer) {
  zs__stream *a_stream_struct;

  a_stream_struct = zs__stream_create_system(a_reader, a_writer);
  if(a_stream_struct) {
    ZSStream *a_stream(new ZSStream(a_stream_struct));
    return std::make_pair(ZSReader(*a_stream), ZSWriter(*a_stream));
  }
  return std::make_pair(ZSReader(), ZSWriter());
}

//inline std::pair<ZSReader, ZSWriter> ZSStream::CreateTCP_IPv4(const char a_ipv4[4], int a_port) {
//  zs__stream *a_stream_struct;

//  a_stream_struct = zs__stream_create_tcp_ipv4(0, 0, a_ipv4, a_port);
//  if(a_stream_struct) {
//    ZSStream *a_stream(new ZSStream(a_stream_struct));
//    return std::make_pair(ZSReader(*a_stream), ZSWriter(*a_stream));
//  }
//  return std::make_pair(ZSReader(), ZSWriter());
//}

inline std::pair<ZSReader, ZSWriter> ZSStream::CreateTCP_IPv4(const char *a_src_ipv4, int a_src_port, const char a_dst_ipv4[4], int a_dst_port) {
  zs__stream *a_stream_struct;

  a_stream_struct = zs__stream_create_tcp_ipv4(a_src_ipv4, a_src_port, a_dst_ipv4, a_dst_port);
  if(a_stream_struct) {
    ZSStream *a_stream(new ZSStream(a_stream_struct));
    return std::make_pair(ZSReader(*a_stream), ZSWriter(*a_stream));
  }
  return std::make_pair(ZSReader(), ZSWriter());
}

inline std::pair<ZSReader, ZSWriter> ZSStream::CreateUDP_IPv4(const char *a_src_ipv4, int a_src_port, const char a_dst_ipv4[4], int a_dst_port) {
  zs__stream *a_stream_struct;

  a_stream_struct = zs__stream_create_udp_ipv4(a_src_ipv4, a_src_port, a_dst_ipv4, a_dst_port);
  if(a_stream_struct) {
    ZSStream *a_stream(new ZSStream(a_stream_struct));
    return std::make_pair(ZSReader(*a_stream), ZSWriter(*a_stream));
  }
  return std::make_pair(ZSReader(), ZSWriter());
}

inline std::pair<ZSReader, ZSWriter> ZSStream::CreateLocal(const std::string &a_name) {
  zs__stream *a_stream_struct;

  a_stream_struct = zs__stream_create_local(a_name.c_str());
  if(a_stream_struct) {
    ZSStream *a_stream(new ZSStream(a_stream_struct));
    return std::make_pair(ZSReader(*a_stream), ZSWriter(*a_stream));
  }
  return std::make_pair(ZSReader(), ZSWriter());
}

#endif /* !SLSTD__STREAM */
