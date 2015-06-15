#ifndef MYSAVETHREAD_H
#define MYSAVETHREAD_H

#include <QThread>

class MyFileSystemModel;
class QFile;

namespace MySaveBarThreadPublic
{


	/*******************************************************************************



	*******************************************************************************/


	class MySaveThread : public QThread
	{
		Q_OBJECT

	public:
		MySaveThread(MyFileSystemModel *arg_model, const QString &arg_path);
		int getStatus() const;

	protected:
		void run() Q_DECL_OVERRIDE;

	signals:
		void updateProgress(int curr);

	private:
		void interruptionPoint(QFile *list_file);
		void runHelper();

		MyFileSystemModel *ptr_model;
		const QString list_path;
		int status;
	};
}

#endif // MYSAVETHREAD_H
