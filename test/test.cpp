#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE numero_test_module
#include <boost/test/unit_test.hpp>

#include <numero/numero.h>

BOOST_AUTO_TEST_CASE(is_number)
{
    num::conversion_options_t english_options;
    english_options.thousands_separator_symbol = ',';
    english_options.decimal_separator_symbol = '.';
    num::converter_c english_converter(english_options);

    BOOST_CHECK(english_converter.is_number("0"));
    BOOST_CHECK(english_converter.is_number("1"));
    BOOST_CHECK(english_converter.is_number("1e3"));
    BOOST_CHECK(english_converter.is_number("1e-3"));
    BOOST_CHECK(english_converter.is_number("1-e3") == false);
    BOOST_CHECK(english_converter.is_number("-") == false);
    
    BOOST_CHECK(english_converter.is_number("1,000"));
    BOOST_CHECK(english_converter.is_number("1,000,000"));
    BOOST_CHECK(english_converter.is_number("1,000,00") == false);
    BOOST_CHECK(english_converter.is_number("1,00,000") == false);
    BOOST_CHECK(english_converter.is_number("0.333333"));
    BOOST_CHECK(english_converter.is_number("0.333.333") == false);
    BOOST_CHECK(english_converter.is_number("0.333 333") == false);
    BOOST_CHECK(english_converter.is_number("-6.25e-2"));

    num::conversion_options_t german_options;
    german_options.thousands_separator_symbol = '.';
    german_options.decimal_separator_symbol = ',';
    num::converter_c german_converter(german_options);

    BOOST_CHECK(german_converter.is_number("1.000"));
    BOOST_CHECK(german_converter.is_number("1.000.000"));
    BOOST_CHECK(german_converter.is_number("1.000.00") == false);
    BOOST_CHECK(german_converter.is_number("1.00.000") == false);
    BOOST_CHECK(german_converter.is_number("0,333333"));
    BOOST_CHECK(german_converter.is_number("0,333,333") == false);
    BOOST_CHECK(german_converter.is_number("0,333 333") == false);
    BOOST_CHECK(german_converter.is_number("-6,25e-2"));
}

