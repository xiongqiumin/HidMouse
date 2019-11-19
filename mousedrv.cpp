#include "mousedrv.h"
#include "CommonDlg.h"
#include <QMouseEvent>
#include <QMenu>
#include <QUuid>
#include <QFileDialog>

#include "KeyTable.h"

const int MOUSE_KEY_NUM = 9;

//Stru_MouseMap
Stru_MouseMap::Stru_MouseMap()
{
    mode = 0;
    Q_ASSERT(0);
}

Stru_MouseMap::Stru_MouseMap(int map_mode,uchar var0,uchar var1,uchar var2)
{
    para.resize(3);

    mode = map_mode;
    para[0] = var0;
    para[1] = var1;
    para[2] = var2;
}

bool Stru_MouseMap::operator ==(const Stru_MouseMap &other) const
{
    if(mode == other.mode && para == other.para)
        return true;

    return false;
}

//Stru_DefineNode
Stru_DefineNode::Stru_DefineNode()
{
    type = 0;
    defineType = 0;
    guid = QUuid::createUuid();
}

QDataStream& operator >>(QDataStream &stream,DefinePtr &profile)
{
    unsigned char buffer[KEY_DEFINE_LENGTH];
    if(!profile)
        profile = DefinePtr(new Stru_DefineNode());

    stream >> profile->type;
    stream >> profile->name;
    stream.readRawData((char*)buffer,KEY_DEFINE_LENGTH);
    profile->define.FromBuffer(buffer);
    stream >> profile->defineType;
    stream >> profile->dir;
    stream >> profile->guid;

    return stream;
}

QDataStream& operator <<(QDataStream &stream,const DefinePtr &profile)
{
    unsigned char buffer[KEY_DEFINE_LENGTH];    

    stream << profile->type;
    stream << profile->name;
    profile->define.ToBuffer(buffer);
    stream.writeRawData((char*)buffer,KEY_DEFINE_LENGTH);  
    stream << profile->defineType;
    stream << profile->dir;
    stream << profile->guid;

    return stream;
}

void updateTreeDefine(QTreeWidgetItem *item,DefinePtr defineNode)
{        
    item->setData(0,Qt::UserRole,QVariant::fromValue(defineNode));        
    item->setText(0,defineNode->name);
    
    if(defineNode->type == 1)
    {
        QIcon icon; 
        //icon.addPixmap(QPixmap("./MouseDrv/skins/open.png"), QIcon::Normal, QIcon::On);//节点打开状态 
        icon.addPixmap(QPixmap(":/MouseDrv/skins/close.png"), QIcon::Normal, QIcon::Off);//节点关闭状态　　
        item->setIcon(0,icon);

        for(int i = 0; i < defineNode->dir.size();i++)
            addTreeDefine(item,defineNode->dir[i]);                                
    }
}

void addTreeDefine(QTreeWidgetItem *item,DefinePtr defineNode)
{
    QTreeWidgetItem *sub_item = new QTreeWidgetItem(item);
    updateTreeDefine(sub_item,defineNode);
    item->addChild(sub_item);
}

//Stru_MouseProfile
Stru_MouseProfile::Stru_MouseProfile()
{       
    reportGap = 0;
    dpiLevel.resize(8);
    defineGuid.resize(MOUSE_KEY_NUM);
}

QDataStream& operator >>(QDataStream &stream,Stru_MouseProfile &profile)
{	    
    stream >> profile.name;
    stream.readRawData((char*)profile.para.value,64);
    stream.readRawData((char*)profile.map.value,64);    
    stream >> profile.reportGap;    
    stream >> profile.defineGuid;
    stream >> profile.dpiLevel;

	return stream;
}

QDataStream& operator <<(QDataStream &stream,const Stru_MouseProfile &profile)
{
    stream << profile.name;
    stream.writeRawData((char*)profile.para.value,64);
    stream.writeRawData((char*)profile.map.value,64);    
    stream << profile.reportGap;    
    stream << profile.defineGuid;   
    stream << profile.dpiLevel;

    return stream;
}

