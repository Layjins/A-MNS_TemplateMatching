#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QpMatView>
#include <opencv2/opencv.hpp>
using namespace cv;

#include <QFileDialog>

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
    void on_pushButtonTemp_clicked();

    void on_pushButtonROI_clicked();

    void on_pushButtonROI_2_clicked();

    void on_pushButtonMatch_clicked();

    void on_pushButtonROI_3_clicked();

    void on_pushButtonROI_4_clicked();

private:
    Ui::MainWindow *ui;

    //定义用于控制mat显示的类指针*mview
    QpMatView *mview;

};

#endif // MAINWINDOW_H
