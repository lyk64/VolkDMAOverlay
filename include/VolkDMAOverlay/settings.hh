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
#include <vector>

namespace IniSettings {
    struct Writer {
        ImGuiTextBuffer& buf;

        void group(std::string_view name) {
            buf.appendf("%.*s:\n", static_cast<int>(name.size()), name.data());
        }
    };

    using ReadLine = std::move_only_function<void(std::string_view group, std::string_view line)>;
    using WriteAll = std::move_only_function<void(Writer& out) const>;

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

        [[nodiscard]] bool parse_floats(std::string_view text, std::span<float> out);

        void write_floats(ImGuiTextBuffer& buf, std::span<const float> values);
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

    template <typename Owner, typename T>
        requires detail::Scalar<T> || detail::FloatTuple<T> || detail::Text<T>
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

            if (const auto parsed = parse<T>(*text))
                target.*field.member = *parsed;

            return true;
        }

        template <typename Owner, typename T>
        void write_field(Writer& out, const Owner& target, const Field<Owner, T>& field) {
            const auto& stored = target.*field.member;

            out.buf.appendf("    %.*s=", static_cast<int>(field.name.size()), field.name.data());

            if constexpr (FloatTuple<T>)
                write_floats(out.buf, { &stored.x, float_count<T> });
            else if constexpr (Text<T>)
                out.buf.appendf("%.*s", static_cast<int>(stored.size()), stored.data());
            else if constexpr (std::floating_point<T>)
                out.buf.appendf("%g", static_cast<double>(stored));
            else
                out.buf.appendf("%lld", static_cast<long long>(stored));

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
        void write_group(Writer& out, const Owner& owner, const Group<Owner, Target, Fields>& definition) {
            out.group(definition.name);

            const auto& target = owner.*definition.member;
            std::apply([&](const auto&... field) {
                (write_field(out, target, field), ...);
            }, definition.fields);
        }
    }

    template <typename Owner, typename... Groups>
    bool read_grouped(std::string_view group, std::string_view line, Owner& owner, const Groups&... groups) {
        return (detail::read_group(group, line, owner, groups) || ...);
    }

    template <typename Owner, typename... Groups>
    void write_grouped(Writer& out, const Owner& owner, const Groups&... groups) {
        (detail::write_group(out, owner, groups), ...);
    }

    class Registry {
    public:
        Registry();
        ~Registry();

        Registry(const Registry&) = delete;
        Registry& operator=(const Registry&) = delete;

        void add(std::string_view type_name, ReadLine read, WriteAll write);
        void poll();

    private:
        std::vector<std::unique_ptr<detail::Section>> sections;
        ImGuiTextBuffer scratch;
        float elapsed{};
    };
}
