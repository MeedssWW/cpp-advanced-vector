#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>
#include <algorithm> 

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
    using iterator = T*;
    using const_iterator = const T*;
    
    Vector() noexcept = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size)
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)
    {
        if constexpr (std::is_copy_constructible_v<T>) {
            std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
        } else {
            std::uninitialized_move_n(const_cast<T*>(other.data_.GetAddress()), size_, data_.GetAddress());
        }
    }

    Vector(Vector&& other) noexcept
        : data_(0)
        , size_(0)
    {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }

private:
    void CopyAssignFrom(const Vector& rhs) {
        if (rhs.size_ <= data_.Capacity()) {
            size_t min_size = std::min(size_, rhs.size_);
            std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + min_size, data_.GetAddress());
            if (rhs.size_ > size_) {
                std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, 
                                         rhs.size_ - size_, 
                                         data_.GetAddress() + size_);
            } else if (size_ > rhs.size_) {
                std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
            }
            size_ = rhs.size_;
        } else {
            Vector temp(rhs);
            Swap(temp);
        }
    }

public:
    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            CopyAssignFrom(rhs);
        }
        return *this;
    }

    Vector& operator=(Vector&& rhs) noexcept {
        if (this != &rhs) {
            Swap(rhs);
        }
        return *this;
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        
        RawMemory<T> new_data(new_capacity);
        if constexpr (std::is_nothrow_move_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        } else if constexpr (std::is_copy_constructible_v<T>) {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        } else {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    void Resize(size_t new_size) {
        if (new_size <= size_) {
            std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
            size_ = new_size;
        } else {
            if (new_size > data_.Capacity()) {
                Reserve(new_size);
            }
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
            size_ = new_size;
        }
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        if (size_ == data_.Capacity()) {
            size_t new_capacity = data_.Capacity() == 0 ? 1 : data_.Capacity() * 2;
            RawMemory<T> new_data(new_capacity);
            T* new_elem_ptr = nullptr;
            try {
                if constexpr (std::is_nothrow_move_constructible_v<T>) {
                    std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
                } else if constexpr (std::is_copy_constructible_v<T>) {
                    std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
                } else {
                    std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
                }
                new_elem_ptr = new (new_data.GetAddress() + size_) T(std::forward<Args>(args)...);
            } catch (...) {
                std::destroy_n(new_data.GetAddress(), size_);
                throw;
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
            ++size_;
            return *new_elem_ptr;
        } else {
            T* place = data_.GetAddress() + size_;
            new (place) T(std::forward<Args>(args)...);
            ++size_;
            return *place;
        }
    }

    void PushBack(const T& value) {
        EmplaceBack(value);
    }

    void PushBack(T&& value) {
        EmplaceBack(std::move(value));
    }

    void PopBack() noexcept {
        if (size_ > 0) {
            --size_;
            data_[size_].~T();
        }
    }

    iterator begin() noexcept {
        return data_.GetAddress();
    }

    iterator end() noexcept {
        return data_.GetAddress() + size_;
    }

    const_iterator begin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator end() const noexcept {
        return data_.GetAddress() + size_;
    }

    const_iterator cbegin() const noexcept {
        return begin();
    }

    const_iterator cend() const noexcept {
        return end();
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        assert(pos >= begin() && pos <= end());
        size_t insert_pos = pos - begin();
        
        if (size_ == data_.Capacity()) {
            size_t new_capacity = data_.Capacity() == 0 ? 1 : data_.Capacity() * 2;
            RawMemory<T> new_data(new_capacity);
            try {
                if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                    std::uninitialized_move_n(data_.GetAddress(), insert_pos, new_data.GetAddress());
                } else {
                    std::uninitialized_copy_n(data_.GetAddress(), insert_pos, new_data.GetAddress());
                }
                new (new_data.GetAddress() + insert_pos) T(std::forward<Args>(args)...);
                if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                    std::uninitialized_move_n(data_.GetAddress() + insert_pos, size_ - insert_pos, 
                                             new_data.GetAddress() + insert_pos + 1);
                } else {
                    std::uninitialized_copy_n(data_.GetAddress() + insert_pos, size_ - insert_pos, 
                                             new_data.GetAddress() + insert_pos + 1);
                }
            } catch (...) {
                std::destroy_n(new_data.GetAddress(), insert_pos);
                if (insert_pos < size_) {
                    std::destroy_n(new_data.GetAddress() + insert_pos, 1);
                }
                throw;
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
            ++size_;
            return begin() + insert_pos;
        } else {
            if (insert_pos == size_) {
                new (data_.GetAddress() + size_) T(std::forward<Args>(args)...);
                ++size_;
                return begin() + insert_pos;
            } else {
                T temp_obj(std::forward<Args>(args)...);
                new (data_.GetAddress() + size_) T(std::move(data_[size_ - 1]));
                if constexpr (std::is_nothrow_move_assignable_v<T>) {
                    std::move_backward(data_.GetAddress() + insert_pos, data_.GetAddress() + size_ - 1, data_.GetAddress() + size_);
                    data_[insert_pos] = std::move(temp_obj);
                } else {
                    std::copy_backward(data_.GetAddress() + insert_pos, data_.GetAddress() + size_ - 1, data_.GetAddress() + size_);
                    data_[insert_pos] = temp_obj;
                }
                ++size_;
                return begin() + insert_pos;
            }
        }
    }

    iterator Insert(const_iterator pos, const T& value) {
        const T* value_ptr = &value;
        if (value_ptr >= data_.GetAddress() && value_ptr < data_.GetAddress() + size_) {
            T temp_value = value;
            return Emplace(pos, std::move(temp_value));
        }
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }

    iterator Erase(const_iterator pos) noexcept(std::is_nothrow_move_assignable_v<T>) {
        assert(pos >= begin() && pos < end());
        size_t erase_pos = pos - begin();
        data_[erase_pos].~T();
        if constexpr (std::is_nothrow_move_assignable_v<T>) {
            std::move(data_.GetAddress() + erase_pos + 1, data_.GetAddress() + size_, data_.GetAddress() + erase_pos);
        } else {
            std::copy(data_.GetAddress() + erase_pos + 1, data_.GetAddress() + size_, data_.GetAddress() + erase_pos);
        }
        --size_;
        return begin() + erase_pos;
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;
};
