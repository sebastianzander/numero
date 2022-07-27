#include <iostream>
#include <stdexcept>
#include <vector>
#include <map>
#include <string>
#include <string_view>
#include <sstream>
#include <regex>

#include <boost/bimap.hpp>
#include <boost/format.hpp>

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
    static const auto value_to_prefix = make_bimap<int, std::string_view>({
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
     * Finds the prefix that the string \param subject starts with.
     * Returns the iterator of a prefix-value pair in value_to_prefix.right if found; value_to_prefix.right.end() if
     * no prefix could be found.
     */
    auto find_prefix(const std::string_view &subject)
    {
        auto it = value_to_prefix.right.begin();
        for (; it != value_to_prefix.right.end(); it++)
        {
            if (subject.starts_with(it->first))
                return it;
        }
        return value_to_prefix.right.end();
    }
    
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
    static const auto factor_to_root = make_bimap<int, std::string_view>({
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
     * The following are distinctly named English unit numerals including irregular forms (if there is more than one 
     * numeral per line). Irregular forms are primarily used in combination with either "-teen" or "-ty", e.g. "fifty".
     * "teen" and "ty" are itself stored as irregular forms of "ten".
     */
    static const std::vector<std::vector<std::string_view>> units = {
        { "zero" },
        { "one" },
        { "two", "twen" },
        { "three", "thir" },
        { "four" },
        { "five", "fif" },
        { "six" },
        { "seven" },
        { "eight", "eigh" },
        { "nine" },
        { "ten", "teen", "ty" },
        { "eleven" },
        { "twelve" },
    };

    /*
     * The following are distinctly named English decimal power-based numerals. These are special numerals between the
     * unit numerals and the standard dictionary (i.e. less than a million). They are based on the formula 10^x where
     * the power x is given by the key.
     */
    static const std::map<uint8_t, std::string_view> stems = {
        { 1, "ty" },
        { 2, "hundred" },
        { 3, "thousand" },
    };

    /*
     * Group boundary words used to split a numeral into multiple group numerals. See `split_numeral` for more details.
     */
    static const std::vector<std::string_view> group_boundaries = { "illiard", "illion", "thousand" };

    /*
     * Helper struct for conversion between group numerals and group numbers. See `split_numeral` for more details.
     */
    struct group
    {
        std::string numeral;
        std::string fragment_str;
        std::string root_str;
        std::string suffix_str;
        std::string value_str;

        uint16_t fragment = 0;
        uint16_t factor_power = 0;
    };

    /*
     * Splits a numeral into multiple group numerals. Each group numeral is then individually but equally converted to
     * a group fragment. A group fragment is always in the range [0, 999]. The group suffix decides the factor with
     * which to multiply the group fragment in order to receive the actual group number.
     * Returns a vector of num::group that holds information about each group, e.g. the numeral and the number in form
     * of a std::string. num::group is used for both conversion from and to numbers. See `struct group`.
     * 
     * Example: "seven hundred four million | eighty three thousand | eleven"
     * The above example consists of three group numerals that were split right after "thousand" or right after an
     * occurrence of "illion". The first and most significant group numeral "seven hundred four" is converted normally
     * to the group fragment 704. The group suffix "million" sets the group factor to 10^6, resulting in a group number
     * of 704.000.000. These steps are repeated for each group numeral. In the end the final number is created by
     * summing up all group numbers: 704.000.000 + 83.000 + 11 = 704.083.011
     */
    std::vector<num::group> split_numeral(const std::string_view &numeral)
    {
        static const std::regex group_pattern("\\b(?:(.+?)(\\w+)(illiard)|(.+?)(\\w+)(illion)|(.+?)(thousand)|(.+))\\b");

        enum indices
        {
            FULL_MATCH = 0,
            ILLIARDS_FRAGMENT,
            ILLIARDS_ROOT,
            ILLIARDS_SUFFIX,
            ILLIONS_FRAGMENT,
            ILLIONS_ROOT,
            ILLIONS_SUFFIX,
            THOUSANDS_FRAGMENT,
            THOUSANDS_ROOT,
            ONES_FRAGMENT
        };
        
        std::smatch matches;
        std::vector<num::group> groups;
        std::string _numeral = std::string(numeral);
        auto it = _numeral.cbegin();

        std::cout << "Groups:" << std::endl;
        
        while (std::regex_search(it, _numeral.cend(), matches, group_pattern))
        {
            num::group group;
            std::cout << "   " << matches.str() << std::endl;

            // R-005: Verify correct order of terms in numeral.
            // R-006: Verify uniquely used terms in numeral.
            // R-007: Verify valid terms in numeral.

            uint16_t root_factor = 0;
            uint16_t short_scale_power = 0;

            if (matches[ILLIARDS_FRAGMENT].matched)
            {
                group.fragment_str = matches[ILLIARDS_FRAGMENT].str();
                group.root_str = matches[ILLIARDS_ROOT].str();
                group.suffix_str = matches[ILLIARDS_SUFFIX].str();
            }
            else if (matches[ILLIONS_FRAGMENT].matched)
            {
                group.fragment_str = matches[ILLIONS_FRAGMENT].str();
                group.root_str = matches[ILLIONS_ROOT].str();
                group.suffix_str = matches[ILLIONS_SUFFIX].str();
            }
            else if (matches[THOUSANDS_FRAGMENT].matched)
            {
                group.fragment_str = matches[THOUSANDS_FRAGMENT].str();
                group.root_str = matches[THOUSANDS_ROOT].str();
                root_factor = 0;
                short_scale_power = 3;
            }
            else if (matches[ONES_FRAGMENT].matched)
            {
                group.fragment_str = matches[ONES_FRAGMENT].str();
            }
            
            // Deduce root factor and power
            if (group.root_str != "thousand" && group.root_str != "")
            {
                const auto factor_it = factor_to_root.right.find(group.root_str);
                if (factor_it != factor_to_root.right.end())
                {
                    root_factor = factor_it->second;
                    short_scale_power = 3 * root_factor + 3;
                }
                else
                {
                    const auto prefix_value_pair_it = find_prefix(group.root_str);
                    if ( prefix_value_pair_it != value_to_prefix.right.end() )
                    {
                        const auto actual_prefix = prefix_value_pair_it->first;
                        const auto actual_root = group.root_str.substr(actual_prefix.size());
                        const auto factor_it = factor_to_root.right.find(actual_root);
                        if (factor_it != factor_to_root.right.end())
                        {
                            root_factor = factor_it->second + prefix_value_pair_it->second;
                            short_scale_power = 3 * root_factor + 3;
                        }
                    }
                    else
                    {
                        const auto message = boost::format("\"%1%\" is not a valid root prefix") % group.root_str;
                        throw std::invalid_argument(message.str());
                    }
                }
            }
            
            // TODO: Parse fragment

            std::cout << "      fragment: " << group.fragment_str << std::endl;
            std::cout << "      root: " << group.root_str << std::endl;
            std::cout << "      suffix: " << group.suffix_str << std::endl;
            std::cout << "      root factor: " << root_factor << std::endl;
            std::cout << "      short scale power: " << short_scale_power << std::endl;

            groups.push_back(group);
            it = matches[0].second;
        }

        return groups;
    }

    std::string_view join_numeral(std::vector<num::group> &groups)
    {
        std::string result;
        return result;
    }

    bool to_number(num::group &group)
    {
        bool success = false;

        for (const auto &stem : num::stems)
        {
            // Find stem and return its value
        }

        return success;
    }

    bool to_numeral(num::group &group)
    {
        bool success = false;

        for (const auto &stem : num::stems)
        {
            // Find stem and return its numeral
        }

        return success;
    }

    std::string_view to_number(const std::string_view &numeral)
    {
        std::stringstream ss;

        bool negative = numeral.rfind("negative ", 0) == 0 || numeral.rfind("minus ", 0) == 0;
        auto groups = num::split_numeral(numeral);
        
        if (negative)
            ss << "-";

        for (auto &group : groups)
            if (num::to_number(group))
                ss << group.value_str;

        return ss.str();
    }

    bool to_number(const std::string_view &numeral, uint64_t &out_value)
    {
        return false;
    }

    bool is_number(const std::string &input)
    {
        static const std::regex number_pattern("^-?\\d{1,3}(?:\\.\\d{3})*(?:,\\d+)?$|^\\d+(?:,)?\\d+$");
        return std::regex_match(input, number_pattern);
    }

    bool is_number(const std::string_view &input)
    {
        return is_number(std::string(input));
    }

    std::string_view to_numeral(const std::string_view &number)
    {
        return {};
    }

    std::string_view convert(const std::string_view &input)
    {
        return num::is_number(input) ? num::to_numeral(input) : num::to_number(input);
    }
}

int main(int argc, const char** argv)
{
    const auto language = std::string_view("en-us");
    const bool use_short_scale = true;
    const bool use_scientific_notation_if_practical = true;
    const bool use_spaces_in_numerals = true;
    const bool use_ands_in_numerals = true;

    std::string output;
    const auto input = std::string_view(argc > 1 ? argv[1] : "seven hundred four million eighty three thousand eleven");
    const auto input_is_number = num::is_number(input);
    
    std::cout << (input_is_number ? "Number: " : "Numeral: ") << input << std::endl;
    
    try
    {
        output = num::convert(input);
    }
    catch(std::invalid_argument const &ex)
    {
        std::cout << "Invalid argument: " << ex.what() << std::endl;
    }
    
    std::cout << (input_is_number ? "Numeral: " : "Number: ") << output << std::endl;
    
    return EXIT_SUCCESS;
}
