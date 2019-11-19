#include "CommonDlg.h"
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QKeyEvent>
#include <QIntValidator>
#include "keyTable.h"
#include "MouseDrv.h"

//MoveDialog
MoveDialog::MoveDialog(QWidget *parent, Qt::WFlags flags)
    : QDialog(parent, flags)
{
    m_drag = false;
    m_move = false;
}

MoveDialog::~MoveDialog()
{
}

void MoveDialog::mousePressEvent(QMouseEvent *event)
{       
    if (event->button() == Qt::LeftButton) {
        this->m_drag = true;
        this->dragPos = event->pos();               
    }
}  

void MoveDialog::mouseMoveEvent(QMouseEvent * event)
{    
    if(m_move) {
        move(event->globalPos() - dragPos);
        return;
    }
    QPoint clientCursorPos = event->pos();
    QRect r = this->rect();
    
    //移动窗体        
    if (m_drag && (event->buttons() & Qt::LeftButton)) {
        m_move = true;
        move(event->globalPos() - dragPos);
    }    
}

void MoveDialog::mouseReleaseEvent(QMouseEvent *event)
{
    m_drag = false;
    if(m_move) {
        m_move = false;        
    }    
    setCursor(Qt::ArrowCursor);
}

//CommonDlg
CommonDlg::CommonDlg(QWidget *parent, Qt::WFlags flags)
    : MoveDialog(parent, flags)
{
    ui.setupUi(this);
 
    centerWidget = ui.centerWidget;

    setWindowFlags(Qt::FramelessWindowHint);      
    setFixedSize(size());

    connect(ui.btnOk,SIGNAL(clicked()),this, SLOT(accept()));
    connect(ui.btnCancel,SIGNAL(clicked()),this, SLOT(reject()));
    connect(ui.btnExit,SIGNAL(clicked()),this, SLOT(close()));        
}

CommonDlg::~CommonDlg()
{

}

//MessageDlg
int MessageDlg::info(QWidget *parent,QString str)
{
    MessageDlg dlg;
    dlg.setText(str);
    return dlg.exec();
}

MessageDlg::MessageDlg(QWidget *parent, Qt::WFlags flags)
    : CommonDlg(parent, flags)
{
    mLabel = new QLabel(this);
    mLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(mLabel);
    centerWidget->setLayout(layout);
}

MessageDlg::~MessageDlg()
{

}

void MessageDlg::setText(QString str)
{
    mLabel->setText(str);
}

//InputDlg
InputDlg::InputDlg(QWidget *parent, Qt::WFlags flags)
    : CommonDlg(parent, flags)
{
    mLineEdit = new QLineEdit(this);
    mLineEdit->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);        

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(mLineEdit);
    centerWidget->setLayout(layout);

    mLineEdit->setFocus();
}

InputDlg::~InputDlg()
{

}

QString InputDlg::getInput(QWidget *parent,QString tips,QString str,bool *ok)
{
    InputDlg dlg;
    dlg.ui.labelTips->setText(tips);
    dlg.mLineEdit->setText(str);
    bool ret = (dlg.exec() == QDialog::Accepted);
    if(ok)
        *ok = ret;
    
    if(ret)
        return dlg.mLineEdit->text();
    else
        return str;
}

int InputDlg::getNum(QWidget *parent,QString tips,int cur,int min,int max,bool *ok)
{    
    InputDlg dlg;
    QIntValidator validator(min,max);

    dlg.ui.labelTips->setText(tips);
    dlg.mLineEdit->setText(QString::number(cur));
    dlg.mLineEdit->setValidator(&validator);
    bool ret = (dlg.exec() == QDialog::Accepted);
    if(ok)
        *ok = ret;
    
    if(ret)
        return dlg.mLineEdit->text().toInt();
    else
        return cur;
}

//ComboKeyDlg
ComboKeyDlg::ComboKeyDlg(QWidget *parent, Qt::WFlags flags)
    : CommonDlg(parent, flags)
{
    mLineEdit = new QLineEdit(this);
    mLineEdit->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    mLineEdit->setReadOnly(true);

    mKeyDown = false;

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(mLineEdit);
    centerWidget->setLayout(layout);
}

ComboKeyDlg::~ComboKeyDlg()
{

}

QString ComboKeyDlg::makeStr(unsigned char key1,unsigned char key2)
{
    QString text;
    if(key2 == 0xff)
    {
        text = tr("单键 ") + HidToText(key1);
    }
    else
    {
        text = tr("组合键 ") + HidToText(key1) + " + " + HidToText(key2);
    }
    return text;
}

void ComboKeyDlg::keyPressEvent(QKeyEvent *event)
{
    if(event->isAutoRepeat())
        return;

    int key = event->nativeVirtualKey();
    unsigned char hid = KeyToHid(key);        
    if(hid != 0xff && key_list.size() < 2 && key_list.indexOf(hid) == -1)
    {
        key_status.push_back(false);
        key_list.push_back(hid);
    }
    

    QString text;
    if(key_list.size() == 1)
    {
        text = makeStr(key_list[0],0xff);
    }
    else if(key_list.size() == 2)
    {
        text = makeStr(key_list[0],key_list[1]);
    }
    mLineEdit->setText(text);
}

void ComboKeyDlg::keyReleaseEvent(QKeyEvent *event)
{    
    int key = event->nativeVirtualKey();   
    unsigned char hid = KeyToHid(key);
    if(mKeyDown && !event->isAutoRepeat())
    {
        int idx = key_list.indexOf(hid);
        if(idx >= 0)
            key_status[idx] = true;

        if(key_status.indexOf(false) < 0)
            mKeyDown = false;
    }
}

QString ComboKeyDlg::getText()
{
    return mLineEdit->text();
}

//DefineDlg
DefineDlg::DefineDlg(QWidget *parent, Qt::WFlags flags)
    : MoveDialog(parent, flags)
{
    ui.setupUi(this);    

    setWindowFlags(Qt::FramelessWindowHint);      
    setFixedSize(size());

    connect(ui.btnOk,SIGNAL(clicked()),this, SLOT(accept()));
    connect(ui.btnCancel,SIGNAL(clicked()),this, SLOT(reject()));
    connect(ui.btnExit,SIGNAL(clicked()),this, SLOT(close()));        
}

DefineDlg::~DefineDlg()
{

}

void DefineDlg::setDefine(DefinePtr ptr)
{    
    updateTreeDefine(ui.treeWidget->invisibleRootItem(),ptr);
}

void DefineDlg::accept()
{
    QTreeWidgetItem* tree_item = ui.treeWidget->currentItem();
    DefinePtr define_item;
    if(tree_item)
       define_item = tree_item->data(0,Qt::UserRole).value<DefinePtr>();
    if(!tree_item || define_item->type == 1)
    {
        MessageDlg::info(this,"请先选择一个宏");
        return;
    }

    mSel = define_item;
    return MoveDialog::accept();
}

DefinePtr DefineDlg::getSel()
{
    return mSel;
}