//MouseDrv
MouseDrv::MouseDrv(QWidget *parent, Qt::WFlags flags)
    : QMainWindow(parent, flags)
{
    ui.setupUi(this);

    mDefineRoot = DefinePtr(new Stru_DefineNode());
    mDefineRoot->type = 1;     

    mProfile = NULL;
    mProFileIdx = 0;
    m_bInit = false;

    mSensorType = SENSOR_INVAILD;
    mMouseID = 0x7D;    
    mRecord = false;
    m_drag = m_move = false;
    mSaveFlag = SAVE_NONE;
    mProfilePath = QCoreApplication::applicationDirPath() + "/Profile.bin";

    mMenuStyle = "QMenu{color: lightgray; background-color:rgb(30,30,30); border:0px;}"                   
                 "QMenu::item:selected {color: black; background-color: rgb(0,158,193);}";
   
    setWindowFlags(Qt::FramelessWindowHint);      
    setFixedSize(size());

    mBtnKey.push_back(ui.btnKey1);
    mBtnKey.push_back(ui.btnKey2);
    mBtnKey.push_back(ui.btnKey3);
    mBtnKey.push_back(ui.btnKey4);
    mBtnKey.push_back(ui.btnKey5);
    mBtnKey.push_back(ui.btnKey6);
    mBtnKey.push_back(ui.btnKey7);
    mBtnKey.push_back(ui.btnKey8);
    mBtnKey.push_back(ui.btnKey9);

    mRadioDefine.push_back(ui.radioDefineType1);
    mRadioDefine.push_back(ui.radioDefineType2);
    mRadioDefine.push_back(ui.radioDefineType3);    

    ui.lineDefineRelay->setValidator(new QIntValidator(1, 255, this));

    initDpiPage();
    mSensorType = SENSOR_INVAILD;

    ui.treeDefineList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui.treeDefineList,SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),this,SLOT(treeKeyUpdate()));
    connect(ui.treeDefineList,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(treeDefineContextMenu(const QPoint&)));

    ui.treeKeyList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui.treeKeyList,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(treeKeyContextMenu(const QPoint&)));    
    connect(ui.treeKeyList,SIGNAL(itemDoubleClicked(QListWidgetItem*)),this,SLOT(treeKeyDoubleClicked(QListWidgetItem*)));    

    connect(ui.btnMin,SIGNAL(clicked()),this, SLOT(onBtnMin()));
    connect(ui.btnExit,SIGNAL(clicked()),this, SLOT(onBtnExit()));    
    
    foreach(QPushButton *btn,mBtnKey)
        connect(btn,SIGNAL(clicked()),this, SLOT(onBtnKey()));

    foreach(QRadioButton *btn,mRadioDefine)
        connect(btn,SIGNAL(toggled(bool)),this, SLOT(onBtnDefineType()));

    connect(ui.sliderLedSpeed,SIGNAL(sliderReleased()),this, SLOT(updateSpeedPara()));
    connect(ui.sliderLedSpeed,SIGNAL(valueChanged(int)),this, SLOT(updateSpeedLabel()));    

    connect(ui.btnProfileSel,SIGNAL(clicked()),this, SLOT(onBtnProfileSel()));
    connect(ui.btnProfileSet,SIGNAL(clicked()),this, SLOT(onBtnProfileSet()));
    connect(ui.btnLed,SIGNAL(clicked()),this, SLOT(onBtnLed()));
    connect(ui.btnFreq,SIGNAL(clicked()),this, SLOT(onBtnFreq()));

    connect(ui.btnSetKey,SIGNAL(clicked()),this, SLOT(onTabChange()));
    connect(ui.btnSetDpi,SIGNAL(clicked()),this, SLOT(onTabChange()));
    connect(ui.btnSetMouse,SIGNAL(clicked()),this, SLOT(onTabChange()));
    connect(ui.btnSetDefine,SIGNAL(clicked()),this, SLOT(onTabChange()));

    connect(ui.btnSave,SIGNAL(clicked()),this, SLOT(onBtnSave()));
    connect(ui.btnRecover,SIGNAL(clicked()),this, SLOT(onBtnRecover()));
    connect(ui.btnStartDefine,SIGNAL(clicked()),this, SLOT(onBtnStartDefine()));
    connect(ui.btnSaveDefine,SIGNAL(clicked()),this, SLOT(onBtnSaveDefine()));

    connect(&mHidTimer,SIGNAL(timeout()),this, SLOT(onHidTimerOut()));

    initHrdMouse();
    initMapTable();

    LoadDefine();
    if(!LoadProFile(mProfilePath))
        InitProFile();
    initMouseSpecial();
    
    updateTabColor();    
    updateTreeDefine(ui.treeDefineList->invisibleRootItem(),mDefineRoot);                     
    mHidTimer.start(2000);

    m_bInit = true;
}

MouseDrv::~MouseDrv()
{
    SaveProFile(mProfilePath);
    SaveDefine();
}

void MouseDrv::initDpiPage()
{
    mDpiChkBox.push_back(ui.chkBoxDPI1);
    mDpiChkBox.push_back(ui.chkBoxDPI2);
    mDpiChkBox.push_back(ui.chkBoxDPI3);
    mDpiChkBox.push_back(ui.chkBoxDPI4);
    mDpiChkBox.push_back(ui.chkBoxDPI5);
    mDpiChkBox.push_back(ui.chkBoxDPI6);

    mDpiSlider.push_back(ui.sliderDPI1);
    mDpiSlider.push_back(ui.sliderDPI2);
    mDpiSlider.push_back(ui.sliderDPI3);
    mDpiSlider.push_back(ui.sliderDPI4);
    mDpiSlider.push_back(ui.sliderDPI5);
    mDpiSlider.push_back(ui.sliderDPI6);

    mDpiLabel.push_back(ui.labelDPI1);
    mDpiLabel.push_back(ui.labelDPI2);
    mDpiLabel.push_back(ui.labelDPI3);
    mDpiLabel.push_back(ui.labelDPI4);
    mDpiLabel.push_back(ui.labelDPI5);
    mDpiLabel.push_back(ui.labelDPI6);

    mDpiColor.push_back(ui.btnDPIColor1);
    mDpiColor.push_back(ui.btnDPIColor2);
    mDpiColor.push_back(ui.btnDPIColor3);
    mDpiColor.push_back(ui.btnDPIColor4);
    mDpiColor.push_back(ui.btnDPIColor5);
    mDpiColor.push_back(ui.btnDPIColor6);

    for(int i = 0; i < mDpiSlider.size(); i++)
    {                
        connect(mDpiSlider[i],SIGNAL(valueChanged(int)),this, SLOT(updateDPILabel()));
        connect(mDpiSlider[i],SIGNAL(sliderReleased()),this, SLOT(updateDPIPara()));
        connect(mDpiChkBox[i],SIGNAL(clicked()),this, SLOT(onDPIChkBox()));
        connect(mDpiColor[i],SIGNAL(colorChanged()),this, SLOT(onDPIColorChanged()));

        mDpiSlider[i]->setRange(0,DPI_TABLE.size() - 1);
        mDpiSlider[i]->setValue(0);        
        mDpiSlider[i]->setSingleStep(1);
    }
}

