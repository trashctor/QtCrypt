#ifndef MYLOADBAR_H
#define MYLOADBAR_H

#include "myabstractbar.h"

// forward declarations
class MyFileSystemModel;
namespace MyLoadBarThreadPublic
{
	class MyLoadThread;
}

namespace MyLoadBarPublic
{
	// more specific updateProgress() stuff
	enum LOAD_UPDATE: int {FILE_READ_ERROR = std::numeric_limits<int>::min(), STREAM_READ_ERROR};
}


/*******************************************************************************



*******************************************************************************/


class MyLoadBar : public MyAbstractBar
{
	Q_OBJECT

public:
	MyLoadBar(QWidget *parent, MyFileSystemModel *arg_ptr_model, QString arg_list_path);
	~MyLoadBar();

protected:
	void handleFinished() Q_DECL_OVERRIDE;
	void handleError(int error) Q_DECL_OVERRIDE;
	void handleRejectYes() Q_DECL_OVERRIDE;

	void deleteProgressThread();

private:
	MyLoadBarThreadPublic::MyLoadThread *worker_thread;
	MyFileSystemModel *ptr_model;
	QString list_path;
};

#endif // MYLOADBAR_H
