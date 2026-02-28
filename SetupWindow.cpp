#include "SetupWindow.h"
#include <QIcon>
#include <QCheckBox>

SetupWindow::SetupWindow(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Initial Setup / Thiết lập ban đầu");
    setMinimumSize(600, 500);
    setModal(true);

    QVBoxLayout *main_layout = new QVBoxLayout(this);
    main_layout->setSpacing(20);
    main_layout->setContentsMargins(20, 20, 20, 20);

    // Title
    QLabel *title_label = new QLabel("Welcome to Invoice Printer\nChào mừng đến với Phần mềm In Hóa Đơn");
    QFont title_font = title_label->font();
    title_font.setPointSize(14);
    title_font.setBold(true);
    title_label->setFont(title_font);
    title_label->setAlignment(Qt::AlignCenter);
    title_label->setStyleSheet("color: #2196F3; padding: 10px;");
    main_layout->addWidget(title_label);

    // Icon Selection Section
    QGroupBox *icon_group = new QGroupBox("Application Icon / Biểu tượng ứng dụng");
    icon_group->setStyleSheet("QGroupBox { font-weight: bold; border: 2px solid #cccccc; border-radius: 5px; margin-top: 10px; padding-top: 10px; }");
    QVBoxLayout *icon_layout = new QVBoxLayout();

    QLabel *icon_info = new QLabel("Select an icon for the application (optional)\nChọn biểu tượng cho ứng dụng (tùy chọn)");
    icon_info->setStyleSheet("color: #666666; font-size: 9pt; font-weight: normal;");
    icon_layout->addWidget(icon_info);

    QHBoxLayout *icon_input_layout = new QHBoxLayout();
    icon_path_input = new QLineEdit();
    icon_path_input->setPlaceholderText("Browse for icon file / Chọn icon...");
    icon_path_input->setReadOnly(true);
    icon_path_input->setMinimumHeight(35);

    QPushButton *browse_icon_button = new QPushButton("Browse\nDuyệt");
    browse_icon_button->setMinimumHeight(50);
    browse_icon_button->setMinimumWidth(100);
    browse_icon_button->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; border-radius: 5px; } QPushButton:hover { background-color: #1976D2; }");

    icon_input_layout->addWidget(icon_path_input);
    icon_input_layout->addWidget(browse_icon_button);
    icon_layout->addLayout(icon_input_layout);

    // Icon preview
    icon_preview_label = new QLabel("No icon selected\nChưa chọn biểu tượng");
    icon_preview_label->setAlignment(Qt::AlignCenter);
    icon_preview_label->setMinimumHeight(100);
    icon_preview_label->setStyleSheet("border: 2px dashed #cccccc; border-radius: 5px; background-color: #f5f5f5;");
    icon_layout->addWidget(icon_preview_label);

    icon_group->setLayout(icon_layout);
    main_layout->addWidget(icon_group);

    // Database Import Section
    QGroupBox *database_group = new QGroupBox("Import Database / Nhập cơ sở dữ liệu");
    database_group->setStyleSheet("QGroupBox { font-weight: bold; border: 2px solid #cccccc; border-radius: 5px; margin-top: 10px; padding-top: 10px; }");
    QVBoxLayout *database_layout = new QVBoxLayout();

    QLabel *database_info = new QLabel("Import product database from CSV file (optional)\nNhập cơ sở dữ liệu sản phẩm từ file CSV (tùy chọn)");
    database_info->setStyleSheet("color: #666666; font-size: 9pt; font-weight: normal;");
    database_layout->addWidget(database_info);

    QHBoxLayout *database_input_layout = new QHBoxLayout();
    database_path_input = new QLineEdit();
    database_path_input->setPlaceholderText("Browse for CSV file / Chọn file CSV...");
    database_path_input->setReadOnly(true);
    database_path_input->setMinimumHeight(35);

    QPushButton *browse_database_button = new QPushButton("Browse\nDuyệt");
    browse_database_button->setMinimumHeight(50);
    browse_database_button->setMinimumWidth(100);
    browse_database_button->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; border-radius: 5px; } QPushButton:hover { background-color: #1976D2; }");

    database_input_layout->addWidget(database_path_input);
    database_input_layout->addWidget(browse_database_button);
    database_layout->addLayout(database_input_layout);

    database_group->setLayout(database_layout);
    main_layout->addWidget(database_group);

    main_layout->addStretch();

    // Continue Button
    continue_button = new QPushButton("Continue to Application\nTiếp tục đến ứng dụng");
    continue_button->setMinimumHeight(60);
    continue_button->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; font-size: 12pt; border-radius: 5px; } QPushButton:hover { background-color: #45a049; }");
    main_layout->addWidget(continue_button);

    // Connect signals
    connect(browse_icon_button, &QPushButton::clicked, this, &SetupWindow::browse_icon);
    connect(browse_database_button, &QPushButton::clicked, this, &SetupWindow::browse_database);
    connect(continue_button, &QPushButton::clicked, this, &SetupWindow::save_and_continue);
}

