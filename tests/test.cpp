#include "slide.h"

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <sstream>
#include <vector>

// An attempt at implementing a slide function with preexisting adapters.
//namespace ar
//{
//	template<std::ranges::input_range R>
//		requires std::integral<std::ranges::range_value_t<R>>
//	[[nodiscard]] constexpr auto slide(R&& r, const std::ranges::range_difference_t<R> window_size = 1)
//	{
//		Expects(window_size > 0);
//		auto window = [=, r_ = std::forward<R>(r), idx = 0](auto) mutable {
//			return r_ | std::ranges::views::drop(idx++) | std::ranges::views::take(window_size);
//		};
//		return std::forward<R>(r)
//			| std::ranges::views::transform(window)
//			| std::views::take(r.size() - (window_size - 1));
//	}
//}

TEST_CASE("vector")
{
    std::vector v = { 1, 2, 3, 4 };
    std::stringstream s{};
    SECTION("Pipe syntax")
    {
        for (auto&& i : v | ar::views::slide(2))
        {
            s << "[" << i[0] << ", " << i[1] << "]";
        }
        REQUIRE(s.str() == "[1, 2][2, 3][3, 4]");
    }
    SECTION("Function syntax")
    {
        for (auto&& i : ar::views::slide(v, 2))
        {
            s << "[" << i[0] << ", " << i[1] << "]";
        }
        REQUIRE(s.str() == "[1, 2][2, 3][3, 4]");
    }
}
