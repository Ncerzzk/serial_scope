#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtCharts/QChart>
#include "QSerialPort"
#include "QSerialPortInfo"

#include "QLineSeries"
#include "QSplineSeries"
#include "QValueAxis"

#include "QChartView"
#include "QDebug"

#include "math.h"

#include "QMessageBox"
#include "string.h"
#include <QTimer>
#include <QScrollBar>

#include <QAbstractSeries>
#include <QSignalMapper>
#include <QSettings>

#include "mychartview.h"

using namespace QtCharts;
using namespace MyChartViewNameSpace;

QChart *m_chart;
MyChartView * m_chartview;

QSerialPort *serial_port;
QList<QByteArray> data_list;
QTimer *timer=NULL;

#define Frame QList<float>
QList<Frame> all_data;
QList<QList<QPointF>> disp_data;
//QList<QSplineSeries *> series;
QList<QLineSeries *> series;
int data_index=0;

bool receive_edit_pause=false;

int16_t Disp_Num=1000;
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    button_init();

    fresh_coms();


    load();
    wave_checkbox_init();
    chart_init();


}

MainWindow::~MainWindow()
{
    delete ui;
}

QList<QCheckBox *> checkboxs;
void MainWindow::checkbox_map_slot(int i){
    //qDebug()<<checkboxs.at(i-1)->isChecked();
    series[i-1]->setVisible(checkboxs.at(i-1)->isChecked());
}

void MainWindow::wave_checkbox_init(){
    QCheckBox * checkbox;
    QSignalMapper * signal_map=new QSignalMapper(this);
    int num=ui->lineEdit_wave_num->text().toInt();
    for(int i=1;i<num+1;++i){
        checkbox=new QCheckBox(this);
        checkbox->setChecked(true);
        checkbox->setText("CH"+QString::number(i));

        connect(checkbox,SIGNAL(toggled(bool)),signal_map,SLOT(map()));
        signal_map->setMapping(checkbox,i);

        ui->horizontalLayout_2->addWidget(checkbox);
        checkboxs.append(checkbox);

    }
    connect(signal_map,SIGNAL(mapped(int)),this,SLOT(checkbox_map_slot(int)));
}

QList<QLineEdit *> edits;
QList<QPushButton *> buttons;
void MainWindow::button_init(){
    QPushButton * button;
    QLineEdit * edit;
    QSignalMapper * signal_map=new QSignalMapper(this);
    for(int i=0;i<7;++i){
        edit=new QLineEdit(this);
        button=new QPushButton(this);
        edits.append(edit);
        buttons.append(button);

        button->setText("发送");
        ui->gridLayout_4->addWidget(edit,i,0);
        ui->gridLayout_4->addWidget(button,i,1);

        connect(button,SIGNAL(clicked(bool)),signal_map,SLOT(map()));
        signal_map->setMapping(button,i);
    }
    button=new QPushButton(this);
    button->setText("保存");
    ui->gridLayout_4->addWidget(button);
    //ui->gridLayout_3->addWidget(button);
    connect(button,SIGNAL(clicked(bool)),this,SLOT(save()));
    connect(signal_map,SIGNAL(mapped(int)),this,SLOT(button_map_slot(int)));
}

void MainWindow::save(){
    QSettings * setting=new QSettings("set.ini",QSettings::IniFormat);
    setting->beginGroup("Fast");     // 快捷命令
    for(int i=0;i<edits.length();++i){
        setting->setValue("edit"+QString::number(i),edits[i]->text());
    }
    setting->endGroup();
    setting->beginGroup("Set");
    setting->setValue("pause",ui->checkBox_pause->isChecked());
    setting->endGroup();
    setting->beginGroup("Wave");
    setting->setValue("ymax",ui->lineEdit_ymax->text());
    setting->setValue("ymin",ui->lineEdit_ymin->text());
    setting->setValue("wavenum",ui->lineEdit_wave_num->text());
    setting->endGroup();

    setting->beginGroup("Com");
    setting->setValue("com_index",ui->comboBox->currentIndex());
    setting->setValue("baudrate_index",ui->comboBox_baudrate->currentIndex());
    setting->endGroup();


    QMessageBox::about(NULL,"提示","保存完毕，如果更改通道数，请重启程序!");

}
void MainWindow::load(){
    QSettings * setting=new QSettings("set.ini",QSettings::IniFormat);
    setting->beginGroup("Fast");
    for(int i=0;i<edits.length();++i){
        edits[i]->setText(setting->value("edit"+QString::number(i),"").toString());
    }
    setting->endGroup();
    setting->beginGroup("Set");
    ui->checkBox_pause->setChecked(setting->value("pause",false).toBool());
    setting->endGroup();
    setting->beginGroup("Wave");
    ui->lineEdit_ymax->setText(setting->value("ymax","100").toString());
    ui->lineEdit_ymin->setText(setting->value("ymin","-100").toString());
    ui->lineEdit_wave_num->setText(setting->value("wavenum","4").toString());
    setting->endGroup();

    setting->beginGroup("Com");
    ui->comboBox->setCurrentIndex(setting->value("com_index",-1).toInt());
    ui->comboBox_baudrate->setCurrentIndex(setting->value("baudrate_index",-1).toInt());
    setting->endGroup();

}

