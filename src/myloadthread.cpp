#include <QTextStream>
#include <QFile>

#include "myloadthread.h"
#include "myloadbar.h"
#include "myinterrupt.h"
#include "myfilesystemmodel.h"

using namespace MyAbstractBarPublic;
using namespace MyLoadBarThreadPublic;
using namespace MyLoadBarPublic;

/*******************************************************************************



*******************************************************************************/


MyLoadThread::MyLoadThread(MyFileSystemModel *arg_model, const QString &arg_path) :
	ptr_model(arg_model), list_path(arg_path), status(RESET) {}


/*******************************************************************************



*******************************************************************************/


int MyLoadThread::getStatus() const
{
	if(this->isRunning())
		return MIN;
	else
		return status;
}


/*******************************************************************************



*******************************************************************************/


void MyLoadThread::interruptionPoint()
{
	if(QThread::currentThread()->isInterruptionRequested())
	{
		ptr_model->reset();
		ptr_model->moveToThread(this->thread());
		status = INTERRUPTED;
		emit updateProgress(INTERRUPTED);
		throw MyInterrupt();
	}
}


/*******************************************************************************



*******************************************************************************/


void MyLoadThread::run()
{
	try
	{
		runHelper();
	}
	catch(MyInterrupt &e)
	{
		return;
	}
}


/*******************************************************************************



*******************************************************************************/


void MyLoadThread::runHelper()
{
	QFile list_file(list_path);

	// was there an error finding the file?
	if(!list_file.open(QIODevice::ReadOnly))
	{
		ptr_model->reset();
		ptr_model->moveToThread(this->thread());
		status = FILE_READ_ERROR;
		emit updateProgress(FILE_READ_ERROR);
		throw MyInterrupt();
	}

	QTextStream list_stream(&list_file);
	list_stream.setCodec("UTF-8");

	// get the total number of items for use in the progress bar
	int total_num = 0;

	while(!list_stream.atEnd())
	{
		total_num++;
		list_stream.readLine();
	}

	// clear the current list of files and directories
	ptr_model->reset();
	emit updateProgress(MIN);

	list_stream.seek(0);

	// start reading from the stream and inserting them into the model
	for(int i = 0; i < total_num; i++)
	{
		interruptionPoint();

		QString full_path = QDir::cleanPath(list_stream.readLine());

		// insert directly into the model
		ptr_model->insertItem(i, full_path);
		emit updateProgress((MAX / total_num) * (i + 1));
	}

	// if any files or directories are missing or redundant, remove them from the list
	ptr_model->removeEmpty();
	interruptionPoint();
	ptr_model->removeRootDir();
	interruptionPoint();
	ptr_model->removeRedundant();
	interruptionPoint();
	ptr_model->sort();

	// update the progress bar to 100%
	emit updateProgress(MAX);

	// push the model back to the caller's thread
	ptr_model->moveToThread(this->thread());

	// tells the progress bar to quit successfully
	status = FINISHED;
	emit updateProgress(FINISHED);
}
