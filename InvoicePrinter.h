#ifndef INVOICEPRINTER_H
#define INVOICEPRINTER_H

#include <iostream>
#include <string>
#include <cmath>
#include <ctime>
#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>
#include "hpdf.h"
#include "hpdf_types.h"
#include <regex>
#include <cstdlib>
#include <filesystem>

using namespace std;

typedef long long ll;
typedef unsigned long long ull;
typedef long double ld;

inline void error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data);

// ___ Product class ___
class Product
{
private:
    string name;
    string category;
    string batch_no; // NEW FIELD - Add this line
    ll unit_price;
    ll quantity;
    ll weight = 0;

public:
    Product();
    Product(const string &name, double price, int qty);
    Product(const string &name, const string &batch, double price, int qty); // NEW CONSTRUCTOR

    // Getters
    string get_name() const;
    string get_batch_no() const; // NEW GETTER - Add this line
    ll get_unit_price() const;
    ll get_quantity() const;
    ll get_weight() const;
    ll get_subtotal() const;

    // Setters
    void set_batch_no(const string &batch); // NEW SETTER - Add this line

    // Utility
    void add_weight();
    string to_string() const;
};

// ___ Bill class ___
class Bill
{
private:
    ll billID;
    string customer_name;
    string date_issued;
    vector<Product> products;
    string filename;

public:
    Bill(ll id, const string &customer);

    ll get_bill_id() const;
    void add_product(const Product &p);
    ll get_total() const;
    string to_string() const;
    string get_filename() const;
    const vector<Product> &get_products() const;
    string get_customer_name() const;

    // PDF Export functionality
    void export_pdf();

    // PDF Import functionality
    static Bill *import_from_pdf(const string &pdf_path);
    static string extract_text_from_pdf(const string &pdf_path);
    static bool parse_bill_data(const string &text, ll &bill_id, string &customer_name, vector<Product> &products);
    static bool parse_gui_format(const string &text, vector<Product> &products);
    static bool parse_simple_format(const string &text, vector<Product> &products);
    static bool is_valid_pdf_file(const string &file_path);
};

#endif