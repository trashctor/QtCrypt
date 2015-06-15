#include "myfilesystemmodel.h"

using namespace MyFileSystemModelPublic;
using namespace MyFileInfoPublic;


/*******************************************************************************



*******************************************************************************/


namespace
{
	QString header_arr[COLUMN_NUM] = {"Selected", "Icon", "Info", "Name", "Path", "Size",
		"Date Modified"};
}

/*******************************************************************************



*******************************************************************************/


MyFileSystemModel::MyFileSystemModel(QObject *parent) : QAbstractListModel(parent) {}


/*******************************************************************************



*******************************************************************************/


int MyFileSystemModel::rowCount(const QModelIndex &parent) const
{
	return file_list.size();
}


/*******************************************************************************



*******************************************************************************/


int MyFileSystemModel::columnCount(const QModelIndex &parent) const
{
	return COLUMN_NUM;
}


/*******************************************************************************



*******************************************************************************/


QVariant MyFileSystemModel::data(const QModelIndex &index, int role) const
{
	// request for text to display
	if(role == Qt::DisplayRole)
	{
		switch(index.column())
		{
			case NAME_HEADER:
				return (file_list.at(index.row()))->getName();

			case INFO_HEADER:
			{
				QString item_type = file_list.at(index.row())->getType();

				if(item_type == MyFileInfo::typeToString(MFIT_FILE))
					return item_type + " (" + file_list.at(index.row())->getSuffix() + ")";
				else
					return item_type;
			}

			case PATH_HEADER:
				return (file_list.at(index.row()))->getPath();

			case SIZE_HEADER:
				return (file_list.at(index.row()))->getSize();

			case DATE_HEADER:
				return (file_list.at(index.row()))->getDate();

			case FULL_PATH_HEADER:
				return (file_list.at(index.row()))->getFullPath();

			case TYPE_HEADER:
				return (file_list.at(index.row()))->getType();

			default:
				return QVariant();
		}
	}

	// request for the state of a check box
	else if(role == Qt::CheckStateRole)
	{
		switch(index.column())
		{
			case CHECKED_HEADER:
				return file_list.at(index.row())->getChecked();

			default:
				return QVariant();
		}
	}

	else if(role == Qt::DecorationRole)
	{
		switch(index.column())
		{
			case ICON_HEADER:
				return file_list[index.row()]->getIcon();

			default:
				return QVariant();
		}
	}

	return QVariant();
}


/*******************************************************************************



*******************************************************************************/


bool MyFileSystemModel::removeRows(int row, int count, const QModelIndex &parent)
{
	if(count > 0)
	{
		beginRemoveRows(QModelIndex(), row, row + count - 1);

		for(int i = 0; i < count; i++)
			file_list.erase(file_list.begin() + row);

		endRemoveRows();

		return true;
	}

	return false;
}


/*******************************************************************************



*******************************************************************************/


void MyFileSystemModel::removeCheckedItems()
{
	int n = file_list.size();

	for(int curr = 0, i = 0; i < n; i++)
	{
		if(file_list[curr]->getChecked())
		{
			beginRemoveRows(QModelIndex(), curr, curr);
			file_list.erase(file_list.begin() + curr);
			endRemoveRows();
		}
		else
			curr++;
	}
}


/*******************************************************************************



*******************************************************************************/


bool MyFileSystemModel::insertRows(int row, int count, const QModelIndex &parent)
{
	if(count > 0)
	{
		beginInsertRows(QModelIndex(), row, row + count - 1);

		for(int i = 0; i < count; i++)
			file_list.insert(file_list.begin() + row, MyFileInfoPtr(new MyFileInfo(this)));

		endInsertRows();

		return true;
	}

	return false;
}


/*******************************************************************************



*******************************************************************************/


bool MyFileSystemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if(role == Qt::EditRole)
	{
		file_list[index.row()] = MyFileInfoPtr(new MyFileInfo(this, value.toString()));
		//file_list[index.row()]->startDirThread();
		emit dataChanged(index, index);
		return true;
	}

	else if(role == Qt::CheckStateRole)
	{
		// convert QVariant into Qt::CheckState
		file_list[index.row()]->setChecked(reinterpret_cast<const Qt::CheckState &>(value));
		emit dataChanged(index, index);
		return true;
	}

	return false;
}


