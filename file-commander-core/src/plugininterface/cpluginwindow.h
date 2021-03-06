#ifndef CVIEWERWINDOW_H
#define CVIEWERWINDOW_H

#include "plugin_export.h"
#include "compiler/compiler_warnings_control.h"

DISABLE_COMPILER_WARNINGS
#include <QMainWindow>
RESTORE_COMPILER_WARNINGS

class PLUGIN_EXPORT CPluginWindow : public QMainWindow
{
	Q_OBJECT

public:
	CPluginWindow();
	~CPluginWindow();

	bool autoDeleteOnClose() const;
	void setAutoDeleteOnClose(bool autoDelete);
};

#endif // CVIEWERWINDOW_H
