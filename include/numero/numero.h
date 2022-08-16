#ifndef NUMERO_NUMERO_H
#define NUMERO_NUMERO_H

#include <string>
#include <string_view>

namespace num
{
    /*
     * Types of numeral naming systems.
     */
    enum class naming_system_t
    {
        undefined = 0,
        short_scale,
        long_scale
    };

    /*
     * Options used to guide conversion between numbers and numerals.
     */
    struct conversion_options_t
    {
        naming_system_t naming_system = naming_system_t::short_scale;
        std::string_view language = "en-us";
        bool debug_output = false;
        bool use_scientific_notation = false;
        bool use_thousands_separators = true;
        bool force_leading_zero = true;
        char thousands_separator_symbol = ',';
        char decimal_separator_symbol = '.';
    };

    bool is_numeral(const std::string_view &input);
    bool is_number(const std::string_view &input, const conversion_options_t &conversion_options);

    std::string to_number(const std::string_view &numeral, const conversion_options_t &conversion_options);
    std::string to_numeral(const std::string_view &number, const conversion_options_t &conversion_options);
    std::string convert(const std::string_view &input, const conversion_options_t &conversion_options);
};

#endif //NUMERO_NUMERO_H
