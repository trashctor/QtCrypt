#include <iostream>

#include <QMessageBox>
#include <QCloseEvent>

#include "crypto_secretbox.h"
#include "quazip.h"

#include "mymainwindow.h"
#include "ui_mymainwindow.h"
#include "myfilesystemmodel.h"
#include "myabstractbar.h"
#include "mysavebar.h"
#include "myloadbar.h"
#include "mysavethread.h"
#include "myloadthread.h"
#include "myencryptbar.h"
#include "mydecryptbar.h"

#define DEFAULT_LIST_PATH QString("session.qtlist")
#define MIN_PASS_LENGTH 6
#define START_ENCRYPT 0
#define START_DECRYPT 1
#define MAX_ENCRYPT_NAME_LENGTH 256
#define EXTRA_COLUMN_SPACE 10
#define EMPTY_COLUMN_SPACE 100

typedef std::unique_ptr<MyAbstractBar> MyAbstractBarPtr;


/*******************************************************************************



*******************************************************************************/


MyMainWindow::MyMainWindow(QWidget *parent) :
	QMainWindow(parent), ui(new Ui::MyMainWindow), file_model(nullptr)//new MyFileSystemModel(this))
{
	ui->setupUi(this);

	setWindowTitle("QtCrypt");

	/*
	ui->tree_view->setModel(file_model);
	ui->tree_view->hideColumn(MyFileSystemModelPublic::FULL_PATH_HEADER);
	ui->tree_view->hideColumn(MyFileSystemModelPublic::TYPE_HEADER);

	for(int i = 0; i < MyFileSystemModelPublic::COLUMN_NUM; i++)
		ui->tree_view->resizeColumnToContents(i);
	*/

	setModel(new MyFileSystemModel(nullptr));

	ui->password_0->setEchoMode(QLineEdit::Password);
	ui->password_1->setEchoMode(QLineEdit::Password);

	// note that some password information will be discarded if the text characters entered use more
	// than	1 byte per character in UTF-8
	ui->password_0->setMaxLength(crypto_secretbox_KEYBYTES);
	ui->password_1->setMaxLength(crypto_secretbox_KEYBYTES);

	// initialize the zipping text codec, specifically, for use in encryption and decryption
	QuaZip::setDefaultFileNameCodec("UTF-8");

	ui->encrypt_filename->setMaxLength(MAX_ENCRYPT_NAME_LENGTH);
	ui->auto_resize->setChecked(true);
}


/*******************************************************************************



*******************************************************************************/


MyMainWindow::~MyMainWindow()
{
	delete ui;
}


/*******************************************************************************



*******************************************************************************/

void MyMainWindow::session()
{
	// show the main window and load from the default item list
	show();

	MyFileSystemModelPtr clone_model = file_model->cloneByValue();
	MyAbstractBarPtr ptr_mpb(new MyLoadBar(this, clone_model.get(), DEFAULT_LIST_PATH));

	if(ptr_mpb != nullptr)
	{
		int ret_val = ptr_mpb->exec();

		// if the progress bar successfully loaded the file into the clone model
		if(ret_val == MyAbstractBarPublic::EXEC_SUCCESS)
		{
			// use it to replace the old model
			setModel(clone_model.release());
		}
		else
			setModel(file_model);
	}
}


/*******************************************************************************



*******************************************************************************/


