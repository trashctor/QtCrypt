#include "mymainwindow.h"
#include "mydecryptbar.h"
#include "mydecryptthread.h"
#include "myfilesystemmodel.h"

using namespace MyAbstractBarPublic;
using namespace MyDecryptBarPublic;
using namespace MyDecryptBarThreadPublic;
using namespace MyCryptPublic;


/*******************************************************************************



*******************************************************************************/


MyDecryptBar::MyDecryptBar(QWidget *parent, MyFileSystemModel *arg_ptr_model, const QString
	&arg_password, bool delete_success) : MyAbstractBar(parent), ptr_model(arg_ptr_model)
{
	setWindowTitle("Decrypting...");

	// the message displayed when the user wants to cancel the operation
	ptr_stop_msg->setWindowTitle("Stop execution?");
	ptr_stop_msg->setText("Are you sure you want to stop decrypting?");

	// create the appropriate thread
	worker_thread = new MyDecryptThread(ptr_model, arg_password, delete_success);
	ptr_model->moveToThread(worker_thread);
	connect(worker_thread, &MyDecryptThread::updateProgress, this, &updateProgress);

	ui->label->setText("Decrypting list...");

	worker_thread->start();
}


/*******************************************************************************



*******************************************************************************/


MyDecryptBar::~MyDecryptBar()
{
	deleteProgressThread();
}


/*******************************************************************************



*******************************************************************************/


void MyDecryptBar::handleError(int error)
{
	// the user interrupted the decryption progress, give him a list of what was done

	QMessageBox *error_msg = new QMessageBox(static_cast<MyMainWindow *>(this->parent()));
	error_msg->setAttribute(Qt::WA_DeleteOnClose);
	error_msg->setWindowTitle("Decryption interrupted!");
	error_msg->setIcon(QMessageBox::Warning);
	error_msg->setStandardButtons(QMessageBox::Close);
	error_msg->setText("The decryption process was interrupted. Some files or directories might not "
		"have been finished. Click below to show details.");

	// wait for the worker thread to completely finish
	worker_thread->wait();

	QString detailed_text = createDetailedText();

	if(detailed_text == QString())
		detailed_text = "No existing items were found!";

	error_msg->setDetailedText(detailed_text);
	error_msg->show();

	ptr_model->startAllDirThread();
}


/*******************************************************************************



*******************************************************************************/


QString MyDecryptBar::createDetailedText()
{
	QString ret_string;

	// get the decryption status of each item from the worker thread
	std::vector<int> status_list = worker_thread->getStatus();
	const std::vector<MyFileInfoPtr> &item_list = worker_thread->getItemList();

	// go through each one and list the result right after the full path of the item
	for(int i = 0; i < item_list.size(); i++)
	{
		ret_string += item_list[i]->getFullPath();
		ret_string += "\n" + errorCodeToString(status_list[i]);

		if(i < status_list.size() - 1)
			ret_string += "\n\n";
	}

	return ret_string;
}


/*******************************************************************************



*******************************************************************************/


QString MyDecryptBar::errorCodeToString(int error_code)
{
	QString ret_string;

	switch(error_code)
	{
		case SRC_NOT_FOUND:
			ret_string += "The encrypted file was not found!";
			break;

		case SRC_CANNOT_OPEN_READ:
			ret_string += "The encrypted file could not be opened for reading!";
			break;

		case SRC_HEADER_READ_ERROR:
			ret_string += "The encrypted file header could not be read!";
			break;

		case PASS_HASH_FAIL:
			ret_string += "The password could not be hashed!";
			break;

		case SRC_HEADER_DECRYPT_ERROR:
			ret_string += "The encrypted file header could not be decrypted! Password is most likely "
				"incorrect.";
			break;

		case DES_FILE_EXISTS:
			ret_string += "The decrypted file already exists!";
			break;

		case DES_CANNOT_OPEN_WRITE:
			ret_string += "The decrypted file could not be opened for writing!";
			break;

		case SRC_BODY_READ_ERROR:
			ret_string += "There was an error reading the encrypted file's data!";
			break;

		case SRC_FILE_CORRUPT:
			ret_string += "The encrypted file's size is invalid!";
			break;

		case DATA_DECRYPT_ERROR:
			ret_string += "The encrypted file's data could not be decrypted! Password is most likely "
				"incorrect";
			break;

		case DES_BODY_WRITE_ERROR:
			ret_string += "The decrypted file could not be written to!";
			break;

		case CRYPT_SUCCESS:
			ret_string += "The file or directory was successfully decrypted!";
			break;

		case ZIP_ERROR:
			ret_string += "The file or directory could not be unzipped!";
			break;

		case NOT_STARTED:
			ret_string += "The file or directory was skipped!";
			break;

		case NOT_QTCRYPT:
			ret_string += "The item wasn't a qtcrypt file!";
			break;
	}

	return ret_string;
}


/*******************************************************************************



*******************************************************************************/


void MyDecryptBar::handleFinished()
{
	// the decryption process was finished, create a list of what was successful and wasn't

	QMessageBox *error_msg = new QMessageBox(static_cast<MyMainWindow *>(this->parent()));
	error_msg->setAttribute(Qt::WA_DeleteOnClose);
	error_msg->setWindowTitle("Decryption finished!");
	error_msg->setIcon(QMessageBox::Warning);
	error_msg->setStandardButtons(QMessageBox::Close);
	error_msg->setText("The decryption process was finished. Click below to show details.");

	// wait for the worker thread to completely finished
	worker_thread->wait();

	QString detailed_text = createDetailedText();

	if(detailed_text == QString())
		detailed_text = "No existing items were found!";

	error_msg->setDetailedText(detailed_text);
	error_msg->show();

	ptr_model->startAllDirThread();
}


/*******************************************************************************



*******************************************************************************/


void MyDecryptBar::deleteProgressThread()
{
	if(worker_thread->isRunning())
		worker_thread->requestInterruption();

	worker_thread->wait();
	delete worker_thread;
}


/*******************************************************************************



*******************************************************************************/

void MyDecryptBar::handleRejectYes()
{
	worker_thread->requestInterruption();
	worker_thread->wait();
}