/*******************************************************************************



*******************************************************************************/


QVariant MyFileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role == Qt::DisplayRole && section >= 0 && section < COLUMN_NUM)
	{
		// anonymous namespace global variable defined above
		return header_arr[section];
	}

	return QVariant();
}


/*******************************************************************************



*******************************************************************************/


Qt::ItemFlags MyFileSystemModel::flags(const QModelIndex &index) const
{
	if(index.column() == CHECKED_HEADER)
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
	else
		return QAbstractListModel::flags(index);
}


/*******************************************************************************



*******************************************************************************/


bool MyFileSystemModel::isRedundant(const QString &full_path) const
{
	int n = file_list.size();
	MyFileInfo mfi(nullptr, full_path);

	for(int i = 0; i < n; i++)
	{
		if(mfi.isRedundant(*file_list[i]))
			return true;
	}

	return false;
}


/*******************************************************************************



*******************************************************************************/


std::vector<QString> MyFileSystemModel::makesRedundant(const QString &full_path) const
{
	int n = file_list.size();
	MyFileInfo mfi(nullptr, full_path);
	std::vector<QString> red_list;

	for(int i = 0; i < n; i++)
	{
		if(mfi.makesRedundant(*file_list[i]))
			red_list.push_back(file_list[i]->getFullPath());
	}

	return red_list;
}


/*******************************************************************************



*******************************************************************************/


MyFileSystemModelPtr MyFileSystemModel::cloneByValue() const
{
	MyFileSystemModelPtr ret_model(new MyFileSystemModel(nullptr));

	int n = file_list.size();

	for(int i = 0; i < n; i++)
	{
		// extract the path for each MyFileInfo in this->file_list and use it to create the
		// corresponding new MyFileInfo in the cloned model
		QString full_path = file_list[i]->getFullPath();
		ret_model->file_list.push_back(MyFileInfoPtr(new MyFileInfo(ret_model.get(), full_path)));

		// also duplicate the checked state of each item
		ret_model->file_list[i]->setChecked(file_list[i]->getChecked());
	}

	return ret_model;
}


/*******************************************************************************



*******************************************************************************/


void MyFileSystemModel::sort()
{
	const int n = file_list.size();

	beginRemoveRows(QModelIndex(), 0, n);

	// curr represents the current unique item being sorted, front is the initial "0" index before
	// being shifted by insertions at the beginning
	for(int curr = 0, front = 0, i = 0; i < n; i++)
	{
		// if a directory is found, dump it to the front of the list. if an empty item is found, dump it
		// to the end of the list. files will end up in the middle.
		if(file_list[curr]->getType() == MyFileInfo::typeToString(MFIT_DIR))
		{
			file_list.insert(file_list.begin() + front, MyFileInfoPtr());
			curr++;
			file_list[front] = std::move(file_list[curr]);
			file_list.erase(file_list.begin() + curr);
			front++;
		}
		else if(file_list[curr]->getType() == MyFileInfo::typeToString(MFIT_EMPTY))
		{
			file_list.push_back(MyFileInfoPtr());
			file_list[n] = std::move(file_list[curr]);
			file_list.erase(file_list.begin() + curr);
		}
		else
			curr++;
	}

	endRemoveRows();
}


/*******************************************************************************



*******************************************************************************/


void MyFileSystemModel::reset()
{
	beginRemoveRows(QModelIndex(), 0, file_list.size());
	file_list.clear();
	endRemoveRows();
}


/*******************************************************************************



*******************************************************************************/


void MyFileSystemModel::removeItem(const QString &name)
{
	int n = file_list.size();
	MyFileInfo mfi(nullptr, name);

	for(int i = 0; i < n; i++)
		if(file_list[i]->equals(mfi))
		{
			beginRemoveRows(QModelIndex(), i, i);
			file_list.erase(file_list.begin() + i);
			endRemoveRows();
			break;
		}
}