void MouseDrv::initMapTable()
{	
    int sensor_type = (mSensorType == SENSOR_INVAILD)? SENSOR_ADNS3050: mSensorType;

    DPI_TABLE.clear();    
    if(sensor_type == SENSOR_ADNS3050 || sensor_type == SENSOR_ADNS3000)
    {
        DPI_TABLE.push_back(250);
        DPI_TABLE.push_back(500);
        DPI_TABLE.push_back(1000);
        DPI_TABLE.push_back(1250);
        DPI_TABLE.push_back(1500);
        DPI_TABLE.push_back(1750);
        DPI_TABLE.push_back(2000);
        DPI_TABLE.push_back(2500);
        DPI_TABLE.push_back(3000);
        DPI_TABLE.push_back(3500);
        DPI_TABLE.push_back(4000);
    }
    else 
    {
        DPI_TABLE.push_back(250);
        DPI_TABLE.push_back(500);
        DPI_TABLE.push_back(1000);
        DPI_TABLE.push_back(1250);
        DPI_TABLE.push_back(1375);
        DPI_TABLE.push_back(1500);
        DPI_TABLE.push_back(2000);
        DPI_TABLE.push_back(2500);
        DPI_TABLE.push_back(2750);
    }

    //鼠标按键
    mouseMap.insert("左键",Stru_MouseMap(MODE_MOUSE,0,0xF0,0));
    mouseMap.insert("右键",Stru_MouseMap(MODE_MOUSE,0,0xF1,0));
    mouseMap.insert("中键",Stru_MouseMap(MODE_MOUSE,0,0xF2,0));
    mouseMap.insert("前进",Stru_MouseMap(MODE_MOUSE,0,0xF3,0));
    mouseMap.insert("后退",Stru_MouseMap(MODE_MOUSE,0,0xF4,0));
    mouseMap.insert("向上滚",Stru_MouseMap(MODE_MOUSE,0,0xF7,0));
    mouseMap.insert("向下滚",Stru_MouseMap(MODE_MOUSE,0,0xF8,0));    
    
    //DPI 键功能
    mouseMap.insert("DPI +",Stru_MouseMap(MODE_DPI,0,0,0));
    mouseMap.insert("DPI -",Stru_MouseMap(MODE_DPI,0,1,0));
    mouseMap.insert("DPI loop",Stru_MouseMap(MODE_DPI,0,2,0));

    //灯光 开关/关闭
    mouseMap.insert("灯光 开关/关闭",Stru_MouseMap(MODE_LED_ONOFF,0,2,0));

    //禁用
    mouseMap.insert("禁用",Stru_MouseMap(MODE_DISABLE,0,0,0));
    
    //led
    ledMap.insert("常亮模式",0);
    ledMap.insert("呼吸模式",1);
    ledMap.insert("光谱模式",2);
    ledMap.insert("APM 模式",3);

    //
    mReportGapMap.insert("1000Hz",0);
    mReportGapMap.insert("500Hz",1);
    mReportGapMap.insert("250Hz",2);
    mReportGapMap.insert("125Hz",3);

    //
    mMediaMap.insert("播放键",Stru_MouseMap(MODE_MEDIA,0,0x83,0x01));    
    mMediaMap.insert("电子邮件",Stru_MouseMap(MODE_MEDIA,0,0x8A,0x01));    
    mMediaMap.insert("静音",Stru_MouseMap(MODE_MEDIA,0,0xE2,00));    
    mMediaMap.insert("音量+",Stru_MouseMap(MODE_MEDIA,0,0xE9,00));    
    mMediaMap.insert("音量-",Stru_MouseMap(MODE_MEDIA,0,0xEA,00));    
    mMediaMap.insert("播放/暂停",Stru_MouseMap(MODE_MEDIA,0,0xCD,00));    
    mMediaMap.insert("停止",Stru_MouseMap(MODE_MEDIA,0,0xB7,00));    
    mMediaMap.insert("上一曲",Stru_MouseMap(MODE_MEDIA,0,0xB6,00));    
    mMediaMap.insert("下一曲",Stru_MouseMap(MODE_MEDIA,0,0xB5,00));    
    mMediaMap.insert("主页",Stru_MouseMap(MODE_MEDIA,0,0x23,02));    
    mMediaMap.insert("资源管理器",Stru_MouseMap(MODE_MEDIA,0,0x94,0x01));    
    mMediaMap.insert("计算器",Stru_MouseMap(MODE_MEDIA,0,0x92,0x01));    

    //
    mouseButtonMap.insert("鼠标左键",Qt::LeftButton);
    mouseButtonMap.insert("鼠标右键",Qt::RightButton);    
}

void MouseDrv::LoadDefine()
{
    QString file_name = QCoreApplication::applicationDirPath() + "/Define.bin";
    QFile file(file_name);
    if(file.open(QIODevice::ReadOnly))
    {
	    QDataStream in(&file);        
        in >> mDefineRoot;

	    file.close();
    }    
}

void MouseDrv::SaveDefine()
{
    QString file_name = QCoreApplication::applicationDirPath() + "/Define.bin";
    QFile file(file_name);
    if(file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
	    QDataStream out(&file);        
        out << mDefineRoot;

	    file.close();
    }
}

void MouseDrv::InitProFile()
{        
    Stru_MouseProfile base;    

    unsigned char cpi[8] = {0x03,0x05,0x07,0x09,0x80,0x80,0x80,0x80};
    unsigned char color[8][3] = {{0xFF,0,0},{0,0xFF,0},{0,0,0xFF},{0xFF,0,0xFF},{0,0,0},{0,0,0},{0,0,0},{0,0,0}};    
    for(int i = 0; i < 8; i++)
    {
        base.para.CPI[i] = cpi[i]; 
        base.para.CPIColor[i][0] = color[i][0];
        base.para.CPIColor[i][1] = color[i][1];
        base.para.CPIColor[i][2] = color[i][2];

        base.dpiLevel[i] = (cpi[i] == 0x80)? 0:cpi[i];
    }
    base.para.Color[0] = 0xFF;
    base.para.Color[1] = 0x0;
    base.para.Color[2] = 0xFF;
    base.para.Color[3] = 0x0;
    base.para.LedMode = 1;    
    base.para.LedSpeed = 0x10;

    QStringList key_list = QString("左键,右键,中键,前进,后退,DPI -,DPI +,向上滚,向下滚").split(",");   
    for(int i = 0; i < MOUSE_KEY_NUM; i++)
    {
        int hrd_idx = toHrdIdx(i);
        const Stru_MouseMap &map_info = mouseMap[key_list[i]];
                
        base.map.keyMap[hrd_idx][0] = map_info.mode;
        base.map.keyMap[hrd_idx][1] = map_info.para[0];
        base.map.keyMap[hrd_idx][2] = map_info.para[1];
        base.map.keyMap[hrd_idx][3] = map_info.para[2];
    }    
    
    mProfileList.resize(5);
    for(int i = 0; i < 5; i ++)
    {        
        mProfileList[i] = base;
        mProfileList[i].name = QString("profile%0").arg(i + 1);
    }

    mProFileIdx = 0;
    mProfile = NULL;    
    SwitchProFile(0);
}

void MouseDrv::SaveProFile(QString file_name)
{    
    QFile file(file_name);
    if(file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
	    QDataStream out(&file);
        out << mProFileIdx;
	    out << mProfileList;	        

	    file.close();
    }
}

bool MouseDrv::LoadProFile(QString file_name)
{
    int idx = -1;
    QVector<Stru_MouseProfile> tmp;
    
    QFile file(file_name);
    if(file.open(QIODevice::ReadOnly))
    {
	    QDataStream in(&file);
        in >> idx;
	    in >> tmp;	        

	    file.close();
    }
    
    if(idx >= 0 && idx < 5 && tmp.size() == 5)     
    {
        mProfileList = tmp;
        SwitchProFile(idx);
        return true;
    }
    
    return false;
}

