#include <QTextStream>

#include "mydirthread.h"
#include "myfilesystemmodel.h"

// trivial exception class used for interrupting thread execution in class MyDirThread
#include "myinterrupt.h"

using namespace MyFileInfoPublicThread;
using namespace MyFileSystemModelPublic;


/*******************************************************************************



*******************************************************************************/


MyDirThread::MyDirThread(const QString &arg_path) : dir_path(arg_path) {}


/*******************************************************************************



*******************************************************************************/


void MyDirThread::run()
{
	qint64 size = 0;

	// catch any exceptions caused by a requestInterruption() from another thread and stop if so
	try
	{
		// recursive function to parse through directories
		size = getDirSize(dir_path);
	}
	catch(MyInterrupt &e)
	{
		return;
	}

	// finished, signal the appropriate value and done
	emit updateSize(size);
}


/*******************************************************************************



*******************************************************************************/


qint64 MyDirThread::getDirSize(const QString &curr_dir) const
{
	qint64 size = 0;

	// get a list of elements in the current directory

	QDir qdir = QDir(curr_dir);
	QList<QFileInfo> list = qdir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoSymLinks |
		QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);

	for(int i = 0; i < list.size(); i++)
	{
		QFileInfo file = list.at(i);

		// check if an interruption was requested
		if(QThread::currentThread()->isInterruptionRequested())
		{
			throw MyInterrupt();
		}

		// otherwise, continue parsing files and directories
		if(file.isFile())
			size += file.size();
		else if(file.isDir())
			size += getDirSize(file.absoluteFilePath());
	}

	return size;
}
