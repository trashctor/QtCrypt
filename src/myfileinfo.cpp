#include <cmath>

#include <QFileInfo>
#include <QDateTime>
#include <QThread>
#include <QDir>
#include <QException>
#include <QCoreApplication>
#include <QFileIconProvider>

#include "mydirthread.h"
#include "myfileinfo.h"
#include "myfilesystemmodel.h"

#define CALCULATING_SIZE -1
#define EMPTY_SIZE -2

using namespace MyFileInfoPublic;
using namespace MyFileInfoPublicThread;


/*******************************************************************************



*******************************************************************************/


MyFileInfo::MyFileInfo(MyFileSystemModel *arg_model) : QObject(arg_model), type(MFIT_EMPTY),
	size(EMPTY_SIZE), checked(Qt::Unchecked),	dir_thread(nullptr) {}


/*******************************************************************************



*******************************************************************************/


MyFileInfo::~MyFileInfo()
{
	deleteDirThread();
}


/*******************************************************************************



*******************************************************************************/


MyFileInfo::MyFileInfo(MyFileSystemModel *arg_model, const QString &arg_path) :
	MyFileInfo(arg_model)
{
	setItemHelper(arg_path);
}


/*******************************************************************************



*******************************************************************************/


void MyFileInfo::deleteDirThread()
{
	if(dir_thread != nullptr)
	{
		if(dir_thread->isRunning())
			dir_thread->requestInterruption();

		dir_thread->wait();
		delete dir_thread;
	}
}


/*******************************************************************************



*******************************************************************************/


void MyFileInfo::startDirThread()
{
	if(dir_thread != nullptr)
		dir_thread->start();
}


/*******************************************************************************



*******************************************************************************/


void MyFileInfo::stopDirThread()
{
	if(dir_thread != nullptr)
	{
		dir_thread->requestInterruption();
		dir_thread->wait();
	}
}


/*******************************************************************************



*******************************************************************************/


void MyFileInfo::setEmpty()
{
	// make sure any previous threads are stopped and their potential signals are handled
	deleteDirThread();
	dir_thread = nullptr;
	QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

	type = MFIT_EMPTY;
	name = "";
	path = "";
	date = "";
	full_path = "";
	size = EMPTY_SIZE;
	checked = Qt::Unchecked;
	icon = QIcon();
}


/*******************************************************************************



*******************************************************************************/


void MyFileInfo::setItem(const QString &arg_path)
{
	// without processing any current events first, a signal from the previous thread might override
	// work done by setItemHelper()
	deleteDirThread();
	dir_thread = nullptr;
	QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

	setItemHelper(arg_path);
}


/*******************************************************************************



*******************************************************************************/


void MyFileInfo::setItemHelper(const QString &arg_path)
{
	// try and sanitize the input path, before feeding it to QFileInfo to get an absolute file path
	QFileInfo q_file = QFileInfo(QDir::cleanPath(arg_path));
	full_path = q_file.absoluteFilePath();

	icon = QFileIconProvider().icon(q_file);

	if(q_file.isDir())
	{
		type = MFIT_DIR;

		// size will be tagged as still calculating until the worker thread finishes
		size = CALCULATING_SIZE;

		dir_thread = new MyDirThread(q_file.absoluteFilePath());
		connect(dir_thread, &MyDirThread::updateSize, this, &MyFileInfo::updateSize);

		//dir_thread->start();
	}

	else if(q_file.isFile())
	{
		type = MFIT_FILE;
		size = q_file.size();
	}

	else
	{
		// arg_path points to nothing, leave it as an empty object
		return;
	}

	//name = q_file.fileName();
	path = q_file.absolutePath();
	date = q_file.lastModified().toString();

	// this method of obtaining the name is much more reliable than using QFileInfo::fileName() which
	// bugs out whenever the path isn't formatted nicely. for example, the path "." or "../".
	name = full_path;

	if(!QDir(path).isRoot())
		name.remove(path + '/');
	else
		name.remove(path);
}


/*******************************************************************************



*******************************************************************************/


QString MyFileInfo::getType() const
{
	return typeToString(type);
}


/*******************************************************************************



*******************************************************************************/


