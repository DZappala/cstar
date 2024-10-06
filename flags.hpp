#pragma once

#include <bitset>
#include <type_traits>

/**
 * @brief Basic bitflags implementation; a rudimentary wrapper for the
 * std::bitset class (https://en.cpprefernce.com/w/cpp/utility/bitset)
 * @example Usage:
 *
 * ```
 * using States = Flags<StateEnum>
 *
 * States states;
 * states.set(StateEnum::some_enum_member);
 * if (states[StateEnum::some_enum_member]) {
 *   // perform some work...
 * }
 *
 * if (states[StateEnum::some_other_enum_member]) {
 *   // perform some other work...
 * }
 * ```
 * @note In the above example we use the enum StateEnum with the members
 * "some_enum_member" and "some_other_enum_member" to illustrate how you might
 * create a new set of Flags. You will need to define the enum yourself.
 * !! This enum must end with a final member named "size" (lower_case) which
 * is used to return the size of the bitset. !!
 *
 * See the "Debug" enum below for an example.
 */
template<typename EnumType>
  requires std::is_enum_v<EnumType>
class Flags {
    using UnderlyingType =
      typename std::make_unsigned_t<typename std::underlying_type_t<EnumType>>;

  public:
    auto set(EnumType e, bool value = true) noexcept -> Flags & {
      bits.set(underlying(e), value);
      return *this;
    }

    auto reset(EnumType e) noexcept -> Flags & {
      set(e, false);
      return *this;
    }

    auto reset() noexcept -> Flags & {
      bits.reset();
      return *this;
    }

    [[nodiscard]] auto all() const noexcept -> bool { return bits.all(); }
    [[nodiscard]] auto any() const noexcept -> bool { return bits.any(); }
    [[nodiscard]] auto none() const noexcept -> bool { return bits.none(); }
    [[nodiscard]] constexpr auto size() const noexcept -> std::size_t {
      return bits.size();
    }

    [[nodiscard]] auto count() const noexcept -> std::size_t {
      return bits.count();
    }

    constexpr auto operator[](EnumType e) const -> bool {
      return bits[underlying(e)];
    }

  private:
    static constexpr auto underlying(EnumType e) -> UnderlyingType {
      return static_cast<UnderlyingType>(e);
    }

    std::bitset<underlying(EnumType::size)> bits;
};

enum class Debug : std::uint8_t {
  Recieve,
  Send,
  Release,
  Process,
  Time,
  SquareTime,
  Instance,
  // ... add other flags here
  size // must be last, must be lower_case
};