void MainWindow::button_map_slot(int i){
    QByteArray s=edits[i]->text().toLatin1();
    if(ui->checkBox_send_newline->isChecked()){
        s.append('\r');
        s.append('\n');
    }
    serial_port->write(s);
}
void MainWindow::chart_init(){
    QLineSeries * temp;
    int wave_num;
    m_chart=new QChart();

    MyChartView *chartView=new MyChartView(m_chart,ui->statusBar);
    m_chartview=chartView;
    QValueAxis *axisX = new QValueAxis(this);
    axisX->setRange(0,Disp_Num);
    axisX->setLabelFormat("%g");
    QValueAxis *axisY = new QValueAxis(this);
    axisY->setRange(ui->lineEdit_ymin->text().toFloat(),ui->lineEdit_ymax->text().toFloat());

    wave_num=ui->lineEdit_wave_num->text().toInt();
    for (int i=0;i<wave_num;++i){
        temp=new  QLineSeries(this);
        m_chart->addSeries(temp);
        series.append(temp);
        temp->setName("Wave"+QString::number(i+1));
        disp_data.append(QList<QPointF>());
        temp->setUseOpenGL(true);//openGl 加速
        m_chart->setAxisX(axisX,temp);
        m_chart->setAxisY(axisY,temp);
    }
    QColor color;
    color.setNamedColor("red");
    series[0]->setColor(color);
    color.setNamedColor("yellow");
    series[1]->setColor(color);
    color.setNamedColor("blue");
    series[2]->setColor(color);
    color.setNamedColor("black");
    series[3]->setColor(color);


    ui->verticalLayout->addWidget(chartView);
    m_chart->legend()->setAlignment(Qt::AlignTop);
    m_chart->legend()->show();

}
void MainWindow::fresh_coms(){
    QStringList coms;
    ui->comboBox->clear();
    foreach(const QSerialPortInfo &info,QSerialPortInfo::availablePorts()){
        ui->comboBox->addItem(info.portName());
    }
    ui->comboBox_baudrate->addItem("9600");
    ui->comboBox_baudrate->addItem("38400");
    ui->comboBox_baudrate->addItem("115200");

    ui->comboBox_baudrate->setCurrentIndex(2);
}
void MainWindow::on_pushButton_open_clicked()
{
    qDebug()<<ui->comboBox->currentText();
    qDebug()<<ui->comboBox_baudrate->currentText();
    if(ui->pushButton_open->text()=="打开串口"){
        serial_port=new QSerialPort(this);
        serial_port->setPortName(ui->comboBox->currentText());
        serial_port->setBaudRate(ui->comboBox_baudrate->currentText().toInt());
        serial_port->setDataBits(QSerialPort::Data8);
        //设置校验位
        serial_port->setParity(QSerialPort::NoParity);
        //设置流控制
        serial_port->setFlowControl(QSerialPort::NoFlowControl);
        //设置停止位
        serial_port->setStopBits(QSerialPort::OneStop);
        serial_port->setReadBufferSize(200000);

        if(serial_port->open(QIODevice::ReadWrite)){
            ui->pushButton_open->setText("关闭串口");
            ui->comboBox->setDisabled(true);
            ui->comboBox_baudrate->setDisabled(true);
            QObject::connect(serial_port, &QSerialPort::readyRead, this, &MainWindow::serial_portRecvMsgEvent);
        }else{
            QMessageBox::about(NULL,"提示","打开串口失败！");
                return ;
        }

        start_update();
    }else{
        serial_port->close();
        ui->pushButton_open->setText("打开串口");

        ui->comboBox->setDisabled(false);
        ui->comboBox_baudrate->setDisabled(false);
        stop_update();
    }
}

void MainWindow::start_update(){
    if(!timer){
        timer=new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(updat_chart_slot()));
    }
    timer->start(2);
}