void MouseDrv::SwitchProFile(int idx)
{
    mProFileIdx = idx;
    mProfile = &mProfileList[mProFileIdx];

    LoadMouseSetting(SAVE_ALL);

    ui.btnProfileSel->setText(mProfile->name);
}

int MouseDrv::toHrdIdx(int sys_key_idx)
{
    int hrd_trans[MOUSE_KEY_NUM] = {0,1,2,3,4,6,7,8,9};    
    Q_ASSERT(sys_key_idx >=0 && sys_key_idx < MOUSE_KEY_NUM);

    return hrd_trans[sys_key_idx];
}

int MouseDrv::toSysIdx(int hrd_key_idx)
{
    int sys_trans[12] = {0,1,2,3,4,-1,5,6,7,8,-1,-1};
    Q_ASSERT(hrd_key_idx >=0 && hrd_key_idx < 12);

    return sys_trans[hrd_key_idx];
}

void MouseDrv::onHidTimerOut()
{                
    int sensor_type = mSensorType;
    int mouse_id = mMouseID;    

    initHrdMouse();
    if(mSensorType != sensor_type || mMouseID != mouse_id)
    {
        if(mSensorType != SENSOR_INVAILD) //只从无效->有效,有效->有效,避免用户拔掉鼠标，数据变化，很奇怪
        {
            initMapTable();            
            LoadMouseSetting(SAVE_ALL);
            initMouseSpecial();
        }
    }
}

void MouseDrv::onDelaySave()
{
    SaveMouseSetting(mSaveFlag);
    mSaveFlag = SAVE_NONE;
}

void MouseDrv::initHrdMouse()
{        
    int b_init = false;
    if(!mHidCtrl.CheckDevice())
        mSensorType = SENSOR_INVAILD;
    else
        b_init =true;
    
    if(mSensorType == SENSOR_INVAILD && b_init)
    {
        HdiMousePara para;                
        if(mHidCtrl.GetPara(mSensorType,mMouseID,para))
        {                      
        }
        else
        {            
            mSensorType = SENSOR_INVAILD;
            mMouseID = 0x7D;            
            mHidCtrl.DeInit();
        }
    }
}

void MouseDrv::initMouseSpecial()
{
    if(mMouseID == 0x6D)
    {        ;
        mBtnKey[5]->setEnabled(false);        
    }
    else
    {
        mBtnKey[5]->setEnabled(true);
    }
}

void MouseDrv::mousePressEvent(QMouseEvent *event)
{       
    if (event->button() == Qt::LeftButton) {
        this->m_drag = true;
        this->dragPos = event->pos();               
    }
}  

void MouseDrv::mouseMoveEvent(QMouseEvent * event)
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

void MouseDrv::mouseReleaseEvent(QMouseEvent *event)
{
    m_drag = false;
    if(m_move) {
        m_move = false;        
    }    
    setCursor(Qt::ArrowCursor);
}

void MouseDrv::onBtnMin()
{
    showMinimized();
}

void MouseDrv::onBtnExit()
{
    close();
}

void MouseDrv::updateTabColor()
{    
    QString	gray_style = "QPushButton{color: lightgray;} QPushButton:hover{color: white;}";
    ui.btnSetKey->setStyleSheet(gray_style);
    ui.btnSetDpi->setStyleSheet(gray_style);
    ui.btnSetMouse->setStyleSheet(gray_style);
    ui.btnSetDefine->setStyleSheet(gray_style);

    QString	blue_style = "QPushButton{color: blue;}";
    if(ui.stackedWidget->currentIndex() == 0)
        ui.btnSetKey->setStyleSheet(blue_style);
    else if(ui.stackedWidget->currentIndex() == 1)
        ui.btnSetDpi->setStyleSheet(blue_style);
    else if(ui.stackedWidget->currentIndex() == 2)
        ui.btnSetMouse->setStyleSheet(blue_style);
    else if(ui.stackedWidget->currentIndex() == 3)
        ui.btnSetDefine->setStyleSheet(blue_style);
}

QAction *MouseDrv::showMenu(QPushButton *btn,QMenu &menu)
{
    QPoint pos = btn->mapToGlobal(QPoint(0,0));
    pos.setY(pos.y() + btn->height());

    QAction *ret = NULL;
    btn->setEnabled(false);
    ret = menu.exec(pos);
    btn->setEnabled(true);
    return ret;
}

void MouseDrv::onTabChange()
{
    if(sender() == ui.btnSetKey)
        ui.stackedWidget->setCurrentIndex(0);
    else if(sender() == ui.btnSetDpi)
        ui.stackedWidget->setCurrentIndex(1);
    else if(sender() == ui.btnSetMouse)
        ui.stackedWidget->setCurrentIndex(2);
    else if(sender() == ui.btnSetDefine)
    {
        ui.stackedWidget->setCurrentIndex(3);
        if(ui.treeDefineList->topLevelItemCount() > 0)
            ui.treeDefineList->setCurrentItem(ui.treeDefineList->topLevelItem(0),QItemSelectionModel::SelectCurrent);
    }
    
    updateTabColor();
    hideBtnSaveDefine();
}

void MouseDrv::onBtnProfileSel()
{
    QString style = mMenuStyle + "QMenu::item {text-align: center; padding:2px 32px;}";

    QStringList name_list;
    QMenu menu(this);        
    menu.setStyleSheet(style);
    for(int i = 0; i < mProfileList.size(); i++)
    {
        menu.addAction(mProfileList[i].name);
        name_list.push_back(mProfileList[i].name);
    }    
    
    QAction *action = showMenu(ui.btnProfileSel,menu);
    if(action)
    {
        int action_idx = name_list.indexOf(action->text());
        SwitchProFile(action_idx);
    }
}

void MouseDrv::onBtnProfileSet()
{
    QString style = mMenuStyle + "QMenu::item {text-align: center; padding:2px 32px;}";

    QMenu menu(this);        
    menu.setStyleSheet(style);
    //menu.addAction("编辑当前配置");
    menu.addAction("导出配置");
    menu.addAction("导入配置");    

    QAction *action = showMenu(ui.btnProfileSet,menu);
    if(action)
    {      
        QString text = action->text();
        if(text == "导出配置")
        {
            QString fileName = QFileDialog::getSaveFileName(this,"Save File","Profile.bin","*.bin");
            if(fileName.isEmpty())
		        return;	

            SaveProFile(fileName);
            MessageDlg::info(this,"导出成功");
        }
        else if(text == "导入配置")
        {
            QString fileName = QFileDialog::getOpenFileName(this,"Open File","Profile.bin","*.bin");
            if(fileName.isEmpty())
		        return;

            if(LoadProFile(fileName))
                MessageDlg::info(this,"导入成功");
        }
    }
}