QString MyFileInfo::getSuffix() const
{
	if(type == MFIT_FILE)
	{
		QFileInfo qfi(full_path);
		return qfi.suffix();
	}
	else
		return QString();
}


/*******************************************************************************



*******************************************************************************/


QIcon MyFileInfo::getIcon() const
{
	return icon;
}



/*******************************************************************************



*******************************************************************************/


QString MyFileInfo::typeToString(MyFileInfoType arg_type)
{
	switch(arg_type)
	{
		case MFIT_DIR:
			return "Directory";
		case MFIT_FILE:
			return "File";
		default:
			//return "Item not found!";
			return "Empty";
	}
}


/*******************************************************************************



*******************************************************************************/


void MyFileInfo::updateSize(qint64 arg_size)
{
	size = arg_size;

	// tell the model to signal the view that its size data changed

	MyFileSystemModel *ptr_model = static_cast<MyFileSystemModel *>(parent());

	if(ptr_model != nullptr)
	{
		QModelIndex left = ptr_model->index(0, MyFileSystemModelPublic::SIZE_HEADER);
		QModelIndex right = ptr_model->index(ptr_model->rowCount(),
			MyFileSystemModelPublic::SIZE_HEADER);
		emit ptr_model->dataChanged(left, right);
	}
}


/*******************************************************************************



*******************************************************************************/


QString MyFileInfo::getSize() const
{
	static const double KiB = pow(2, 10);
	static const double MiB = pow(2, 20);
	static const double GiB = pow(2, 30);
	static const double TiB = pow(2, 40);
	static const double PiB = pow(2, 50);

	// convert to appropriate units based on the size of the item
	if(size >= 0)
	{
		static const int precision = 0;

		if(size < KiB)
			return QString::number(size, 'f', precision) + " B";
		else if(size < MiB)
			return QString::number(size / KiB, 'f', precision) + " KiB";
		else if(size < GiB)
			return QString::number(size / MiB, 'f', precision) + " MiB";
		else if(size < TiB)
			return QString::number(size / GiB, 'f', precision) + " GiB";
		else if(size < PiB)
			return QString::number(size / TiB, 'f', precision) + " TiB";
		else
			return QString::number(size / PiB, 'f', precision) + " PiB";
	}
	else if(size == CALCULATING_SIZE)
		return "Calculating";
	else
		return "";
}


/*******************************************************************************



*******************************************************************************/


QString MyFileInfo::getPath() const {return path;}
QString MyFileInfo::getName() const {return name;}
QString MyFileInfo::getDate() const {return date;}
QString MyFileInfo::getFullPath() const {return full_path;}
Qt::CheckState MyFileInfo::getChecked() const {return checked;}
void MyFileInfo::setChecked(Qt::CheckState arg_checked) {checked = arg_checked;}


/*******************************************************************************



*******************************************************************************/


bool MyFileInfo::equals(const MyFileInfo &rhs) const
{
	return full_path == rhs.full_path ? true : false;
}


/*******************************************************************************



*******************************************************************************/


bool MyFileInfo::operator==(const MyFileInfo &rhs) const
{
	return this->equals(rhs);
}


/*******************************************************************************



*******************************************************************************/

bool MyFileInfo::contains(const MyFileInfo &rhs) const
{
	if(type == MFIT_DIR)
	{
		// only a root directory will have a '/' already appended. otherwise, append it to the path
		// for checking if "this" contains "rhs"
		QDir this_dir(full_path);
		QString contain_path = full_path;

		// note that the function isRoot() doesn't depend on this_dir actually existing or not
		if(!this_dir.isRoot())
			contain_path += '/';

		if(rhs.full_path.startsWith(contain_path))
			return true;
	}

	return false;
}


/*******************************************************************************



*******************************************************************************/


bool MyFileInfo::isRedundant(const MyFileInfo &rhs) const
{
	if(this->equals(rhs) || rhs.contains(*this))
		return true;

	return false;
}


/*******************************************************************************



*******************************************************************************/


bool MyFileInfo::makesRedundant(const MyFileInfo &rhs) const
{
	if(this->equals(rhs) || this->contains(rhs))
		return true;

	return false;
}
