#ifndef MOUSEDRV_H
#define MOUSEDRV_H

#include <QtGui/QMainWindow>
#include <QSharedPointer>
#include <QTimer>
#include "ui_mousedrv.h"
#include "HidCtrl.h"


struct Stru_MouseMap
{
    Stru_MouseMap();
    Stru_MouseMap(int mode,uchar var0,uchar var1,uchar var2);
    bool operator ==(const Stru_MouseMap &other) const;
	
	unsigned char mode;	
	QVector<unsigned char> para;
};

struct Stru_DefineNode;
typedef QSharedPointer<Stru_DefineNode> DefinePtr;
struct Stru_DefineNode
{        
    Stru_DefineNode();

    int type;  //0 file,1 dir
    QString name;    
    HdiMouseDefine define;    
    int defineType; //00 指定循环次数 01 直到有任一键按下 02 直到按键松开
    QList<DefinePtr> dir;
    QString guid;
};
Q_DECLARE_METATYPE(DefinePtr)
QDataStream& operator >>(QDataStream &stream,DefinePtr &profile);
QDataStream& operator <<(QDataStream &stream,const DefinePtr &profile);

void updateTreeDefine(QTreeWidgetItem *item,DefinePtr defineNode); //用defineNode，更新item
void addTreeDefine(QTreeWidgetItem *item,DefinePtr defineNode);    //将defineNode，添加到item

//Stru_MouseProfile
struct Stru_MouseProfile
{
    Stru_MouseProfile();

    QString name;
    HdiMousePara para;
    HdiMouseMap map;
    unsigned char reportGap;    
    QVector<QString> defineGuid;
    QVector<int> dpiLevel;
};
QDataStream& operator >>(QDataStream &stream,Stru_MouseProfile &profile);
QDataStream& operator <<(QDataStream &stream,const Stru_MouseProfile &profile);

class MouseDrv : public QMainWindow
{
    Q_OBJECT

public:
    MouseDrv(QWidget *parent = 0, Qt::WFlags flags = 0);
    ~MouseDrv();        

protected slots:
    void onTabChange();  

    void onBtnMin();
    void onBtnExit();

    void onBtnProfileSel();
    void onBtnProfileSet();
    void onBtnSave();
    void onBtnRecover();

    void onBtnKey();    
        
    void onDPIChkBox();
    void onDPIColorChanged();
    void updateDPILabel(); 
    void updateDPIPara();  
    
    void onBtnLed();
    void onBtnFreq();         
    void updateSpeedLabel();
    void updateSpeedPara();

    void onBtnStartDefine();
    void onBtnSaveDefine();    
    void onBtnDefineType();
    void treeKeyUpdate();
    void treeDefineContextMenu(const QPoint &pos);
    void treeKeyContextMenu(const QPoint &pos);    
    void treeKeyDoubleClicked(QListWidgetItem *item);

    void onHidTimerOut();
    void onDelaySave();

protected:
    enum{
        SAVE_NONE       = 0,
        SAVE_PARA       = 1,
        SAVE_KEY_MAP    = 2,
        SAVE_REPORT_GAP = 4,
        SAVE_ALL        = SAVE_PARA | SAVE_KEY_MAP | SAVE_REPORT_GAP,
    };

    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

    virtual bool eventFilter(QObject *obj, QEvent *event);

    void initHrdMouse();   
    void initMouseSpecial();
    void initDpiPage();        
    void updateTabColor();
    QAction *showMenu(QPushButton *btn,QMenu &menu);    

    int toHrdIdx(int sys_key_idx);
    int toSysIdx(int hdr_key_idx);    

    void InitProFile();
    void SaveProFile(QString file_name);    
    bool LoadProFile(QString file_name);
    void SwitchProFile(int idx);
    void LoadDefine();
    void SaveDefine();

    bool SaveMouseSetting(int type);
    bool SaveMouseSettingDelay(int type);
    void LoadMouseSetting(int type);    

    void SetMousePara();
    void GetMousePara();
    void initMapTable();
    void SetMouseMap();
    void GetMouseMap(); 
    void SetMouseGap();
    void GetMouseGap();

    DefinePtr FindDefine(DefinePtr ptr,QString name);
    DefinePtr FindDefineByGuid(DefinePtr ptr,QString guid);
    DefinePtr getTreeDefine(); //得到当前选中的宏定义，dir返回空
    void updateTreeKeyList(HdiMouseDefine define);
    int getDefineType();
    void showBtnSaveDefine();
    void hideBtnSaveDefine();

    void onSetDefineDelay(const QPoint &pos);

private:
    Ui::MouseDrvClass ui;
    
    QString mMenuStyle;

    QVector<QPushButton*> mBtnKey;   

    QVector<QSlider*> mDpiSlider;
    QVector<QLabel*> mDpiLabel;
    QVector<QCheckBox*> mDpiChkBox;
    QVector<ColorButton*> mDpiColor;     
    
    QVector<QRadioButton*> mRadioDefine;

    //define
    bool mRecord;    
    HdiMouseDefine mRecordDefine;    
    DefinePtr curDefine;

    //move window
    QPoint windowPos,mousePos,dPos;
    bool m_drag, m_move;
    QPoint dragPos;                  

    //hid ctrl
    QVector<int> DPI_TABLE;
    QMap<QString,Stru_MouseMap> mouseMap;
    QMap<QString,unsigned char> ledMap;      
    QMap<QString,unsigned char> mReportGapMap;
    QMap<QString,int> mouseButtonMap;    
    QMap<QString,Stru_MouseMap> mMediaMap;

    Stru_MouseProfile *mProfile;
    QVector<Stru_MouseProfile> mProfileList;
    DefinePtr mDefineRoot;
    int mProFileIdx;

    QString mProfilePath;
    CHidCtrl mHidCtrl;    
    int mSensorType;
    int mMouseID;
    int mSaveFlag;
    bool m_bInit;    

    QTimer mHidTimer;
};

#endif // MOUSEDRV_H
