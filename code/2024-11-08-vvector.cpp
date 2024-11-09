#define VVECTOR_DEBUG 1

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#if VVECTOR_DEBUG
#include <cstdio>
#include <ostream>
#endif

namespace vvhelper {
#ifdef __cpp_lib_trivially_relocatable
  using std::is_trivially_relocatable_v;
#else
  template<class T>
  inline constexpr bool is_trivially_relocatable_v = std::is_trivially_copyable_v<T>;
#endif
} // namespace vvhelper


// Warning: This API is difficult to use unless all the types in Ts... are distinct.
// Warning: If relocating any element ever throws an exception, we call std::terminate.
// Warning: If constructing the element in push_front throws, we call std::terminate.
//
template<class... Ts>
class VVector {
  template<size_t K>
  using TypeAt = std::variant_alternative_t<K, std::variant<Ts...>>;

  static constexpr size_t sizeofImpl[] = {
    sizeof(Ts)...
  };
  static constexpr size_t alignofImpl[] = {
    alignof(Ts)...
  };
  static constexpr void (*copyImpl[])(const void*, void*) = {
    +[](const void *src, void *dest) { ::new (dest) Ts(*(Ts*)src); }...
  };
  static constexpr void (*relocateImpl[])(void*, void*) noexcept = {
    +[](void *src, void *dest) noexcept {
#if VVECTOR_DEBUG
      printf("relocateImpl is relocating %s from %p to %p (%+td bytes)\n", typeid(Ts).name(), src, dest, (char*)dest - (char*)src);
#endif
#ifdef __cpp_lib_trivially_relocatable
      std::relocate_at((Ts*)src, (Ts*)dest);
#else
      ::new (dest) Ts(std::move(*(Ts*)src)); (*(Ts*)src).~Ts();
#endif
    } ...
  };
  static constexpr void (*relocateOverlappingImpl[])(void*, void*) noexcept = {
    +[](void *src, void *dest) noexcept {
#if VVECTOR_DEBUG
      printf("relocateOverlappingImpl is relocating %s from %p to %p (%+td bytes)\n", typeid(Ts).name(), src, dest, (char*)dest - (char*)src);
#endif
#ifdef __cpp_lib_trivially_relocatable
      ::new (dest) Ts(std::move(std::relocate((Ts*)src)));
#else
      Ts tmp = std::move(*(Ts*)src); (*(Ts*)src).~Ts(); ::new (dest) Ts(std::move(tmp));
#endif
    }...
  };
  static constexpr void (*destroyImpl[])(void*) noexcept = {
    +[](void *p) noexcept { (*(Ts*)p).~Ts(); }...
  };

  static constexpr size_t max_alignment = std::max({ alignof(Ts)... });

public:
  explicit VVector() = default;
  VVector(VVector&& rhs) noexcept :
    meta_(std::move(rhs.meta_)),
    data_(std::move(rhs.data_)),
    capacity_(std::exchange(rhs.capacity_, 0)) {}

  VVector(const VVector& rhs) : meta_(rhs.meta_), capacity_(rhs.capacity_) {
    if (meta_.empty()) {
      data_ = nullptr;
    } else {
      data_ = AlignedDeleter::make(capacity_);
      if constexpr ((std::is_trivially_copy_constructible_v<Ts> && ...)) {
        std::memcpy(data_.get(), rhs.data_.get(), capacity_);
      } else {
        for (const auto& elt : meta_) {
          copyImpl[elt.type_](rhs.data_.get() + elt.offset_, data_.get() + elt.offset_);
        }
      }
    }
  }

  VVector& operator=(const VVector& rhs) {
    VVector copy(rhs);
    copy.swap(*this);
    return *this;
  }

  VVector& operator=(VVector&& rhs) noexcept {
    VVector copy(std::move(rhs));
    copy.swap(*this);
    return *this;
  }

  void swap(VVector& rhs) noexcept {
    meta_.swap(rhs.meta_);
    data_.swap(rhs.data_);
    std::swap(capacity_, rhs.capacity_);
  }

  friend void swap(VVector& a, VVector& b) noexcept { a.swap(b); }

  ~VVector() {
    if constexpr ((std::is_trivially_destructible_v<Ts> && ...)) {
      // do nothing
    } else {
      for (const auto& elt : meta_) {
        destroyImpl[elt.type_](data_.get() + elt.offset_);
      }
    }
  }

  size_t size() const { return meta_.size(); }
  [[nodiscard]] bool empty() const { return meta_.empty(); }

  void resize(size_t n, const std::variant<Ts...>& value) {
    while (size() > n) {
      pop_back();
    }
    while (size() < n) {
      push_back(value);
    }
  }

  template<size_t K>
  const TypeAt<K> *get_if(size_t i) const {
    if (meta_[i].type_ == K) {
      return reinterpret_cast<const TypeAt<K>*>(data_.get() + meta_[i].offset_);
    } else {
      return nullptr;
    }
  }

  template<size_t K>
  const TypeAt<K>& at(size_t i) const {
    if (i >= size()) {
      throw std::out_of_range("VVector::at");
    } else if (const auto *p = get_if<K>(i)) {
      return *p;
    } else {
      throw std::bad_variant_access();
    }
  }

