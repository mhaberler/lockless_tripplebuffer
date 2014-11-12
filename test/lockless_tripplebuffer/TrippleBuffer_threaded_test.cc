//  Copyright 2014, Adrian Gierakowski
//
//  TrippleBuffer_test_threads.cc
//  threaded test for TrippleBuffer.cc

#include <iostream>
#include <chrono>
#include <thread>
#include <array>
#include <algorithm>


#include <gtest/gtest.h>
#include <lockless_tripplebuffer/TripleBuffer.h>

TEST(tripplebuffer, array_test1) {
  TripleBuffer<std::array<int, 3>> buffer;

  auto& bufferWriteRef = buffer.getWriteRef();

  bufferWriteRef[0] = 1;
  bufferWriteRef[1] = 2;
  bufferWriteRef[2] = 3;

  buffer.flipWriter();

  buffer.newSnap();

  ASSERT_EQ(1, buffer.getReadRef()[0]);
  ASSERT_EQ(2, buffer.getReadRef()[1]);
  ASSERT_EQ(3, buffer.getReadRef()[2]);
}

TEST(tripplebuffer, array_test2) {
  TripleBuffer<std::array<int, 3>> buffer;

  buffer.getWriteRef().fill(4);
  buffer.flipWriter();
  buffer.getWriteRef().fill(5);
  buffer.flipWriter();
  buffer.getWriteRef().fill(6);
  buffer.flipWriter();

  buffer.newSnap();

  buffer.getWriteRef().fill(7);
  buffer.flipWriter();
  buffer.getWriteRef().fill(8);
  buffer.flipWriter();

  ASSERT_EQ(6, buffer.getReadRef()[0]);
  ASSERT_EQ(6, buffer.getReadRef()[1]);
  ASSERT_EQ(6, buffer.getReadRef()[2]);
}

TEST(tripplebuffer, array_test3) {
  TripleBuffer<std::array<int, 3>> buffer;

  buffer.getWriteRef().fill(7);
  buffer.flipWriter();

  buffer.getWriteRef().fill(8);
  buffer.flipWriter();

  buffer.newSnap();
  ASSERT_EQ(8 , buffer.getReadRef()[0]);
  ASSERT_EQ(8 , buffer.getReadRef()[1]);
  ASSERT_EQ(8 , buffer.getReadRef()[2]);

  buffer.newSnap();
  ASSERT_EQ(8 , buffer.getReadRef()[0]);
  ASSERT_EQ(8 , buffer.getReadRef()[1]);
  ASSERT_EQ(8 , buffer.getReadRef()[2]);
}


// forward declaration of a helper function
void ExecuteAtIntervalForDuration(std::chrono::microseconds at_interval,
                                  std::chrono::seconds for_duration,
                                  std::function<void()> method,
                                  std::string name);

TEST(tripplebuffer, array_thread_test1) {

  struct Ramp {
    int current;
    Ramp(int start_val) {current = start_val;}
    int operator()() {return current++;}
  };

  std::array<int, 1024> buffer_init;
  std::generate(buffer_init.begin(), buffer_init.end(), Ramp(0));

  TripleBuffer<decltype(buffer_init)> buffer(buffer_init);

  // audio buffer refresh interval with 64 sample\buffer at 96kHz sampling rate
  // 1000 / 96000 * 64 = 0.6666 milliseconds
  std::chrono::microseconds	write_interval(666);
  // screen refresh interval at 60kHz
  // 1000 / 60 = 16.666 milliseconds
  std::chrono::microseconds	read_interval(16666);
  std::chrono::seconds	duration(1);

  int writeIndex = 0;
  std::function<void()> writerFunction = [&] {
    auto& dirtyBuffer = buffer.getWriteRef();
    std::generate(dirtyBuffer.begin(), dirtyBuffer.end(), Ramp(writeIndex));
    writeIndex += dirtyBuffer.size();
    buffer.flipWriter();
  };

  // storage for values read from buffer
  std::vector<int> readValues;
  std::function<void()> readFunction = [&] {
    // swap read buffers
    buffer.newSnap();
    auto& readBuffer = buffer.getReadRef();
    
    bool isFirst = true;
    for (const auto &value : readBuffer) {
      // store value
      readValues.push_back(value);
      // ignore first
      if (isFirst) {
        isFirst = false;
      // otherwise check consecutive number incerement by 1
      } else {
        ASSERT_EQ(readValues.rbegin()[1] + 1, readValues.rbegin()[0]);
      }
    }
  };

  // spawn reader and writer threads
  std::thread reader (ExecuteAtIntervalForDuration,
                      read_interval,
                      duration,
                      readFunction,
                      "reader"
                      );

  std::thread writer (ExecuteAtIntervalForDuration,
                      write_interval,
                      duration,
                      writerFunction,
                      "writer"
                      );

  // wait for threads to finish
  writer.join();
  reader.join();

  // some additional info about what happend during the test
  std::cout << "elements copied: " << readValues.size() << "\n";

  unsigned int num_tears = 0;

  if (readValues.size() >= 1) {
    for (auto it = std::next(readValues.begin()); it != readValues.end(); ++it) {
      if (it[-1] + 1 != it[0]) {
        num_tears++;
      }
    }
  }

  std::cout << "last value: " << readValues.back() << std::endl;
  std::cout << "number of data tears: " << num_tears << std::endl;
}


// helper function for executing a function multiple times
// at specified interval for specified amount of duration

// global mutex to used to prevent from character intevleaving
// when calling std::cout from different threads
std::mutex cout_mutex;
void ExecuteAtIntervalForDuration(std::chrono::microseconds at_interval,
                                  std::chrono::seconds for_duration,
                                  std::function<void()> method,
                                  std::string name) {
  cout_mutex.lock();
  std::cout << name << " starting with interval: "
  << std::chrono::duration_cast<std::chrono::milliseconds>(at_interval).count()
  << " milliseconds\n" << " for duration of: "
  << std::chrono::duration_cast<std::chrono::seconds>(for_duration).count()
  << " seconds" << std::endl;
  cout_mutex.unlock();

  long long number_of_executions = 0;
  auto start_time = std::chrono::high_resolution_clock::now();

  auto now = std::chrono::high_resolution_clock::now();
  auto until = now + for_duration;
  auto next_run_time_point = now + at_interval;

  auto zero_duration = std::chrono::microseconds::zero();


  while(std::chrono::high_resolution_clock::now() < until) {
    method();
    number_of_executions++;

    auto wait_interval =
    next_run_time_point - std::chrono::high_resolution_clock::now();
    next_run_time_point += at_interval;


    if (wait_interval > zero_duration) {
      std::this_thread::sleep_for(wait_interval);
    }
  }

  now = std::chrono::high_resolution_clock::now();

  cout_mutex.lock();
  std::cout << name << " stopping after: "
  << std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count()
  << " milliseconds\n" << "method executed: " << number_of_executions << " times." << std::endl;
  cout_mutex.unlock();
}
