export module Event;

import "EventDefines.hpp";
import std.compat;

export enum class EventType : uint8_t {
    None = 0,
    WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
    AppTick, AppUpdate, AppRender,
    KeyPressed, KeyReleased, KeyTyped,
    MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
};

export enum class EventCategory : uint8_t {
    None = 0,
    EventCategoryApplication = 0b1,
    EventCategoryInput = 0b10,
    EventCategoryKeyboard = 0b100,
    EventCategoryMouse = 0b1000,
    EventCategoryMouseButton = 0b10000
};

export class Event {
public:
    virtual ~Event() = default;

    bool Handled = false;

    virtual constexpr EventType GetEventType() const = 0;

    virtual constexpr const char *GetName() const = 0;

    virtual constexpr uint8_t GetCategoryFlags() const = 0;

    virtual std::string ToString() const {
        return GetName();
    }

    static EventType GetStaticType() {
        return EventType::None;
    }

    bool IsInCategory(EventCategory category) const {
        return GetCategoryFlags() & static_cast<uint8_t>(category);
    }
};

export class EventDispatcher {
public:
    EventDispatcher(Event &event)
        : m_Event(event) {}

    template<typename T, typename F>
        requires std::derived_from<T, Event> && requires(F f, T &e) { { f(e) } -> std::convertible_to<bool>; }
    bool Dispatch(const F &func) {
        auto type = m_Event.GetEventType();
        if (type == T::GetStaticType()) {
            m_Event.Handled |= func(static_cast<T &>(m_Event));
            return true;
        }
        return false;
    }

private:
    template<typename T, typename Evt>
    using member_fn_t = bool(T::*)(Evt &);

public:
    template<typename T, typename Evt>
        requires std::derived_from<Evt, Event>
    bool Dispatch(member_fn_t<T, Evt> fn_ptr, T *obj) {
        auto type = m_Event.GetEventType();
        if (type == Evt::GetStaticType()) {
            m_Event.Handled |= (obj->*fn_ptr)(static_cast<Evt &>(m_Event));
            return true;
        }
        return false;
    }

private:
    Event &m_Event;
};

export std::ostream &operator<<(std::ostream &os, const Event &e) {
    return os << e.ToString();
}

export uint8_t operator|(EventCategory lhs, EventCategory rhs) {
    return static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs);
}

export uint8_t operator|(EventCategory lhs, uint8_t rhs) {
    return static_cast<uint8_t>(lhs) | rhs;
}

export uint8_t operator|(uint8_t lhs, EventCategory rhs) {
    return lhs | static_cast<uint8_t>(rhs);
}

export uint8_t operator&(EventCategory lhs, EventCategory rhs) {
    return static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs);
}

export uint8_t operator&(EventCategory lhs, uint8_t rhs) {
    return static_cast<uint8_t>(lhs) & rhs;
}

export uint8_t operator&(uint8_t lhs, EventCategory rhs) {
    return lhs & static_cast<uint8_t>(rhs);
}
