#ifndef MYENCRYPTTHREAD_H
#define MYENCRYPTTHREAD_H

#include "myfileinfo.h"

#include <QThread>

class MyFileSystemModel;

namespace MyEncryptBarThreadPublic
{


	/*******************************************************************************



	*******************************************************************************/


	class MyEncryptThread : public QThread
	{
		Q_OBJECT

	public:
		MyEncryptThread(MyFileSystemModel *arg_model, const QString &arg_password, bool
			arg_delete_success, const QString *arg_encrypt_name);
		std::vector<int> getStatus() const;
		const std::vector<MyFileInfoPtr> &getItemList() const;

	protected:
		void run() Q_DECL_OVERRIDE;

	signals:
		void updateProgress(int curr);

	private:
		void interruptionPoint();
		void runHelper();
		int getNextFilenameSuffix(const QString &path, const QString &base_name, const QString
			&file_type, int start_num = 0,	int	max_tries = 1000) const;

		MyFileSystemModel *ptr_model;
		std::vector<int> status_list;
		std::vector<MyFileInfoPtr> item_list;
		const std::vector<MyFileInfoPtr> empty_list;
		QString password;
		bool delete_success;
		const QString *encrypt_name;
	};
}

#endif // MYENCRYPTTHREAD_H
