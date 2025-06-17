export module Events.ApplicationEvents;

export import Event;
import "EventDefines.hpp";
import std.compat;

export class WindowResizeEvent : public Event {
public:
    WindowResizeEvent(uint32_t width, uint32_t height)
        : m_Width(width), m_Height(height) {}

    [[nodiscard]] uint32_t GetWidth() const { return m_Width; }
    [[nodiscard]] uint32_t GetHeight() const { return m_Height; }

    [[nodiscard]] std::pair<uint32_t, uint32_t> GetSize() const {
        return {m_Width, m_Height};
    }

    [[nodiscard]] virtual std::string ToString() const override {
        std::ostringstream oss;
        oss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
        return oss.str();
    }

    EVENT_CLASS_CATEGORY(EventCategory::EventCategoryApplication)
    EVENT_CLASS_TYPE(WindowResize)

private:
    uint32_t m_Width;
    uint32_t m_Height;
};

export class WindowCloseEvent : public Event {
public:
    WindowCloseEvent() = default;

    [[nodiscard]] virtual std::string ToString() const override {
        return "WindowCloseEvent";
    }

    EVENT_CLASS_CATEGORY(EventCategory::EventCategoryApplication)
    EVENT_CLASS_TYPE(WindowClose)
};

export class AppTickEvent : public Event {
public:
    AppTickEvent() = default;

    [[nodiscard]] virtual std::string ToString() const override {
        return "AppTickEvent";
    }

    EVENT_CLASS_CATEGORY(EventCategory::EventCategoryApplication)
    EVENT_CLASS_TYPE(AppTick)
};

export class AppRenderEvent : public Event {
public:
    AppRenderEvent() = default;

    [[nodiscard]] virtual std::string ToString() const override {
        return "AppRenderEvent";
    }

    EVENT_CLASS_CATEGORY(EventCategory::EventCategoryApplication)
    EVENT_CLASS_TYPE(AppRender)
};
