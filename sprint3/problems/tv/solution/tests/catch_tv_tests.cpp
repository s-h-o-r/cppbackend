#include <catch2/catch_test_macros.hpp>
#include <iostream>

#include "../src/tv.h"

namespace Catch {

template <>
struct StringMaker<std::nullopt_t> {
    static std::string convert(std::nullopt_t) {
        using namespace std::literals;
        return "nullopt"s;
    }
};

template <typename T>
struct StringMaker<std::optional<T>> {
    static std::string convert(const std::optional<T>& opt_value) {
        if (opt_value) {
            return StringMaker<T>::convert(*opt_value);
        } else {
            return StringMaker<std::nullopt_t>::convert(std::nullopt);
        }
    }
};

}  // namespace Catch

SCENARIO("TV", "[TV]") {
    GIVEN("A TV") {  // Дано: Телевизор
        TV tv;

        // Изначально он выключен и не показывает никаких каналов
        SECTION("Initially it is off and doesn't show any channel") {
            CHECK(!tv.IsTurnedOn());
            CHECK(!tv.GetChannel().has_value());
        }

        // Когда он выключен,
        WHEN("it is turned off") {
            REQUIRE(!tv.IsTurnedOn());

// Включите эту секцию и доработайте класс TV, чтобы он проходил проверки в ней
            
            // он не может переключать каналы
            THEN("it can't select any channel") {
                CHECK_THROWS_AS(tv.SelectChannel(10), std::logic_error);
                CHECK(tv.GetChannel() == std::nullopt);
                tv.TurnOn();
                CHECK(tv.GetChannel() == 1);
            }

            AND_THEN("SelectLastViewedChannel throw logic_error") {
                CHECK_THROWS_AS(tv.SelectLastViewedChannel(), std::logic_error);
            }
        }

        WHEN("it is turned on first time") {  // Когда его включают в первый раз,
            tv.TurnOn();

            // то он включается и показывает канал #1
            THEN("it is turned on and shows channel #1") {
                CHECK(tv.IsTurnedOn());
                CHECK(tv.GetChannel() == 1);

                // А когда его выключают,
                AND_WHEN("it is turned off") {
                    tv.TurnOff();

                    // то он выключается и не показывает никаких каналов
                    THEN("it is turned off and doesn't show any channel") {
                        CHECK(!tv.IsTurnedOn());
                        CHECK(tv.GetChannel() == std::nullopt);
                    }
                }
            }
            // И затем может выбирать канал с 1 по 99
            AND_THEN("it can select channel from 1 to 99") {
                WHEN("choose same channel do nothing") {
                    tv.SelectChannel(1);
                    CHECK(tv.GetChannel() == 1);
                }

                WHEN("choose incorrect channel") {
                    THEN("throw an exeption \"std::out_of_range\"") {
                        WHEN("choose channel less than 1") {
                            REQUIRE_THROWS_AS(tv.SelectChannel(0), std::out_of_range);
                        }
                        AND_WHEN("choose channel less than 99") {
                            REQUIRE_THROWS_AS(tv.SelectChannel(100), std::out_of_range);
                        }
                    }
                }

                THEN("select three channels in a row: 2, 1 and 99") {
                    tv.SelectChannel(2);
                    CHECK(tv.GetChannel() == 2);

                    tv.SelectChannel(1);
                    CHECK(tv.GetChannel() == 1);

                    tv.SelectChannel(99);
                    CHECK(tv.GetChannel() == 99);

                    WHEN("push last viewed channel") {
                        THEN("go back to 1 we chose before the last") {
                            tv.SelectLastViewedChannel();
                            CHECK(tv.GetChannel() == 1);

                            WHEN("push select last two times in a row") {
                                THEN("switch channels back to 99") {
                                    tv.SelectLastViewedChannel();
                                    CHECK(tv.GetChannel() == 99);
                                }
                            }

                            WHEN("choose new channel after select last") {
                                tv.SelectChannel(55);
                                AND_WHEN("push select last") {
                                    tv.SelectLastViewedChannel();
                                    THEN("switch back to 1") {
                                        CHECK(tv.GetChannel() == 1);
                                    }
                                }
                            }
                        }

                    }
                }
            }
            /* Реализуйте самостоятельно остальные тесты */
        }

        WHEN("it is turned on and ") {

        }
    }
}
