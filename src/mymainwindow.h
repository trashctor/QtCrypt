#ifndef MYMAINWINDOW_H
#define MYMAINWINDOW_H

#include <QMainWindow>

#include "myfilesystemmodel.h"


/*******************************************************************************



*******************************************************************************/


namespace Ui {
class MyMainWindow;
}

class MyMainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MyMainWindow(QWidget *parent = 0);
	~MyMainWindow();

	void session();

private slots:

	void on_encrypt_clicked();
	void on_decrypt_clicked();
	void on_check_all_clicked();
	void on_add_file_clicked();
	void on_add_directory_clicked();
	void on_remove_checked_clicked();
	void on_uncheck_all_clicked();
	void on_show_password_stateChanged(int state);
	void on_save_list_clicked();
	void on_load_list_clicked();
	void on_refresh_view_clicked();
	void on_delete_checked_clicked();

	void setModel(MyFileSystemModel *ptr_model);

protected:

	void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

private:

	void start_crypto(int crypto_type);

	Ui::MyMainWindow *ui;
	MyFileSystemModel *file_model;
	QString password;
};

#endif // MYMAINWINDOW_H
