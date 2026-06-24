#ifndef CONN_SETTINGS_H
#define CONN_SETTINGS_H

#include <QDialog>

namespace Ui {
class conn_settings;
}

class conn_settings : public QDialog
{
    Q_OBJECT

public:
    explicit conn_settings(QWidget *parent = nullptr);
    ~conn_settings();
signals:
    void connect_to(QStringList,QStringList,QStringList);
public slots:

private slots:
    void on_btn_save_clicked();

private:
    Ui::conn_settings *ui;
};

#endif // CONN_SETTINGS_H
