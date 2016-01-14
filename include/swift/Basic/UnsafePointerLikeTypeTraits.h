//===--- UnsafePointerLikeTypeTraits.h ------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2016 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://swift.org/LICENSE.txt for license information
// See http://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#ifndef SWIFT_BASIC_UNSAFEPOINTERLIKETYPETRAITS_H
#define SWIFT_BASIC_UNSAFEPOINTERLIKETYPETRAITS_H

#include "llvm/ADT/PointerUnion.h"
#include "llvm/Support/PointerLikeTypeTraits.h"
#include <type_traits>

namespace swift {

template <typename T> struct UnsafePointerLikeTypeTraits {
  using Traits = llvm::PointerLikeTypeTraits<T>;
  static inline void *getAsVoidPointer(T *P) {
    return Traits::getAsVoidPointer(P);
  }
  static inline T *getFromVoidPointer(void *P) {
    return Traits::getFromVoidPointer(P);
  }

  /// Note, we assume here that void* is related to raw malloc'ed memory and
  /// that malloc returns objects at least 4-byte aligned. However, this may be
  /// wrong, or pointers may be from something other than malloc. In this case,
  /// you should specify a real typed pointer or avoid this template.
  ///
  /// All clients should use assertions to do a run-time check to ensure that
  /// this is actually true.
  enum { NumLowBitsAvailable = Traits::NumLowBitsAvailable };
};

template <typename T> struct UnsafePointerLikeTypeTraits<T *> {
  static inline void *getAsVoidPointer(T *P) { return P; }
  static inline T *getFromVoidPointer(void *P) { return static_cast<T *>(P); }

  /// Note, we assume here that void* is related to raw malloc'ed memory and
  /// that malloc returns objects at least 4-byte aligned. However, this may be
  /// wrong, or pointers may be from something other than malloc. In this case,
  /// you should specify a real typed pointer or avoid this template.
  ///
  /// All clients should use assertions to do a run-time check to ensure that
  /// this is actually true.
  enum { NumLowBitsAvailable = 2 };
};

template <typename T> struct UnsafePointerLikeTypeTraits<const T *> {
  static inline void *getAsVoidPointer(const T *P) {
    return const_cast<T *>(P);
  }
  static inline const T *getFromVoidPointer(void *P) {
    return static_cast<const T *>(P);
  }

  /// Note, we assume here that void* is related to raw malloc'ed memory and
  /// that malloc returns objects at least 4-byte aligned. However, this may be
  /// wrong, or pointers may be from something other than malloc. In this case,
  /// you should specify a real typed pointer or avoid this template.
  ///
  /// All clients should use assertions to do a run-time check to ensure that
  /// this is actually true.
  enum { NumLowBitsAvailable = 2 };
};

/// This is a box that causes the pointer in side of it to be assumed by data
/// structures to be pointing to memory that is at least 2 byte aligned.
///
/// All clients should use assertion to do a run-time check to ensure that this
/// is actually true.
///
/// The reason to have this and UnsafePointerLikeTypeTraits is that
template <typename T> struct UnsafeAssumedAligned2 {
  T Value;

  UnsafeAssumedAligned2() = default;
  UnsafeAssumedAligned2(T V) : Value(V) {}
  operator T() const { return Value; }
  T &operator->() { return Value; }
};

template <typename T> inline T assumeAligned2(T t) { return t; }

template <typename T> inline UnsafeAssumedAligned2<T *> assumeAligned2(T *t) {
  return UnsafeAssumedAligned2<T *>(t);
}

} // end swift namespace

namespace llvm {

template <typename T>
struct PointerLikeTypeTraits<swift::UnsafeAssumedAligned2<T *>> {
  using BoxTy = swift::UnsafeAssumedAligned2<T *>;
  static_assert(std::is_trivially_copyable<BoxTy>::value,
                "Expected a trivially copyable type!");

