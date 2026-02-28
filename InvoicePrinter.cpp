#include "InvoicePrinter.h"

inline void error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data)
{
    std::cerr << "[LibHaru ERROR] Code: " << std::hex << error_no
              << " Detail: " << detail_no << std::dec << std::endl;

    throw std::runtime_error("LibHaru PDF generation failed.");
}

// ==================== Product ====================

Product::Product() : name(""), unit_price(0), quantity(0), weight(0) {}

Product::Product(const string &name, double price, int qty) : name(name), unit_price(price), quantity(qty), weight(0) {}

Product::Product(const string &name, const string &batch, double price, int qty)
    : name(name), batch_no(batch), unit_price(price), quantity(qty), weight(0) {}

// Getters
string Product::get_name() const { return name; }
ll Product::get_unit_price() const { return unit_price; }
ll Product::get_quantity() const { return quantity; }
ll Product::get_weight() const { return weight; }
ll Product::get_subtotal() const { return unit_price * quantity; }
string Product::get_batch_no() const { return batch_no; }

void Product::set_batch_no(const string &batch) { batch_no = batch; }

// Increases the weight
void Product::add_weight()
{
    weight++;

    return;
}

// Formatted product output string
string Product::to_string() const
{
    ostringstream oss;
    oss << left << setw(20) << name
        << setw(12) << batch_no // Add batch_no column
        << right << setw(5) << quantity
        << setw(10) << unit_price
        << setw(15) << get_subtotal();

    return oss.str();
}
// ==================== Bill ====================

Bill::Bill(ll id, const string &customer) : billID(id), customer_name(customer)
{
    time_t now = time(nullptr);
    date_issued = ctime(&now);
    if (!date_issued.empty() && date_issued.back() == '\n')
        date_issued.pop_back();

    // filename = customer_name + "_" + date_issued.substr(4, 3) + date_issued.substr(8, 2) + date_issued.substr(20, 4) + ".pdf";
    filename = std::to_string(billID) + ".pdf";
}

ll Bill::get_bill_id() const
{
    return billID;
}

void Bill::add_product(const Product &p)
{
    products.push_back(p);
}

ll Bill::get_total() const
{
    ll total = 0;
    for (const auto &p : products)
        total += p.get_subtotal();
    return total;
}

string Bill::to_string() const
{
    ostringstream oss;
    oss << "--------------------------------------------------\n";
    oss << "Bill ID: " << billID << "\n";
    oss << "Customer: " << customer_name << "\n";
    oss << "Date: " << date_issued << "\n";
    oss << "--------------------------------------------------\n";
    oss << left << setw(20) << "Item"
        << right << setw(5) << "Qty"
        << setw(10) << "Price"
        << setw(15) << "Subtotal" << "\n";
    oss << left << "--------------------------------------------------\n";

    for (auto &p : products)
        oss << p.to_string() << "\n";

    oss << "--------------------------------------------------\n";
    oss << left << setw(30) << "Total: "
        << right << setw(15) << get_total() << " dong\n";
    oss << "--------------------------------------------------\n";
    return oss.str();
}

string Bill::get_filename() const
{
    return filename;
}

const vector<Product> &Bill::get_products() const
{
    return products;
}

string Bill::get_customer_name() const
{
    return customer_name;
}

