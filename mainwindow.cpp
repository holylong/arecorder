#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "audio_rec_thread.h"
#include "newgroupbox.h"
#include "ffmpeg_util.h"
#include "config.h"

#include <QPushButton>
#include <QLabel>
#include <QTime>
#include <QGridLayout>
#include <QDebug>
#include <QListWidget>
#include <QGroupBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QToolBar>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>

extern "C"{
    #include <libavdevice/avdevice.h>
}

#ifdef _WIN32
#pragma comment(lib, "Strmiids.lib")
#endif

/// list all devices   windows command
/// ffmpeg -list_devices true -f dshow -i dummy
/// ffmpeg -list_options true -f dshow -i video="Integrated Camera"
/// ffmpeg -list_options true -f dshow -i audio="audiome (Realtek(R) Audio)"
/// ffmpeg -f dshow -i video="Integrated Camera" -f dshow -i audio="audiome (Realtek(R) Audio)" -vcodec libx264 -acodec aac -strict -2 mycamera.mkv
/// ffmpeg -ar 44100 -ac 2 -f s16le -i 44100_s16le_2.pcm -ar 48000 -ac 1 -f f32le 48000_f32le_1.pcm

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    VbConfig::Instance()->LoadConfig();
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlags(Qt::FramelessWindowHint);
    setFixedSize(400, 200);
    ui->setupUi(this);

    _font.setBold(true);
//    _font.setPixelSize(12);
//    avcodec_register_all();
    avdevice_register_all();

    QVector<QPair<QString, QString>> arrDevs = FFmpegUtil::GetDeviceList();
    for (auto devpair : arrDevs) {
        _devs.push_back(devpair.second);
    }

    InitView();
    InitTrayIcon();

    _recThread = new AudioRecorderThread(this);
    InitSigSlot();
}

void MainWindow::InitSigSlot()
{
    QObject::connect(_btnSwitch, &QPushButton::clicked, [=]{
        if(_recThread->IsRecorderRunning()){
            _recThread->stop();
            _btnSwitch->setText(tr("Switch_Start"));
            _recThread->requestInterruption();
        }else{
            _recThread->start();
            _btnSwitch->setText(tr("Switch_Stop"));
        }
    });

    QObject::connect(_recThread, &AudioRecorderThread::TimeChangeSignal, [=](qulonglong t){
        QTime time(0, 0, 0, 0);
        _labelTime->setText(time.addMSecs(t).toString("mm:ss:zz").left(5));
        UpdateTrayIcon();
    });

    QObject::connect(_recThread, SIGNAL(ReportMsgSignal(QString)), this, SLOT(RecvMsgSlot(QString)));

    QObject::connect(_recThread, &AudioRecorderThread::finished, [=](){
        _trayIcon->setIcon(QIcon(":/res/recorder.png"));
    });
}

void MainWindow::RecvMsgSlot(const QString &s)
{
    _btnSwitch->setText(tr("Switch_Start"));
    _recThread->stop();
    QMessageBox::warning(this, tr("TopTitle"), s, QMessageBox::Ok);
}

