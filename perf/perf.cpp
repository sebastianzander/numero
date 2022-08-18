#include <cassert>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <boost/format.hpp>
#include <boost/program_options.hpp>

#include <numero/numero.h>

using hr_clock = std::chrono::high_resolution_clock;

static const std::vector<std::string> example_english_numbers = {
    "3",
    "12",
    "13",
    "37",
    "1,001",
    "16,016",
    "233,082",
    "1,234,567",
    "1,002,003,004",
    "9,999,999,999,999,999",
    "1,234,567,890,123,456,789,012,345,678,901,234,567,890",
    "-1,024",
    "3.141592653589"
};

static const std::vector<std::string> example_german_numbers = {
    "3",
    "12",
    "13",
    "37",
    "1.001",
    "16.016",
    "233.082",
    "1.234.567",
    "1.002.003.004",
    "9.999.999.999.999.999",
    "1.234.567.890.123.456.789.012.345.678.901.234.567.890",
    "-1.024",
    "3,141592653589"
};

static const std::vector<std::string> example_numerals = {
    "three",
    "twelve",
    "thirteen",
    "one thousand one",
    "sixteen thousand sixteen",
    "one million two hundred thirty-four thousand five hundred sixty-seven",
    "one billion two million three thousand four",
    "nine quadrillion nine hundred ninety-nine trillion nine hundred ninety-nine billion nine hundred ninety-nine million nine hundred ninety-nine",
    "thousand nine hundred ninety-nine",
    "negative one thousand twenty-four",
    "thousand eighty",
    "nineteen hundred eighteen",
    "one thousand million",
    "two million million"
};

int main(int argc, const char** argv)
{
    using namespace boost::program_options;

    options_description program_options("Options");
    program_options.add_options()
        ( "help,h",
          "Help and usage information" );
        
    options_description hidden_program_options("Hidden Options");
    hidden_program_options.add_options()
        ( "debug-output", bool_switch() );
        
    options_description parsed_program_options;
    parsed_program_options.add(program_options).add(hidden_program_options);
        
    const auto print_usage_information = [&]() {
        std::cout << "Usage:\n  numero_perf [options]\n\n" <<
                     program_options << "\n";
        return EXIT_FAILURE;
    };
    
    try
    {
        command_line_parser parser(argc, argv);
        parser.options(parsed_program_options)
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
    }
    catch (const std::exception &ex)
    {
        std::cerr << "\033[31mError: " << ex.what() << "\033[0m\n\n";
        return EXIT_FAILURE;
    }

    // Construct converter
    auto start = hr_clock::now();

    num::converter_c converter;

    auto end = hr_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << boost::format("Constructing converter took %1% us") % elapsed << std::endl;
    
    // Convert number to numeral using initial number pattern
    start = hr_clock::now();
    
    std::vector<std::string> results;
    for (const auto &number : example_english_numbers)
        results.emplace_back(converter.to_numeral(number));

    assert(example_english_numbers.size() == results.size());

    end = hr_clock::now();
    elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    auto average = std::lround(static_cast<double>(elapsed) / example_english_numbers.size());
    std::cout << boost::format("Converting number to numeral using initial number pattern took on average %1% us")
                               % average << std::endl;
    results.clear();
    
    // Convert number to numeral using altered number pattern
    start = hr_clock::now();
    
    converter.conversion_options().decimal_separator_symbol = ',';
    converter.conversion_options().thousands_separator_symbol = '.';
    for (const auto &number : example_german_numbers)
        results.emplace_back(converter.to_numeral(number));

    assert(example_german_numbers.size() == results.size());

    end = hr_clock::now();
    elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    auto baseline_average = average;
    average = std::lround(static_cast<double>(elapsed) / example_german_numbers.size());
    auto factor = average / baseline_average;
    std::cout << boost::format("Converting number to numeral using altered number pattern took on average %1% us "
                               "(about %2% times longer)") % average % factor << std::endl;
    results.clear();
    
    // Convert numeral to number
    start = hr_clock::now();
    
    converter.conversion_options().decimal_separator_symbol = ',';
    converter.conversion_options().thousands_separator_symbol = '.';
    for (const auto &numeral : example_numerals)
        results.emplace_back(converter.to_number(numeral));

    assert(example_numerals.size() == results.size());

    end = hr_clock::now();
    elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    average = std::lround(static_cast<double>(elapsed) / example_numerals.size());
    std::cout << boost::format("Converting numeral to number took on average %1% us") % average << std::endl;

    results.clear();
    
    return EXIT_SUCCESS;
}