  static inline void *getAsVoidPointer(BoxTy P) {
    return const_cast<void *>(static_cast<const void *>(P));
  }
  static inline BoxTy getFromVoidPointer(void *P) {
    return {static_cast<T *>(P)};
  }

  /// Note, we assume here that void* is related to raw malloc'ed memory and
  /// that malloc returns objects at least 4-byte aligned. However, this may be
  /// wrong, or pointers may be from something other than malloc. In this case,
  /// you should specify a real typed pointer or avoid this template.
  ///
  /// All clients should use assertions to do a run-time check to ensure that
  /// this is actually true.
  enum { NumLowBitsAvailable = 2 };
};

} // end llvm namespace

namespace swift {

// Base case, not a pointer.
template <typename T> class UnsafePointerUnionBox {
  typename std::enable_if<!std::is_pointer<T>::value, T>::type Value;
  using PtrTraits = llvm::PointerLikeTypeTraits<T>;

public:
  UnsafePointerUnionBox() {}
  UnsafePointerUnionBox(T t) : Value(t) {}
  operator T() const { return Value; }
  operator bool() const { return bool(Value); }
};

// Base case a pointer.
template <typename T> class UnsafePointerUnionBox<T *> {
  UnsafeAssumedAligned2<T *> Value;

public:
  UnsafePointerUnionBox() {}
  UnsafePointerUnionBox(T *t) : Value(t) {}
  UnsafePointerUnionBox(UnsafeAssumedAligned2<T *> t) : Value(t) {}
  operator T *() const { return Value; }
  operator UnsafeAssumedAligned2<T *>() const { return Value; }
  operator bool() const { return bool(Value); }
};

} // end swift namespace

namespace llvm {

template <typename T>
struct PointerLikeTypeTraits<swift::UnsafePointerUnionBox<T>> {
  using sfinae = typename std::enable_if<!std::is_pointer<T>::value>::type;
  using PtrTraits = PointerLikeTypeTraits<T>;

  using BoxTy = swift::UnsafePointerUnionBox<T>;
  static_assert(std::is_trivially_copyable<BoxTy>::value,
                "Expected a trivially copyable type!");

  static inline void *getAsVoidPointer(BoxTy P) {
    return PtrTraits::getAsVoidPointer(P);
  }
  static inline BoxTy getFromVoidPointer(void *P) {
    return {PtrTraits::getFromVoidPointer(P)};
  }

  enum { NumLowBitsAvailable = PtrTraits::NumLowBitsAvailable };
};

template <typename T>
struct PointerLikeTypeTraits<swift::UnsafePointerUnionBox<T *>> {
  using PtrTraits = PointerLikeTypeTraits<swift::UnsafeAssumedAligned2<T *>>;

  using BoxTy = swift::UnsafePointerUnionBox<T *>;
  static_assert(std::is_trivially_copyable<BoxTy>::value,
                "Expected a trivially copyable type!");

  static inline void *getAsVoidPointer(BoxTy P) {
    return PtrTraits::getAsVoidPointer(P);
  }
  static inline BoxTy getFromVoidPointer(void *P) {
    return {PtrTraits::getFromVoidPointer(P)};
  }

  /// Note, we assume here that void* is related to raw malloc'ed memory and
  /// that malloc returns objects at least 4-byte aligned. However, this may be
  /// wrong, or pointers may be from something other than malloc. In this case,
  /// you should specify a real typed pointer or avoid this template.
  ///
  /// All clients should use assertions to do a run-time check to ensure that
  /// this is actually true.
  enum { NumLowBitsAvailable = PtrTraits::NumLowBitsAvailable };
};

} // end llvm namespace

