// Modified by Adrian Gierakowski, 2014
//
// added (const T&) constructor to enable custom buffer initialisation
// added getReadRef and getWriteRef to enable by reference access to
// objects contained in the buffer
//
// minore code cleanup
//
//
// ORIGINAL COPYRIGHT NOTICE
//==============================================================================
// Name        : TripleBuffer.hxx
// Author      : André Pacheco Neves
// Version     : 1.0 (27/01/13)
// Copyright   : Copyright (c) 2013, André Pacheco Neves
//               All rights reserved.

/*

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
- Neither the name of the <organization> nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

// Description :
//    Template class for a TripleBuffer as a concurrency mechanism,
//    using atomic operations
// Credits :
//    http://remis-thoughts.blogspot.pt/2012/01/triple-buffering-as-concurrency_30.html
//==============================================================================

#ifndef TRIPLEBUFFER_H_
#define TRIPLEBUFFER_H_

#include <atomic>

template <typename T>
class TripleBuffer {
 public:
  TripleBuffer<T>();
  TripleBuffer<T>(const T&);

  // get the current snap to read
  T snap() const;
  // write a new value
  void write(const T newT);
  // get reference to current dirty buffer
  T& getWriteRef();
  // get reference to current read buffer
  const T& getReadRef() const;
  // swap to the latest value, if any
  bool newSnap();
  // flip writer positions dirty / clean
  void flipWriter();
  // wrapper to read the last available element (newSnap + snap)
  T readLast();
  // wrapper to update with a new element (write + flipWriter)
  void update(T newT);

 private:
  // non-copyable behavior
  TripleBuffer<T>(const TripleBuffer<T>&) = delete;
  TripleBuffer<T>& operator=(const TripleBuffer<T>&) = delete;

  // check if the newWrite bit is 1
  bool isNewWrite(uint_fast8_t flags);
  // swap Snap and Clean indexes
  uint_fast8_t swapSnapWithClean(uint_fast8_t flags);
  // set newWrite to 1 and swap Clean and Dirty indexes
  uint_fast8_t newWriteSwapCleanWithDirty(uint_fast8_t flags);

  // 8 bit flags are (unused) (new write) (2x dirty) (2x clean) (2x snap)
  // newWrite   = (flags & 0x40)
  // dirtyIndex = (flags & 0x30) >> 4
  // cleanIndex = (flags & 0xC) >> 2
  // snapIndex  = (flags & 0x3)
  mutable std::atomic_uint_fast8_t flags;

  T buffer[3];
};

// include implementation in header since it is a template

template <typename T>
TripleBuffer<T>::TripleBuffer() {
  T dummy = T();

  buffer[0] = dummy;
  buffer[1] = dummy;
  buffer[2] = dummy;
  // initially dirty = 0, clean = 1 and snap = 2
  flags.store(0x6, std::memory_order_relaxed);
}

template <typename T>
TripleBuffer<T>::TripleBuffer(const T& init) {
  buffer[0] = init;
  buffer[1] = init;
  buffer[2] = init;
  // initially dirty = 0, clean = 1 and snap = 2
  flags.store(0x6, std::memory_order_relaxed);
}

template <typename T>
T TripleBuffer<T>::snap() const {
  // read snap index
  return buffer[flags.load(std::memory_order_consume) & 0x3];
}

template <typename T>
void TripleBuffer<T>::write(const T newT) {
  // write into dirty index
  buffer[(flags.load(std::memory_order_consume) & 0x30) >> 4] = newT;
}

template <typename T>
T& TripleBuffer<T>::getWriteRef() {
  return buffer[(flags.load(std::memory_order_consume) & 0x30) >> 4];
}

template <typename T>
const T& TripleBuffer<T>::getReadRef() const {
  return buffer[flags.load(std::memory_order_consume) & 0x3];
}

template <typename T>
bool TripleBuffer<T>::newSnap() {
  uint_fast8_t flagsNow(flags.load(std::memory_order_consume));

  if (!isNewWrite(flagsNow)) {
    return false;
  }

  while (!flags.compare_exchange_weak(flagsNow,
          swapSnapWithClean(flagsNow),
          std::memory_order_release,
          std::memory_order_consume));

  return true;
}

template <typename T>
void TripleBuffer<T>::flipWriter() {
  uint_fast8_t flagsNow(flags.load(std::memory_order_consume));
  while (!flags.compare_exchange_weak(flagsNow,
        newWriteSwapCleanWithDirty(flagsNow),
        std::memory_order_release,
        std::memory_order_consume));
}

template <typename T>
T TripleBuffer<T>::readLast() {
  // get most recent value
  newSnap();
  // return it
  return snap();
}

template <typename T>
void TripleBuffer<T>::update(T newT) {
  // write new value
  write(newT);
  // change dirty/clean buffer positions for the next update
  flipWriter();
}

template <typename T>
bool TripleBuffer<T>::isNewWrite(uint_fast8_t flags) {
  // check if the newWrite bit is 1
  return ((flags & 0x40) != 0);
}

template <typename T>
uint_fast8_t TripleBuffer<T>::swapSnapWithClean(uint_fast8_t flags) {
  // swap snap with clean
  uint_fast8_t flag_mask_1 = 0x30;
  uint_fast8_t flag_mask_2 = 0x3;
  uint_fast8_t flag_mask_3 = 0xC;
  uint_fast8_t bit_shift = 2;
  return (flags & flag_mask_1)
    | ((flags & flag_mask_2) << bit_shift)
    | ((flags & flag_mask_3) >> bit_shift);
}

template <typename T>
uint_fast8_t TripleBuffer<T>::newWriteSwapCleanWithDirty(uint_fast8_t flags) {
  // set newWrite bit to 1 and swap clean with dirty
  return 0x40
    | ((flags & 0xC) << 2)
    | ((flags & 0x30) >> 2)
    | (flags & 0x3);
}

#endif /* TRIPLEBUFFER_H_ */