void MainWindow::stop_update(){
    if(timer){
        timer->stop();
    }
}
void MainWindow::updat_chart_slot(){
    int length=all_data.length();  //本次进入函数时，数据的总长度

    for(int i=data_index;i<length;++i){
        for(int j=0;j<series.length();++j){
            if(disp_data[j].length()<Disp_Num){
                disp_data[j].append(QPointF(i,all_data[i][j]));
            }else{
                disp_data[j].removeFirst(); //删除最前的一个数，以在末尾添加一个
                disp_data[j].append(QPointF(i,all_data[i][j]));
            }
        }
    }
    data_index=length;

    if(disp_data[0].length()>=Disp_Num){
        m_chart->axisX()->setMin(data_index-Disp_Num);
        m_chart->axisX()->setMax(data_index);

    }
    for(int i=0;i<m_chart->series().length();++i){
        series[i]->replace(disp_data[i]);
    }
}

QByteArray rx_buffer;
int rx_cnt=0;
void MainWindow::serial_portRecvMsgEvent()
{
    QByteArray temp;
    QScrollBar *scrollbar;
    QList<QList<float>> result;

    /*
     * 这个方法用来同时更新编辑框和波形
     * 但发现这样的话编辑框更新太慢，会有卡顿现象
     * 目前的解决方法是显示波形时就不更新编辑框
     *
     */
    temp=serial_port->readAll();
    rx_buffer.append(temp);
    rx_cnt+=temp.length();
    if(rx_cnt>100){
        qDebug()<<serial_port->bytesAvailable();
        result=analize(rx_buffer);
        rx_buffer.clear();
        foreach(Frame frame ,result){
            all_data.append(frame);
        }
        rx_cnt=0;
        ui->horizontalScrollBar->setMaximum(all_data.length());
    }

    if(!receive_edit_pause){
        ui->textEdit->setText(ui->textEdit->toPlainText()+QString(temp));
        scrollbar=ui->textEdit->verticalScrollBar();
        if(scrollbar){
            scrollbar->setSliderPosition(scrollbar->maximum());
        }
    }

}
void MainWindow::on_pushButton_fresh_clicked()
{
    fresh_coms();
}

QList<QList<float>> MainWindow::analize(QByteArray bytes){
    int data_length=0;
    int data_type=6;
    int data_num=0;
    QList<float> frame;
    QList<QList<float>> result;
    float temp=0;
    int len=bytes.length();

    char names[4][4];
    for(int i=0;i<len-6;++i){
        if(bytes[i]=='b'){
            //找到了疑似帧头
            frame.clear();
            if(i+6+data_length>len){
                break; //长度不够了
            }
            if(bytes[i+1]!='y'){
                continue;
            }
            data_length=bytes[i+2];
            data_type=bytes[i+3];

            if(bytes[i+4+data_length]!='\r' || bytes[i+5+data_length]!='\n'){
                continue; //确认帧尾失败
            }
            switch(data_type){
            case 0:
                break;
            case 1:
                break;
            case 2:
                break;
            case 3:break;
            case 4:break;
            case 5:break;
            case 6:
                data_num=data_length/4;
                for(int data_cnt=0;data_cnt<data_num;++data_cnt){
                    memcpy(&temp,bytes.data()+i + 4 + data_cnt * 4,4);
                    frame.append(temp);
                }
                result.append(frame);
                break;
            case 7:
                //波形名字
                for(int name_cnt=0;name_cnt<4;++name_cnt){
                    memcpy(names[name_cnt],bytes.data()+i+4+name_cnt*4,4);
                    series[name_cnt]->setName(QString(names[name_cnt]));
                }
                break;
            }


            i += 5 + data_length; //跳过本帧
        }
    }
    return result;
}

void MainWindow::on_pushButton_clicked()
{
    qDebug()<<disp_data[0].length();
}


/*
 *  清空当前波形
 * */
static void clear_series(){
    for(int i=0;i<series.length();++i){
        disp_data[i].clear();
    }
}

static void restore_series(int x_min,int x_max){
    x_min=x_min<0?0:x_min;
    for(int i=0;i<series.length();++i){
        disp_data[i].clear();
        for(int j=x_min;j<x_max&&j<all_data.length();++j){
            disp_data[i].append(QPointF(j,all_data[j][i]));
        }
        series[i]->replace(disp_data[i]);
    }
}

void MainWindow::on_checkBox_stateChanged(int arg1)
{
    Q_UNUSED(arg1);
    if(ui->checkBox->isChecked()){
        //选择跟随
        clear_series();
        data_index=all_data.length()-Disp_Num/2;
        if(data_index<0){
            data_index=0;
        }
        m_chart->axisX()->setMin(data_index);
        m_chart->axisX()->setMax(data_index+Disp_Num);
        start_update();
        ui->horizontalScrollBar->setDisabled(true);
    }else{
        stop_update();
        ui->horizontalScrollBar->setDisabled(false);
    }
}

