#ifndef _COMMON_DLG_H_
#define _COMMON_DLG_H_

#include <QtGui/QDialog>
#include "ui_CommonDlg.h"
#include "ui_DefineDialog.h"
class QLineEdit;
struct Stru_DefineNode;

class MoveDialog : public QDialog
{
    Q_OBJECT

protected:
    MoveDialog(QWidget *parent = 0, Qt::WFlags flags = 0);
    ~MoveDialog();

    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

private:    
    //move window
    QPoint windowPos,mousePos,dPos;
    bool m_drag, m_move;
    QPoint dragPos; 
};

class CommonDlg : public MoveDialog
{
    Q_OBJECT

public:
    CommonDlg(QWidget *parent = 0, Qt::WFlags flags = 0);
    ~CommonDlg();

protected:    
    QWidget *centerWidget;

protected:
    Ui::Dialog ui;                   
};

class MessageDlg: public CommonDlg
{
    Q_OBJECT

public:
    static int info(QWidget *parent,QString str);

    MessageDlg(QWidget *parent = 0, Qt::WFlags flags = 0);
    ~MessageDlg();

    void setText(QString str);

protected:
    QLabel *mLabel;
};

//InputDlg
class InputDlg: public CommonDlg
{
    Q_OBJECT

public:
    static QString getInput(QWidget *parent,QString tips,QString str,bool *ok = NULL);
    static int getNum(QWidget *parent,QString tips,int cur = 0,int min = 0,int max = 1000,bool *ok = NULL);

    InputDlg(QWidget *parent = 0, Qt::WFlags flags = 0);
    ~InputDlg();    

protected:
    QLineEdit *mLineEdit; 
};


//ComboKeyDlg
class ComboKeyDlg: public CommonDlg
{
    Q_OBJECT

public:
    static int info(QWidget *parent,QString str);

    ComboKeyDlg(QWidget *parent = 0, Qt::WFlags flags = 0);
    ~ComboKeyDlg();

    static QString makeStr(unsigned char key1,unsigned char key2);    
    QVector<unsigned char> key_list;

    QString getText();

protected:
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

protected:
    QLineEdit *mLineEdit;    
    QVector<bool> key_status;
    bool mKeyDown;
};

//ComboKeyDlg
struct Stru_DefineNode;
typedef QSharedPointer<Stru_DefineNode> DefinePtr;
class DefineDlg: public MoveDialog
{
    Q_OBJECT

public:    
    DefineDlg(QWidget *parent = 0, Qt::WFlags flags = 0);
    ~DefineDlg();

    void setDefine(DefinePtr ptr);
    DefinePtr getSel();

protected slots:
    void accept();

private:
    DefinePtr mSel;

    Ui::Define_Dialog ui;
};

#endif