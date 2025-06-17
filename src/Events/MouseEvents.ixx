export module Events.MouseEvents;

import "EventDefines.hpp";

export import MouseCodes;
export import Event;
import std.compat;

export class MouseMovedEvent : public Event {
public:
    MouseMovedEvent(float x, float y)
        : m_MouseX(x), m_MouseY(y) {}

    [[nodiscard]] float GetX() const { return m_MouseX; }
    [[nodiscard]] float GetY() const { return m_MouseY; }

    [[nodiscard]] std::pair<float, float> GetPosition() const {
        return {m_MouseX, m_MouseY};
    }

    [[nodiscard]] virtual std::string ToString() const override {
        std::ostringstream oss;
        oss << "MouseMovedEvent: " << m_MouseX << ", " << m_MouseY;
        return oss.str();
    }

    EVENT_CLASS_CATEGORY(EventCategory::EventCategoryInput | EventCategory::EventCategoryMouse)
    EVENT_CLASS_TYPE(MouseMoved)

private:
    float m_MouseX;
    float m_MouseY;
};

export class MouseScrolledEvent : public Event {
public:
    MouseScrolledEvent(float xOffset, float yOffset)
        : m_XOffset(xOffset), m_YOffset(yOffset) {}

    [[nodiscard]] float GetXOffset() const { return m_XOffset; }
    [[nodiscard]] float GetYOffset() const { return m_YOffset; }

    [[nodiscard]] virtual std::string ToString() const override {
        std::ostringstream oss;
        oss << "MouseScrolledEvent: " << m_XOffset << ", " << m_YOffset;
        return oss.str();
    }

    EVENT_CLASS_CATEGORY(EventCategory::EventCategoryInput | EventCategory::EventCategoryMouse)
    EVENT_CLASS_TYPE(MouseScrolled)

private:
    float m_XOffset;
    float m_YOffset;
};

export class MouseButtonEvent : public Event {
public:
    [[nodiscard]] Mouse::MouseCode GetMouseButton() const { return m_Button; }

    [[nodiscard]] virtual std::string ToString() const override {
        std::ostringstream oss;
        oss << "MouseButtonEvent: " << static_cast<int>(m_Button);
        return oss.str();
    }

    EVENT_CLASS_CATEGORY(
        EventCategory::EventCategoryInput | EventCategory::EventCategoryMouse |
        EventCategory:: EventCategoryMouseButton)

protected:
    Mouse::MouseCode m_Button;

    explicit MouseButtonEvent(const Mouse::MouseCode button)
        : m_Button(button) {}
};

export class MouseButtonPressedEvent : public MouseButtonEvent {
public:
    MouseButtonPressedEvent(const Mouse::MouseCode button)
        : MouseButtonEvent(button) {}

    [[nodiscard]] virtual std::string ToString() const override {
        std::ostringstream oss;
        oss << "MouseButtonPressedEvent: " << static_cast<int>(m_Button);
        return oss.str();
    }

    EVENT_CLASS_TYPE(MouseButtonPressed)
};

export class MouseButtonReleasedEvent : public MouseButtonEvent {
public:
    MouseButtonReleasedEvent(const Mouse::MouseCode button)
        : MouseButtonEvent(button) {}

    [[nodiscard]] virtual std::string ToString() const override {
        std::ostringstream oss;
        oss << "MouseButtonReleasedEvent: " << static_cast<int>(m_Button);
        return oss.str();
    }

    EVENT_CLASS_TYPE(MouseButtonReleased)
};