void MyMainWindow::setModel(MyFileSystemModel *ptr_model)
{
	// when ptr_model is different from the current one, have it replace the old model and resize the
	// columns. otherwise, don't replace but still resize the columns if auto resize was checked
	if(file_model != ptr_model)
	{
		int width_array[MyFileSystemModelPublic::COLUMN_NUM];

		if(!ui->auto_resize->isChecked())
		{
			// save the current widths before switching over to the new model, which will reset them
			for(int i = 0; i < MyFileSystemModelPublic::COLUMN_NUM; i++)
				width_array[i] = ui->tree_view->columnWidth(i);
		}

		// use it to replace the old model
		ui->tree_view->setModel(nullptr);
		delete file_model;
		file_model = ptr_model;
		file_model->setParent(this);

		// setup the view for the new model
		ui->tree_view->setModel(file_model);
		ui->tree_view->hideColumn(MyFileSystemModelPublic::FULL_PATH_HEADER);
		ui->tree_view->hideColumn(MyFileSystemModelPublic::TYPE_HEADER);

		if(!ui->auto_resize->isChecked())
		{
			// restore the previous column settings
			for(int i = 0; i < MyFileSystemModelPublic::COLUMN_NUM; i++)
				ui->tree_view->setColumnWidth(i, width_array[i]);
		}
	}

	// configure the new width to the contents
	if(ui->auto_resize->isChecked())
		for(int i = 0; i < MyFileSystemModelPublic::COLUMN_NUM; i++)
		{
			ui->tree_view->resizeColumnToContents(i);
			ui->tree_view->setColumnWidth(i, ui->tree_view->columnWidth(i) + EXTRA_COLUMN_SPACE);
		}
}


/*******************************************************************************



*******************************************************************************/


void MyMainWindow::on_encrypt_clicked()
{
	start_crypto(START_ENCRYPT);
}


/*******************************************************************************



*******************************************************************************/


void MyMainWindow::on_decrypt_clicked()
{
	start_crypto(START_DECRYPT);
}


/*******************************************************************************



*******************************************************************************/


void MyMainWindow::start_crypto(int crypto_type)
{
	// check if the password fields match
	if(ui->password_0->text() == ui->password_1->text())
	{
		QString pass = ui->password_0->text();
		QString encrypt_name;

		bool checked = false;

		// make sure that there is actually something selected in the model for cryptography
		for(int n = file_model->rowCount(), i = 0; i < n; i++)
		{
			if(file_model->data(file_model->index(i, MyFileSystemModelPublic::CHECKED_HEADER),
				Qt::CheckStateRole) == Qt::Checked)
			{
				checked = true;
				break;
			}
		}

		// check that the password is long enough
		if(pass.toUtf8().size() < MIN_PASS_LENGTH)
		{
			QString num_text = QString::number(MIN_PASS_LENGTH);
			QString warn_text = QString("The password entered was too short. ") + "Please enter a longer "
			"one (at least " + num_text + " ASCII characters/bytes) before continuing.";

			QMessageBox::warning(this, "Password too short!", warn_text);
		}

		// give the user a warning message if nothing was selected
		else if(!checked)
		{
			QString crypto_text = (crypto_type == START_ENCRYPT) ? "encryption!" : "decryption!";
			QString warn_text = QString("There were no items selected for " + crypto_text);

			QMessageBox::warning(this, "Nothing selected!", warn_text);
		}

		else
		{
			// start the encryption process
			bool delete_success = ui->delete_success->isChecked();

			MyFileSystemModelPtr clone_model(file_model->cloneByValue());

			/*
			MyAbstractBarPtr ptr_mpb = (crypto_type == START_ENCRYPT) ?
				MyAbstractBarPtr(new MyEncryptBar(this, clone_model.get(), pass, delete_success)) :
				MyAbstractBarPtr(new MyDecryptBar(this, clone_model.get(), pass, delete_success));
			*/

			MyAbstractBarPtr ptr_mpb;

			if(crypto_type == START_ENCRYPT)
			{
				// check if the user wants to rename the encrypted files to something else
				if(ui->encrypt_rename->isChecked())
				{
					encrypt_name = ui->encrypt_filename->text();
					ptr_mpb = MyAbstractBarPtr(new MyEncryptBar(this, clone_model.get(), pass, delete_success,
						&encrypt_name));
				}
				else
				{
					ptr_mpb = MyAbstractBarPtr(new MyEncryptBar(this, clone_model.get(), pass,
						delete_success));
				}
			}
			else
				ptr_mpb = MyAbstractBarPtr(new MyDecryptBar(this, clone_model.get(), pass, delete_success));

			if(ptr_mpb != nullptr)
				ptr_mpb->exec();

			// after crypto, replace the old model with the new one
			setModel(clone_model.release());

			// clear the password fields when done
			ui->password_0->clear();
			ui->password_1->clear();
		}
	}
	else
		QMessageBox::warning(this, "Passwords do not match!", "The password fields do not match! "
			"Please make sure they were entered correctly and try again.");
}


