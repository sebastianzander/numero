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
        char thousands_separator_symbol = ',';
        char decimal_separator_symbol = '.';
    };

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
    uint32_t find_multiplicative_shift(const std::string_view &term)
    {
        static const std::regex latin_root_pattern("(.*)(illion|illiard)$");

        std::string _term = std::string(term);
        std::smatch matches;

        if (std::regex_search(_term.cbegin(), _term.cend(), matches, latin_root_pattern))
        {
            const auto &root_base = matches[1].str();
            const auto &root_suffix = matches[2].str();

            if (root_suffix == "illiard")
                throw std::invalid_argument("long scale number system is currently not supported");

            const auto factor_it = factor_to_root.right.find(root_base);
            if (factor_it != factor_to_root.right.end())
            {
                const auto root_factor = factor_it->second;
                return 3 * root_factor + 3;
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
                        return 3 * root_factor + 3;
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
        std::stringstream ss;
        const auto offset = target.size() % 3;
        
        for (std::size_t i = 0; i < target.size(); i++)
        {
            if (i > 0 && i % 3 == offset) ss << thousands_separator_symbol;
            ss << target[i];
        }

        target = ss.str();
    }

    std::string parse_integer_number(const std::string_view &integer, const conversion_options_t &conversion_options)
    {
        static const std::regex split_pattern("[\\s-]+");

        if (integer.empty())
            return {};

        bool negative = false;
        std::string _numeral = std::string(integer);

        auto it = std::sregex_token_iterator(_numeral.begin(), _numeral.end(), split_pattern, -1);
        
        std::vector<std::string> groups;
        std::string current_group;
        uint32_t last_multiplicative_shift = 1;
        bool last_term_multiplicative = false;

        for (; it != std::sregex_token_iterator(); it++)
        {
            const auto match = *it;
            const auto term = match.str();
            
            if (groups.empty() && current_group.empty() && (term == "negative" || term == "minus"))
            {
                negative = true;
                continue;
            }

            if (groups.empty() && current_group.empty() && term == "a")
            {
                current_group = "1";
                continue;
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
                current_multiplicative_shift = find_multiplicative_shift(term);
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
                    groups.push_back(current_group);
                    
                    if (conversion_options.debug_output)
                    {
                        std::cout << "Group number: " << current_group << std::endl;
                        std::cout << "New group" << std::endl;
                    }
                    
                    current_group = "";
                }

                if (conversion_options.debug_output)
                {
                    std::cout << "Term: " << term << std::endl;
                    std::cout << "  Additive value: " << current_additive_value << std::endl;
                }
                
                last_term_multiplicative = false;

                merge_places(current_additive_value, current_group);
            }
            // Process multiplicative term.
            else
            {
                // Add an implicit 1 if that is missing at the beginning of the numeral.
                if (groups.empty() && current_group.empty())
                    current_group = "1";
                
                if (conversion_options.debug_output)
                {
                    std::cout << "Term: " << term << std::endl;
                    std::cout << "  Multiplicative value: 10^" << current_multiplicative_shift << std::endl;
                }
                
                last_term_multiplicative = true;
                last_multiplicative_shift = current_multiplicative_shift;

                shift_places(current_multiplicative_shift, current_group);
            }
        }

        if (groups.empty() && current_group.empty() && negative)
            throw std::invalid_argument("the numeral must not be empty");

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

    std::string parse_decimal_number(const std::string_view &decimal, const conversion_options_t &conversion_options)
    {
        static const std::regex split_pattern("[\\s-]+");

        if (decimal.empty())
            return {};

        std::stringstream ss;
        std::string _decimal = std::string(decimal);

        auto it = std::sregex_token_iterator(_decimal.begin(), _decimal.end(), split_pattern, -1);

        for (; it != std::sregex_token_iterator(); it++)
        {
            const auto match = *it;
            const auto &digit = match.str();
            ss << find_additive_value(digit, 1, true);
        }

        return ss.str();
    }

    std::string to_number(const std::string_view &numeral, const conversion_options_t &conversion_options)
    {
        static const std::regex split_pattern(" ?point ");

        if (numeral.empty())
            throw std::invalid_argument("the numeral must not be empty");
        
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

        auto number = parse_integer_number(parts[0], conversion_options);

        if (parts.size() == 2)
        {
            const auto parsed_decimal = parse_decimal_number(parts[1], conversion_options);

            if (number.empty())
                number = "0";

            number.insert(number.end(), conversion_options.decimal_separator_symbol);

            if (parsed_decimal.empty())
                number += "0";
            else
                number += parsed_decimal;
        }

        return number;
    }

    bool is_number(const std::string &input, const conversion_options_t &conversion_options)
    {
        const auto number_pattern_string = 
            boost::format("^-?\\d{1,3}(?:\\%1%\\d{3})*(?:\\%2%\\d+)?$|^\\d+(?:\\%2%)?\\d+$")
                          % std::string(1, conversion_options.thousands_separator_symbol)
                          % std::string(1, conversion_options.decimal_separator_symbol);

        const std::regex number_pattern(number_pattern_string.str());
        return std::regex_match(input, number_pattern);
    }

    bool is_number(const std::string_view &input, const conversion_options_t &conversion_options)
    {
        return is_number(std::string(input), conversion_options);
    }

    std::string parse_integer_numeral(const std::string_view &integer, const conversion_options_t &conversion_options)
    {
        if (integer.empty())
            return {};

        std::stringstream ss;
        std::size_t place = integer.size() - 1;
        auto it = integer.begin();

        if (integer[0] == '-')
        {
            it++;
            place--;
            ss << "negative ";
        }

        auto group_digits = std::string();
        bool any_group_digit_not_zero = false;
        bool added_two_digits_term = false;

        for (; it != integer.end(); it++, place--)
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

            //std::cout << digit << " : " << group_place << " : " << place << std::endl;

            auto value = std::string();
            if (digit != '0') value += digit;

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
                const auto factor = (place - 3) / 3;

                if (factor > 100)
                    throw std::logic_error("latin roots greater than \"centillion\" are not supported");

                const auto &factor_root_pair_it = factor_to_root.left.find(factor);
                if (factor_root_pair_it != factor_to_root.left.end())
                {
                    ss << factor_root_pair_it->second << "illion ";
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
                            ss << value_prefix_pair_it->second << base_factor_root_pair_it->second << "illion ";
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

    std::string parse_decimal_numeral(const std::string_view &decimal, const conversion_options_t &conversion_options)
    {
        if (decimal.empty())
            return {};

        std::stringstream ss;

        for (const auto digit : decimal)
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

    std::string to_numeral(const std::string_view &number, const conversion_options_t &conversion_options)
    {
        const auto strip_pattern_string = 
            boost::format("[\\s\\%1%]+") % std::string(1, conversion_options.thousands_separator_symbol);
        const std::regex strip_pattern(strip_pattern_string.str());

        const auto split_pattern_string = 
            boost::format("\\%1%") % std::string(1, conversion_options.decimal_separator_symbol);
        const std::regex split_pattern(split_pattern_string.str());

        if (number.empty())
            return {};

        std::string _number = std::regex_replace(std::string(number), strip_pattern, "");
        std::vector<std::string> parts;

        auto it = std::sregex_token_iterator(_number.begin(), _number.end(), split_pattern, -1);

        for (; it != std::sregex_token_iterator(); it++)
        {
            const auto match = *it;
            const auto part = match.str();
            parts.push_back(part);
        }

        if (parts.size() >= 3)
        {
            const auto message = boost::format("decimal separator \"%1%\" is only allowed once in a number")
                                               % std::string(1, conversion_options.decimal_separator_symbol);
            throw std::logic_error(message.str());
        }

        auto numeral = parse_integer_numeral(parts[0], conversion_options);

        if (parts.size() == 2)
        {
            const auto parsed_decimal = parse_decimal_numeral(parts[1], conversion_options);

            if (numeral.empty())
                numeral = "zero";

            if (!parsed_decimal.empty())
                numeral += " point " + parsed_decimal;
        }

        return numeral;
    }

    std::string convert(const std::string_view &input, const conversion_options_t &conversion_options)
    {
        return num::is_number(input, conversion_options) ? num::to_numeral(input, conversion_options) : 
                                                           num::to_number(input, conversion_options);
    }
}

void process_program_options(const boost::program_options::variables_map &vm,
                             num::conversion_options_t &conversion_options)
{
    using namespace boost::program_options;
    
    if (vm.count("debug-output"))
        conversion_options.debug_output = vm["debug-output"].as<bool>();

    if (vm.count("naming-system"))
    {
        const auto &naming_system = vm["naming-system"].as<std::string>();
        if (naming_system == "short-scale")
            conversion_options.naming_system = num::naming_system_t::short_scale;
        else if (naming_system == "long-scale")
            conversion_options.naming_system = num::naming_system_t::long_scale;
        else
        {
            const auto message = boost::format("\"%1%\" is not a valid number naming system. "
                                               "Supported naming systems are \"short-scale\" and \"long-scale\".")
                                               % naming_system;
            throw std::logic_error(message.str());
        }
    }
    
    if (vm.count("language"))
        conversion_options.language = vm["language"].as<std::string>();
    
    if (vm.count("use-scientific-notation"))
        conversion_options.use_scientific_notation = vm["use-scientific-notation"].as<bool>();
    
    if (vm.count("use-thousands-separator"))
        conversion_options.use_thousands_separators = vm["use-thousands-separator"].as<bool>();
    
    if (vm.count("thousands-separator-symbol"))
    {
        conversion_options.thousands_separator_symbol = vm["thousands-separator-symbol"].as<char>();
        if (conversion_options.thousands_separator_symbol == '.' )
            conversion_options.decimal_separator_symbol = ',';
    }
    
    if (vm.count("decimal-separator-symbol"))
    {
        conversion_options.decimal_separator_symbol = vm["decimal-separator-symbol"].as<char>();
        if (conversion_options.decimal_separator_symbol == conversion_options.thousands_separator_symbol)
            throw std::invalid_argument("Error: Thousands and decimal separators have to be different");
    }
}

int main(int argc, const char** argv)
{
    using namespace boost::program_options;

    num::conversion_options_t conversion_options;

    options_description program_options("Options");
    program_options.add_options()
        ( "help,h",
          "Help and usage information" )
        ( "input,i", value<std::vector<std::string>>()->multitoken(),
          "Input value (either number or numeral)" )
        ( "naming-system", value<std::string>()->default_value("short-scale"),
          "Number naming system (either \"short-scale\" or \"long-scale\")" )
        ( "language,l", value<std::string>()->default_value("en-us"),
          "ISO 639-1 standard language code for conversion to numerals" )
        ( "use-scientific-notation", value<bool>()->default_value(false),
          "Uses scientific notation if applicable in conversion to numbers" )
        ( "use-thousands-separator", value<bool>()->default_value(true),
          "Uses thousands separators in conversion to numbers" )
        ( "thousands-separator-symbol", value<char>(),
          "Thousands separator symbol" )
        ( "decimal-separator-symbol", value<char>(),
          "Decimal separator symbol" );
        
    options_description hidden_program_options("Hidden Options");
    hidden_program_options.add_options()
        ( "debug-output", value<bool>()->default_value(false) );
        
    options_description parsed_program_options;
    parsed_program_options.add(program_options).add(hidden_program_options);
        
    const auto print_usage_information = [&]() {
        std::cout << "Usage:" << std::endl <<
            "  numero [options] <input-1> [<input-2>] [\"<input-3 with spaces\"]" << std::endl << std::endl << 
            program_options << std::endl;
        return EXIT_FAILURE;
    };
    
    if (argc == 1)
    {
        print_usage_information();
        return EXIT_FAILURE;
    }

    positional_options_description positional_program_options;
    positional_program_options.add("input", -1);

    std::vector<std::string> inputs;

    try
    {
        command_line_parser parser(argc, argv);
        parser.options(parsed_program_options)
              .positional(positional_program_options)
              .allow_unregistered();
        parsed_options parsed_options = parser.run();
        
        variables_map vm;
        store(parsed_options, vm);
        notify(vm);

        if (vm.count("help"))
        {
            print_usage_information();
            return EXIT_FAILURE;
        }

        if (vm.count("input"))
        {
            inputs = vm["input"].as<std::vector<std::string>>();
        }
        
        process_program_options(vm, conversion_options);
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Error: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    for (const auto &input : inputs)
    {
        std::string output;
        const auto input_is_number = num::is_number(input, conversion_options);
        
        std::cout << (input_is_number ? "Number: " : "Numeral: ") << input << std::endl;
        
        try
        {
            output = num::convert(input, conversion_options);
        }
        catch (const std::exception &ex)
        {
            std::cout << "Error: " << ex.what() << std::endl;
            return EXIT_FAILURE;
        }
        
        std::cout << (input_is_number ? "Numeral: " : "Number: ") << output << std::endl << std::endl;
    }
    
    return EXIT_SUCCESS;
}
