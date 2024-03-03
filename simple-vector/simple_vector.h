#pragma once

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <stdexcept>
#include <iterator>
#include <iostream>
#include <utility>
#include "array_ptr.h"

class ReserveProxyObj {
public:
    explicit ReserveProxyObj(size_t capacity_to_reserve)
        : capacity_(capacity_to_reserve)
    {
    }

    size_t ReserveCapacity() {
        return capacity_;
    }

private:
    size_t capacity_;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
};

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size) : SimpleVector(size, Type()) {
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value) 
        : items_(size)
        , size_(size)
        , capacite_(size) {
        std::fill(items_.Get(), items_.Get() + size, value);
    }

    SimpleVector(size_t size, Type&& value)
            : items_(size)
            , size_(size)
            , capacite_(size) {
        std::fill(items_.Get(), items_.Get() + size, std::move(value));
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) 
        : items_(init.size())
        , size_(init.size())
        , capacite_(init.size()) {
        std::copy(init.begin(), init.end(), items_.Get());
    }

    explicit SimpleVector(ReserveProxyObj reserve_proxy_obj)
    {
        Reserve(reserve_proxy_obj.ReserveCapacity());
    }

    SimpleVector(const SimpleVector& other)
        : items_(other.capacite_)
        , size_(other.size_) {        
        std::copy(other.begin(), other.end(), items_.Get());
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (&items_ == &rhs.items_) {
            return *this;
        }
        ArrayPtr<Type> tmp(rhs.GetCapacity());
        //std::copy(std::make_move_iterator(rhs.begin()), std::make_move_iterator(rhs.end()), tmp.Get());
        std::move(std::make_move_iterator(rhs.begin()), std::make_move_iterator(rhs.end()), tmp.Get());
        items_.swap(tmp);
        size_ = rhs.size_;
        capacite_ = rhs.capacite_;

        return *this;
    }

    SimpleVector(SimpleVector&& other) noexcept : items_(nullptr) {
        swap(other);
    }

    SimpleVector& operator=(SimpleVector&& other) noexcept {
        swap(other);
        return *this;
    }


    void Reserve(size_t new_capacity) {
        if (capacite_ < new_capacity) {
            ArrayPtr<Type> tmp(new_capacity);
            std::copy(begin(), end(), tmp.Get());            
            items_.swap(tmp);
            //size_ = 0;
            capacite_ = new_capacity;
        }

    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        if (capacite_ == size_) {
            size_t size = std::max<size_t>(capacite_ * 2, 1);
            ArrayPtr<Type> tmp(size);
            std::fill(tmp.Get(), tmp.Get() + size, Type());
            std::copy(items_.Get(), items_.Get() + size_, tmp.Get());
            items_.swap(tmp);
            capacite_ = size;
        }
        items_[size_++] = item;
    }

    void PushBack(Type&& item) {
        if (capacite_ == size_) {
            size_t size = std::max<size_t>(capacite_ * 2, 1);
            ArrayPtr<Type> tmp(size);
            //std::fill(std::make_move_iterator(tmp.Get()), std::make_move_iterator(tmp.Get() + size), Type());
            //std::generate(std::make_move_iterator(tmp.Get()), std::make_move_iterator(tmp.Get() + size), Type());
            //std::copy(std::make_move_iterator(begin()), std::make_move_iterator(end()), tmp.Get());
            std::move(std::make_move_iterator(begin()), std::make_move_iterator(end()), tmp.Get());
            items_.swap(tmp);
            capacite_ = size;
        }
        items_[size_++] = std::move(item);
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        assert(pos >= begin() && pos <= end());
        size_t dist = pos - begin();
        if (capacite_ == 0) {
            ArrayPtr<Type>tmp(1);
            tmp[dist] = value;
            items_.swap(tmp);
            ++capacite_;
            ++size_;
        }
        else if (size_ < capacite_) {                         //insert in end() + 1
            std::copy_backward(items_.Get() + dist, end(), items_.Get() + size_ + 1);
            items_[dist] = value;
            ++size_;
        }
        else {
            size_t size = std::max<size_t>(capacite_ * 2, 1);
            ArrayPtr<Type> tmp(size);
            std::copy(items_.Get(), items_.Get() + dist, tmp.Get());
            std::copy(items_.Get() + dist, end(), tmp.Get() + dist + 1);
            tmp[dist] = value;
            items_.swap(tmp);
            capacite_ = size;
            ++size_;
        }
        return &items_[dist];
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        assert(pos >= begin() && pos <= end());
        size_t dist = pos - begin();
        if (capacite_ == 0) {
            ArrayPtr<Type>tmp(1);
            tmp[dist] = std::move(value);
            items_.swap(tmp);
            ++capacite_;
            ++size_;
        }
        else if (size_ < capacite_) {                         //insert in end() + 1
            std::copy_backward(std::make_move_iterator(items_.Get() + dist), std::make_move_iterator(end()),
                               items_.Get() + size_ + 1);
            items_[dist] = std::move(value);
            ++size_;
        }
        else {
            size_t size = std::max<size_t>(capacite_ * 2, 1);
            ArrayPtr<Type> tmp(size);
            std::copy(std::make_move_iterator(items_.Get()), std::make_move_iterator(items_.Get() + dist),
                      tmp.Get());
            std::copy(std::make_move_iterator(items_.Get() + dist), std::make_move_iterator(end()),
                      tmp.Get() + dist + 1);
            tmp[dist] = std::move(value);
            items_.swap(tmp);
            capacite_ = size;
            ++size_;
        }
        return &items_[dist];
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        if (items_) {
            --size_;
        }
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        assert(pos >= begin() && pos <= end());
        if (size_ > 0) {
            size_t dist = pos - begin();                                      //insert in end() - 1              
            /*std::copy_backward(std::make_move_iterator(items_.Get() + dist + 1), std::make_move_iterator(items_.Get() + size_),
                               std::make_move_iterator(items_.Get() + size_ - 1));*/
            ArrayPtr<Type> tmp(size_ - 1);
            std::copy(std::make_move_iterator(items_.Get()), std::make_move_iterator(items_.Get() + dist),
                    tmp.Get());
            std::copy(std::make_move_iterator(items_.Get() + dist + 1), std::make_move_iterator(items_.Get() + size_),
                      tmp.Get() + dist);
            items_.swap(tmp);
            --size_;
            return begin() + dist;
        }
        return begin();
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        items_.swap(other.items_);
        std::swap(size_, other.size_);
        std::swap(capacite_, other.capacite_);
    }

    void swap(SimpleVector&& other) noexcept {
        items_.swap(other.items_);
        std::swap(size_, other.size_);
        std::swap(capacite_, other.capacite_);
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {        
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {        
        return capacite_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("index >= size_");
        }        
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("index >= size_");
        }
        return items_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (new_size <= capacite_) {
            size_ = new_size;
        }
        else {
            size_t new_capacity = std::max(new_size, capacite_ * 2);
            ArrayPtr<Type> new_array(new_capacity);
            std::copy(std::make_move_iterator(items_.Get()), std::make_move_iterator(items_.Get() + size_),
                     new_array.Get());
            //std::fill(std::make_move_iterator(new_array.Get() + size_), std::make_move_iterator(new_array.Get() + new_size), Type());
            //std::generate(std::make_move_iterator(new_array.Get() + size_), std::make_move_iterator(new_array.Get() + new_size), Type());
            for(auto it = new_array.Get() + size_; it != new_array.Get() + new_size; ++it){
                *it = std::move(Type());
            }
            items_.swap(new_array);
            size_ = new_size;
            capacite_ = new_capacity;
        }
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return items_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return items_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return items_.Get() + size_;
    }

private:
    ArrayPtr<Type> items_;
    size_t size_ = 0;
    size_t capacite_ = 0;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs > lhs);
}

