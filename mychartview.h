#ifndef MYCHARTVIEW_H
#define MYCHARTVIEW_H

#include <QtCharts>
#include <QChartView>
#include <QMouseEvent>
#include <QDebug>
#include <QStatusBar>
#include <QLabel>
#include <QChart>

namespace MyChartViewNameSpace {
typedef enum{
    Normal,
    Bigger,
    Smaller,
    Pull //拉动
}MyChartViewMode;
}

using namespace MyChartViewNameSpace;

class MyChartView : public QChartView
{
    Q_OBJECT
public:
    MyChartView(QChart *t,QStatusBar *s);
    void setMode(MyChartViewMode mode);
protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
private:
    QStatusBar * status_bar;
    QLabel * label;
    QChart * chart;
    MyChartViewMode mode;
    bool pressed;
    QPointF pull_start;
};

#endif // MYCHARTVIEW_H