void MainWindow::on_horizontalScrollBar_sliderMoved(int position)
{
    //将position作为中间
    if(position-Disp_Num/2<0){
        position=Disp_Num/2;
    }

    restore_series(position-Disp_Num/2,position+Disp_Num/2);
    /*
    for(int i=0;i<series.length();++i){
        disp_data[i].clear();
        for(int j=(position-Disp_Num/2);j<position+Disp_Num/2&&j<all_data.length();++j){
            disp_data[i].append(QPointF(j,all_data[j][i]));
        }
        series[i]->replace(disp_data[i]);
    }*/
    m_chart->axisX()->setMin(position-Disp_Num/2);
    m_chart->axisX()->setMax(position+Disp_Num/2);
}

void MainWindow::on_pushButton_set_range_clicked()
{
    int x_max,x_min;
    x_max=ui->lineEdit_xmax->text().toInt();
    x_min=ui->lineEdit_xmin->text().toInt();


    m_chart->axisY()->setMin(ui->lineEdit_ymin->text().toFloat());
    m_chart->axisY()->setMax(ui->lineEdit_ymax->text().toFloat());

    adjust_X_disp_range(x_min,x_max);

}

void MainWindow::on_pushButton_send_clicked()
{
    qDebug()<<"send!";
    if(ui->pushButton_open->text()!="关闭串口"){
        return ;
    }

    QByteArray s=ui->lineEdit_send->text().toLatin1();
    if(ui->checkBox_send_newline->isChecked()){
        s.append('\r');
        s.append('\n');
    }
    serial_port->write(s);
}

void MainWindow::on_pushButton_clear_clicked()
{
    ui->textEdit->clear();
}

void MainWindow::on_checkBox_pause_stateChanged(int arg1)
{
    if(arg1){
        //暂停
        receive_edit_pause=true;
    }else{
        //不暂停
        receive_edit_pause=false;
    }
}

void MainWindow::on_pushButton_clear_wave_clicked()
{
    stop_update();
    foreach (QLineSeries * temp, series) {
        temp->clear();
    }
    all_data.clear();
    foreach (QList<QPointF> temp, disp_data) {
        temp.clear();
    }
    data_index=0;
    start_update();

}

void MainWindow::on_toolButton_bigger_clicked()
{
    m_chartview->setMode(Bigger);
}

void MainWindow::on_toolButton_normal_clicked()
{
    m_chartview->setMode(Normal);
}

void MainWindow::on_toolButton_smaller_clicked()
{
    m_chartview->setMode(Smaller);
}

void MainWindow::on_toolButton_pull_clicked()
{
    m_chartview->setMode(Pull);
}


void MainWindow::on_actionabout_triggered()
{
    QMessageBox::information(this,"BUPTSmartRobot","designed by hcm.");

}



void MainWindow::on_pushButton_save_clicked()
{
    save();

}

void MainWindow::adjust_X_disp_range(int x_min,int x_max){
    Disp_Num=x_max-x_min;
    if(timer && timer->isActive()){
        stop_update();
        if(Disp_Num>disp_data[0].length()){
            for(int i=0;i<disp_data[0].length()-Disp_Num;++i){
                for(int j=0;j<series.length();++j){
                    disp_data[j].removeAt(i);
                }
            }
        }
        start_update();
    }else{
        //未处于跟随状态

        restore_series(x_min,x_max); //清空，并重新灌入数据
        m_chart->axisX()->setRange(x_min,x_max);
    }
}
void MainWindow::on_toolButton_X_bigger_clicked()
{
    int temp_disp_num;
    QValueAxis * axis;
    int x_min,x_max;
    axis=(QValueAxis *)(m_chart->axisX());
    x_min=(int)axis->min();
    x_max=(int)axis->max();

    temp_disp_num=x_max-x_min;
    adjust_X_disp_range(x_min+temp_disp_num/4,x_max-temp_disp_num/4);

}

void MainWindow::on_toolButton_X_smaller_clicked()
{
    int temp_disp_num;
    QValueAxis * axis;
    int x_min,x_max;
    axis=(QValueAxis *)(m_chart->axisX());
    x_min=(int)axis->min();
    x_max=(int)axis->max();

    temp_disp_num=x_max-x_min;
    adjust_X_disp_range(x_min-temp_disp_num/2,x_max+temp_disp_num/2);
}

void MainWindow::on_pushButton_save_wave_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                        tr("保存当前波形"),
                                                        "",
                                                        tr("波形数据(*.wave)"));
    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this,tr("错误"),tr("打开文件失败"));
        return;
    }

    qDebug()<<fileName;

    QString text;

    int count=0;
    foreach(Frame temp_data,all_data){
        text+=QString::number(count)+",";     // X
        for(int i=0;i<series.length();++i){
            text+=QString::number((double)temp_data[i]);
            if(i<series.length()-1){
                text+=",";
            }
        }
        text+="\n";
        count++;
    }
    QTextStream textStream(&file);
    textStream<<text<<"test";
    file.close();
}