void MouseDrv::onBtnSave()
{
    SaveMouseSetting(SAVE_PARA | SAVE_KEY_MAP);
}

void MouseDrv::onBtnRecover()
{
    if(MessageDlg::info(this,"是否恢复出厂设置") == QDialog::Accepted)
    {
        InitProFile();
        SaveMouseSetting(SAVE_ALL);

        MessageDlg::info(this,"恢复出厂设置成功");
    }
}

void MouseDrv::onBtnKey()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());    
    QString style = mMenuStyle + "QMenu::item {text-align: center; padding:2px 40px;}";    
    int key_idx = mBtnKey.indexOf(btn);

    QMenu menu(this);        
    menu.setStyleSheet(style);    

    QMenu *dpi_menu = new QMenu("DPI",&menu);            
    dpi_menu->addAction("DPI +");
    dpi_menu->addAction("DPI -");    
    dpi_menu->addAction("DPI loop");    

    QMenu *mouse_menu = new QMenu("鼠标按键",&menu);            
    mouse_menu->addAction(tr("左键"));
    mouse_menu->addAction(tr("右键"));
    mouse_menu->addAction(tr("中键"));
    mouse_menu->addAction(tr("前进"));
    mouse_menu->addAction(tr("后退"));
    if(key_idx == 7 || key_idx == 8)
    {
        mouse_menu->addAction(tr("向上滚"));
        mouse_menu->addAction(tr("向下滚"));
    }
    menu.addMenu(mouse_menu);
    
    QMenu *media_menu = new QMenu("多媒体功能",&menu);  
    QMap<QString,Stru_MouseMap>::iterator media_it = mMediaMap.begin();
    while(media_it != mMediaMap.end())
    {
        media_menu->addAction(media_it.key());
        media_it++;
    }

    menu.addAction(tr("组合键"));
    menu.addAction(tr("灯光 开关/关闭"));
    menu.addAction(tr("宏定义"));    
    menu.addMenu(dpi_menu);
    menu.addMenu(media_menu);
    menu.addAction(tr("禁用"));            
    
    int hrd_idx = toHrdIdx(key_idx);
    QAction *action = showMenu(btn,menu);
    if(action)
    {
        QString text = action->text();        

        //必须有左键，检测
        bool has_rkey = false;
        for(int i = 0; i < mBtnKey.size(); i++)
        {
            if(i != key_idx && mBtnKey[i]->text() == "左键")
            {
                has_rkey = true;
                break;
            }
            else if(i == key_idx && text == "左键")
            {
                has_rkey = true;
                break;
            }
        }
        if(!has_rkey)
        {
            MessageDlg::info(this,tr("至少要有一个左键"));
            return;
        }

        //开始设置
        unsigned char mode  = 0;
        unsigned char para0 = 0;
        unsigned char para1 = 0;
        unsigned char para2 = 0;
        //mouse
        if(mouseMap.contains(text)) 
        {
            const Stru_MouseMap &map_info = mouseMap[text];
            
            mode = map_info.mode;            
            para0 = map_info.para[0];
            para1 = map_info.para[1];
            para2 = map_info.para[2];            
        }
        else if(mMediaMap.contains(text)) 
        {
            const Stru_MouseMap &map_info = mMediaMap[text];

            mode = map_info.mode;            
            para0 = map_info.para[0];
            para1 = map_info.para[1];
            para2 = map_info.para[2]; 
        }
        else
        {
            if(text == "组合键")
            {
                ComboKeyDlg dlg;
                if(dlg.exec() != QDialog::Accepted || dlg.key_list.size() == 0)
                    return;
                
                mode  = MODE_KEY;
                text  = dlg.getText();
                para1 = dlg.key_list[0];
                para2 = dlg.key_list.size() > 1? dlg.key_list[1]:0;

            }
            else if(text == "宏定义")
            {
                DefineDlg dlg;
                dlg.setDefine(mDefineRoot);
                if(dlg.exec() != QDialog::Accepted)
                    return;
                
                DefinePtr selDefine = dlg.getSel();
                text = "宏 - " + selDefine->name;
                mProfile->defineGuid[key_idx] = selDefine->guid;

                mode  = MODE_DEFINE;
            }
            else
                Q_ASSERT(0);                              
        }
    
        mProfile->map.keyMap[hrd_idx][0] = mode;
        mProfile->map.keyMap[hrd_idx][1] = para0;
        mProfile->map.keyMap[hrd_idx][2] = para1;
        mProfile->map.keyMap[hrd_idx][3] = para2;

        btn->setText(text);
        SaveMouseSetting(SAVE_KEY_MAP);
    }
}

void MouseDrv::onDPIColorChanged()
{
    ColorButton *btn = qobject_cast<ColorButton*>(sender());
    int i = mDpiColor.indexOf(btn);

    QColor color = mDpiColor[i]->getColor();
    mProfile->para.CPIColor[i][0] = color.red();
    mProfile->para.CPIColor[i][1] = color.green();
    mProfile->para.CPIColor[i][2] = color.blue(); 
    SaveMouseSetting(SAVE_PARA);
}

void MouseDrv::onDPIChkBox()
{
    QCheckBox *box = qobject_cast<QCheckBox*>(sender());
    int i = mDpiChkBox.indexOf(box);    
    updateDPIPara();
}

void MouseDrv::updateDPILabel()
{
    QSlider *slider = qobject_cast<QSlider*>(sender());
    int i = mDpiSlider.indexOf(slider);                         

    mProfile->dpiLevel[i] = mDpiSlider[i]->value();
    mDpiLabel[i]->setText(QString::number(DPI_TABLE[mProfile->dpiLevel[i]])); 

    if(!slider->isSliderDown())
        updateDPIPara();
}

void MouseDrv::updateDPIPara()
{            
    for(int i = 0; i < 6; i++)
    {
        if(mDpiChkBox[i]->isChecked())         
            mProfile->para.CPI[i] = mDpiSlider[i]->value();        
        else                    
            mProfile->para.CPI[i] = 0x80;            
    }
    SaveMouseSettingDelay(SAVE_PARA);
}

