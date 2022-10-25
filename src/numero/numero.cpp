#include <iostream>
#include <stdexcept>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <sstream>
#include <regex>
#include <limits>

#include <boost/bimap.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "numero/numero.h"

namespace num
{
    template <typename L, typename R>
    boost::bimap<L, R> make_bimap(std::initializer_list<typename boost::bimap<L, R>::value_type> list)
    {
        return boost::bimap<L, R>(list.begin(), list.end());
    }
    
    static inline void ltrim(std::string &s)
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
    }

    static inline void rtrim(std::string &s)
    {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    }

    static inline void trim(std::string &s)
    {
        ltrim(s);
        rtrim(s);
    }

    /*
     * The following are the distinctly named Latin prefixes used in standard dictionary numbers. Together with a latin
     * root and the common Latin suffix "-illion" or "-illiard" they form a standard dictionary number.
     * Entry form: [value] <=> [prefix]
     * Example of a standard dictionary number: trevigintillion (23-illion) => short scale: 10^(3*23+3), long scale:
     * 10^(6*23)
     */
    const auto value_to_prefix = make_bimap<int, std::string_view>({
        { 1, "un" },
        { 2, "duo" },
        { 3, "tre" },
        { 4, "quattuor" },
        { 5, "quin" },
        { 6, "sex" },
        { 7, "septen" },
        { 8, "octo" },
        { 9, "novem" },
    });
    
    /*
     * The following are distinctly named Latin roots used in standard dictionary numbers. They are stored without the
     * Latin suffixes "-illion" and "-illiard".
     * The "-illion" suffixes follow the formula 10^(3*x+3) in short scale and 10^(6*x) in long scale, where x is the
     * factor given by the key. The "-illiard" suffixes follow the formula 10^(6*x+3) in long scale.
     * The biggest number that can be converted to a short scale numeral is 10^304 - 1. The biggest number that can be
     * converted to a long scale numeral is 10^601 - 1. That numeral begins with "nine hundred ninety nine centillion
     * nine hundred ninety nine novemnonagintillion nine hundred ninety nine octononagintillion nine hundred ninety...".
     * The smallest number equals the biggest number, only that it begins with a minus sign/the word "negative".
     */
    const auto factor_to_root = make_bimap<int, std::string_view>({
        {   1, "m" },
        {   2, "b" },
        {   3, "tr" },
        {   4, "quadr" },
        {   5, "quint" },
        {   6, "sext" },
        {   7, "sept" },
        {   8, "oct" },
        {   9, "non" },
        {  10, "dec" },
        {  20, "vigint" },
        {  30, "trigint" },
        {  40, "quadragint" },
        {  50, "quinquagint" },
        {  60, "sexagint" },
        {  70, "septuagint" },
        {  80, "octogint" },
        {  90, "nonagint" },
        { 100, "cent" },
    });

    /*
     * The following are distinctly named English base numerals and their number value as a string.
     */
    const auto value_to_term = make_bimap<std::string_view, std::string_view>({
        {  "0", "zero" },
        {  "1", "one" },
        {  "2", "two" },
        {  "3", "three" },
        {  "4", "four" },
        {  "5", "five" },
        {  "6", "six" },
        {  "7", "seven" },
        {  "8", "eight" },
        {  "9", "nine" },
        { "10", "ten" },
        { "11", "eleven" },
        { "12", "twelve" },
        { "13", "thirteen" },
        { "14", "fourteen" },
        { "15", "fifteen" },
        { "16", "sixteen" },
        { "17", "seventeen" },
        { "18", "eighteen" },
        { "19", "nineteen" },
        { "20", "twenty" },
        { "30", "thirty" },
        { "40", "fourty" },
        { "50", "fifty" },
        { "60", "sixty" },
        { "70", "seventy" },
        { "80", "eighty" },
        { "90", "ninety" },
    });

    /*
     * The following defines some constant multiplicative shifts that, other than -illion and -illiard don't follow a
     * special deduction rule.
     */
    const auto multiplicative_shifts = make_bimap<int, std::string_view>({
        { 2, "hundred" },
        { 3, "thousand" },
        { 4, "myriad" }
    });
    
    /*
     * Finds the prefix that the subject starts with.
     * \param subject the subject to find the prefix for.
     * \returns the iterator of a prefix-value pair in value_to_prefix.right if found; value_to_prefix.right.end() if
     * no prefix could be found.
     */
    auto find_prefix(const std::string_view &subject)
    {
        auto it = value_to_prefix.right.begin();
        for (; it != value_to_prefix.right.end(); it++)
        {
            if (subject.substr(0, it->first.size()) == it->first)
                return it;
        }
        return value_to_prefix.right.end();
    }

    /*
     * Finds the additive value for the given term.
     * \param term the term to find the additive value for.
     * \param max_allowed_digits the number of digits allowed for that term at its place in its numeral.
     * \param allow_numbers_greater_99 whether to allow numerics that are greater than 99.
     * \returns the additive value, a value greater than 0 if term is valid; 0 if the term is invalid.
     * \throws std::invalid_argument exception if the term does not resolve to an additive value.
     */
    std::string_view find_additive_value(const std::string_view &term,
                                         int max_allowed_digits,
                                         bool allow_numbers_greater_99)
    {
        static const std::regex number_pattern("\\d+");

        std::string _term = std::string(term);
        std::smatch matches;

        if (std::regex_search(_term.cbegin(), _term.cend(), matches, number_pattern))
        {
            int number;
            std::stringstream ss;
            ss << term;
            ss >> number;

            if (!allow_numbers_greater_99 && number > 99)
                throw std::invalid_argument("actual numbers in a numeral at this place must not be greater than 99");
            
            return term;
        }

        const auto term_value_pair_it = value_to_term.right.find(term);
        if (term_value_pair_it != value_to_term.right.end())
        {
            const auto value = term_value_pair_it->second;
            if (value.size() > max_allowed_digits)
            {
                const auto message = boost::format("\"%1%\" is not allowed at this place") % term;
                throw std::invalid_argument(message.str());
            }

            return value;
        }
        else
        {
            const auto message = boost::format("\"%1%\" is not a valid term") % term;
            throw std::invalid_argument(message.str());
        }

        return {};
    }

    /*
     * Finds the multiplicative shift of places dictated by the given term, e.g. the term "thousand" returns 3 as multi-
     * plying by 1,000 shifts the multiplicand 3 places to the left.
     * \param term the term to find the multiplicative shift for.
     * \returns the multiplicative shift, a value greater than 0 if term is valid; 0 if the term is invalid.
     * \throws std::invalid_argument exception if the term does not resolve to a multiplicative shift.
     */
    uint32_t find_multiplicative_shift(const std::string_view &term, const conversion_options_t &conversion_options)
    {
        static const std::regex latin_root_pattern("(.*)(illion|illiard)$");

        std::string _term = std::string(term);
        std::smatch matches;

        if (std::regex_search(_term.cbegin(), _term.cend(), matches, latin_root_pattern))
        {
            const auto &root_base = matches[1].str();
            const auto &root_suffix = matches[2].str();

            if (conversion_options.naming_system != naming_system_t::long_scale && root_suffix == "illiard")
                throw std::invalid_argument("using long scale terms but number naming system is not set to long scale");

            auto short_scale_shift = [&](const int root_factor) {
                return 3 * root_factor + 3;
            };

            auto long_scale_shift = [&](const int root_factor) {
                return root_suffix == "illiard" ? 6 * root_factor + 3 : 6 * root_factor;
            };

            std::function<int(const int)> scale_shift;
            if (conversion_options.naming_system == naming_system_t::long_scale)
                scale_shift = long_scale_shift;
            else
                scale_shift = short_scale_shift;

            const auto factor_it = factor_to_root.right.find(root_base);
            if (factor_it != factor_to_root.right.end())
            {
                const auto root_factor = factor_it->second;
                return scale_shift(root_factor);
            }
            else
            {
                const auto prefix_value_pair_it = find_prefix(root_base);
                if (prefix_value_pair_it != value_to_prefix.right.end())
                {
                    const auto actual_prefix = prefix_value_pair_it->first;
                    const auto actual_root = root_base.substr(actual_prefix.size());
                    
                    const auto factor_it = factor_to_root.right.find(actual_root);
                    if (factor_it != factor_to_root.right.end())
                    {
                        const auto root_factor = factor_it->second + prefix_value_pair_it->second;
                        return scale_shift(root_factor);
                    }
                    // R-007: Verify valid terms in numeral.
                    else
                    {
                        const auto message = boost::format("\"%1%\" is not a valid root term") % actual_root;
                        throw std::invalid_argument(message.str());
                    }
                }
                // R-007: Verify valid terms in numeral.
                else
                {
                    const auto message = boost::format("\"%1%\" is not a valid root term") % root_base;
                    throw std::invalid_argument(message.str());
                }
            }
        }
        else
        {
            const auto multiplicative_shift_it = multiplicative_shifts.right.find(term);
            if (multiplicative_shift_it != multiplicative_shifts.right.end())
            {
                return multiplicative_shift_it->second;
            }
            else
            {
                const auto message = boost::format("\"%1%\" is not a valid term") % term;
                throw std::invalid_argument(message.str());
            }
        }

        return 0;
    }

    void merge_places(const std::string &source, std::string &target)
    {
        if (target.empty())
        {
            target = source;
            return;
        }

        const auto original_target = std::string(target);
        auto s = source.rbegin();
        auto t = target.rbegin();
        
        for (int place = 1; s != source.rend() && t != target.rend(); s++, t++, place++)
        {
            if (*s != '0' && *t != '0')
                throw std::logic_error("sub numerals overlap the same place and cannot be merged");
            else if (*s != '0')
                *t = *s;
        }
        
        if (s != source.rend() && t == target.rend())
            target.insert(target.begin(), source.begin(), s.base());
    }

    void shift_places(const uint32_t places_count, std::string &target)
    {
        target.insert(target.end(), places_count, '0');
    }

    void add_thousands_separators(std::string &target, const char thousands_separator_symbol)
    {
        if (target.find(thousands_separator_symbol) != std::string::npos)
            return;
        
        std::stringstream ss;
        const auto offset = target.size() % 3;
        
        for (std::size_t i = 0; i < target.size(); i++)
        {
            if (i > 0 && i % 3 == offset) ss << thousands_separator_symbol;
            ss << target[i];
        }

        target = ss.str();
    }

    void strip_thousands_separators(std::string &target, const char thousands_separator_symbol)
    {
        boost::replace_all(target, std::string(1, thousands_separator_symbol), "");
    }

    std::string parse_integral_number(const std::string_view &integral, const conversion_options_t &conversion_options)
    {
        static const std::regex split_pattern("[\\s-]+");

        const auto throw_duplicate_sub_numeral_magnitudes = [](const std::string &first_sub_numeral,
                                                               const std::string &second_sub_numeral)
        {
            const auto message = boost::format("there must not be multiple sub numerals with the same magnitude: "
                                               "\"%1%\" and \"%2%\".") % first_sub_numeral % second_sub_numeral;
            throw std::invalid_argument(message.str());
        };

        const auto throw_incorrect_sub_numeral_order = [](const std::string &lower_magnitude_sub_numeral,
                                                          const std::string &higher_magnitude_sub_numeral)
        {
            const auto message = boost::format("a higher magnitude sub numeral is not allowed to follow a "
                                               "lower magnitude sub numeral: \"%1%\" follows \"%2%\". "
                                               "Did you mean \"%1% %2%\"?")
                                               % higher_magnitude_sub_numeral % lower_magnitude_sub_numeral;
            throw std::invalid_argument(message.str());
        };

        if (integral.empty())
            return {};

        bool negative = false;
        std::string _integral = std::string(integral);

        auto it = std::sregex_token_iterator(_integral.begin(), _integral.end(), split_pattern, -1);
        
        std::vector<std::string> groups;
        std::set<uint32_t> used_shifts;

        std::string current_group;
        std::string last_term;
        std::string last_sub_numeral;
        std::string current_sub_numeral;
        std::string last_additive_value;
        
        uint32_t last_multiplicative_shift = 0;
        uint32_t last_group_total_multiplicative_shift = std::numeric_limits<uint32_t>::max();
        uint32_t current_group_total_multiplicative_shift = 0;
        bool last_term_multiplicative = false;

        for (; it != std::sregex_token_iterator(); it++)
        {
            const auto match = *it;
            const auto term = match.str();
            
            if (groups.empty() && current_group.empty())
            {
                current_sub_numeral = term;
                
                if (term == "a")
                {
                    current_group = "1";
                    last_term = term;
                    continue;
                }
                else if (term == "negative" || term == "minus")
                {
                    negative = true;
                    last_term = term;
                    continue;
                }
            }
            
            std::exception_ptr find_additive_value_exception = nullptr;
            std::exception_ptr find_multiplicative_shift_exception = nullptr;

            std::string current_additive_value;
            uint32_t current_multiplicative_shift = 0;

            try {
                current_additive_value = find_additive_value(term, 3, groups.empty() && current_group.empty());
            } catch (const std::exception &e) {
                find_additive_value_exception = std::current_exception();
            }

            try {
                current_multiplicative_shift = find_multiplicative_shift(term, conversion_options);
            } catch (const std::exception &e) {
                find_multiplicative_shift_exception = std::current_exception();
            }

            // If the term is neither additive nor multiplicative, rethrow first exception and tell the term is unknown.
            if (find_additive_value_exception && find_multiplicative_shift_exception)
                std::rethrow_exception(find_additive_value_exception);

            // Process additive term.
            if (!find_additive_value_exception)
            {
                if (last_term_multiplicative && last_multiplicative_shift >= 3)
                {
                    if (current_group_total_multiplicative_shift == last_group_total_multiplicative_shift)
                        throw_duplicate_sub_numeral_magnitudes(last_sub_numeral, current_sub_numeral);
                    else if (current_group_total_multiplicative_shift > last_group_total_multiplicative_shift)
                        throw_incorrect_sub_numeral_order(last_sub_numeral, current_sub_numeral);
                    
                    groups.push_back(current_group);
                    
                    if (conversion_options.debug_output)
                    {
                        std::cout << "Group number: " << current_group << "\n";
                        std::cout << "New group" << "\n";
                    }
                    
                    current_group = "";
                    last_sub_numeral = current_sub_numeral;
                    current_sub_numeral = term;
                    last_group_total_multiplicative_shift = current_group_total_multiplicative_shift;
                    current_group_total_multiplicative_shift = 0;
                    last_multiplicative_shift = 0;
                }

                if (conversion_options.debug_output)
                {
                    std::cout << "Term: " << term << "\n";
                    std::cout << "  Additive value: " << current_additive_value << "\n";
                }
                
                last_term_multiplicative = false;

                if (!last_additive_value.empty() && last_additive_value.size() < current_additive_value.size())
                {
                    const auto message = boost::format("greater value terms have to precede lower value terms. "
                                                       "Did you mean \"%1% %2%\"?") % term % last_term;
                    throw std::invalid_argument(message.str());
                }
                
                merge_places(current_additive_value, current_group);

                last_additive_value = current_additive_value;
            }
            // Process multiplicative term.
            else
            {
                if (current_multiplicative_shift < last_multiplicative_shift)
                {
                    const auto message = boost::format("a lower multiplicative term is not allowed to follow a "
                                                       "higher multiplicative term: \"%1% %2%\". "
                                                       "Did you mean \"%2% %1%\" or did you forget an additive term "
                                                       "in front of \"%2%\"?") % last_term % term;
                    throw std::invalid_argument(message.str());
                }
                
                // Add an implicit 1 if that is missing at the beginning of the numeral.
                if (groups.empty() && current_group.empty())
                    current_group = "1";
                
                if (current_group == "0")
                    throw std::invalid_argument("in the integral part \"zero\" is only allowed on its own.");
                
                if (conversion_options.debug_output)
                {
                    std::cout << "Term: " << term << "\n";
                    std::cout << "  Multiplicative value: 10^" << current_multiplicative_shift << "\n";
                }
                
                current_sub_numeral += " " + term;
                last_term_multiplicative = true;
                last_multiplicative_shift = current_multiplicative_shift;
                current_group_total_multiplicative_shift += current_multiplicative_shift;

                shift_places(current_multiplicative_shift, current_group);

                last_additive_value.clear();
            }
            
            last_term = term;
        }

        if (groups.empty() && current_group.empty() && negative)
            throw std::invalid_argument("the numeral must not be empty");

        if (current_group_total_multiplicative_shift == last_group_total_multiplicative_shift)
            throw_duplicate_sub_numeral_magnitudes(last_sub_numeral, current_sub_numeral);
        else if (current_group_total_multiplicative_shift > last_group_total_multiplicative_shift)
            throw_incorrect_sub_numeral_order(last_sub_numeral, current_sub_numeral);

        groups.push_back(current_group);

        std::string result;

        for (const auto &group : groups)
            merge_places(group, result);
        
        if (conversion_options.use_thousands_separators)
            add_thousands_separators(result, conversion_options.thousands_separator_symbol);

        if (negative)
            result.insert(0, 1, '-');

        return result;
    }

    std::string parse_fractional_number(const std::string_view &fractional,
                                        const conversion_options_t &conversion_options)
    {
        static const std::regex split_pattern("[\\s-]+");

        if (fractional.empty())
            return {};

        std::stringstream ss;
        std::string _fractional = std::string(fractional);

        auto it = std::sregex_token_iterator(_fractional.begin(), _fractional.end(), split_pattern, -1);

        for (; it != std::sregex_token_iterator(); it++)
        {
            const auto match = *it;
            const auto &digit = match.str();
            ss << find_additive_value(digit, 1, true);
        }

        return ss.str();
    }

    std::string converter_c::to_number(const std::string_view &numeral)
    {
        static const std::regex split_pattern(" ?point ");

        if (numeral.empty())
            throw std::invalid_argument("the numeral must not be empty");
        
        if (!is_numeral(numeral))
            throw std::invalid_argument("the numeral is invalid");
        
        std::string _numeral = std::string(numeral);
        std::vector<std::string> parts;

        auto it = std::sregex_token_iterator(_numeral.begin(), _numeral.end(), split_pattern, -1);

        for (; it != std::sregex_token_iterator(); it++)
        {
            const auto match = *it;
            const auto part = match.str();
            parts.push_back(part);
        }

        if (parts.size() >= 3)
            throw std::logic_error("\"point\" is only allowed once in a numeral as a decimal separator");

        auto number = parse_integral_number(parts[0], _conversion_options);

        if (parts.size() == 2)
        {
            const auto parsed_fractional = parse_fractional_number(parts[1], _conversion_options);

            if (number.empty())
                number = "0";

            number.insert(number.end(), _conversion_options.decimal_separator_symbol);

            if (parsed_fractional.empty())
                number += "0";
            else
                number += parsed_fractional;
        }

        return number;
    }

    /*
     * Checks whether the given input is likely a numeral. Attention: You are better off checking whether the given
     * input is a valid number before, because numerals also allow simple positive numbers that have no thousands
     * separators, no decimal separator and no exponent (i.e. scientific notation).
     *
     * \param input The input to be checked.
     * \returns True if the input likely represents a valid numeral, false otherwise.
     */
    bool converter_c::is_numeral(const std::string_view &input)
    {
        return std::regex_match(std::string(input), _numeral_pattern) && input != "negative" && input != "minus";
    }

    /*
     * Checks whether the given input is a number. A number in this sense may lead with an optional minus sign, it is 
     * then followed either an integral part, a fractional part or both. At the end there may be an optional exponent
     * specified. The integral part may group each three digits beginning at the right, where the groups are separated
     * by the thousands separator symbol given in the conversion options. If both integral and fractional part are given
     * they are separated by the decimal separator symbol that is also given in the conversion options.
     * 
     * Examples of valid numbers:
     *   1
     *   -1.0625
     *   .75
     *   1,025,000
     *   3.85e9
     *
     * \param input The input to be checked.
     * \returns True if the input is a valid number, false otherwise.
     */
    bool converter_c::is_number(const std::string_view &input)
    {
        bool negative;
        std::string integral_part, fractional_part;
        int32_t exponent;
        return extract_number_parts(std::string(input), negative, integral_part, fractional_part, exponent);
    }
    
    /*
     * Extracts a decimal number, either integer or floating-point, either in scientific notation or not, from the given
     * input string. It uses the thousands and decimal separator symbols given in the conversion options. If input
     * represents a valid number, the integral part of the number is written to out_integral_part, the fractional part
     * of the number is written to out_fractional_part and the exponent is written to out_exponent. If the number is
     * negative, out_negative will be set to true. Out parameters will only be set if the function returns true, i.e.
     * the input represents a valid number.
     * 
     * Examples of valid numbers:
     *   1
     *   -1.0625
     *   .75
     *   1,025,000
     *   3.85e9
     *
     * \param input The input representing the number to be extracted.
     * \param out_negative A boolean that receives true if the number is negative.
     * \param out_integral_part A string that receives the integral part of the number (if any) without thousands
     *   separators.
     * \param out_fractional_part A string that receives the fractional part of the number (if any).
     * \param out_exponent An integer that receives the exponent (power) of the number.
     * \param resolve_exponent Whether the decimal point shall be moved according to the number's exponent.
     * \returns True if the input represents a valid number, false otherwise.
     * \throws std::invalid_argument exception (see std::stoi).
     * \throws std::out_of_range exception (see std::stoi).
     */
    bool converter_c::extract_number_parts(const std::string_view &input, bool &out_negative,
                                           std::string &out_integral_part, std::string &out_fractional_part,
                                           int32_t &out_exponent, bool resolve_exponent)
    {
        enum indices { SIGN = 1, INTEGRAL, FRACTIONAL, EXPONENT };

        const auto &number_pattern = get_number_pattern_regex();
        std::smatch matches;
        std::string _input = std::string(input);

        if (std::regex_search(_input.cbegin(), _input.cend(), matches, number_pattern))
        {
            const auto is_negative = matches[SIGN].matched;
            const auto has_integral_part = matches[INTEGRAL].matched;
            const auto has_fractional_part = matches[FRACTIONAL].matched;
            const auto has_exponent = matches[EXPONENT].matched;
            
            if (!has_integral_part && !has_fractional_part)
                return false;
                
            std::string integral_part = has_integral_part ? matches[INTEGRAL].str() : "";
            std::string fractional_part = has_fractional_part ? matches[FRACTIONAL].str() : "";
            auto exponent = has_exponent ? std::stoi(matches[EXPONENT].str()) : 0;

            strip_thousands_separators(integral_part, _conversion_options.thousands_separator_symbol);

            if (resolve_exponent && exponent != 0)
            {
                const auto integral_part_size = static_cast<int>(integral_part.size());
                const auto fractional_part_size = static_cast<int>(fractional_part.size());
                const auto full_number_size = integral_part_size + fractional_part_size;
                const auto decimal_separator_position = integral_part_size + exponent;
                const auto offset = decimal_separator_position - full_number_size;

                std::string full_number = integral_part + fractional_part;

                // Append zeros.
                if (offset > 0)
                {
                    for (int i = 0; i < offset; i++)
                        full_number += '0';
                    integral_part = full_number;
                    fractional_part.erase();
                }
                // Prepend zeros.
                else if (decimal_separator_position < 0)
                {
                    for (int i = 0; i < -decimal_separator_position; i++)
                        full_number.insert(full_number.begin(), '0');
                    if (_conversion_options.force_leading_zero)
                        integral_part = "0";
                    else
                        integral_part.erase();
                    fractional_part = full_number;
                }
                // Move decimal separator within the digits.
                else
                {
                    integral_part = full_number.substr(0, decimal_separator_position);
                    fractional_part = full_number.substr(decimal_separator_position);
                }
            }

            out_negative = is_negative;
            out_integral_part = integral_part;
            out_fractional_part = fractional_part;
            out_exponent = exponent;
            
            return true;
        }
        
        return false;
    }

    std::string parse_integral_numeral(const std::string_view &integral, const conversion_options_t &conversion_options)
    {
        if (integral.empty())
            return {};

        std::stringstream ss;
        std::size_t place = integral.size() - 1;
        auto it = integral.begin();

        auto group_digits = std::string();
        bool any_group_digit_not_zero = false;
        bool added_two_digits_term = false;

        for (; it != integral.end(); it++, place--)
        {
            const auto digit = *it;
            const auto group_place = place % 3;

            if (group_place == 2)
            {
                group_digits.clear();
                any_group_digit_not_zero = false;
                added_two_digits_term = false;
            }

            group_digits += digit;
            any_group_digit_not_zero |= digit != '0';

            //std::cout << digit << " : " << group_place << " : " << place << "\n";

            auto value = std::string();
            if (digit != '0' || integral == "0") value += digit;

            if (digit != '0' && group_place == 1)
            {
                // Get next digit as well because we need that for 2-digit base terms (e.g. thirteen or fourty)
                const auto next_digit = *std::next(it);
                value += next_digit;
            }

            // Encode actual number term.
            if (!value.empty() && !added_two_digits_term)
            {
                const auto value_term_pair_it = value_to_term.left.find(value);
                if (value_term_pair_it != value_to_term.left.end())
                {
                    ss << value_term_pair_it->second << " ";
                    added_two_digits_term |= value.size() == 2;
                }
                else if (value.size() == 2)
                {
                    value[1] = '0';
                    const auto value_term_pair_it = value_to_term.left.find(value);
                    if (value_term_pair_it != value_to_term.left.end())
                    {
                        ss << value_term_pair_it->second << "-";
                    }
                    else
                    {
                        const auto message = boost::format("unable to resolve term for value \"%1%\"") % value;
                        throw std::logic_error(message.str());
                    }
                }
                else
                {
                    const auto message = boost::format("unable to resolve term for value \"%1%\"") % value;
                    throw std::logic_error(message.str());
                }
            }

            // Encode an "-illion" or "-illiard" term.
            if (any_group_digit_not_zero && place >= 6 && group_place == 0)
            {
                const auto factor = conversion_options.naming_system == naming_system_t::short_scale ?
                                    (place - 3) / 3 : place / 6;
                const auto remainder = conversion_options.naming_system == naming_system_t::short_scale ?
                                       0 : place % 6;

                if (factor > 100)
                    throw std::logic_error("latin roots greater than \"centillion\" are not supported");

                const auto &factor_root_pair_it = factor_to_root.left.find(factor);
                if (factor_root_pair_it != factor_to_root.left.end())
                {
                    ss << factor_root_pair_it->second << (remainder == 3 ? "illiard " : "illion ");
                }
                else
                {
                    const auto prefix_value = factor % 10;
                    const auto &value_prefix_pair_it = value_to_prefix.left.find(prefix_value);
                    if (value_prefix_pair_it != value_to_prefix.left.end())
                    {
                        const auto base_factor = factor - prefix_value;
                        const auto &base_factor_root_pair_it = factor_to_root.left.find(base_factor);
                        if (base_factor_root_pair_it != factor_to_root.left.end())
                        {
                            ss << value_prefix_pair_it->second << base_factor_root_pair_it->second
                               << (remainder == 3 ? "illiard " : "illion ");
                        }
                        else
                        {
                            const auto message = boost::format("unable to resolve latin root for base factor %1%")
                                                               % base_factor;
                            throw std::logic_error(message.str());
                        }
                    }
                    else
                    {
                        const auto message = boost::format("unable to resolve latin prefix for value %1%")
                                                           % prefix_value;
                        throw std::logic_error(message.str());
                    }
                }
            }
            else if (any_group_digit_not_zero && place == 3)
            {
                ss << "thousand ";
            }
            else if (digit != '0' && group_place == 2)
            {
                ss << "hundred ";
            }
        }

        auto result = ss.str();
        rtrim(result);

        return result;
    }

    std::string parse_fractional_numeral(const std::string_view &fractional, const conversion_options_t &conversion_options)
    {
        if (fractional.empty())
            return {};

        std::stringstream ss;

        for (const auto digit : fractional)
        {
            const auto value = std::string(1, digit);
            const auto value_term_pair_it = value_to_term.left.find(value);
            if (value_term_pair_it != value_to_term.left.end())
            {
                ss << value_term_pair_it->second << " ";
            }
            else
            {
                const auto message = boost::format("unable to resolve term for value \"%1%\"") % value;
                throw std::logic_error(message.str());
            }
        }

        auto result = ss.str();
        rtrim(result);

        return result;
    }

    std::string converter_c::to_numeral(const std::string_view &number)
    {
        if (number.empty())
            return {};

        bool negative = false;
        std::string integral_part;
        std::string fractional_part;
        int32_t exponent = 0;

        if (!extract_number_parts(number, negative, integral_part, fractional_part, exponent))
            return {};

        std::string numeral;
        
        if (negative)
            numeral = "negative";

        if (!integral_part.empty())
        {
            const auto parsed_integral = parse_integral_numeral(integral_part, _conversion_options);
            if (!parsed_integral.empty())
            {
                if (numeral.empty() && (integral_part != "0" || _conversion_options.force_leading_zero))
                    numeral = parsed_integral;
                else if (!numeral.empty())
                    numeral += " " + parsed_integral;
            }
        }

        if (!fractional_part.empty())
        {
            const auto parsed_fractional = parse_fractional_numeral(fractional_part, _conversion_options);
            if (!parsed_fractional.empty())
            {
                if (numeral.empty())
                    numeral = "point " + parsed_fractional;
                else
                    numeral += " point " + parsed_fractional;
            }
        }

        return numeral;
    }

    std::string converter_c::convert(const std::string_view &input)
    {
        return is_number(input) ? to_numeral(input) : to_number(input);
    }

    converter_c::converter_c() :
        _numeral_pattern("^(?:[a-z]+|[0-9]+)(?:(?:[\\t ]+|-)(?:[a-z]+|[0-9]+))*$", std::regex::optimize)
    {
        // Create the initial number pattern regular expression.
        get_number_pattern_regex();
    }

    converter_c::converter_c(const conversion_options_t &conversion_options) :
        _conversion_options(conversion_options),
        _numeral_pattern("^(?:[a-z]+|[0-9]+)(?:(?:[\\t ]+|-)(?:[a-z]+|[0-9]+))*$", std::regex::optimize)
    {
        // Create the initial number pattern regular expression.
        get_number_pattern_regex();
    }

    const std::regex &converter_c::get_number_pattern_regex()
    {
        const int16_t key = _conversion_options.thousands_separator_symbol << 8 |
                            _conversion_options.decimal_separator_symbol;

        const auto number_pattern_it = _number_patterns.find(key);
        if (number_pattern_it != _number_patterns.end())
            return number_pattern_it->second;
        
        const auto pattern = std::regex(
            (boost::format("^(-)?((?:\\d{1,3}(?:\\%1%\\d{3})*)|(?:\\d+))?(?:\\%2%(\\d+))?(?:e(-?\\d+))?$")
                           % std::string(1, _conversion_options.thousands_separator_symbol)
                           % std::string(1, _conversion_options.decimal_separator_symbol)).str(),
                           !_number_patterns.empty() ? static_cast<std::regex_constants::syntax_option_type>(0) : 
                                                       std::regex::optimize);
        return (_number_patterns.insert({ key, pattern }).first)->second;
    }
}