namespace swift {

template <typename PT1, typename PT2> struct UnsafePointerUnion {

  using PTBox1 = UnsafePointerUnionBox<PT1>;
  using PTBox2 = UnsafePointerUnionBox<PT2>;

  using InnerUnionTy = llvm::PointerUnion<PTBox1, PTBox2>;
  InnerUnionTy Ptr;

  using ValTy = typename InnerUnionTy::ValTy;

  UnsafePointerUnion() {}
  UnsafePointerUnion(InnerUnionTy P) : Ptr(P) {}
  UnsafePointerUnion(PT1 V) : Ptr(assumeAligned2(V)) {}
  UnsafePointerUnion(PT2 V) : Ptr(assumeAligned2(V)) {}

  bool isNull() const { return Ptr.isNull(); }
  explicit operator bool() const { return !Ptr.isNull(); }

#if 0
  template <typename T>
  int is(typename std::enable_if<std::is_pointer<T>::value>::type *_=0) const {
    return Ptr.template is<UnsafePointerUnionBox<T>>();
  }

  template <typename T>
  int is(typename std::enable_if<!std::is_pointer<T>::value>::type *_=0) const {
    return Ptr.template is<T>();
  }

  template <typename T>
  T get(typename std::enable_if<std::is_pointer<T>::value>::type *_=0) const {
    return Ptr.template get<UnsafePointerUnionBox<T>>();
  }

  template <typename T>
  T get(typename std::enable_if<!std::is_pointer<T>::value>::type *_=0) const {
    return Ptr.template get<T>();
  }

  template <typename T>
  T dyn_cast(typename std::enable_if<std::is_pointer<T>::value>::type *_=0) const {
    return Ptr.template dyn_cast<UnsafePointerUnionBox<T>>();
  }

  template <typename T>
  T dyn_cast(typename std::enable_if<!std::is_pointer<T>::value>::type *_=0) const {
    return Ptr.template dyn_cast<T>();
  }
#else
  template <typename T> int is() const {
    return Ptr.template is<UnsafePointerUnionBox<T>>();
  }

  template <typename T> T get() const {
    return Ptr.template get<UnsafePointerUnionBox<T>>();
  }

  template <typename T> T dyn_cast() const {
    return Ptr.template dyn_cast<UnsafePointerUnionBox<T>>();
  }
#endif

  /// Assignment from nullptr which just clears the union.
  const UnsafePointerUnion &operator=(std::nullptr_t) {
    Ptr = nullptr;
    return *this;
  }

  /// Assignment operators - Allow assigning into this union from either
  /// pointer type, setting the discriminator to remember what it came from.
  const UnsafePointerUnion &operator=(const PT1 &RHS) {
    Ptr = assumeAligned2(RHS);
    return *this;
  }
  const UnsafePointerUnion &operator=(const PT2 &RHS) {
    Ptr = assumeAligned2(RHS);
    return *this;
  }

  void *getOpaqueValue() const { return Ptr.getOpaqueValue(); }
  static inline UnsafePointerUnion getFromOpaqueValue(void *VP) {
    return UnsafePointerUnion(InnerUnionTy::getFromOpaqueValue(VP));
  }
};

template <typename PT1, typename PT2>
static bool operator==(UnsafePointerUnion<PT1, PT2> lhs,
                       UnsafePointerUnion<PT1, PT2> rhs) {
  return lhs.getOpaqueValue() == rhs.getOpaqueValue();
}

template <typename PT1, typename PT2>
static bool operator!=(UnsafePointerUnion<PT1, PT2> lhs,
                       UnsafePointerUnion<PT1, PT2> rhs) {
  return lhs.getOpaqueValue() != rhs.getOpaqueValue();
}

template <typename PT1, typename PT2>
static bool operator<(UnsafePointerUnion<PT1, PT2> lhs,
                      UnsafePointerUnion<PT1, PT2> rhs) {
  return lhs.getOpaqueValue() < rhs.getOpaqueValue();
}

} // end swift namespace