void MouseDrv::updateSpeedLabel()
{
    ui.labelLedSpeed->setText(QString::number(ui.sliderLedSpeed->value()));    
    if(!ui.sliderLedSpeed->isSliderDown())
        updateSpeedPara();     
}

void MouseDrv::updateSpeedPara()
{
    mProfile->para.LedSpeed = ui.sliderLedSpeed->value();
    SaveMouseSettingDelay(SAVE_PARA);
}

void MouseDrv::onBtnLed()
{  
    QString style = mMenuStyle + "QMenu::item {text-align: center; padding:2px 32px;}";

    QMenu menu(this);        
    menu.setStyleSheet(style);
    menu.addAction("常亮模式");
    menu.addAction("呼吸模式");
    menu.addAction("光谱模式");
    menu.addAction("APM 模式");

    QAction *action = showMenu(ui.btnLed,menu);
    if(action)
    {
        ui.btnLed->setText(action->text());   
        mProfile->para.LedMode = ledMap[ui.btnLed->text()];
        SaveMouseSetting(SAVE_PARA);
    }
}

void MouseDrv::onBtnFreq()
{
    QString style = mMenuStyle + "QMenu::item {text-align: center; padding:2px 32px;}";

    QMenu menu(this);        
    menu.setStyleSheet(style);
    menu.addAction("1000Hz");
    menu.addAction("500Hz");
    menu.addAction("250Hz");
    menu.addAction("125Hz");

    QAction *action = showMenu(ui.btnFreq,menu);
    if(action)
    {
        QString text = action->text();
        ui.btnFreq->setText(text);  
        mProfile->reportGap = mReportGapMap[text];
        SaveMouseSetting(SAVE_REPORT_GAP);
    }
}

void MouseDrv::onBtnStartDefine()
{
    QString	nr_style = "#btnStartDefine{background:url(:/MouseDrv/skins/recbtn_nr.png);}";
    QString	dn_style = "#btnStartDefine{background:url(:/MouseDrv/skins/recbtn_dn.png);}";

    if(mRecord)
    {                
        QApplication::instance()->removeEventFilter(this);

        ui.btnStartDefine->setStyleSheet(nr_style);
        ui.btnStartDefine->setText(tr("开始录制宏"));
        showBtnSaveDefine();

        mRecordDefine.StopRecord();        
        mRecord = false;        
        
        updateTreeKeyList(mRecordDefine);
    }
    else
    {    
        if(!getTreeDefine())
        {
            MessageDlg::info(this,"请先选择一个宏");
            return;
        }

        bool auto_delay = ui.checkBoxAutoDelay->isChecked();        

        mRecord = true;
        ui.treeKeyList->clear();
        
        hideBtnSaveDefine();        

        mRecordDefine.Clear(); 
        mRecordDefine.StartRecord(auto_delay);        

        ui.btnStartDefine->setStyleSheet(dn_style);
        ui.btnStartDefine->setText(tr("停止录制宏"));

        QApplication::instance()->installEventFilter(this);
    }    
}

DefinePtr MouseDrv::getTreeDefine()
{
    QTreeWidgetItem* tree_item = ui.treeDefineList->currentItem();
    DefinePtr define_item;
    if(tree_item)
        define_item = tree_item->data(0,Qt::UserRole).value<DefinePtr>();
    
    if(!tree_item || define_item->type == 1)
        return DefinePtr();

    return define_item;
}

int MouseDrv::getDefineType()
{    
    for(int i = 0; i < mRadioDefine.size(); i++)
    {
        if(mRadioDefine[i]->isChecked())
            return i;
    }

    return -1;
}

void MouseDrv::showBtnSaveDefine()
{
    curDefine = getTreeDefine();
    ui.btnSaveDefine->setVisible(true);    
}

void MouseDrv::hideBtnSaveDefine()
{    
    mRecordDefine.Clear();
    ui.btnSaveDefine->setVisible(false);
}

void MouseDrv::onBtnSaveDefine()
{
    int type = getDefineType();    

    if(mRecordDefine.keyList.size() > 0)
        curDefine->define = mRecordDefine;

    curDefine->defineType = type;
    if(type == 2)
        curDefine->define.repeat = ui.lineDefineRelay->text().toInt();

    hideBtnSaveDefine();    
}

void MouseDrv::onBtnDefineType()
{
    DefinePtr ptr = getTreeDefine();
    if(ptr)
    {
        int cur_type = getDefineType();
        if(ptr->defineType != cur_type)
            showBtnSaveDefine();

        ui.lineDefineRelay->setReadOnly(cur_type != 2);
        ui.lineDefineRelay->setText(QString::number(ptr->define.repeat));
    }
}

DefinePtr MouseDrv::FindDefine(DefinePtr ptr,QString name)
{
    if(ptr->name == name)
        return ptr;

    if(ptr->type == 1)
    {        
        for(int i = 0; i < ptr->dir.size();i++)
        {
            if(ptr->dir[i]->name == name)
                return ptr->dir[i];
        }        
    }

    return DefinePtr();
}

DefinePtr MouseDrv::FindDefineByGuid(DefinePtr ptr,QString guid)
{
    if(ptr->guid == guid)
        return ptr;

    if(ptr->type == 1)
    {        
        for(int i = 0; i < ptr->dir.size();i++)
        {
            DefinePtr find_ptr = FindDefineByGuid(ptr->dir[i],guid);
            if(find_ptr)
                return find_ptr;
        }        
    }
    return DefinePtr();
}

