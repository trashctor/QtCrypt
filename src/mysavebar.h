#ifndef MYSAVEBAR_H
#define MYSAVEBAR_H

#include "myabstractbar.h"

// forward declarations
class MyFileSystemModel;
namespace MySaveBarThreadPublic
{
	class MySaveThread;
}

namespace MySaveBarPublic
{
	// more specific updateProgress() stuff
	enum SAVE_UPDATE: int {FILE_WRITE_ERROR = std::numeric_limits<int>::min(), STREAM_WRITE_ERROR};
}


/*******************************************************************************



*******************************************************************************/


class MySaveBar : public MyAbstractBar
{
	Q_OBJECT

public:
	MySaveBar(QWidget *parent, MyFileSystemModel *arg_ptr_model, QString arg_list_path);
	~MySaveBar();

protected:
	void handleFinished() Q_DECL_OVERRIDE;
	void handleError(int error) Q_DECL_OVERRIDE;
	void handleRejectYes() Q_DECL_OVERRIDE;

	void deleteProgressThread();

private:
	MySaveBarThreadPublic::MySaveThread *worker_thread;
	MyFileSystemModel *ptr_model;
	QString list_path;
};

#endif // MYSAVEBAR_H
