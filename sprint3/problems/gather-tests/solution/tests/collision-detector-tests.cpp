#define _USE_MATH_DEFINES

#include "../src/collision_detector.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_contains.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>
#include <vector>
// Напишите здесь тесты для функции collision_detector::FindGatherEvents

using Catch::Matchers::Contains;
using Catch::Matchers::WithinRel;
using namespace collision_detector;
using namespace std::literals;

namespace Catch {
template<>
struct StringMaker<GatheringEvent> {
  static std::string convert(GatheringEvent const& value) {
      std::ostringstream tmp;
      tmp << "(" << value.gatherer_id << "," << value.item_id << "," << value.sq_distance << "," << value.time << ")";

      return tmp.str();
  }
};
} // namespace Catch

class TestItemGathererProvider : public ItemGathererProvider {
public:

    size_t ItemsCount() const override {
        return items_.size();
    }

    Item GetItem(size_t idx) const override {
        return items_.at(idx);
    }

    size_t GatherersCount() const override {
        return gatherers_.size();
    }

    Gatherer GetGatherer(size_t idx) const override {
        return gatherers_.at(idx);
    }

    TestItemGathererProvider& AddGatherer(Gatherer gatherer) {
        gatherers_.push_back(std::move(gatherer));
        return *this;
    }

    TestItemGathererProvider& AddItem(Item item) {
        items_.push_back(std::move(item));
        return *this;
    }

private:
    std::vector<Gatherer> gatherers_;
    std::vector<Item> items_;
};

template <typename Range>
struct IsPermutationMatcher : Catch::Matchers::MatcherGenericBase {
    IsPermutationMatcher(const Range& range)
        : range_{range} {
    }
    IsPermutationMatcher(IsPermutationMatcher&&) = default;

    template <typename OtherRange>
    bool match(OtherRange other) const {
        using std::begin;
        using std::end;

        return std::equal(begin(range_), end(range_), begin(other), end(other), [](const auto& lhs,
                                                                                   const auto& rhs) {
            return lhs.item_id == rhs.item_id && lhs.gatherer_id == rhs.gatherer_id;
        });
    }

    std::string describe() const override {
        // Описание свойства, проверяемого матчером:
        return "Is permutation of: "s + Catch::rangeToString(range_);
    }

private:
    Range range_;
};

template<typename Range>
IsPermutationMatcher<Range> IsPermutation(Range&& range) {
    return IsPermutationMatcher<Range>{std::forward<Range>(range)};
}

