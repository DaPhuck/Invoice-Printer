#include "InvoicePrinter.h"
#include "SetupWindow.h"
#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QMessageBox>
#include <QDateTime>
#include <QFont>
#include <QFrame>
#include <QFileDialog>
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QListWidget>
#include <QKeyEvent>
#include <QTimer>
#include <vector>
#include <algorithm>

class InvoicePrinterGUI : public QMainWindow
{
    Q_OBJECT

protected:
    void focusOutEvent(QFocusEvent *event) override
    {
        hide_suggestions();
        QMainWindow::focusOutEvent(event);
    }

    void changeEvent(QEvent *event) override
    {
        if (event->type() == QEvent::ActivationChange)
        {
            if (!isActiveWindow())
            {
                hide_suggestions();
            }
        }
        QMainWindow::changeEvent(event);
    }

    bool eventFilter(QObject *obj, QEvent *event) override
    {
        // Handle product name input with suggestions
        if (obj == product_name_input)
        {
            if (event->type() == QEvent::FocusOut)
            {
                QTimer::singleShot(150, this, [this]()
                                   {
                if (!suggestion_list->hasFocus())
                {
                    hide_suggestions();
                } });
            }
            else if (event->type() == QEvent::KeyPress && suggestion_list_visible)
            {
                QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

                switch (keyEvent->key())
                {
                case Qt::Key_Down:
                    if (suggestion_list->currentRow() < suggestion_list->count() - 1)
                    {
                        suggestion_list->setCurrentRow(suggestion_list->currentRow() + 1);
                    }
                    return true;

                case Qt::Key_Up:
                    if (suggestion_list->currentRow() > 0)
                    {
                        suggestion_list->setCurrentRow(suggestion_list->currentRow() - 1);
                    }
                    return true;

                case Qt::Key_Return:
                case Qt::Key_Enter:
                    if (suggestion_list->currentItem())
                    {
                        on_suggestion_clicked(suggestion_list->currentItem());
                        return true;
                    }
                    break;

                case Qt::Key_Escape:
                    hide_suggestions();
                    return true;

                case Qt::Key_Tab:
                    hide_suggestions();
                    batch_no_input->setFocus();
                    return true;

                default:
                    break;
                }
            }
        }

        // Handle quantity_input (QSpinBox)
        else if (obj == quantity_input)
        {
            if (event->type() == QEvent::KeyPress)
            {
                QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
                if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
                {
                    // Move to unit price input
                    unit_price_input->setFocus();
                    unit_price_input->selectAll();
                    return true; // Event handled
                }
            }
        }

        // Handle unit_price_input (QDoubleSpinBox)
        else if (obj == unit_price_input)
        {
            if (event->type() == QEvent::KeyPress)
            {
                QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
                if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
                {
                    // Add the item when Enter is pressed
                    add_item();
                    return true; // Event handled
                }
            }
        }

        return QMainWindow::eventFilter(obj, event);
    }

private:
    // Core data
    Bill *current_bill;
    ll current_bill_ID;
    QString invoice_ID_file;

    // Database
    QVector<Product> product_database;
    QString database_file_path; // Track current database file path

    // UI Components - Header
    QLabel *invoice_number_label;
    QLabel *date_label;
    QLineEdit *customer_name_input;

    // UI Components - Product Entry
    QLineEdit *product_name_input;
    QLineEdit *batch_no_input;
    QSpinBox *quantity_input;
    QDoubleSpinBox *unit_price_input;
    QPushButton *add_button;

    // UI Components - Items Table
    QTableWidget *items_table;

    // UI Components - Summary Panel
    QLabel *item_count_label;
    QLabel *subtotal_label;
    QLabel *total_amount_label;

    // UI Components - Action Buttons
    QPushButton *print_button;

    // Menu actions
    QAction *reset_ID_action;

    // Autocomplete feature
    QListWidget *suggestion_list;
    bool suggestion_list_visible;

public:
    InvoicePrinterGUI(QWidget *parent = nullptr) : QMainWindow(parent)
    {
        invoice_ID_file = "invoice_counter.txt";
        current_bill = nullptr;
        current_bill_ID = load_invoice_ID();

        QSettings settings("setup.ini", QSettings::IniFormat);
        database_file_path = settings.value("database_path", "").toString();

        load_database();

        suggestion_list = nullptr;
        suggestion_list_visible = false;

        setWindowTitle("Invoice Printer");
        setMinimumSize(1200, 800);

        setup_UI();
        setup_menu_bar();
        setup_connections();
        new_invoice();

        // Install event filter on product name input
        product_name_input->installEventFilter(this);

        if (product_database.isEmpty())
        {
            statusBar()->showMessage("Ready / Sẵn sàng - No database loaded / Không có cơ sở dữ liệu");
        }
        else
        {
            statusBar()->showMessage(QString("Ready / Sẵn sàng - Database loaded: %1 products / Đã tải %1 sản phẩm")
                                         .arg(product_database.size()));
        }
    }

    ~InvoicePrinterGUI()
    {
        delete current_bill;
    }

private:
    // ==================== DATABASE MANAGEMENT METHODS ====================

    void sort_database()
    {
        // Sort database by product name using std::sort
        // Time Complexity: O(n log n) - optimal for sorting
        std::sort(product_database.begin(), product_database.end(),
                  [](const Product &a, const Product &b)
                  {
                      return QString::fromStdString(a.get_name()).toLower() <
                             QString::fromStdString(b.get_name()).toLower();
                  });
    }

    int binary_search_product(const QString &product_name)
    {
        // Binary search for EXACT product match in sorted database
        // Time Complexity: O(log n) - fastest search for sorted data
        // Returns index if found, -1 if not found
        // PURPOSE: Used to check if a product exists in database (for unsaved products feature)

        int left = 0;
        int right = product_database.size() - 1;
        QString search_name = product_name.toLower().trimmed();

        while (left <= right)
        {
            int mid = left + (right - left) / 2;
            QString mid_name = QString::fromStdString(product_database[mid].get_name()).toLower().trimmed();

            if (mid_name == search_name)
            {
                return mid; // Found exact match
            }
            else if (mid_name < search_name)
            {
                left = mid + 1;
            }
            else
            {
                right = mid - 1;
            }
        }

        return -1; // Not found
    }

