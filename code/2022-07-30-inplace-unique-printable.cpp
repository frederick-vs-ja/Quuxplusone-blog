#include <cstring>
#include <memory>
#include <ostream>
#include <type_traits>
#include <utility>

class InplaceUniquePrintable {
    alignas(std::string) unsigned char data_[sizeof(std::string)];
    void (*print_)(std::ostream&, const void *) = nullptr;
    void (*relocate_)(void *, const void *) noexcept = nullptr;
    void (*destroy_)(void *) noexcept = nullptr;
public:
    template<class T, class StoredT = std::decay_t<T>>
    InplaceUniquePrintable(T&& t)
        // constraints are left as an exercise for the reader
    {
        constexpr bool fits_in_buffer =
            sizeof(StoredT) <= sizeof(std::string) &&
            alignof(StoredT) <= alignof(std::string) &&
            std::is_nothrow_move_constructible_v<StoredT>;
        if constexpr (fits_in_buffer) {
            ::new ((void*)data_) StoredT(std::forward<T>(t));
            print_ = [](std::ostream& os, const void *data) {
                os << *(const StoredT*)data;
            };
            if (!std::is_trivially_relocatable_v<StoredT>) {
                relocate_ = [](void *dst, const void *src) noexcept {
                    ::new (dst) StoredT(std::move(*(StoredT*)src));
                    ((StoredT*)src)->~StoredT();
                };
            }
            if (!std::is_trivially_destructible_v<StoredT>) {
                destroy_ = [](void *src) noexcept {
                    ((StoredT*)src)->~StoredT();
                };
            }
        } else {
            using StoredPtr = StoredT*;
            auto p = std::make_unique<StoredT>(std::forward<T>(t));
            ::new ((void*)data_) StoredPtr(p.release());
            print_ = [](std::ostream& os, const void *data) {
                const StoredT *p = *(StoredPtr*)data;
                os << *p;
            };
            destroy_ = [](void *src) noexcept {
                StoredT *p = *(StoredPtr*)src;
                delete p;
            };
        }
    }

    InplaceUniquePrintable(InplaceUniquePrintable&& rhs) noexcept {
        print_ = std::exchange(rhs.print_, nullptr);
        relocate_ = std::exchange(rhs.relocate_, nullptr);
        destroy_ = std::exchange(rhs.destroy_, nullptr);
        if (relocate_ != nullptr) {
            relocate_(data_, rhs.data_);
        } else {
            std::memcpy(data_, rhs.data_, sizeof(data_));
        };
    }

    friend void swap(InplaceUniquePrintable& lhs, InplaceUniquePrintable& rhs) noexcept {
        std::swap(lhs.data_, rhs.data_);
        std::swap(lhs.print_, rhs.print_);
        std::swap(lhs.relocate_, rhs.relocate_);
        std::swap(lhs.destroy_, rhs.destroy_);
    }

    InplaceUniquePrintable& operator=(InplaceUniquePrintable&& rhs) noexcept {
        auto copy = std::move(rhs);
        swap(*this, copy);
        return *this;
    }

    ~InplaceUniquePrintable() {
        if (destroy_ != nullptr) {
            destroy_(data_);
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const InplaceUniquePrintable& self) {
        self.print_(os, self.data_);
        return os;
    }
};

#include <iostream>

void printit(InplaceUniquePrintable p) {
    std::cout << "The printable thing was: " << p << "." << std::endl;
}

int main() {
    printit(42);
    printit("hello world");
}
