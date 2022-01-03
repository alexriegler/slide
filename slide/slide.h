#pragma once

#include <concepts>
#include <ranges>
#include <type_traits>

namespace ar
{
    namespace ranges
    {
        namespace detail
        {
            template<class R>
            concept simple_view =                         // exposition only
                std::ranges::view<R> && std::ranges::range<const R> &&
                std::same_as<std::ranges::iterator_t<R>, std::ranges::iterator_t<const R>> &&
                std::same_as<std::ranges::sentinel_t<R>, std::ranges::sentinel_t<const R>>;

            template<class T>
            inline constexpr auto to_unsigned_like(T number)
            {
                return static_cast<std::make_unsigned_t<T>>(number);
            }
        }

        template<class V>
        concept slide_caches_nothing = std::ranges::random_access_range<V> && std::ranges::sized_range<V>; // exposition only

        template<class V>
        concept slide_caches_last = !slide_caches_nothing<V> && std::ranges::bidirectional_range<V> && std::ranges::common_range<V>; // exposition only

        template<class V>
        concept slide_caches_first = !slide_caches_nothing<V> && !slide_caches_last<V>; // exposition only

        template<std::ranges::forward_range V>
            requires std::ranges::view<V>
        class slide_view : public std::ranges::view_interface<slide_view<V>>
        {
            V base_ = V();                                  // exposition only
            std::ranges::range_difference_t<V> n_ = 0;      // exposition only

            template<bool> class iterator;     // exposition only
            class sentinel;                    // exposition only

            // TODO: Remove?
            // Caches
            iterator<false> begin_cache_{};
            iterator<false> end_cache_{};

        public:
            // Constructors
            slide_view() requires std::default_initializable<V> = default;
            constexpr explicit slide_view(V base, std::ranges::range_difference_t<V> n)
                : base_{ std::move(base) }, n_{ n } {}

            // Functions
            constexpr auto begin()
                requires (!(detail::simple_view<V>&& slide_caches_nothing<const V>))
            {
                if constexpr (slide_caches_first<V>)
                {
                    if (begin_cache_ == sentinel{}) // TODO: Fix
                    {
                        begin_cache_ = iterator<false>(
                            std::ranges::begin(base_),
                            std::ranges::next(
                                std::ranges::begin(base_),
                                n_ - 1,
                                std::ranges::end(base_)),
                            n_
                            );
                    }
                    return begin_cache_;
                }
                else
                {
                    return iterator<false>(std::ranges::begin(base_), n_);
                }
            }

            constexpr auto begin() const requires slide_caches_nothing<const V>
            {
                return iterator<true>(std::ranges::begin(base_), n_);
                //1.21
            }

            constexpr auto end()
                requires (!(detail::simple_view<V>&& slide_caches_nothing<const V>))
            {
                if constexpr (slide_caches_nothing<V>)
                {
                    return iterator<false>(std::ranges::begin(base_) + range_difference_t<V>(size()), n_);
                }
                else if (slide_caches_last<V>)
                {
                    if (end_cache_ == sentinel{})  // TODO: Fix
                    {
                        end_cache_ = iterator<false>(
                            std::ranges::prev(
                                std::ranges::end(base_),
                                n_ - 1,
                                std::ranges::begin(base_)),
                            n_);
                    }
                    return end_cache_;
                }
                else if (std::ranges::common_range<V>)
                {
                    return iterator<false>(std::ranges::end(base_), std::ranges::end(base_), n_);
                }
                else
                {
                    return sentinel(std::ranges::end(base_));
                }
            }

            constexpr auto end() const requires slide_caches_nothing<const V>
            {
                return begin() + std::ranges::range_difference_t<const V>(size());
            }

            constexpr auto size() requires std::ranges::sized_range<V>
            {
                return size();
            }

            constexpr auto size() const requires std::ranges::sized_range<const V>
            {
                auto sz = std::ranges::distance(base_) - n_ + 1;
                if (sz < 0)
                {
                    sz = 0;
                }
                return detail::to_unsigned_like(sz);
            }

            /*constexpr V base() const& requires std::copy_constructible<V> { return base_; }
            constexpr V base()&& { return std::move(base_); }*/
        };

        template<class R>
        slide_view(R&& r, std::ranges::range_difference_t<R>)->slide_view<std::views::all_t<R>>;

