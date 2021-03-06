#pragma once

#include "cfilesystemobject.h"
#include "assert/advanced_assert.h"

#include <functional>
#include <vector>
#include <stdint.h>
#include <cmath>

inline std::vector<CFileSystemObject> recurseDirectoryItems(const QString &dirPath, bool includeFolders, const std::function<void (const QString&)>& itemBeingScannedPathCallback = std::function<void (const QString&)>())
{
	std::vector<CFileSystemObject> objects;
	if (QFileInfo(dirPath).isDir())
	{
		QDir dir (dirPath);
		assert_r(dir.exists());
		const QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::Dirs |  QDir::Hidden | QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::System);
		for (const auto item: list)
		{
			if(item.isDir())
			{
				if (itemBeingScannedPathCallback)
					itemBeingScannedPathCallback(item.absoluteFilePath());

				auto childrenItems = recurseDirectoryItems(item.absoluteFilePath(), includeFolders, itemBeingScannedPathCallback);
				objects.insert(objects.end(), childrenItems.begin(), childrenItems.end());
				if (includeFolders)
					objects.emplace_back(item);
			}
			else if (item.isFile())
				objects.emplace_back(item);
		}
	}
	else
		objects.emplace_back(dirPath);

	return objects;
}

inline QString toNativeSeparators(const QString &path)
{
#ifdef _WIN32
	return QString(path).replace('/', '\\');
#else
	return path;
#endif
}

inline QString toPosixSeparators(const QString &path)
{
#ifdef _WIN32
	return QString(path).replace('\\', '/');
#else
	return path;
#endif
}

inline QString cleanPath(const QString& path)
{
	return QString(path).replace("\\\\", "\\").replace("//", "/");
}

inline QString fileSizeToString(uint64_t size, const char maxUnit = '\0', const QString& spacer = QString())
{
	const unsigned int KB = 1024;
	const unsigned int MB = 1024 * KB;
	const unsigned int GB = 1024 * MB;

	const std::map<char, unsigned int> unitCodes {{'B', 0}, {'K', KB}, {'M', MB}};
	const unsigned int maxUnitSize = unitCodes.count(maxUnit) > 0 ? unitCodes.at(maxUnit) : std::numeric_limits<unsigned int>::max();

	QString str;
	float n = 0.0f;
	if (size >= GB && maxUnitSize >= GB)
	{
		n = size / float(GB);
		str = QString("%1 GiB").arg(QString::number(n, 'f', 1));
	}
	else if (size >= MB && maxUnitSize >= MB)
	{
		n = size / float(MB);
		str = QString("%1 MiB").arg(QString::number(n, 'f', 1));
	}
	else if (size >= KB && maxUnitSize >= KB)
	{
		n = size / float(KB);
		str = QString("%1 KiB").arg(QString::number(n, 'f', 1));
	}
	else
	{
		n = (float)size;
		str = QString("%1 B").arg(size);
	}

	if (!spacer.isEmpty() && n > 0.0f)
	{
		for (int spacerPos = (int)std::log10(n) - 3; spacerPos > 0; spacerPos -= 3)
			str.insert(spacerPos + 1, spacer);
	}

	return str;
}