TEST_CASE("FindGatherEvents test case", "[gather events]") {
    TestItemGathererProvider provider;
    REQUIRE(provider.GatherersCount() == 0);
    REQUIRE(provider.ItemsCount() == 0);
    REQUIRE(FindGatherEvents(provider).size() == 0);

    std::vector<GatheringEvent> right_events;
    SECTION("Gatherer moves through the items") {

        SECTION("Gatherer moves vertical") {
            provider.AddGatherer({{1., 1.}, {1., 2.}, 1.})
                    .AddItem({{1., 1.5}, 1.});
            right_events.push_back({0, 0, 0., 0.});
            SECTION("Gatherer takes one item in the middle of the route") {
                CHECK_THAT(FindGatherEvents(provider), IsPermutation(right_events));
            }

            provider.AddItem({{1., 2.}, 1.});
            right_events.push_back({1, 0, 0., 0.});
            SECTION("Gatherer takes two items: in the middle of the route and in the end") {
                CHECK_THAT(FindGatherEvents(provider), IsPermutation(right_events));
            }

            provider.AddItem({{3., 2.}, 1.})
                    .AddItem({{-1., 2.}, 1.});

            right_events.push_back({2, 0, 0., 0.});
            right_events.push_back({3, 0, 0., 0.});
            SECTION("Gatherer must collect items on the edges") {
                CHECK_THAT(FindGatherEvents(provider), IsPermutation(right_events));
            }

            provider.AddItem({{1., 2.1}, 1.})
                    .AddItem({{1., 0.9}, 1.})
                    .AddItem({{-1.01, 2.}, 1.})
                    .AddItem({{3.01, 2.}, 1.})
                    .AddItem({{2., 3.}, 1.})
                    .AddItem({{10., 12.}, 1.});

            SECTION("Gatherer doesn't take unnecessary items") {
                INFO("added couple random items out of gatherer path");
                CHECK_THAT(FindGatherEvents(provider), IsPermutation(right_events));
            }

            auto events = FindGatherEvents(provider);
            SECTION("events follow in chronological order") {
                CHECK(std::is_sorted(events.begin(), events.end(), [](const GatheringEvent& lhs,
                                                                      const GatheringEvent& rhs) {
                    return lhs.time < rhs.time;
                }));
            }

            SECTION("FindGatherEvents returns right data (sq_distance and time)") {
                for (const auto& event : events) {
                    Item item = provider.GetItem(event.item_id);
                    Gatherer gatherer = provider.GetGatherer(event.gatherer_id);

                    auto [sq_distance, proj_ratio] = TryCollectPoint(gatherer.start_pos, gatherer.end_pos,
                                                                     item.position);

                    REQUIRE_THAT(event.sq_distance, WithinRel(sq_distance, 1e-10));
                    REQUIRE_THAT(event.time, WithinRel(proj_ratio, 1e-10));
                }
            }
        }

        SECTION("Gatherer moves horizontal") {
            provider.AddGatherer({{1., 1.}, {2., 1.}, 1.})
                    .AddItem({{1.5, 1.}, 1.});
            right_events.push_back({0, 0, 0., 0.});
            SECTION("Gatherer takes one item in the middle of the route") {
                CHECK_THAT(FindGatherEvents(provider), IsPermutation(right_events));
            }

            provider.AddItem({{2., 1.}, 1.});
            right_events.push_back({1, 0, 0., 0.});
            SECTION("Gatherer takes two items: in the middle of the route and in the end") {
                CHECK_THAT(FindGatherEvents(provider), IsPermutation(right_events));
            }

            provider.AddItem({{2., 3.}, 1.})
                    .AddItem({{2., -1.}, 1.});

            right_events.push_back({2, 0, 0., 0.});
            right_events.push_back({3, 0, 0., 0.});
            SECTION("Gatherer must collect items on the edges") {
                CHECK_THAT(FindGatherEvents(provider), IsPermutation(right_events));
            }

            provider.AddItem({{2.1, 1.}, 1.})
                    .AddItem({{0.9, 1.}, 1.})
                    .AddItem({{2., -1.01}, 1.})
                    .AddItem({{2., 3.01}, 1.})
                    .AddItem({{3., 2.}, 1.})
                    .AddItem({{12., 10.}, 1.});
            SECTION("Gatherer doesn't take unnecessary items") {
                INFO("added couple random items out of gatherer path");
                CHECK_THAT(FindGatherEvents(provider), IsPermutation(right_events));
            }

            auto events = FindGatherEvents(provider);
            SECTION("events follow in chronological order") {
                CHECK(std::is_sorted(events.begin(), events.end(), [](const GatheringEvent& lhs,
                                                                      const GatheringEvent& rhs) {
                    return lhs.time < rhs.time;
                }));
            }

            SECTION("FindGatherEvents returns right data (sq_distance and time)") {
                for (const auto& event : events) {
                    Item item = provider.GetItem(event.item_id);
                    Gatherer gatherer = provider.GetGatherer(event.gatherer_id);

                    auto [sq_distance, proj_ratio] = TryCollectPoint(gatherer.start_pos, gatherer.end_pos,
                                                                     item.position);

                    REQUIRE_THAT(event.sq_distance, WithinRel(sq_distance, 1e-10));
                    REQUIRE_THAT(event.time, WithinRel(proj_ratio, 1e-10));
                }
            }
        }
    }
}
