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

#ifndef ZS__SERVER
#define ZS__SERVER

#include <zsystem/zslib.h>
#include <zsystem/zsthread.h>

class ZSServer {
public:
  enum StreamProtocol { spLocal, spTCP, spUDP };
  
  ZSThread::Handle GetHandle() const {
    return thread_handle;
  }
  
  template <class Server>
  static bool DeleteOnFailure(Server &aServer) {
    ZSServer *aZSServer(&aServer);
    
    if(aZSServer->GetHandle() > 0)
      return false;
    delete aZSServer;
    return true;
  }
    
protected:
  ZSServer(ZSServer::StreamProtocol aStreamProtocol, const char *a_ipv4, int a_port)
  {
    switch(aStreamProtocol) {
      case ZSServer::spUDP:
        thread_handle = zs__server_create_udp_ipv4(a_ipv4, a_port, this, ZSServer::InternOnConnect);
        break;
      case ZSServer::spTCP:
        thread_handle = zs__server_create_tcp_ipv4(a_ipv4, a_port, this, ZSServer::InternOnConnect);
        break;
      default:
        thread_handle = -1;
        break;
    }
  }
  ZSServer(ZSServer::StreamProtocol aStreamProtocol, const char* a_name)
  {
    switch(aStreamProtocol) {
/*
      case ZSServer::spUDP:
        thread_handle = zs__server_create_udp_ipv4(a_ipv4, a_port, this, ZSServer::InternOnConnect);
        break;
      case ZSServer::spTCP:
        thread_handle = zs__server_create_tcp_ipv4(a_ipv4, a_port, this, ZSServer::InternOnConnect);
        break;
*/
      case ZSServer::spLocal:
        thread_handle = zs__server_create_local(a_name, this, ZSServer::InternOnConnect);
        break;
      default:
        thread_handle = -1;
        break;
    }
  }
  virtual ~ZSServer() {  }
  virtual void OnConnect(struct zs__stream *a_stream_struct) { }

private:
  static void InternOnConnect(struct zs__stream *a_stream_struct, void *a_data) {
    ((ZSServer*) a_data)->OnConnect(a_stream_struct);
  }
  zs__thread_handle thread_handle;
};

#endif /* !SLSTD__SERVERTCPIP */
