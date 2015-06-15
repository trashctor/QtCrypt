#ifndef MYDECRYPTTHREAD_H
#define MYDECRYPTTHREAD_H

#include "myfileinfo.h"

#include <QThread>

class MyFileSystemModel;

namespace MyDecryptBarThreadPublic
{


	/*******************************************************************************



	*******************************************************************************/


	class MyDecryptThread : public QThread
	{
		Q_OBJECT

	public:
		MyDecryptThread(MyFileSystemModel *arg_model, const QString &arg_password, bool
			arg_delete_success);
		std::vector<int> getStatus() const;
		const std::vector<MyFileInfoPtr> &getItemList() const;

	protected:
		void run() Q_DECL_OVERRIDE;

	signals:
		void updateProgress(int curr);

	private:
		void interruptionPoint();
		void runHelper();

		MyFileSystemModel *ptr_model;
		std::vector<int> status_list;
		std::vector<MyFileInfoPtr> item_list;
		const std::vector<MyFileInfoPtr> empty_list;
		QString password;
		bool delete_success;
	};
}

#endif // MYDECRYPTTHREAD_H
