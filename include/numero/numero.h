#ifndef NUMERO_NUMERO_H
#define NUMERO_NUMERO_H

#include <string>
#include <string_view>
#include <regex>

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

    class converter_c
    {
    public:
        converter_c();
        converter_c(const conversion_options_t &conversion_options);

        bool is_numeral(const std::string_view &input);
        bool is_number(const std::string_view &input);

        std::string to_number(const std::string_view &numeral);
        std::string to_numeral(const std::string_view &number);
        std::string convert(const std::string_view &input);

        inline conversion_options_t &conversion_options() {
            return _conversion_options;
        }

        inline const conversion_options_t &conversion_options() const {
            return _conversion_options;
        }

    private:
        bool extract_number_parts(const std::string_view &input, bool &out_negative, std::string &out_integral_part,
                                  std::string &out_fractional_part, int32_t &out_exponent,
                                  bool resolve_exponent = true);

    private:
        conversion_options_t _conversion_options;
        const std::string _number_pattern_string;
        const std::regex _number_pattern;
        const std::regex _numeral_pattern;
    };
};

#endif //NUMERO_NUMERO_H
