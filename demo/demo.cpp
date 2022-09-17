#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

#include <boost/format.hpp>
#include <boost/program_options.hpp>

#include <numero/numero.h>

using hr_clock = std::chrono::high_resolution_clock;

enum class output_mode_t
{
    unset = 0,
    descriptive,
    associative,
    bare,
    suppress
};

enum class timing_mode_t
{
    dont_time = 0,
    time_total_duration,
    time_single_durations,
    time_all_durations
};

struct conversion_t
{
    bool input_is_number;
    std::string result;
    int64_t duration;
    bool error;
};

void convert_inputs(const std::vector<std::string> &inputs,
                    std::vector<conversion_t> &conversions,
                    const std::size_t start_index,
                    const std::size_t increment,
                    num::converter_c &converter,
                    const output_mode_t &output_mode,
                    const timing_mode_t &timing_mode)
{
    for (auto i = start_index; i < inputs.size(); i += increment)
    {
        const auto &input = inputs[i];
        const auto input_is_number = converter.is_number(input);
        auto &conversion = conversions[i];

        int64_t single_time = 0;
        std::string result;
        
        if (!input_is_number)
        {
            const auto input_is_numeral = converter.is_numeral(input);
            if (!input_is_numeral)
            {
                const auto message = boost::format("\"%1%\" is neither number nor numeral.") % input;
                conversion = { input_is_number, message.str(), single_time, true };
                continue;
            }
        }
        
        std::chrono::system_clock::time_point before_convert, after_convert;
        
        if (timing_mode != timing_mode_t::dont_time)
            before_convert = hr_clock::now();
        
        try
        {
            result = converter.convert(input);
        }
        catch (const std::exception &ex)
        {
            conversion = { input_is_number, ex.what(), single_time, true };
            continue;
        }

        if (timing_mode != timing_mode_t::dont_time)
        {
            after_convert = hr_clock::now();
            single_time = std::chrono::duration_cast<std::chrono::microseconds>(after_convert - before_convert).count();
        }

        conversion = { input_is_number, result, single_time, false };
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
        if (naming_system == "short-scale" || naming_system == "short" ||
            naming_system == "ss" || naming_system == "SS")
            conversion_options.naming_system = num::naming_system_t::short_scale;
        else if (naming_system == "long-scale" || naming_system == "long" ||
            naming_system == "ls" || naming_system == "LS")
            conversion_options.naming_system = num::naming_system_t::long_scale;
        else
        {
            const auto message = boost::format("\"%1%\" is not a valid number naming system. "
                                               "Supported naming systems are 'short-scale' and 'long-scale'.")
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
        ( "jobs-count,j", value<std::size_t>(),
          "Maximum number of parallel jobs for conversion" )
        ( "output-mode,o", value<std::string>(),
          "Either 'descriptive', 'associative' or 'bare'" )
        ( "naming-system,s", value<std::string>()->default_value("short-scale"),
          "Number naming system; either 'short-scale' ('SS') or 'long-scale' ('LS')" )
        ( "language,l", value<std::string>()->default_value("en-us"),
          "ISO 639-1 standard language code for conversion to numerals" )
        ( "use-scientific-notation", value<bool>()->default_value(false),
          "Uses scientific notation if applicable in conversion to numbers" )
        ( "use-thousands-separator,t", value<bool>()->default_value(true),
          "Uses thousands separators in conversion to numbers" )
        ( "force-leading-zero,z", value<bool>()->default_value(true),
          "Forces a leading zero in conversion to decimal numbers if the integral part of a number is effectively zero" )
        ( "thousands-separator-symbol,T", value<char>(),
          "Thousands separator symbol" )
        ( "decimal-separator-symbol,D", value<char>(),
          "Decimal separator symbol" );
        
    options_description hidden_program_options("Hidden Options");
    hidden_program_options.add_options()
        ( "debug-output", bool_switch() )
        ( "timing-mode", value<std::string>() );
        
    options_description parsed_program_options;
    parsed_program_options.add(program_options).add(hidden_program_options);
        
    const auto print_usage_information = [&]() {
        std::cout << "Usage:\n  numero [options] <input-1> [<input-2>] [\"<input-3 with spaces\"]\n\n" <<
                     program_options << "\n";
        return EXIT_FAILURE;
    };

    std::vector<std::string> cmdline_inputs, stdin_inputs;
    output_mode_t output_mode = output_mode_t::unset;
    timing_mode_t timing_mode = timing_mode_t::dont_time;
    std::size_t jobs_count = 1;
    
    positional_options_description positional_program_options;
    positional_program_options.add("input", -1);

    try
    {
        command_line_parser parser(argc, argv);
        parser.options(parsed_program_options)
              .positional(positional_program_options);
        parsed_options parsed_options = parser.run();
        
        variables_map vm;
        store(parsed_options, vm);
        notify(vm);

        if (vm.count("help"))
        {
            print_usage_information();
            return EXIT_FAILURE;
        }

        if (vm.count("output-mode"))
        {
            const auto &output_mode_string = vm["output-mode"].as<std::string>();
            if (output_mode_string == "descriptive" || output_mode_string == "d")
                output_mode = output_mode_t::descriptive;
            else if (output_mode_string == "associative" || output_mode_string == "a")
                output_mode = output_mode_t::associative;
            else if (output_mode_string == "bare" || output_mode_string == "b")
                output_mode = output_mode_t::bare;
            else if (output_mode_string == "suppress" || output_mode_string == "s")
                output_mode = output_mode_t::suppress;
            else
            {
                const auto message = boost::format("\"%1%\" is not a valid output mode. Supported output "
                                                   "modes are 'descriptive', 'associative' and 'bare'.")
                                                   % output_mode_string;
                throw std::invalid_argument(message.str());
            }
        }

        if (vm.count("timing-mode"))
        {
            const auto &timing_mode_string = vm["timing-mode"].as<std::string>();
            if (timing_mode_string == "total" || timing_mode_string == "t")
                timing_mode = timing_mode_t::time_total_duration;
            else if (timing_mode_string == "single" || timing_mode_string == "s")
                timing_mode = timing_mode_t::time_single_durations;
            else if (timing_mode_string == "all" || timing_mode_string == "a")
                timing_mode = timing_mode_t::time_all_durations;
            else
            {
                const auto message = boost::format("\"%1%\" is not a valid timing mode. Supported timing "
                                                   "modes are 'total', 'single' and 'all'.")
                                                   % timing_mode_string;
                throw std::invalid_argument(message.str());
            }
        }

        if (vm.count("input"))
            cmdline_inputs = vm["input"].as<std::vector<std::string>>();
        
        if (vm.count("jobs-count"))
            jobs_count = std::clamp<std::size_t>(vm["jobs-count"].as<std::size_t>(),
                                                 1, std::thread::hardware_concurrency());
        
        process_program_options(vm, conversion_options);
    }
    catch (const std::exception &ex)
    {
        std::cerr << "\033[31mError: " << ex.what() << "\033[0m\n\n";
        return EXIT_FAILURE;
    }

    if (cmdline_inputs.empty())
    {
        for (std::string line; std::getline(std::cin, line);)
        {
            if (line == "") break;
            stdin_inputs.push_back(line);
        }
    }

    if (output_mode == output_mode_t::unset)
        output_mode = stdin_inputs.empty() ? output_mode_t::descriptive : output_mode_t::associative;

    if (cmdline_inputs.empty() && stdin_inputs.empty())
    {
        print_usage_information();
        return EXIT_FAILURE;
    }

    const auto &inputs = !stdin_inputs.empty() ? stdin_inputs : cmdline_inputs;
    const auto threads_count = std::max<std::size_t>(1, std::min<std::size_t>(inputs.size() / 10, jobs_count));
    
    std::vector<conversion_t> conversions(inputs.size());
    std::vector<std::thread> threads;
    std::string naming_system_string;
    int64_t total_time = 0;
    std::size_t total_failure_count = 0;

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

    num::converter_c converter(conversion_options);
    std::chrono::system_clock::time_point before_convert, after_convert;
        
    if (timing_mode != timing_mode_t::time_all_durations || timing_mode != timing_mode_t::time_total_duration)
        before_convert = hr_clock::now();

    for (std::size_t i = 0; i < threads_count; i++)
        threads.emplace_back([&, start_index = i]() {
            convert_inputs(inputs, conversions, start_index, threads_count, converter, output_mode, timing_mode);
        });

    for (auto &thread : threads)
        thread.join();

    if (timing_mode != timing_mode_t::time_all_durations || timing_mode != timing_mode_t::time_total_duration)
        after_convert = hr_clock::now();

    for (std::size_t i = 0; i < inputs.size(); i++)
    {
        const auto &input = inputs[i];
        const auto &conversion = conversions[i];
        
        if (output_mode == output_mode_t::descriptive)
        {
            if (conversion.input_is_number)
                std::cout << "Number:  \033[34m" << input << "\033[0m\n";
            else
                std::cout << "Numeral: \033[34m" << input << " \033[37m(" << naming_system_string << ")\033[0m\n";
        }
        
        if (output_mode == output_mode_t::descriptive)
        {
            if (conversion.error)
                std::cerr << "\033[31mError: " << conversion.result << "\033[0m\n";
            else if (conversion.input_is_number)
                std::cout << "Numeral: \033[33m" << conversion.result << " \033[37m(" << naming_system_string << 
                    ")\033[0m\n";
            else
                std::cout << "Number:  \033[33m" << conversion.result << "\033[0m\n";
        }
        else if (output_mode == output_mode_t::associative)
        {
            if (conversion.error)
                std::cerr << "\033[34m" << input << "\033[0m = \033[31mError: " << conversion.result << "\033[0m\n";
            else
                std::cout << "\033[34m" << input << "\033[0m = \033[33m" << conversion.result << "\033[0m\n";
        }
        else if (output_mode == output_mode_t::bare)
        {
            if (conversion.error)
                std::cerr << "\033[31mError: " << conversion.result << "\033[0m\n";
            else
                std::cout << "\033[33m" << conversion.result << "\033[0m\n";
        }

        if (timing_mode == timing_mode_t::time_single_durations || timing_mode == timing_mode_t::time_all_durations)
            std::cout << "   - took " << conversion.duration << " us\n";

        if (output_mode == output_mode_t::descriptive)
            std::cout << "\n";

        total_time += conversion.duration;
        
        if (conversion.error)
            total_failure_count++;
    }

    if (timing_mode == timing_mode_t::time_total_duration || timing_mode == timing_mode_t::time_all_durations)
    {
        int64_t average_time = total_time / inputs.size();
        std::cout << "   - took " << total_time << " us in absolute total (" << average_time << " us on average)\n";

        if (threads_count > 1)
        {
            const auto total_parallel_time =
                std::chrono::duration_cast<std::chrono::microseconds>(after_convert - before_convert).count();
            int64_t average_parallel_time = total_parallel_time / inputs.size();
            std::cout << "   - took " << total_parallel_time << " us in parallel total (" << average_parallel_time 
                      << " us on average) using " << threads_count << " jobs\n";
        }
    }
    
    return total_failure_count ? total_failure_count : EXIT_SUCCESS;
}
