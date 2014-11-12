lockless_tripplebuffer
=================

A lockless single writer, single reader triple buffer
Implemented using C++11 atomic operations

This class can be used to transport data between two threads
it doesn't guarantee that all data will be transported
however it can guarantee access to a complete (predefined) chunk of data

for example: when used to transport audio samples from audio thread to graphics
thread, we can ensure that the graphics thread is always accessing a block
of CONSECUTIVE samples and that this block is the block which was most recently
filled by the audio thread.

this is a slightly modified version of implementation published here:
https://github.com/p4checo/triplebuffer-sync
which in turn was based on following blog post:
http://remis-thoughts.blogspot.pt/2012/01/triple-buffering-as-concurrency_30.html

my contributions:
+ added by-reference access to the content of the buffer
+ added a multithreaded unit test (using gmock)
+ packaged this code as a header only CMAKE project


####To build and run tests:
cd to the top level repository folder:  
```
mkdir build && cd build  
cmake ..  
make
```

the above will automatically download gmock and compile the tests  
then to actually run the test do:
```
test/lockless_tripplebuffer_GTest
```