void Bill::export_pdf()
{
    HPDF_Doc pdf = HPDF_New(error_handler, NULL);
    if (!pdf)
    {
        cerr << "Error: Cannot create PDF object\n";
        return;
    }

    try
    {
        // Helper: format integer values with thousand separators (e.g., 1234567 -> "1,234,567")
        auto format_with_commas = [](ll value) -> string
        {
            string s = std::to_string(value);
            string out;
            int count = 0;

            for (int i = static_cast<int>(s.size()) - 1; i >= 0; --i)
            {
                out.push_back(s[i]);
                count++;
                if (count == 3 && i != 0)
                {
                    out.push_back(',');
                    count = 0;
                }
            }

            std::reverse(out.begin(), out.end());
            return out;
        };

        // ===== ENABLE UTF-8 ENCODING FOR VIETNAMESE SUPPORT =====
        HPDF_UseUTFEncodings(pdf);

        // Add first page
        HPDF_Page page = HPDF_AddPage(pdf);
        HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);

        // Page metrics
        HPDF_REAL page_width = HPDF_Page_GetWidth(page);
        HPDF_REAL page_height = HPDF_Page_GetHeight(page);
        HPDF_REAL margin = 50.0;
        HPDF_REAL bottom_margin = 80.0; // Extra space at bottom for totals
        HPDF_REAL usable_width = page_width - 2 * margin;
        HPDF_REAL y_pos = page_height - margin;

        // ===== LOAD TRUETYPE FONTS FOR VIETNAMESE SUPPORT =====
        const char *font_regular_path = "Roboto-Regular.ttf";
        const char *font_bold_path = "Roboto-Bold.ttf";

        // Load TrueType fonts with embedding enabled
        const char *font_regular_name = HPDF_LoadTTFontFromFile(pdf, font_regular_path, HPDF_TRUE);
        const char *font_bold_name = HPDF_LoadTTFontFromFile(pdf, font_bold_path, HPDF_TRUE);

        if (!font_regular_name || !font_bold_name)
        {
            cerr << "Error: Cannot load TrueType fonts.\n";
            cerr << "Make sure Roboto-Regular.ttf and Roboto-Bold.ttf are in the executable directory.\n";
            cerr << "Current working directory: " << filesystem::current_path() << endl;
            HPDF_Free(pdf);
            return;
        }

        // Get font objects with UTF-8 encoding
        HPDF_Font font = HPDF_GetFont(pdf, font_regular_name, "UTF-8");
        HPDF_Font font_bold = HPDF_GetFont(pdf, font_bold_name, "UTF-8");

        if (!font || !font_bold)
        {
            cerr << "Error: Cannot get font objects with UTF-8 encoding\n";
            HPDF_Free(pdf);
            return;
        }

        // Column layout (scales with A4 width)
        // Columns: Item | Batch No. | Qty | Price | Subtotal
        HPDF_REAL col_item_w = usable_width * 0.45;
        HPDF_REAL col_batch_w = usable_width * 0.15;
        HPDF_REAL col_qty_w = usable_width * 0.10;
        HPDF_REAL col_price_w = usable_width * 0.15;
        HPDF_REAL col_subtotal_w = usable_width * 0.15;

        HPDF_REAL col_item_x = margin;
        HPDF_REAL col_batch_x = col_item_x + col_item_w;
        HPDF_REAL col_qty_x = col_batch_x + col_batch_w;
        HPDF_REAL col_price_x = col_qty_x + col_qty_w;
        HPDF_REAL col_subtotal_x = col_price_x + col_price_w;

        // Utility: draw a horizontal line
        auto draw_line = [&](HPDF_REAL y)
        {
            HPDF_Page_SetLineWidth(page, 0.5);
            HPDF_Page_MoveTo(page, margin, y);
            HPDF_Page_LineTo(page, margin + usable_width, y);
            HPDF_Page_Stroke(page);
        };

        // Utility: draw table header
        auto draw_table_header = [&]()
        {
            HPDF_Page_BeginText(page);
            HPDF_Page_SetFontAndSize(page, font_bold, 11);

            // Item (left)
            HPDF_Page_TextOut(page, col_item_x + 2, y_pos, "Item");

            // Batch No. (center aligned within its column)
            const char *batch_txt = "Batch No.";
            HPDF_REAL batch_w = HPDF_Page_TextWidth(page, batch_txt);
            HPDF_REAL batch_x = col_batch_x + (col_batch_w - batch_w) / 2;
            HPDF_Page_TextOut(page, batch_x, y_pos, batch_txt);

            // Qty (right aligned)
            const char *qty_txt = "Qty";
            HPDF_REAL qty_w = HPDF_Page_TextWidth(page, qty_txt);
            HPDF_Page_TextOut(page, col_qty_x + col_qty_w - qty_w - 2, y_pos, qty_txt);

            // Price (right aligned)
            const char *price_txt = "Price";
            HPDF_REAL price_w = HPDF_Page_TextWidth(page, price_txt);
            HPDF_Page_TextOut(page, col_price_x + col_price_w - price_w - 2, y_pos, price_txt);

            // Subtotal (right aligned)
            const char *sub_txt = "Subtotal";
            HPDF_REAL sub_w = HPDF_Page_TextWidth(page, sub_txt);
            HPDF_Page_TextOut(page, col_subtotal_x + col_subtotal_w - sub_w - 2, y_pos, sub_txt);

            HPDF_Page_EndText(page);
            y_pos -= 15;
            draw_line(y_pos);
            y_pos -= 18;
        };

        // Utility: create new page with header
        auto create_new_page = [&]()
        {
            page = HPDF_AddPage(pdf);
            HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);
            y_pos = page_height - margin;

            // Draw continuation header
            HPDF_Page_BeginText(page);
            HPDF_Page_SetFontAndSize(page, font_bold, 11);
            string continuation = "Bill ID: " + std::to_string(billID) + " (Continued)";
            HPDF_Page_TextOut(page, margin, y_pos, continuation.c_str());
            HPDF_Page_EndText(page);
            y_pos -= 20;

            draw_line(y_pos);
            y_pos -= 15;

            // Redraw table header on new page
            draw_table_header();
        };

        // ========== Header block (first page only) ==========
        HPDF_Page_BeginText(page);
        HPDF_Page_SetFontAndSize(page, font_bold, 12);

        string bill_header = "Bill ID: " + std::to_string(billID);
        HPDF_Page_TextOut(page, margin, y_pos, bill_header.c_str());
        y_pos -= 20;

        string cust_header = "Customer: " + customer_name;
        HPDF_Page_TextOut(page, margin, y_pos, cust_header.c_str());
        y_pos -= 20;

        string date_header = "Date: " + date_issued;
        HPDF_Page_TextOut(page, margin, y_pos, date_header.c_str());
        HPDF_Page_EndText(page);

        y_pos -= 15;
        draw_line(y_pos);
        y_pos -= 25;

        // ========== Table header ==========
        draw_table_header();

        // ========== Table body ==========
        HPDF_Page_SetFontAndSize(page, font, 10);
        const HPDF_REAL row_spacing = 18.0;

        for (const auto &prod : products)
        {
            // Check if we need a new page (need space for row + bottom margin)
            if (y_pos < bottom_margin)
            {
                create_new_page();
                HPDF_Page_SetFontAndSize(page, font, 10); // Reset font after new page
            }

            HPDF_Page_BeginText(page);

            // Item name (left aligned, truncated if too long)
            string name = prod.get_name();

            // Calculate text width for UTF-8 strings
            HPDF_REAL name_w = HPDF_Page_TextWidth(page, name.c_str());

            // Truncate if necessary
            if (name_w > col_item_w - 6)
            {
                string truncated = name;
                while (!truncated.empty() && HPDF_Page_TextWidth(page, (truncated + "...").c_str()) > col_item_w - 6)
                {
                    size_t len = truncated.length();
                    if (len > 0)
                    {
                        truncated.pop_back();

                        // If we removed part of a multi-byte character, keep removing
                        while (!truncated.empty() && (truncated.back() & 0xC0) == 0x80)
                        {
                            truncated.pop_back();
                        }
                    }
                }
                name = truncated + "...";
            }

            HPDF_Page_TextOut(page, col_item_x + 2, y_pos, name.c_str());

            // Batch No. (right aligned in its column; empty string stays blank)
            string batch = prod.get_batch_no();
            if (!batch.empty())
            {
                HPDF_REAL batch_width = HPDF_Page_TextWidth(page, batch.c_str());
                HPDF_Page_TextOut(page, col_batch_x + col_batch_w - batch_width - 2, y_pos, batch.c_str());
            }

            // Qty (right aligned)
            string qty_str = std::to_string(prod.get_quantity());
            HPDF_REAL qty_width = HPDF_Page_TextWidth(page, qty_str.c_str());
            HPDF_Page_TextOut(page, col_qty_x + col_qty_w - qty_width - 2, y_pos, qty_str.c_str());

            // Price (right aligned)
            string price_str = format_with_commas(prod.get_unit_price());
            HPDF_REAL price_width = HPDF_Page_TextWidth(page, price_str.c_str());
            HPDF_Page_TextOut(page, col_price_x + col_price_w - price_width - 2, y_pos, price_str.c_str());

            // Subtotal (right aligned)
            string subtotal_str = format_with_commas(prod.get_subtotal());
            HPDF_REAL subtotal_width = HPDF_Page_TextWidth(page, subtotal_str.c_str());
            HPDF_Page_TextOut(page, col_subtotal_x + col_subtotal_w - subtotal_width - 2, y_pos, subtotal_str.c_str());

            HPDF_Page_EndText(page);
            y_pos -= row_spacing;
        }

        // ========== Total section (always needs space for 3 lines) ==========
        // Check if we have space for total section (need ~60 points)
        if (y_pos < (margin + 60))
        {
            create_new_page();
        }

        y_pos -= 5;
        draw_line(y_pos);
        y_pos -= 20;

        // ========== Total line ==========
        HPDF_Page_BeginText(page);
        HPDF_Page_SetFontAndSize(page, font_bold, 11);

        const char *total_lbl = "Total:";
        HPDF_Page_TextOut(page, col_price_x + 2, y_pos, total_lbl);

        string total_str = format_with_commas(get_total()) + " đồng";
        HPDF_REAL total_w = HPDF_Page_TextWidth(page, total_str.c_str());
        HPDF_Page_TextOut(page, col_subtotal_x + col_subtotal_w - total_w - 2, y_pos, total_str.c_str());

        HPDF_Page_EndText(page);

        y_pos -= 15;
        draw_line(y_pos);

        // Save PDF
        HPDF_SaveToFile(pdf, filename.c_str());
        cout << "\nBill exported successfully to " << filename << endl;
        cout << "Total products: " << products.size() << endl;
    }
    catch (const exception &e)
    {
        cerr << "Error during PDF export: " << e.what() << endl;
        HPDF_Free(pdf);
        throw;
    }

    HPDF_Free(pdf);
}

