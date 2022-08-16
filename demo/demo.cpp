#include <iostream>
#include <stdexcept>
#include <vector>

#include <boost/format.hpp>
#include <boost/program_options.hpp>

#include "numero/numero.h"

void process_program_options(const boost::program_options::variables_map &vm,
                             num::conversion_options_t &conversion_options)
{
    using namespace boost::program_options;
    
    if (vm.count("debug-output"))
        conversion_options.debug_output = vm["debug-output"].as<bool>();

    if (vm.count("naming-system"))
    {
        const auto &naming_system = vm["naming-system"].as<std::string>();
        if (naming_system == "short-scale" || naming_system == "short" ||
            naming_system == "ss" || naming_system == "SS")
            conversion_options.naming_system = num::naming_system_t::short_scale;
        else if (naming_system == "long-scale" || naming_system == "long" ||
            naming_system == "ls" || naming_system == "LS")
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
    
    if (vm.count("force-leading-zero"))
        conversion_options.force_leading_zero = vm["force-leading-zero"].as<bool>();
    
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
        ( "naming-system,s", value<std::string>()->default_value("short-scale"),
          "Number naming system; either \"short-scale\" (\"SS\") or \"long-scale\" (\"LS\")" )
        ( "language,l", value<std::string>()->default_value("en-us"),
          "ISO 639-1 standard language code for conversion to numerals" )
        ( "use-scientific-notation", value<bool>()->default_value(false),
          "Uses scientific notation if applicable in conversion to numbers" )
        ( "use-thousands-separator", value<bool>()->default_value(true),
          "Uses thousands separators in conversion to numbers" )
        ( "force-leading-zero", value<bool>()->default_value(true),
          "Forces a leading zero in conversion to decimal numbers if the integral part of a number is effectively zero" )
        ( "thousands-separator-symbol", value<char>(),
          "Thousands separator symbol" )
        ( "decimal-separator-symbol", value<char>(),
          "Decimal separator symbol" );
        
    options_description hidden_program_options("Hidden Options");
    hidden_program_options.add_options()
        ( "debug-output", bool_switch() );
        
    options_description parsed_program_options;
    parsed_program_options.add(program_options).add(hidden_program_options);
        
    const auto print_usage_information = [&]() {
        std::cout << "Usage:\n  numero [options] <input-1> [<input-2>] [\"<input-3 with spaces\"]\n\n" <<
                     program_options << "\n";
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
        std::cerr << "\033[31mError: " << ex.what() << "\033[0m\n\n";
        return EXIT_FAILURE;
    }

    std::string naming_system_string;

    switch (conversion_options.naming_system)
    {
    case num::naming_system_t::short_scale:
        naming_system_string = "short scale";
        break;
    case num::naming_system_t::long_scale:
        naming_system_string = "long scale";
        break;
    default:
        naming_system_string = "undefined scale";
    }

    std::size_t failure_count = 0;

    for (const auto &input : inputs)
    {
        std::string output;
        const auto input_is_number = num::is_number(input, conversion_options);
        
        if (!input_is_number)
        {
            const auto input_is_numeral = num::is_numeral(input);
            if (!input_is_numeral)
            {
                std::cout << "Input: \033[34m" << input << "\033[0m\n";
                std::cerr << "\033[31mError: the given input is neither number nor numeral\033[0m\n\n";
                failure_count++;
                continue;
            }
        }
        
        if (input_is_number)
            std::cout << "Number: \033[34m" << input << "\033[0m\n";
        else
            std::cout << "Numeral: \033[34m" << input << " \033[37m(" << naming_system_string << ")\033[0m\n";
        
        try
        {
            output = num::convert(input, conversion_options);
        }
        catch (const std::exception &ex)
        {
            std::cerr << "\033[31mError: " << ex.what() << "\033[0m\n\n";
            failure_count++;
            continue;
        }
        
        if (input_is_number)
            std::cout << "Numeral: \033[33m" << output << " \033[37m(" << naming_system_string << ")\033[0m\n";
        else
            std::cout << "Number: \033[33m" << output << "\033[0m\n";
    }
    
    return failure_count ? failure_count : EXIT_SUCCESS;
}
