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

#include <zsystem/zsprocess.h>
#include <iostream>

int my_main(void *a_data);

int main(int a_Argc, char **a_Argv) {
  return zs__execute(0, my_main);
}

int my_main(void *a_data) {
  ZSThread::Handle aHandle;
  ZSReader aReader;
  
  std::cout << "Warte 5 Sekunden!" << std::endl;
  aHandle = (new ZSProcess("/bin/sleep", 0, 0, 1, "5"))->GetHandle();
  ZSThread::Execute(aHandle);
  ZSThread::Wait(aHandle);

  std::cout << "Zeige Inhaltsverzeichnis:" << std::endl;
  aHandle = (new ZSProcess("/bin/ls", &aReader, 0, 0))->GetHandle();
  ZSThread::Execute(aHandle);

//  while(ZSThread::Available(aHandle)) {
  while(!aReader.IsClosed()) {
    std::string aStr;

    aReader >> aStr;
    std::cout << aStr << std::endl;
  }
  ZSThread::Wait(aHandle);
  return 0;
}