    QVector<Product> search_products_by_prefix(const QString &prefix)
    {
        // Search for products that START WITH the prefix
        // Using BINARY SEARCH to find the first match, then linear scan for all matches
        // Then SORT BY WEIGHT (frequency) with PENALTY for products already in current bill
        // Time Complexity: O(log n + k + k log k + m) where n = database size, k = matches, m = bill items
        // PURPOSE: Weighted autocomplete that avoids suggesting duplicate products
        // NOTE: Weight/frequency is used for sorting ONLY, not displayed to user

        QVector<Product> matches;
        QString search_text = prefix.toLower().trimmed();

        if (search_text.isEmpty() || product_database.isEmpty())
            return matches;

        // Build a set of products already in the current bill for fast lookup
        // Time Complexity: O(m) where m = number of items in bill
        QSet<QString> products_in_bill;
        if (current_bill)
        {
            const auto &bill_products = current_bill->get_products();
            for (const auto &product : bill_products)
            {
                products_in_bill.insert(QString::fromStdString(product.get_name()).toLower());
            }
        }

        // BINARY SEARCH: Find the first product that starts with the prefix
        int left = 0;
        int right = product_database.size();

        while (left < right)
        {
            int mid = left + (right - left) / 2;
            QString mid_name = QString::fromStdString(product_database[mid].get_name()).toLower();

            if (mid_name < search_text)
            {
                left = mid + 1;
            }
            else
            {
                right = mid;
            }
        }

        int first_match = left;

        // LINEAR SCAN: Collect ALL consecutive matches
        for (int i = first_match; i < product_database.size(); ++i)
        {
            QString product_name = QString::fromStdString(product_database[i].get_name()).toLower();

            if (product_name.startsWith(search_text))
            {
                matches.append(product_database[i]);
            }
            else
            {
                break;
            }
        }

        // WEIGHTED SORTING with DUPLICATION PENALTY:
        // - Products NOT in bill: sorted by weight (descending)
        // - Products IN bill: pushed to bottom, then sorted by weight
        // This ensures new products appear first, avoiding accidental duplicates
        std::sort(matches.begin(), matches.end(),
                  [&products_in_bill](const Product &a, const Product &b)
                  {
                      QString name_a = QString::fromStdString(a.get_name()).toLower();
                      QString name_b = QString::fromStdString(b.get_name()).toLower();

                      bool a_in_bill = products_in_bill.contains(name_a);
                      bool b_in_bill = products_in_bill.contains(name_b);

                      if (a_in_bill != b_in_bill)
                      {
                          return !a_in_bill; // true (not in bill) comes before false (in bill)
                      }

                      if (a.get_weight() != b.get_weight())
                      {
                          return a.get_weight() > b.get_weight();
                      }

                      return name_a < name_b;
                  });

        if (matches.size() > 10)
        {
            matches.resize(10);
        }

        return matches;
    }

    QVector<Product> find_unsaved_products(const vector<Product> &bill_products)
    {
        QVector<Product> unsaved_products;

        // Ensure database is sorted before binary search
        sort_database();

        // Use binary search to find products not in database
        // Time Complexity: O(m log n) where m = bill products, n = database size
        for (const auto &product : bill_products)
        {
            QString product_name = QString::fromStdString(product.get_name());

            // Binary search: O(log n)
            if (binary_search_product(product_name) == -1)
            {
                // Product not found in database
                unsaved_products.append(product);
            }
        }

        return unsaved_products;
    }

    bool save_database_to_file(const QString &file_path)
    {
        QFile file(file_path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QMessageBox::critical(this, "Error / Lỗi",
                                  QString("Cannot open file for writing:\n%1").arg(file_path));
            return false;
        }

        QTextStream out(&file);

        // Write header
        out << "name,weight\n";

        // Write all products in the database
        for (const auto &product : product_database)
        {
            QString name = QString::fromStdString(product.get_name());
            ll weight = product.get_weight();

            // Escape commas in product names by wrapping in quotes
            if (name.contains(','))
            {
                name = "\"" + name + "\"";
            }

            out << name << "," << weight << "\n";
        }

        file.close();
        return true;
    }

    void prompt_save_new_products(const QVector<Product> &unsaved_products)
    {
        if (unsaved_products.isEmpty())
        {
            return; // No unsaved products
        }

        // Create dialog showing unsaved products
        QDialog dialog(this);
        dialog.setWindowTitle("Save New Products / Lưu sản phẩm mới");
        dialog.setMinimumSize(600, 400);
        dialog.setModal(true);

        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        layout->setSpacing(15);
        layout->setContentsMargins(20, 20, 20, 20);

        // Title
        QLabel *title = new QLabel(QString("Found %1 new product(s) not in database\n"
                                           "Tìm thấy %1 sản phẩm mới chưa có trong cơ sở dữ liệu")
                                       .arg(unsaved_products.size()));
        title->setStyleSheet("font-weight: bold; font-size: 11pt; color: #2196F3; padding: 10px;");
        title->setAlignment(Qt::AlignCenter);
        layout->addWidget(title);

        // List of unsaved products
        QTableWidget *products_table = new QTableWidget(unsaved_products.size(), 3);
        products_table->setHorizontalHeaderLabels({"Product Name\nTên sản phẩm",
                                                   "Quantity\nSố lượng",
                                                   "Unit Price\nĐơn giá"});
        products_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        products_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        products_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        products_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        products_table->setSelectionMode(QAbstractItemView::NoSelection);
        products_table->setAlternatingRowColors(true);
        products_table->verticalHeader()->setVisible(false);

        for (int i = 0; i < unsaved_products.size(); ++i)
        {
            const Product &p = unsaved_products[i];

            products_table->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(p.get_name())));

            QTableWidgetItem *qty_item = new QTableWidgetItem(QString::number(p.get_quantity()));
            qty_item->setTextAlignment(Qt::AlignCenter);
            products_table->setItem(i, 1, qty_item);