  void push_front(std::variant<Ts...> vvalue) {
    meta_.reserve(meta_.size() + 1);
    int k = vvalue.index();
    // Each element needs to slide right by `sizeofImpl[k]` bytes
    // to make room, but might also need to be bumped for alignment.
    std::vector<int> newOffsets;
    int offset = sizeofImpl[k];
    for (const auto& elt : meta_) {
      offset = alignUp(offset, alignofImpl[elt.type_]);
      newOffsets.push_back(offset);
      offset += sizeofImpl[elt.type_];
    }
    reserveImpl(offset);
    for (int i = meta_.size() - 1; i >= 0; --i) {
      if (newOffsets[i] > meta_[i].offset_) {
        relocateOverlappingImpl[meta_[i].type_](data_.get() + meta_[i].offset_, data_.get() + newOffsets[i]);
        meta_[i].offset_ = newOffsets[i];
      }
    }
    [&]() noexcept {
      // If this construction throws, we're in trouble.
      // Punt for now by wrapping it in a noexcept block,
      // so that if it throws we'll `std::terminate`.
      std::visit([&](const auto& value) {
        copyImpl[k](&value, data_.get() + 0);
      }, vvalue);
    }();
    meta_.insert(meta_.begin(), { k, 0 });
  }

  void pop_front() {
    destroyImpl[meta_.front().type_](data_.get() + 0);
    meta_.erase(meta_.begin());
    int newOffset = 0;
    for (auto& elt : meta_) {
      newOffset = alignUp(newOffset, alignofImpl[elt.type_]);
      if (newOffset == elt.offset_) {
        break;
      }
      relocateOverlappingImpl[elt.type_](data_.get() + elt.offset_, data_.get() + newOffset);
      elt.offset_ = newOffset;
      newOffset += sizeofImpl[elt.type_];
    }
  }

  void push_back(std::variant<Ts...> vvalue) {
    meta_.reserve(meta_.size() + 1);
    int k = vvalue.index();
    int beginbyte = meta_.empty() ? 0 : meta_.back().endbyte();
    beginbyte = alignUp(beginbyte, alignofImpl[k]);
    if (beginbyte + sizeofImpl[k] > capacity_) {
      reserveImpl(beginbyte + sizeofImpl[k]);
    }
    std::visit([&](const auto& value) {
      copyImpl[k](&value, data_.get() + beginbyte);
    }, vvalue);
    meta_.push_back({ k, beginbyte });
  }

  void pop_back() {
    const auto& elt = meta_.back();
    destroyImpl[elt.type_](data_.get() + elt.offset_);
    meta_.pop_back();
  }

private:
  static size_t alignUp(size_t n, size_t align) {
    return (n + align - 1) / align * align;
  }

  void reserveImpl(size_t newcapacity) {
    if (newcapacity <= capacity_) {
      return;
    }
    newcapacity = alignUp(newcapacity, max_alignment);
    newcapacity = std::max(newcapacity, 2 * capacity_);
    auto newdata = AlignedDeleter::make(newcapacity);

    if ((vvhelper::is_trivially_relocatable_v<Ts> && ...)) {
      std::memcpy(newdata.get(), data_.get(), capacity_);
    } else {
      for (const auto& elt : meta_) {
        relocateImpl[elt.type_](data_.get() + elt.offset_, newdata.get() + elt.offset_);
      }
    }
    data_ = std::move(newdata);
    capacity_ = newcapacity;
  }

#if VVECTOR_DEBUG
  friend std::ostream& operator<<(std::ostream& os, const VVector& vv) {
    os << "META TYPES:";
    for (auto&& elt : vv.meta_) os << " " << elt.type_;
    os << "\nMETA OFFSETS:";
    for (auto&& elt : vv.meta_) os << " " << elt.offset_;
    os << "\nCAPACITY: " << vv.capacity_;
    os << "\nPOINTER: " << (void*)vv.data_.get() << "\n";
    return os;
  }
#endif

  struct Metadata {
    int type_;
    int offset_;
    size_t endbyte() const { return offset_ + sizeofImpl[type_]; }
  };
  struct AlignedDeleter {
    static auto make(size_t n) {
      void *p = std::aligned_alloc(max_alignment, alignUp(n, max_alignment));
      return std::unique_ptr<char[], AlignedDeleter>(static_cast<char*>(p));
    }
    void operator()(char *p) const { std::free(p); }
  };
  std::vector<Metadata> meta_;
  std::unique_ptr<char[], AlignedDeleter> data_ = nullptr;
  size_t capacity_ = 0;
};


// =======================================================================

#include <string>
#include <iostream>
#include <list>

struct String {
  String(const char *s) : data_(s) {}
  friend std::ostream& operator<<(std::ostream& os, const String& s) { return os << s.data_; }
  std::string data_;
  std::list<int> dummy_;
};

int main() {
  VVector<int, String> vv;

  vv.push_back(1);
  std::cout
    << vv
    << vv.at<0>(0)
    << std::endl;

  vv.push_back("hello world");
  std::cout
    << vv
    << vv.at<0>(0)
    << vv.at<1>(1)
    << std::endl;

  vv.resize(3, 2);

  std::cout
    << vv
    << vv.at<0>(0)
    << vv.at<1>(1)
    << vv.at<0>(2)
    << std::endl;

  vv.push_front("the curious incident of the dog in the night-time");

  std::cout
    << vv
    << vv.at<1>(0)
    << vv.at<0>(1)
    << vv.at<1>(2)
    << vv.at<0>(3)
    << std::endl;

  vv.push_front(3);

  std::cout
    << vv
    << vv.at<0>(0)
    << vv.at<1>(1)
    << vv.at<0>(2)
    << vv.at<1>(3)
    << vv.at<0>(4)
    << std::endl;
}