namespace llvm {

/// Provide PointerLikeTypeTraits for void* that is used by PointerUnion
/// for the two template arguments.
template <typename PT1, typename PT2>
class PointerUnionUIntTraits<swift::UnsafePointerUnionBox<PT1>,
                             swift::UnsafePointerUnionBox<PT2>> {
  using UnsafePT1Box = swift::UnsafePointerUnionBox<PT1>;
  using UnsafePT2Box = swift::UnsafePointerUnionBox<PT2>;

public:
  static inline void *getAsVoidPointer(void *P) { return P; }
  static inline void *getFromVoidPointer(void *P) { return P; }
  enum {
    PT1BitsAv = (int)(PointerLikeTypeTraits<UnsafePT1Box>::NumLowBitsAvailable),
    PT2BitsAv = (int)(PointerLikeTypeTraits<UnsafePT2Box>::NumLowBitsAvailable),
    NumLowBitsAvailable = PT1BitsAv < PT2BitsAv ? PT1BitsAv : PT2BitsAv
  };
};

// Teach SmallPtrSet that PointerUnion is "basically a pointer", that has
// # low bits available = min(PT1bits,PT2bits)-1.
template <typename PT1, typename PT2>
class PointerLikeTypeTraits<swift::UnsafePointerUnion<PT1, PT2>> {
public:
  static inline void *
  getAsVoidPointer(const swift::UnsafePointerUnion<PT1, PT2> &P) {
    return P.getOpaqueValue();
  }
  static inline swift::UnsafePointerUnion<PT1, PT2>
  getFromVoidPointer(void *P) {
    return swift::UnsafePointerUnion<PT1, PT2>::getFromOpaqueValue(P);
  }

  // The number of bits available are the min of the two pointer types.
  enum {
    NumLowBitsAvailable =
        PointerLikeTypeTraits<typename swift::UnsafePointerUnion<
            PT1, PT2>::ValTy>::NumLowBitsAvailable
  };
};

} // end llvm namespace

namespace swift {

template <typename PT1, typename PT2, typename PT3> struct UnsafePointerUnion3 {

  using InnerUnionTy = llvm::PointerUnion3<UnsafeAssumedAligned2<PT1>,
                                           UnsafeAssumedAligned2<PT2>,
                                           UnsafeAssumedAligned2<PT3>>;
  InnerUnionTy Ptr;

  UnsafePointerUnion3() {}
  UnsafePointerUnion3(InnerUnionTy P) : Ptr(P) {}
  UnsafePointerUnion3(PT1 V) : Ptr(assumeAligned2(V)) {}
  UnsafePointerUnion3(PT2 V) : Ptr(assumeAligned2(V)) {}
  UnsafePointerUnion3(PT3 V) : Ptr(assumeAligned2(V)) {}

  bool isNull() const { return Ptr.isNull(); }
  explicit operator bool() const { return !Ptr.isNull(); }

  template <typename T> int is() const {
    return Ptr.template is<UnsafeAssumedAligned2<T>>();
  }

  template <typename T> T get() const {
    return Ptr.template get<UnsafeAssumedAligned2<T>>();
  }

  template <typename T> T dyn_cast() const {
    return Ptr.template dyn_cast<UnsafeAssumedAligned2<T>>();
  }

  /// Assignment from nullptr which just clears the union.
  const UnsafePointerUnion3 &operator=(std::nullptr_t) {
    Ptr = nullptr;
    return *this;
  }

  /// Assignment operators - Allow assigning into this union from either
  /// pointer type, setting the discriminator to remember what it came from.
  const UnsafePointerUnion3 &operator=(const PT1 &RHS) {
    Ptr = assumeAligned2(RHS);
    return *this;
  }
  const UnsafePointerUnion3 &operator=(const PT2 &RHS) {
    Ptr = assumeAligned2(RHS);
    return *this;
  }
  const UnsafePointerUnion3 &operator=(const PT3 &RHS) {
    Ptr = assumeAligned2(RHS);
    return *this;
  }

  void *getOpaqueValue() const { return Ptr.getOpaqueValue(); }
  static inline UnsafePointerUnion3 getFromOpaqueValue(void *VP) {
    return UnsafePointerUnion3(InnerUnionTy::getFromOpaqueValue(VP));
  }
};

} // end swift namespace

#endif
