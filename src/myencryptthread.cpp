#include <unordered_map>
#include <string>

#include "JlCompress.h"

#include "myencryptthread.h"
#include "myencryptbar.h"
#include "myinterrupt.h"
#include "myfilesystemmodel.h"
#include "mycleardir.h"

using namespace MyEncryptBarThreadPublic;
using namespace MyEncryptBarPublic;
using namespace MyAbstractBarPublic;
using namespace MyFileSystemModelPublic;
using namespace MyCryptPublic;


/*******************************************************************************



*******************************************************************************/


MyEncryptThread::MyEncryptThread(MyFileSystemModel *arg_model, const QString &arg_password, bool
	arg_delete_success, const QString *arg_encrypt_name) : ptr_model(arg_model),
	password(arg_password), delete_success(arg_delete_success), encrypt_name(arg_encrypt_name) {}


/*******************************************************************************



*******************************************************************************/


std::vector<int> MyEncryptThread::getStatus() const
{
	if(this->isRunning())
		return std::vector<int>();
	else
		return status_list;
}


/*******************************************************************************



*******************************************************************************/


const std::vector<MyFileInfoPtr> &MyEncryptThread::getItemList() const
{
	if(this->isRunning())
		return empty_list;
	else
		return item_list;
}


/*******************************************************************************



*******************************************************************************/


void MyEncryptThread::interruptionPoint()
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


void MyEncryptThread::run()
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


void MyEncryptThread::runHelper()
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

	// store the paths of all the items from the model that are checked for encryption
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

	// for use in naming the encrypted files
	int name_suffix;
	std::unordered_map<std::string, int> suffix_map(total_items);

	// remove any junk in the temp directory
	QString temp_path = QDir::cleanPath(QDir::currentPath() + "/temp");
	clearDir(temp_path);

	// go through each one and encrypt the file or directory after zipping
	for(int i = 0; i < total_items; i++)
	{
		interruptionPoint();

		QString full_path = item_list[i]->getFullPath();
		QString item_type = item_list[i]->getType();

		QString zip_path = QDir::cleanPath(QDir::currentPath() + "/temp/" + item_list[i]->getName() +
			".zip");

		bool ret_val = false;

		// make sure the item exists
		if(item_type != MyFileInfo::typeToString(MyFileInfoPublic::MFIT_EMPTY))
		{
			QuaZip qz(zip_path);

			// try and compress the file or directory
			if(item_type == MyFileInfo::typeToString(MyFileInfoPublic::MFIT_FILE))
			{
				ret_val = JlCompress::compressFile(zip_path, full_path);

				// if the zip file was created, add a comment on what type it originally was
				if(ret_val)
				{
					qz.open(QuaZip::mdAdd);
					qz.setComment(MyFileInfo::typeToString(MyFileInfoPublic::MFIT_FILE));
					qz.close();
				}
			}
			else
			{
				ret_val = JlCompress::compressDir(zip_path, full_path);

				if(ret_val)
				{
					qz.open(QuaZip::mdAdd);
					qz.setComment(MyFileInfo::typeToString(MyFileInfoPublic::MFIT_DIR));
					qz.close();
				}
			}

			// if successful, try encrypting the zipped file
			if(ret_val)
			{
				QString encrypt_path;

				// create the base name of the final encrypted file. defaults to the original filename
				// unless the user specified something for encrypt_name
				if(encrypt_name == nullptr)
					encrypt_path = QDir::cleanPath(full_path + ".qtcrypt");
				else
				{
					QString item_path = item_list[i]->getPath();

					// find the current number suffix for the given directory. 0 if it's the first time
					name_suffix = suffix_map[item_path.toStdString()];

					if(name_suffix != -1)
					{
						// get the next available filename, -1 if none is found within max tries
						name_suffix = getNextFilenameSuffix(item_path, *encrypt_name + "_",	".qtcrypt",
							name_suffix);
					}

					encrypt_path = QDir::cleanPath(item_path + "/" + *encrypt_name + "_" +
						QString::number(name_suffix) + ".qtcrypt");
					suffix_map[item_path.toStdString()] = name_suffix;
				}

				// make sure that a file with the same name as the encrypted doesn't already exist
				if(!QFile::exists(encrypt_path) && name_suffix != -1)
				{
					status_list[i] = myEncryptFile(encrypt_path, zip_path, password);

					// if there was an error, delete the unfinished encryption file
					if(status_list[i] != CRYPT_SUCCESS)
						QFile::remove(encrypt_path);
					// otherwise, successful. add the encrypted item to the model
					else
					{
						ptr_model->insertItem(ptr_model->rowCount(), encrypt_path);

						// remove the old item if the user selected the option to
						if(delete_success)
						{
							if(item_type == MyFileInfo::typeToString(MyFileInfoPublic::MFIT_FILE))
							{
								if(QFile::remove(full_path))
									ptr_model->removeItem(full_path);
							}
							else
							{
								if(QDir(full_path).removeRecursively())
									ptr_model->removeItem(full_path);
							}
						}

						// status_list[i] == CRYPT_SUCCESS
					}
				}
				// the encrypted file already exists. do nothing and assign an error code
				else
					status_list[i] = DES_FILE_EXISTS;

				// delete the intermediate zip file when done
				QFile::remove(zip_path);
			}
			// could not create the intermediate zip file
			else
				status_list[i] = ZIP_ERROR;
		}

		// item_type was empty, meaning the file or directory was not found
		else
			status_list[i] = SRC_NOT_FOUND;

		emit updateProgress((MAX / total_items) * (i + 1));
	}

	emit updateProgress(MAX);
	ptr_model->moveToThread(this->thread());

	emit updateProgress(FINISHED);
}


/*******************************************************************************



*******************************************************************************/


int MyEncryptThread::getNextFilenameSuffix(const QString &path, const QString &base_name, const
	QString &file_type, int start_num, int max_tries) const
{
	QDir qd(path);

	// check if the directory exists
	if(qd.exists())
	{
		// try to find the first name + number that isn't already used
		for(int i = 0; i < max_tries; i++)
		{
			int curr_num = start_num + i;
			QString full_name = base_name + QString::number(curr_num) + file_type;

			if(!qd.exists(full_name))
				return curr_num;
		}

		// no free natural number suffix was found
		return -1;
	}
	// the directory does not exist, thus 0 will work
	else
		return 0;
}
