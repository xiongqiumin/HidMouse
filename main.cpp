#include "mousedrv.h"
#include <QtGui/QApplication>
#include <QTextCodec>
#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QTextCodec::setCodecForTr(QTextCodec::codecForLocale());	
	QTextCodec::setCodecForCStrings(QTextCodec::codecForLocale());

    QFont font = a.font();    
    font.setPointSize(9);
	a.setFont(font);

    MouseDrv w;
    w.show();
    return a.exec();
}
