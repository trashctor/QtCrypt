#include "mymainwindow.h"
#include "myencryptbar.h"
#include "myencryptthread.h"
#include "myfilesystemmodel.h"

using namespace MyAbstractBarPublic;
using namespace MyEncryptBarPublic;
using namespace MyEncryptBarThreadPublic;
using namespace MyCryptPublic;


/*******************************************************************************



*******************************************************************************/


MyEncryptBar::MyEncryptBar(QWidget *parent, MyFileSystemModel *arg_ptr_model, const QString
	&arg_password, bool delete_success, const QString *encrypt_name) : MyAbstractBar(parent),
	ptr_model(arg_ptr_model)
{
	setWindowTitle("Encrypting...");

	// the message displayed when the user wants to cancel the operation
	ptr_stop_msg->setWindowTitle("Stop execution?");
	ptr_stop_msg->setText("Are you sure you want to stop encrypting?");

	// create the appropriate thread
	worker_thread = new MyEncryptThread(ptr_model, arg_password, delete_success, encrypt_name);
	ptr_model->moveToThread(worker_thread);
	connect(worker_thread, &MyEncryptThread::updateProgress, this, &updateProgress);

	ui->label->setText("Encrypting list...");

	worker_thread->start();
}


/*******************************************************************************



*******************************************************************************/


MyEncryptBar::~MyEncryptBar()
{
	deleteProgressThread();
}


/*******************************************************************************



*******************************************************************************/


void MyEncryptBar::handleError(int error)
{
	// the user interrupted the encryption progress, give him a list of what was done

	QMessageBox *error_msg = new QMessageBox(static_cast<MyMainWindow *>(this->parent()));
	error_msg->setAttribute(Qt::WA_DeleteOnClose);
	error_msg->setWindowTitle("Encryption interrupted!");
	error_msg->setIcon(QMessageBox::Warning);
	error_msg->setStandardButtons(QMessageBox::Close);
	error_msg->setText("The encryption process was interrupted. Some files or directories might not "
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


QString MyEncryptBar::createDetailedText()
{
	QString ret_string;

	// get the encryption status of each item from the worker thread
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


QString MyEncryptBar::errorCodeToString(int error_code)
{
	QString ret_string;

	switch(error_code)
	{
		case SRC_NOT_FOUND:
			ret_string += "The intermediate file was not found!";
			break;

		case SRC_CANNOT_OPEN_READ:
			ret_string += "The intermediate file could not be opened for reading!";
			break;

		case PASS_HASH_FAIL:
			ret_string += "The password could not be hashed!";
			break;

		case DES_HEADER_ENCRYPT_ERROR:
			ret_string += "The intermediate file header could not be encrypted!";
			break;

		case DES_FILE_EXISTS:
			ret_string += "The encrypted file already exists!";
			break;

		case DES_CANNOT_OPEN_WRITE:
			ret_string += "The encrypted file could not be opened for writing!";
			break;

		case DES_HEADER_WRITE_ERROR:
			ret_string += "The encrypted file could not be written to!";
			break;

		case SRC_BODY_READ_ERROR:
			ret_string += "The intermediate file could not be read!";
			break;

		case DATA_ENCRYPT_ERROR:
			ret_string += "The intermediate file's data could not be encrypted!";
			break;

		case DES_BODY_WRITE_ERROR:
			ret_string += "The encrypted file could not be written to!";
			break;

		case CRYPT_SUCCESS:
			ret_string += "The file or directory was successfully encrypted!";
			break;

		case ZIP_ERROR:
			ret_string += "The file or directory could not be zipped!";
			break;

		case NOT_STARTED:
			ret_string += "The file or directory was skipped!";
			break;
	}

	return ret_string;
}


/*******************************************************************************



*******************************************************************************/


void MyEncryptBar::handleFinished()
{
	// the encryption process was finished, create a list of what was successful and wasn't

	QMessageBox *error_msg = new QMessageBox(static_cast<MyMainWindow *>(this->parent()));
	error_msg->setAttribute(Qt::WA_DeleteOnClose);
	error_msg->setWindowTitle("Encryption finished!");
	error_msg->setIcon(QMessageBox::Warning);
	error_msg->setStandardButtons(QMessageBox::Close);
	error_msg->setText("The encryption process was finished. Click below to show details.");

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


void MyEncryptBar::deleteProgressThread()
{
	if(worker_thread->isRunning())
		worker_thread->requestInterruption();

	worker_thread->wait();
	delete worker_thread;
}


/*******************************************************************************



*******************************************************************************/

void MyEncryptBar::handleRejectYes()
{
	worker_thread->requestInterruption();
	worker_thread->wait();
}
