#include <QEventLoop>

#include "myabstractbar.h"
#include "ui_myabstractbar.h"
#include "myfilesystemmodel.h"

using namespace MyAbstractBarPublic;


/*******************************************************************************



*******************************************************************************/


MyAbstractBar::MyAbstractBar(QWidget *parent) : QDialog(parent), ui(new Ui::MyAbstractBar),
	ptr_stop_msg(new QMessageBox(this))
{
	// setup the basic progress bar and message box layout

	ui->setupUi(this);

	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	ui->progress_bar->setRange(MIN, MAX);
	ui->progress_bar->setValue(MIN);

	ptr_stop_msg->setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	ptr_stop_msg->setIcon(QMessageBox::Question);
}

MyAbstractBar::~MyAbstractBar()
{
	delete ui;
	delete ptr_stop_msg;
}

void MyAbstractBar::updateProgress(int curr)
{
	// check if there was an error
	if(curr < RESET)
	{
		// there was a problem, handle it and close the progress bar
		handleError(curr);

		// destroy the progress bar afterwards
		if(!ptr_stop_msg->isHidden())
			ptr_stop_msg->done(QMessageBox::Cancel);

		// quit in failure
		this->done(EXEC_FAILURE);
	}

	// check if the thread is finished
	else if(curr == FINISHED)
	{
		handleFinished();

		// close the on_cancel_clicked() message box if open
		if(!ptr_stop_msg->isHidden())
			ptr_stop_msg->done(QMessageBox::Cancel);

		// quit successfully
		this->done(EXEC_SUCCESS);
	}

	// otherwise, the signal was an update to the progress bar
	else
		ui->progress_bar->setValue(curr);
}


void MyAbstractBar::on_cancel_clicked()
{
	// reject() is called for close events and the esc key as well
	reject();
}


void MyAbstractBar::reject()
{
	// only respond if the progress bar isn't hidden
	if(!this->isHidden())
	{
		if(ptr_stop_msg->exec() == QMessageBox::Yes)
			// user chose to stop
			handleRejectYes();
	}
}
