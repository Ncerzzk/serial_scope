#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsSceneMouseEvent>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pushButton_open_clicked();

    void on_pushButton_fresh_clicked();

    void serial_portRecvMsgEvent();
    void updat_chart_slot();
    void on_pushButton_clicked();

    void on_checkBox_stateChanged(int arg1);

    void on_horizontalScrollBar_sliderMoved(int position);

    void on_pushButton_set_range_clicked();

    void on_pushButton_send_clicked();
    void button_map_slot(int);
    void save();

    void on_pushButton_clear_clicked();

    void on_checkBox_pause_stateChanged(int arg1);

    void on_pushButton_clear_wave_clicked();

    void on_toolButton_bigger_clicked();

    void on_toolButton_normal_clicked();

    void on_toolButton_smaller_clicked();

    void on_toolButton_pull_clicked();


    void on_actionabout_triggered();
    void checkbox_map_slot(int i);
    void wave_checkbox_init();

    void on_pushButton_save_clicked();

    void on_toolButton_X_bigger_clicked();

    void adjust_X_disp_range(int x_min,int x_max);

    void on_toolButton_X_smaller_clicked();

    void on_pushButton_save_wave_clicked();

private:
    void chart_init();
    void fresh_coms();
    void start_update();
    void stop_update();
    void button_init();
    void load();
    QList<QList<float>> analize(QByteArray temp);
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
