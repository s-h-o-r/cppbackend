#include "urldecode.h"

#include <charconv>
#include <stdexcept>
#include <iostream>
#include <iomanip>

static std::string DecodeUrlSpaces(std::string_view sub_uri) {
    std::string encoded_sub_uri;

    size_t pos = 0;
    while (pos != std::string_view::npos) {
        size_t space_pos = sub_uri.find_first_of('+', pos);
        if (space_pos != std::string_view::npos) {
            encoded_sub_uri += std::string{sub_uri.substr(pos, space_pos - pos)};
            encoded_sub_uri += ' ';
            pos = space_pos + 1;
        } else {
            encoded_sub_uri += std::string{sub_uri.substr(pos)};
            pos = space_pos;
        }
    }
    return encoded_sub_uri;
}

std::string UrlDecode(std::string_view str) {
    std::string decoded_url;

    size_t pos = 0;
    while (pos != std::string_view::npos) {
        size_t decoding_pos = str.find_first_of('%', pos);

        if (decoding_pos != std::string_view::npos && str.size() - pos < 3) {
            throw std::invalid_argument("Cannot decode URL, invalid coding");
        }

        if (decoding_pos != std::string_view::npos) {
            std::string decoding_sumbols{str.data() + decoding_pos + 1, str.data() + decoding_pos + 3};

            if (str.size() - decoding_pos < 2) {
                throw std::invalid_argument("Cannot decode URL, invalid coding");
            } else if (decoding_sumbols[0] == '0') {
                decoding_sumbols.erase(decoding_sumbols.begin());
            }

            decoded_url += DecodeUrlSpaces(str.substr(pos, decoding_pos - pos));

            char decoded_symbol;
            auto [ptr, ec] = std::from_chars(decoding_sumbols.data(), decoding_sumbols.data() + decoding_sumbols.size(),
                                             decoded_symbol, 16);

            if (ec == std::errc::invalid_argument
                || ec == std::errc::result_out_of_range
                || std::strlen(ptr) > 0) {
                throw std::invalid_argument("Cannot decode URL, invalid coding");
            }

            decoded_url += decoded_symbol;

            pos = decoding_pos + 3;
        } else {
            decoded_url += DecodeUrlSpaces(str.substr(pos));
            pos = decoding_pos;
        }
    }
    
    return decoded_url;
}
