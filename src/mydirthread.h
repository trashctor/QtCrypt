#ifndef MYDIRTHREAD_H
#define MYDIRTHREAD_H

#include <climits>
#include <QThread>

class MyFileSystemModel;
class QFile;

/*******************************************************************************



*******************************************************************************/


namespace MyFileInfoPublicThread
{


	/*******************************************************************************



	*******************************************************************************/


	class MyDirThread : public QThread
	{
		Q_OBJECT

	public:
		//MyDirThread() {}
		MyDirThread(const QString &arg_path);

	protected:
		void run() Q_DECL_OVERRIDE;
		//void setDirPath(const QString &arg_path) {dir_path = arg_path;}
		//QString getDirPath() const {return dir_path;}

	signals:
		void updateSize(qint64 info);

	private:
		const QString dir_path;
		qint64 getDirSize(const QString &curr_path) const;
	};
}

#endif // MYDIRTHREAD_H