// ==================== PDF Import Methods ====================

bool Bill::is_valid_pdf_file(const string &file_path)
{
    // Check if file exists and has .pdf extension
    if (!filesystem::exists(file_path))
    {
        return false;
    }

    string extension = filesystem::path(file_path).extension().string();
    transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    return extension == ".pdf";
}

// Replace BOTH extract_text_from_pdf AND parse_bill_data in InvoicePrinter.cpp:

string Bill::extract_text_from_pdf(const string &pdf_path)
{
    // Create temporary text file
    string temp_file = "temp_pdf_text.txt";

    // Use pdftotext with -layout option to preserve formatting
    // -layout preserves the original physical layout
    string command = "pdftotext -layout \"" + pdf_path + "\" \"" + temp_file + "\"";

    int result = system(command.c_str());
    if (result != 0)
    {
        // Try without -layout option as fallback
        command = "pdftotext \"" + pdf_path + "\" \"" + temp_file + "\"";
        result = system(command.c_str());

        if (result != 0)
        {
            throw runtime_error("Failed to extract text from PDF. Make sure pdftotext is installed.");
        }
    }

    // Read the extracted text
    ifstream file(temp_file);
    if (!file.is_open())
    {
        throw runtime_error("Failed to read extracted text file.");
    }

    string text((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    // Clean up temporary file
    remove(temp_file.c_str());

    return text;
}

bool Bill::parse_bill_data(const string &text, ll &bill_id, string &customer_name, vector<Product> &products)
{
    try
    {
        cout << "\n===== PARSING BILL DATA =====" << endl;

        // Parse Bill ID - multiple pattern attempts
        bill_id = 0;

        vector<regex> bill_patterns = {
            regex(R"(Bill\s*ID\s*:\s*(\d+))", regex_constants::icase),
            regex(R"(Bill\s*#\s*:\s*(\d+))", regex_constants::icase),
            regex(R"(Invoice\s*#\s*:\s*(\d+))", regex_constants::icase),
            regex(R"(Invoice\s*ID\s*:\s*(\d+))", regex_constants::icase),
            regex(R"(ID\s*:\s*(\d+))", regex_constants::icase),
            regex(R"(#\s*(\d+))", regex_constants::icase),
            regex(R"(Invoice\s*(\d+))", regex_constants::icase),
            regex(R"(Bill\s*(\d+))", regex_constants::icase)};

        smatch match;
        for (const auto &pattern : bill_patterns)
        {
            if (regex_search(text, match, pattern))
            {
                bill_id = stoll(match[1].str());
                cout << "Found Bill ID: " << bill_id << endl;
                break;
            }
        }

        if (bill_id == 0)
        {
            cerr << "Failed to parse Bill ID" << endl;
            return false;
        }

        // Parse Customer Name
        vector<regex> customer_patterns = {
            regex(R"(Customer\s*:\s*([^\n\r]+))", regex_constants::icase),
            regex(R"(Client\s*:\s*([^\n\r]+))", regex_constants::icase),
            regex(R"(Name\s*:\s*([^\n\r]+))", regex_constants::icase)};

        customer_name = "Unknown Customer";
        for (const auto &pattern : customer_patterns)
        {
            if (regex_search(text, match, pattern))
            {
                customer_name = match[1].str();
                // Trim whitespace
                customer_name.erase(0, customer_name.find_first_not_of(" \t\r\n"));
                customer_name.erase(customer_name.find_last_not_of(" \t\r\n") + 1);
                cout << "Found Customer: " << customer_name << endl;
                break;
            }
        }

        // Find table section between header and total
        size_t header_pos = string::npos;

        // Look for table header (our exported PDF uses "Item" header)
        vector<string> header_markers = {"Item", "Product", "Description"};
        for (const auto &marker : header_markers)
        {
            header_pos = text.find(marker);
            if (header_pos != string::npos)
            {
                cout << "Found table header at position: " << header_pos << endl;
                break;
            }
        }

        if (header_pos == string::npos)
        {
            cerr << "Failed to find table header" << endl;
            return false;
        }

        // Find where table ends
        size_t total_pos = text.find("Total:", header_pos);
        if (total_pos == string::npos)
        {
            total_pos = text.find("TOTAL:", header_pos);
        }
        if (total_pos == string::npos)
        {
            total_pos = text.find("total:", header_pos);
        }

        if (total_pos == string::npos)
        {
            cerr << "Failed to find total line" << endl;
            return false;
        }

        // Extract table content
        string table_section = text.substr(header_pos, total_pos - header_pos);
        cout << "\nTable section extracted (" << table_section.length() << " chars)" << endl;

        // Split into lines and parse
        istringstream stream(table_section);
        string line;
        products.clear();

        auto trim = [](string &s)
        {
            if (s.empty())
                return;
            s.erase(0, s.find_first_not_of(" \t\r\n"));
            if (!s.empty())
                s.erase(s.find_last_not_of(" \t\r\n") + 1);
        };

        // Regex that matches our exported layout with Batch column:
        // ItemName  [2+ spaces] BatchNo  [2+ spaces] Qty  [2+ spaces] Price  [2+ spaces] Subtotal
        // Note: Price and Subtotal may contain thousand separators (commas)
        regex re_with_batch(R"(^(.+?)\s{2,}(.+?)\s{2,}(\d+)\s{2,}([\d,]+)\s{2,}([\d,]+)\s*$)");

        // Fallback regex without Batch column: ItemName  Qty  Price  Subtotal
        regex re_no_batch(R"(^(.+?)\s{2,}(\d+)\s{2,}([\d,]+)\s{2,}([\d,]+)\s*$)");

        bool header_passed = false;

        // Now parse remaining lines as table rows
        while (getline(stream, line))
        {
            string original = line;

            // Trim for quick empty check
            trim(line);
            if (line.empty())
                continue;

            // Skip the header line(s) once
            if (!header_passed &&
                (line.find("Item") != string::npos ||
                 line.find("Product") != string::npos) &&
                line.find("Qty") != string::npos &&
                line.find("Price") != string::npos)
            {
                header_passed = true;
                cout << "Skipping table header line: " << line << endl;
                continue;
            }

            if (!header_passed)
                continue;

            smatch m;

            // First try format WITH batch column
            if (regex_match(original, m, re_with_batch))
            {
                string name_field = m[1].str();
                string batch_field = m[2].str();
                string qty_field = m[3].str();
                string price_field = m[4].str();
                string subtotal_field = m[5].str();

                trim(name_field);
                trim(batch_field);
                trim(qty_field);
                trim(price_field);
                trim(subtotal_field);

                if (name_field.empty())
                {
                    cout << "Skipping line (no product name): " << original << endl;
                    continue;
                }

                try
                {
                    // Remove thousand separators if any
                    qty_field.erase(remove(qty_field.begin(), qty_field.end(), ','), qty_field.end());
                    price_field.erase(remove(price_field.begin(), price_field.end(), ','), price_field.end());
                    subtotal_field.erase(remove(subtotal_field.begin(), subtotal_field.end(), ','), subtotal_field.end());

                    int quantity = stoi(qty_field);
                    ll unit_price = stoll(price_field);
                    ll subtotal = stoll(subtotal_field);

                    if (abs(subtotal - (unit_price * quantity)) > 1)
                    {
                        cout << "Skipping line (subtotal mismatch): " << original << endl;
                        continue;
                    }

                    products.push_back(Product(name_field, batch_field, unit_price, quantity));
                    cout << "✓ Parsed (with batch): " << name_field << " | Batch: " << batch_field
                         << " | Qty: " << quantity << " | Price: " << unit_price
                         << " | Subtotal: " << subtotal << endl;
                }
                catch (...)
                {
                    cout << "Skipping line (numeric parse failed): " << original << endl;
                    continue;
                }
            }
            // Then try format WITHOUT batch column
            else if (regex_match(original, m, re_no_batch))
            {
                string name_field = m[1].str();
                string qty_field = m[2].str();
                string price_field = m[3].str();
                string subtotal_field = m[4].str();

                trim(name_field);
                trim(qty_field);
                trim(price_field);
                trim(subtotal_field);

                if (name_field.empty())
                {
                    cout << "Skipping line (no product name): " << original << endl;
                    continue;
                }

                try
                {
                    qty_field.erase(remove(qty_field.begin(), qty_field.end(), ','), qty_field.end());
                    price_field.erase(remove(price_field.begin(), price_field.end(), ','), price_field.end());
                    subtotal_field.erase(remove(subtotal_field.begin(), subtotal_field.end(), ','), subtotal_field.end());

                    int quantity = stoi(qty_field);
                    ll unit_price = stoll(price_field);
                    ll subtotal = stoll(subtotal_field);

                    if (abs(subtotal - (unit_price * quantity)) > 1)
                    {
                        cout << "Skipping line (subtotal mismatch): " << original << endl;
                        continue;
                    }

                    products.push_back(Product(name_field, "", unit_price, quantity));
                    cout << "✓ Parsed (no batch): " << name_field
                         << " | Qty: " << quantity << " | Price: " << unit_price
                         << " | Subtotal: " << subtotal << endl;
                }
                catch (...)
                {
                    cout << "Skipping line (numeric parse failed): " << original << endl;
                    continue;
                }
            }
            else
            {
                cout << "Skipping line (no matching format): " << original << endl;
            }
        }

        if (products.empty())
        {
            cerr << "\nERROR: No products found in PDF!" << endl;
            return false;
        }

        cout << "\n✓ Successfully parsed " << products.size() << " product(s)" << endl;
        cout << "============================\n"
             << endl;
        return true;
    }
    catch (const exception &e)
    {
        cerr << "Error parsing bill data: " << e.what() << endl;
        return false;
    }
}

bool Bill::parse_gui_format(const string &text, vector<Product> &products)
{
    // Look for GUI table format: "Product Name    Quantity    Unit Price (₫)    Total (₫)"
    // But we need to find the actual table, not the form input
    size_t table_start = text.find("Product Name    Quantity    Unit Price");
    if (table_start == string::npos)
        return false;

    // Find where table ends (at SUMMARY or TOTAL)
    size_t table_end = text.find("SUMMARY", table_start);
    if (table_end == string::npos)
        table_end = text.find("TOTAL", table_start);
    if (table_end == string::npos)
        return false;

    // Extract table section
    string table_section = text.substr(table_start, table_end - table_start);

    // Split into lines
    istringstream stream(table_section);
    string line;
    bool header_passed = false;

    while (getline(stream, line))
    {
        // Trim the line
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        // Skip empty lines
        if (line.empty())
            continue;

        // Skip header lines
        if (!header_passed &&
            (line.find("Product Name") != string::npos ||
             line.find("Tên hàng") != string::npos ||
             line.find("Quantity") != string::npos ||
             line.find("Unit Price") != string::npos))
        {
            header_passed = true;
            continue;
        }

        if (!header_passed)
            continue;

        // Try to parse GUI format line: "Product A       1           1,000 ₫          1,000 ₫      Delete"
        // Look for pattern: product_name + spaces + quantity + spaces + price_with_currency + spaces + total_with_currency + extra_text
        regex gui_product_regex(R"((\S+(?:\s+\S+)*?)\s+(\d+)\s+([\d,]+)\s*₫\s+([\d,]+)\s*₫.*)");
        smatch match;

        if (regex_match(line, match, gui_product_regex))
        {
            string name = match[1].str();
            int quantity = stoi(match[2].str());

            // Extract unit price from the price string (remove commas)
            string price_str = match[3].str();
            price_str.erase(remove(price_str.begin(), price_str.end(), ','), price_str.end());
            price_str.erase(0, price_str.find_first_not_of(" \t"));
            price_str.erase(price_str.find_last_not_of(" \t") + 1);
            ll unit_price = stoll(price_str);

            // Trim product name
            name.erase(0, name.find_first_not_of(" \t"));
            name.erase(name.find_last_not_of(" \t") + 1);

            products.push_back(Product(name, unit_price, quantity));
            cout << "Parsed GUI product: " << name << " | Qty: " << quantity
                 << " | Price: " << unit_price << endl;
        }
    }

    return !products.empty();
}

bool Bill::parse_simple_format(const string &text, vector<Product> &products)
{
    // Find the table section - look for header line
    size_t table_start = text.find("Item");
    if (table_start == string::npos)
        return false;

    // Find where table ends (at Total line)
    size_t table_end = text.find("Total:", table_start);
    if (table_end == string::npos)
        return false;

    // Extract table section
    string table_section = text.substr(table_start, table_end - table_start);

    // Split into lines
    istringstream stream(table_section);
    string line;
    bool header_passed = false;

    while (getline(stream, line))
    {
        // Trim the line
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        // Skip empty lines
        if (line.empty())
            continue;

        // Skip header line (contains "Item", "Qty", "Price", "Subtotal")
        if (!header_passed &&
            (line.find("Item") != string::npos ||
             line.find("Qty") != string::npos ||
             line.find("Price") != string::npos))
        {
            header_passed = true;
            continue;
        }

        if (!header_passed)
            continue;

        // Try to parse product line
        // Expected format: ProductName Quantity UnitPrice Subtotal
        regex product_regex(R"(^(.+?)\s+(\d+)\s+(\d+)\s+(\d+)\s*$)");
        smatch match;

        if (regex_match(line, match, product_regex))
        {
            string name = match[1].str();
            int quantity = stoi(match[2].str());
            ll unit_price = stoll(match[3].str());

            // Trim product name
            name.erase(0, name.find_first_not_of(" \t"));
            name.erase(name.find_last_not_of(" \t") + 1);

            products.push_back(Product(name, unit_price, quantity));
            cout << "Parsed simple product: " << name << " | Qty: " << quantity
                 << " | Price: " << unit_price << endl;
        }
        else
        {
            // Try alternative parsing for lines with irregular spacing
            istringstream line_stream(line);
            vector<string> tokens;
            string token;

            while (line_stream >> token)
            {
                tokens.push_back(token);
            }

            // Need at least 4 tokens: name, qty, price, subtotal
            if (tokens.size() >= 4)
            {
                // Last 3 tokens should be numbers
                try
                {
                    ll subtotal = stoll(tokens.back());
                    tokens.pop_back();

                    ll unit_price = stoll(tokens.back());
                    tokens.pop_back();

                    int quantity = stoi(tokens.back());
                    tokens.pop_back();

                    // Remaining tokens are the product name
                    string name;
                    for (size_t i = 0; i < tokens.size(); ++i)
                    {
                        if (i > 0)
                            name += " ";
                        name += tokens[i];
                    }

                    if (!name.empty())
                    {
                        products.push_back(Product(name, unit_price, quantity));
                        cout << "Parsed simple product (alt): " << name << " | Qty: " << quantity
                             << " | Price: " << unit_price << endl;
                    }
                }
                catch (...)
                {
                    // Not a valid product line, skip
                    continue;
                }
            }
        }
    }

    return !products.empty();
}

Bill *Bill::import_from_pdf(const string &pdf_path)
{
    try
    {
        // Validate PDF file
        if (!is_valid_pdf_file(pdf_path))
        {
            throw invalid_argument("Invalid PDF file: " + pdf_path);
        }

        // Extract text from PDF
        string text = extract_text_from_pdf(pdf_path);

        // Parse bill data
        ll bill_id;
        string customer_name;
        vector<Product> products;

        if (!parse_bill_data(text, bill_id, customer_name, products))
        {
            throw runtime_error("Failed to parse bill data from PDF. Invalid format.");
        }

        // Create new Bill object
        Bill *imported_bill = new Bill(bill_id, customer_name);

        // Add products
        for (const auto &product : products)
        {
            imported_bill->add_product(product);
        }

        return imported_bill;
    }
    catch (const exception &e)
    {
        cerr << "Error importing PDF: " << e.what() << endl;
        return nullptr;
    }
}