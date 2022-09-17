#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>

#include <boost/format.hpp>
#include <boost/program_options.hpp>

#include <numero/numero.h>

enum class generation_mode_t
{
    unset = 0,
    number,
    numeral
};

template <class T>
constexpr int num_digits(T number)
{
    int digits = 1;
    while (number /= 10)
        digits++;
    return digits;
}

static inline void ltrim_zeroes(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return ch != '0';
    }));

    if (s.empty())
        s = "0";
}

int main(int argc, const char** argv)
{
    using namespace boost::program_options;
    using namespace num;

    std::srand(time(0));

    int count, min_places, max_places;
    naming_system_t naming_system = naming_system_t::undefined;
    generation_mode_t generation_mode = generation_mode_t::unset;

    options_description program_options("Options");
    program_options.add_options()
        ( "help,h",
          "Help and usage information" )
        ( "count,c", value<int>(),
          "Count of numbers or numerals to be generated" )
        ( "generation-mode,g", value<std::string>()->default_value("numbers"),
          "Either 'numbers' or 'numerals'" )
        ( "naming-system,s", value<std::string>()->default_value("short-scale"),
          "Number naming system; either 'short-scale' ('SS') or 'long-scale' ('LS')" )
        ( "min-places,m", value<int>()->default_value(1),
          "Minimum number of places the generated random numbers (or its equivalent numerals) shall have" )
        ( "max-places,M", value<int>()->default_value(12),
          "Maximum number of places the generated random numbers (or its equivalent numerals) shall have; this number "
          "may be as high as 303 if the 'short-scale' number system is being used, and as high as 600 if the 'long-"
          "scale' number system is being used" );
        
    options_description hidden_program_options("Hidden Options");
    hidden_program_options.add_options()
        ( "debug-output", bool_switch() );
        
    options_description parsed_program_options;
    parsed_program_options.add(program_options).add(hidden_program_options);
        
    const auto print_usage_information = [&]() {
        std::cout << "Usage:\n  numero_generator [options]\n\n" << program_options << "\n";
        return EXIT_FAILURE;
    };
    
    try
    {
        command_line_parser parser(argc, argv);
        parser.options(parsed_program_options);
        parsed_options parsed_options = parser.run();
        
        variables_map vm;
        store(parsed_options, vm);
        notify(vm);

        if (vm.count("help"))
        {
            print_usage_information();
            return EXIT_FAILURE;
        }

        if (vm.count("count"))
        {
            count = vm["count"].as<int>();
            if (count < 1)
                throw std::invalid_argument("count must not be zero or less");
        }
        else
            throw std::invalid_argument("the option '--count' is required but missing");

        if (vm.count("generation-mode"))
        {
            const auto &generation_mode_string = vm["generation-mode"].as<std::string>();
            if (generation_mode_string == "numbers" || generation_mode_string == "0")
                generation_mode = generation_mode_t::number;
            else if (generation_mode_string == "numerals" || generation_mode_string == "a")
                generation_mode = generation_mode_t::numeral;
            else
            {
                const auto message = boost::format("\"%1%\" is not a valid generation mode. "
                                                   "Supported generation modes are 'number' and 'numeral'.")
                                                   % generation_mode_string;
                throw std::invalid_argument(message.str());
            }
        }

        if (vm.count("naming-system"))
        {
            const auto &naming_system_string = vm["naming-system"].as<std::string>();
            if (naming_system_string == "short-scale" || naming_system_string == "short" ||
                naming_system_string == "ss" || naming_system_string == "SS")
                naming_system = naming_system_t::short_scale;
            else if (naming_system_string == "long-scale" || naming_system_string == "long" ||
                naming_system_string == "ls" || naming_system_string == "LS")
                naming_system = naming_system_t::long_scale;
            else
            {
                const auto message = boost::format("\"%1%\" is not a valid number naming system. "
                                                   "Supported naming systems are 'short-scale' and 'long-scale'.")
                                                   % naming_system_string;
                throw std::invalid_argument(message.str());
            }
        }

        if (vm.count("min-places"))
        {
            min_places = vm["min-places"].as<int>();
            if (min_places < 1)
                throw std::invalid_argument("'min-places' must at least be '1'");
        }

        if (vm.count("max-places"))
        {
            max_places = vm["max-places"].as<int>();
            if (max_places > 303 && naming_system == naming_system_t::short_scale)
                throw std::invalid_argument("'max-places' must at most be '303' in the 'short-scale' naming system");
            else if (max_places > 600 && naming_system == naming_system_t::long_scale)
                throw std::invalid_argument("'max-places' must at most be '600' in the 'long-scale' naming system");
        }
    }
    catch (const std::exception &ex)
    {
        std::cerr << "\033[31mError: " << ex.what() << "\033[0m\n\n";
        return EXIT_FAILURE;
    }

    num::converter_c converter;
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> digits_uniform_distribution;
    std::uniform_int_distribution<int> places_uniform_distribution(min_places, max_places);
    
    for (int i = 0; i < count; i++)
    {
        // First, generate random number of places between the given minimum and maximum places.
        int random_places = places_uniform_distribution(rng);
        std::stringstream ss;

        // If the maximum number of places is higher than can be generated, we have to split the task into multiple
        // groups of random numbers and concatenate them eventually.
        constexpr auto places_per_group = num_digits(std::numeric_limits<int>::max()) - 1;

        // Generate random numbers almost as big as possible for each group.
        for (int places = 0; places < random_places; places += places_per_group)
            ss << digits_uniform_distribution(rng);

        std::string random_number = ss.str();
        const auto excess = random_number.size() - random_places;

        // Cut the random number string down to fit the generated random number of places if required.
        if (excess > 0)
        {
            std::uniform_int_distribution<int> offset_uniform_distribution(0, excess - 1);
            random_number = random_number.substr(offset_uniform_distribution(rng), random_places);
            assert(!random_number.empty());
        }

        // Left trim the random number and remove any leading zeroes.
        ltrim_zeroes(random_number);
        
        if (generation_mode == generation_mode_t::number)
            std::cout << random_number << "\n";
        else
        {
            const auto &random_numeral = converter.to_numeral(random_number);
            std::cout << random_numeral << "\n";
        }
    }
    
    return EXIT_SUCCESS;
}
