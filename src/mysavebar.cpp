#include "mymainwindow.h"
#include "mysavebar.h"
#include "mysavethread.h"
#include "myfilesystemmodel.h"

using namespace MyAbstractBarPublic;
using namespace MySaveBarThreadPublic;
using namespace MySaveBarPublic;


/*******************************************************************************



*******************************************************************************/


MySaveBar::MySaveBar(QWidget *parent, MyFileSystemModel *arg_ptr_model, QString arg_list_path) :
	MyAbstractBar(parent), ptr_model(arg_ptr_model), list_path(arg_list_path)
{
	setWindowTitle("Saving...");

	// the message displayed when the user wants to cancel the operation
	ptr_stop_msg->setWindowTitle("Stop execution?");
	ptr_stop_msg->setText("Are you sure you want to stop saving the list?");

	// create the appropriate thread
	worker_thread = new MySaveThread(ptr_model, list_path);
	ptr_model->moveToThread(worker_thread);
	connect(worker_thread, &MySaveThread::updateProgress, this, &updateProgress);

	ui->label->setText("Saving list...");

	worker_thread->start();
}


/*******************************************************************************



*******************************************************************************/


MySaveBar::~MySaveBar()
{
	deleteProgressThread();
}


/*******************************************************************************



*******************************************************************************/


void MySaveBar::handleError(int error)
{
	if(error != INTERRUPTED)
	{
		// there was a problem and the thread has returned, give the user a warning message. it will
		// automatically delete itself when the user closes it

		QMessageBox *error_msg = new QMessageBox(static_cast<MyMainWindow *>(this->parent()));
		error_msg->setAttribute(Qt::WA_DeleteOnClose);
		error_msg->setWindowTitle("Error writing list!");
		error_msg->setIcon(QMessageBox::Warning);
		error_msg->setStandardButtons(QMessageBox::Ok);

		switch(error)
		{
			case FILE_WRITE_ERROR:
				error_msg->setText("There was an error creating the data file.");
				error_msg->show();
				break;

			case STREAM_WRITE_ERROR:
				error_msg->setText("There was an error writing the list of files and directories.");
				error_msg->show();
				break;
		}
	}
}


/*******************************************************************************



*******************************************************************************/


void MySaveBar::handleFinished() {}


/*******************************************************************************



*******************************************************************************/


void MySaveBar::deleteProgressThread()
{
	if(worker_thread->isRunning())
		worker_thread->requestInterruption();

	worker_thread->wait();
	delete worker_thread;
}


/*******************************************************************************



*******************************************************************************/

void MySaveBar::handleRejectYes()
{
	worker_thread->requestInterruption();
	worker_thread->wait();
}