/*******************************************************************************



*******************************************************************************/


void MyMainWindow::on_add_file_clicked()
{
	QList<QString> file_list = QFileDialog::getOpenFileNames(this, "Open File(s)", QString(),
		QString(), nullptr, QFileDialog::DontResolveSymlinks);
	QList<QString> red_list = QList<QString>();

	bool added = false;

	for(QList<QString>::iterator it = file_list.begin(); it != file_list.end(); ++it)
	{
		int row_num = file_model->rowCount();

		// check to see if the file is already in the model
		if(file_model->isRedundant(*it))
		{
			// create a list of all these files that were redundant
			red_list.append(*it);
		}
		else
		{
			file_model->insertItem(row_num, *it);
			added = true;
		}
	}

	// resize the columns if a file was added
	if(added)
		setModel(file_model);

	if(red_list.size() > 0)
	{
		// create message
		QString warn_title = "File(s) already added!";
		QString warn_text = "The following file(s) were already added. They will not be added again.";
		QString warn_detail;

		for(int i = 0; i < red_list.size(); i++)
		{
			warn_detail += red_list[i];

			if(i != red_list.size() - 1)
				warn_detail += '\n';
		}

		// display in message box
		QMessageBox msg_box(QMessageBox::Warning, warn_title, warn_text, QMessageBox::Close, this);
		msg_box.setDetailedText(warn_detail);
		msg_box.exec();
	}
}


/*******************************************************************************



*******************************************************************************/