void MouseDrv::treeDefineContextMenu(const QPoint &pos)
{
    QString style = mMenuStyle + "QMenu::item {text-align: center; padding:2px 32px;}";
    
    QTreeWidgetItem *root = ui.treeDefineList->invisibleRootItem();
    QTreeWidgetItem *item = ui.treeDefineList->itemAt(pos);    

    DefinePtr parentNode;
    QTreeWidgetItem *parent_item = NULL;
    if(!item)
    {
        item = root;    
        parentNode = mDefineRoot;        
    }
    else
    {
        parent_item = item->parent();    
        if(!parent_item)
            parent_item = root;

        parentNode = parent_item->data(0,Qt::UserRole).value<DefinePtr>();            
    }
    DefinePtr defineNode = item->data(0,Qt::UserRole).value<DefinePtr>();            

    QMenu menu(this);        
    menu.setStyleSheet(style);

    QAction *new_file = new QAction(tr("新建宏"),&menu);
    QAction *new_dir = new QAction(tr("新建宏目录"),&menu);
    QAction *rename = new QAction(tr("重命名"),&menu);
    QAction *delete_item = new QAction(tr("删除"),&menu);        
    
    if(defineNode->type == 1)
    {
        menu.addAction(new_file);
        menu.addAction(new_dir);
    }
    if(item != root)
    {
        menu.addAction(rename);    
        menu.addAction(delete_item);
    }
        
    QAction *action = menu.exec(ui.treeDefineList->mapToGlobal(pos));
    if(action)
    {        
        if(action == new_file || action == new_dir || action == rename)
        {         
            bool ok;
            QString name = InputDlg::getInput(this,tr("输入名称"),QString(),&ok);
            if(name.isEmpty() || !ok)
                return;

            DefinePtr findNode = (action == rename)? parentNode : defineNode;
            if(FindDefine(findNode,name))
            {
                MessageDlg::info(this,"此名称已存在");
                return;
            }

            if(action == new_file || action == new_dir)
            {
                DefinePtr new_node(new Stru_DefineNode());            
                new_node->type = (action == new_dir);
                new_node->name = name;

                defineNode->dir.push_back(new_node);
                addTreeDefine(item,new_node);

                ui.treeDefineList->expandItem(item);
            }
            else
            {            
                defineNode->name = name;            
                item->setText(0,defineNode->name);
            }
        }
        else if(action == delete_item)
        {            
            parentNode->dir.removeAll(defineNode);                       
            parent_item->removeChild(item);
        }        
    }    
}

void MouseDrv::updateTreeKeyList(HdiMouseDefine mouseDefine)
{
    ui.treeKeyList->clear();
    
    for(int i = 0; i < mouseDefine.keyList.size(); i++)
    {
        HdiMouseDefine::keyComp comp = mouseDefine.keyList[i];				
        int size = mouseDefine.keyList.size();        

        QString str;                    
        if(comp.type == 0)
            str = QString("按下 '%0'").arg(HidToText(comp.keyHid));
        else
            str = QString("抬起 '%0'").arg(HidToText(comp.keyHid));

        ui.treeKeyList->addItem(str);
        if(comp.time > 0 && i != mouseDefine.keyList.size() - 1)
        {
            str = QString("延迟 %0ms").arg(comp.time);					        
            ui.treeKeyList->addItem(str);
        }            
    }
}

void MouseDrv::treeKeyUpdate()
{            
    ui.treeKeyList->clear();

    DefinePtr define_item = getTreeDefine();        
    if(!define_item)
        return;
    
    updateTreeKeyList(define_item->define);   
    mRadioDefine[define_item->defineType]->setChecked(true);    
    hideBtnSaveDefine();  
    
}

void MouseDrv::onSetDefineDelay(const QPoint &pos)
{    
    if(ui.btnSaveDefine->isVisible() || mRecord)
        return;

    bool show_menu = !pos.isNull();
    QString style = mMenuStyle + "QMenu::item {text-align: center; padding:2px 32px;}";
    
    DefinePtr define_item = getTreeDefine();
    int row = ui.treeKeyList->currentRow();        
    if(row < 0 || !define_item || row == ui.treeKeyList->count() - 1)
        return;

    //计算在defien 中的位置
    int define_idx = 0;
    int list_idx = 0;
    HdiMouseDefine &mouseDefine = define_item->define;   
    HdiMouseDefine::keyComp comp;
    bool b_time = false;
    for(int i = 0; i < mouseDefine.keyList.size(); i++)
    {
        comp = mouseDefine.keyList[i];				
        int size = mouseDefine.keyList.size();        
        
        if(row == list_idx)
        {
            define_idx = i;
            break;
        }    
        list_idx++;
        if(comp.time > 0 && i != mouseDefine.keyList.size() - 1)
        {            
            if(row == list_idx)
            {
                define_idx = i;
                b_time = true;
                break;
            }
            list_idx++;
        }            
    }               
    comp = mouseDefine.keyList[define_idx];
    if(!b_time && comp.time > 0)
        return;

    QMenu menu(this);        
    menu.setStyleSheet(style);
    if(comp.time > 0 || b_time)
        menu.addAction(tr("修改延迟"));
    else
        menu.addAction(tr("插入延迟"));        
    
    QAction *run = NULL;
    if(show_menu)
        run = menu.exec(ui.treeKeyList->mapToGlobal(pos) + QPoint(0,4));

    if(!show_menu || run)
    {
        bool ok = false;
        int delay_time = InputDlg::getNum(this,tr("输入延迟毫秒(ms)"),comp.time,0,60000,&ok);
        if(ok)
        {
            if(delay_time != comp.time)
            {
                mouseDefine.keyList[define_idx].time = delay_time;                
                treeKeyUpdate();               
            }
        }
    }
}

void MouseDrv::treeKeyContextMenu(const QPoint &pos)
{    
    onSetDefineDelay(pos);
}

void MouseDrv::treeKeyDoubleClicked(QListWidgetItem *item)
{
    onSetDefineDelay(QPoint());    
}

