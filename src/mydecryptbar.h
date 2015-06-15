#ifndef MYDECRYPTBAR_H
#define MYDECRYPTBAR_H

#include "myabstractbar.h"
#include "mycrypt.h"

// forward declarations
class MyFileSystemModel;
namespace MyDecryptBarThreadPublic
{
	class MyDecryptThread;
}


/*******************************************************************************



*******************************************************************************/


namespace MyDecryptBarPublic
{
	// used for getting the status of each item after the decryption thread is finished
	enum : int {ZIP_ERROR = MyCryptPublic::CRYPT_SUCCESS + 1, NOT_STARTED, NOT_QTCRYPT};
}


/*******************************************************************************



*******************************************************************************/


class MyDecryptBar : public MyAbstractBar
{
	Q_OBJECT

public:
	MyDecryptBar(QWidget *parent, MyFileSystemModel *arg_ptr_model, const QString &arg_password, bool
		delete_success);
	~MyDecryptBar();

protected:
	void handleFinished() Q_DECL_OVERRIDE;
	void handleError(int error) Q_DECL_OVERRIDE;
	void handleRejectYes() Q_DECL_OVERRIDE;

	void deleteProgressThread();

private:
	QString createDetailedText();
	void cleanModel();
	QString errorCodeToString(int error_code);
	MyDecryptBarThreadPublic::MyDecryptThread *worker_thread;
	MyFileSystemModel *ptr_model;
};

#endif // MYDECRYPTBAR_H