void MyMainWindow::on_add_directory_clicked()
{
	QString dir = QFileDialog::getExistingDirectory(this, "Open Directory", QString(),
		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if(dir != QString())
	{
		QString warn_title;
		QString warn_text;
		QString warn_detail;

		// don't let the user add in a root directory
		if(QDir(QDir(dir).absolutePath()).isRoot())
		{
			warn_title = "Can't add a root directory!";
			warn_text = "The root directory (" + dir + ") cannot be added.";

			QMessageBox msg_box(QMessageBox::Warning, warn_title, warn_text, QMessageBox::Close, this);
			msg_box.exec();
		}

		// is the directory contained by anything already inside the model or a duplicate?
		else if(file_model->isRedundant(dir))
		{
			// create message
			warn_title = "Directory already added!";
			warn_text = "The following directory was already added. It will not be added again.";
			warn_detail = dir;

			// display in message box
			QMessageBox msg_box(QMessageBox::Warning, warn_title, warn_text, QMessageBox::Close, this);
			msg_box.setDetailedText(warn_detail);
			msg_box.exec();
		}

		else
		{
			std::vector<QString> red_list = file_model->makesRedundant(dir);

			// check if the directory chosen makes something redundant inside the model
			if(red_list.size() > 0)
			{
				// ask if they want to add the directory. if so, the redundant items will be removed

				warn_title = "Directory makes item(s) redundant!";
				warn_text = "The directory chosen (" + dir + ") contains item(s) inside it that "
				"were added previously. If you add this new directory, the following redundant items will "
				"be removed.";

				for(int i = 0; i < red_list.size(); i++)
				{
					warn_detail += red_list[i];

					if(i != red_list.size() - 1)
						warn_detail += '\n';
				}

				// display the message with Ok and Cancel buttons
				QMessageBox msg_box(QMessageBox::Warning, warn_title, warn_text, QMessageBox::Ok |
					QMessageBox::Cancel, this);
				msg_box.setDetailedText(warn_detail);

				if(msg_box.exec() == QMessageBox::Ok)
				{
					file_model->removeItems(red_list);

					file_model->insertItem(0, dir);
					file_model->startDirThread(0);

					// resize the columns
					setModel(file_model);
				}
			}

			// the chosen directory doesn't make anything inside the model redundant, simply add it
			else
			{
				file_model->insertItem(0, dir);
				file_model->startDirThread(0);
				setModel(file_model);
			}
		}

	}	// if(dir != QString())
}


/*******************************************************************************



*******************************************************************************/


void MyMainWindow::on_remove_checked_clicked()
{
	// go through the entire model and remove any checked items
	file_model->removeCheckedItems();
	setModel(file_model);

	/*
	int total = file_model->rowCount();

	for(int curr = 0, i = 0; i < total; i++)
	{
		QModelIndex qmi = file_model->index(curr, MyFileSystemModelPublic::CHECKED_HEADER);

		if(file_model->data(qmi, Qt::CheckStateRole) == Qt::Checked)
			file_model->removeRow(curr);
		else
			curr++;
	}
	*/
}


/*******************************************************************************



*******************************************************************************/


void MyMainWindow::on_check_all_clicked()
{
	int n = file_model->rowCount();

	for(int i = 0; i < n; i++)
		file_model->setData(file_model->index(i), Qt::Checked, Qt::CheckStateRole);
}


/*******************************************************************************



*******************************************************************************/


void MyMainWindow::on_uncheck_all_clicked()
{
	int n = file_model->rowCount();

	for(int i = 0; i < n; i++)
		file_model->setData(file_model->index(i), Qt::Unchecked, Qt::CheckStateRole);
}


/*******************************************************************************



*******************************************************************************/


void MyMainWindow::on_show_password_stateChanged(int state)
{
	if(state == Qt::Checked)
	{
		ui->password_0->setEchoMode(QLineEdit::Normal);
		ui->password_1->setEchoMode(QLineEdit::Normal);
	}
	else if(state == Qt::Unchecked)
	{
		ui->password_0->setEchoMode(QLineEdit::Password);
		ui->password_1->setEchoMode(QLineEdit::Password);
	}
}


/*******************************************************************************



*******************************************************************************/


void MyMainWindow::on_save_list_clicked()
{
	QString list_path = QFileDialog::getSaveFileName(this, "Save List", QDir::currentPath(),
		"QtCrypt list files (*.qtlist)", nullptr, QFileDialog::DontResolveSymlinks);

	// check if the user actually selected a file
	if(list_path != QString())
	{
		MyFileSystemModelPtr clone_model = file_model->cloneByValue();
		MyAbstractBarPtr ptr_mpb(new MySaveBar(this, clone_model.get(), list_path));

		if(ptr_mpb != nullptr)
			ptr_mpb->exec();
	}
}


/*******************************************************************************



*******************************************************************************/


void MyMainWindow::on_load_list_clicked()
{
	QString list_path = QFileDialog::getOpenFileName(this, "Open List", QDir::currentPath(),
		"QtCrypt list files (*.qtlist)", nullptr, QFileDialog::DontResolveSymlinks);

	// check if the user actually selected a file
	if(list_path != QString())
	{
		MyFileSystemModelPtr clone_model = file_model->cloneByValue();
		MyAbstractBarPtr ptr_mpb(new MyLoadBar(this, clone_model.get(), list_path));

		if(ptr_mpb != nullptr)
		{
			int ret_val = ptr_mpb->exec();

			// if the progress bar successfully loaded the file into the clone model
			if(ret_val == MyAbstractBarPublic::EXEC_SUCCESS)
			{
				// use it to replace the old model
				setModel(clone_model.release());
			}
		}
	}	// list_path != QString()
}


/*******************************************************************************



*******************************************************************************/


void MyMainWindow::closeEvent(QCloseEvent *event)
{
	// save the current list before quitting
	MyFileSystemModelPtr clone_model = file_model->cloneByValue();
	MyAbstractBarPtr ptr_mpb(new MySaveBar(this, clone_model.get(), DEFAULT_LIST_PATH));

	int ret_val;

	if(ptr_mpb != nullptr)
		ret_val = ptr_mpb->exec();

	// if there was a problem saving the list, ask the user before exiting
	if(ret_val == MyAbstractBarPublic::EXEC_FAILURE)
	{
		ret_val = QMessageBox::warning(this, "Error saving session!", "QtCrypt wasn't able to save "
			"your current file and directory list. Do you still want to quit?", QMessageBox::Yes |
			QMessageBox::Cancel, QMessageBox::Cancel);

		if(ret_val == QMessageBox::Yes)
			event->accept();
		else
			event->ignore();
	}

	// successfully saved the list, quit
	else
		event->accept();
}


/*******************************************************************************



*******************************************************************************/


void MyMainWindow::on_refresh_view_clicked()
{
	// creating a new model will recreate and update all the internal MyFileInfo objects
	MyFileSystemModelPtr new_model = file_model->cloneByValue();

	new_model->removeEmpty();
	new_model->removeRootDir();
	new_model->removeRedundant();
	new_model->sort();

	setModel(new_model.release());
	file_model->startAllDirThread();
}


/*******************************************************************************



*******************************************************************************/


void MyMainWindow::on_delete_checked_clicked()
{
	std::vector<MyFileInfoPtr> item_list;

	// get a list of all the items checked for deletion
	for(int n = file_model->rowCount(), i = 0; i < n; i++)
	{
		if(file_model->data(file_model->index(i, MyFileSystemModelPublic::CHECKED_HEADER),
			Qt::CheckStateRole) == Qt::Checked)
		{
			QString full_path = file_model->data(file_model->index(i,
				MyFileSystemModelPublic::FULL_PATH_HEADER)).toString();
			item_list.push_back(MyFileInfoPtr(new MyFileInfo(nullptr, full_path)));
		}
	}

	int total_items = item_list.size();

	if(total_items != 0)
	{
		QMessageBox::StandardButton ret_val = QMessageBox::question(this, "Delete items?", "Are you "
			"sure you wish to permanently delete the selected items?", QMessageBox::Ok |
			QMessageBox::Cancel, QMessageBox::Cancel);

		if(ret_val == QMessageBox::Ok)
		{
			std::vector<QString> result_list;

			for(int i = 0; i < total_items; i++)
			{
				QString full_path = item_list[i]->getFullPath();
				QString item_type = item_list[i]->getType();

				if(item_type == MyFileInfo::typeToString(MyFileInfoPublic::MFIT_FILE))
				{
					if(QFile::remove(full_path))
					{
						file_model->removeItem(full_path);
						result_list.push_back("Successfully deleted file!");
					}
					else
						result_list.push_back("Error deleting file!");
				}
				else if(item_type == MyFileInfo::typeToString(MyFileInfoPublic::MFIT_DIR))
				{
					if(QDir(full_path).removeRecursively())
					{
						file_model->removeItem(full_path);
						result_list.push_back("Successfully deleted directory!");
					}
					else
						result_list.push_back("Error deleting directory!");
				}
				else
				{
					file_model->removeItem(full_path);
					result_list.push_back("Item was not found!");
				}
			}

			// items were most likely removed from the model, resize columns
			setModel(file_model);

			QString det_string;

			// create a string containing the list of items and results
			for(int i = 0; i < item_list.size(); i++)
			{
				det_string += item_list[i]->getFullPath();
				det_string += "\n" + result_list[i];

				if(i < item_list.size() - 1)
					det_string += "\n\n";
			}

			QMessageBox del_msg(QMessageBox::Information, "Deletion completed!", "The deletion process "
				"was completed. Click below to show details.", QMessageBox::Close, this);

			del_msg.setDetailedText(det_string);
			del_msg.exec();
		}

		// total_items != 0
	}
}
