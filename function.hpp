#ifndef EXAM_FUNCTION_H
#define EXAM_FUNCTION_H
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <type_traits>

namespace exam
{

template <typename>
struct function;

template <typename F, typename... Args>
struct function<F(Args...)> {

    static const size_t BUFFER_SIZE = 64;

    struct concept_ {
        virtual F call(Args... args) const = 0;
        virtual void build_copy(void *buf) = 0;
        virtual void build_copy(void *buf) const = 0;
        virtual void build_moved_copy(void *buf) noexcept = 0;
        virtual std::unique_ptr<concept_> copy() const = 0;
        virtual ~concept_() = default;
    };

    template <typename T>
    struct model_ : concept_ {
        model_(T &&t) : t(std::forward<T>(t)) {}
        model_(T const &t) : t(t) {}

        std::unique_ptr<concept_> copy() const override
        {
            return std::make_unique<model_<T>>(t);
        }

        void build_copy(void *buf) override { new (buf) model_<T>(t); }

        void build_copy(void *buf) const override { new (buf) model_<T>(t); }

        void build_moved_copy(void *buf) noexcept
        {
            new (buf) model_<T>(std::move(t));
        }

        ~model_() override = default;

        F call(Args... args) const override
        {
            return t(std::forward<Args>(args)...);
        }

    private:
        T t;
    };

    function() noexcept : ptr(nullptr), is_small(false) {}

    function(std::nullptr_t) noexcept : ptr(nullptr), is_small(false) {}

    template <typename T>
    function(T t)
    {
        if constexpr (std::is_nothrow_move_constructible<T>::value &&
                      sizeof(model_<T>) <= BUFFER_SIZE &&
                      alignof(model_<T>) <= alignof(size_t)) {
            is_small = true;
            new (&buffer) model_<T>(std::move(t));
        } else {
            is_small = false;
            new (&buffer) std::unique_ptr<concept_>(
                std::make_unique<model_<T>>(std::move(t)));
        }
    }

    function(const function &other)
    {
        if (other.is_small) {
            is_small = true;
            concept_ const *other_concept =
                reinterpret_cast<concept_ const *>(&other.buffer);
            other_concept->build_copy(&buffer);
        } else {
            is_small = false;
            new (&buffer) std::unique_ptr<concept_>(other.ptr->copy());
        }
    }

    function(function &&other) noexcept
    {
        if (other.is_small) {
            is_small = true;
            concept_ *other_concept =
                reinterpret_cast<concept_ *>(&other.buffer);
            other_concept->build_moved_copy(&buffer);
            other_concept->~concept_();
            other.is_small = false;
            new (&other.buffer) std::unique_ptr<concept_>(nullptr);
        } else {
            is_small = false;
            new (&buffer) std::unique_ptr<concept_>(std::move(other.ptr));
        }
    }

    function &operator=(const function &other)
    {
        function temp(other);
        swap(temp);
        return *this;
    }

    function &operator=(function &&other) noexcept
    {
        if (this->is_small) {
            (reinterpret_cast<concept_ *>(&buffer))->~concept_();
        } else {
            (reinterpret_cast<std::unique_ptr<concept_> *>(&buffer))
                ->~unique_ptr();
        }
        if (other.is_small) {
            is_small = true;
            concept_ *other_concept =
                reinterpret_cast<concept_ *>(&other.buffer);
            other_concept->build_moved_copy(&buffer);
            other_concept->~concept_();
            other.is_small = false;
            new (&other.buffer) std::unique_ptr<concept_>(nullptr);
        } else {
            is_small = false;
            new (&buffer) std::unique_ptr<concept_>(std::move(other.ptr));
        }
        return *this;
    }

    void swap(function &other) noexcept
    {
        function f(std::move(other));
        other = std::move(*this);
        *this = std::move(f);
    }

    F operator()(Args... args) const
    {
        if (is_small) {
            return reinterpret_cast<concept_ const *>(&buffer)->call(
                std::forward<Args>(args)...);
        } else {
            return ptr->call(std::forward<Args>(args)...);
        }
    }

    explicit operator bool() const noexcept
    {
        return is_small || ptr.operator bool();
    }

    ~function()
    {
        if (!is_small) {
            (reinterpret_cast<std::unique_ptr<concept_> *>(&buffer))
                ->~unique_ptr();
        } else {
            (reinterpret_cast<concept_ *>(&buffer))->~concept_();
        }
    }

private:
    union {
        typename std::aligned_storage<BUFFER_SIZE, alignof(size_t)>::type
            buffer;
        std::unique_ptr<concept_> ptr;
    };
    bool is_small;
};

template <typename F, typename... Args>
void swap(function<F(Args...)> &f1, function<F(Args...)> &f2) noexcept
{
    f1.swap(f2);
}

} // namespace exam

#endif
