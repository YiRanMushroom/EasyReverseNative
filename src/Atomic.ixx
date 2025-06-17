export module Atomic;

import std;

export template<typename T>
class Atomic {
public:
    Atomic() = default;
    Atomic(const Atomic&) = delete;
    Atomic(Atomic&&) = delete;

    Atomic(auto&&... args) : m_Value(std::forward<decltype(args)>(args)...) {
    }

    Atomic& operator=(auto&& value) {
        this->GetProxy().Set(std::forward<decltype(value)>(value));
        return *this;
    }

    struct Proxy {
        Atomic* m_Atomic;
        Proxy(Atomic& atomic) : m_Atomic(&atomic) {
        }

        T& operator*() {
            return m_Atomic->m_Value;
        }

        ~Proxy() {
            m_Atomic->m_Mutex.unlock();
        }

        void Set(const T& value) {
            m_Atomic->m_Value = value;
        }

        T& Get() {
            return m_Atomic->m_Value;
        }

        T* operator->() {
            return &m_Atomic->m_Value;
        }
    };

    Proxy GetProxy() {
        m_Mutex.lock();
        return Proxy(*this);
    }

    std::optional<Proxy> TryGetProxy() {
        if (m_Mutex.try_lock()) {
            return Proxy(*this);
        }
        return std::nullopt;
    }

private:
    std::mutex m_Mutex{};
    T m_Value;
};