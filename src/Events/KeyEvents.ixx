export module Events.KeyEvents;


export import KeyCodes;
export import Event;
import "EventDefines.hpp";
import std.compat;


    export class KeyEvent : public Event {
    public:
        [[nodiscard]] Key::KeyCode GetKeyCode() const { return m_KeyCode; }

        EVENT_CLASS_CATEGORY(EventCategory::EventCategoryInput | EventCategory::EventCategoryKeyboard)

    protected:
        explicit KeyEvent(const Key::KeyCode keyCode)
            : m_KeyCode(keyCode) {}

    private:
        Key::KeyCode m_KeyCode;
    };

    export class KeyPressedEvent : public KeyEvent {
    public:
        explicit KeyPressedEvent(const Key::KeyCode keyCode, bool isRepeat = false)
            : KeyEvent(keyCode), m_IsRepeat(isRepeat) {}

        [[nodiscard]] bool IsRepeat() const { return m_IsRepeat; }

        [[nodiscard]] virtual std::string ToString() const override {
            std::ostringstream oss;
            oss << "KeyPressedEvent: " << static_cast<int>(GetKeyCode()) << " ("
                    << (m_IsRepeat ? "repeat" : "not repeat") << ")";
            return oss.str();
        }

        EVENT_CLASS_TYPE(KeyPressed)
    private:
        bool m_IsRepeat;
    };

    export class KeyReleasedEvent : public KeyEvent {
    public:
        explicit KeyReleasedEvent(const Key::KeyCode keyCode)
            : KeyEvent(keyCode) {}

        [[nodiscard]] virtual std::string ToString() const override {
            std::ostringstream oss;
            oss << "KeyReleasedEvent: " << static_cast<int>(GetKeyCode());
            return oss.str();
        }

        EVENT_CLASS_TYPE(KeyReleased)
    };

    export class KeyTypedEvent : public KeyEvent {
    public:
        explicit KeyTypedEvent(const Key::KeyCode keyCode)
            : KeyEvent(keyCode) {}

        [[nodiscard]] virtual std::string ToString() const override {
            std::ostringstream oss;
            oss << "KeyTypedEvent: " << static_cast<char>(GetKeyCode());
            return oss.str();
        }

        EVENT_CLASS_TYPE(KeyTyped)
    };
