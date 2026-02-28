#define CATCH_CONFIG_MAIN
#include <filesystem>
#include "catch_amalgamated.hpp"
#include "invoiceprinter.h"

using Catch::Approx;
namespace fs = filesystem;

TEST_CASE("Product class functionality")
{
    Product p("Paracetamol", 500, 2);

    SECTION("Initial values")
    {
        REQUIRE(p.get_name() == "Paracetamol");
        REQUIRE(p.get_unit_price() == 500);
        REQUIRE(p.get_quantity() == 2);
        REQUIRE(p.get_weight() == 0);
        REQUIRE(p.get_subtotal() == 1000);
    }

    SECTION("Adding weight")
    {
        p.add_weight();
        REQUIRE(p.get_weight() == 1);
        p.add_weight();
        REQUIRE(p.get_weight() == 2);
    }
}

TEST_CASE("Bill class functionality")
{
    Bill b(101, "mvan");
    Product p1("Paracetamol", 500, 2);
    Product p2("Vitamin C", 1200, 1);

    SECTION("Add products and compute total")
    {
        b.add_product(p1);
        b.add_product(p2);

        REQUIRE(b.get_total() == 1000 + 1200);
    }

    SECTION("to_string contains product names and totals")
    {
        b.add_product(p1);
        b.add_product(p2);

        string bill_str = b.to_string();
        REQUIRE(bill_str.find("Paracetamol") != ::string::npos);
        REQUIRE(bill_str.find("Vitamin C") != string::npos);
        REQUIRE(bill_str.find("2200") != string::npos); // total
    }
}

bool file_exists_in_dir(const string &filename)
{
    return fs::exists(fs::current_path() / filename);
}

TEST_CASE("PDF export functionality")
{
    Bill b(001, "mvan");
    Product p1("Paracetamol", 500, 2);
    Product p2("Vitamin C", 1200, 1);

    b.export_pdf();

    REQUIRE(file_exists_in_dir(b.get_filename()));
}
