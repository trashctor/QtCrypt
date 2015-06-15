#ifndef MYABSTRACTBAR_H
#define MYABSTRACTBAR_H

#include <QMessageBox>
#include <QDialog>

#include "ui_myabstractbar.h"

namespace Ui {
class MyAbstractBar;
}


/*******************************************************************************



*******************************************************************************/


namespace MyAbstractBarPublic
{
	// used for signaling between the class MyAbstractBar and the underlying worker
	enum PROGRESS_UPDATE: int {INTERRUPTED = -3, RESET = -2, FINISHED = -1, MIN = 0,
		MAX = std::numeric_limits<int>::max()};
	enum EXEC_RESULT: int {EXEC_SUCCESS, EXEC_FAILURE};
}


/*******************************************************************************



*******************************************************************************/


class MyAbstractBar : public QDialog
{
	Q_OBJECT

public:
	MyAbstractBar(QWidget *parent);
	~MyAbstractBar();

protected:
	virtual void handleFinished() = 0;
	virtual void handleError(int error) = 0;
	virtual void handleRejectYes() = 0;

	Ui::MyAbstractBar *ui;
	QMessageBox *ptr_stop_msg;

protected slots:
	void updateProgress(int curr);

private slots:
	void on_cancel_clicked();
	void reject() Q_DECL_OVERRIDE;
};

#endif // MYABSTRACTBAR_H