/*******************************************************************************



*******************************************************************************/


void MyFileSystemModel::removeItems(const std::vector<QString> &rem_list)
{
	int n = rem_list.size();

	for(int i = 0; i < n; i++)
		removeItem(rem_list[i]);
}


/*******************************************************************************



*******************************************************************************/


void MyFileSystemModel::removeEmpty()
{
	int n = file_list.size();
	const QString empty_mfi = MyFileInfo::typeToString(MFIT_EMPTY);

	// loop n times using i, with curr as the element for checking if empty
	for(int curr = 0, i = 0; i < n; i++)
	{
		if(file_list[curr]->getType() == empty_mfi)
		{
			beginRemoveRows(QModelIndex(), curr, curr);
			file_list.erase(file_list.begin() + curr);
			endRemoveRows();
		}
		else
			curr++;
	}
}


/*******************************************************************************



*******************************************************************************/


void MyFileSystemModel::removeRedundant()
{
	int n = file_list.size();

	// set i and compare it against all the other elements, j
	for(int i = 0; i < n; i++)
	{
		for(int j = 0; j < n; j++)
		{
			// see if i is contained by or equal to j. if so, remove it from the file list
			if(i != j && file_list[i]->isRedundant(*file_list[j]))
			{
				beginRemoveRows(QModelIndex(), i, i);
				file_list.erase(file_list.begin() + i);
				endRemoveRows();

				// i doesn't need to increment since everything in the list was shifted over by 1
				i--;
				n--;
				break;
			}
		}
	}

}


/*******************************************************************************



*******************************************************************************/


void MyFileSystemModel::removeRootDir()
{
	int n = file_list.size();

	for(int curr = 0, i = 0; i < n; i++)
	{
		// assign to a QDir object and use it to check if the current item is a root directory

		QDir q_dir(QDir::cleanPath(file_list[curr]->getFullPath()));

		if(q_dir.isRoot())
		{
			beginRemoveRows(QModelIndex(), curr, curr);
			file_list.erase(file_list.begin() + curr);
			endRemoveRows();
		}
		else
			curr++;
	}
}


/*******************************************************************************



*******************************************************************************/


void MyFileSystemModel::insertItem(int pos, const QString &name)
{
	beginInsertRows(QModelIndex(), pos, pos);

	MyFileInfo *ptr_mfi = new MyFileInfo(this, name);
	MyFileInfoPtr unique_mfi(ptr_mfi);

	file_list.insert(file_list.begin() + pos, std::move(unique_mfi));


	//file_list.insert(file_list.begin() + pos, MyFileInfoPtr(new MyFileInfo(this, name)));
	endInsertRows();
	//file_list[pos]->startDirThread();
}


/*******************************************************************************



*******************************************************************************/


void MyFileSystemModel::insertItems(int pos, const std::vector<QString> &ins_list)
{
	int n = ins_list.size();

	for(int i = 0; i < n; i++)
		insertItem(pos + i, ins_list[i]);
}


/*******************************************************************************



*******************************************************************************/


void MyFileSystemModel::startDirThread(int pos)
{
	file_list[pos]->startDirThread();
}


/*******************************************************************************



*******************************************************************************/


void MyFileSystemModel::stopDirThread(int pos)
{
	file_list[pos]->stopDirThread();
}


/*******************************************************************************



*******************************************************************************/


void MyFileSystemModel::startAllDirThread()
{
	for(auto it = file_list.begin(); it != file_list.end(); ++it)
		(*it)->startDirThread();
}


/*******************************************************************************



*******************************************************************************/


void MyFileSystemModel::stopAllDirThread()
{
	for(auto it = file_list.begin(); it != file_list.end(); ++it)
		(*it)->stopDirThread();
}


/*******************************************************************************



*******************************************************************************/


void MyFileSystemModel::checkItem(const QModelIndex &index)
{
		Qt::CheckState new_state = file_list[index.row()]->getChecked() == Qt::Unchecked ? Qt::Checked :
			Qt::Unchecked;

		file_list[index.row()]->setChecked(new_state);
		emit dataChanged(index, index);
}
