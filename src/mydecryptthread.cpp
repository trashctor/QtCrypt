#include "JlCompress.h"

#include "mydecryptthread.h"
#include "mydecryptbar.h"
#include "myinterrupt.h"
#include "myfilesystemmodel.h"
#include "mycleardir.h"

using namespace MyDecryptBarThreadPublic;
using namespace MyDecryptBarPublic;
using namespace MyAbstractBarPublic;
using namespace MyFileSystemModelPublic;
using namespace MyCryptPublic;


/*******************************************************************************



*******************************************************************************/


MyDecryptThread::MyDecryptThread(MyFileSystemModel *arg_model, const QString &arg_password, bool
	arg_delete_success) : ptr_model(arg_model), password(arg_password),
	delete_success(arg_delete_success) {}


/*******************************************************************************



*******************************************************************************/


std::vector<int> MyDecryptThread::getStatus() const
{
	if(this->isRunning())
		return std::vector<int>();
	else
		return status_list;
}


/*******************************************************************************



*******************************************************************************/


const std::vector<MyFileInfoPtr> &MyDecryptThread::getItemList() const
{
	if(this->isRunning())
		return empty_list;
	else
		return item_list;
}


/*******************************************************************************



*******************************************************************************/


void MyDecryptThread::interruptionPoint()
{
	if(this->isInterruptionRequested())
	{
		ptr_model->moveToThread(this->thread());
		emit updateProgress(INTERRUPTED);
		throw MyInterrupt();
	}
}


/*******************************************************************************



*******************************************************************************/


void MyDecryptThread::run()
{
	try
	{
		runHelper();
	}
	catch(MyInterrupt &e)
	{
		return;
	}
}


/*******************************************************************************



*******************************************************************************/


void MyDecryptThread::runHelper()
{
	emit updateProgress(MIN);

	// clean up the model first
	ptr_model->removeEmpty();
	interruptionPoint();
	ptr_model->removeRootDir();
	interruptionPoint();
	ptr_model->removeRedundant();
	interruptionPoint();
	ptr_model->sort();
	interruptionPoint();

	int n = ptr_model->rowCount();

	// store the paths of all the items from the model that are checked for decryption
	for(int i = 0; i < n; i++)
	{
		if(ptr_model->data(ptr_model->index(i, CHECKED_HEADER), Qt::CheckStateRole) == Qt::Checked)
		{
			QString full_path = ptr_model->data(ptr_model->index(i,	FULL_PATH_HEADER)).toString();
			item_list.push_back(MyFileInfoPtr(new MyFileInfo(nullptr, full_path)));
		}
	}

	int total_items = item_list.size();
	status_list.resize(total_items, NOT_STARTED);

	interruptionPoint();

	// remove any junk in the temp directory
	QString temp_path = QDir::cleanPath(QDir::currentPath() + "/temp");
	clearDir(temp_path);

	// go through each one and decrypt the file or directory, then unzip
	for(int i = 0; i < total_items; i++)
	{
		interruptionPoint();

		QString full_path = item_list[i]->getFullPath();
		QString decrypt_name;

		// make sure the item is a file with a .qtcrypt extension before attempting to decrypt
		if(item_list[i]->getSuffix() == "qtcrypt")
		{
			// decrypt the file and save the resulting name
			int decrypt_ret_val = myDecryptFile(temp_path, full_path, password, &decrypt_name);

			QString decrypt_path = QDir::cleanPath(QDir::currentPath() + "/temp/" + decrypt_name);
			QString unzip_name = decrypt_name;
			unzip_name.chop(sizeof(".zip") - 1);
			QString unzip_path = QDir::cleanPath(item_list[i]->getPath() + "/" + unzip_name);

			// check if there was an error. if so, do nothing else
			if(decrypt_ret_val != CRYPT_SUCCESS)
				status_list[i] = decrypt_ret_val;
			// otherwise, decryption was successful. unzip the file into the encrypted file's directory
			else
			{
				QuaZip qz(decrypt_path);

				if(QFileInfo::exists(unzip_path))
					status_list[i] = ZIP_ERROR;
				else if(qz.open(QuaZip::mdUnzip))
				{
					// check what item type the zipped file is from the embedded comment. depending on the
					// type, change the directory to extract the zip to

					QString item_type = qz.getComment();
					QString unzip_dir;

					if(item_type == MyFileInfo::typeToString(MyFileInfoPublic::MFIT_FILE))
						unzip_dir = QDir::cleanPath(item_list[i]->getPath());
					else
						unzip_dir = QDir::cleanPath(item_list[i]->getPath() + "/" + unzip_name);

					qz.close();

					// unzip to target directory
					if(JlCompress::extractDir(decrypt_path, unzip_dir).size() != 0)
					{
						// unzipping was a success, process is finished. add in the decrypted item
						status_list[i] = CRYPT_SUCCESS;

						if(item_type == MyFileInfo::typeToString(MyFileInfoPublic::MFIT_FILE))
							ptr_model->insertItem(ptr_model->rowCount(), unzip_path);
						else
							ptr_model->insertItem(0, unzip_path);

						// remove the encrypted file from the model if the user chose the option to
						if(delete_success)
						{
							if(QFile::remove(full_path))
								ptr_model->removeItem(full_path);
						}
					}
					// otherwise, there was a problem unzipping the decrypted file
					else
						status_list[i] = ZIP_ERROR;
				}
				// could not open the decrypted zip file
				else
					status_list[i] = ZIP_ERROR;
			}

			// clean up the decrypted zip file
			QFile::remove(decrypt_path);
		}
		// was not a qtcrypt file
		else
			status_list[i] = NOT_QTCRYPT;

		emit updateProgress((MAX / total_items) * (i + 1));
	}

	emit updateProgress(MAX);
	ptr_model->moveToThread(this->thread());

	emit updateProgress(FINISHED);
}
