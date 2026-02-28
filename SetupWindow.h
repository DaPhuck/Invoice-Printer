#ifndef SETUPWINDOW_H
#define SETUPWINDOW_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QPixmap>
#include <QImage>
#include <QImageWriter>
#include <QBuffer>
#include <QFileInfo>
#include <QDataStream>

class SetupWindow : public QDialog
{
    Q_OBJECT

private:
    QLineEdit *icon_path_input;
    QLineEdit *database_path_input;
    QLabel *icon_preview_label;
    QPushButton *continue_button;

    QString selected_icon_path;
    QString selected_database_path;

    // Helper methods for ICO conversion
    QString convert_to_ico(const QString &image_path);
    bool write_ico_file(const QString &path, const QList<QImage> &images);

public:
    SetupWindow(QWidget *parent = nullptr);
    QString getIconPath() const;
    QString getDatabasePath() const;

private slots:
    void browse_icon();
    void browse_database();
    void save_and_continue();
};

#endif // SETUPWINDOW_H