QString SetupWindow::getIconPath() const
{
    return selected_icon_path;
}

QString SetupWindow::getDatabasePath() const
{
    return selected_database_path;
}

void SetupWindow::browse_icon()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Select Icon / Chọn biểu tượng",
        "",
        "Image Files (*.png *.ico *.jpg *.jpeg *.bmp);;All Files (*)");

    if (!fileName.isEmpty())
    {
        // Check file extension
        QFileInfo fileInfo(fileName);
        QString extension = fileInfo.suffix().toLower();

        if (extension == "png" || extension == "jpg" || extension == "jpeg" || extension == "bmp")
        {
            // Convert to ICO
            QString icoPath = convert_to_ico(fileName);
            if (!icoPath.isEmpty())
            {
                selected_icon_path = icoPath;
                icon_path_input->setText(icoPath);

                // Show preview
                QPixmap pixmap(icoPath);
                if (!pixmap.isNull())
                {
                    QPixmap scaled = pixmap.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    icon_preview_label->setPixmap(scaled);
                    icon_preview_label->setText("");
                }

                QMessageBox::information(this, "Conversion Success / Chuyển đổi thành công",
                                         QString("Image converted to ICO format!\n"
                                                 "Đã chuyển đổi hình ảnh sang định dạng ICO!\n\n"
                                                 "Original: %1\n"
                                                 "ICO file: %2")
                                             .arg(fileName)
                                             .arg(icoPath));
            }
            else
            {
                QMessageBox::warning(this, "Conversion Failed / Chuyển đổi thất bại",
                                     "Failed to convert image to ICO format.\n"
                                     "Không thể chuyển đổi hình ảnh sang định dạng ICO.");
            }
        }
        else if (extension == "ico")
        {
            // Already ICO, use directly
            selected_icon_path = fileName;
            icon_path_input->setText(fileName);

            // Show preview
            QPixmap pixmap(fileName);
            if (!pixmap.isNull())
            {
                QPixmap scaled = pixmap.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                icon_preview_label->setPixmap(scaled);
                icon_preview_label->setText("");
            }
            else
            {
                icon_preview_label->setText("Invalid icon file\nFile biểu tượng không hợp lệ");
            }
        }
        else
        {
            QMessageBox::warning(this, "Invalid Format / Định dạng không hợp lệ",
                                 "Please select a PNG, JPG, BMP, or ICO file.\n"
                                 "Vui lòng chọn file PNG, JPG, BMP, hoặc ICO.");
        }
    }
}

