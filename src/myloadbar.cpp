#include "mymainwindow.h"
#include "myloadbar.h"
#include "myloadthread.h"
#include "myfilesystemmodel.h"

using namespace MyAbstractBarPublic;
using namespace MyLoadBarThreadPublic;
using namespace MyLoadBarPublic;


/*******************************************************************************



*******************************************************************************/


MyLoadBar::MyLoadBar(QWidget *parent, MyFileSystemModel *arg_ptr_model, QString arg_list_path) :
	MyAbstractBar(parent), ptr_model(arg_ptr_model), list_path(arg_list_path)
{
	setWindowTitle("Loading...");

	// the message displayed when the user wants to cancel the operation
	ptr_stop_msg->setWindowTitle("Stop execution?");
	ptr_stop_msg->setText("Are you sure you want to stop loading the list?");

	// create the appropriate thread
	worker_thread = new MyLoadThread(ptr_model, list_path);
	ptr_model->moveToThread(worker_thread);
	connect(worker_thread, &MyLoadThread::updateProgress, this, &updateProgress);

	ui->label->setText("Loading list...");

	worker_thread->start();
}


/*******************************************************************************



*******************************************************************************/


MyLoadBar::~MyLoadBar()
{
	deleteProgressThread();
}


/*******************************************************************************



*******************************************************************************/


void MyLoadBar::handleError(int error)
{
	if(error != INTERRUPTED)
	{
		// there was a problem and the thread has returned, give the user a warning message. it will
		// automatically delete itself when the user closes it

		QMessageBox *error_msg = new QMessageBox(static_cast<MyMainWindow *>(this->parent()));
		error_msg->setAttribute(Qt::WA_DeleteOnClose);
		error_msg->setWindowTitle("Error opening list!");
		error_msg->setIcon(QMessageBox::Warning);
		error_msg->setStandardButtons(QMessageBox::Ok);

		switch(error)
		{
			case FILE_READ_ERROR:
				error_msg->setText("There was an error opening the data file.");
				error_msg->show();
				break;

			case STREAM_READ_ERROR:
				error_msg->setText("There was an error reading the list of files and directories.");
				error_msg->show();
				break;
		}
	}
}


/*******************************************************************************



*******************************************************************************/


void MyLoadBar::handleFinished()
{
	// start the directory size calculation threads for the model
	ptr_model->startAllDirThread();
}


/*******************************************************************************



*******************************************************************************/


void MyLoadBar::deleteProgressThread()
{
	if(worker_thread->isRunning())
		worker_thread->requestInterruption();

	worker_thread->wait();
	delete worker_thread;
}


/*******************************************************************************



*******************************************************************************/

void MyLoadBar::handleRejectYes()
{
	worker_thread->requestInterruption();
	worker_thread->wait();
}
