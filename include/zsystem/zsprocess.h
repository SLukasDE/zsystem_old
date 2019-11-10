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

#ifndef ZS__PROCESS
#define ZS__PROCESS

#include <zsystem/zslib.h>
#include <zsystem/zsthread.h>
#include <zsystem/zsstream.h>
#include <stdarg.h>
#include <string.h>
#include <string>
#include <vector>

class ZSProcess : public ZSThread {
public:
  typedef zs__process_handle Handle;

  ZSProcess(const std::string &aFile, ZSReader *aReader, ZSWriter *aWriter, unsigned int aArgc, char *const aArgv[])
  : ZSThread((int(*)(ZSThread &)) &ZSProcess::InternRun, (void(*)(ZSThread &)) &ZSProcess::InternAbort),
    File(aFile),
    Arguments(aArgc),
    process_handle(0),
    stream_reader(0),
    stream_writer(0)
  {
    for(unsigned int i=0; i<aArgc; i++)
      Arguments[i] = aArgv[i];
    
    if(aReader) {
      stream_reader = zs__stream_create_pipe();
      *aReader = ZSStream::Create(stream_reader).first;
    }

    if(aWriter) {
      stream_writer = zs__stream_create_pipe();
      *aWriter = ZSStream::Create(stream_writer).second;
    }
  }
  
  ZSProcess(const std::string &aFile, ZSReader *aReader, ZSWriter *aWriter, unsigned int aArgc, ...)
  : ZSThread((int(*)(ZSThread &)) &ZSProcess::InternRun, (void(*)(ZSThread &)) &ZSProcess::InternAbort),
    File(aFile),
    Arguments(aArgc),
    process_handle(0),
    stream_reader(0),
    stream_writer(0)
  {
    va_list aParams; // Zugriffshandle f�r Parameter
    
    va_start(aParams, aArgc); // Zugriff vorbereiten
    // Durchlaufe alle Parameter (steht in Anzahl)
    for(unsigned int i=0; i<aArgc; i++) {
      char *aTmp(va_arg(aParams, char*)); // hole den Parameter
      Arguments[i] = aTmp;
    }
    va_end(aParams); // Zugriff abschlie�en
    
    if(aReader) {
      stream_reader = zs__stream_create_pipe();
      *aReader = ZSStream::Create(stream_reader).first;
    }

    if(aWriter) {
      stream_writer = zs__stream_create_pipe();
      *aWriter = ZSStream::Create(stream_writer).second;
    }
  }
  
  ZSProcess::Handle GetHandleProcess() {
    return process_handle;
  }
  
protected:
  virtual ~ZSProcess() {
  }
  
  virtual void OnStarted() {
  }

private:
  static void InternOnStarted(ZSProcess *aProcess) {
    if(aProcess)
      aProcess->OnStarted();
  }
  
  static int InternRun(ZSProcess &aProcess) {
    int aBack;
    char ** aArgv;
    
    aArgv = new char* [aProcess.Arguments.size()+2];
    aArgv[0] = (char*) aProcess.File.c_str();
    for(unsigned int i=0; i<aProcess.Arguments.size(); i++)
      aArgv[i+1] = (char*) aProcess.Arguments[i].c_str();
    aArgv[aProcess.Arguments.size()+1] = 0;
    aBack = zs__process_execute(aProcess.File.c_str(), aArgv, aProcess.stream_reader, aProcess.stream_writer, &aProcess.process_handle, &aProcess, (void(*)(void*)) InternOnStarted);
    delete[] aArgv;
    return aBack;
  }
  
  static void InternAbort(ZSProcess &aProcess) {
    zs__process_terminate(aProcess.process_handle);
//    zs__process_kill(aProcess.process_handle);
  }
  
  std::string File;
  std::vector<std::string> Arguments;
  ZSProcess::Handle process_handle;
  zs__stream *stream_reader;
  zs__stream *stream_writer;
};

#endif /* !ZS__PROCESS */