bool MouseDrv::eventFilter(QObject *obj, QEvent *event)
{    
    bool record_key = ui.checkBoxRecordKey->isChecked();
    bool record_mouse = ui.checkBoxRecordMouse->isChecked();
    if(mRecord)
    {
        int event_type = event->type();
        bool key_event = (event_type == QEvent::KeyPress || event_type == QEvent::KeyRelease);
        bool mouse_event = (event_type == QEvent::MouseButtonPress || event_type == QEvent::MouseButtonRelease);
        if(key_event || mouse_event)
        {
            if(ui.btnStartDefine != obj)
            {
                unsigned char hid = 0;
                int type = -1;
                QString key_name;
                if(key_event && record_key)
                {
                    QKeyEvent *key = dynamic_cast<QKeyEvent*>(event);
                    if(!key->isAutoRepeat())
                    {
                        hid = KeyToHid(key->nativeVirtualKey());
                        type = event_type == QEvent::KeyPress? 0:1;       
                        key_name = HidToText(hid);
                    }
                }
                if(mouse_event && record_mouse)
                {
                    QMouseEvent *mouse = dynamic_cast<QMouseEvent*>(event);                    
                    key_name = mouseButtonMap.key(mouse->button());
                    hid = TextToHid(key_name.toStdString().c_str());
                    type = event_type == QEvent::MouseButtonPress? 0:1;                    
                }

                HdiMouseDefine &mouseDefine = mRecordDefine;
                if(type != -1 && mouseDefine.AddKey(hid,type))
                {                                        
			        HdiMouseDefine::keyComp comp = mouseDefine.keyList.back();				
			        int size = mouseDefine.keyList.size();

                    QString str;                    
			        if(size > 1 && mouseDefine.keyList[size - 2].time > 0)
			        {
                        str = QString("延迟 %0ms").arg(mouseDefine.keyList[size - 2].time);					        
                        ui.treeKeyList->addItem(str);
			        }

			        if(comp.type == 0)
				        str = QString("按下 '%0'").arg(key_name);
			        else
				        str = QString("抬起 '%0'").arg(key_name);

                    ui.treeKeyList->addItem(str);
                    ui.treeKeyList->scrollToBottom();
                }

                if(!mouseDefine.CheckSpace()) //空间不够了
                    onBtnStartDefine();

                return true;
            }
            else
                return QMainWindow::eventFilter(obj,event);
        }
        else
            return QMainWindow::eventFilter(obj,event);
    }   
    else
        return QMainWindow::eventFilter(obj,event);
}

bool MouseDrv::SaveMouseSetting(int type)
{    
    if(!m_bInit)
        return false;

    if(mSensorType == SENSOR_INVAILD)	        
        return false;

    if(type & SAVE_PARA)    
        SetMousePara();    

    if(type & SAVE_KEY_MAP)
        SetMouseMap();    

    mHidCtrl.SetFinish();    
    if(type & SAVE_REPORT_GAP)
        SetMouseGap();
    
    return true;
}

bool MouseDrv::SaveMouseSettingDelay(int type)
{
    if(!m_bInit)
        return false;

    if(mSaveFlag == SAVE_NONE)
        QTimer::singleShot(0,this,SLOT(onDelaySave()));

    mSaveFlag |= type;   
    return true;
}

void MouseDrv::LoadMouseSetting(int type)
{
    if(type & SAVE_PARA)    
        GetMousePara();

    if(type & SAVE_REPORT_GAP)
        GetMouseGap();

    if(type & SAVE_KEY_MAP)
        GetMouseMap(); 
}

void MouseDrv::GetMousePara()
{    
    HdiMousePara para = mProfile->para;
    
    mProfile->para = para;	
	for(int i = 0; i < 6; i++)
	{
        if(mProfile->dpiLevel[i] > DPI_TABLE.size() - 1)
            mProfile->dpiLevel[i] = DPI_TABLE.size() - 1; 

        mDpiChkBox[i]->setChecked(para.CPI[i] != 0x80);
        mDpiSlider[i]->setRange(0,DPI_TABLE.size() - 1);
        mDpiSlider[i]->setValue(mProfile->dpiLevel[i]);		    
        
        mDpiColor[i]->setColor(QColor(para.CPIColor[i][0],para.CPIColor[i][1],para.CPIColor[i][2]));
        mDpiLabel[i]->setText(QString::number(DPI_TABLE[mDpiSlider[i]->value()]));
    }    

    ui.btnLed->setText(ledMap.key(para.LedMode));		
    ui.sliderLedSpeed->setValue(para.LedSpeed);
}

void MouseDrv::SetMousePara()
{            
    mHidCtrl.SetPara(mProfile->para);    
}

void MouseDrv::GetMouseMap() //得到按键映射
{	
	HdiMouseMap para = mProfile->map;	    
	
    mProfile->map = para;
    //根据硬件鼠标映射关系,定义转换表	
	for(int i = 0; i < 12; i++)
	{
		int key_idx = toSysIdx(i);
		if(key_idx < 0)
			continue;
				        
        Stru_MouseMap map_info(para.keyMap[i][0],para.keyMap[i][1],para.keyMap[i][2],para.keyMap[i][3]);
    
        QString key_mouse = mouseMap.key(map_info);
        QString key_media = mMediaMap.key(map_info);

        if(!key_mouse.isEmpty())        
            mBtnKey[key_idx]->setText(key_mouse);
        else if(!key_media.isEmpty())        
            mBtnKey[key_idx]->setText(key_media);            
        else
        {                    
            if(map_info.mode == MODE_DEFINE)            
            {
                QString guid = mProfile->defineGuid[key_idx];
                DefinePtr ptr = FindDefineByGuid(mDefineRoot,guid);                
                mBtnKey[key_idx]->setText("宏 - " + ptr->name);
            }
            else if(map_info.mode == MODE_HIT_KEY)
                mBtnKey[key_idx]->setText("连击键");  
            else if(map_info.mode == MODE_KEY)
                mBtnKey[key_idx]->setText(ComboKeyDlg::makeStr(map_info.para[0],map_info.para[1]));  
            else
                mBtnKey[key_idx]->setText("InValid");    
        }
	}
}

void MouseDrv::SetMouseMap() 
{
    int seq = 1;
    int cur_seq = 1;
    int hrd_idx;
    int hrd_define_type[3] = {2,1,0};
    QMap<QString,int> defineMap;
    for(int i = 0; i < mProfile->defineGuid.size(); i++)
    {        
        QString guid = mProfile->defineGuid[i];
        if(!guid.isEmpty())
        {
            DefinePtr ptr = FindDefineByGuid(mDefineRoot,guid);
            if(defineMap.contains(guid))
            {
                cur_seq = defineMap[guid];
            }
            else
            {                            
                mHidCtrl.SetDefine(seq,ptr->define);
                cur_seq = seq;
                seq++;
            }

            hrd_idx = toHrdIdx(i);
            mProfile->map.keyMap[hrd_idx][1] = hrd_define_type[ptr->defineType];
            mProfile->map.keyMap[hrd_idx][2] = cur_seq;
            mProfile->map.keyMap[hrd_idx][3] = 0xFF;            
        }
    }

    mHidCtrl.SetMap(mProfile->map);
}

void MouseDrv::SetMouseGap()
{    
    QString text = ui.btnFreq->text();
    mHidCtrl.SetLedReportGap(mReportGapMap[text]);        
}

void MouseDrv::GetMouseGap()
{
    int var = mProfile->reportGap;
    ui.btnFreq->setText(mReportGapMap.key(var));    
}