            QTableWidgetItem *price_item = new QTableWidgetItem(format_currency(p.get_unit_price()));
            price_item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            products_table->setItem(i, 2, price_item);
        }

        layout->addWidget(products_table);

        // Question
        QLabel *question = new QLabel("Do you want to add these products to the database?\n"
                                      "Bạn có muốn thêm các sản phẩm này vào cơ sở dữ liệu?");
        question->setStyleSheet("font-size: 10pt; margin: 10px; color: #666;");
        question->setAlignment(Qt::AlignCenter);
        layout->addWidget(question);

        // Buttons
        QHBoxLayout *button_layout = new QHBoxLayout();
        button_layout->setSpacing(10);

        QPushButton *yes_button = new QPushButton("Yes, Save Them\nCó, lưu lại");
        QPushButton *no_button = new QPushButton("No, Skip\nKhông");

        yes_button->setMinimumHeight(50);
        no_button->setMinimumHeight(50);

        yes_button->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; border-radius: 5px; padding: 10px; } QPushButton:hover { background-color: #45a049; }");
        no_button->setStyleSheet("QPushButton { background-color: #f44336; color: white; font-weight: bold; border-radius: 5px; padding: 10px; } QPushButton:hover { background-color: #d32f2f; }");

        button_layout->addWidget(yes_button);
        button_layout->addWidget(no_button);
        layout->addLayout(button_layout);

        connect(yes_button, &QPushButton::clicked, &dialog, &QDialog::accept);
        connect(no_button, &QPushButton::clicked, &dialog, &QDialog::reject);

        if (dialog.exec() == QDialog::Accepted)
        {
            // User wants to save the products
            QString save_path = database_file_path;

            // If no database path exists, ask user to select one
            if (save_path.isEmpty() || !QFile::exists(save_path))
            {
                save_path = QFileDialog::getSaveFileName(
                    this,
                    "Select Database File Location / Chọn vị trí lưu cơ sở dữ liệu",
                    "product_database.csv",
                    "CSV Files (*.csv);;All Files (*)");

                if (save_path.isEmpty())
                {
                    QMessageBox::information(this, "Cancelled / Đã hủy",
                                             "Database save cancelled\nĐã hủy lưu cơ sở dữ liệu");
                    return;
                }

                // Update database path in settings
                database_file_path = save_path;
                QSettings settings("setup.ini", QSettings::IniFormat);
                settings.setValue("database_path", save_path);
                settings.sync();
            }

            // Add unsaved products to the in-memory database
            // Set weight to 1 for all new products
            for (const auto &product : unsaved_products)
            {
                // Create a new product with weight = 0 initially
                Product new_product(product.get_name(), product.get_unit_price(), 0);

                // Set weight to 1 (represents initial frequency)
                new_product.add_weight();

                product_database.append(new_product);
            }

            // Sort the database by name (prepares for future binary searches)
            sort_database();

            // Save to CSV file
            if (save_database_to_file(save_path))
            {
                QMessageBox::information(this, "Success / Thành công",
                                         QString("Successfully added %1 product(s) to database!\n"
                                                 "Đã thêm thành công %1 sản phẩm vào cơ sở dữ liệu!\n\n"
                                                 "Database file: %2\n"
                                                 "Total products: %3")
                                             .arg(unsaved_products.size())
                                             .arg(save_path)
                                             .arg(product_database.size()));

                statusBar()->showMessage(QString("Database updated: %1 products / Đã cập nhật: %1 sản phẩm")
                                             .arg(product_database.size()),
                                         5000);
            }
            else
            {
                QMessageBox::warning(this, "Warning / Cảnh báo",
                                     "Products were added to memory but failed to save to file.\n"
                                     "Đã thêm sản phẩm vào bộ nhớ nhưng không thể lưu vào file.");
            }
        }
    }

    // ==================== END DATABASE METHODS ====================

    void load_database()
    {
        product_database.clear();

        QSettings settings("setup.ini", QSettings::IniFormat);
        QString database_path = settings.value("database_path", "").toString();

        if (database_path.isEmpty() || !QFile::exists(database_path))
        {
            qDebug() << "No database file configured or file does not exist";
            return;
        }

        QFile file(database_path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QMessageBox::warning(this, "Database Error / Lỗi cơ sở dữ liệu",
                                 QString("Cannot open database file:\n%1").arg(database_path));
            return;
        }

        QTextStream in(&file);
        bool first_line = true;
        int line_number = 0;
        int loaded_count = 0;

        while (!in.atEnd())
        {
            QString line = in.readLine().trimmed();
            line_number++;

            // Skip empty lines
            if (line.isEmpty())
                continue;

            // Skip header line (first line with "name,weight")
            if (first_line)
            {
                first_line = false;
                if (line.toLower().contains("name") && line.toLower().contains("weight"))
                {
                    continue;
                }
            }

            // Parse CSV line: name,weight
            // Handle quoted product names (if they contain commas)
            QStringList parts;
            if (line.startsWith("\""))
            {
                // Extract quoted name
                int quote_end = line.indexOf("\"", 1);
                if (quote_end != -1)
                {
                    parts.append(line.mid(1, quote_end - 1));
                    QString remaining = line.mid(quote_end + 1).trimmed();
                    if (remaining.startsWith(","))
                    {
                        parts.append(remaining.mid(1).trimmed());
                    }
                }
            }
            else
            {
                parts = line.split(',');
            }

            if (parts.size() >= 2)
            {
                QString name = parts[0].trimmed();
                QString weight_str = parts[1].trimmed();

                bool ok;
                double weight = weight_str.toDouble(&ok);

                if (ok && !name.isEmpty())
                {
                    // Create Product with name and weight
                    // unit_price = 0, quantity = 0 for database entries
                    Product p(name.toStdString(), 0, 0);

                    product_database.append(p);
                    loaded_count++;
                }
                else
                {
                    qWarning() << "Invalid data at line" << line_number << ":" << line;
                }
            }
            else
            {
                qWarning() << "Invalid format at line" << line_number << ":" << line;
            }
        }

        file.close();

        // Sort database after loading for efficient binary search
        sort_database();
    }

    ll load_invoice_ID()
    {
        QFile file(invoice_ID_file);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QTextStream in(&file);
            QString idStr = in.readLine();
            file.close();
            bool ok;
            ll id = idStr.toLongLong(&ok);
            if (ok && id > 0)
            {
                return id;
            }
        }
        return 1;
    }

    void saveInvoiceId(ll id)
    {
        QFile file(invoice_ID_file);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&file);
            out << id;
            file.close();
        }
    }

    void resetInvoiceId()
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Reset Invoice ID / Đặt lại số hóa đơn",
                                      "Are you sure you want to reset invoice ID to 0001?\n"
                                      "Bạn có chắc muốn đặt lại số hóa đơn về 0001?",
                                      QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes)
        {
            current_bill_ID = 1;
            saveInvoiceId(1);
            invoice_number_label->setText(QString("Invoice #: %1").arg(QString::number(current_bill_ID).rightJustified(4, '0')));
            QMessageBox::information(this, "Success / Thành công",
                                     "Invoice ID has been reset to 0001\n"
                                     "Số hóa đơn đã được đặt lại về 0001");
        }
    }

    void setup_menu_bar()
    {
        QMenuBar *menuBar = new QMenuBar(this);
        setMenuBar(menuBar);

        QMenu *file_menu = menuBar->addMenu("&File / Tệp");

        reset_ID_action = new QAction("Reset Invoice ID / Đặt lại số hóa đơn", this);
        file_menu->addAction(reset_ID_action);

        file_menu->addSeparator();

        QAction *reset_setup_action = new QAction("Reset Setup / Đặt lại thiết lập", this);
        file_menu->addAction(reset_setup_action);
        connect(reset_setup_action, &QAction::triggered, this, &InvoicePrinterGUI::reset_setup);

        file_menu->addSeparator();

        QAction *import_action = new QAction("Import PDF Invoice / Nhập hóa đơn PDF", this);
        import_action->setShortcut(QKeySequence::Open);
        file_menu->addAction(import_action);
        connect(import_action, &QAction::triggered, this, &InvoicePrinterGUI::import_pdf_invoice);

        file_menu->addSeparator();

        QAction *reload_db_action = new QAction("Reload Database / Tải lại cơ sở dữ liệu", this);
        file_menu->addAction(reload_db_action);
        connect(reload_db_action, &QAction::triggered, this, &InvoicePrinterGUI::reload_database);

        file_menu->addSeparator();

        QAction *exit_action = new QAction("E&xit / Thoát", this);
        exit_action->setShortcut(QKeySequence::Quit);
        file_menu->addAction(exit_action);
        connect(exit_action, &QAction::triggered, this, &QWidget::close);
    }

    void reload_database()
    {
        load_database();

        if (product_database.isEmpty())
        {
            statusBar()->showMessage("No database loaded / Không có cơ sở dữ liệu", 5000);
        }
        else
        {
            statusBar()->showMessage(QString("Database reloaded: %1 products / Đã tải lại: %1 sản phẩm")
                                         .arg(product_database.size()),
                                     5000);
        }
    }

    void setup_UI()
    {
        // Create central widget
        QWidget *central_widget = new QWidget(this);
        setCentralWidget(central_widget);

        QVBoxLayout *main_layout = new QVBoxLayout(central_widget);
        main_layout->setSpacing(10);
        main_layout->setContentsMargins(10, 10, 10, 10);

        // ===== TOP HEADER SECTION =====
        QFrame *header_frame = new QFrame();
        header_frame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
        header_frame->setStyleSheet("QFrame { background-color: #2196F3; padding: 5px; border-radius: 5px; }");

        QHBoxLayout *header_layout = new QHBoxLayout(header_frame);
        header_layout->setSpacing(15);

        // Left: Invoice info
        QVBoxLayout *invoice_info_layout = new QVBoxLayout();
        invoice_info_layout->setSpacing(2);

        QLabel *title_label = new QLabel("INVOICE / HÓA ĐƠN");
        QFont title_font = title_label->font();
        title_font.setPointSize(14);
        title_font.setBold(true);
        title_label->setFont(title_font);
        title_label->setStyleSheet("color: white;");

        invoice_number_label = new QLabel(QString("Invoice #: %1").arg(QString::number(current_bill_ID).rightJustified(4, '0')));
        invoice_number_label->setStyleSheet("color: white; font-size: 11pt;");

        date_label = new QLabel("Date: " + QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm"));
        date_label->setStyleSheet("color: white; font-size: 9pt;");

        invoice_info_layout->addWidget(title_label);
        invoice_info_layout->addWidget(invoice_number_label);
        invoice_info_layout->addWidget(date_label);

        // Right: Customer info
        QVBoxLayout *customer_info_layout = new QVBoxLayout();
        customer_info_layout->setSpacing(2);

        QLabel *customer_label = new QLabel("Customer / Khách hàng:");
        customer_label->setStyleSheet("color: white; font-size: 10pt; font-weight: bold;");

        customer_name_input = new QLineEdit();
        customer_name_input->setPlaceholderText("Enter customer name...");
        customer_name_input->setMinimumHeight(28);
        customer_name_input->setMaximumWidth(400);
        customer_name_input->setStyleSheet("QLineEdit { background-color: white; color: black; border-radius: 3px; padding: 3px; }");

        customer_info_layout->addWidget(customer_label);
        customer_info_layout->addWidget(customer_name_input);

        header_layout->addLayout(invoice_info_layout);
        header_layout->addStretch();
        header_layout->addLayout(customer_info_layout);

        main_layout->addWidget(header_frame);

        // ===== PRODUCT ENTRY SECTION =====
        QGroupBox *entry_group = new QGroupBox("Add Product / Thêm sản phẩm");
        entry_group->setStyleSheet("QGroupBox { font-weight: bold; border: 2px solid #cccccc; border-radius: 5px; margin-top: 10px; padding-top: 10px; }");

        QHBoxLayout *entry_layout = new QHBoxLayout();

        QLabel *name_label2 = new QLabel("Product Name / Tên mặt hàng:");
        product_name_input = new QLineEdit();
        product_name_input->setPlaceholderText("Enter product name / Nhập tên sản phẩm...");
        product_name_input->setMinimumHeight(35);
        product_name_input->setMinimumWidth(250);

        // Setup autocomplete list widget
        suggestion_list = new QListWidget(this);
        suggestion_list->setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
        suggestion_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        suggestion_list->setFocusPolicy(Qt::NoFocus);
        suggestion_list->setMaximumHeight(200);
        suggestion_list->setMaximumWidth(400);
        suggestion_list->setStyleSheet(
            "QListWidget { "
            "border: 2px solid #2196F3; "
            "border-radius: 5px; "
            "background-color: white; "
            "padding: 5px; "
            "color: black; "
            "}"
            "QListWidget::item { "
            "padding: 8px; "
            "border-radius: 3px; "
            "color: black; "
            "}"
            "QListWidget::item:hover { "
            "background-color: #E3F2FD; "
            "color: black; "
            "}"
            "QListWidget::item:selected { "
            "background-color: #2196F3; "
            "color: white; "
            "}");
        suggestion_list->hide();
        suggestion_list_visible = false;

        QLabel *batch_label = new QLabel("Batch No. / Số lô:");
        batch_no_input = new QLineEdit();
        // batch_no_input->setPlaceholderText("Optional / Tùy chọn");
        batch_no_input->setMinimumHeight(35);
        batch_no_input->setMinimumWidth(100);

        QLabel *qty_label = new QLabel("Quantity / Số lượng:");
        quantity_input = new QSpinBox();
        quantity_input->installEventFilter(this);
        quantity_input->setMinimum(1);
        quantity_input->setMaximum(10000);
        quantity_input->setValue(1);
        quantity_input->setMinimumHeight(35);
        quantity_input->setMinimumWidth(100);

        QLabel *price_label = new QLabel("Unit Price / Đơn giá:");
        unit_price_input = new QDoubleSpinBox();
        unit_price_input->installEventFilter(this);
        unit_price_input->setMinimum(0);
        unit_price_input->setMaximum(999999999.99);
        unit_price_input->setDecimals(0);
        unit_price_input->setMinimumHeight(35);
        unit_price_input->setMinimumWidth(150);
        unit_price_input->setSuffix(" ₫");

        add_button = new QPushButton("ADD\nTHÊM");
        add_button->setMinimumHeight(50);
        add_button->setMinimumWidth(100);
        add_button->setDefault(false);
        add_button->setAutoDefault(false);
        add_button->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; font-size: 10pt; border-radius: 5px; padding: 5px; } QPushButton:hover { background-color: #45a049; }");

        entry_layout->addWidget(name_label2);
        entry_layout->addWidget(product_name_input);
        entry_layout->addWidget(batch_label);
        entry_layout->addWidget(batch_no_input);
        entry_layout->addWidget(qty_label);
        entry_layout->addWidget(quantity_input);
        entry_layout->addWidget(price_label);
        entry_layout->addWidget(unit_price_input);
        entry_layout->addWidget(add_button);

        entry_group->setLayout(entry_layout);
        main_layout->addWidget(entry_group);
        setTabOrder(product_name_input, batch_no_input);
        setTabOrder(batch_no_input, quantity_input);
        setTabOrder(quantity_input, unit_price_input);
        setTabOrder(unit_price_input, add_button);
        setTabOrder(add_button, customer_name_input);

        // ===== MIDDLE SECTION: TABLE + SUMMARY =====
        QHBoxLayout *middle_layout = new QHBoxLayout();

        // Left: Items table
        QGroupBox *table_group = new QGroupBox("Item List / Danh sách hàng hóa");
        table_group->setStyleSheet("QGroupBox { font-weight: bold; border: 2px solid #cccccc; border-radius: 5px; margin-top: 10px; padding-top: 10px; }");
        QVBoxLayout *table_layout = new QVBoxLayout();

        items_table = new QTableWidget(0, 5);
        items_table->setHorizontalHeaderLabels({"Product Name\nTên mặt hàng",
                                                "Batch No.\nSố lô",
                                                "Quantity\nSố lượng",
                                                "Unit Price (₫)\nĐơn giá",
                                                "Total (₫)\nThành tiền",
                                                "Action\nThao tác"});

        items_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
        items_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
        items_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);
        items_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive);
        items_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
        items_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
        items_table->setColumnWidth(0, 200);
        items_table->setColumnWidth(1, 100);
        items_table->setColumnWidth(5, 120);

        items_table->setSelectionBehavior(QAbstractItemView::SelectRows);
        items_table->setEditTriggers(QAbstractItemView::DoubleClicked |
                                     QAbstractItemView::EditKeyPressed);
        items_table->setAlternatingRowColors(true);
        items_table->verticalHeader()->setVisible(false);

        QFont tableFont = items_table->font();
        tableFont.setPointSize(10);
        items_table->setFont(tableFont);
        items_table->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: #2196F3; color: white; font-weight: bold; padding: 5px; }");

        table_layout->addWidget(items_table);
        table_group->setLayout(table_layout);

        connect(items_table, &QTableWidget::itemChanged, this, &InvoicePrinterGUI::on_table_item_changed);

        // Right: Summary panel
        QFrame *summary_frame = new QFrame();
        summary_frame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
        summary_frame->setStyleSheet("QFrame { border: 2px solid #cccccc; border-radius: 5px; background-color: #f5f5f5; }");
        summary_frame->setMinimumWidth(350);
        summary_frame->setMaximumWidth(400);

        QVBoxLayout *summary_layout = new QVBoxLayout(summary_frame);

        QLabel *summary_title = new QLabel("SUMMARY / TỔNG KẾT");
        QFont summary_title_font = summary_title->font();
        summary_title_font.setPointSize(12);
        summary_title_font.setBold(true);
        summary_title->setFont(summary_title_font);
        summary_title->setAlignment(Qt::AlignCenter);
        summary_title->setStyleSheet("background-color: #2196F3; color: white; padding: 10px; border-radius: 3px;");
        summary_layout->addWidget(summary_title);

        summary_layout->addSpacing(20);

        // Item count
        QHBoxLayout *item_count_layout = new QHBoxLayout();
        QLabel *item_count_text_label = new QLabel("Items / Số mặt hàng:");
        item_count_text_label->setStyleSheet("font-size: 10pt; color: #000000;");
        item_count_layout->addWidget(item_count_text_label);
        item_count_label = new QLabel("0");
        item_count_label->setStyleSheet("font-size: 10pt; color: #000000;");
        item_count_layout->addStretch();
        item_count_layout->addWidget(item_count_label);
        summary_layout->addLayout(item_count_layout);

        summary_layout->addSpacing(10);
        summary_layout->addWidget(create_separator());
        summary_layout->addSpacing(10);

        // Subtotal
        QHBoxLayout *subtotal_layout = new QHBoxLayout();
        QLabel *subtotal_text_label = new QLabel("Subtotal / Tạm tính:");
        subtotal_text_label->setStyleSheet("font-size: 10pt; color: #000000;");
        subtotal_layout->addWidget(subtotal_text_label);
        subtotal_label = new QLabel("0 ₫");
        subtotal_label->setStyleSheet("font-size: 10pt; color: #000000; min-width: 120px; text-align: right;");
        subtotal_label->setAlignment(Qt::AlignRight);
        subtotal_layout->addStretch();
        subtotal_layout->addWidget(subtotal_label);
        summary_layout->addLayout(subtotal_layout);

        summary_layout->addSpacing(15);
        summary_layout->addWidget(create_separator());
        summary_layout->addSpacing(15);

        // Total amount
        QHBoxLayout *total_layout = new QHBoxLayout();
        QLabel *total_text_label = new QLabel("TOTAL / TỔNG CỘNG:");
        total_text_label->setStyleSheet("font-size: 11pt; font-weight: bold; color: #000000;");
        total_layout->addWidget(total_text_label);
        total_amount_label = new QLabel("0 ₫");
        total_amount_label->setStyleSheet("font-size: 14pt; font-weight: bold; color: #4CAF50; min-width: 150px;");
        total_amount_label->setAlignment(Qt::AlignRight);
        total_layout->addStretch();
        total_layout->addWidget(total_amount_label);
        summary_layout->addLayout(total_layout);

        summary_layout->addStretch();

        middle_layout->addWidget(table_group, 2);
        middle_layout->addWidget(summary_frame, 1);

        main_layout->addLayout(middle_layout, 1);

        // ===== BOTTOM ACTION BUTTON =====
        QHBoxLayout *action_layout = new QHBoxLayout();
        action_layout->setSpacing(10);

        print_button = new QPushButton("END INVOICE && PRINT\nKẾT THÚC && IN HÓA ĐƠN");
        print_button->setMinimumHeight(70);
        print_button->setDefault(false);
        print_button->setAutoDefault(false);
        print_button->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; font-size: 12pt; border-radius: 5px; } QPushButton:hover { background-color: #388E3C; }");

        action_layout->addWidget(print_button);

        main_layout->addLayout(action_layout);

        // Status bar
        statusBar()->setStyleSheet("QStatusBar { border-top: 1px solid #cccccc; }");
    }

    QFrame *create_separator()
    {
        QFrame *line = new QFrame();
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        line->setStyleSheet("background-color: #cccccc;");
        return line;
    }

    void setup_connections()
    {
        // Add button - use clicked signal only
        connect(add_button, &QPushButton::clicked, this, &InvoicePrinterGUI::add_item);

        // Print button
        connect(print_button, &QPushButton::clicked, this, &InvoicePrinterGUI::end_invoice_and_print);

        // Menu actions
        connect(reset_ID_action, &QAction::triggered, this, &InvoicePrinterGUI::resetInvoiceId);

        // Product name input - move to batch_no when no suggestions
        connect(product_name_input, &QLineEdit::returnPressed, this, [this]()
                {
        if (!suggestion_list_visible) {
            batch_no_input->setFocus();
        } });

        // Batch no input - move to quantity
        connect(batch_no_input, &QLineEdit::returnPressed, this, [this]()
                {
        quantity_input->setFocus();
        quantity_input->selectAll(); });

        // NOTE: quantity_input and unit_price_input use eventFilter instead of signals
        // because QSpinBox/QDoubleSpinBox don't have returnPressed signal

        // Product name text changes - show suggestions
        connect(product_name_input, &QLineEdit::textChanged, this, &InvoicePrinterGUI::on_product_name_changed);

        // Suggestion list item clicked
        connect(suggestion_list, &QListWidget::itemClicked, this, &InvoicePrinterGUI::on_suggestion_clicked);

        // Hide suggestions when clicking elsewhere
        connect(qApp, &QApplication::focusChanged, this, [this](QWidget *old, QWidget *now)
                {
        if (now != product_name_input && now != suggestion_list)
        {
            hide_suggestions();
        } });
    }

    void import_pdf_invoice()
    {
        QString fileName = QFileDialog::getOpenFileName(
            this,
            "Import Invoice PDF / Nhập hóa đơn PDF",
            "",
            "PDF Files (*.pdf);;All Files (*)");

        if (fileName.isEmpty())
        {
            return;
        }

        try
        {
            Bill *imported_bill = Bill::import_from_pdf(fileName.toStdString());

            if (!imported_bill)
            {
                QMessageBox::warning(this, "Import Error / Lỗi nhập file",
                                     "Failed to import PDF. Please check if the file is a valid invoice.\n"
                                     "Không thể nhập PDF. Vui lòng kiểm tra file có phải hóa đơn hợp lệ.");
                return;
            }

            items_table->setRowCount(0);

            delete current_bill;
            current_bill = imported_bill;

            ll imported_bill_id = imported_bill->get_bill_id();
            customer_name_input->setText(QString::fromStdString(imported_bill->get_customer_name()));

            invoice_number_label->setText(QString("Invoice #: %1 (Imported)")
                                              .arg(QString::number(imported_bill_id).rightJustified(4, '0')));

            const auto &products = imported_bill->get_products();
            for (const auto &product : products)
            {
                add_item_to_table(
                    QString::fromStdString(product.get_name()),
                    QString::fromStdString(product.get_batch_no()),
                    product.get_quantity(),
                    product.get_unit_price());
            }

            update_summary();

            QMessageBox::information(this, "Import Success / Nhập thành công",
                                     QString("Invoice imported successfully!\n"
                                             "Hóa đơn đã được nhập thành công!\n\n"
                                             "Invoice ID: %1\n"
                                             "File: %2")
                                         .arg(QString::number(imported_bill_id).rightJustified(4, '0'))
                                         .arg(fileName));

            statusBar()->showMessage("PDF imported / Đã nhập PDF: " + fileName, 5000);
        }
        catch (const exception &e)
        {
            QMessageBox::critical(this, "Import Error / Lỗi nhập file",
                                  QString("Failed to import PDF / Không thể nhập PDF:\n%1")
                                      .arg(e.what()));
        }
    }

    void reset_setup()
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Reset Setup / Đặt lại thiết lập",
                                      "This will clear all setup settings and show the setup window on next launch.\n"
                                      "Điều này sẽ xóa tất cả cài đặt và hiển thị cửa sổ thiết lập khi khởi động lại.\n\n"
                                      "Continue?",
                                      QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes)
        {
            QSettings settings("setup.ini", QSettings::IniFormat);
            settings.setValue("first_run_completed", false);
            settings.remove("icon_path");
            settings.remove("database_path");

            QMessageBox::information(this, "Success / Thành công",
                                     "Setup has been reset. The setup window will appear on next launch.\n"
                                     "Đã đặt lại thiết lập. Cửa sổ thiết lập sẽ xuất hiện khi khởi động lại.");
        }
    }

    void add_item()
    {
        QString product_name = product_name_input->text().trimmed();
        QString batch_no = batch_no_input->text().trimmed(); // GET BATCH NO.
        int quantity = quantity_input->value();
        double unit_price = unit_price_input->value();

        if (product_name.isEmpty())
        {
            QMessageBox::warning(this, "Invalid Input", "Please enter a product name\nVui lòng nhập tên sản phẩm");
            product_name_input->setFocus();
            return;
        }

        // REMOVED: Price validation - now allows 0

        if (!current_bill)
        {
            new_invoice();
        }

        // Create product WITH batch_no
        Product p(product_name.toStdString(), batch_no.toStdString(), unit_price, quantity);
        current_bill->add_product(p);

        // Add to table WITH batch_no parameter
        add_item_to_table(product_name, batch_no, quantity, unit_price);

        update_summary();

        // Clear inputs
        product_name_input->clear();
        batch_no_input->clear(); // CLEAR BATCH NO.
        unit_price_input->setValue(0);
        quantity_input->setValue(1);
        product_name_input->setFocus();

        hide_suggestions();

        statusBar()->showMessage("Item added successfully / Đã thêm sản phẩm", 3000);
    }

    void add_item_to_table(const QString &name, const QString &batch_no, int qty, double price)
    {
        // BLOCK SIGNALS to prevent on_table_item_changed from firing during setup
        items_table->blockSignals(true);

        int row = items_table->rowCount();
        items_table->insertRow(row);

        double total = price * qty;

        // Column 0: Product Name
        QTableWidgetItem *name_item = new QTableWidgetItem(name);
        items_table->setItem(row, 0, name_item);

        // Column 1: Batch No. (center aligned)
        QTableWidgetItem *batch_item = new QTableWidgetItem(batch_no);
        batch_item->setTextAlignment(Qt::AlignCenter);
        items_table->setItem(row, 1, batch_item);

        // Column 2: Quantity
        QTableWidgetItem *quantity_item = new QTableWidgetItem(QString::number(qty));
        quantity_item->setTextAlignment(Qt::AlignCenter);
        items_table->setItem(row, 2, quantity_item);

        // Column 3: Unit Price
        QTableWidgetItem *price_item = new QTableWidgetItem(format_currency(price));
        price_item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        items_table->setItem(row, 3, price_item);

        // Column 4: Total (read-only)
        QTableWidgetItem *total_item = new QTableWidgetItem(format_currency(total));
        total_item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        total_item->setFlags(total_item->flags() & ~Qt::ItemIsEditable);
        items_table->setItem(row, 4, total_item);

        // Column 5: Delete button
        QPushButton *remove_button = new QPushButton("Delete\nXóa");
        remove_button->setStyleSheet("QPushButton { background-color: #f44336; color: white; font-weight: bold; border-radius: 3px; padding: 8px; } QPushButton:hover { background-color: #d32f2f; }");
        items_table->setCellWidget(row, 5, remove_button);

        connect(remove_button, &QPushButton::clicked, [this, row]()
                { remove_item(row); });

        // UNBLOCK SIGNALS after all items are added
        items_table->blockSignals(false);
    }

    QString format_currency(ld amount)
    {
        QLocale locale(QLocale::English);
        QString formatted = locale.toString(static_cast<long long>(amount));
        return formatted + " ₫";
    }

    void remove_item(int row)
    {
        if (row < 0 || row >= items_table->rowCount())
            return;

        items_table->removeRow(row);
        rebuild_bill();
        update_summary();

        statusBar()->showMessage("Item removed / Đã xóa sản phẩm", 3000);
    }

    void update_table_from_bill()
    {
        if (!current_bill)
            return;

        // Temporarily disconnect to prevent triggering itemChanged
        disconnect(items_table, &QTableWidget::itemChanged, this, &InvoicePrinterGUI::on_table_item_changed);

        items_table->setRowCount(0);
        const auto &products = current_bill->get_products();

        for (const auto &product : products)
        {
            add_item_to_table(
                QString::fromStdString(product.get_name()),
                QString::fromStdString(product.get_batch_no()),
                product.get_quantity(),
                product.get_unit_price());
        }

        // Reconnect
        connect(items_table, &QTableWidget::itemChanged, this, &InvoicePrinterGUI::on_table_item_changed);
    }

    void rebuild_bill()
    {
        if (!current_bill)
            return;

        string customer_name = customer_name_input->text().toStdString();
        if (customer_name.empty())
            customer_name = "Customer";

        delete current_bill;
        current_bill = new Bill(current_bill_ID, customer_name);

        for (int i = 0; i < items_table->rowCount(); ++i)
        {
            QString name = items_table->item(i, 0)->text();
            QString batch_no = items_table->item(i, 1)->text(); // GET BATCH NO.
            int quantity = items_table->item(i, 2)->text().toInt();

            QString price_str = items_table->item(i, 3)->text();
            price_str.remove(" ₫");
            price_str.remove(",");
            double price = price_str.toDouble();

            // Create product WITH batch_no
            Product p(name.toStdString(), batch_no.toStdString(), price, quantity);
            current_bill->add_product(p);
        }
    }

    void update_summary()
    {
        if (!current_bill)
        {
            item_count_label->setText("0");
            subtotal_label->setText("0 ₫");
            total_amount_label->setText("0 ₫");
            return;
        }

        int item_count = items_table->rowCount();
        item_count_label->setText(QString::number(item_count));

        ld subtotal = (ld)current_bill->get_total();
        subtotal_label->setText(format_currency(subtotal));

        // No discounts: total equals subtotal
        ld total_amount = subtotal;
        total_amount_label->setText(format_currency(total_amount));
    }

    void new_invoice()
    {
        items_table->setRowCount(0);

        delete current_bill;

        string customer_name = customer_name_input->text().toStdString();
        if (customer_name.empty())
            customer_name = "Customer";

        current_bill = new Bill(current_bill_ID, customer_name);

        update_summary();
        product_name_input->clear();
        batch_no_input->clear(); // ADD THIS LINE
        unit_price_input->setValue(0);
        quantity_input->setValue(1);

        invoice_number_label->setText(QString("Invoice #: %1").arg(QString::number(current_bill_ID).rightJustified(4, '0')));
        date_label->setText("Date: " + QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm"));
    }

    void update_product_frequencies(const vector<Product> &bill_products)
    {
        // Update the weight (frequency) of products in the database
        // Time Complexity: O(m * log n) where m = bill products, n = database size
        // Uses binary search for each product lookup

        if (product_database.isEmpty())
        {
            return; // No database to update
        }

        // Ensure database is sorted for binary search
        sort_database();

        bool database_modified = false;

        for (const auto &product : bill_products)
        {
            QString product_name = QString::fromStdString(product.get_name());

            // Use binary search to find the product in database
            int index = binary_search_product(product_name);

            if (index != -1)
            {
                // Product found in database - increment its weight (frequency)
                product_database[index].add_weight();
                database_modified = true;
            }
            // Note: If product is not found, it means it's a new product
            // It will be handled by the prompt_save_new_products() feature
        }

        // Save the updated database to CSV file if any changes were made
        if (database_modified && !database_file_path.isEmpty())
        {
            if (save_database_to_file(database_file_path))
            {
                qDebug() << "Product frequencies updated in database";
            }
            else
            {
                qWarning() << "Failed to save updated frequencies to database file";
            }
        }
        else if (database_modified && database_file_path.isEmpty())
        {
            // Database was modified but no file path exists
            // Ask user where to save
            QString save_path = QFileDialog::getSaveFileName(
                this,
                "Save Database with Updated Frequencies / Lưu cơ sở dữ liệu với tần suất cập nhật",
                "product_database.csv",
                "CSV Files (*.csv);;All Files (*)");

            if (!save_path.isEmpty())
            {
                database_file_path = save_path;
                QSettings settings("setup.ini", QSettings::IniFormat);
                settings.setValue("database_path", save_path);
                settings.sync();

                if (save_database_to_file(save_path))
                {
                    qDebug() << "Product frequencies saved to new database file";
                }
            }
        }
    }

    void end_invoice_and_print()
    {
        if (!current_bill || items_table->rowCount() == 0)
        {
            QMessageBox::warning(this, "Error / Lỗi",
                                 "No invoice to print\nKhông có hóa đơn để in");
            return;
        }

        string customer_name = customer_name_input->text().toStdString();
        if (!customer_name.empty())
        {
            rebuild_bill();
        }
        else
        {
            customer_name = "Customer";
        }

        // Check for unsaved products BEFORE saving PDF
        QVector<Product> unsaved_products = find_unsaved_products(current_bill->get_products());
        if (!unsaved_products.isEmpty())
        {
            prompt_save_new_products(unsaved_products);
        }

        QString safecustomer_name = QString::fromStdString(customer_name);
        safecustomer_name.replace(QRegularExpression("[/\\\\:*?\"<>|]"), "_");

        QString defaultFileName = QString("%1_%2.pdf")
                                      .arg(QString::number(current_bill_ID).rightJustified(4, '0'))
                                      .arg(safecustomer_name);

        QString fileName = QFileDialog::getSaveFileName(this,
                                                        "Save Invoice PDF / Lưu hóa đơn PDF",
                                                        defaultFileName,
                                                        "PDF Files (*.pdf);;All Files (*)");

        if (fileName.isEmpty())
        {
            return;
        }

        try
        {
            current_bill->export_pdf();

            QString defaultOutput = QString::fromStdString(current_bill->get_filename());

            if (QFile::exists(fileName))
            {
                QFile::remove(fileName);
            }

            if (QFile::rename(defaultOutput, fileName))
            {
                // ===== NEW: Update product frequencies AFTER successful PDF export =====
                update_product_frequencies(current_bill->get_products());
                // ===== END NEW CODE =====

                QMessageBox::information(this, "Success / Thành công",
                                         QString("Invoice exported successfully!\nHóa đơn đã được xuất thành công!\n\nFile: %1")
                                             .arg(fileName));

                statusBar()->showMessage("PDF exported / Đã xuất PDF: " + fileName, 5000);

                current_bill_ID++;
                saveInvoiceId(current_bill_ID);

                customer_name_input->clear();
                new_invoice();
            }
            else
            {
                QMessageBox::warning(this, "Warning / Cảnh báo",
                                     QString("PDF created but couldn't move to selected location.\n"
                                             "File saved as: %1")
                                         .arg(defaultOutput));
            }
        }
        catch (const exception &e)
        {
            QMessageBox::critical(this, "Export Error / Lỗi xuất file",
                                  QString("Failed to export PDF / Không thể xuất PDF:\n%1").arg(e.what()));
        }
    }

    // ==================== AUTOCOMPLETE METHODS ====================

private slots:
    void on_table_item_changed(QTableWidgetItem *item)
    {
        if (!item)
            return;

        int row = item->row();
        int col = item->column();

        // Block signals to prevent recursion
        items_table->blockSignals(true);

        if (col == 2) // Quantity changed
        {
            bool ok;
            int new_qty = item->text().toInt(&ok);
            if (ok && new_qty > 0)
            {
                // Recalculate total
                QString price_str = items_table->item(row, 3)->text();
                price_str.remove(" ₫").remove(",");
                double price = price_str.toDouble();
                double total = price * new_qty;
                items_table->item(row, 4)->setText(format_currency(total));

                rebuild_bill();
                update_summary();
            }
        }
        else if (col == 3) // Price changed
        {
            QString price_text = item->text();
            price_text.remove(" ₫").remove(",");
            bool ok;
            double new_price = price_text.toDouble(&ok);

            if (ok && new_price >= 0) // Allow 0
            {
                // Update display
                item->setText(format_currency(new_price));

                // Recalculate total
                int qty = items_table->item(row, 2)->text().toInt();
                double total = new_price * qty;
                items_table->item(row, 4)->setText(format_currency(total));

                rebuild_bill();
                update_summary();
            }
        }
        else if (col == 0 || col == 1)
        {
            rebuild_bill();
        }

        items_table->blockSignals(false);
    }

    void on_product_name_changed(const QString &text)
    {
        if (text.isEmpty() || product_database.isEmpty())
        {
            hide_suggestions();
            return;
        }

        // Use the dedicated search method (already handles duplication avoidance)
        QVector<Product> matches = search_products_by_prefix(text);

        // If no "starts with" matches, do fallback "contains" search
        if (matches.isEmpty())
        {
            QString search_text = text.toLower().trimmed();

            // Build set of products in bill for duplication check
            QSet<QString> products_in_bill;
            if (current_bill)
            {
                const auto &bill_products = current_bill->get_products();
                for (const auto &product : bill_products)
                {
                    products_in_bill.insert(QString::fromStdString(product.get_name()).toLower());
                }
            }

            // Collect ALL matches
            for (const auto &product : product_database)
            {
                QString product_name = QString::fromStdString(product.get_name()).toLower();

                if (product_name.contains(search_text))
                {
                    matches.append(product);
                }
            }

            // Sort with duplication penalty
            if (!matches.isEmpty())
            {
                std::sort(matches.begin(), matches.end(),
                          [&products_in_bill](const Product &a, const Product &b)
                          {
                              QString name_a = QString::fromStdString(a.get_name()).toLower();
                              QString name_b = QString::fromStdString(b.get_name()).toLower();

                              bool a_in_bill = products_in_bill.contains(name_a);
                              bool b_in_bill = products_in_bill.contains(name_b);

                              // Primary: Products NOT in bill come first
                              if (a_in_bill != b_in_bill)
                              {
                                  return !a_in_bill;
                              }

                              // Secondary: By weight (frequency) - descending
                              if (a.get_weight() != b.get_weight())
                              {
                                  return a.get_weight() > b.get_weight();
                              }

                              // Tertiary: Alphabetically
                              return name_a < name_b;
                          });

                // Limit to 10 AFTER sorting
                if (matches.size() > 10)
                {
                    matches.resize(10);
                }
            }
        }

        if (matches.isEmpty())
        {
            hide_suggestions();
        }
        else
        {
            show_suggestions(matches);
        }
    }

    void show_suggestions(const QVector<Product> &matches)
    {
        suggestion_list->clear();

        // Limit to 10 suggestions
        int count = qMin(matches.size(), 10);

        for (int i = 0; i < count; ++i)
        {
            const Product &product = matches[i];
            QString display_text = QString::fromStdString(product.get_name());

            // Don't show weight/frequency - just product name

            QListWidgetItem *item = new QListWidgetItem(display_text);
            item->setData(Qt::UserRole, QString::fromStdString(product.get_name()));
            item->setData(Qt::UserRole + 1, static_cast<double>(product.get_unit_price()));

            suggestion_list->addItem(item);
        }

        // Position the suggestion list below the input field
        QPoint global_pos = product_name_input->mapToGlobal(QPoint(0, product_name_input->height()));
        suggestion_list->move(global_pos);
        suggestion_list->setFixedWidth(product_name_input->width());

        suggestion_list->show();
        suggestion_list->setCurrentRow(0);
        suggestion_list_visible = true;
    }

    void hide_suggestions()
    {
        if (suggestion_list && suggestion_list_visible)
        {
            suggestion_list->hide();
            suggestion_list_visible = false;
        }
    }

    void on_suggestion_clicked(QListWidgetItem *item)
    {
        if (!item)
            return;

        QString product_name = item->data(Qt::UserRole).toString();
        double unit_price = item->data(Qt::UserRole + 1).toDouble();

        // Fill in the product details
        product_name_input->setText(product_name);
        unit_price_input->setValue(unit_price);

        hide_suggestions();

        // Focus on batch_no input
        batch_no_input->setFocus();
        batch_no_input->selectAll();
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QSettings settings("setup.ini", QSettings::IniFormat);
    bool first_run_completed = settings.value("first_run_completed", false).toBool();

    if (!first_run_completed)
    {
        SetupWindow setup_window;

        if (setup_window.exec() == QDialog::Accepted)
        {
            QString icon_path = setup_window.getIconPath();
            QString database_path = setup_window.getDatabasePath();

            if (!icon_path.isEmpty())
            {
                app.setWindowIcon(QIcon(icon_path));
            }

            if (!database_path.isEmpty())
            {
                qDebug() << "Database path saved:" << database_path;
            }
        }
        else
        {
            return 0;
        }
    }
    else
    {
        QSettings settings("setup.ini", QSettings::IniFormat);
        QString saved_icon_path = settings.value("icon_path", "").toString();
        if (!saved_icon_path.isEmpty() && QFile::exists(saved_icon_path))
        {
            app.setWindowIcon(QIcon(saved_icon_path));
        }
    }

    InvoicePrinterGUI window;
    window.show();

    return app.exec();
}

#include "InvoicePrinterGUI.moc"