BOOST_AUTO_TEST_CASE(convert_invalid_arguments)
{
    num::converter_c converter;

    BOOST_CHECK_THROW(converter.to_number(""), std::invalid_argument);
    BOOST_CHECK_THROW(converter.to_number("@"), std::invalid_argument);
    BOOST_CHECK_THROW(converter.to_number("negative"), std::invalid_argument);
    BOOST_CHECK_THROW(converter.to_number("gazillion"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(convert_fundamentals)
{
    num::converter_c converter;

    BOOST_CHECK(converter.to_number("zero") == "0");
    BOOST_CHECK(converter.to_numeral("0") == "zero");

    BOOST_CHECK(converter.to_number("one") == "1");
    BOOST_CHECK(converter.to_numeral("1") == "one");

    BOOST_CHECK(converter.to_number("eleven") == "11");
    BOOST_CHECK(converter.to_numeral("11") == "eleven");

    BOOST_CHECK(converter.to_number("thirteen") == "13");
    BOOST_CHECK(converter.to_numeral("13") == "thirteen");

    BOOST_CHECK(converter.to_number("twenty") == "20");
    BOOST_CHECK(converter.to_numeral("20") == "twenty");

    BOOST_CHECK(converter.to_number("twenty-one") == "21");
    BOOST_CHECK(converter.to_numeral("21") == "twenty-one");

    BOOST_CHECK(converter.to_number("minus fifty-six") == "-56");
    BOOST_CHECK(converter.to_numeral("-56") == "negative fifty-six");

    BOOST_CHECK(converter.to_number("negative sixty-six") == "-66");
    BOOST_CHECK(converter.to_numeral("-66") == "negative sixty-six");
}

BOOST_AUTO_TEST_CASE(convert_hundreds)
{
    num::converter_c converter;

    BOOST_CHECK(converter.to_number("hundred") == "100");
    BOOST_CHECK(converter.to_number("a hundred") == "100");
    BOOST_CHECK(converter.to_number("one hundred") == "100");
    BOOST_CHECK(converter.to_numeral("100") == "one hundred");

    BOOST_CHECK(converter.to_number("nineteen hundred") == "1,900");
    BOOST_CHECK(converter.to_numeral("1,900") == "one thousand nine hundred");
}

BOOST_AUTO_TEST_CASE(convert_thousands)
{
    num::converter_c converter;

    BOOST_CHECK(converter.to_number("one thousand") == "1,000");
    BOOST_CHECK(converter.to_numeral("1,000") == "one thousand");

    BOOST_CHECK(converter.to_number("twelve thousand") == "12,000");
    BOOST_CHECK(converter.to_numeral("12,000") == "twelve thousand");
}

BOOST_AUTO_TEST_CASE(convert_latin_roots)
{
    num::converter_c converter;
    
    BOOST_CHECK(converter.to_number("one million") == "1,000,000");
    BOOST_CHECK(converter.to_numeral("1,000,000") == "one million");

    BOOST_CHECK(converter.to_number("one thousand million") == "1,000,000,000");
    BOOST_CHECK(converter.to_numeral("1,000,000,000") == "one billion");

    BOOST_CHECK(converter.to_number("two billion") == "2,000,000,000");
    BOOST_CHECK(converter.to_numeral("2,000,000,000") == "two billion");

    BOOST_CHECK(converter.to_number("two thousand billion") == "2,000,000,000,000");
    BOOST_CHECK(converter.to_numeral("2,000,000,000,000") == "two trillion");

    BOOST_CHECK(converter.to_number("three trillion") == "3,000,000,000,000");
    BOOST_CHECK(converter.to_numeral("3,000,000,000,000") == "three trillion");

    BOOST_CHECK(converter.to_number("fifteen quindecillion") == "15,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000");
    BOOST_CHECK(converter.to_numeral("15,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000") == "fifteen quindecillion");

    BOOST_CHECK(converter.to_number("twenty-three trevigintillion") == "23,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000");
    BOOST_CHECK(converter.to_numeral("23,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000") == "twenty-three trevigintillion");

    BOOST_CHECK(converter.to_number("seventy-eight octoseptuagintillion") == "78,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000");
    BOOST_CHECK(converter.to_numeral("78,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000") == "seventy-eight octoseptuagintillion");

    BOOST_CHECK(converter.to_number("hundred centillion") == "100,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000");
    BOOST_CHECK(converter.to_numeral("100,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000") == "one hundred centillion");
}

BOOST_AUTO_TEST_CASE(convert_long_scale)
{
    num::conversion_options_t options;
    options.naming_system = num::naming_system_t::long_scale;
    num::converter_c converter(options);
    
    BOOST_CHECK(converter.to_number("one milliard") == "1,000,000,000");
    BOOST_CHECK(converter.to_numeral("1,000,000,000") == "one milliard");

    BOOST_CHECK(converter.to_number("two billion") == "2,000,000,000,000");
    BOOST_CHECK(converter.to_numeral("2,000,000,000,000") == "two billion");

    BOOST_CHECK(converter.to_number("four quadrilliard") == "4,000,000,000,000,000,000,000,000,000");
    BOOST_CHECK(converter.to_numeral("4,000,000,000,000,000,000,000,000,000") == "four quadrilliard");
}

BOOST_AUTO_TEST_CASE(convert_scientic_notation)
{
    num::converter_c converter;
    
    BOOST_CHECK(converter.to_numeral("1e3") == "one thousand");
    BOOST_CHECK(converter.to_numeral("1e27") == "one octillion");
    BOOST_CHECK(converter.to_numeral("1.23e6") == "one million two hundred thirty thousand");
}

BOOST_AUTO_TEST_CASE(convert_decimals)
{
    num::converter_c converter;
    converter.conversion_options().force_leading_zero = false;
    
    BOOST_CHECK(converter.to_number("point zero six two five") == "0.0625");
    BOOST_CHECK(converter.to_numeral("0.0625") == "point zero six two five");

    converter.conversion_options().force_leading_zero = true;

    BOOST_CHECK(converter.to_numeral("0.0625") == "zero point zero six two five");

    BOOST_CHECK(converter.to_number("three point one four one five nine two six") == "3.1415926");
    BOOST_CHECK(converter.to_numeral("3.1415926") == "three point one four one five nine two six");
}

BOOST_AUTO_TEST_CASE(convert_complex_examples)
{
    num::converter_c converter;
    
    BOOST_CHECK(converter.to_number("twelve million eighty-three thousand fifty-six") == "12,083,056");
    BOOST_CHECK(converter.to_numeral("12,083,056") == "twelve million eighty-three thousand fifty-six");

    BOOST_CHECK(converter.to_number("nine hundred ninety-nine thousand eleven") == "999,011");
    BOOST_CHECK(converter.to_numeral("999,011") == "nine hundred ninety-nine thousand eleven");
    
    converter.conversion_options().use_thousands_separators = false;

    BOOST_CHECK(converter.to_number("twelve million eighty-three thousand fifty-six") == "12083056");
    BOOST_CHECK(converter.to_number("nine hundred ninety-nine thousand eleven") == "999011");
}

BOOST_AUTO_TEST_CASE(convert_logic_errors)
{
    num::converter_c converter;
    
    BOOST_CHECK_THROW(converter.to_number("six thousand fourty-four million"), std::logic_error);
    BOOST_CHECK_THROW(converter.to_number("six thousand twenty thousand ten"), std::logic_error);
    BOOST_CHECK_THROW(converter.to_number("six thousand seventeen hundred"), std::logic_error);
    BOOST_CHECK_THROW(converter.to_number("four million thousand"), std::logic_error);
}
