# PascalParser
Simple Pascal interpreter and parser. Also providing tool for converting Pascal sources to C++.

- To see how to use interpreter and bindings, see tests directory.

# requirements

- CMake 3.5
- C++11 compiler
- Boost headers
- Qt5Core (Qt4 will work too with small cmake fixes)
- Qt5Tests to run parser tests.  

To use pascal to C++ converter, compile Pascal2cpp target, then run it :  
```./Pascal2cpp pascalFilename.pas cppOutput.cpp```  
Translating units currently unsupported, but can be done with some straight fixes.