void MainWindow::InitView()
{
    /// title
    NewGroupBox *transportGroup = new NewGroupBox();
    transportGroup->setTitle(tr("TopTitle"));
    transportGroup->setFont(_font);

    /// wav or pcm
    QVBoxLayout *transportLayout = new QVBoxLayout(transportGroup);
    transportLayout->setContentsMargins(20,20,20,20);

    QHBoxLayout *ckLayout = new QHBoxLayout();
    QCheckBox   *ckBoxWav = new QCheckBox(transportGroup);
    ckBoxWav->setText(tr("DataTypeWav"));
    ckBoxWav->setFont(_font);
    ckBoxWav->setFixedWidth(100);
    if(VbConfig::Instance()->_saveWav != 0)ckBoxWav->setChecked(true);
    QObject::connect(ckBoxWav, &QCheckBox::stateChanged, [=](int state){
        qDebug() << "wav state:" << state;
        VbConfig::Instance()->UpdateSaveWavType(state);
    });


    QCheckBox   *ckBoxPcm = new QCheckBox(transportGroup);
    ckBoxPcm->setText(tr("DataTypePcm"));
    ckBoxPcm->setFont(_font);
    ckBoxPcm->setFixedWidth(100);
    if(VbConfig::Instance()->_savePcm != 0)ckBoxPcm->setChecked(true);
    QObject::connect(ckBoxPcm, &QCheckBox::stateChanged, [=](int state){
        qDebug() << "pcm state:" << state;
        VbConfig::Instance()->UpdateSavePcmType(state);
    });

    ckLayout->addWidget(ckBoxPcm, 0, Qt::AlignLeft);
    ckLayout->addWidget(ckBoxWav, 1, Qt::AlignLeft);

    transportLayout->addLayout(ckLayout);

    /// device
    QHBoxLayout *deviceLayout = new QHBoxLayout();
    QLabel      *cbBoxDevTitle = new QLabel(transportGroup);
    cbBoxDevTitle->setText(tr("DeviceList"));
    cbBoxDevTitle->setFont(_font);
    QComboBox *cbBoxDev = new QComboBox(transportGroup);
    _listDev = new QListWidget(cbBoxDev);
    cbBoxDev->setModel(_listDev->model());
    cbBoxDev->setView(_listDev);
    cbBoxDev->setFont(_font);
    cbBoxDev->setFixedWidth(240);
    deviceLayout->addWidget(cbBoxDevTitle, 0, Qt::AlignLeft);
    deviceLayout->addWidget(cbBoxDev, 1, Qt::AlignLeft);

    for(auto dv:_devs)
        _listDev->addItem(new QListWidgetItem(dv));
    cbBoxDev->setCurrentText(VbConfig::Instance()->_selectDev);
    connect(cbBoxDev, QOverload<const QString &>::of(&QComboBox::currentIndexChanged),
        [=](const QString &text){
        qDebug() << "text:" << text;
        VbConfig::Instance()->UpdateSelectDev(text);
    });
    transportLayout->addLayout(deviceLayout, Qt::AlignLeft);

    /// samplerate
    QHBoxLayout *sampleRateLayout = new QHBoxLayout();
    QLabel      *cbBoxRateTitle = new QLabel(transportGroup);
    cbBoxRateTitle->setText(tr("SampleRate"));
    cbBoxRateTitle->setFont(_font);
    QComboBox *cbBoxRate = new QComboBox(transportGroup);
    QListWidget *listRate = new QListWidget(cbBoxRate);
    cbBoxRate->setModel(listRate->model());
    cbBoxRate->setView(listRate);
    cbBoxRate->setFont(_font);
    listRate->addItem(new QListWidgetItem(tr("Rate_16000")));
    listRate->addItem(new QListWidgetItem(tr("Rate_44100")));
    listRate->addItem(new QListWidgetItem(tr("Rate_48000")));
    cbBoxRate->setFixedWidth(240);
    sampleRateLayout->addWidget(cbBoxRateTitle, 0, Qt::AlignLeft);
    sampleRateLayout->addWidget(cbBoxRate, 1, Qt::AlignLeft);
    cbBoxRate->setCurrentText(VbConfig::Instance()->_sampleRate);
    connect(cbBoxRate, QOverload<const QString &>::of(&QComboBox::currentIndexChanged),
        [=](const QString &text){
        qDebug() << "text:" << text;
        VbConfig::Instance()->UpdateSampleRate(text);
    });
    transportLayout->addLayout(sampleRateLayout, Qt::AlignLeft);

    /// output dir
    QHBoxLayout *outputLayout = new QHBoxLayout();
    QLabel      *outputTitle = new QLabel(transportGroup);
    outputTitle->setText(tr("Output_dir"));
    outputTitle->setFont(_font);
    QLineEdit   *outputEdit = new QLineEdit(transportGroup);
    outputEdit->setText(VbConfig::Instance()->_saveDir);
    outputEdit->setFixedWidth(220);
    QPushButton *btnSelectDir = new QPushButton(transportGroup);
    btnSelectDir->setIcon(QIcon(":/res/openfolder.png"));
    btnSelectDir->setIconSize(QSize(20,20));
    QObject::connect(btnSelectDir, &QPushButton::clicked, [=]{
        QString selectedDir = QFileDialog::getExistingDirectory(this,
                             tr("Directory"), VbConfig::Instance()->_saveDir, QFileDialog::ShowDirsOnly);
        if (!selectedDir.isEmpty())
        {
            selectedDir = selectedDir.replace(QRegExp("\\"), "/");
            outputEdit->setText(selectedDir);
            VbConfig::Instance()->UpdateSaveDir(selectedDir);
        }
    });

    outputLayout->addWidget(outputTitle, 0, Qt::AlignRight);
    outputLayout->addWidget(outputEdit);
    outputLayout->addWidget(btnSelectDir);

    transportLayout->addLayout(outputLayout, Qt::AlignRight);


    /// control
    QHBoxLayout *controlLayout = new QHBoxLayout();
    _btnSwitch = new QPushButton();
    _btnSwitch->setText(tr("Switch_Start"));
    _btnSwitch->setFixedHeight(20);
    _btnSwitch->setFixedWidth(70);
    _btnSwitch->setFont(_font);
    _btnSwitch->setStyleSheet("QPushButton{background-color:rgb(76,76,76)}"
                              "QPushButton:hover{background-color:rgb(125,125,125);}"
                              "QPushButton:pressed{background-color:rgb(0,125,125);}");

    _labelTime = new QLabel();
    _labelTime->setText("00:00");
    _labelTime->setFont(_font);
    _labelTime->setFixedHeight(20);

    QLabel *timeLabel = new QLabel();
    timeLabel->setText(tr("Duration"));
    timeLabel->setFixedHeight(20);
    timeLabel->setFont(_font);
    controlLayout->addWidget(timeLabel, 0, Qt::AlignLeft);
    controlLayout->addWidget(_labelTime, 1, Qt::AlignLeft);
    controlLayout->addWidget(_btnSwitch, 1, Qt::AlignRight);

    transportLayout->addLayout(controlLayout, Qt::AlignRight);


    transportGroup->setLayout(transportLayout);

    QHBoxLayout *mylayout = new QHBoxLayout();
    mylayout->addWidget(transportGroup);

    QVBoxLayout *vLayout = new QVBoxLayout();

    QPushButton *closeBtn = new QPushButton;
    closeBtn->setIcon(QIcon(":/res/close.png"));
    closeBtn->setFixedSize(25,25);
    closeBtn->setIconSize(QSize(20, 20));
    QObject::connect(closeBtn, &QPushButton::clicked, [=]{
        QWidget::close();
    });


    QPushButton *minus = new QPushButton;
    minus->setIcon(QIcon(":/res/minus.png"));
    minus->setFixedSize(25,25);
    minus->setIconSize(QSize(20, 20));
    QObject::connect(minus, &QPushButton::clicked, [=]{
        QWidget::hide();
    });

    vLayout->addWidget(closeBtn, 0, Qt::AlignTop);
    vLayout->addWidget(minus, 1, Qt::AlignTop);

    vLayout->setMargin(1);  //内边距

    QWidget *vWidget = new QWidget();
    vWidget->setFixedWidth(28);
    vWidget->setLayout(vLayout);
    mylayout->addWidget(vWidget);
    mylayout->setSpacing(1); //外边距

    ui->centralwidget->setLayout(mylayout);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->globalX() - _lastPos.x();
    int dy = event->globalY() - _lastPos.y();

    _lastPos = event->globalPos();

    move(x() + dx, y() + dy);
}

void MainWindow::InitTrayIcon()
{
    _trayIcon = new QSystemTrayIcon(this);
    _trayIcon->setIcon(QIcon(":/res/recorder.png"));
    _trayIcon->setToolTip(tr("TopTitle"));
    _trayMenu = new QMenu(this);
    _trayMenu->addAction(tr("ShowWin"),this,[=](){
          this->show();
          this->activateWindow();
     });
    _trayMenu->addAction(tr("QuitWin"),this,[=](){
        QWidget::close();
//          qApp->quit();
     });
    _trayIcon->setContextMenu(_trayMenu);
    _trayIcon->show();
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
   _lastPos = event->globalPos();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    qDebug() << "close event" << event->type();
    if(_recThread->isRunning())_recThread->requestInterruption();
    while(_recThread->isRunning())QThread::sleep(1);
}

void MainWindow::UpdateTrayIcon()
{
    if(_step++%2 == 0)
        _trayIcon->setIcon(QIcon(":/res/recorder_anim_1.png"));
    else
        _trayIcon->setIcon(QIcon(":/res/recorder_anim_2.png"));
}

MainWindow::~MainWindow()
{
    qDebug() << "destructure event";
    delete ui;
}