        template<std::ranges::forward_range V>
            requires std::ranges::view<V>
        template<bool Const>
        class slide_view<V>::iterator
        {
            using Base = std::conditional_t<Const, const V, V>;                         // exposition only
            std::ranges::iterator_t<Base> current_ = std::ranges::iterator_t<Base>();   // exposition only
            std::ranges::iterator_t<Base> last_ele_ = std::ranges::iterator_t<Base>();  // exposition only, present only if Base models slide-caches-first
            std::ranges::range_difference_t<Base> n_ = 0;                               // exposition only

        public:
            // Type aliases
            using iterator_category = std::input_iterator_tag;
            /// <summary>
            /// iterator::iterator_concept is defined as follows:
            /// (1.1)
            /// If Base models random_access_range, then iterator_concept denotes random_access_iterator_tag.
            /// (1.2)
            /// Otherwise, if Base models bidirectional_range, then iterator_concept denotes bidirectional_iterator_tag.
            /// (1.3)
            /// Otherwise, iterator_concept denotes forward_iterator_tag.
            /// </summary>
            using iterator_concept =
                std::conditional_t<std::ranges::random_access_range<Base>,
                std::random_access_iterator_tag,
                std::conditional_t<std::ranges::bidirectional_range<Base>,
                std::bidirectional_iterator_tag,
                std::forward_iterator_tag>>;
            using value_type = decltype(std::views::counted(current_, n_));
            using difference_type = std::ranges::range_difference_t<Base>;

            // Constructors
            iterator() = default;
            // Constructor
            constexpr iterator(std::ranges::iterator_t<Base> current, std::ranges::range_difference_t<Base> n)    // exposition only
                requires (!slide_caches_first<Base>) : current_{ current }, n_{ n } {}

            constexpr iterator(std::ranges::iterator_t<Base> current, std::ranges::iterator_t<Base> last_ele, std::ranges::range_difference_t<Base> n) // exposition only
                requires slide_caches_first<Base> : current_{ current }, last_ele_{ last_ele }, n_{ n } {}
            // NOTE: iterator<true> can only be formed when Base models slide_caches_nothing,
            //       in which case last_ele_ is not present.
            constexpr iterator(iterator<!Const> i)
                requires Const&& std::convertible_to<std::ranges::iterator_t<V>, std::ranges::iterator_t<Base>>
            : current_{ std::move(i.current_) }, n_{ i.n_ } {}

            /*constexpr std::ranges::iterator_t<Base> base() const&
                requires std::copyable<std::ranges::iterator_t<Base>>
            {
                return current_;
            }

            constexpr std::ranges::iterator_t<Base> base()&&
            {
                return std::move(current_);
            }*/

            constexpr auto operator*() const
            {
                return std::views::counted(current_, n_);
            }

            constexpr iterator& operator++()
            {
                // TODO: Preconditions: current_ and last_ele_ (if present) are incrementable
                current_ = std::ranges::next(current_);
                if constexpr (slide_caches_first<V>)
                {
                    last_ele_ = std::ranges::next(last_ele_);
                }
                return *this;
            }

            constexpr iterator operator++(int)
            {
                auto tmp = *this;
                ++(*this);
                return tmp;
            }

            constexpr iterator& operator--() requires std::ranges::bidirectional_range<Base>
            {
                // TODO:  Preconditions: current_ and last_ele_ (if present) are decrementable.
                current_ = std::ranges::prev(current_);
                if constexpr (slide_caches_first<V>)
                {
                    last_ele_ = std::ranges::prev(last_ele_);
                }
                return *this;
            }

            constexpr iterator operator--(int) requires std::ranges::bidirectional_range<Base>
            {
                auto tmp = *this;
                --(*this);
                return tmp;
            }

            constexpr iterator& operator+=(difference_type x)
                requires std::ranges::random_access_range<Base>
            {
                // TODO: Preconditions: current_ + x and last_ele_ + x (if present) has well-defined behavior.
                std::ranges::advance(current_, x);
                if constexpr (slide_caches_first<V>)
                {
                    std::ranges::advance(last_ele_, x);
                }
                return *this;
            }

            constexpr iterator& operator-=(difference_type x)
                requires std::ranges::random_access_range<Base>
            {
                // TODO: Preconditions: current_ - x and last_ele_ - x (if present) has well-defined behavior.
                current_ = std::ranges::prev(current_, x);
                if constexpr (slide_caches_first<V>)
                {
                    last_ele_ = std::ranges::prev(last_ele_, x);
                }
                return *this;
            }

            constexpr auto operator[](difference_type n) const
                requires std::ranges::random_access_range<Base>
            {
                return std::views::counted(current_ + n, n_);
            }

