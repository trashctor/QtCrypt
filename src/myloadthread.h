#ifndef MYLOADTHREAD_H
#define MYLOADTHREAD_H

#include <QThread>

class MyFileSystemModel;

namespace MyLoadBarThreadPublic
{


	/*******************************************************************************



	*******************************************************************************/


	class MyLoadThread : public QThread
	{
		Q_OBJECT

	public:
		MyLoadThread(MyFileSystemModel *arg_model, const QString &arg_path);
		int getStatus() const;

	protected:
		void run() Q_DECL_OVERRIDE;

	signals:
		void updateProgress(int curr);

	private:
		void interruptionPoint();
		void runHelper();

		MyFileSystemModel *ptr_model;
		const QString list_path;
		int status;
	};
}

#endif // MYLOADTHREAD_H
