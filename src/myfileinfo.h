#ifndef MYFILEINFO_H
#define MYFILEINFO_H

#include <memory>

#include <QString>
#include <QIcon>

#include "mydirthread.h"

class MyFileInfo;
typedef std::unique_ptr<MyFileInfo> MyFileInfoPtr;

namespace MyFileInfoPublic
{
	enum MyFileInfoType : int {MFIT_EMPTY, MFIT_DIR, MFIT_FILE};
}


/*******************************************************************************



*******************************************************************************/


class MyFileInfo : public QObject
{
	Q_OBJECT

public:

	MyFileInfo(MyFileSystemModel* arg_model);
	MyFileInfo(MyFileSystemModel* arg_model, const QString &arg_path);
	//MyFileInfo(const MyFileInfo &rhs);
	//MyFileInfo &operator=(MyFileInfo rhs);
	//MyFileInfo &operator=(const MyFileInfo &rhs);
	~MyFileInfo();

	QString getPath() const;
	QString getType() const;
	QString getName() const;
	QString getSize() const;
	QString getDate() const;
	QString getFullPath() const;
	QString getSuffix() const;
	QIcon getIcon() const;
	Qt::CheckState getChecked() const;
	void setChecked(Qt::CheckState arg_checked);
	void startDirThread();
	void stopDirThread();
	bool equals(const MyFileInfo &rhs) const;
	bool operator==(const MyFileInfo &rhs) const;
	bool contains(const MyFileInfo &rhs) const;
	bool isRedundant(const MyFileInfo &rhs) const;
	bool makesRedundant(const MyFileInfo &rhs) const;

	static QString typeToString(MyFileInfoPublic::MyFileInfoType arg_type);
	void setEmpty();
	void setItem(const QString &arg_path);

private:

	// member functions
	void deleteDirThread();
	void setItemHelper(const QString &arg_path);

	// member variables
	QString name;
	QString path;
	QString date;
	QString full_path;
	MyFileInfoPublic::MyFileInfoType type;
	qint64 size;
	Qt::CheckState checked;
	//MyFileSystemModel *ptr_model;
	QIcon icon;

	// used for potentially calculating directory size, so as to not block the main event loop
	MyFileInfoPublicThread::MyDirThread *dir_thread;

private slots:

	void updateSize(qint64 arg_size);

};	// class MyFileInfo

#endif // MYFILEINFO_H
