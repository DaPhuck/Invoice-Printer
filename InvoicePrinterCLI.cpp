#include "invoiceprinter.h"

// Error handler function
void error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data)
{
    fprintf(stderr, "ERROR: %04x %04x\n", (unsigned int)error_no, (unsigned int)detail_no);
}

int main()
{
    ll bill_id;
    string cust_name;

    cout << "=============================\n";
    cout << "     INVOICE PRINTER CLI     \n";
    cout << "=============================\n";

    cout << "Enter Bill ID: ";
    cin >> bill_id;
    cin.ignore();

    cout << "Enter Customer Name: ";
    getline(cin, cust_name);

    // Bill automatically sets the date using ctime()
    Bill my_bill(bill_id, cust_name);

    int choice;
    while (true)
    {
        cout << "\n--- MAIN MENU ---\n";
        cout << "1. Add product\n";
        cout << "2. Preview bill\n";
        cout << "3. End bill & export as PDF\n";
        cout << "Enter choice: ";
        cin >> choice;

        if (choice == 1)
        {
            string name;
            ll price, quantity;

            cin.ignore();
            cout << "Enter product name: ";
            getline(cin, name);
            cout << "Enter price (in VND): ";
            cin >> price;
            cout << "Enter quantity: ";
            cin >> quantity;

            Product new_product(name, price, quantity);
            my_bill.add_product(new_product);
            cout << "Product added successfully!\n";
        }
        else if (choice == 2)
        {
            cout << "\n===== BILL SUMMARY =====\n";
            cout << my_bill.to_string();
            cout << "=========================\n";
        }
        else if (!(choice - 3))
        {
            my_bill.export_pdf();
            return 0;
        }
        else
        {
            cout << "Invalid choice. Try again.\n";
        }
    }

    return 0;
}