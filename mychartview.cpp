#include "mychartview.h"

MyChartView::MyChartView(QChart *t,QStatusBar *bar=nullptr):QChartView(t)
{
    status_bar=bar;
    mode=Normal;
    if(bar){
        label=new QLabel(this);
        bar->addPermanentWidget(label);
        chart=t;
    }
}


void MyChartView::mousePressEvent(QMouseEvent *event){
    int x,y;

    x=event->x();
    y=event->y();

    QPointF temp=chart->mapToValue(QPointF(x,y),chart->series().at(0));
    switch(mode){
    case Normal:
        if(label){
            label->setText(QString::number(temp.x())+"      ,       "+QString::number((temp.y())));
        }
        break;
    case Bigger:

        break;
    case Smaller:
        break;
    case Pull:
        pressed=true;
        pull_start.setX(x);
        pull_start.setY(y);
        break;
    }
    QChartView::mousePressEvent(event);

}

#define PULL_Y_Factor 0.2
void MyChartView::mouseMoveEvent(QMouseEvent *event){
    int x,y;
    x=event->x();
    y=event->y();
    QValueAxis * axisx,*axisy;
    Q_UNUSED(axisx);
    axisx=(QValueAxis *)(chart->axisX());
    axisy=(QValueAxis *)(chart->axisY());
    if(mode==Pull){
        if(!pressed)
            return ;
        if(x>pull_start.x()){
            //右拉
        }else if(x<pull_start.x()){
            //左拉
        }
        if(y>pull_start.y()){
            //下拉
            float sub_y=fabs(pull_start.y()-y);
            float max=fabs(axisy->max()-axisy->min());

            float temp=sub_y/this->height()*max*PULL_Y_Factor;
            chart->axisY()->setMin(axisy->min()+temp);
            chart->axisY()->setMax(axisy->max()+temp);
        }else if(y<pull_start.y()){
            float sub_y=fabs(pull_start.y()-y);
            float max=fabs(axisy->max()-axisy->min());

            float temp=sub_y/this->height()*max*PULL_Y_Factor;
            chart->axisY()->setMin(axisy->min()-temp);
            chart->axisY()->setMax(axisy->max()-temp);
        }
    }
    QChartView::mouseMoveEvent(event);
}

void MyChartView::mouseReleaseEvent(QMouseEvent *event){
    Q_UNUSED(event);
    if(mode==Pull){
        pressed=false;
    }

}

void MyChartView::setMode(MyChartViewMode mode){
    QValueAxis * axisx,*axisy;
    axisx=(QValueAxis *)(chart->axisX());
    axisy=(QValueAxis *)(chart->axisY());
    Q_UNUSED(axisx);
    float max=fabs(axisy->max()-axisy->min());
    float add_temp;
    float middle=max/2+axisy->min();


    if(mode==Normal){
        this->mode=mode;
        this->setCursor(QCursor(Qt::CrossCursor));
    }else if(mode==Bigger){
        //this->setCursor(QCursor(Qt::SizeVerCursor));
        add_temp=max/4;
        axisy->setMax(middle+add_temp);
        axisy->setMin(middle-add_temp);
    }else if(mode==Smaller){
        add_temp=max;
        axisy->setMax(middle+add_temp);
        axisy->setMin(middle-add_temp);
        //this->setCursor(QCursor(Qt::SplitVCursor));
    }else if(mode==Pull){
        this->mode=mode;
        this->setCursor(QCursor(Qt::OpenHandCursor));
    }
}
