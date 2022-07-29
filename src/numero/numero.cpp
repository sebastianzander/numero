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
    
    static const uint64_t pow10[20] = {
        1ull,
        10ull,
        100ull,
        1000ull,
        10000ull,
        100000ull,
        1000000ull,
        10000000ull,
        100000000ull,
        1000000000ull,
        10000000000ull,
        100000000000ull,
        1000000000000ull,
        10000000000000ull,
        100000000000000ull,
        1000000000000000ull,
        10000000000000000ull,
        100000000000000000ull,
        1000000000000000000ull,
        10000000000000000000ull
    };

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
    
    static const auto value_to_term = make_bimap<std::string_view, std::string_view>({
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
     * The following are distinctly named English decimal power-based numerals. These are special numerals between the
     * unit numerals and the standard dictionary (i.e. less than a million). They are based on the formula 10^x where
     * the power x is given by the key.
     */
    static const std::map<uint8_t, std::string_view> stems = {
        { 1, "ty" },
        { 2, "hundred" },
        { 3, "thousand" },
    };
    
    static const auto multiplicative_terms = make_bimap<int, std::string_view>({
        {     100, "hundred" },
        {    1000, "thousand" },
        { 1000000, "million" }
    });

    static const auto multiplicative_shifts = make_bimap<int, std::string_view>({
        { 2, "hundred" },
        { 3, "thousand" },
        { 4, "myriad" }
    });

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
     * Finds the additive value for the given term.
     * \param term the term to find the additive value for.
     * \returns the additive value, a value greater than 0 if term is valid; 0 if the term is invalid.
     * \throws std::invalid_argument exception if the term does not resolve to an additive value.
     */
    std::string_view find_additive_value(const std::string_view &term)
    {
        const auto term_value_pair_it = value_to_term.right.find(term);
        if (term_value_pair_it != value_to_term.right.end())
        {
            return term_value_pair_it->second;
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
            {
                const auto message = boost::format("long scale number system is currently not supported") % term;
                throw std::invalid_argument(message.str());
            }

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
            {
                const auto message = boost::format("cannot merge %1% and %2% at place %3%")
                                                   % original_target % source % place;
                throw std::logic_error(message.str());
            }
            else if (*s != '0')
                *t = *s;
        }
    }

    void shift_places(const uint32_t places_count, std::string &target)
    {
        target.insert(target.end(), places_count, '0');
    }

    void add_thousands_separators(std::string &target)
    {
        std::stringstream ss;
        const auto offset = target.size() % 3;
        
        for (std::size_t i = 0; i < target.size(); i++)
        {
            if (i > 0 && i % 3 == offset) ss << ",";
            ss << target[i];
        }

        target = ss.str();
    }

    std::string to_number(const std::string_view &numeral)
    {
        static const std::regex split_pattern("[\\s-]+");

        if (numeral.empty())
        {
            const auto message = boost::format("the numeral must not be empty");
            throw std::invalid_argument(message.str());
        }
        
        bool negative = false;
        std::string _numeral = std::string(numeral);

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
                current_additive_value = find_additive_value(term);
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
                    
                    //std::cout << "Group number: " << current_group << std::endl;
                    //std::cout << "New group" << std::endl;
                    
                    current_group = "";
                }

                //std::cout << "Term: " << term << std::endl;
                //std::cout << "  Additive value: " << current_additive_value << std::endl;
                last_term_multiplicative = false;

                merge_places(current_additive_value, current_group);
            }
            // Process multiplicative term.
            else
            {
                // Add an implicit 1 if that is missing at the beginning of the numeral.
                if (groups.empty() && current_group.empty())
                    current_group = "1";
                
                //std::cout << "Term: " << term << std::endl;
                //std::cout << "  Multiplicative value: 10^" << current_multiplicative_shift << std::endl;
                last_term_multiplicative = true;
                last_multiplicative_shift = current_multiplicative_shift;

                shift_places(current_multiplicative_shift, current_group);
            }
        }

        if (groups.empty() && current_group.empty() && negative)
        {
            const auto message = boost::format("the numeral must not be empty");
            throw std::invalid_argument(message.str());
        }

        groups.push_back(current_group);

        std::string result;

        for (const auto &group : groups)
            merge_places(group, result);
        
        add_thousands_separators(result);

        if (negative)
            result.insert(0, 1, '-');

        return result;
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

    std::string to_numeral(const std::string_view &number)
    {
        return {};
    }

    std::string convert(const std::string_view &input)
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
    const auto input = std::string_view(argc > 1 ? argv[1] : "seven hundred four million eighty-three thousand eleven");
    const auto input_is_number = num::is_number(input);
    
    std::cout << (input_is_number ? "Number: " : "Numeral: ") << input << std::endl;
    
    try
    {
        output = num::convert(input);
    }
    catch(std::exception const &ex)
    {
        std::cout << "Exception: " << ex.what() << std::endl;
    }
    
    std::cout << (input_is_number ? "Numeral: " : "Number: ") << output << std::endl;
    
    return EXIT_SUCCESS;
}
