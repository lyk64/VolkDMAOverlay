#pragma once

#include <imgui.h>

#include <charconv>
#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace IniSettings {
    class DocumentWriter {
    public:
        explicit DocumentWriter(ImGuiTextBuffer& buf) : buf{ buf } {}

        void group(std::string_view path);
        void key(std::string_view name);

        ImGuiTextBuffer& buf;

    private:
        void indent();

        std::string open;
        int depth{};
    };

    using ReadLine = std::move_only_function<void(std::string_view group, std::string_view line)>;
    using WriteAll = std::move_only_function<void(DocumentWriter& out) const>;
    using Applied = std::move_only_function<void()>;

    [[nodiscard]] constexpr std::optional<std::string_view> value(std::string_view line, std::string_view key) {
        if (!line.starts_with(key) || line.size() <= key.size() || line[key.size()] != '=')
            return std::nullopt;

        return line.substr(key.size() + 1);
    }

    namespace detail {
        struct Section;

        template <typename T>
        inline constexpr size_t float_count = 0;

        template <>
        inline constexpr size_t float_count<ImVec2> = 2;

        template <>
        inline constexpr size_t float_count<ImVec4> = 4;

        template <typename T>
        concept FloatTuple = float_count<T> > 0;

        template <typename T>
        concept Scalar = std::integral<T> || std::floating_point<T>;

        template <typename T>
        concept Text = std::same_as<T, std::string>;

        template <typename T>
        concept Enum = std::is_enum_v<T>;

        template <typename T>
        concept Composite = requires(const T& stored, ImGuiTextBuffer& buf, T& target, std::string_view text) {
            stored.write(buf);
            { target.read(text) } -> std::convertible_to<bool>;
        };

        template <typename T>
        concept Map = requires(T& target, const T& stored) {
            typename T::key_type;
            typename T::mapped_type;
            target.clear();
            stored.begin();
            stored.end();
        };

        template <typename T>
        concept Storable = Scalar<T> || FloatTuple<T> || Text<T> || Enum<T> || Composite<T> || Map<T>;

        [[nodiscard]] bool parse_floats(std::string_view text, std::span<float> out);

        void write_floats(ImGuiTextBuffer& buf, std::span<const float> values);

        [[nodiscard]] std::string_view trim(std::string_view text);

        [[nodiscard]] std::optional<std::string_view> group_marker(std::string_view line);

        class GroupTracker {
        public:
            [[nodiscard]] bool feed(std::string_view raw, std::string_view trimmed);
            [[nodiscard]] std::string_view path() const noexcept { return joined; }

            void reset();

        private:
            struct Level {
                size_t columns;
                std::string name;
            };

            std::vector<Level> stack;
            std::string joined;
        };

        template <typename OnLine>
        void parse_lines(std::string_view text, GroupTracker& tracker, OnLine on_line) {
            while (!text.empty()) {
                const auto newline = text.find('\n');
                const auto raw = text.substr(0, newline);
                const auto line = trim(raw);

                if (newline == std::string_view::npos)
                    text = {};
                else
                    text.remove_prefix(newline + 1);

                if (line.empty())
                    continue;

                if (!tracker.feed(raw, line))
                    on_line(tracker.path(), line);
            }
        }
    }

    template <detail::Scalar T>
    [[nodiscard]] std::optional<T> parse(std::string_view text) {
        if constexpr (std::same_as<T, bool>) {
            const auto parsed = parse<int>(text);
            return parsed ? std::optional{ *parsed != 0 } : std::nullopt;
        }
        else {
            T parsed{};
            const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed);
            return result.ec == std::errc{} ? std::optional{ parsed } : std::nullopt;
        }
    }

    template <detail::FloatTuple T>
    [[nodiscard]] std::optional<T> parse(std::string_view text) {
        T parsed{};
        if (!detail::parse_floats(text, { &parsed.x, detail::float_count<T> }))
            return std::nullopt;

        return parsed;
    }

    template <detail::Text T>
    [[nodiscard]] std::optional<T> parse(std::string_view text) {
        return T{ text };
    }

    template <detail::Enum T>
    [[nodiscard]] std::optional<T> parse(std::string_view text) {
        const auto parsed = parse<std::underlying_type_t<T>>(text);
        return parsed ? std::optional{ static_cast<T>(*parsed) } : std::nullopt;
    }

    template <detail::Map T>
    [[nodiscard]] std::optional<T> parse(std::string_view text) {
        T parsed{};

        while (!text.empty()) {
            const auto comma = text.find(',');
            const auto entry = text.substr(0, comma);
            const auto colon = entry.find(':');
            if (colon == std::string_view::npos)
                return std::nullopt;

            auto key = parse<typename T::key_type>(entry.substr(0, colon));
            auto value = parse<typename T::mapped_type>(entry.substr(colon + 1));
            if (!key || !value)
                return std::nullopt;

            parsed.emplace(std::move(*key), std::move(*value));

            if (comma == std::string_view::npos)
                break;

            text.remove_prefix(comma + 1);
        }

        return parsed;
    }

    template <typename Owner, detail::Storable T>
    struct Field {
        std::string_view name;
        T Owner::* member;
    };

    template <typename Owner, typename T>
    Field(std::string_view, T Owner::*) -> Field<Owner, T>;

    template <typename Owner, typename Target, typename Fields>
    struct Group {
        std::string_view name;
        Target Owner::* member;
        Fields fields;
    };

    template <typename Owner, typename Target, typename Fields>
    Group(std::string_view, Target Owner::*, Fields) -> Group<Owner, Target, Fields>;

    namespace detail {
        template <typename Owner, typename T>
        bool read_field(std::string_view line, Owner& target, const Field<Owner, T>& field) {
            const auto text = value(line, field.name);
            if (!text)
                return false;

            if constexpr (Composite<T>)
                (target.*field.member).read(*text);
            else if (const auto parsed = parse<T>(*text))
                target.*field.member = *parsed;

            return true;
        }

        template <typename T>
        void write_value(ImGuiTextBuffer& buf, const T& stored) {
            if constexpr (FloatTuple<T>)
                write_floats(buf, { &stored.x, float_count<T> });
            else if constexpr (Text<T>)
                buf.appendf("%.*s", static_cast<int>(stored.size()), stored.data());
            else if constexpr (Composite<T>)
                stored.write(buf);
            else if constexpr (Enum<T>)
                buf.appendf("%lld", static_cast<long long>(std::to_underlying(stored)));
            else if constexpr (Map<T>) {
                bool first = true;

                for (const auto& [key, value] : stored) {
                    if (!first)
                        buf.append(",");

                    first = false;
                    write_value(buf, key);
                    buf.append(":");
                    write_value(buf, value);
                }
            }
            else if constexpr (std::floating_point<T>)
                buf.appendf("%g", static_cast<double>(stored));
            else
                buf.appendf("%lld", static_cast<long long>(stored));
        }

        template <typename Owner, typename T>
        void write_field(DocumentWriter& out, const Owner& target, const Field<Owner, T>& field) {
            out.key(field.name);
            write_value(out.buf, target.*field.member);
            out.buf.append("\n");
        }

        template <typename Owner, typename Target, typename Fields>
        bool read_group(std::string_view group, std::string_view line, Owner& owner,
                        const Group<Owner, Target, Fields>& definition) {
            if (group != definition.name)
                return false;

            auto& target = owner.*definition.member;
            return std::apply([&](const auto&... field) {
                return (read_field(line, target, field) || ...);
            }, definition.fields);
        }

        template <typename Owner, typename Target, typename Fields>
        void write_group(DocumentWriter& out, const Owner& owner, const Group<Owner, Target, Fields>& definition) {
            out.group(definition.name);

            const auto& target = owner.*definition.member;
            std::apply([&](const auto&... field) {
                (write_field(out, target, field), ...);
            }, definition.fields);
        }
    }

    class ValueReader {
    public:
        explicit ValueReader(std::string_view text) : remaining{ text } {}

        template <typename T>
        [[nodiscard]] bool take(T& out) {
            if constexpr (detail::FloatTuple<T>) {
                for (float& component : std::span<float>{ &out.x, detail::float_count<T> })
                    if (!take(component))
                        return false;

                return true;
            }
            else {
                if (remaining.empty())
                    return false;

                const auto comma = remaining.find(',');
                const auto token = remaining.substr(0, comma);
                remaining = comma == std::string_view::npos ? std::string_view{} : remaining.substr(comma + 1);

                const auto parsed = parse<T>(token);
                if (!parsed)
                    return false;

                out = *parsed;
                return true;
            }
        }

    private:
        std::string_view remaining;
    };

    class ValueWriter {
    public:
        explicit ValueWriter(ImGuiTextBuffer& buf) : buf{ buf } {}

        template <typename T>
        void put(const T& value) {
            if (!first)
                buf.append(",");

            first = false;
            detail::write_value(buf, value);
        }

    private:
        ImGuiTextBuffer& buf;
        bool first = true;
    };

    template <typename Owner, typename... Groups>
    bool read_grouped(std::string_view group, std::string_view line, Owner& owner, const Groups&... groups) {
        return (detail::read_group(group, line, owner, groups) || ...);
    }

    template <typename Owner, typename... Groups>
    void write_grouped(DocumentWriter& out, const Owner& owner, const Groups&... groups) {
        (detail::write_group(out, owner, groups), ...);
    }

    template <typename Owner, typename... Groups>
    void read_document(std::string_view text, Owner& owner, const Groups&... groups) {
        detail::GroupTracker tracker;

        detail::parse_lines(text, tracker, [&](std::string_view group, std::string_view line) {
            read_grouped(group, line, owner, groups...);
        });
    }

    template <typename Owner, typename... Groups>
    void write_document(ImGuiTextBuffer& buf, const Owner& owner, const Groups&... groups) {
        DocumentWriter out{ buf };
        write_grouped(out, owner, groups...);
    }

    class Registry {
    public:
        Registry();
        ~Registry();

        Registry(const Registry&) = delete;
        Registry& operator=(const Registry&) = delete;

        void add(std::string_view type_name, ReadLine read, WriteAll write, Applied applied = {});
        void add_document(std::string_view path, ReadLine read, WriteAll write, Applied applied = {});
        void poll();
        void flush();

    private:
        std::vector<std::unique_ptr<detail::Section>> sections;
        ImGuiTextBuffer scratch;
        float elapsed{};
    };
}
