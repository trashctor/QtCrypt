#ifndef MYENCRYPTBAR_H
#define MYENCRYPTBAR_H

#include "myabstractbar.h"
#include "mycrypt.h"

// forward declarations
class MyFileSystemModel;
namespace MyEncryptBarThreadPublic
{
	class MyEncryptThread;
}


/*******************************************************************************



*******************************************************************************/


namespace MyEncryptBarPublic
{
	// used for getting the status of each item after the encryption thread is finished
	enum : int {ZIP_ERROR = MyCryptPublic::CRYPT_SUCCESS + 1, NOT_STARTED};
}


/*******************************************************************************



*******************************************************************************/


class MyEncryptBar : public MyAbstractBar
{
	Q_OBJECT

public:
	MyEncryptBar(QWidget *parent, MyFileSystemModel *arg_ptr_model, const QString &arg_password, bool
		delete_success, const QString *encrypt_name = nullptr);
	~MyEncryptBar();

protected:
	void handleFinished() Q_DECL_OVERRIDE;
	void handleError(int error) Q_DECL_OVERRIDE;
	void handleRejectYes() Q_DECL_OVERRIDE;

	void deleteProgressThread();

private:
	QString createDetailedText();
	void cleanModel();
	QString errorCodeToString(int error_code);
	MyEncryptBarThreadPublic::MyEncryptThread *worker_thread;
	MyFileSystemModel *ptr_model;
};

#endif // MYENCRYPTBAR_H
