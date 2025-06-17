#pragma once

#define EVENT_CLASS_TYPE(type) static constexpr EventType GetStaticType() { return EventType::type; }\
virtual constexpr EventType GetEventType() const override { return GetStaticType(); }\
virtual constexpr const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) virtual uint8_t GetCategoryFlags() const override { return static_cast<uint8_t>(category); }