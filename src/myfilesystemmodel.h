#ifndef MYFILESYSTEMMODEL_H
#define MYFILESYSTEMMODEL_H

#include <memory>
#include <vector>

#include <QAbstractListModel>
#include <QFileDialog>

#include "myfileinfo.h"

class MyFileSystemModel;
typedef std::unique_ptr<MyFileSystemModel> MyFileSystemModelPtr;


/*******************************************************************************



*******************************************************************************/


namespace MyFileSystemModelPublic
{
	// the individual rows in the model make up a file or directory, and the columns are then used to
	// access the various relevant values for that specific item. note that FULL_PATH_HEADER and
	// TYPE_HEADER aren't used by the view and should be hidden
	enum : int
	{
		CHECKED_HEADER, ICON_HEADER, INFO_HEADER, NAME_HEADER, PATH_HEADER, SIZE_HEADER, DATE_HEADER,
		FULL_PATH_HEADER, TYPE_HEADER, COLUMN_NUM
	};
}


/*******************************************************************************



*******************************************************************************/


class MyFileSystemModel : public QAbstractListModel
{
public:
	MyFileSystemModel(QObject *parent);

	//QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
	//QModelIndex parent(const QModelIndex &child) const;
	//int columnCount(const QModelIndex &parent = QModelIndex()) const;

	int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
	int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
	bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) Q_DECL_OVERRIDE;
	bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) Q_DECL_OVERRIDE;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole)
		Q_DECL_OVERRIDE;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const
		Q_DECL_OVERRIDE;
	Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;

	bool isRedundant(const QString &full_path) const;
	std::vector<QString> makesRedundant(const QString &full_path) const;
	MyFileSystemModelPtr cloneByValue() const;
	void startDirThread(int pos);
	void stopDirThread(int pos);
	void stopAllDirThread();
	void startAllDirThread();

public slots:

	void sort();
	void reset();
	void removeItem(const QString &name);
	void removeItems(const std::vector<QString> &rem_list);
	void removeEmpty();
	void removeRedundant();
	void removeCheckedItems();
	void removeRootDir();
	void insertItem(int pos, const QString &name);
	void insertItems(int pos, const std::vector<QString> &ins_list);
	void checkItem(const QModelIndex &index);

protected:

	std::vector<MyFileInfoPtr> file_list;
};

#endif // MYFILESYSTEMMODEL
