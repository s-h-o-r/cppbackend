#include <catch2/catch_test_macros.hpp>
#include <iostream>

#include "../src/controller.h"

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

SCENARIO("Controller", "[Controller]") {
    using namespace std::literals;
    GIVEN("Controller and TV") {
        TV tv;
        std::istringstream input;
        std::ostringstream output;
        Menu menu{input, output};
        Controller controller{tv, menu};

        auto run_menu_command = [&menu, &input](std::string command) {
            input.str(std::move(command));
            input.clear();
            menu.Run();
        };
        auto expect_output = [&output](std::string_view expected) {
            // В g++ 10.3 не реализован метод ostringstream::view(), поэтому приходится
            // использовать метод str()
            // Также в conan есть баг, из-за которого Catch2 не подхватывает поддержку string_view:
            // https://github.com/conan-io/conan-center-index/issues/13993
            // Поэтому expected преобразуется к строке, чтобы обойти ошибку компиляции
            CHECK(output.str() == std::string{expected});
        };
        auto expect_extra_arguments_error = [&expect_output](std::string_view command) {
            expect_output("Error: the "s.append(command).append(
                " command does not require any arguments\n"sv));
        };
        auto expect_empty_output = [&expect_output] {
            expect_output({});
        };

        WHEN("The TV is turned off") {
            AND_WHEN("Info command is entered without arguments") {
                run_menu_command("Info"s);

                THEN("output contains info that TV is turned off") {
                    expect_output("TV is turned off\n"s);
                }
            }

            AND_WHEN("Info command is entered with some arguments") {
                run_menu_command("Info some extra arguments");

                THEN("Error message is printed") {
                    expect_extra_arguments_error("Info"s);
                }
            }

            AND_WHEN("Info command has trailing spaces") {
                run_menu_command("Info  "s);

                THEN("output contains information that TV is turned off") {
                    expect_output("TV is turned off\n"s);
                }
            }

            AND_WHEN("TurnOn command is entered without arguments") {
                run_menu_command("TurnOn"s);

                THEN("TV is turned on") {
                    CHECK(tv.IsTurnedOn());
                    expect_empty_output();
                }
            }

            AND_WHEN("TurnOn command is entered with some arguments") {
                run_menu_command("TurnOn some args"s);

                THEN("the error message is printed and TV is not turned on") {
                    CHECK(!tv.IsTurnedOn());
                    expect_extra_arguments_error("TurnOn"s);
                }
            }
            /* Протестируйте остальные аспекты поведения класса Controller, когда TV выключен */

            AND_WHEN("SelectChannel command with wrong arguments") {
                run_menu_command("SelectChannel some args"s);

                THEN("Invalid channel is printed and TV is not turned on") {
                    CHECK(!tv.IsTurnedOn());
                    expect_output("Invalid channel\n"s);
                }
            }

            AND_WHEN("SelectChannel command without arguments") {
                run_menu_command("SelectChannel"s);

                THEN("Invalid channel is printed and TV is not turned on") {
                    CHECK(!tv.IsTurnedOn());
                    expect_output("Invalid channel\n"s);
                }
            }

            AND_WHEN("SelectChannel command with not int channel") {
                run_menu_command("SelectChannel 3.12"s);

                THEN("Invalid channel is printed and TV is not turned on") {
                    CHECK(!tv.IsTurnedOn());
                    expect_output("Invalid channel\n"s);
                }
            }

            AND_WHEN("SelectChannel command with right channel") {
                run_menu_command("SelectChannel 64"s);
                THEN("output contains info that TV is turned off") {
                    expect_output("TV is turned off\n"s);
                }
            }

            AND_WHEN("SelectPreviousChannel command with some arguments") {
                run_menu_command("SelectPreviousChannel some extra arguments");

                THEN("Error message is printed") {
                    expect_extra_arguments_error("SelectPreviousChannel"s);
                }
            }

            AND_WHEN("SelectPreviousChannel command without arguments") {
                run_menu_command("SelectPreviousChannel");

                THEN("output contains information that TV is turned off") {
                    expect_output("TV is turned off\n"s);
                }
            }
        }

        WHEN("The TV is turned on") {
            tv.TurnOn();
            AND_WHEN("TurnOff command is entered without arguments") {
                run_menu_command("TurnOff"s);

                THEN("TV is turned off") {
                    CHECK(!tv.IsTurnedOn());
                    expect_empty_output();
                }
            }

            AND_WHEN("TurnOff command is entered with some arguments") {
                run_menu_command("TurnOff some args");

                THEN("the error message is printed and TV is not turned off") {
                    CHECK(tv.IsTurnedOn());
                    expect_extra_arguments_error("TurnOff"s);
                }
            }

            AND_WHEN("Info command is entered without arguments") {
                tv.SelectChannel(12);
                run_menu_command("Info"s);

                THEN("current channel is printed") {
                    expect_output("TV is turned on\nChannel number is 12\n"s);
                }
            }

            AND_WHEN("SelectChannel command is entered without arguments") {
                run_menu_command("SelectChannel"s);

                THEN("the error message is printed") {
                    expect_output("Invalid channel\n");
                }
            }

            AND_WHEN("SelectChannel command is entered with right argument") {
                run_menu_command("SelectChannel 34"s);

                THEN("tv change the channel and tv prints info with new channel") {
                    expect_empty_output();
                    run_menu_command("Info"s);
                    expect_output("TV is turned on\nChannel number is 34\n");
                }
            }

            AND_WHEN("SelectChannel command is entered with right out of range channel") {
                run_menu_command("SelectChannel 3100"s);

                THEN("expect error message that channel out of range") {
                    CHECK(tv.IsTurnedOn());
                    expect_output("Channel is out of range\n"s);
                }
            }

            AND_WHEN("SelectChannel command is entered with too large channel") {
                run_menu_command("SelectChannel 5000000000000000000000000000000000000000"s);

                THEN("expect error message Invalid channel") {
                    CHECK(tv.IsTurnedOn());
                    expect_output("Invalid channel\n"s);
                }
            }

            AND_WHEN("SelectPreviousChannel command is entered with arguments") {
                run_menu_command("SelectPreviousChannel with args"s);

                THEN("the error message is printed") {
                    expect_extra_arguments_error("SelectPreviousChannel"s);
                }
            }

            AND_WHEN("choose new channel and SelectPreviousChannel command is entered without arguments") {
                tv.SelectChannel(38);
                run_menu_command("SelectPreviousChannel"s);

                THEN("empty output is printed") {
                    expect_empty_output();
                }

                AND_WHEN("check channel") {
                    THEN("it should change to precious (previous is basic - 1)") {
                        CHECK(tv.GetChannel() == 1);
                    }
                }

                AND_WHEN("enter SelectPreviousChannel again") {
                    run_menu_command("SelectPreviousChannel"s);

                    THEN("channel switches to 38 chose before") {
                        CHECK(tv.GetChannel() == 38);
                    }
                }
            }

            /* Протестируйте остальные аспекты поведения класса Controller, когда TV включен */
        }
    }
}