            friend constexpr bool operator==(const iterator& x, const iterator& y)
            {
                if constexpr (slide_caches_first<V>)
                {
                    return x.last_ele_ == y.last_ele_;
                }
                else
                {
                    return x.current_ == y.current_;
                }
            }

            friend constexpr bool operator<(const iterator& x, const iterator& y)
                requires std::ranges::random_access_range<Base>
            {
                return x.current_ < y.current_;
            }
            friend constexpr bool operator>(const iterator& x, const iterator& y)
                requires std::ranges::random_access_range<Base>
            {
                return y < x;
            }
            friend constexpr bool operator<=(const iterator& x, const iterator& y)
                requires std::ranges::random_access_range<Base>
            {
                return !(y < x);
            }
            friend constexpr bool operator>=(const iterator& x, const iterator& y)
                requires std::ranges::random_access_range<Base>
            {
                return !(x < y);
            }
            friend constexpr auto operator<=>(const iterator& x, const iterator& y)
                requires std::ranges::random_access_range<Base>&& std::three_way_comparable<std::ranges::iterator_t<Base>>
            {
                return x.current_ <=> y.current_;
            }

            friend constexpr iterator operator+(const iterator& i, difference_type n)
                requires std::ranges::random_access_range<Base>
            {
                auto r = i;
                r += n;
                return r;
            }
            friend constexpr iterator operator+(difference_type n, const iterator& i)
                requires std::ranges::random_access_range<Base>
            {
                return i + n;
            }
            friend constexpr iterator operator-(const iterator& i, difference_type n)
                requires std::ranges::random_access_range<Base>
            {
                auto r = i;
                r -= n;
                return r;
            }
            friend constexpr difference_type operator-(const iterator& x, const iterator& y)
                requires std::sized_sentinel_for<std::ranges::iterator_t<Base>, std::ranges::iterator_t<Base>>
            {
                if constexpr (slide_caches_first<V>)
                {
                    return x.last_ele_ - y.last_ele_;
                }
                else
                {
                    return x.current_ - y.current_;
                }
            }
        };

        // NOTE: sentinel is only used when slide_caches_first<V> is true
        template<std::ranges::forward_range V>
            requires std::ranges::view<V>
        class slide_view<V>::sentinel
        {
            std::ranges::sentinel_t<V> end_ = std::ranges::sentinel_t<V>();                 // exposition only

        public:
            // Constructor
            sentinel() = default;
            // Constructor
            constexpr explicit sentinel(std::ranges::sentinel_t<V> end) : end_{ end } {}    // exposition only

            // TODO: Need base()?
            //constexpr std::ranges::sentinel_t<Base> base() const { return base_; }

            friend constexpr bool operator==(const iterator<false>& x, const sentinel& y)
            {
                return x.last_ele_ == y.end_;
            }

            friend constexpr std::ranges::range_difference_t<V>
                operator-(const iterator<false>& x, const sentinel& y)
                requires std::sized_sentinel_for<std::ranges::sentinel_t<V>, std::ranges::iterator_t<V>>
            {
                return x.last_ele_ - y.end_;
            }

            friend constexpr std::ranges::range_difference_t<V>
                operator-(const sentinel& y, const iterator<false>& x)
                requires std::sized_sentinel_for<std::ranges::sentinel_t<V>, std::ranges::iterator_t<V>>
            {
                return y.end_ - x.last_ele_;
            }
        };

        namespace views
        {
            namespace detail
            {
                template<std::integral N>
                struct slide_view_closure
                {
                    N size_;

                    template <std::ranges::viewable_range V>
                    friend constexpr auto operator|(V&& v, slide_view_closure const& clos)
                    {
                        return slide_view{ std::forward<V>(v), clos.size_ };
                    }
                };

                struct slide_fn
                {
                    template<std::ranges::viewable_range V>
                    constexpr auto operator()(V&& v, std::ranges::range_difference_t<V> n) const
                    {
                        return slide_view{ std::forward<V>(v), n };
                    }

                    template<std::integral N>
                    constexpr auto operator()(N n) const
                    {
                        return slide_view_closure{ n };
                    }

                    template<std::ranges::viewable_range V>
                    friend constexpr auto operator|(V&& v, slide_fn)
                    {
                        return slide_view{ std::forward<V>(v) };
                    }
                };
            }

            inline constexpr detail::slide_fn slide;
        }
    }

    namespace views = ranges::views;
}