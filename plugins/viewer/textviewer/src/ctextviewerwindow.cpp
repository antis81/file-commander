#include "ctextviewerwindow.h"
#include "ui_ctextviewerwindow.h"
#include "ctextencodingdetector.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QTextCodec>

#include <assert.h>


CTextViewerWindow::CTextViewerWindow(QWidget *parent) :
	CPluginWindow(parent),
	_findDialog(this),
	ui(new Ui::CTextViewerWindow)
{
	ui->setupUi(this);

	connect(ui->actionOpen, &QAction::triggered, [this](){
		const QString fileName = QFileDialog::getOpenFileName(this);
		if (!fileName.isEmpty())
			loadTextFile(fileName);
	});
	connect(ui->actionReload, &QAction::triggered, [this](){
		loadTextFile(_sourceFilePath);
	});
	connect(ui->actionClose, SIGNAL(triggered()), SLOT(close()));

	connect(ui->actionFind, &QAction::triggered, [this](){
		_findDialog.exec();
	});
	connect(ui->actionFind_next, SIGNAL(triggered()), SLOT(findNext()));

	connect(ui->actionAuto_detect_encoding, SIGNAL(triggered()), SLOT(asDetectedAutomatically()));
	connect(ui->actionSystemLocale, SIGNAL(triggered()), SLOT(asSystemDefault()));
	connect(ui->actionUTF_8, SIGNAL(triggered()), SLOT(asUtf8()));
	connect(ui->actionUTF_16, SIGNAL(triggered()), SLOT(asUtf16()));
	connect(ui->actionHTML_RTF, SIGNAL(triggered()), SLOT(asRichText()));

	QActionGroup * group = new QActionGroup(this);
	group->addAction(ui->actionSystemLocale);
	group->addAction(ui->actionUTF_8);
	group->addAction(ui->actionUTF_16);
	group->addAction(ui->actionHTML_RTF);

	connect(&_findDialog, SIGNAL(find()), SLOT(find()));
	connect(&_findDialog, SIGNAL(findNext()), SLOT(findNext()));

	auto escScut = new QShortcut(QKeySequence("Esc"), this, SLOT(close()));
	connect(this, SIGNAL(destroyed()), escScut, SLOT(deleteLater()));
}

CTextViewerWindow::~CTextViewerWindow()
{
	delete ui;
}

bool CTextViewerWindow::loadTextFile(const QString& file)
{
	setWindowTitle(file);
	_sourceFilePath = file;
	if (_sourceFilePath.endsWith(".htm", Qt::CaseInsensitive) || _sourceFilePath.endsWith(".html", Qt::CaseInsensitive) || _sourceFilePath.endsWith(".rtf", Qt::CaseInsensitive))
		return asRichText();
	else
		return asDetectedAutomatically();
}

bool CTextViewerWindow::asDetectedAutomatically()
{
	QTextCodec::ConverterState state;
	QTextCodec *codec = QTextCodec::codecForName("UTF-8");
	if (!codec)
		return false;

	QByteArray data;
	if (!readSource(data))
	{
		QMessageBox::warning(dynamic_cast<QWidget*>(parent()), "Failed to read the file", QString("Failed to load the file\n\n") + _sourceFilePath + "\n\nIt is inaccessible or doesn't exist.");
		return false;
	}

	QString text(codec->toUnicode(data.constData(), data.size(), &state));
	if (state.invalidChars > 0)
	{
		text = CTextEncodingDetector::decode(data).first;
		if (!text.isEmpty())
			ui->textBrowser->setPlainText(text);
		else
			return asSystemDefault();
	}
	else
	{
		ui->textBrowser->setPlainText(text);
		ui->actionUTF_8->setChecked(true);
	}

	return true;
}

bool CTextViewerWindow::asSystemDefault()
{
	QTextCodec * codec = QTextCodec::codecForLocale();
	if (!codec)
		return false;

	QByteArray data;
	if (!readSource(data))
	{
		QMessageBox::warning(dynamic_cast<QWidget*>(parent()), "Failed to read the file", QString("Failed to load the file\n\n") + _sourceFilePath + "\n\nIt is inaccessible or doesn't exist.");
		return false;
	}

	ui->textBrowser->setPlainText(codec->toUnicode(data));
	ui->actionSystemLocale->setChecked(true);

	return true;
}

bool CTextViewerWindow::asUtf8()
{
	QByteArray data;
	if (!readSource(data))
	{
		QMessageBox::warning(dynamic_cast<QWidget*>(parent()), "Failed to read the file", QString("Failed to load the file\n\n") + _sourceFilePath + "\n\nIt is inaccessible or doesn't exist.");
		return false;
	}

	ui->textBrowser->setPlainText(QString::fromUtf8((const char*)data.data(), data.size()));
	ui->actionUTF_8->setChecked(true);

	return true;
}

bool CTextViewerWindow::asUtf16()
{
	QByteArray data;
	if (!readSource(data))
	{
		QMessageBox::warning(dynamic_cast<QWidget*>(parent()), "Failed to read the file", QString("Failed to load the file\n\n") + _sourceFilePath + "\n\nIt is inaccessible or doesn't exist.");
		return false;
	}

	ui->textBrowser->setPlainText(QString::fromUtf16((const ushort*)data.data(), data.size()/2));
	ui->actionUTF_16->setChecked(true);

	return true;
}

bool CTextViewerWindow::asRichText()
{
	ui->textBrowser->setSource(QUrl::fromLocalFile(_sourceFilePath));
	ui->actionHTML_RTF->setChecked(true);

	return true;
}

void CTextViewerWindow::find()
{
	ui->textBrowser->moveCursor(_findDialog.searchBackwards() ? QTextCursor::End : QTextCursor::Start);
	findNext();
}

void CTextViewerWindow::findNext()
{
	const QString expression = _findDialog.searchExpression();
	if (expression.isEmpty())
		return;

	QTextDocument::FindFlags flags = 0;
	if (_findDialog.caseSensitive())
		flags |= QTextDocument::FindCaseSensitively;
	if (_findDialog.searchBackwards())
		flags |= QTextDocument::FindBackward;
	if (_findDialog.wholeWords())
		flags |= QTextDocument::FindWholeWords;

	bool found;
	const QTextCursor startCursor = ui->textBrowser->textCursor();
#if  QT_VERSION >= QT_VERSION_CHECK(5,3,0)
	if (_findDialog.regex())
		found = ui->textBrowser->find(QRegExp(_findDialog.searchExpression(), _findDialog.caseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive), flags);
	else
#endif
		found = ui->textBrowser->find(_findDialog.searchExpression(), flags);

	if(!found && (startCursor.isNull() || startCursor.position() == 0))
		QMessageBox::information(this, "Not found", QString("Expression \"")+expression+"\" not found");
	else if (!found && startCursor.position() > 0)
	{
		if (QMessageBox::question(this, "Not found", _findDialog.searchBackwards() ? "Beginning of file reached, do you want to restart search from the end?" : "End of file reached, do you want to restart search from the top?") == QMessageBox::Yes)
			find();
	}
}

bool CTextViewerWindow::readSource(QByteArray& data) const
{
	QFile file(_sourceFilePath);
	if (file.exists() && file.open(QIODevice::ReadOnly))
	{
		data = file.readAll();
		return data.size() > 0 || file.size() == 0;
	}
	else
		return false;
}
