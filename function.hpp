#ifndef EXAM_FUNCTION_H
#define EXAM_FUNCTION_H
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>

namespace exam
{

template <typename>
struct function;

template <typename F, typename... Args>
struct function<F(Args...)> {

    static const size_t BUFFER_SIZE = 64;

    struct concept_ {
        virtual F call(Args... args) = 0;
        virtual void build_copy(void *buf) const = 0;
        virtual std::unique_ptr<concept_> copy() const = 0;
        virtual ~concept_() = default;
    };

    template <typename T>
    struct model_ : concept_ {
        model_(T t) : t(std::forward<T>(t)) {}

        std::unique_ptr<concept_> copy() const override
        {
            return std::make_unique<model_<T>>(t);
        }

        void build_copy(void *buf) const override { new (buf) model_<T>(t); }

        ~model_() override = default;

        F call(Args... args) override { return t(std::forward<Args>(args)...); }

    private:
        T t;
    };

    function() noexcept : ptr(nullptr), is_small(false) {}

    function(std::nullptr_t) noexcept : ptr(nullptr), is_small(false) {}

    template <typename T>
    function(T t)
    {
        if (sizeof(T) < BUFFER_SIZE) {
            is_small = true;
            new (buffer) model_<T>(std::forward<T>(t));
        } else {
            is_small = false;
            new (buffer) std::unique_ptr<concept_>(
                std::make_unique<model_<T>>(std::forward<T>(t)));
        }
    }

    function(const function &other)
    {
        if (other.is_small) {
            is_small = true;
            concept_ *other_concept = (concept_ *)(other.buffer);
            other_concept->build_copy(buffer);
        } else {
            is_small = false;
            new (buffer) std::unique_ptr<concept_>(other.ptr->copy());
        }
    }

    function(function &&other) noexcept
    {
        if (other.is_small) {
            is_small = true;
            memcpy(buffer, other.buffer, BUFFER_SIZE);
            std::unique_ptr<concept_> temp = nullptr;
            other.is_small = false;
            memcpy(other.buffer, &temp, sizeof(temp));
        } else {
            is_small = false;
            new (buffer) std::unique_ptr<concept_>(std::move(other.ptr));
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
        function f(std::move(other));
        swap(f);
        return *this;
    }

    void swap(function &other) noexcept
    {
        std::swap(buffer, other.buffer);
        std::swap(is_small, other.is_small);
    }

    F operator()(Args... args) const
    {
        if (is_small) {
            concept_ *concept_ptr = (concept_ *)(buffer);
            return concept_ptr->call(std::forward<Args>(args)...);
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
            ptr.reset();
        } else {
            ((concept_ *)(buffer))->~concept_();
        }
    }

private:
    union {
        std::unique_ptr<concept_> ptr;
        char buffer[BUFFER_SIZE];
    };
    bool is_small;
};

template <typename F, typename... Args>
void swap(function<F(Args...)> &f1, function<F(Args...)> &f2)
{
    f1.swap(f2);
}

} // namespace exam

#endif