QString SetupWindow::convert_to_ico(const QString &image_path)
{
    try
    {
        // Load the image
        QImage image(image_path);
        if (image.isNull())
        {
            qWarning() << "Failed to load image:" << image_path;
            return QString();
        }

        // Create output path (same directory, same name, .ico extension)
        QFileInfo fileInfo(image_path);
        QString output_path = fileInfo.absolutePath() + "/" + fileInfo.baseName() + ".ico";

        // ICO files should contain multiple sizes for best results
        // We'll create common sizes: 16x16, 32x32, 48x48, 64x64, 128x128, 256x256
        QList<QSize> sizes = {
            QSize(16, 16),
            QSize(32, 32),
            QSize(48, 48),
            QSize(64, 64),
            QSize(128, 128),
            QSize(256, 256)};

        // Create a list of images at different sizes
        QList<QImage> images;
        for (const QSize &size : sizes)
        {
            QImage scaled = image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

            // Ensure the image has an alpha channel for transparency
            if (scaled.format() != QImage::Format_ARGB32)
            {
                scaled = scaled.convertToFormat(QImage::Format_ARGB32);
            }

            images.append(scaled);
        }

        // Save as ICO using QImageWriter with multiple sizes
        // Qt's ICO writer can handle multiple images
        QFile file(output_path);
        if (!file.open(QIODevice::WriteOnly))
        {
            qWarning() << "Failed to open output file:" << output_path;
            return QString();
        }

        // Write ICO file manually with multiple sizes
        if (!write_ico_file(output_path, images))
        {
            qWarning() << "Failed to write ICO file";
            return QString();
        }

        return output_path;
    }
    catch (...)
    {
        qWarning() << "Exception during ICO conversion";
        return QString();
    }
}

bool SetupWindow::write_ico_file(const QString &path, const QList<QImage> &images)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    // ICO Header
    stream << quint16(0);             // Reserved (must be 0)
    stream << quint16(1);             // Type (1 = ICO)
    stream << quint16(images.size()); // Number of images

    qint64 image_data_offset = 6 + (images.size() * 16); // Header + directory entries

    // Write directory entries
    QList<QByteArray> image_data_list;
    for (const QImage &img : images)
    {
        // Convert image to PNG format for storage in ICO
        QByteArray png_data;
        QBuffer buffer(&png_data);
        buffer.open(QIODevice::WriteOnly);
        img.save(&buffer, "PNG");

        image_data_list.append(png_data);

        // Directory entry
        stream << quint8(img.width() == 256 ? 0 : img.width());   // Width (0 = 256)
        stream << quint8(img.height() == 256 ? 0 : img.height()); // Height (0 = 256)
        stream << quint8(0);                                      // Color palette (0 = no palette)
        stream << quint8(0);                                      // Reserved
        stream << quint16(1);                                     // Color planes
        stream << quint16(32);                                    // Bits per pixel
        stream << quint32(png_data.size());                       // Size of image data
        stream << quint32(image_data_offset);                     // Offset to image data

        image_data_offset += png_data.size();
    }

    // Write image data
    for (const QByteArray &data : image_data_list)
    {
        file.write(data);
    }

    file.close();
    return true;
}

void SetupWindow::browse_database()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Select CSV Database / Chọn file CSV",
        "",
        "CSV Files (*.csv);;All Files (*)");

    if (!fileName.isEmpty())
    {
        // Validate CSV file
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QMessageBox::warning(this, "Error / Lỗi",
                                 "Cannot open CSV file\nKhông thể mở file CSV");
            return;
        }

        QTextStream in(&file);
        QString firstLine = in.readLine();
        file.close();

        if (firstLine.isEmpty())
        {
            QMessageBox::warning(this, "Error / Lỗi",
                                 "CSV file is empty\nFile CSV trống");
            return;
        }

        selected_database_path = fileName;
        database_path_input->setText(fileName);

        QMessageBox::information(this, "Success / Thành công",
                                 QString("CSV file loaded successfully\nĐã tải file CSV thành công\n\nFile: %1").arg(fileName));
    }
}

void SetupWindow::save_and_continue()
{
    // Save settings using the SAME format as main checks
    QSettings settings("setup.ini", QSettings::IniFormat);
    settings.setValue("first_run_completed", true);
    settings.setValue("icon_path", selected_icon_path);
    settings.setValue("database_path", selected_database_path);
    settings.sync(); // Force write to disk

    // Apply icon if selected
    if (!selected_icon_path.isEmpty())
    {
        QMessageBox::information(this, "Success / Thành công",
                                 "Icon has been saved to application folder!\n"
                                 "Đã lưu biểu tượng vào thư mục ứng dụng!\n\n"
                                 "Location / Vị trí: " +
                                     selected_icon_path);
    }

    accept();
}