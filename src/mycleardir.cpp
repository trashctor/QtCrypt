#include <QDir>

#include "mycleardir.h"


/*******************************************************************************



*******************************************************************************/


void clearDir(QString dir_path)
{
	QDir qd(dir_path);

	if(qd.exists())
	{
		QList<QFileInfo> item_list = qd.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoSymLinks |
			QDir::NoDotAndDotDot | QDir::Hidden);

		for(auto it = item_list.begin(); it != item_list.end(); ++it)
		{
			if(it->isDir())
				QDir(it->absoluteFilePath()).removeRecursively();
			else if(it->isFile())
				QDir().remove(it->absoluteFilePath());
		}
	}
}
