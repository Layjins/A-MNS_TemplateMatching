#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qdebug"
#include <QtCore/QTextStream>
#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <stdio.h>
#include "math.h"
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include<iostream>
#include<fstream>
#include <QFile>
#include<cstdlib>
#include<ctime>
#define random(a,b) (rand()%(b-a+1)+a)


//模板匹配参数
//定义全局变量
//定义用于控制mat显示的类指针
QpMatView *ROIview;
QpMatView *Imgview;

int Ldisk=16;//局部方块边长
int R=(int)Ldisk/2;//局部方块半径
float PSMode=2;//邻居距离倍数；邻居距离=PSMode*Ldisk/2；
float Thresh_distance=(float)(PSMode*Ldisk);//邻居距离阈值
int MatchingNum2=5;//环投影候选点数量
int Thresh_MBNSnum=10;//MBNS邻居数量阈值
int N=20;//Zernike矩的阶数/2=N
float Rthre=0.9;//Zernike矩阈值
bool templateProcessFlag=0;//模板图处理过程的标志
int graphicsViewWidth;//图片显示框大小（正方形）


//输出性能参数
float MNS_second_SumTime=0;//MNS耗时

//遗传算法参数
int popular=0;//种群数量
int Maxgen=0;//最大迭代次数（种群最大进化代数generation）



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //绑定mat显示控件，必须在setupUi之后
    mview = new QpMatView(ui->graphicsViewTemp);
    ROIview = new QpMatView(ui->graphicsViewROI);
    Imgview = new QpMatView(ui->graphicsViewImg);

    //显示模板匹配初始输入参数
    QString CharPSMode;
    CharPSMode=QString::number(PSMode);
    ui->lineEdit_3->setText(CharPSMode);

    QString CharLdisk;
    CharLdisk=QString::number(Ldisk);
    ui->lineEdit->setText(CharLdisk);

    QString CharRthre;
    CharRthre=QString::number(Rthre);
    ui->lineEdit_2->setText(CharRthre);

    QString CharThresh_MBNSnum;
    CharThresh_MBNSnum=QString::number(Thresh_MBNSnum);
    ui->lineEdit_6->setText(CharThresh_MBNSnum);

    QString CharMatchingNum2;
    CharMatchingNum2=QString::number(MatchingNum2);
    ui->lineEdit_4->setText(CharMatchingNum2);

    QString CharN;
    CharN=QString::number(N);
    ui->lineEdit_5->setText(CharN);

    QString Charpopular;
    Charpopular=QString::number(popular);
    ui->lineEdit_8->setText(Charpopular);

    QString CharMaxgen;
    CharMaxgen=QString::number(Maxgen);
    ui->lineEdit_7->setText(CharMaxgen);



    graphicsViewWidth=ui->graphicsViewImg->width();
}

MainWindow::~MainWindow()
{
    delete ui;
}


//图像添加白噪声函数
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <cstdlib>
#include <limits>
#include <cmath>
using namespace cv;
using namespace std;
double generateGaussianNoise(double mu, double sigma)
{
    //定义一个特别小的值
    const double epsilon = numeric_limits<double>::min();//返回目标数据类型能表示的最逼近1的正数和1的差的绝对值
    static double z0, z1;
    static bool flag = false;
    flag = !flag;
    //flag为假，构造高斯随机变量
    if (!flag)
        return z1*sigma + mu;
    double u1, u2;
    //构造随机变量

    do
    {
        u1 = rand()*(1.0 / RAND_MAX);
        u2 = rand()*(1.0 / RAND_MAX);
    } while (u1 <= epsilon);
    //flag为真构造高斯随机变量X
    z0 = sqrt(-2.0*log(u1))*cos(2 * CV_PI * u2);
    z1 = sqrt(-2.0*log(u1))*sin(2 * CV_PI * u2);
    return z1*sigma + mu;
}
//为图像添加高斯噪声
Mat addGaussianNoise(Mat& srcImage,uint Gtimes)
{
    Mat resultImage = srcImage.clone();    //深拷贝,克隆
    int channels = resultImage.channels();    //获取图像的通道
    int nRows = resultImage.rows;    //图像的行数

    int nCols = resultImage.cols*channels;   //图像的总列数
    //判断图像的连续性
    if (resultImage.isContinuous())    //判断矩阵是否连续，若连续，我们相当于只需要遍历一个一维数组
    {
        nCols *= nRows;
        nRows = 1;
    }
    for (int i = 0; i < nRows; i++)
    {
        for (int j = 0; j < nCols; j++)
        {	//添加高斯噪声
            int val = resultImage.ptr<uchar>(i)[j] + generateGaussianNoise(2, 0.8)*Gtimes;//均值为2，方差为0.8的高斯噪声;Gtimes为高斯噪声倍数
            if (val < 0)
                val = 0;
            if (val > 255)
                val = 255;
            resultImage.ptr<uchar>(i)[j] = (uchar)val;
        }
    }
    return resultImage;
}
//为图像添加加性噪声（改变图像整体亮度）
Mat addAddNoise(Mat& srcImage,int AddValue)
{
    Mat resultImage = srcImage.clone();    //深拷贝,克隆
    int channels = resultImage.channels();    //获取图像的通道
    int nRows = resultImage.rows;    //图像的行数

    int nCols = resultImage.cols*channels;   //图像的总列数
    //判断图像的连续性
    if (resultImage.isContinuous())    //判断矩阵是否连续，若连续，我们相当于只需要遍历一个一维数组
    {
        nCols *= nRows;
        nRows = 1;
    }
    for (int i = 0; i < nRows; i++)
    {
        for (int j = 0; j < nCols; j++)
        {	//添加加性噪声
            int val = resultImage.ptr<uchar>(i)[j] + AddValue;//AddValue为迭加的灰度值
            if (val < 0)
                val = 0;
            if (val > 255)
                val = 255;
            resultImage.ptr<uchar>(i)[j] = (uchar)val;
        }
    }
    return resultImage;
}
//为图像添加乘性噪声（改变图像对比度）
Mat addContrastNoise(Mat& srcImage,float MulValue)
{
    Mat resultImage = srcImage.clone();    //深拷贝,克隆
    int channels = resultImage.channels();    //获取图像的通道
    int nRows = resultImage.rows;    //图像的行数

    int nCols = resultImage.cols*channels;   //图像的总列数
    //判断图像的连续性
    if (resultImage.isContinuous())    //判断矩阵是否连续，若连续，我们相当于只需要遍历一个一维数组
    {
        nCols *= nRows;
        nRows = 1;
    }
    for (int i = 0; i < nRows; i++)
    {
        for (int j = 0; j < nCols; j++)
        {	//添加加性噪声
            int val = resultImage.ptr<uchar>(i)[j] * MulValue;//MulValue为迭加的灰度值
            if (val < 0)
                val = 0;
            if (val > 255)
                val = 255;
            resultImage.ptr<uchar>(i)[j] = (uchar)val;
        }
    }
    return resultImage;
}


//读取模板图
Mat TemplateImg;
int beta=0;//图像显示的缩放倍数
void MainWindow::on_pushButtonTemp_clicked()
{
    //创建一张100*100的蓝色图片
    //Mat img(100,100,CV_8UC3,Scalar(255,0,0));

    //读模板图
    QString fileName = QFileDialog::getOpenFileName(NULL,tr("Open Image"),".",tr("Image Files (*.png *.jpg *.bmp)"));
    if(fileName.length()!=0){
        TemplateImg=imread(fileName.toStdString());
        //自适应窗口显示图像
        Mat TemplateImgDisp;
        int resizeWidth;//显示宽度
        int resizeHeight;//显示高度     
        int maxLength;//图像最长边
        maxLength = (TemplateImg.cols>TemplateImg.rows)?TemplateImg.cols:TemplateImg.rows;
        beta=(int)maxLength/graphicsViewWidth;

        switch(beta){
        case 0:
            {
                resizeWidth=TemplateImg.cols;
                resizeHeight=TemplateImg.rows;
            }break;
        case 1:
            {
                resizeWidth=(int)TemplateImg.cols/2;
                resizeHeight=(int)TemplateImg.rows/2;
            }break;
        case 2:
            {
                resizeWidth=(int)TemplateImg.cols/3;
                resizeHeight=(int)TemplateImg.rows/3;
            }break;
        case 3:
            {
                resizeWidth=(int)TemplateImg.cols/4;
                resizeHeight=(int)TemplateImg.rows/4;
            }break;
        case 4:
            {
                resizeWidth=(int)TemplateImg.cols/5;
                resizeHeight=(int)TemplateImg.rows/5;
            }break;
        case 5:
            {
                resizeWidth=(int)TemplateImg.cols/6;
                resizeHeight=(int)TemplateImg.rows/6;
            }break;
        case 6:
            {
                resizeWidth=(int)TemplateImg.cols/7;
                resizeHeight=(int)TemplateImg.rows/7;
            }break;
        case 7:
            {
                resizeWidth=(int)TemplateImg.cols/8;
                resizeHeight=(int)TemplateImg.rows/8;
            }break;
        case 8:
            {
                resizeWidth=(int)TemplateImg.cols/9;
                resizeHeight=(int)TemplateImg.rows/9;
            }break;
        case 9:
            {
                resizeWidth=(int)TemplateImg.cols/10;
                resizeHeight=(int)TemplateImg.rows/10;
            }break;
        case 10:
            {
                resizeWidth=(int)TemplateImg.cols/11;
                resizeHeight=(int)TemplateImg.rows/11;
            }break;
        default:
            {
                resizeWidth=(int)TemplateImg.cols/12;
                resizeHeight=(int)TemplateImg.rows/12;
            }break;
        }


        //resize函数前加cv：：，因为Qt也有resize函数，故加cv：：加以区别，不然会出现错误
        cv::resize(TemplateImg,TemplateImgDisp,Size(resizeWidth,resizeHeight),0,0,3);
        mview->imshow(TemplateImgDisp);//显示到绑定的控件上
        //cv::resize(TemplateImg,TemplateImg,Size(resizeWidth,resizeHeight),0,0,3);
        //mview->imshow(TemplateImg);//显示到绑定的控件上
    }


    //透视变换



/*
    //获取模板匹配测试图像集（添加白噪声后旋转）
    Mat gaussianImg,gaussresultImage;
    TemplateImg.copyTo(gaussianImg);
    gaussianImg.copyTo(gaussresultImage);

    //Point RotationCenter2=Point(376,240);//绕图像中心
    //Point RotationCenter2=Point(356,283);//绕60华为模板
    //Point RotationCenter2=Point(355,224);//绕120华为模板
    //Point RotationCenter2=Point(368,207);//绕200老干妈模板
    //Point RotationCenter2=Point(380,235);//绕200华为LOGO模板

    //Point RotationCenter2=Point(170,246);//绕200logo模板
    Point RotationCenter2=Point(437,303);//绕200word模板
    double RotationScale2=1;
    Mat RotationMat2(2,3,CV_32FC1);
    Mat RotationImg2;
    for(int i=0;i<360;i++)
    {

        //gaussresultImage = addGaussianNoise(gaussianImg,30);//添加高斯噪声：倍数区间1到12，每隔1取值
        //gaussresultImage = addAddNoise(gaussianImg,-60);//添加加性噪声（改变图像整体亮度）:叠加亮度区间-30到30，每隔5取值
        gaussresultImage = addContrastNoise(gaussianImg,0.2);//添加乘性噪声（改变图像对比度）：倍数区间
        RotationMat2=getRotationMatrix2D(RotationCenter2,(double)i,RotationScale2);
        warpAffine(gaussresultImage,RotationImg2,RotationMat2,TemplateImg.size());
        char str[200];  
        //sprintf(str,"F:/Project/0QT/QtCode/image_Match/huawei/%d.bmp",i);
        //sprintf(str,"F:/Project/0QT/QtCode/image_Match/laoganma/%d.bmp",i);
        //添加白噪声的文件夹
        //sprintf(str,"F:/Project/0QT/QtCode/image_Match/60Gausshuawei/%d.bmp",i);
        //sprintf(str,"F:/Project/0QT/QtCode/image_Match/200Gausslaoganma/%d.bmp",i);
        sprintf(str,"F:/Project/0QT/QtCode/image_Match/temp/%d.bmp",i);
        imwrite(str,RotationImg2);
    }

*/
/*
    //求测试图旋转后，模板图的精确坐标
    Point ImgCenter,OriginTemplateCenter60,OriginTemplateCenter120,OriginTemplateCenter200;//中心坐标
    ImgCenter.x=376;
    ImgCenter.y=240;
    OriginTemplateCenter60.x=326;
    OriginTemplateCenter60.y=253;
    OriginTemplateCenter120.x=295;
    OriginTemplateCenter120.y=164;
    OriginTemplateCenter200.x=268;
    OriginTemplateCenter200.y=107;
    Point TemplateCenter60,TemplateCenter120,TemplateCenter200;//模板中心相对测试图的坐标
    TemplateCenter60.x=OriginTemplateCenter60.x-ImgCenter.x;
    TemplateCenter60.y=OriginTemplateCenter60.y-ImgCenter.y;
    TemplateCenter120.x=OriginTemplateCenter120.x-ImgCenter.x;
    TemplateCenter120.y=OriginTemplateCenter120.y-ImgCenter.y;
    TemplateCenter200.x=OriginTemplateCenter200.x-ImgCenter.x;
    TemplateCenter200.y=OriginTemplateCenter200.y-ImgCenter.y;

    Point tempCoor;
    int tempcoorX,tempcoorY;
    FILE* fp;
    fp=fopen("xycoordinate60.txt","w");
    for(int i=0;i<360;i++)
    {
        tempcoorX=(int)((float)TemplateCenter60.x*cos(i)-(float)TemplateCenter60.y*sin(i)+0.5);
        tempcoorY=(int)((float)TemplateCenter60.x*sin(i)+(float)TemplateCenter60.y*cos(i)+0.5);
        tempCoor.x=ImgCenter.x+tempcoorX;
        tempCoor.y=ImgCenter.y+tempcoorY;
        fprintf(fp,"(%d,%d)\n",tempCoor.x,tempCoor.y);

    }
    fclose(fp);
    */

}

//鼠标画矩形框选取ROI
Mat org,ROI,img,tmp;
void on_mouse(int event,int x,int y,int flags,void *ustc)//event鼠标事件代号，x,y鼠标坐标，flags拖拽和键盘操作的代号
{
    static Point pre_pt = Point(-1,-1);//初始坐标
    static Point cur_pt = Point(-1,-1);//实时坐标
    char temp[16];
    if (event == CV_EVENT_LBUTTONDOWN)//左键按下，读取初始坐标，并在图像上该点处划圆
    {
        org.copyTo(img);//将原始图片复制到img中
        sprintf(temp,"(%d,%d)",x,y);
        pre_pt = Point(x,y);
        putText(img,temp,pre_pt,FONT_HERSHEY_SIMPLEX,0.5,Scalar(0,0,255));//在窗口上显示坐标
        circle(img,pre_pt,2,Scalar(255,0,0),CV_FILLED,CV_AA,0);//划圆
        imshow("TemplateImg",img);
    }
    else if (event == CV_EVENT_MOUSEMOVE && !(flags & CV_EVENT_FLAG_LBUTTON))//左键没有按下的情况下鼠标移动的处理函数
    {
        img.copyTo(tmp);//将img复制到临时图像tmp上，用于显示实时坐标
        sprintf(temp,"(%d,%d)",x,y);
        cur_pt = Point(x,y);
        putText(tmp,temp,cur_pt,FONT_HERSHEY_SIMPLEX,0.5,Scalar(0,0,255));//只是实时显示鼠标移动的坐标
        imshow("TemplateImg",tmp);
    }
    else if (event == CV_EVENT_MOUSEMOVE && (flags & CV_EVENT_FLAG_LBUTTON))//左键按下时，鼠标移动，则在图像上划矩形
    {
        img.copyTo(tmp);
        sprintf(temp,"(%d,%d)",x,y);
        cur_pt = Point(x,y);
        putText(tmp,temp,cur_pt,FONT_HERSHEY_SIMPLEX,0.5,Scalar(0,0,255));
        rectangle(tmp,pre_pt,cur_pt,Scalar(0,255,0),1,8,0);//在临时图像上实时显示鼠标拖动时形成的矩形
        imshow("TemplateImg",tmp);
    }
    else if (event == CV_EVENT_LBUTTONUP)//左键松开，将在图像上划矩形
    {
        org.copyTo(img);
        sprintf(temp,"(%d,%d)[%d x %d]",x,y,cur_pt.x-pre_pt.x,cur_pt.y-pre_pt.y);
        cur_pt = Point(x,y);
        putText(img,temp,Point(10,15),FONT_HERSHEY_SIMPLEX,0.4,Scalar(0,0,255));
        qDebug("ROI宽高=【%d x %d】",cur_pt.x-pre_pt.x,cur_pt.y-pre_pt.y);
        circle(img,pre_pt,2,Scalar(255,0,0),CV_FILLED,CV_AA,0);
        rectangle(img,pre_pt,cur_pt,Scalar(0,255,0),1,8,0);//根据初始点和结束点，将矩形画到img上
        imshow("TemplateImg",img);
        img.copyTo(tmp);
        //截取矩形包围的图像，并保存到dst中
        int width = abs(pre_pt.x - cur_pt.x);
        int height = abs(pre_pt.y - cur_pt.y);
        if (width == 0 || height == 0)
        {
            printf("width == 0 || height == 0");
            return;
        }
        ROI = org(Rect(min(cur_pt.x,pre_pt.x),min(cur_pt.y,pre_pt.y),width,height));
        //自适应窗口显示图像
        Mat ROIDisp;
        int resizeWidth;//显示宽度
        int resizeHeight;//显示高度

        switch(beta){
        case 0:
            {
                resizeWidth=ROI.cols;
                resizeHeight=ROI.rows;
            }break;
        case 1:
            {
                resizeWidth=(int)ROI.cols/2;
                resizeHeight=(int)ROI.rows/2;
            }break;
        case 2:
            {
                resizeWidth=(int)ROI.cols/3;
                resizeHeight=(int)ROI.rows/3;
            }break;
        case 3:
            {
                resizeWidth=(int)ROI.cols/4;
                resizeHeight=(int)ROI.rows/4;
            }break;
        case 4:
            {
                resizeWidth=(int)ROI.cols/5;
                resizeHeight=(int)ROI.rows/5;
            }break;
        case 5:
            {
                resizeWidth=(int)ROI.cols/6;
                resizeHeight=(int)ROI.rows/6;
            }break;
        case 6:
            {
                resizeWidth=(int)ROI.cols/7;
                resizeHeight=(int)ROI.rows/7;
            }break;
        case 7:
            {
                resizeWidth=(int)ROI.cols/8;
                resizeHeight=(int)ROI.rows/8;
            }break;
        case 8:
            {
                resizeWidth=(int)ROI.cols/9;
                resizeHeight=(int)ROI.rows/9;
            }break;
        case 9:
            {
                resizeWidth=(int)ROI.cols/10;
                resizeHeight=(int)ROI.rows/10;
            }break;
        case 10:
            {
                resizeWidth=(int)ROI.cols/11;
                resizeHeight=(int)ROI.rows/11;
            }break;
        default:
            {
                resizeWidth=(int)ROI.cols/12;
                resizeHeight=(int)ROI.rows/12;
            }break;
        }

        //resize函数前加cv：：，因为Qt也有resize函数，故加cv：：加以区别，不然会出现错误
        cv::resize(ROI,ROIDisp,Size(resizeWidth,resizeHeight),0,0,3);
        ROIview->imshow(ROIDisp);//显示到绑定的控件上

        Ldisk=(int)sqrt(ROI.cols*ROI.rows/26);
        R=(int)Ldisk/2;//局部方块半径
        qDebug("Ldisk=%d",Ldisk);

        templateProcessFlag=!templateProcessFlag;


    }
}
//鼠标画矩形框选取ROI
void MainWindow::on_pushButtonROI_clicked()
{
    TemplateImg.copyTo(org);
    org.copyTo(img);
    org.copyTo(tmp);
    namedWindow("TemplateImg");//定义一个img窗口
    setMouseCallback("TemplateImg",on_mouse,0);//调用回调函数  


}


//直接读取ROI
void MainWindow::on_pushButtonROI_4_clicked()
{
    //接读取ROI
    QString fileName = QFileDialog::getOpenFileName(NULL,tr("Open Image"),".",tr("Image Files (*.png *.jpg *.bmp)"));
    if(fileName.length()!=0){
        ROI=imread(fileName.toStdString());
        //自适应窗口显示图像
        Mat ROIDisp;
        int resizeWidth;//显示宽度
        int resizeHeight;//显示高度

        switch(beta){
        case 0:
            {
                resizeWidth=ROI.cols;
                resizeHeight=ROI.rows;
            }break;
        case 1:
            {
                resizeWidth=(int)ROI.cols/2;
                resizeHeight=(int)ROI.rows/2;
            }break;
        case 2:
            {
                resizeWidth=(int)ROI.cols/3;
                resizeHeight=(int)ROI.rows/3;
            }break;
        case 3:
            {
                resizeWidth=(int)ROI.cols/4;
                resizeHeight=(int)ROI.rows/4;
            }break;
        case 4:
            {
                resizeWidth=(int)ROI.cols/5;
                resizeHeight=(int)ROI.rows/5;
            }break;
        case 5:
            {
                resizeWidth=(int)ROI.cols/6;
                resizeHeight=(int)ROI.rows/6;
            }break;
        case 6:
            {
                resizeWidth=(int)ROI.cols/7;
                resizeHeight=(int)ROI.rows/7;
            }break;
        case 7:
            {
                resizeWidth=(int)ROI.cols/8;
                resizeHeight=(int)ROI.rows/8;
            }break;
        case 8:
            {
                resizeWidth=(int)ROI.cols/9;
                resizeHeight=(int)ROI.rows/9;
            }break;
        case 9:
            {
                resizeWidth=(int)ROI.cols/10;
                resizeHeight=(int)ROI.rows/10;
            }break;
        case 10:
            {
                resizeWidth=(int)ROI.cols/11;
                resizeHeight=(int)ROI.rows/11;
            }break;
        default:
            {
                resizeWidth=(int)ROI.cols/12;
                resizeHeight=(int)ROI.rows/12;
            }break;
        }

        //resize函数前加cv：：，因为Qt也有resize函数，故加cv：：加以区别，不然会出现错误
        cv::resize(ROI,ROIDisp,Size(resizeWidth,resizeHeight),0,0,3);
        ROIview->imshow(ROIDisp);//显示到绑定的控件上

        Ldisk=(int)sqrt(ROI.cols*ROI.rows/26);
        R=(int)Ldisk/2;//局部方块半径
        qDebug("Ldisk=%d",Ldisk);

        templateProcessFlag=!templateProcessFlag;

    }
}


//读待匹配图
Mat Image;
void MainWindow::on_pushButtonROI_2_clicked()
{

    QString CharLdisk;
    CharLdisk=QString::number(Ldisk);
    ui->lineEdit->setText(CharLdisk);
    //读待匹配图
    QString fileName = QFileDialog::getOpenFileName(NULL,tr("Open Image"),".",tr("Image Files (*.png *.jpg *.bmp)"));
    if(fileName.length()!=0){
        Image=imread(fileName.toStdString());
        //自适应窗口显示图像
        Mat ImageDisp;
        int resizeWidth;//显示宽度
        int resizeHeight;//显示高度

        switch(beta){
        case 0:
            {
                resizeWidth=Image.cols;
                resizeHeight=Image.rows;
            }break;
        case 1:
            {
                resizeWidth=(int)Image.cols/2;
                resizeHeight=(int)Image.rows/2;
            }break;
        case 2:
            {
                resizeWidth=(int)Image.cols/3;
                resizeHeight=(int)Image.rows/3;
            }break;
        case 3:
            {
                resizeWidth=(int)Image.cols/4;
                resizeHeight=(int)Image.rows/4;
            }break;
        case 4:
            {
                resizeWidth=(int)Image.cols/5;
                resizeHeight=(int)Image.rows/5;
            }break;
        case 5:
            {
                resizeWidth=(int)Image.cols/6;
                resizeHeight=(int)Image.rows/6;
            }break;
        case 6:
            {
                resizeWidth=(int)Image.cols/7;
                resizeHeight=(int)Image.rows/7;
            }break;
        case 7:
            {
                resizeWidth=(int)Image.cols/8;
                resizeHeight=(int)Image.rows/8;
            }break;
        case 8:
            {
                resizeWidth=(int)Image.cols/9;
                resizeHeight=(int)Image.rows/9;
            }break;
        case 9:
            {
                resizeWidth=(int)Image.cols/10;
                resizeHeight=(int)Image.rows/10;
            }break;
        case 10:
            {
                resizeWidth=(int)Image.cols/11;
                resizeHeight=(int)Image.rows/11;
            }break;
        default:
            {
                resizeWidth=(int)Image.cols/12;
                resizeHeight=(int)Image.rows/12;
            }break;
        }


        //resize函数前加cv：：，因为Qt也有resize函数，故加cv：：加以区别，不然会出现错误
        cv::resize(Image,ImageDisp,Size(resizeWidth,resizeHeight),0,0,3);
        Imgview->imshow(ImageDisp);//显示到绑定的控件上
        //cv::resize(Image,Image,Size(resizeWidth,resizeHeight),0,0,3);
        //Imgview->imshow(Image);//显示到绑定的控件上




       // for(int i=0;i<20;i++)
          //  qDebug("CenterPoint_array%d=%d",i,CenterPoint_array.at(i));


    }
}



//************模板匹配相关函数**************
//计算两点距离
float get_distance(Point Point1, Point Point2)
{
    float distance=0;
    distance=sqrt((Point1.x-Point2.x)*(Point1.x-Point2.x)+(Point1.y-Point2.y)*(Point1.y-Point2.y));

    return distance;

}

//计算环投影向量
void Anvector(Mat DiskImage,uint* Antable,int xnum,uint* aNumber,float* P)
{
    int i,j;
    int Length = DiskImage.rows;
    uchar* Imagedata;
    uint S[xnum]={0};//同个环上像素的和
    //求和
    for( i = 0; i < Length; i++)
    {
        Imagedata = DiskImage.ptr<uchar>(i);//获取行指针
        for ( j = 0; j < Length; j++)
        {
            if(Antable[i*Length+j]<xnum)
            {
                S[Antable[i*Length+j]] = S[Antable[i*Length+j]] + Imagedata[j];
            }
        }
    }
    //求平均，得到环投影向量
    for(i=0;i<xnum;i++)
    {
        P[i] = (float)S[i]/aNumber[i];
    }
}

//计算环投影向量,带方差输出
void Anvector2(Mat DiskImage,uint* Antable,int xnum,uint* aNumber,float* P,float* Pvariance)
{
    int i,j;
    int Length = DiskImage.rows;
    uchar* Imagedata;
    uint S[xnum]={0};//同个环上像素的和
    //求和
    for( i = 0; i < Length; i++)
    {
        Imagedata = DiskImage.ptr<uchar>(i);//获取行指针
        for ( j = 0; j < Length; j++)
        {
            if(Antable[i*Length+j]<xnum)
            {
                S[Antable[i*Length+j]] = S[Antable[i*Length+j]] + Imagedata[j];
            }
        }
    }
    //求平均，得到环投影向量
    for(i=0;i<xnum;i++)
    {
        P[i] = (float)S[i]/aNumber[i];
    }

    //求方差
    for( i = 0; i < Length; i++)
    {
        Imagedata = DiskImage.ptr<uchar>(i);//获取行指针
        for ( j = 0; j < Length; j++)
        {
            if(Antable[i*Length+j]<xnum)
            {
                Pvariance[Antable[i*Length+j]] = Pvariance[Antable[i*Length+j]]+(Imagedata[j]-P[Antable[i*Length+j]])*(Imagedata[j]-P[Antable[i*Length+j]]);
            }
        }
    }
}

//阶乘函数
int Multip(int a)
{
    int b=1;
    for(int i=1;i<a+1;i++)
    {
        b=b*i;
    }
    if(a==0)
        return 1;
    else
        return b;
}


//计算Zernike矩
//int N:阶数
void Zvector(Mat Img,int minLength,int N,uint* Rtable,uint* Atable,double* ZRtable,double* Z)
{
    int R=(int)minLength/2;
    int num_N=0;
    int n,m;
    int i,j;
    for(n=0;n<20;n++)
    {
        for(m=0;m<20;m++)
        {
            if((n>=m) && ((n-m)%2==0))
            {
                uchar* Imgdata;
                double Znms=0;//实部
                double Znmx=0;//虚部
                for(i=0;i<minLength;i++)
                {
                    Imgdata=Img.ptr<uchar>(i);
                    for(j=0;j<minLength;j++)
                    {
                        int r=Rtable[i*minLength+j];
                        float sita=(float)Atable[i*minLength+j]/180*3.14159;
                        if(r<R+1)
                        {
                            double Vnms=0;//实部
                            double Vnmx=0;//虚部

                            Vnms=ZRtable[num_N*(R+1)+r]*cos(m*sita);
                            Vnmx=-ZRtable[num_N*(R+1)+r]*sin(m*sita);

                            Znms=Znms+Imgdata[j]*Vnms;
                            Znmx=Znmx+Imgdata[j]*Vnmx;

                        }
                    }
                }

                Znms=Znms*(n+1)/3.14159;
                Znmx=Znmx*(n+1)/3.14159;
                Z[num_N]=sqrt(Znms*Znms+Znmx*Znmx);

                num_N++;
            }

            if(num_N==N)
                break;
        }
        if(num_N==N)
            break;
    }

}

//画有角度的旋转矩形
void DrawRotationRect(Mat img,Point start,int Width,int Height,double angle)
{  
    int thickness=2;
    int lineType=8;
    double cosAngle=(double)cos(angle/180*3.14159);
    double sinAngle=(double)sin(angle/180*3.14159);
    Point upR;
    Point upL;
    Point downR;
    Point downL;
    upR.x=(int)(Width*cosAngle+start.x);
    upR.y=(int)(-Width*sinAngle+start.y);

    upL.x=start.x;
    upL.y=start.y;

    downR.x=(int)(Width*cosAngle+Height*sinAngle+start.x);
    downR.y=(int)(-Width*sinAngle+Height*cosAngle+start.y);

    downL.x=(int)(Height*sinAngle+start.x);
    downL.y=(int)(Height*cosAngle+start.y);

    line(img,upL,upR,Scalar(255,255,255),thickness,lineType);
    line(img,upL,downL,Scalar(255,255,255),thickness,lineType);
    line(img,downL,downR,Scalar(255,255,255),thickness,lineType);
    line(img,upR,downR,Scalar(255,255,255),thickness,lineType);

}

//数组求平均
float mean_float(float* array,int length)
{
    int i;
    float m=0;
    for(i=0;i<length;i++)
    {
        m=m+array[i];
    }
    m=m/length;

    return(m);
}


//圆投影向量平方差匹配公式查找表
//输入：
//float* P:圆投影向量
//int R：圆投影向量半径
//uint* rNumber：半径r对应的像素数
//输出：
//float* Stable：圆投影向量平方差匹配公式查找表
void Ring_Stable(float* P,float Paver,int R,Mat* Stable)
{
    int i,j,r;

    float PP_Normalize=0;//归一化互相关系数
    for(r=0;r<R+1;r++)
    {
        PP_Normalize=PP_Normalize+(P[r]-Paver)*(P[r]-Paver);
    }
    PP_Normalize=sqrt((double)PP_Normalize);
    if(PP_Normalize==0)
        PP_Normalize=1;

    //Mat tempMat(256,256,CV_32FC1,Scalar(0));
    float* buffer;
    for(r=0;r<R+1;r++)
    {
        //tempMat.copyTo(Stable[r]);
        Stable[r]=Mat::zeros(Size(256,256),CV_32FC1);
        for(i=0;i<256;i++)
        {
            buffer=Stable[r].ptr<float>(i);
            for(j=0;j<256;j++)
            {

                float TT_Normalize=0;//归一化互相关系数
                for(int rr=0;rr<R+1;rr++)
                {
                    TT_Normalize=TT_Normalize+(i-j)*(i-j);
                }
                TT_Normalize=sqrt((double)TT_Normalize);
                if(TT_Normalize==0)
                    TT_Normalize=1;


                buffer[j]=buffer[j]+(float)((i-j)-(P[r]-Paver))*((i-j)-(P[r]-Paver))/PP_Normalize/TT_Normalize*2*3.14159*r;

            }
        }
    }
}


//极坐标角度计算0°-360°
float AnglePolar(Point A)
{
    float Angle;//旋转角
    int x=A.x;
    int y=-A.y;
    float YdivX=(float)y/x;//斜率
    if(x>=0 && y==0)
    {
        Angle=0;
    }
    else if(x<0 && y==0)
    {
        Angle=180;
    }
    else if(x==0 && y>=0)
    {
        Angle=90;
    }
    else if(x==0 && y<0)
    {
        Angle=270;
    }
    else if(x>0 && y>0)
    {
        Angle=(int)(atan(YdivX)*180/3.14159);
    }
    else if(x<0 && y>0)
    {
        Angle=(int)(-atan(-YdivX)*180/3.14159)+180;
    }
    else if(x<0 && y<0)
    {
        Angle=(int)(atan(YdivX)*180/3.14159)+180;
    }
    else if(x>0 && y<0)
    {
        Angle=(int)(-atan(-YdivX)*180/3.14159)+360;
    }

    return Angle;
}

//计算邻居数量
//返回值：PointNum为邻居数量
//输入值：
//int Sort,单个图像方块的编号
//Point* Points, 所有的图像方块编号
//int sizeofPoints, Points的大小
//vector<Point> DisksCenterPoints, 所有的图像方块中心点坐标
//float ThreshDis, 判定为邻居的距离阈值
//输出值：
//float* SumDistance, 距离偏差总和
int CountNeighbors(int Sort, Point* Points, int sizeofPoints, vector<Point> DisksCenterPoints, float ThreshDis)
{
    int PointNum=0;
    float distance=0;

    if(Points[Sort].x==0 && Points[Sort].y==0)
    {
        PointNum=0;
    }
    else
    {
        for(int i=0;i<sizeofPoints;i++)
        {
            if(i!=Sort)
            {
                if(Points[i].x==0 && Points[i].y==0)
                {
                    distance=999999999;
                }
                else
                {
                    distance=get_distance(Points[Sort],Points[i]);
                    float distemp=get_distance(DisksCenterPoints[Sort],DisksCenterPoints[i]);
                    distance=fabs(distance-distemp);//距离
                }

                if(distance<ThreshDis)
                {
                    PointNum++;
                }
            }
        }
    }

    return PointNum;
}

int CountNeighborsFast(int Sort, Point* Points, int sizeofPoints, float* DisksCenterPointsDistance, float ThreshDis)
{
    int PointNum=0;
    float distance=0;

    if(Points[Sort].x==0 && Points[Sort].y==0)
    {
        PointNum=0;
    }
    else
    {
        for(int i=0;i<sizeofPoints;i++)
        {
            if(i!=Sort)
            {
                if(Points[i].x==0 && Points[i].y==0)
                {
                    distance=999999999;
                }
                else
                {
                    distance=get_distance(Points[Sort],Points[i]);
                    float distemp=DisksCenterPointsDistance[Sort*sizeofPoints+i];
                    distance=fabs(distance-distemp);//距离
                }

                if(distance<ThreshDis)
                {
                    PointNum++;
                }
            }
        }
    }

    return PointNum;
}


//**********************************************************

//开始模板匹配
//Mat CV_resultImage;
//int MatchMethod=1;
void MBNS_tMatching(Mat Image,Mat TemplateImage, Point* MatchUpLPoint, float* MatchTime){

    int i,j;
    //一、图像和模板都转为单通道的灰度图
    Mat GrayImage;
    cvtColor(Image,GrayImage,CV_RGB2GRAY,1);
    Mat GrayTemplate;
    cvtColor(TemplateImage,GrayTemplate,CV_RGB2GRAY,1);
    qDebug("待匹配图宽高=【%d x %d】；模板图宽高=【%d x %d】",GrayImage.cols,GrayImage.rows,GrayTemplate.cols,GrayTemplate.rows);

    //二、将模板图分成多个Ldisk*Ldisk的小方块，每个方块圆心相距R
    R=(int)Ldisk/2;//局部方块半径
    int WidthNum=(int)GrayTemplate.cols/Ldisk*2 - 1;//一行的方块数
    int HeightNum=(int)GrayTemplate.rows/Ldisk*2 - 1;//一列的方块数
    qDebug("模板方块边长=%d",Ldisk);
    qDebug("模板方块数量=%d",WidthNum*HeightNum);

    //四（1）、建立环投影的环查找表
    int x[R]={0};//环距离
    x[0]=1;
    int xnum=1;
    for(i=0;i<R;i++)
    {
        x[i+1]=(int)(R-sqrt((R-x[i])*(R-x[i])-(2*R-1))+0.5);
        xnum++;
        if(x[i+1]<=x[i])
        {
            x[i+1]=0;
            xnum--;
            break;
        }
    }

    uint AnnulusTable[Ldisk*Ldisk];//环查找表
    uint anNumber[xnum]={0};//统计同个环的像素数量
    uint Rtable[Ldisk*Ldisk];//半径查找表
    uint rNumber[R+1]={0};//统计同个半径值的数量
    uint aNumber[360]={0};//统计同个角度值的数量
    uint Atable[Ldisk*Ldisk];//角度查找表
    double YdivX;//斜率
    int A=0;//角度
    int r;//半径

    for(i=0;i<Ldisk;i++)
    {
        for(j=0;j<Ldisk;j++)
        {
            r=(int)(sqrt((i-R)*(i-R)+(j-R)*(j-R)) + 0.5);

            //建立极坐标（半径和角度）查找表
            //角度转换
             if(j>R-1 && i==R)
             {
                 A=0;
             }
             else if(j<R && i==R)
             {
                 A=180;
             }
             else if(j==R && i<R)
             {
                 A=90;
             }
             else if(j==R && i>R)
             {
                 A=270;
             }
             else if(j>R && i<R)
             {
                 YdivX=(double)(R-i)/(j-R);
                 A=(int)(atan(YdivX)*180/3.14159);
             }
             else if(j<R && i<R)
             {
                 YdivX=(double)(R-j)/(R-i);
                 A=(int)(atan(YdivX)*180/3.14159)+90;
             }
             else if(j<R && i>R)
             {
                 YdivX=(double)(i-R)/(R-j);
                 A=(int)(atan(YdivX)*180/3.14159)+180;
             }
             else if(j>R && i>R)
             {
                 YdivX=(double)(j-R)/(i-R);
                 A=(int)(atan(YdivX)*180/3.14159)+270;
             }


             if(r>R)
             {
                 Rtable[i*Ldisk+j] = R+1;
                 Atable[i*Ldisk+j] = 361;
             }
             else
             {
                 Rtable[i*Ldisk+j] = r;
                 Atable[i*Ldisk+j] = A;
                 rNumber[r]++;
                 aNumber[A]++;
             }

            // **************************
            // **************************
            //建立环查找表
            if(r>R)
            {
                AnnulusTable[i*Ldisk+j] = xnum;
            }
            else
            {
                for(int ii=0;ii<xnum;ii++)
                {
                    if(r>(R-x[ii]))
                    {
                        AnnulusTable[i*Ldisk+j] = ii;
                        anNumber[ii]++;
                        break;
                    }
                }

                if(r<=(R-x[xnum-1]))
                {
                    AnnulusTable[i*Ldisk+j] = xnum-1;
                    anNumber[xnum-1]++;
                }
            }

        }
    }

    //四（2）、计算模板图的环投影向量
    vector<Mat> Disks;//方块
    vector<Point> DisksCenterPoints;//每个方块的中心坐标点
    int DiskNum=0;//真正的方块数量
    for(i=0;i<WidthNum*HeightNum;i++)
    {
        Mat GrayTemplatetemp(GrayTemplate(Rect(((int)i%WidthNum)*R,((int)i/WidthNum)*R,Ldisk,Ldisk)));
        float Ptemp[xnum]={0};
        Anvector(GrayTemplatetemp,AnnulusTable,xnum,anNumber,Ptemp);
        float PtempAver=0;
        for(j=0;j<xnum;j++)
        {
            PtempAver=PtempAver+Ptemp[j];
        }
        PtempAver=PtempAver/xnum;
        float P_SSD=0;
        for(j=0;j<xnum;j++)
        {
            P_SSD=P_SSD+(Ptemp[j]-PtempAver)*(Ptemp[j]-PtempAver);
        }

        if(P_SSD>0)
        {
            Disks.push_back(GrayTemplate(Rect(((int)i%WidthNum)*R,((int)i/WidthNum)*R,Ldisk,Ldisk)));
            DisksCenterPoints.push_back(Point(((int)i%WidthNum)*R+R,((int)i/WidthNum)*R+R));
            DiskNum++;
        }

    }
    qDebug("真正的方块数量= %d",DiskNum);

    //环投影向量
    float P[DiskNum][xnum]={0};//模板图环投影向量
    for(i=0;i<DiskNum;i++)
    {
        //环投影向量
        Anvector(Disks[i],AnnulusTable,xnum,anNumber,P[i]);

    }

    //*****五、开始匹配************************************
    //*******************************************
    double timeSum;//总耗时
    timeSum=static_cast<double>(getTickCount());
    //***********************************************************
    double timeAnnulus;
    timeAnnulus=static_cast<double>(getTickCount());
    //设置圆投影搜索范围
    int LeftUpx=0;
    int LeftUpy=0;
    int RrightDownx=(int)(GrayImage.cols-Ldisk-1);
    int RrightDowny=(int)(GrayImage.rows-Ldisk-1);

    //获取并标出候选点位置
    double RminValue=0;
    Point RminLoc;
    double RmaxValue=0;
    Point RmaxLoc;
    Mat GrayImageDisp;
    GrayImage.copyTo(GrayImageDisp);

    int CandidateNum=MatchingNum2;
    vector<Point> Points[DiskNum];//精确匹配点坐标数组
    vector<Mat> MatchingValue;//环投影相似度值
    for(int ii=0;ii<DiskNum;ii++)
    {
        Mat MatchingValueTemp(GrayImage.rows-Ldisk,GrayImage.cols-Ldisk,CV_32FC1,Scalar(999999999));//float型单通道，每个点初始化
        MatchingValue.push_back(MatchingValueTemp);
    }
    for(i=LeftUpy;i<RrightDowny+1;i++)
    {

        for(j=LeftUpx;j<RrightDownx;j++)
        {

            Mat ScanROI;//待匹配的子图
            ScanROI=GrayImage(Rect(j,i,Ldisk,Ldisk));
            float T[xnum]={0};//模板图环投影向量
            Anvector(ScanROI,AnnulusTable,xnum,anNumber,T);
            float T_Aver=0;
            for(int it=0;it<xnum;it++)
            {
                T_Aver=T_Aver+T[it];
            }
            T_Aver=T_Aver/xnum;
            float T_SSD=0;
            for(int it=0;it<xnum;it++)
            {
                T_SSD=T_SSD+(T[it]-T_Aver)*(T[it]-T_Aver);
            }

            if(T_SSD>4*xnum)
            {
                for(int ii=0;ii<DiskNum;ii++)
                {
                    float* buffer;
                    buffer=MatchingValue[ii].ptr<float>(i);
                    //相似度计算
                    int tempbuffer=0;
                    for(int k=0;k<xnum;k++)
                    {
                        tempbuffer=tempbuffer+(P[ii][k]-T[k])*(P[ii][k]-T[k]);

                    }
                    buffer[j]=tempbuffer;

                }
            }
        }
    }

    for(int ii=0;ii<DiskNum;ii++)
    {
        for(i=0;i<CandidateNum;i++)
        {
            //1、选取候选匹配点
            minMaxLoc(MatchingValue[ii],&RminValue,&RmaxValue,&RminLoc,&RmaxLoc);
            Point BestMatch=RminLoc;
            Point Centerpoint=Point(BestMatch.x+R,BestMatch.y+R);

            //画圆
            circle(GrayImageDisp,Centerpoint,2,CV_RGB(255,255,255),2,8,0);
            Points[ii].push_back(Centerpoint);

            float* buffervalue=MatchingValue[ii].ptr<float>(BestMatch.y);
            buffervalue[BestMatch.x]=999999999;
        }
    }
    timeAnnulus=((double)getTickCount() - timeAnnulus)/getTickFrequency();//运行时间
    qDebug("环投影匹配耗时：%f秒",timeAnnulus);


    double timeMBNS;//MBNS耗时
    timeMBNS=static_cast<double>(getTickCount());
    //***************************************************
    //Neighbors Constraint Similarity确定最终匹配点
    //***************************************************
    Point BestPoints[DiskNum];//最佳匹配点
    int BestPointNum[DiskNum]={0};//最佳匹配点邻居数量
    Thresh_distance=(float)(PSMode*Ldisk);//邻居距离阈值
    Mat ImageFinalResult;//显示结果
    GrayImage.copyTo(ImageFinalResult);

    for(int ii=0;ii<DiskNum;ii++)
    {

        for(i=0;i<Points[ii].size();i++)
        {     

            float tempMinDis=999999999;

            //第二多的邻居数量
            int PointNum=0;
            Point Pointtemp(0,0);
            Point Pointss[DiskNum];
            for(j=0;j<DiskNum;j++)
            {
                Pointss[j]=Point(0,0);
                if(j==ii)   Pointss[j]=Points[ii][i];
            }

            for(int jj=0;jj<DiskNum;jj++)
            {
                //获取邻居点坐标
                for(j=0;j<Points[jj].size();j++)
                {
                    if(jj!=ii)
                    {
                        float distance=get_distance(Points[ii][i],Points[jj][j]);
                        float distemp=get_distance(DisksCenterPoints[ii],DisksCenterPoints[jj]);
                        distance=fabs(distance-distemp);//距离
                        if(distance<tempMinDis)
                        {
                            tempMinDis=distance;
                            Pointtemp=Points[jj][j];
                        }
                    }

                }
                if(tempMinDis<Thresh_distance)
                {
                    Pointss[jj]=Pointtemp;
                    PointNum++;//第一多的邻居数量
                }
                tempMinDis=999999999;           

            }
            //阈值：第一多的邻居数量不少于Thresh_MBNSnum个
            if(PointNum>Thresh_MBNSnum)
            {
                PointNum=0;
                //计算第二多的邻居数量
                int PointNumtemp=0;
                for(int tt=0;tt<DiskNum;tt++)
                {
                    if(tt!=ii)
                    {
                        PointNumtemp=CountNeighbors(tt, Pointss, DiskNum,DisksCenterPoints, Thresh_distance);
                        if(PointNum<PointNumtemp)
                        {
                            PointNum=PointNumtemp;//第二多的邻居数量
                        }
                    }
                }

                //获取最佳点坐标
                if(PointNum>BestPointNum[ii])
                {
                    BestPoints[ii]=Points[ii][i];
                    BestPointNum[ii]=PointNum;
                }

            }

        }
        if(BestPointNum[ii]<Thresh_MBNSnum)  BestPoints[ii]=Point(0,0);//阈值：第二多邻居数量不少于Thresh_MBNSnum


    }

    //****从精确匹配得到的WidthNum*HeightNum个点中确定最终匹配点****
    Point FirstBestPoint=BestPoints[0];
    int FirstBestPointNum=0;
    int FirstBestPointSort=0;
    for(i=0;i<DiskNum;i++)
    {
        int PointNum=0;

        PointNum=CountNeighbors(i, BestPoints, DiskNum,DisksCenterPoints, Thresh_distance);//Thresh_distance=R
        //获取最佳点
        if(PointNum>FirstBestPointNum)
        {
            FirstBestPoint=BestPoints[i];
            FirstBestPointNum=PointNum;
            FirstBestPointSort=i;
        }


    }

    //剔除错误点，与最佳匹配点是邻居的点认为是正确点
    Point RectUplPoint=(FirstBestPoint-DisksCenterPoints[FirstBestPointSort]);//矩形左上角坐标
    int rectupnum=1;
    for(i=0;i<DiskNum;i++)
    {
        if(BestPoints[i].x==0 && BestPoints[i].y==0)
        {
            BestPoints[i]=Point(0,0);
        }
        else
        {
            if(FirstBestPointSort!=i)
            {
                float distance=get_distance(FirstBestPoint,BestPoints[i]);
                float distemp=get_distance(DisksCenterPoints[FirstBestPointSort],DisksCenterPoints[i]);
                distance=fabs(distance-distemp);//距离
                if(distance>Thresh_distance)
                {
                    BestPoints[i]=Point(0,0);
                }
                else
                {
                    RectUplPoint=RectUplPoint+(BestPoints[i]-DisksCenterPoints[i]);
                    rectupnum++;
                }
            }
        }


        circle(ImageFinalResult,BestPoints[i],2,CV_RGB(255,255,255),2,8,0);
    }

    timeMBNS=((double)getTickCount() - timeMBNS)/getTickFrequency();//运行时间
    qDebug("MBNS耗时：%f秒",timeMBNS);
    Mat ImageFinalResult2;
    GrayImage.copyTo(ImageFinalResult2);
    circle(ImageFinalResult2,FirstBestPoint,2,CV_RGB(255,255,255),2,8,0);


    RectUplPoint.x=(int)RectUplPoint.x/rectupnum;
    RectUplPoint.y=(int)RectUplPoint.y/rectupnum;
    DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)0);


/*
    //利用2个最佳匹配点画矩形框
    //计算两向量夹角
    float AvectorAngle=AnglePolar(DisksCenterPoints[SecondBestPointSort]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
    float BvectorAngle=AnglePolar(SecondBestPoint-FirstBestPoint);//实际向量
    //两向量夹角0-360°
    float Sita=BvectorAngle-AvectorAngle;
    if(Sita<0) Sita=Sita+360;


    Point RectUplPoint;//矩形左上角坐标
    RectUplPoint.x=(int)(-DisksCenterPoints[FirstBestPointSort].x*cos(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*sin(Sita/180*3.14) + FirstBestPoint.x);
    RectUplPoint.y=(int)(-(-DisksCenterPoints[FirstBestPointSort].x)*sin(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*cos(Sita/180*3.14)  + FirstBestPoint.y);
    DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)Sita);
*/
/*
    //显示结果
    qDebug("最终匹配的矩形左上角坐标=(%d,%d);旋转角=%f°",RectUplPoint.x,RectUplPoint.y,0);
    imshow("GrayImageDisp",GrayImageDisp);//局部特征粗匹配得到候选匹配点集
    imshow("ImageFinalResult",ImageFinalResult);//Most Neighbors Similarity: 获取9个最佳匹配点
    imshow("ImageFinalResult2",ImageFinalResult2);//Most Neighbors Similarity: 从9个匹配点中获取最佳的2个匹配点
*/
    //***********************************************************************
    //***********************************************************************
    timeSum=((double)getTickCount() - timeSum)/getTickFrequency();//运行时间
    qDebug("模板匹配结束，总耗时：%f秒",timeSum);
    qDebug("***********************");

/*
    Mat srcImageDisp;
    //resize函数前加cv：：，因为Qt也有resize函数，故加cv：：加以区别，不然会出现错误
    int resizeW=(int)ImageFinalResult2.cols/(beta+1);
    int resizeH=(int)ImageFinalResult2.rows/(beta+1);
    cv::resize(ImageFinalResult2,srcImageDisp,Size(resizeW,resizeH),0,0,3);
    Imgview->imshow(srcImageDisp);//显示到绑定的控件上

*/

    //返回结果参数
    MatchUpLPoint[0]=RectUplPoint;
    MatchTime[0]=timeSum;

}

void MNS_SSDtMatching(Mat Image,Mat TemplateImage, Point* MatchUpLPoint, float* MatchTime){

    int i,j;
    //一、图像和模板都转为单通道的灰度图
    Mat GrayImage;
    cvtColor(Image,GrayImage,CV_RGB2GRAY,1);
    Mat GrayTemplate;
    cvtColor(TemplateImage,GrayTemplate,CV_RGB2GRAY,1);
    qDebug("待匹配图宽高=【%d x %d】；模板图宽高=【%d x %d】",GrayImage.cols,GrayImage.rows,GrayTemplate.cols,GrayTemplate.rows);

    //二、将模板图分成多个Ldisk*Ldisk的小方块，每个方块圆心相距
    R=(int)Ldisk/2;//局部方块半径
    int tt=2;//初始距离倍数
    int disL=(int)Ldisk/tt;//每个方块圆心相距的距离
    int WidthNum=(int)GrayTemplate.cols/Ldisk*tt - tt;//一行的方块数
    int HeightNum=(int)GrayTemplate.rows/Ldisk*tt - tt;//一列的方块数
    if(Ldisk==9 || WidthNum*HeightNum<70)
    {
        while(WidthNum*HeightNum<70)
        {
            tt++;
            disL=(int)(Ldisk/tt);//每个方块圆心相距的距离
            WidthNum=(int)GrayTemplate.cols/Ldisk*tt - tt;//一行的方块数
            HeightNum=(int)GrayTemplate.rows/Ldisk*tt - tt;//一列的方块数
        }
    }
    Thresh_distance=(float)(PSMode*Ldisk/tt);//邻居距离阈值
    qDebug("邻居距离阈值=%f",Thresh_distance);
    qDebug("模板方块边长=%d",Ldisk);
    qDebug("模板方块数量=%d",WidthNum*HeightNum);

    //四（1）、建立环投影的环查找表
    int x[R]={0};//环距离
    x[0]=1;
    int xnum=1;
    for(i=0;i<R;i++)
    {
        x[i+1]=(int)(R-sqrt((R-x[i])*(R-x[i])-(2*R-1))+0.5);
        xnum++;
        if(x[i+1]<=x[i])
        {
            x[i+1]=0;
            xnum--;
            break;
        }
    }

    uint AnnulusTable[Ldisk*Ldisk];//环查找表
    uint anNumber[xnum]={0};//统计同个环的像素数量
    uint Rtable[Ldisk*Ldisk];//半径查找表
    uint rNumber[R+1]={0};//统计同个半径值的数量
    uint aNumber[360]={0};//统计同个角度值的数量
    uint Atable[Ldisk*Ldisk];//角度查找表
    double YdivX;//斜率
    int A=0;//角度
    int r;//半径

    for(i=0;i<Ldisk;i++)
    {
        for(j=0;j<Ldisk;j++)
        {
            r=(int)(sqrt((i-R)*(i-R)+(j-R)*(j-R)) + 0.5);

            //建立极坐标（半径和角度）查找表
            //角度转换
             if(j>R-1 && i==R)
             {
                 A=0;
             }
             else if(j<R && i==R)
             {
                 A=180;
             }
             else if(j==R && i<R)
             {
                 A=90;
             }
             else if(j==R && i>R)
             {
                 A=270;
             }
             else if(j>R && i<R)
             {
                 YdivX=(double)(R-i)/(j-R);
                 A=(int)(atan(YdivX)*180/3.14159);
             }
             else if(j<R && i<R)
             {
                 YdivX=(double)(R-j)/(R-i);
                 A=(int)(atan(YdivX)*180/3.14159)+90;
             }
             else if(j<R && i>R)
             {
                 YdivX=(double)(i-R)/(R-j);
                 A=(int)(atan(YdivX)*180/3.14159)+180;
             }
             else if(j>R && i>R)
             {
                 YdivX=(double)(j-R)/(i-R);
                 A=(int)(atan(YdivX)*180/3.14159)+270;
             }


             if(r>R)
             {
                 Rtable[i*Ldisk+j] = R+1;
                 Atable[i*Ldisk+j] = 361;
             }
             else
             {
                 Rtable[i*Ldisk+j] = r;
                 Atable[i*Ldisk+j] = A;
                 rNumber[r]++;
                 aNumber[A]++;
             }

            // **************************
            // **************************
            //建立环查找表
            if(r>R)
            {
                AnnulusTable[i*Ldisk+j] = xnum;
            }
            else
            {
                for(int ii=0;ii<xnum;ii++)
                {
                    if(r>(R-x[ii]))
                    {
                        AnnulusTable[i*Ldisk+j] = ii;
                        anNumber[ii]++;
                        break;
                    }
                }

                if(r<=(R-x[xnum-1]))
                {
                    AnnulusTable[i*Ldisk+j] = xnum-1;
                    anNumber[xnum-1]++;
                }
            }

        }
    }

    //四（2）、计算模板图的环投影向量
    vector<Mat> Disks;//方块
    vector<Point> DisksCenterPoints;//每个方块的中心坐标点
    int DiskNum=0;//真正的方块数量
    for(i=0;i<WidthNum*HeightNum;i++)
    {
        Mat GrayTemplatetemp(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
        float Ptemp[xnum]={0};
        Anvector(GrayTemplatetemp,AnnulusTable,xnum,anNumber,Ptemp);
        float PtempAver=0;
        for(j=0;j<xnum;j++)
        {
            PtempAver=PtempAver+Ptemp[j];
        }
        PtempAver=PtempAver/xnum;
        float P_SSD=0;
        for(j=0;j<xnum;j++)
        {
            P_SSD=P_SSD+(Ptemp[j]-PtempAver)*(Ptemp[j]-PtempAver);
        }

        if(P_SSD>4*xnum)
        {
            Disks.push_back(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
            DisksCenterPoints.push_back(Point(((int)i%WidthNum)*disL+R,((int)i/WidthNum)*disL+R));
            DiskNum++;
        }

    }
    qDebug("真正的方块数量= %d",DiskNum);

    //DiskNum<=10的话直接放弃匹配
    if(DiskNum>10)
    {
        //环投影向量
        float P[DiskNum][xnum]={0};//模板图环投影向量
        for(i=0;i<DiskNum;i++)
        {
            //环投影向量
            Anvector(Disks[i],AnnulusTable,xnum,anNumber,P[i]);

        }

        //*****五、开始匹配************************************
        //*******************************************
        double timeSum;//总耗时
        timeSum=static_cast<double>(getTickCount());
        //***********************************************************
        double timeAnnulus;
        timeAnnulus=static_cast<double>(getTickCount());
        //设置圆投影搜索范围
        int LeftUpx=0;
        int LeftUpy=0;
        int RrightDownx=(int)(GrayImage.cols-Ldisk-1);
        int RrightDowny=(int)(GrayImage.rows-Ldisk-1);

        //获取并标出候选点位置
        double RminValue=0;
        Point RminLoc;
        double RmaxValue=0;
        Point RmaxLoc;
        Mat GrayImageDisp;
        GrayImage.copyTo(GrayImageDisp);

        int CandidateNum=MatchingNum2;
        vector<Point> Points[DiskNum];//精确匹配点坐标数组
        vector<Mat> MatchingValue;//环投影相似度值
        for(int ii=0;ii<DiskNum;ii++)
        {
            Mat MatchingValueTemp(GrayImage.rows-Ldisk,GrayImage.cols-Ldisk,CV_32FC1,Scalar(999999999));//float型单通道，每个点初始化
            MatchingValue.push_back(MatchingValueTemp);
        }
        for(i=LeftUpy;i<RrightDowny+1;i++)
        {

            for(j=LeftUpx;j<RrightDownx;j++)
            {

                Mat ScanROI;//待匹配的子图
                ScanROI=GrayImage(Rect(j,i,Ldisk,Ldisk));
                float T[xnum]={0};//模板图环投影向量
                Anvector(ScanROI,AnnulusTable,xnum,anNumber,T);
                float T_Aver=0;
                for(int it=0;it<xnum;it++)
                {
                    T_Aver=T_Aver+T[it];
                }
                T_Aver=T_Aver/xnum;
                float T_SSD=0;
                for(int it=0;it<xnum;it++)
                {
                    T_SSD=T_SSD+(T[it]-T_Aver)*(T[it]-T_Aver);
                }

                if(T_SSD>4*xnum)
                {
                    for(int ii=0;ii<DiskNum;ii++)
                    {
                        float* buffer;
                        buffer=MatchingValue[ii].ptr<float>(i);
                        //相似度计算
                        int tempbuffer=0;
                        for(int k=0;k<xnum;k++)
                        {
                            tempbuffer=tempbuffer+(P[ii][k]-T[k])*(P[ii][k]-T[k]);

                        }
                        buffer[j]=tempbuffer;

                    }
                }
            }
        }

        for(int ii=0;ii<DiskNum;ii++)
        {
            for(i=0;i<CandidateNum;i++)
            {
                //1、选取候选匹配点
                minMaxLoc(MatchingValue[ii],&RminValue,&RmaxValue,&RminLoc,&RmaxLoc);
                Point BestMatch=RminLoc;
                Point Centerpoint=Point(BestMatch.x+R,BestMatch.y+R);

                //画圆
                circle(GrayImageDisp,Centerpoint,2,CV_RGB(255,255,255),2,8,0);
                Points[ii].push_back(Centerpoint);

                float* buffervalue=MatchingValue[ii].ptr<float>(BestMatch.y);
                buffervalue[BestMatch.x]=999999999;
            }
        }
        timeAnnulus=((double)getTickCount() - timeAnnulus)/getTickFrequency();//运行时间
        //qDebug("环投影匹配耗时：%f秒",timeAnnulus);


        double timeMNS;//MNS耗时
        timeMNS=static_cast<double>(getTickCount());
        //***************************************************
        //Neighbors Constraint Similarity确定最终匹配点
        //***************************************************
        Point BestPoints[DiskNum];//最佳匹配点
        int BestPointNum[DiskNum]={0};//最佳匹配点邻居数量
        Mat ImageFinalResult;//显示结果
        GrayImage.copyTo(ImageFinalResult);

        for(int ii=0;ii<DiskNum;ii++)
        {

            for(i=0;i<Points[ii].size();i++)
            {

                //邻居数量
                int PointNum=0;
                for(int jj=0;jj<DiskNum;jj++)
                {
                    //获取邻居点坐标
                    for(j=0;j<Points[jj].size();j++)
                    {
                        if(jj!=ii)
                        {
                            float distance=get_distance(Points[ii][i],Points[jj][j]);
                            float distemp=get_distance(DisksCenterPoints[ii],DisksCenterPoints[jj]);
                            distance=fabs(distance-distemp);
                            if(distance<Thresh_distance)
                            {
                                PointNum++;//第一多的邻居数量
                                break;
                            }
                        }

                    }

                    //获取最佳点坐标
                    if(PointNum>BestPointNum[ii])
                    {
                        BestPoints[ii]=Points[ii][i];
                        BestPointNum[ii]=PointNum;
                    }
                }

            }
            if(BestPointNum[ii]<Thresh_MBNSnum)  BestPoints[ii]=Point(0,0);//阈值：最多邻居数量不少于Thresh_MBNSnum

        }


        //****从精确匹配得到的WidthNum*HeightNum个点中确定最终匹配点****
        Point FirstBestPoint=BestPoints[0];
        int FirstBestPointNum=0;
        int FirstBestPointSort=0;

        for(i=0;i<DiskNum;i++)
        {
            int PointNum=0;

            PointNum=CountNeighbors(i, BestPoints, DiskNum,DisksCenterPoints, Thresh_distance);//Thresh_distance=R
            //获取最佳点
            if(PointNum>FirstBestPointNum)
            {
                FirstBestPoint=BestPoints[i];
                FirstBestPointNum=PointNum;
                FirstBestPointSort=i;
            }

        }

        //剔除错误点，与最佳匹配点是邻居的点认为是正确点
        Point RectUplPoint=(FirstBestPoint-DisksCenterPoints[FirstBestPointSort]);//矩形左上角坐标
        int rectupnum=1;
        for(i=0;i<DiskNum;i++)
        {
            if(BestPoints[i].x==0 && BestPoints[i].y==0)
            {
                BestPoints[i]=Point(0,0);
            }
            else
            {
                if(FirstBestPointSort!=i)
                {
                    float distance=get_distance(FirstBestPoint,BestPoints[i]);
                    float distemp=get_distance(DisksCenterPoints[FirstBestPointSort],DisksCenterPoints[i]);
                    distance=fabs(distance-distemp);//距离
                    if(distance>Thresh_distance)
                    {
                        BestPoints[i]=Point(0,0);
                    }
                    else
                    {
                        RectUplPoint=RectUplPoint+(BestPoints[i]-DisksCenterPoints[i]);
                        rectupnum++;
                    }
                }
            }


            circle(ImageFinalResult,BestPoints[i],2,CV_RGB(255,255,255),2,8,0);
        }

        timeMNS=((double)getTickCount() - timeMNS)/getTickFrequency();//运行时间
        qDebug("MNS_SSD耗时：%f秒",timeMNS);
        Mat ImageFinalResult2;
        GrayImage.copyTo(ImageFinalResult2);
        circle(ImageFinalResult2,FirstBestPoint,2,CV_RGB(255,255,255),2,8,0);

        RectUplPoint.x=(int)RectUplPoint.x/rectupnum;
        RectUplPoint.y=(int)RectUplPoint.y/rectupnum;
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)0);


    /*
        //利用2个最佳匹配点画矩形框
        //计算两向量夹角
        float AvectorAngle=AnglePolar(DisksCenterPoints[SecondBestPointSort]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
        float BvectorAngle=AnglePolar(SecondBestPoint-FirstBestPoint);//实际向量
        //两向量夹角0-360°
        float Sita=BvectorAngle-AvectorAngle;
        if(Sita<0) Sita=Sita+360;


        Point RectUplPoint;//矩形左上角坐标
        RectUplPoint.x=(int)(-DisksCenterPoints[FirstBestPointSort].x*cos(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*sin(Sita/180*3.14) + FirstBestPoint.x);
        RectUplPoint.y=(int)(-(-DisksCenterPoints[FirstBestPointSort].x)*sin(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*cos(Sita/180*3.14)  + FirstBestPoint.y);
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)Sita);
    */

        imshow("GrayImageDisp",GrayImageDisp);//局部特征粗匹配得到候选匹配点集
        imshow("ImageFinalResult",ImageFinalResult);//Most Neighbors Similarity: 获取9个最佳匹配点
        imshow("ImageFinalResult2",ImageFinalResult2);//Most Neighbors Similarity: 从9个匹配点中获取最佳的2个匹配点

        //***********************************************************************
        //***********************************************************************
        timeSum=((double)getTickCount() - timeSum)/getTickFrequency();//运行时间
        qDebug("模板匹配结束，总耗时：%f秒",timeSum);
        qDebug("***********************");


        //返回结果参数
        MatchUpLPoint[0]=RectUplPoint;
        MatchTime[0]=timeSum;
    }
    else
    {
        //返回结果参数
        MatchUpLPoint[0]=Point(0,0);
        MatchTime[0]=0;
    }

}

void MNS_NCCtMatching(Mat Image,Mat TemplateImage, Point* MatchUpLPoint, float* MatchTime){

    int i,j;
    //一、图像和模板都转为单通道的灰度图
    Mat GrayImage;
    cvtColor(Image,GrayImage,CV_RGB2GRAY,1);
    Mat GrayTemplate;
    cvtColor(TemplateImage,GrayTemplate,CV_RGB2GRAY,1);
    qDebug("待匹配图宽高=【%d x %d】；模板图宽高=【%d x %d】",GrayImage.cols,GrayImage.rows,GrayTemplate.cols,GrayTemplate.rows);

    //二、将模板图分成多个Ldisk*Ldisk的小方块，每个方块圆心相距
    R=(int)Ldisk/2;//局部方块半径
    int tt=2;//初始距离倍数
    int disL=(int)Ldisk/tt;//每个方块圆心相距的距离
    int WidthNum=(int)GrayTemplate.cols/Ldisk*tt - tt;//一行的方块数
    int HeightNum=(int)GrayTemplate.rows/Ldisk*tt - tt;//一列的方块数
    if(Ldisk==9 || WidthNum*HeightNum<70)
    {
        while(WidthNum*HeightNum<70)
        {
            tt++;
            disL=(int)(Ldisk/tt);//每个方块圆心相距的距离
            WidthNum=(int)GrayTemplate.cols/Ldisk*tt - tt;//一行的方块数
            HeightNum=(int)GrayTemplate.rows/Ldisk*tt - tt;//一列的方块数
        }
    }
    Thresh_distance=(float)(PSMode*Ldisk/tt);//邻居距离阈值
    qDebug("邻居距离阈值=%f",Thresh_distance);
    qDebug("模板方块边长=%d",Ldisk);
    qDebug("模板方块数量=%d",WidthNum*HeightNum);

    //四（1）、建立环投影的环查找表
    int x[R]={0};//环距离
    x[0]=1;
    int xnum=1;
    for(i=0;i<R;i++)
    {
        x[i+1]=(int)(R-sqrt((R-x[i])*(R-x[i])-(2*R-1))+0.5);
        xnum++;
        if(x[i+1]<=x[i])
        {
            x[i+1]=0;
            xnum--;
            break;
        }
    }

    uint AnnulusTable[Ldisk*Ldisk];//环查找表
    uint anNumber[xnum]={0};//统计同个环的像素数量
    uint Rtable[Ldisk*Ldisk];//半径查找表
    uint rNumber[R+1]={0};//统计同个半径值的数量
    uint aNumber[360]={0};//统计同个角度值的数量
    uint Atable[Ldisk*Ldisk];//角度查找表
    double YdivX;//斜率
    int A=0;//角度
    int r;//半径

    for(i=0;i<Ldisk;i++)
    {
        for(j=0;j<Ldisk;j++)
        {
            r=(int)(sqrt((i-R)*(i-R)+(j-R)*(j-R)) + 0.5);

            //建立极坐标（半径和角度）查找表
            //角度转换
             if(j>R-1 && i==R)
             {
                 A=0;
             }
             else if(j<R && i==R)
             {
                 A=180;
             }
             else if(j==R && i<R)
             {
                 A=90;
             }
             else if(j==R && i>R)
             {
                 A=270;
             }
             else if(j>R && i<R)
             {
                 YdivX=(double)(R-i)/(j-R);
                 A=(int)(atan(YdivX)*180/3.14159);
             }
             else if(j<R && i<R)
             {
                 YdivX=(double)(R-j)/(R-i);
                 A=(int)(atan(YdivX)*180/3.14159)+90;
             }
             else if(j<R && i>R)
             {
                 YdivX=(double)(i-R)/(R-j);
                 A=(int)(atan(YdivX)*180/3.14159)+180;
             }
             else if(j>R && i>R)
             {
                 YdivX=(double)(j-R)/(i-R);
                 A=(int)(atan(YdivX)*180/3.14159)+270;
             }


             if(r>R)
             {
                 Rtable[i*Ldisk+j] = R+1;
                 Atable[i*Ldisk+j] = 361;
             }
             else
             {
                 Rtable[i*Ldisk+j] = r;
                 Atable[i*Ldisk+j] = A;
                 rNumber[r]++;
                 aNumber[A]++;
             }

            // **************************
            // **************************
            //建立环查找表
            if(r>R)
            {
                AnnulusTable[i*Ldisk+j] = xnum;
            }
            else
            {
                for(int ii=0;ii<xnum;ii++)
                {
                    if(r>(R-x[ii]))
                    {
                        AnnulusTable[i*Ldisk+j] = ii;
                        anNumber[ii]++;
                        break;
                    }
                }

                if(r<=(R-x[xnum-1]))
                {
                    AnnulusTable[i*Ldisk+j] = xnum-1;
                    anNumber[xnum-1]++;
                }
            }

        }
    }

    //四（2）、计算模板图的环投影向量
    vector<Mat> Disks;//方块
    vector<Point> DisksCenterPoints;//每个方块的中心坐标点
    int DiskNum=0;//真正的方块数量
    for(i=0;i<WidthNum*HeightNum;i++)
    {
        Mat GrayTemplatetemp(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
        float Ptemp[xnum]={0};
        Anvector(GrayTemplatetemp,AnnulusTable,xnum,anNumber,Ptemp);
        float PtempAver=0;
        for(j=0;j<xnum;j++)
        {
            PtempAver=PtempAver+Ptemp[j];
        }
        PtempAver=PtempAver/xnum;
        float P_SSD=0;
        for(j=0;j<xnum;j++)
        {
            P_SSD=P_SSD+(Ptemp[j]-PtempAver)*(Ptemp[j]-PtempAver);
        }

        if(P_SSD>=0)
        {
            Disks.push_back(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
            DisksCenterPoints.push_back(Point(((int)i%WidthNum)*disL+R,((int)i/WidthNum)*disL+R));
            DiskNum++;
        }

    }
    qDebug("真正的方块数量= %d",DiskNum);

    //环投影向量
    float P[DiskNum][xnum]={0};//模板图环投影向量
    float P_aver[DiskNum]={0};//模板图环投影向量均值
    float P_normalize[DiskNum]={0};//模板图环投影向量归一化参数
    for(i=0;i<DiskNum;i++)
    {
        //环投影向量
        Anvector(Disks[i],AnnulusTable,xnum,anNumber,P[i]);
        //求均值
        for(int iaver=0;iaver<xnum;iaver++)
        {
            P_aver[i]=P_aver[i]+P[i][iaver];
        }
        P_aver[i]=P_aver[i]/xnum;
        //求归一化参数
        for(int iaver=0;iaver<xnum;iaver++)
        {
            P_normalize[i]=P_normalize[i]+(P[i][iaver]-P_aver[i])*(P[i][iaver]-P_aver[i]);
        }
        P_normalize[i]=sqrt(P_normalize[i]);
        if(P_normalize[i]==0) P_normalize[i]=1;

    }

    //*****五、开始匹配************************************
    //*******************************************
    double timeSum;//总耗时
    timeSum=static_cast<double>(getTickCount());
    //***********************************************************
    double timeAnnulus;
    timeAnnulus=static_cast<double>(getTickCount());
    //设置圆投影搜索范围
    int LeftUpx=0;
    int LeftUpy=0;
    int RrightDownx=(int)(GrayImage.cols-Ldisk-1);
    int RrightDowny=(int)(GrayImage.rows-Ldisk-1);

    //获取并标出候选点位置
    double RminValue=0;
    Point RminLoc;
    double RmaxValue=0;
    Point RmaxLoc;
    Mat GrayImageDisp;
    GrayImage.copyTo(GrayImageDisp);

    int CandidateNum=MatchingNum2;
    vector<Point> Points[DiskNum];//精确匹配点坐标数组
    vector<Mat> MatchingValue;//环投影相似度值
    for(int ii=0;ii<DiskNum;ii++)
    {
        Mat MatchingValueTemp(GrayImage.rows-Ldisk,GrayImage.cols-Ldisk,CV_32FC1,Scalar(0));//float型单通道，每个点初始化
        MatchingValue.push_back(MatchingValueTemp);
    }
    for(i=LeftUpy;i<RrightDowny+1;i++)
    {

        for(j=LeftUpx;j<RrightDownx;j++)
        {

            Mat ScanROI;//待匹配的子图
            ScanROI=GrayImage(Rect(j,i,Ldisk,Ldisk));
            float T[xnum]={0};//模板图环投影向量
            Anvector(ScanROI,AnnulusTable,xnum,anNumber,T);
            //求均值
            float T_Aver=0;
            for(int it=0;it<xnum;it++)
            {
                T_Aver=T_Aver+T[it];
            }
            T_Aver=T_Aver/xnum;
            //求归一化参数
            float T_SSD=0;
            for(int it=0;it<xnum;it++)
            {
                T_SSD=T_SSD+(T[it]-T_Aver)*(T[it]-T_Aver);
            }
            float T_normalize=sqrt(T_SSD);
            if(T_normalize==0) T_normalize=1;

            if(T_SSD>=0)
            {
                for(int ii=0;ii<DiskNum;ii++)
                {
                    float* buffer;
                    buffer=MatchingValue[ii].ptr<float>(i);
                    //相似度计算
                    int tempbuffer=0;
                    for(int k=0;k<xnum;k++)
                    {
                        tempbuffer=tempbuffer+(P[ii][k]-P_aver[ii])*(T[k]-T_Aver);

                    }
                    buffer[j]=tempbuffer/P_normalize[ii]/T_normalize;

                }
            }
        }
    }

    for(int ii=0;ii<DiskNum;ii++)
    {
        for(i=0;i<CandidateNum;i++)
        {
            //1、选取候选匹配点
            minMaxLoc(MatchingValue[ii],&RminValue,&RmaxValue,&RminLoc,&RmaxLoc);
            Point BestMatch=RmaxLoc;
            Point Centerpoint=Point(BestMatch.x+R,BestMatch.y+R);

            //画圆
            circle(GrayImageDisp,Centerpoint,2,CV_RGB(255,255,255),2,8,0);
            Points[ii].push_back(Centerpoint);

            float* buffervalue=MatchingValue[ii].ptr<float>(BestMatch.y);
            buffervalue[BestMatch.x]=0;
        }
    }
    timeAnnulus=((double)getTickCount() - timeAnnulus)/getTickFrequency();//运行时间
    //qDebug("环投影匹配耗时：%f秒",timeAnnulus);


    double timeMNS;//MNS耗时
    timeMNS=static_cast<double>(getTickCount());
    //***************************************************
    //Neighbors Constraint Similarity确定最终匹配点
    //***************************************************
    Point BestPoints[DiskNum];//最佳匹配点
    int BestPointNum[DiskNum]={0};//最佳匹配点邻居数量
    Mat ImageFinalResult;//显示结果
    GrayImage.copyTo(ImageFinalResult);

    for(int ii=0;ii<DiskNum;ii++)
    {

        for(i=0;i<Points[ii].size();i++)
        {

            //邻居数量
            int PointNum=0;
            for(int jj=0;jj<DiskNum;jj++)
            {
                //获取邻居点坐标
                for(j=0;j<Points[jj].size();j++)
                {
                    if(jj!=ii)
                    {
                        float distance=get_distance(Points[ii][i],Points[jj][j]);
                        float distemp=get_distance(DisksCenterPoints[ii],DisksCenterPoints[jj]);
                        distance=fabs(distance-distemp);
                        if(distance<Thresh_distance)
                        {
                            PointNum++;//第一多的邻居数量
                            break;
                        }
                    }

                }

                //获取最佳点坐标
                if(PointNum>BestPointNum[ii])
                {
                    BestPoints[ii]=Points[ii][i];
                    BestPointNum[ii]=PointNum;
                }
            }

        }
        if(BestPointNum[ii]<Thresh_MBNSnum)  BestPoints[ii]=Point(0,0);//阈值：最多邻居数量不少于Thresh_MBNSnum

    }


    //****从精确匹配得到的WidthNum*HeightNum个点中确定最终匹配点****
    Point FirstBestPoint=BestPoints[0];
    int FirstBestPointNum=0;
    int FirstBestPointSort=0;

    for(i=0;i<DiskNum;i++)
    {
        int PointNum=0;

        PointNum=CountNeighbors(i, BestPoints, DiskNum,DisksCenterPoints, Thresh_distance);//Thresh_distance=R
        //获取最佳点
        if(PointNum>FirstBestPointNum)
        {
            FirstBestPoint=BestPoints[i];
            FirstBestPointNum=PointNum;
            FirstBestPointSort=i;
        }

    }

    //剔除错误点，与最佳匹配点是邻居的点认为是正确点
    Point RectUplPoint=(FirstBestPoint-DisksCenterPoints[FirstBestPointSort]);//矩形左上角坐标
    int rectupnum=1;
    for(i=0;i<DiskNum;i++)
    {
        if(BestPoints[i].x==0 && BestPoints[i].y==0)
        {
            BestPoints[i]=Point(0,0);
        }
        else
        {
            if(FirstBestPointSort!=i)
            {
                float distance=get_distance(FirstBestPoint,BestPoints[i]);
                float distemp=get_distance(DisksCenterPoints[FirstBestPointSort],DisksCenterPoints[i]);
                distance=fabs(distance-distemp);//距离
                if(distance>Thresh_distance)
                {
                    BestPoints[i]=Point(0,0);
                }
                else
                {
                    RectUplPoint=RectUplPoint+(BestPoints[i]-DisksCenterPoints[i]);
                    rectupnum++;
                }
            }
        }


        circle(ImageFinalResult,BestPoints[i],2,CV_RGB(255,255,255),2,8,0);
    }

    timeMNS=((double)getTickCount() - timeMNS)/getTickFrequency();//运行时间
    qDebug("MNS_NCC耗时：%f秒",timeMNS);
    Mat ImageFinalResult2;
    GrayImage.copyTo(ImageFinalResult2);
    circle(ImageFinalResult2,FirstBestPoint,2,CV_RGB(255,255,255),2,8,0);

    RectUplPoint.x=(int)RectUplPoint.x/rectupnum;
    RectUplPoint.y=(int)RectUplPoint.y/rectupnum;
    DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)0);


/*
    //利用2个最佳匹配点画矩形框
    //计算两向量夹角
    float AvectorAngle=AnglePolar(DisksCenterPoints[SecondBestPointSort]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
    float BvectorAngle=AnglePolar(SecondBestPoint-FirstBestPoint);//实际向量
    //两向量夹角0-360°
    float Sita=BvectorAngle-AvectorAngle;
    if(Sita<0) Sita=Sita+360;


    Point RectUplPoint;//矩形左上角坐标
    RectUplPoint.x=(int)(-DisksCenterPoints[FirstBestPointSort].x*cos(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*sin(Sita/180*3.14) + FirstBestPoint.x);
    RectUplPoint.y=(int)(-(-DisksCenterPoints[FirstBestPointSort].x)*sin(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*cos(Sita/180*3.14)  + FirstBestPoint.y);
    DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)Sita);
*/

    imshow("GrayImageDisp",GrayImageDisp);//局部特征粗匹配得到候选匹配点集
    imshow("ImageFinalResult",ImageFinalResult);//Most Neighbors Similarity: 获取9个最佳匹配点
    imshow("ImageFinalResult2",ImageFinalResult2);//Most Neighbors Similarity: 从9个匹配点中获取最佳的2个匹配点

    //***********************************************************************
    //***********************************************************************
    timeSum=((double)getTickCount() - timeSum)/getTickFrequency();//运行时间
    qDebug("模板匹配结束，总耗时：%f秒",timeSum);
    qDebug("***********************");


    //返回结果参数
    MatchUpLPoint[0]=RectUplPoint;
    MatchTime[0]=timeSum;

}

//特征：环投影均值、灰度均值、HSV各分量均值
void MNShsv_SSDtMatching(Mat Image,Mat TemplateImage, Point* MatchUpLPoint, float* MatchTime){

    //分离颜色通道
    Mat Image_h,Image_s,Image_v,Imagehsv;
    if(Image.channels()!=1)
    {
        //分离颜色通道HSV
        vector<Mat> hsv_vec;
        cvtColor(Image, Imagehsv, CV_BGR2HSV);
        split(Imagehsv, hsv_vec);
        Image_h = hsv_vec[0];
        Image_s = hsv_vec[1];
        Image_v = hsv_vec[2];
        Image_h.convertTo(Image_h, CV_8UC1);
        Image_s.convertTo(Image_s, CV_8UC1);
        Image_v.convertTo(Image_v, CV_8UC1);
    }

    int i,j;
    //一、图像和模板都转为单通道的灰度图
    Mat GrayImage;
    cvtColor(Image,GrayImage,CV_RGB2GRAY,1);
    Mat GrayTemplate;
    cvtColor(TemplateImage,GrayTemplate,CV_RGB2GRAY,1);
    qDebug("待匹配图宽高=【%d x %d】；模板图宽高=【%d x %d】",GrayImage.cols,GrayImage.rows,GrayTemplate.cols,GrayTemplate.rows);

    //二、将模板图分成多个Ldisk*Ldisk的小方块，每个方块圆心相距
    R=(int)Ldisk/2;//局部方块半径
    int tt=2;//初始距离倍数
    int disL=(int)Ldisk/tt;//每个方块圆心相距的距离
    int WidthNum=(int)GrayTemplate.cols/Ldisk*tt - tt;//一行的方块数
    int HeightNum=(int)GrayTemplate.rows/Ldisk*tt - tt;//一列的方块数
    if(Ldisk==9 || WidthNum*HeightNum<70)
    {
        while(WidthNum*HeightNum<70)
        {
            tt++;
            disL=(int)(Ldisk/tt);//每个方块圆心相距的距离
            WidthNum=(int)GrayTemplate.cols/Ldisk*tt - tt;//一行的方块数
            HeightNum=(int)GrayTemplate.rows/Ldisk*tt - tt;//一列的方块数
        }
    }
    Thresh_distance=(float)(PSMode*Ldisk/tt);//邻居距离阈值
    qDebug("邻居距离阈值=%f",Thresh_distance);
    qDebug("模板方块边长=%d",Ldisk);
    qDebug("模板方块数量=%d",WidthNum*HeightNum);

    //四（1）、建立环投影的环查找表
    int x[R]={0};//环距离
    x[0]=1;
    int xnum=1;
    for(i=0;i<R;i++)
    {
        x[i+1]=(int)(R-sqrt((R-x[i])*(R-x[i])-(2*R-1))+0.5);
        xnum++;
        if(x[i+1]<=x[i])
        {
            x[i+1]=0;
            xnum--;
            break;
        }
    }

    uint AnnulusTable[Ldisk*Ldisk];//环查找表
    uint anNumber[xnum]={0};//统计同个环的像素数量
    uint Rtable[Ldisk*Ldisk];//半径查找表
    uint rNumber[R+1]={0};//统计同个半径值的数量
    uint aNumber[360]={0};//统计同个角度值的数量
    uint Atable[Ldisk*Ldisk];//角度查找表
    double YdivX;//斜率
    int A=0;//角度
    int r;//半径

    for(i=0;i<Ldisk;i++)
    {
        for(j=0;j<Ldisk;j++)
        {
            r=(int)(sqrt((i-R)*(i-R)+(j-R)*(j-R)) + 0.5);

            //建立极坐标（半径和角度）查找表
            //角度转换
             if(j>R-1 && i==R)
             {
                 A=0;
             }
             else if(j<R && i==R)
             {
                 A=180;
             }
             else if(j==R && i<R)
             {
                 A=90;
             }
             else if(j==R && i>R)
             {
                 A=270;
             }
             else if(j>R && i<R)
             {
                 YdivX=(double)(R-i)/(j-R);
                 A=(int)(atan(YdivX)*180/3.14159);
             }
             else if(j<R && i<R)
             {
                 YdivX=(double)(R-j)/(R-i);
                 A=(int)(atan(YdivX)*180/3.14159)+90;
             }
             else if(j<R && i>R)
             {
                 YdivX=(double)(i-R)/(R-j);
                 A=(int)(atan(YdivX)*180/3.14159)+180;
             }
             else if(j>R && i>R)
             {
                 YdivX=(double)(j-R)/(i-R);
                 A=(int)(atan(YdivX)*180/3.14159)+270;
             }


             if(r>R)
             {
                 Rtable[i*Ldisk+j] = R+1;
                 Atable[i*Ldisk+j] = 361;
             }
             else
             {
                 Rtable[i*Ldisk+j] = r;
                 Atable[i*Ldisk+j] = A;
                 rNumber[r]++;
                 aNumber[A]++;
             }

            // **************************
            // **************************
            //建立环查找表
            if(r>R)
            {
                AnnulusTable[i*Ldisk+j] = xnum;
            }
            else
            {
                for(int ii=0;ii<xnum;ii++)
                {
                    if(r>(R-x[ii]))
                    {
                        AnnulusTable[i*Ldisk+j] = ii;
                        anNumber[ii]++;
                        break;
                    }
                }

                if(r<=(R-x[xnum-1]))
                {
                    AnnulusTable[i*Ldisk+j] = xnum-1;
                    anNumber[xnum-1]++;
                }
            }

        }
    }

    //四（2）、计算模板图的环投影向量
    vector<Mat> Disks;//方块
    vector<Point> DisksCenterPoints;//每个方块的中心坐标点
    int DiskNum=0;//真正的方块数量
    int iarray[WidthNum*HeightNum]={-1};//正真方块的序号
    for(i=0;i<WidthNum*HeightNum;i++)
    {
        Mat GrayTemplatetemp(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
        float Ptemp[xnum]={0};
        Anvector(GrayTemplatetemp,AnnulusTable,xnum,anNumber,Ptemp);
        float PtempAver=0;
        for(j=0;j<xnum;j++)
        {
            PtempAver=PtempAver+Ptemp[j];
        }
        PtempAver=PtempAver/xnum;
        float P_SSD=0;
        for(j=0;j<xnum;j++)
        {
            P_SSD=P_SSD+(Ptemp[j]-PtempAver)*(Ptemp[j]-PtempAver);
        }

        if(P_SSD>4*xnum)
        {
            Disks.push_back(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
            DisksCenterPoints.push_back(Point(((int)i%WidthNum)*disL+R,((int)i/WidthNum)*disL+R));
            DiskNum++;
            iarray[i]=i;
        }

    }
    qDebug("真正的方块数量= %d",DiskNum);

    //DiskNum<=30的话直接放弃匹配
    if(DiskNum>30)
    {
        //环投影向量
        float P[DiskNum][xnum]={0};//模板图环投影向量
        for(i=0;i<DiskNum;i++)
        {
            //环投影向量
            Anvector(Disks[i],AnnulusTable,xnum,anNumber,P[i]);

        }
        //添加颜色特征
        float P_h[DiskNum]={0};//模板图h分量均值
        float P_s[DiskNum]={0};//模板图s分量均值
        float P_v[DiskNum]={0};//模板图v分量均值
        if(TemplateImage.channels()!=1)
        {
            //分离颜色通道HSV
            Mat TemplateImage_h,TemplateImage_s,TemplateImage_v,TemplateImagehsv;
            vector<Mat> hsv_vec;
            cvtColor(TemplateImage, TemplateImagehsv, CV_BGR2HSV);
            split(TemplateImagehsv, hsv_vec);
            TemplateImage_h = hsv_vec[0];
            TemplateImage_s = hsv_vec[1];
            TemplateImage_v = hsv_vec[2];
            TemplateImage_h.convertTo(TemplateImage_h, CV_8UC1);
            TemplateImage_s.convertTo(TemplateImage_s, CV_8UC1);
            TemplateImage_v.convertTo(TemplateImage_v, CV_8UC1);

            int iii=0;
            for(i=0;i<WidthNum*HeightNum;i++)
            {
                if(iarray[i]==i)
                {
                    Mat Disks_h=TemplateImage_h(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    Scalar tempVal = mean( Disks_h );
                    P_h[iii]= tempVal.val[0];

                    Mat Disks_s=TemplateImage_s(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = mean( Disks_s );
                    P_s[iii]= tempVal.val[0];

                    Mat Disks_v=TemplateImage_v(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = mean( Disks_v );
                    P_v[iii]= tempVal.val[0];

                    iii++;
                }
            }

        }
        //添加灰度均值特征
        float P_gray[DiskNum]={0};//模板图灰度均值
        int iii=0;
        for(i=0;i<WidthNum*HeightNum;i++)
        {
            if(iarray[i]==i)
            {
                Mat Disks_gray=GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                Scalar tempVal = mean( Disks_gray );
                P_gray[iii]= tempVal.val[0];

                iii++;
            }
        }

        //*****五、开始匹配************************************
        //*******************************************
        double timeSum;//总耗时
        timeSum=static_cast<double>(getTickCount());
        //***********************************************************
        double timeAnnulus;
        timeAnnulus=static_cast<double>(getTickCount());
        //设置圆投影搜索范围
        int LeftUpx=0;
        int LeftUpy=0;
        int RrightDownx=(int)(GrayImage.cols-Ldisk-1);
        int RrightDowny=(int)(GrayImage.rows-Ldisk-1);

        //获取并标出候选点位置
        double RminValue=0;
        Point RminLoc;
        double RmaxValue=0;
        Point RmaxLoc;
        Mat GrayImageDisp;
        GrayImage.copyTo(GrayImageDisp);

        int CandidateNum=MatchingNum2;
        vector<Point> Points[DiskNum];//精确匹配点坐标数组
        vector<Mat> MatchingValue;//环投影相似度值
        for(int ii=0;ii<DiskNum;ii++)
        {
            Mat MatchingValueTemp(GrayImage.rows-Ldisk,GrayImage.cols-Ldisk,CV_32FC1,Scalar(999999999));//float型单通道，每个点初始化
            MatchingValue.push_back(MatchingValueTemp);
        }
        for(i=LeftUpy;i<RrightDowny+1;i++)
        {

            for(j=LeftUpx;j<RrightDownx;j++)
            {

                Mat ScanROI;//待匹配的子图
                ScanROI=GrayImage(Rect(j,i,Ldisk,Ldisk));
                float T[xnum]={0};//模板图环投影向量
                Anvector(ScanROI,AnnulusTable,xnum,anNumber,T);
                Scalar tempVal1 = mean( ScanROI );
                float T_gray= tempVal1.val[0];

                //添加颜色特征
                float T_h=0;//h分量均值
                float T_s=0;//s分量均值
                float T_v=0;//v分量均值
                if(Image.channels()!=1)
                {
                    Mat ScanROI_hsv;//待匹配的子图
                    ScanROI_hsv=Image_h(Rect(j,i,Ldisk,Ldisk));
                    Scalar tempVal = mean( ScanROI_hsv );
                    T_h= tempVal.val[0];

                    ScanROI_hsv=Image_s(Rect(j,i,Ldisk,Ldisk));
                    tempVal = mean( ScanROI_hsv );
                    T_s= tempVal.val[0];

                    ScanROI_hsv=Image_v(Rect(j,i,Ldisk,Ldisk));
                    tempVal = mean( ScanROI_hsv );
                    T_v= tempVal.val[0];

                }

                for(int ii=0;ii<DiskNum;ii++)
                {
                    float* buffer;
                    buffer=MatchingValue[ii].ptr<float>(i);
                    //相似度计算
                    int tempbuffer=0;
                    for(int k=0;k<xnum;k++)
                    {
                        tempbuffer=tempbuffer+(P[ii][k]-T[k])*(P[ii][k]-T[k]);

                    }
                    buffer[j]=tempbuffer+(P_h[ii]-T_h)*(P_h[ii]-T_h)+(P_s[ii]-T_s)*(P_s[ii]-T_s)+(P_v[ii]-T_v)*(P_v[ii]-T_v)+(P_gray[ii]-T_gray)*(P_gray[ii]-T_gray);

                }

            }
        }

        for(int ii=0;ii<DiskNum;ii++)
        {
            for(i=0;i<CandidateNum;i++)
            {
                //1、选取候选匹配点
                minMaxLoc(MatchingValue[ii],&RminValue,&RmaxValue,&RminLoc,&RmaxLoc);
                Point BestMatch=RminLoc;
                Point Centerpoint=Point(BestMatch.x+R,BestMatch.y+R);

                //画圆
                circle(GrayImageDisp,Centerpoint,2,CV_RGB(255,255,255),2,8,0);
                Points[ii].push_back(Centerpoint);

                float* buffervalue=MatchingValue[ii].ptr<float>(BestMatch.y);
                buffervalue[BestMatch.x]=999999999;
            }
        }
        timeAnnulus=((double)getTickCount() - timeAnnulus)/getTickFrequency();//运行时间
        //qDebug("环投影匹配耗时：%f秒",timeAnnulus);


        double timeMNS;//MNS耗时
        timeMNS=static_cast<double>(getTickCount());
        //***************************************************
        //Neighbors Constraint Similarity确定最终匹配点
        //***************************************************
        Point BestPoints[DiskNum];//最佳匹配点
        int BestPointNum[DiskNum]={0};//最佳匹配点邻居数量
        Mat ImageFinalResult;//显示结果
        GrayImage.copyTo(ImageFinalResult);

        for(int ii=0;ii<DiskNum;ii++)
        {

            for(i=0;i<Points[ii].size();i++)
            {

                //邻居数量
                int PointNum=0;
                for(int jj=0;jj<DiskNum;jj++)
                {
                    //获取邻居点坐标
                    for(j=0;j<Points[jj].size();j++)
                    {
                        if(jj!=ii)
                        {
                            float distance=get_distance(Points[ii][i],Points[jj][j]);
                            float distemp=get_distance(DisksCenterPoints[ii],DisksCenterPoints[jj]);
                            distance=fabs(distance-distemp);
                            if(distance<Thresh_distance)
                            {
                                PointNum++;//第一多的邻居数量
                                break;
                            }
                        }

                    }

                    //获取最佳点坐标
                    if(PointNum>BestPointNum[ii])
                    {
                        BestPoints[ii]=Points[ii][i];
                        BestPointNum[ii]=PointNum;
                    }
                }

            }
            if(BestPointNum[ii]<Thresh_MBNSnum)  BestPoints[ii]=Point(0,0);//阈值：最多邻居数量不少于Thresh_MBNSnum

        }


        //****从精确匹配得到的WidthNum*HeightNum个点中确定最终匹配点****
        Point FirstBestPoint=BestPoints[0];
        int FirstBestPointNum=0;
        int FirstBestPointSort=0;

        for(i=0;i<DiskNum;i++)
        {
            int PointNum=0;

            PointNum=CountNeighbors(i, BestPoints, DiskNum,DisksCenterPoints, Thresh_distance);//Thresh_distance=R
            //获取最佳点
            if(PointNum>FirstBestPointNum)
            {
                FirstBestPoint=BestPoints[i];
                FirstBestPointNum=PointNum;
                FirstBestPointSort=i;
            }

        }

        //剔除错误点，与最佳匹配点是邻居的点认为是正确点
        Point RectUplPoint=(FirstBestPoint-DisksCenterPoints[FirstBestPointSort]);//矩形左上角坐标
        int rectupnum=1;
        for(i=0;i<DiskNum;i++)
        {
            if(BestPoints[i].x==0 && BestPoints[i].y==0)
            {
                BestPoints[i]=Point(0,0);
            }
            else
            {
                if(FirstBestPointSort!=i)
                {
                    float distance=get_distance(FirstBestPoint,BestPoints[i]);
                    float distemp=get_distance(DisksCenterPoints[FirstBestPointSort],DisksCenterPoints[i]);
                    distance=fabs(distance-distemp);//距离
                    if(distance>Thresh_distance)
                    {
                        BestPoints[i]=Point(0,0);
                    }
                    else
                    {
                        RectUplPoint=RectUplPoint+(BestPoints[i]-DisksCenterPoints[i]);
                        rectupnum++;
                    }
                }
            }


            circle(ImageFinalResult,BestPoints[i],2,CV_RGB(255,255,255),2,8,0);
        }

        timeMNS=((double)getTickCount() - timeMNS)/getTickFrequency();//运行时间
        qDebug("MNS_SSD耗时：%f秒",timeMNS);
        Mat ImageFinalResult2;
        GrayImage.copyTo(ImageFinalResult2);
        circle(ImageFinalResult2,FirstBestPoint,2,CV_RGB(255,255,255),2,8,0);

        RectUplPoint.x=(int)RectUplPoint.x/rectupnum;
        RectUplPoint.y=(int)RectUplPoint.y/rectupnum;
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)0);


    /*
        //利用2个最佳匹配点画矩形框
        //计算两向量夹角
        float AvectorAngle=AnglePolar(DisksCenterPoints[SecondBestPointSort]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
        float BvectorAngle=AnglePolar(SecondBestPoint-FirstBestPoint);//实际向量
        //两向量夹角0-360°
        float Sita=BvectorAngle-AvectorAngle;
        if(Sita<0) Sita=Sita+360;


        Point RectUplPoint;//矩形左上角坐标
        RectUplPoint.x=(int)(-DisksCenterPoints[FirstBestPointSort].x*cos(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*sin(Sita/180*3.14) + FirstBestPoint.x);
        RectUplPoint.y=(int)(-(-DisksCenterPoints[FirstBestPointSort].x)*sin(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*cos(Sita/180*3.14)  + FirstBestPoint.y);
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)Sita);
    */

        imshow("GrayImageDisp",GrayImageDisp);//局部特征粗匹配得到候选匹配点集
        imshow("ImageFinalResult",ImageFinalResult);//Most Neighbors Similarity: 获取9个最佳匹配点
        imshow("ImageFinalResult2",ImageFinalResult2);//Most Neighbors Similarity: 从9个匹配点中获取最佳的2个匹配点

        //***********************************************************************
        //***********************************************************************
        timeSum=((double)getTickCount() - timeSum)/getTickFrequency();//运行时间
        qDebug("模板匹配结束，总耗时：%f秒",timeSum);
        qDebug("***********************");


        //返回结果参数
        MatchUpLPoint[0]=RectUplPoint;
        MatchTime[0]=timeSum;
    }
    else
    {
        //返回结果参数
        MatchUpLPoint[0]=Point(0,0);
        MatchTime[0]=0;
    }

}

//特征：环投影均值、灰度均值、HSV各分量均值
void MBNShsv_SSDtMatching(Mat Image,Mat TemplateImage, Point* MatchUpLPoint, float* MatchTime){

    //分离颜色通道
    Mat Image_h,Image_s,Image_v,Imagehsv;
    if(Image.channels()!=1)
    {
        //分离颜色通道HSV
        vector<Mat> hsv_vec;
        cvtColor(Image, Imagehsv, CV_BGR2HSV);
        split(Imagehsv, hsv_vec);
        Image_h = hsv_vec[0];
        Image_s = hsv_vec[1];
        Image_v = hsv_vec[2];
        Image_h.convertTo(Image_h, CV_8UC1);
        Image_s.convertTo(Image_s, CV_8UC1);
        Image_v.convertTo(Image_v, CV_8UC1);
    }

    int i,j;
    //一、图像和模板都转为单通道的灰度图
    Mat GrayImage;
    cvtColor(Image,GrayImage,CV_RGB2GRAY,1);
    Mat GrayTemplate;
    cvtColor(TemplateImage,GrayTemplate,CV_RGB2GRAY,1);
    qDebug("待匹配图宽高=【%d x %d】；模板图宽高=【%d x %d】",GrayImage.cols,GrayImage.rows,GrayTemplate.cols,GrayTemplate.rows);

    //二、将模板图分成多个Ldisk*Ldisk的小方块，每个方块圆心相距
    R=(int)Ldisk/2;//局部方块半径
    int tt=2;//初始距离倍数
    int disL=(int)Ldisk/tt;//每个方块圆心相距的距离
    int WidthNum=(int)GrayTemplate.cols/Ldisk*tt - tt;//一行的方块数
    int HeightNum=(int)GrayTemplate.rows/Ldisk*tt - tt;//一列的方块数
    if(Ldisk==9 || WidthNum*HeightNum<70)
    {
        while(WidthNum*HeightNum<70)
        {
            tt++;
            disL=(int)(Ldisk/tt);//每个方块圆心相距的距离
            WidthNum=(int)GrayTemplate.cols/Ldisk*tt - tt;//一行的方块数
            HeightNum=(int)GrayTemplate.rows/Ldisk*tt - tt;//一列的方块数
        }
    }
    Thresh_distance=(float)(PSMode*Ldisk/tt);//邻居距离阈值
    qDebug("邻居距离阈值=%f",Thresh_distance);
    qDebug("模板方块边长=%d",Ldisk);
    qDebug("模板方块数量=%d",WidthNum*HeightNum);

    //四（1）、建立环投影的环查找表
    int x[R]={0};//环距离
    x[0]=1;
    int xnum=1;
    for(i=0;i<R;i++)
    {
        x[i+1]=(int)(R-sqrt((R-x[i])*(R-x[i])-(2*R-1))+0.5);
        xnum++;
        if(x[i+1]<=x[i])
        {
            x[i+1]=0;
            xnum--;
            break;
        }
    }

    uint AnnulusTable[Ldisk*Ldisk];//环查找表
    uint anNumber[xnum]={0};//统计同个环的像素数量
    uint Rtable[Ldisk*Ldisk];//半径查找表
    uint rNumber[R+1]={0};//统计同个半径值的数量
    uint aNumber[360]={0};//统计同个角度值的数量
    uint Atable[Ldisk*Ldisk];//角度查找表
    double YdivX;//斜率
    int A=0;//角度
    int r;//半径

    for(i=0;i<Ldisk;i++)
    {
        for(j=0;j<Ldisk;j++)
        {
            r=(int)(sqrt((i-R)*(i-R)+(j-R)*(j-R)) + 0.5);

            //建立极坐标（半径和角度）查找表
            //角度转换
             if(j>R-1 && i==R)
             {
                 A=0;
             }
             else if(j<R && i==R)
             {
                 A=180;
             }
             else if(j==R && i<R)
             {
                 A=90;
             }
             else if(j==R && i>R)
             {
                 A=270;
             }
             else if(j>R && i<R)
             {
                 YdivX=(double)(R-i)/(j-R);
                 A=(int)(atan(YdivX)*180/3.14159);
             }
             else if(j<R && i<R)
             {
                 YdivX=(double)(R-j)/(R-i);
                 A=(int)(atan(YdivX)*180/3.14159)+90;
             }
             else if(j<R && i>R)
             {
                 YdivX=(double)(i-R)/(R-j);
                 A=(int)(atan(YdivX)*180/3.14159)+180;
             }
             else if(j>R && i>R)
             {
                 YdivX=(double)(j-R)/(i-R);
                 A=(int)(atan(YdivX)*180/3.14159)+270;
             }


             if(r>R)
             {
                 Rtable[i*Ldisk+j] = R+1;
                 Atable[i*Ldisk+j] = 361;
             }
             else
             {
                 Rtable[i*Ldisk+j] = r;
                 Atable[i*Ldisk+j] = A;
                 rNumber[r]++;
                 aNumber[A]++;
             }

            // **************************
            // **************************
            //建立环查找表
            if(r>R)
            {
                AnnulusTable[i*Ldisk+j] = xnum;
            }
            else
            {
                for(int ii=0;ii<xnum;ii++)
                {
                    if(r>(R-x[ii]))
                    {
                        AnnulusTable[i*Ldisk+j] = ii;
                        anNumber[ii]++;
                        break;
                    }
                }

                if(r<=(R-x[xnum-1]))
                {
                    AnnulusTable[i*Ldisk+j] = xnum-1;
                    anNumber[xnum-1]++;
                }
            }

        }
    }

    //四（2）、计算模板图的环投影向量
    vector<Mat> Disks;//方块
    vector<Point> DisksCenterPoints;//每个方块的中心坐标点
    int DiskNum=0;//真正的方块数量
    int iarray[WidthNum*HeightNum]={-1};//正真方块的序号
    for(i=0;i<WidthNum*HeightNum;i++)
    {
        Mat GrayTemplatetemp(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
        float Ptemp[xnum]={0};
        Anvector(GrayTemplatetemp,AnnulusTable,xnum,anNumber,Ptemp);
        float PtempAver=0;
        for(j=0;j<xnum;j++)
        {
            PtempAver=PtempAver+Ptemp[j];
        }
        PtempAver=PtempAver/xnum;
        float P_SSD=0;
        for(j=0;j<xnum;j++)
        {
            P_SSD=P_SSD+(Ptemp[j]-PtempAver)*(Ptemp[j]-PtempAver);
        }

        if(P_SSD>4*xnum)
        {
            Disks.push_back(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
            DisksCenterPoints.push_back(Point(((int)i%WidthNum)*disL+R,((int)i/WidthNum)*disL+R));
            DiskNum++;
            iarray[i]=i;
        }

    }
    qDebug("真正的方块数量= %d",DiskNum);

    //DiskNum<=30的话直接放弃匹配
    if(DiskNum>30)
    {
        //环投影向量
        float P[DiskNum][xnum]={0};//模板图环投影向量
        for(i=0;i<DiskNum;i++)
        {
            //环投影向量
            Anvector(Disks[i],AnnulusTable,xnum,anNumber,P[i]);

        }
        //添加颜色特征
        float P_h[DiskNum]={0};//模板图h分量均值
        float P_s[DiskNum]={0};//模板图s分量均值
        float P_v[DiskNum]={0};//模板图v分量均值
        if(TemplateImage.channels()!=1)
        {
            //分离颜色通道HSV
            Mat TemplateImage_h,TemplateImage_s,TemplateImage_v,TemplateImagehsv;
            vector<Mat> hsv_vec;
            cvtColor(TemplateImage, TemplateImagehsv, CV_BGR2HSV);
            split(TemplateImagehsv, hsv_vec);
            TemplateImage_h = hsv_vec[0];
            TemplateImage_s = hsv_vec[1];
            TemplateImage_v = hsv_vec[2];
            TemplateImage_h.convertTo(TemplateImage_h, CV_8UC1);
            TemplateImage_s.convertTo(TemplateImage_s, CV_8UC1);
            TemplateImage_v.convertTo(TemplateImage_v, CV_8UC1);

            int iii=0;
            for(i=0;i<WidthNum*HeightNum;i++)
            {
                if(iarray[i]==i)
                {
                    Mat Disks_h=TemplateImage_h(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    Scalar tempVal = mean( Disks_h );
                    P_h[iii]= tempVal.val[0];

                    Mat Disks_s=TemplateImage_s(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = mean( Disks_s );
                    P_s[iii]= tempVal.val[0];

                    Mat Disks_v=TemplateImage_v(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = mean( Disks_v );
                    P_v[iii]= tempVal.val[0];

                    iii++;
                }
            }

        }
        //添加灰度均值特征
        float P_gray[DiskNum]={0};//模板图灰度均值
        int iii=0;
        for(i=0;i<WidthNum*HeightNum;i++)
        {
            if(iarray[i]==i)
            {
                Mat Disks_gray=GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                Scalar tempVal = mean( Disks_gray );
                P_gray[iii]= tempVal.val[0];

                iii++;
            }
        }

        //*****五、开始匹配************************************
        //*******************************************
        double timeSum;//总耗时
        timeSum=static_cast<double>(getTickCount());
        //***********************************************************
        double timeAnnulus;
        timeAnnulus=static_cast<double>(getTickCount());
        //设置圆投影搜索范围
        int LeftUpx=0;
        int LeftUpy=0;
        int RrightDownx=(int)(GrayImage.cols-Ldisk-1);
        int RrightDowny=(int)(GrayImage.rows-Ldisk-1);

        //获取并标出候选点位置
        double RminValue=0;
        Point RminLoc;
        double RmaxValue=0;
        Point RmaxLoc;
        Mat GrayImageDisp;
        GrayImage.copyTo(GrayImageDisp);

        int CandidateNum=MatchingNum2;
        vector<Point> Points[DiskNum];//精确匹配点坐标数组
        vector<Mat> MatchingValue;//环投影相似度值
        for(int ii=0;ii<DiskNum;ii++)
        {
            Mat MatchingValueTemp(GrayImage.rows-Ldisk,GrayImage.cols-Ldisk,CV_32FC1,Scalar(999999999));//float型单通道，每个点初始化
            MatchingValue.push_back(MatchingValueTemp);
        }
        for(i=LeftUpy;i<RrightDowny+1;i++)
        {

            for(j=LeftUpx;j<RrightDownx;j++)
            {

                Mat ScanROI;//待匹配的子图
                ScanROI=GrayImage(Rect(j,i,Ldisk,Ldisk));
                float T[xnum]={0};//模板图环投影向量
                Anvector(ScanROI,AnnulusTable,xnum,anNumber,T);
                Scalar tempVal1 = mean( ScanROI );
                float T_gray= tempVal1.val[0];

                //添加颜色特征
                float T_h=0;//h分量均值
                float T_s=0;//s分量均值
                float T_v=0;//v分量均值
                if(Image.channels()!=1)
                {
                    Mat ScanROI_hsv;//待匹配的子图
                    ScanROI_hsv=Image_h(Rect(j,i,Ldisk,Ldisk));
                    Scalar tempVal = mean( ScanROI_hsv );
                    T_h= tempVal.val[0];

                    ScanROI_hsv=Image_s(Rect(j,i,Ldisk,Ldisk));
                    tempVal = mean( ScanROI_hsv );
                    T_s= tempVal.val[0];

                    ScanROI_hsv=Image_v(Rect(j,i,Ldisk,Ldisk));
                    tempVal = mean( ScanROI_hsv );
                    T_v= tempVal.val[0];

                }

                for(int ii=0;ii<DiskNum;ii++)
                {
                    float* buffer;
                    buffer=MatchingValue[ii].ptr<float>(i);
                    //相似度计算
                    int tempbuffer=0;
                    for(int k=0;k<xnum;k++)
                    {
                        tempbuffer=tempbuffer+(P[ii][k]-T[k])*(P[ii][k]-T[k]);

                    }
                    buffer[j]=tempbuffer+(P_h[ii]-T_h)*(P_h[ii]-T_h)+(P_s[ii]-T_s)*(P_s[ii]-T_s)+(P_v[ii]-T_v)*(P_v[ii]-T_v)+(P_gray[ii]-T_gray)*(P_gray[ii]-T_gray);

                }

            }
        }

        for(int ii=0;ii<DiskNum;ii++)
        {
            for(i=0;i<CandidateNum;i++)
            {
                //1、选取候选匹配点
                minMaxLoc(MatchingValue[ii],&RminValue,&RmaxValue,&RminLoc,&RmaxLoc);
                Point BestMatch=RminLoc;
                Point Centerpoint=Point(BestMatch.x+R,BestMatch.y+R);

                //画圆
                circle(GrayImageDisp,Centerpoint,2,CV_RGB(255,255,255),2,8,0);
                Points[ii].push_back(Centerpoint);

                float* buffervalue=MatchingValue[ii].ptr<float>(BestMatch.y);
                buffervalue[BestMatch.x]=999999999;
            }
        }
        timeAnnulus=((double)getTickCount() - timeAnnulus)/getTickFrequency();//运行时间
        //qDebug("环投影匹配耗时：%f秒",timeAnnulus);


        double timeMBNS;//MBNS耗时
        timeMBNS=static_cast<double>(getTickCount());
        //***************************************************
        //Neighbors Constraint Similarity确定最终匹配点
        //***************************************************
        Point BestPoints[DiskNum];//最佳匹配点
        int BestPointNum[DiskNum]={0};//最佳匹配点邻居数量
        Mat ImageFinalResult;//显示结果
        GrayImage.copyTo(ImageFinalResult);

        for(int ii=0;ii<DiskNum;ii++)
        {

            for(i=0;i<Points[ii].size();i++)
            {

                float tempMinDis=999999999;

                //第二多的邻居数量
                int PointNum=0;
                Point Pointtemp(0,0);
                Point Pointss[DiskNum];
                for(j=0;j<DiskNum;j++)
                {
                    Pointss[j]=Point(0,0);
                    if(j==ii)   Pointss[j]=Points[ii][i];
                }

                for(int jj=0;jj<DiskNum;jj++)
                {
                    //获取邻居点坐标
                    for(j=0;j<Points[jj].size();j++)
                    {
                        if(jj!=ii)
                        {
                            float distance=get_distance(Points[ii][i],Points[jj][j]);
                            float distemp=get_distance(DisksCenterPoints[ii],DisksCenterPoints[jj]);
                            distance=fabs(distance-distemp);//距离
                            if(distance<tempMinDis)
                            {
                                tempMinDis=distance;
                                Pointtemp=Points[jj][j];
                            }
                        }

                    }
                    if(tempMinDis<Thresh_distance)
                    {
                        Pointss[jj]=Pointtemp;
                        PointNum++;//第一多的邻居数量
                    }
                    tempMinDis=999999999;

                }
                //阈值：第一多的邻居数量不少于Thresh_MBNSnum个
                if(PointNum>Thresh_MBNSnum)
                {
                    PointNum=0;
                    //计算第二多的邻居数量
                    int PointNumtemp=0;
                    for(int tt=0;tt<DiskNum;tt++)
                    {
                        if(tt!=ii)
                        {
                            PointNumtemp=CountNeighbors(tt, Pointss, DiskNum,DisksCenterPoints, Thresh_distance);
                            if(PointNum<PointNumtemp)
                            {
                                PointNum=PointNumtemp;//第二多的邻居数量
                            }
                        }
                    }

                    //获取最佳点坐标
                    if(PointNum>BestPointNum[ii])
                    {
                        BestPoints[ii]=Points[ii][i];
                        BestPointNum[ii]=PointNum;
                    }

                }

            }
            if(BestPointNum[ii]<Thresh_MBNSnum)  BestPoints[ii]=Point(0,0);//阈值：第二多邻居数量不少于Thresh_MBNSnum


        }

        //****从精确匹配得到的WidthNum*HeightNum个点中确定最终匹配点****
        Point FirstBestPoint=BestPoints[0];
        int FirstBestPointNum=0;
        int FirstBestPointSort=0;
        for(i=0;i<DiskNum;i++)
        {
            int PointNum=0;

            PointNum=CountNeighbors(i, BestPoints, DiskNum,DisksCenterPoints, Thresh_distance);//Thresh_distance=R
            //获取最佳点
            if(PointNum>FirstBestPointNum)
            {
                FirstBestPoint=BestPoints[i];
                FirstBestPointNum=PointNum;
                FirstBestPointSort=i;
            }


        }

        //剔除错误点，与最佳匹配点是邻居的点认为是正确点
        Point RectUplPoint=(FirstBestPoint-DisksCenterPoints[FirstBestPointSort]);//矩形左上角坐标
        int rectupnum=1;
        for(i=0;i<DiskNum;i++)
        {
            if(BestPoints[i].x==0 && BestPoints[i].y==0)
            {
                BestPoints[i]=Point(0,0);
            }
            else
            {
                if(FirstBestPointSort!=i)
                {
                    float distance=get_distance(FirstBestPoint,BestPoints[i]);
                    float distemp=get_distance(DisksCenterPoints[FirstBestPointSort],DisksCenterPoints[i]);
                    distance=fabs(distance-distemp);//距离
                    if(distance>Thresh_distance)
                    {
                        BestPoints[i]=Point(0,0);
                    }
                    else
                    {
                        RectUplPoint=RectUplPoint+(BestPoints[i]-DisksCenterPoints[i]);
                        rectupnum++;
                    }
                }
            }


            circle(ImageFinalResult,BestPoints[i],2,CV_RGB(255,255,255),2,8,0);
        }

        timeMBNS=((double)getTickCount() - timeMBNS)/getTickFrequency();//运行时间
        qDebug("MBNS耗时：%f秒",timeMBNS);
        Mat ImageFinalResult2;
        GrayImage.copyTo(ImageFinalResult2);
        circle(ImageFinalResult2,FirstBestPoint,2,CV_RGB(255,255,255),2,8,0);


        RectUplPoint.x=(int)RectUplPoint.x/rectupnum;
        RectUplPoint.y=(int)RectUplPoint.y/rectupnum;
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)0);


    /*
        //利用2个最佳匹配点画矩形框
        //计算两向量夹角
        float AvectorAngle=AnglePolar(DisksCenterPoints[SecondBestPointSort]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
        float BvectorAngle=AnglePolar(SecondBestPoint-FirstBestPoint);//实际向量
        //两向量夹角0-360°
        float Sita=BvectorAngle-AvectorAngle;
        if(Sita<0) Sita=Sita+360;


        Point RectUplPoint;//矩形左上角坐标
        RectUplPoint.x=(int)(-DisksCenterPoints[FirstBestPointSort].x*cos(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*sin(Sita/180*3.14) + FirstBestPoint.x);
        RectUplPoint.y=(int)(-(-DisksCenterPoints[FirstBestPointSort].x)*sin(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*cos(Sita/180*3.14)  + FirstBestPoint.y);
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)Sita);
    */
    /*
        //显示结果
        qDebug("最终匹配的矩形左上角坐标=(%d,%d);旋转角=%f°",RectUplPoint.x,RectUplPoint.y,0);
        imshow("GrayImageDisp",GrayImageDisp);//局部特征粗匹配得到候选匹配点集
        imshow("ImageFinalResult",ImageFinalResult);//Most Neighbors Similarity: 获取9个最佳匹配点
        imshow("ImageFinalResult2",ImageFinalResult2);//Most Neighbors Similarity: 从9个匹配点中获取最佳的2个匹配点
    */
        //***********************************************************************
        //***********************************************************************
        timeSum=((double)getTickCount() - timeSum)/getTickFrequency();//运行时间
        qDebug("模板匹配结束，总耗时：%f秒",timeSum);
        qDebug("***********************");

    /*
        Mat srcImageDisp;
        //resize函数前加cv：：，因为Qt也有resize函数，故加cv：：加以区别，不然会出现错误
        int resizeW=(int)ImageFinalResult2.cols/(beta+1);
        int resizeH=(int)ImageFinalResult2.rows/(beta+1);
        cv::resize(ImageFinalResult2,srcImageDisp,Size(resizeW,resizeH),0,0,3);
        Imgview->imshow(srcImageDisp);//显示到绑定的控件上

    */

        //返回结果参数
        MatchUpLPoint[0]=RectUplPoint;
        MatchTime[0]=timeSum;
    }
    else
    {
        //返回结果参数
        MatchUpLPoint[0]=Point(0,0);
        MatchTime[0]=0;
    }

}

//特征：环投影均值、灰度均值和方差、HSV各分量均值
void MNShsv_SSDtMatching2(Mat Image,Mat TemplateImage, Point* MatchUpLPoint, float* MatchTime){

    //分离颜色通道
    Mat Image_h,Image_s,Image_v,Imagehsv;
    if(Image.channels()!=1)
    {
        //分离颜色通道HSV
        vector<Mat> hsv_vec;
        cvtColor(Image, Imagehsv, CV_BGR2HSV);
        split(Imagehsv, hsv_vec);
        Image_h = hsv_vec[0];
        Image_s = hsv_vec[1];
        Image_v = hsv_vec[2];
        Image_h.convertTo(Image_h, CV_8UC1);
        Image_s.convertTo(Image_s, CV_8UC1);
        Image_v.convertTo(Image_v, CV_8UC1);
    }

    int i,j;
    //一、图像和模板都转为单通道的灰度图
    Mat GrayImage;
    cvtColor(Image,GrayImage,CV_RGB2GRAY,1);
    Mat GrayTemplate;
    cvtColor(TemplateImage,GrayTemplate,CV_RGB2GRAY,1);
    qDebug("待匹配图宽高=【%d x %d】；模板图宽高=【%d x %d】",GrayImage.cols,GrayImage.rows,GrayTemplate.cols,GrayTemplate.rows);

    //二、将模板图分成多个Ldisk*Ldisk的小方块，每个方块圆心相距
    R=(int)Ldisk/2;//局部方块半径
    int tt=2;//初始距离倍数
    int disL=(int)Ldisk/tt;//每个方块圆心相距的距离
    int WidthNum=(int)GrayTemplate.cols/Ldisk*tt - tt;//一行的方块数
    int HeightNum=(int)GrayTemplate.rows/Ldisk*tt - tt;//一列的方块数
    if(Ldisk==9 || WidthNum*HeightNum<70)
    {
        while(WidthNum*HeightNum<70)
        {
            tt++;
            disL=(int)(Ldisk/tt);//每个方块圆心相距的距离
            WidthNum=(int)GrayTemplate.cols/Ldisk*tt - tt;//一行的方块数
            HeightNum=(int)GrayTemplate.rows/Ldisk*tt - tt;//一列的方块数
        }
    }
    Thresh_distance=(float)(PSMode*Ldisk/tt);//邻居距离阈值
    qDebug("邻居距离阈值=%f",Thresh_distance);
    qDebug("模板方块边长=%d",Ldisk);
    qDebug("模板方块数量=%d",WidthNum*HeightNum);

    //四（1）、建立环投影的环查找表
    int x[R]={0};//环距离
    x[0]=1;
    int xnum=1;
    for(i=0;i<R;i++)
    {
        x[i+1]=(int)(R-sqrt((R-x[i])*(R-x[i])-(2*R-1))+0.5);
        xnum++;
        if(x[i+1]<=x[i])
        {
            x[i+1]=0;
            xnum--;
            break;
        }
    }

    uint AnnulusTable[Ldisk*Ldisk];//环查找表
    uint anNumber[xnum]={0};//统计同个环的像素数量
    uint Rtable[Ldisk*Ldisk];//半径查找表
    uint rNumber[R+1]={0};//统计同个半径值的数量
    uint aNumber[360]={0};//统计同个角度值的数量
    uint Atable[Ldisk*Ldisk];//角度查找表
    double YdivX;//斜率
    int A=0;//角度
    int r;//半径

    for(i=0;i<Ldisk;i++)
    {
        for(j=0;j<Ldisk;j++)
        {
            r=(int)(sqrt((i-R)*(i-R)+(j-R)*(j-R)) + 0.5);

            //建立极坐标（半径和角度）查找表
            //角度转换
             if(j>R-1 && i==R)
             {
                 A=0;
             }
             else if(j<R && i==R)
             {
                 A=180;
             }
             else if(j==R && i<R)
             {
                 A=90;
             }
             else if(j==R && i>R)
             {
                 A=270;
             }
             else if(j>R && i<R)
             {
                 YdivX=(double)(R-i)/(j-R);
                 A=(int)(atan(YdivX)*180/3.14159);
             }
             else if(j<R && i<R)
             {
                 YdivX=(double)(R-j)/(R-i);
                 A=(int)(atan(YdivX)*180/3.14159)+90;
             }
             else if(j<R && i>R)
             {
                 YdivX=(double)(i-R)/(R-j);
                 A=(int)(atan(YdivX)*180/3.14159)+180;
             }
             else if(j>R && i>R)
             {
                 YdivX=(double)(j-R)/(i-R);
                 A=(int)(atan(YdivX)*180/3.14159)+270;
             }


             if(r>R)
             {
                 Rtable[i*Ldisk+j] = R+1;
                 Atable[i*Ldisk+j] = 361;
             }
             else
             {
                 Rtable[i*Ldisk+j] = r;
                 Atable[i*Ldisk+j] = A;
                 rNumber[r]++;
                 aNumber[A]++;
             }

            // **************************
            // **************************
            //建立环查找表
            if(r>R)
            {
                AnnulusTable[i*Ldisk+j] = xnum;
            }
            else
            {
                for(int ii=0;ii<xnum;ii++)
                {
                    if(r>(R-x[ii]))
                    {
                        AnnulusTable[i*Ldisk+j] = ii;
                        anNumber[ii]++;
                        break;
                    }
                }

                if(r<=(R-x[xnum-1]))
                {
                    AnnulusTable[i*Ldisk+j] = xnum-1;
                    anNumber[xnum-1]++;
                }
            }

        }
    }

    //四（2）、计算模板图的环投影向量
    vector<Mat> Disks;//方块
    vector<Point> DisksCenterPoints;//每个方块的中心坐标点
    int DiskNum=0;//真正的方块数量
    int iarray[WidthNum*HeightNum]={-1};//正真方块的序号
    for(i=0;i<WidthNum*HeightNum;i++)
    {
        Mat GrayTemplatetemp(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
        float Ptemp[xnum]={0};
        Anvector(GrayTemplatetemp,AnnulusTable,xnum,anNumber,Ptemp);
        float PtempAver=0;
        for(j=0;j<xnum;j++)
        {
            PtempAver=PtempAver+Ptemp[j];
        }
        PtempAver=PtempAver/xnum;
        float P_SSD=0;
        for(j=0;j<xnum;j++)
        {
            P_SSD=P_SSD+(Ptemp[j]-PtempAver)*(Ptemp[j]-PtempAver);
        }

        if(P_SSD>4*xnum)
        {
            Disks.push_back(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
            DisksCenterPoints.push_back(Point(((int)i%WidthNum)*disL+R,((int)i/WidthNum)*disL+R));
            DiskNum++;
            iarray[i]=i;
        }

    }
    qDebug("真正的方块数量= %d",DiskNum);

    //DiskNum<=30的话直接放弃匹配
    if(DiskNum>30)
    {
        //环投影向量
        float P[DiskNum][xnum]={0};//模板图环投影向量
        for(i=0;i<DiskNum;i++)
        {
            //环投影向量
            Anvector(Disks[i],AnnulusTable,xnum,anNumber,P[i]);

        }
        //添加颜色特征
        float P_h[DiskNum]={0};//模板图h分量均值
        float P_s[DiskNum]={0};//模板图s分量均值
        float P_v[DiskNum]={0};//模板图v分量均值
        if(TemplateImage.channels()!=1)
        {
            //分离颜色通道HSV
            Mat TemplateImage_h,TemplateImage_s,TemplateImage_v,TemplateImagehsv;
            vector<Mat> hsv_vec;
            cvtColor(TemplateImage, TemplateImagehsv, CV_BGR2HSV);
            split(TemplateImagehsv, hsv_vec);
            TemplateImage_h = hsv_vec[0];
            TemplateImage_s = hsv_vec[1];
            TemplateImage_v = hsv_vec[2];
            TemplateImage_h.convertTo(TemplateImage_h, CV_8UC1);
            TemplateImage_s.convertTo(TemplateImage_s, CV_8UC1);
            TemplateImage_v.convertTo(TemplateImage_v, CV_8UC1);

            int iii=0;
            for(i=0;i<WidthNum*HeightNum;i++)
            {
                if(iarray[i]==i)
                {
                    Mat Disks_h=TemplateImage_h(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    Scalar tempVal = cv::mean( Disks_h );
                    P_h[iii]= tempVal.val[0];

                    Mat Disks_s=TemplateImage_s(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = cv::mean( Disks_s );
                    P_s[iii]= tempVal.val[0];

                    Mat Disks_v=TemplateImage_v(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = cv::mean( Disks_v );
                    P_v[iii]= tempVal.val[0];

                    iii++;
                }
            }

        }
        //添加灰度均值和方差特征
        float P_grayM[DiskNum]={0};//模板图灰度均值
        float P_grayD[DiskNum]={0};//模板图灰度方差
        int iii=0;
        for(i=0;i<WidthNum*HeightNum;i++)
        {
            if(iarray[i]==i)
            {
                Mat Disks_gray=GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                Scalar Mean,dev;
                cv::meanStdDev(Disks_gray,Mean,dev);
                P_grayM[iii]= Mean.val[0];
                P_grayD[iii]= dev.val[0];

                iii++;
            }
        }

        //*****五、开始匹配************************************
        //*******************************************
        double timeSum;//总耗时
        timeSum=static_cast<double>(getTickCount());
        //***********************************************************
        double timeAnnulus;
        timeAnnulus=static_cast<double>(getTickCount());
        //设置圆投影搜索范围
        int LeftUpx=0;
        int LeftUpy=0;
        int RrightDownx=(int)(GrayImage.cols-Ldisk-1);
        int RrightDowny=(int)(GrayImage.rows-Ldisk-1);

        //获取并标出候选点位置
        double RminValue=0;
        Point RminLoc;
        double RmaxValue=0;
        Point RmaxLoc;
        Mat GrayImageDisp;
        GrayImage.copyTo(GrayImageDisp);

        int CandidateNum=MatchingNum2;
        vector<Point> Points[DiskNum];//精确匹配点坐标数组
        vector<Mat> MatchingValue;//环投影相似度值
        for(int ii=0;ii<DiskNum;ii++)
        {
            Mat MatchingValueTemp(GrayImage.rows-Ldisk,GrayImage.cols-Ldisk,CV_32FC1,Scalar(999999999));//float型单通道，每个点初始化
            MatchingValue.push_back(MatchingValueTemp);
        }
        for(i=LeftUpy;i<RrightDowny+1;i++)
        {

            for(j=LeftUpx;j<RrightDownx;j++)
            {

                Mat ScanROI;//待匹配的子图
                ScanROI=GrayImage(Rect(j,i,Ldisk,Ldisk));
                float T[xnum]={0};//模板图环投影向量
                Anvector(ScanROI,AnnulusTable,xnum,anNumber,T);
                Scalar Mean,dev;
                cv::meanStdDev(ScanROI,Mean,dev);
                float T_grayM= Mean.val[0];
                float T_grayD= dev.val[0];

                //添加颜色特征
                float T_h=0;//h分量均值
                float T_s=0;//s分量均值
                float T_v=0;//v分量均值
                if(Image.channels()!=1)
                {
                    Mat ScanROI_hsv;//待匹配的子图
                    ScanROI_hsv=Image_h(Rect(j,i,Ldisk,Ldisk));
                    Scalar tempVal = cv::mean( ScanROI_hsv );
                    T_h= tempVal.val[0];

                    ScanROI_hsv=Image_s(Rect(j,i,Ldisk,Ldisk));
                    tempVal = cv::mean( ScanROI_hsv );
                    T_s= tempVal.val[0];

                    ScanROI_hsv=Image_v(Rect(j,i,Ldisk,Ldisk));
                    tempVal = cv::mean( ScanROI_hsv );
                    T_v= tempVal.val[0];

                }

                for(int ii=0;ii<DiskNum;ii++)
                {
                    float* buffer;
                    buffer=MatchingValue[ii].ptr<float>(i);
                    //相似度计算
                    int tempbuffer=0;
                    for(int k=0;k<xnum;k++)
                    {
                        tempbuffer=tempbuffer+(P[ii][k]-T[k])*(P[ii][k]-T[k]);

                    }
                    buffer[j]=tempbuffer+(P_h[ii]-T_h)*(P_h[ii]-T_h)+(P_s[ii]-T_s)*(P_s[ii]-T_s)+(P_v[ii]-T_v)*(P_v[ii]-T_v)+(P_grayM[ii]-T_grayM)*(P_grayM[ii]-T_grayM)+(P_grayD[ii]-T_grayD)*(P_grayD[ii]-T_grayD);

                }

            }
        }

        for(int ii=0;ii<DiskNum;ii++)
        {
            for(i=0;i<CandidateNum;i++)
            {
                //1、选取候选匹配点
                minMaxLoc(MatchingValue[ii],&RminValue,&RmaxValue,&RminLoc,&RmaxLoc);
                Point BestMatch=RminLoc;
                Point Centerpoint=Point(BestMatch.x+R,BestMatch.y+R);

                //画圆
                circle(GrayImageDisp,Centerpoint,2,CV_RGB(255,255,255),2,8,0);
                Points[ii].push_back(Centerpoint);

                float* buffervalue=MatchingValue[ii].ptr<float>(BestMatch.y);
                buffervalue[BestMatch.x]=999999999;
            }
        }
        timeAnnulus=((double)getTickCount() - timeAnnulus)/getTickFrequency();//运行时间
        //qDebug("环投影匹配耗时：%f秒",timeAnnulus);


        double timeMNS;//MNS耗时
        timeMNS=static_cast<double>(getTickCount());
        //***************************************************
        //Neighbors Constraint Similarity确定最终匹配点
        //***************************************************
        Point BestPoints[DiskNum];//最佳匹配点
        int BestPointNum[DiskNum]={0};//最佳匹配点邻居数量
        Mat ImageFinalResult;//显示结果
        GrayImage.copyTo(ImageFinalResult);

        for(int ii=0;ii<DiskNum;ii++)
        {

            for(i=0;i<Points[ii].size();i++)
            {

                //邻居数量
                int PointNum=0;
                for(int jj=0;jj<DiskNum;jj++)
                {
                    //获取邻居点坐标
                    for(j=0;j<Points[jj].size();j++)
                    {
                        if(jj!=ii)
                        {
                            float distance=get_distance(Points[ii][i],Points[jj][j]);
                            float distemp=get_distance(DisksCenterPoints[ii],DisksCenterPoints[jj]);
                            distance=fabs(distance-distemp);
                            if(distance<Thresh_distance)
                            {
                                PointNum++;//第一多的邻居数量
                                break;
                            }
                        }

                    }

                    //获取最佳点坐标
                    if(PointNum>BestPointNum[ii])
                    {
                        BestPoints[ii]=Points[ii][i];
                        BestPointNum[ii]=PointNum;
                    }
                }

            }
            if(BestPointNum[ii]<Thresh_MBNSnum)  BestPoints[ii]=Point(0,0);//阈值：最多邻居数量不少于Thresh_MBNSnum

        }


        //****从精确匹配得到的WidthNum*HeightNum个点中确定最终匹配点****
        Point FirstBestPoint=BestPoints[0];
        int FirstBestPointNum=0;
        int FirstBestPointSort=0;

        for(i=0;i<DiskNum;i++)
        {
            int PointNum=0;

            PointNum=CountNeighbors(i, BestPoints, DiskNum,DisksCenterPoints, Thresh_distance);//Thresh_distance=R
            //获取最佳点
            if(PointNum>FirstBestPointNum)
            {
                FirstBestPoint=BestPoints[i];
                FirstBestPointNum=PointNum;
                FirstBestPointSort=i;
            }

        }

        //剔除错误点，与最佳匹配点是邻居的点认为是正确点
        Point RectUplPoint=(FirstBestPoint-DisksCenterPoints[FirstBestPointSort]);//矩形左上角坐标
        int rectupnum=1;
        for(i=0;i<DiskNum;i++)
        {
            if(BestPoints[i].x==0 && BestPoints[i].y==0)
            {
                BestPoints[i]=Point(0,0);
            }
            else
            {
                if(FirstBestPointSort!=i)
                {
                    float distance=get_distance(FirstBestPoint,BestPoints[i]);
                    float distemp=get_distance(DisksCenterPoints[FirstBestPointSort],DisksCenterPoints[i]);
                    distance=fabs(distance-distemp);//距离
                    if(distance>Thresh_distance)
                    {
                        BestPoints[i]=Point(0,0);
                    }
                    else
                    {
                        RectUplPoint=RectUplPoint+(BestPoints[i]-DisksCenterPoints[i]);
                        rectupnum++;
                    }
                }
            }


            circle(ImageFinalResult,BestPoints[i],2,CV_RGB(255,255,255),2,8,0);
        }

        timeMNS=((double)getTickCount() - timeMNS)/getTickFrequency();//运行时间
        qDebug("MNS_SSD耗时：%f秒",timeMNS);
        Mat ImageFinalResult2;
        GrayImage.copyTo(ImageFinalResult2);
        circle(ImageFinalResult2,FirstBestPoint,2,CV_RGB(255,255,255),2,8,0);

        RectUplPoint.x=(int)RectUplPoint.x/rectupnum;
        RectUplPoint.y=(int)RectUplPoint.y/rectupnum;
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)0);


    /*
        //利用2个最佳匹配点画矩形框
        //计算两向量夹角
        float AvectorAngle=AnglePolar(DisksCenterPoints[SecondBestPointSort]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
        float BvectorAngle=AnglePolar(SecondBestPoint-FirstBestPoint);//实际向量
        //两向量夹角0-360°
        float Sita=BvectorAngle-AvectorAngle;
        if(Sita<0) Sita=Sita+360;


        Point RectUplPoint;//矩形左上角坐标
        RectUplPoint.x=(int)(-DisksCenterPoints[FirstBestPointSort].x*cos(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*sin(Sita/180*3.14) + FirstBestPoint.x);
        RectUplPoint.y=(int)(-(-DisksCenterPoints[FirstBestPointSort].x)*sin(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*cos(Sita/180*3.14)  + FirstBestPoint.y);
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)Sita);
    */

        imshow("GrayImageDisp",GrayImageDisp);//局部特征粗匹配得到候选匹配点集
        imshow("ImageFinalResult",ImageFinalResult);//Most Neighbors Similarity: 获取9个最佳匹配点
        imshow("ImageFinalResult2",ImageFinalResult2);//Most Neighbors Similarity: 从9个匹配点中获取最佳的2个匹配点

        //***********************************************************************
        //***********************************************************************
        timeSum=((double)getTickCount() - timeSum)/getTickFrequency();//运行时间
        qDebug("模板匹配结束，总耗时：%f秒",timeSum);
        qDebug("***********************");


        //返回结果参数
        MatchUpLPoint[0]=RectUplPoint;
        MatchTime[0]=timeSum;
    }
    else
    {
        //返回结果参数
        MatchUpLPoint[0]=Point(0,0);
        MatchTime[0]=0;
    }

}

//特征：环投影均值、灰度均值、HSV各分量均值
void MBNShsv_SSDtMatching2(Mat Image,Mat TemplateImage, Point* MatchUpLPoint, float* MatchTime){

    //分离颜色通道
    Mat Image_h,Image_s,Image_v,Imagehsv;
    if(Image.channels()!=1)
    {
        //分离颜色通道HSV
        vector<Mat> hsv_vec;
        cvtColor(Image, Imagehsv, CV_BGR2HSV);
        split(Imagehsv, hsv_vec);
        Image_h = hsv_vec[0];
        Image_s = hsv_vec[1];
        Image_v = hsv_vec[2];
        Image_h.convertTo(Image_h, CV_8UC1);
        Image_s.convertTo(Image_s, CV_8UC1);
        Image_v.convertTo(Image_v, CV_8UC1);
    }

    int i,j;
    //一、图像和模板都转为单通道的灰度图
    Mat GrayImage;
    cvtColor(Image,GrayImage,CV_RGB2GRAY,1);
    Mat GrayTemplate;
    cvtColor(TemplateImage,GrayTemplate,CV_RGB2GRAY,1);
    qDebug("待匹配图宽高=【%d x %d】；模板图宽高=【%d x %d】",GrayImage.cols,GrayImage.rows,GrayTemplate.cols,GrayTemplate.rows);

    //二、将模板图分成多个Ldisk*Ldisk的小方块，每个方块圆心相距
    R=(int)Ldisk/2;//局部方块半径
    int tt=2;//初始距离倍数
    int disL=(int)Ldisk/tt;//每个方块圆心相距的距离
    int WidthNum=(int)GrayTemplate.cols/Ldisk*tt - tt;//一行的方块数
    int HeightNum=(int)GrayTemplate.rows/Ldisk*tt - tt;//一列的方块数
    if(Ldisk==9 || WidthNum*HeightNum<70)
    {
        while(WidthNum*HeightNum<70)
        {
            tt++;
            disL=(int)(Ldisk/tt);//每个方块圆心相距的距离
            WidthNum=(int)GrayTemplate.cols/Ldisk*tt - tt;//一行的方块数
            HeightNum=(int)GrayTemplate.rows/Ldisk*tt - tt;//一列的方块数
        }
    }
    Thresh_distance=(float)(PSMode*Ldisk/tt);//邻居距离阈值
    qDebug("邻居距离阈值=%f",Thresh_distance);
    qDebug("模板方块边长=%d",Ldisk);
    qDebug("模板方块数量=%d",WidthNum*HeightNum);

    //四（1）、建立环投影的环查找表
    int x[R]={0};//环距离
    x[0]=1;
    int xnum=1;
    for(i=0;i<R;i++)
    {
        x[i+1]=(int)(R-sqrt((R-x[i])*(R-x[i])-(2*R-1))+0.5);
        xnum++;
        if(x[i+1]<=x[i])
        {
            x[i+1]=0;
            xnum--;
            break;
        }
    }

    uint AnnulusTable[Ldisk*Ldisk];//环查找表
    uint anNumber[xnum]={0};//统计同个环的像素数量
    uint Rtable[Ldisk*Ldisk];//半径查找表
    uint rNumber[R+1]={0};//统计同个半径值的数量
    uint aNumber[360]={0};//统计同个角度值的数量
    uint Atable[Ldisk*Ldisk];//角度查找表
    double YdivX;//斜率
    int A=0;//角度
    int r;//半径

    for(i=0;i<Ldisk;i++)
    {
        for(j=0;j<Ldisk;j++)
        {
            r=(int)(sqrt((i-R)*(i-R)+(j-R)*(j-R)) + 0.5);

            //建立极坐标（半径和角度）查找表
            //角度转换
             if(j>R-1 && i==R)
             {
                 A=0;
             }
             else if(j<R && i==R)
             {
                 A=180;
             }
             else if(j==R && i<R)
             {
                 A=90;
             }
             else if(j==R && i>R)
             {
                 A=270;
             }
             else if(j>R && i<R)
             {
                 YdivX=(double)(R-i)/(j-R);
                 A=(int)(atan(YdivX)*180/3.14159);
             }
             else if(j<R && i<R)
             {
                 YdivX=(double)(R-j)/(R-i);
                 A=(int)(atan(YdivX)*180/3.14159)+90;
             }
             else if(j<R && i>R)
             {
                 YdivX=(double)(i-R)/(R-j);
                 A=(int)(atan(YdivX)*180/3.14159)+180;
             }
             else if(j>R && i>R)
             {
                 YdivX=(double)(j-R)/(i-R);
                 A=(int)(atan(YdivX)*180/3.14159)+270;
             }


             if(r>R)
             {
                 Rtable[i*Ldisk+j] = R+1;
                 Atable[i*Ldisk+j] = 361;
             }
             else
             {
                 Rtable[i*Ldisk+j] = r;
                 Atable[i*Ldisk+j] = A;
                 rNumber[r]++;
                 aNumber[A]++;
             }

            // **************************
            // **************************
            //建立环查找表
            if(r>R)
            {
                AnnulusTable[i*Ldisk+j] = xnum;
            }
            else
            {
                for(int ii=0;ii<xnum;ii++)
                {
                    if(r>(R-x[ii]))
                    {
                        AnnulusTable[i*Ldisk+j] = ii;
                        anNumber[ii]++;
                        break;
                    }
                }

                if(r<=(R-x[xnum-1]))
                {
                    AnnulusTable[i*Ldisk+j] = xnum-1;
                    anNumber[xnum-1]++;
                }
            }

        }
    }

    //四（2）、计算模板图的环投影向量
    vector<Mat> Disks;//方块
    vector<Point> DisksCenterPoints;//每个方块的中心坐标点
    int DiskNum=0;//真正的方块数量
    int iarray[WidthNum*HeightNum]={-1};//正真方块的序号
    for(i=0;i<WidthNum*HeightNum;i++)
    {
        Mat GrayTemplatetemp(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
        float Ptemp[xnum]={0};
        Anvector(GrayTemplatetemp,AnnulusTable,xnum,anNumber,Ptemp);
        float PtempAver=0;
        for(j=0;j<xnum;j++)
        {
            PtempAver=PtempAver+Ptemp[j];
        }
        PtempAver=PtempAver/xnum;
        float P_SSD=0;
        for(j=0;j<xnum;j++)
        {
            P_SSD=P_SSD+(Ptemp[j]-PtempAver)*(Ptemp[j]-PtempAver);
        }

        if(P_SSD>4*xnum)
        {
            Disks.push_back(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
            DisksCenterPoints.push_back(Point(((int)i%WidthNum)*disL+R,((int)i/WidthNum)*disL+R));
            DiskNum++;
            iarray[i]=i;
        }

    }
    qDebug("真正的方块数量= %d",DiskNum);

    //DiskNum<=30的话直接放弃匹配
    if(DiskNum>30)
    {
        //环投影向量
        float P[DiskNum][xnum]={0};//模板图环投影向量
        for(i=0;i<DiskNum;i++)
        {
            //环投影向量
            Anvector(Disks[i],AnnulusTable,xnum,anNumber,P[i]);

        }
        //添加颜色特征
        float P_h[DiskNum]={0};//模板图h分量均值
        float P_s[DiskNum]={0};//模板图s分量均值
        float P_v[DiskNum]={0};//模板图v分量均值
        if(TemplateImage.channels()!=1)
        {
            //分离颜色通道HSV
            Mat TemplateImage_h,TemplateImage_s,TemplateImage_v,TemplateImagehsv;
            vector<Mat> hsv_vec;
            cvtColor(TemplateImage, TemplateImagehsv, CV_BGR2HSV);
            split(TemplateImagehsv, hsv_vec);
            TemplateImage_h = hsv_vec[0];
            TemplateImage_s = hsv_vec[1];
            TemplateImage_v = hsv_vec[2];
            TemplateImage_h.convertTo(TemplateImage_h, CV_8UC1);
            TemplateImage_s.convertTo(TemplateImage_s, CV_8UC1);
            TemplateImage_v.convertTo(TemplateImage_v, CV_8UC1);

            int iii=0;
            for(i=0;i<WidthNum*HeightNum;i++)
            {
                if(iarray[i]==i)
                {
                    Mat Disks_h=TemplateImage_h(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    Scalar tempVal = cv::mean( Disks_h );
                    P_h[iii]= tempVal.val[0];

                    Mat Disks_s=TemplateImage_s(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = cv::mean( Disks_s );
                    P_s[iii]= tempVal.val[0];

                    Mat Disks_v=TemplateImage_v(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = cv::mean( Disks_v );
                    P_v[iii]= tempVal.val[0];

                    iii++;
                }
            }

        }
        //添加灰度均值和方差特征
        float P_grayM[DiskNum]={0};//模板图灰度均值
        float P_grayD[DiskNum]={0};//模板图灰度方差
        int iii=0;
        for(i=0;i<WidthNum*HeightNum;i++)
        {
            if(iarray[i]==i)
            {
                Mat Disks_gray=GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                Scalar Mean,dev;
                cv::meanStdDev(Disks_gray,Mean,dev);
                P_grayM[iii]= Mean.val[0];
                P_grayD[iii]= dev.val[0];

                iii++;
            }
        }

        //*****五、开始匹配************************************
        //*******************************************
        double timeSum;//总耗时
        timeSum=static_cast<double>(getTickCount());
        //***********************************************************
        double timeAnnulus;
        timeAnnulus=static_cast<double>(getTickCount());
        //设置圆投影搜索范围
        int LeftUpx=0;
        int LeftUpy=0;
        int RrightDownx=(int)(GrayImage.cols-Ldisk-1);
        int RrightDowny=(int)(GrayImage.rows-Ldisk-1);

        //获取并标出候选点位置
        double RminValue=0;
        Point RminLoc;
        double RmaxValue=0;
        Point RmaxLoc;
        Mat GrayImageDisp;
        GrayImage.copyTo(GrayImageDisp);

        int CandidateNum=MatchingNum2;
        vector<Point> Points[DiskNum];//精确匹配点坐标数组
        vector<Mat> MatchingValue;//环投影相似度值
        for(int ii=0;ii<DiskNum;ii++)
        {
            Mat MatchingValueTemp(GrayImage.rows-Ldisk,GrayImage.cols-Ldisk,CV_32FC1,Scalar(999999999));//float型单通道，每个点初始化
            MatchingValue.push_back(MatchingValueTemp);
        }
        for(i=LeftUpy;i<RrightDowny+1;i++)
        {

            for(j=LeftUpx;j<RrightDownx;j++)
            {

                Mat ScanROI;//待匹配的子图
                ScanROI=GrayImage(Rect(j,i,Ldisk,Ldisk));
                float T[xnum]={0};//模板图环投影向量
                Anvector(ScanROI,AnnulusTable,xnum,anNumber,T);
                Scalar Mean,dev;
                cv::meanStdDev(ScanROI,Mean,dev);
                float T_grayM= Mean.val[0];
                float T_grayD= dev.val[0];

                //添加颜色特征
                float T_h=0;//h分量均值
                float T_s=0;//s分量均值
                float T_v=0;//v分量均值
                if(Image.channels()!=1)
                {
                    Mat ScanROI_hsv;//待匹配的子图
                    ScanROI_hsv=Image_h(Rect(j,i,Ldisk,Ldisk));
                    Scalar tempVal = cv::mean( ScanROI_hsv );
                    T_h= tempVal.val[0];

                    ScanROI_hsv=Image_s(Rect(j,i,Ldisk,Ldisk));
                    tempVal = cv::mean( ScanROI_hsv );
                    T_s= tempVal.val[0];

                    ScanROI_hsv=Image_v(Rect(j,i,Ldisk,Ldisk));
                    tempVal = cv::mean( ScanROI_hsv );
                    T_v= tempVal.val[0];

                }

                for(int ii=0;ii<DiskNum;ii++)
                {
                    float* buffer;
                    buffer=MatchingValue[ii].ptr<float>(i);
                    //相似度计算
                    int tempbuffer=0;
                    for(int k=0;k<xnum;k++)
                    {
                        tempbuffer=tempbuffer+(P[ii][k]-T[k])*(P[ii][k]-T[k]);

                    }
                    buffer[j]=tempbuffer+(P_h[ii]-T_h)*(P_h[ii]-T_h)+(P_s[ii]-T_s)*(P_s[ii]-T_s)+(P_v[ii]-T_v)*(P_v[ii]-T_v)+(P_grayM[ii]-T_grayM)*(P_grayM[ii]-T_grayM)+(P_grayD[ii]-T_grayD)*(P_grayD[ii]-T_grayD);

                }

            }
        }

        for(int ii=0;ii<DiskNum;ii++)
        {
            for(i=0;i<CandidateNum;i++)
            {
                //1、选取候选匹配点
                minMaxLoc(MatchingValue[ii],&RminValue,&RmaxValue,&RminLoc,&RmaxLoc);
                Point BestMatch=RminLoc;
                Point Centerpoint=Point(BestMatch.x+R,BestMatch.y+R);

                //画圆
                circle(GrayImageDisp,Centerpoint,2,CV_RGB(255,255,255),2,8,0);
                Points[ii].push_back(Centerpoint);

                float* buffervalue=MatchingValue[ii].ptr<float>(BestMatch.y);
                buffervalue[BestMatch.x]=999999999;
            }
        }
        timeAnnulus=((double)getTickCount() - timeAnnulus)/getTickFrequency();//运行时间
        //qDebug("环投影匹配耗时：%f秒",timeAnnulus);


        double timeMBNS;//MBNS耗时
        timeMBNS=static_cast<double>(getTickCount());
        //***************************************************
        //Neighbors Constraint Similarity确定最终匹配点
        //***************************************************
        Point BestPoints[DiskNum];//最佳匹配点
        int BestPointNum[DiskNum]={0};//最佳匹配点邻居数量
        Mat ImageBestMatches;//最佳匹配点集
        Mat ImageFinalResult;//FinalOne的邻居点集
        GrayImage.copyTo(ImageFinalResult);
        GrayImage.copyTo(ImageBestMatches);

        for(int ii=0;ii<DiskNum;ii++)
        {

            for(i=0;i<Points[ii].size();i++)
            {

                float tempMinDis=999999999;

                //第二多的邻居数量
                int PointNum=0;
                Point Pointtemp(0,0);
                Point Pointss[DiskNum];
                for(j=0;j<DiskNum;j++)
                {
                    Pointss[j]=Point(0,0);
                    if(j==ii)   Pointss[j]=Points[ii][i];
                }

                for(int jj=0;jj<DiskNum;jj++)
                {
                    //获取邻居点坐标
                    for(j=0;j<Points[jj].size();j++)
                    {
                        if(jj!=ii)
                        {
                            float distance=get_distance(Points[ii][i],Points[jj][j]);
                            float distemp=get_distance(DisksCenterPoints[ii],DisksCenterPoints[jj]);
                            distance=fabs(distance-distemp);//距离
                            if(distance<tempMinDis)
                            {
                                tempMinDis=distance;
                                Pointtemp=Points[jj][j];
                            }
                        }

                    }
                    if(tempMinDis<Thresh_distance)
                    {
                        Pointss[jj]=Pointtemp;
                        PointNum++;//第一多的邻居数量
                    }
                    tempMinDis=999999999;

                }
                //阈值：第一多的邻居数量不少于Thresh_MBNSnum个
                if(PointNum>Thresh_MBNSnum)
                {
                    PointNum=0;
                    //计算第二多的邻居数量
                    int PointNumtemp=0;
                    for(int tt=0;tt<DiskNum;tt++)
                    {
                        if(tt!=ii)
                        {
                            PointNumtemp=CountNeighbors(tt, Pointss, DiskNum,DisksCenterPoints, Thresh_distance);
                            if(PointNum<PointNumtemp)
                            {
                                PointNum=PointNumtemp;//第二多的邻居数量
                            }
                        }
                    }

                    //获取最佳点坐标
                    if(PointNum>BestPointNum[ii])
                    {
                        BestPoints[ii]=Points[ii][i];
                        BestPointNum[ii]=PointNum;
                    }

                }

            }
            if(BestPointNum[ii]<Thresh_MBNSnum)  BestPoints[ii]=Point(0,0);//阈值：第二多邻居数量不少于Thresh_MBNSnum
            circle(ImageBestMatches,BestPoints[ii],2,CV_RGB(255,255,255),2,8,0);


        }

        //****从精确匹配得到的WidthNum*HeightNum个点中确定最终匹配点****
        Point FirstBestPoint=BestPoints[0];
        int FirstBestPointNum=0;
        int FirstBestPointSort=0;
        /*
        for(i=0;i<DiskNum;i++)
        {
            int PointNum=0;

            PointNum=CountNeighbors(i, BestPoints, DiskNum,DisksCenterPoints, Thresh_distance);//Thresh_distance=R
            //获取最佳点
            if(PointNum>FirstBestPointNum)
            {
                FirstBestPoint=BestPoints[i];
                FirstBestPointNum=PointNum;
                FirstBestPointSort=i;
            }


        }
        */
        for(int ii=0;ii<DiskNum;ii++)
        {

            int PointNum=0;//邻居数量总和
            Point Pointss[DiskNum];
            for(j=0;j<DiskNum;j++)
            {
                Pointss[j]=Point(0,0);
                if(j==ii)   Pointss[j]=BestPoints[ii];
            }

            for(int jj=0;jj<DiskNum;jj++)
            {
                //获取邻居点坐标
                if(jj!=ii)
                {
                    float distance=get_distance(BestPoints[ii],BestPoints[jj]);
                    float distemp=get_distance(DisksCenterPoints[ii],DisksCenterPoints[jj]);
                    distance=fabs(distance-distemp);//距离
                    if(distance<Thresh_distance)
                    {
                        Pointss[jj]=BestPoints[jj];
                        PointNum++;//第一多的邻居数量
                    }
                }
            }

            //******
            float PointNum1=PointNum;//第一多的邻居数量
            //计算第一多的邻居中的所有点的的邻居数量和
            int PointNumtemp=0;
            for(int tt=0;tt<DiskNum;tt++)
            {
                if(tt!=ii)
                {
                    PointNumtemp=CountNeighbors(tt, Pointss, DiskNum,DisksCenterPoints, Thresh_distance);
                    PointNum=PointNum+PointNumtemp;//邻居数量总和

                }
            }

            //获取最佳点坐标
            if(PointNum/PointNum1>FirstBestPointNum)
            {
                FirstBestPoint=BestPoints[ii];
                FirstBestPointNum=PointNum/PointNum1;
                FirstBestPointSort=ii;
            }
        }

        //剔除错误点，与最佳匹配点是邻居的点认为是正确点
        for(i=0;i<DiskNum;i++)
        {
            if(BestPoints[i].x==0 && BestPoints[i].y==0)
            {
                BestPoints[i]=Point(0,0);
            }
            else
            {
                float distance=get_distance(FirstBestPoint,BestPoints[i]);
                float distemp=get_distance(DisksCenterPoints[FirstBestPointSort],DisksCenterPoints[i]);
                distance=fabs(distance-distemp);//距离
                if(distance>Thresh_distance)
                {
                    BestPoints[i]=Point(0,0);
                }
            }
            circle(ImageFinalResult,BestPoints[i],2,CV_RGB(255,255,255),2,8,0);
        }

        //确定矩形框坐标
        Point RectUplPoint=Point(0,0);//矩形左上角坐标
        int rectupnum=0;
        for(i=0;i<DiskNum;i++)
        {
            if(BestPoints[i].x==0 && BestPoints[i].y==0)
            {
                BestPoints[i]=Point(0,0);
            }
            else
            {
                //计算每个正确点的邻居数量，此数量作为矩形框坐标计算的权重
                int PointNum=0;
                PointNum=CountNeighbors(i, BestPoints, DiskNum,DisksCenterPoints, Thresh_distance);//Thresh_distance=R

                RectUplPoint=RectUplPoint+PointNum*(BestPoints[i]-DisksCenterPoints[i]);
                rectupnum=rectupnum+PointNum;

            }

        }
        if(rectupnum==0) rectupnum=1;

        timeMBNS=((double)getTickCount() - timeMBNS)/getTickFrequency();//运行时间
        qDebug("MBNS耗时：%f秒",timeMBNS);
        Mat ImageFinalResult2;//FinalOne
        GrayImage.copyTo(ImageFinalResult2);
        circle(ImageFinalResult2,FirstBestPoint,2,CV_RGB(255,255,255),2,8,0);


        RectUplPoint.x=(int)RectUplPoint.x/rectupnum;
        RectUplPoint.y=(int)RectUplPoint.y/rectupnum;
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)0);


    /*
        //利用2个最佳匹配点画矩形框
        //计算两向量夹角
        float AvectorAngle=AnglePolar(DisksCenterPoints[SecondBestPointSort]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
        float BvectorAngle=AnglePolar(SecondBestPoint-FirstBestPoint);//实际向量
        //两向量夹角0-360°
        float Sita=BvectorAngle-AvectorAngle;
        if(Sita<0) Sita=Sita+360;


        Point RectUplPoint;//矩形左上角坐标
        RectUplPoint.x=(int)(-DisksCenterPoints[FirstBestPointSort].x*cos(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*sin(Sita/180*3.14) + FirstBestPoint.x);
        RectUplPoint.y=(int)(-(-DisksCenterPoints[FirstBestPointSort].x)*sin(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*cos(Sita/180*3.14)  + FirstBestPoint.y);
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)Sita);
    */

        //显示结果
        qDebug("最终匹配的矩形左上角坐标=(%d,%d);旋转角=%f°",RectUplPoint.x,RectUplPoint.y,0);
        imshow("ImageCandidates",GrayImageDisp);//局部特征粗匹配得到候选匹配点集
        imshow("ImageBestMatches",ImageBestMatches);
        imshow("ImageFinalResult",ImageFinalResult);//Most Neighbors Similarity: 获取9个最佳匹配点
        imshow("ImageFinalOne",ImageFinalResult2);//Most Neighbors Similarity: 从9个匹配点中获取最佳的2个匹配点

        //***********************************************************************
        //***********************************************************************
        timeSum=((double)getTickCount() - timeSum)/getTickFrequency();//运行时间
        qDebug("模板匹配结束，总耗时：%f秒",timeSum);
        qDebug("***********************");

    /*
        Mat srcImageDisp;
        //resize函数前加cv：：，因为Qt也有resize函数，故加cv：：加以区别，不然会出现错误
        int resizeW=(int)ImageFinalResult2.cols/(beta+1);
        int resizeH=(int)ImageFinalResult2.rows/(beta+1);
        cv::resize(ImageFinalResult2,srcImageDisp,Size(resizeW,resizeH),0,0,3);
        Imgview->imshow(srcImageDisp);//显示到绑定的控件上

    */

        //返回结果参数
        MatchUpLPoint[0]=RectUplPoint;
        MatchTime[0]=timeSum;
    }
    else
    {
        //返回结果参数
        MatchUpLPoint[0]=Point(0,0);
        MatchTime[0]=0;
    }

}

//特征：环投影均值、灰度均值、HSV各分量均值
void MBNShsv_SSDtMatching2_Fast(Mat Image,Mat TemplateImage, Point* MatchUpLPoint, float* MatchTime){

    Scalar temprg=cv::mean(Image);
    float mean_1=temprg.val[0];
    float mean_2=temprg.val[1];
    qDebug("mean_1=%f,mean_2=%f",mean_1,mean_2);
    int channels=3;
    if(fabs(mean_1-mean_2)<1)   channels=1;
    qDebug("channels=%d",channels);

    //分离颜色通道HSV
    Mat Image_h,Image_s,Image_v,Imagehsv;
    if(channels>=3)
    {
        vector<Mat> hsv_vec;
        cvtColor(Image, Imagehsv, CV_BGR2HSV);
        split(Imagehsv, hsv_vec);
        Image_h = hsv_vec[0];
        Image_s = hsv_vec[1];
        Image_v = hsv_vec[2];
        Image_h.convertTo(Image_h, CV_8UC1);
        Image_s.convertTo(Image_s, CV_8UC1);
        Image_v.convertTo(Image_v, CV_8UC1);
   }

    int i,j;
    //一、图像和模板都转为单通道的灰度图
    Mat GrayImage;
    cvtColor(Image,GrayImage,CV_RGB2GRAY,1);
    Mat GrayTemplate;
    cvtColor(TemplateImage,GrayTemplate,CV_RGB2GRAY,1);
    qDebug("待匹配图宽高=【%d x %d】；模板图宽高=【%d x %d】",GrayImage.cols,GrayImage.rows,GrayTemplate.cols,GrayTemplate.rows);

    //二、将模板图分成多个Ldisk*Ldisk的小方块，每个方块圆心相距
    R=(int)Ldisk/2;//局部方块半径
    int tt=2;//初始距离倍数
    int disL=(int)Ldisk/tt;//每个方块圆心相距的距离
    int WidthNum=(int)(GrayTemplate.cols-Ldisk)/disL+1;//一行的方块数
    int HeightNum=(int)(GrayTemplate.rows-Ldisk)/disL+1;//一列的方块数
    if(Ldisk==9 || WidthNum*HeightNum<70)
    {
        while(WidthNum*HeightNum<70)
        {
            tt++;
            disL=(int)(Ldisk/tt);//每个方块圆心相距的距离
            WidthNum=(int)(GrayTemplate.cols-Ldisk)/disL+1;//一行的方块数
            HeightNum=(int)(GrayTemplate.rows-Ldisk)/disL+1;//一列的方块数
        }
    }
    Thresh_distance=(float)(PSMode*disL);//邻居距离阈值
    qDebug("邻居距离阈值=%f",Thresh_distance);
    qDebug("模板方块边长=%d",Ldisk);
    qDebug("模板方块数量=%d",WidthNum*HeightNum);

    //四（1）、建立环投影的环查找表
    int x[R]={0};//环距离
    x[0]=1;
    int xnum=1;
    for(i=0;i<R;i++)
    {
        x[i+1]=(int)(R-sqrt((R-x[i])*(R-x[i])-(2*R-1))+0.5);
        xnum++;
        if(x[i+1]<=x[i])
        {
            x[i+1]=0;
            xnum--;
            break;
        }
    }

    uint AnnulusTable[Ldisk*Ldisk];//环查找表
    uint anNumber[xnum]={0};//统计同个环的像素数量
    uint Rtable[Ldisk*Ldisk];//半径查找表
    uint rNumber[R+1]={0};//统计同个半径值的数量
    uint aNumber[360]={0};//统计同个角度值的数量
    uint Atable[Ldisk*Ldisk];//角度查找表
    double YdivX;//斜率
    int A=0;//角度
    int r;//半径

    for(i=0;i<Ldisk;i++)
    {
        for(j=0;j<Ldisk;j++)
        {
            r=(int)(sqrt((i-R)*(i-R)+(j-R)*(j-R)) + 0.5);

            //建立极坐标（半径和角度）查找表
            //角度转换
             if(j>R-1 && i==R)
             {
                 A=0;
             }
             else if(j<R && i==R)
             {
                 A=180;
             }
             else if(j==R && i<R)
             {
                 A=90;
             }
             else if(j==R && i>R)
             {
                 A=270;
             }
             else if(j>R && i<R)
             {
                 YdivX=(double)(R-i)/(j-R);
                 A=(int)(atan(YdivX)*180/3.14159);
             }
             else if(j<R && i<R)
             {
                 YdivX=(double)(R-j)/(R-i);
                 A=(int)(atan(YdivX)*180/3.14159)+90;
             }
             else if(j<R && i>R)
             {
                 YdivX=(double)(i-R)/(R-j);
                 A=(int)(atan(YdivX)*180/3.14159)+180;
             }
             else if(j>R && i>R)
             {
                 YdivX=(double)(j-R)/(i-R);
                 A=(int)(atan(YdivX)*180/3.14159)+270;
             }


             if(r>R)
             {
                 Rtable[i*Ldisk+j] = R+1;
                 Atable[i*Ldisk+j] = 361;
             }
             else
             {
                 Rtable[i*Ldisk+j] = r;
                 Atable[i*Ldisk+j] = A;
                 rNumber[r]++;
                 aNumber[A]++;
             }

            // **************************
            // **************************
            //建立环查找表
            if(r>R)
            {
                AnnulusTable[i*Ldisk+j] = xnum;
            }
            else
            {
                for(int ii=0;ii<xnum;ii++)
                {
                    if(r>(R-x[ii]))
                    {
                        AnnulusTable[i*Ldisk+j] = ii;
                        anNumber[ii]++;
                        break;
                    }
                }

                if(r<=(R-x[xnum-1]))
                {
                    AnnulusTable[i*Ldisk+j] = xnum-1;
                    anNumber[xnum-1]++;
                }
            }

        }
    }

    //四（2）、计算模板图的环投影向量
    vector<Mat> Disks;//方块
    vector<Point> DisksCenterPoints;//每个方块的中心坐标点
    int DiskNum=0;//真正的方块数量
    int iarray[WidthNum*HeightNum]={-1};//正真方块的序号
    for(i=0;i<WidthNum*HeightNum;i++)
    {
        Mat GrayTemplatetemp(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
        float Ptemp[xnum]={0};
        Anvector(GrayTemplatetemp,AnnulusTable,xnum,anNumber,Ptemp);
        float PtempAver=0;
        for(j=0;j<xnum;j++)
        {
            PtempAver=PtempAver+Ptemp[j];
        }
        PtempAver=PtempAver/xnum;
        float P_SSD=0;
        for(j=0;j<xnum;j++)
        {
            P_SSD=P_SSD+(Ptemp[j]-PtempAver)*(Ptemp[j]-PtempAver);
        }

        if(P_SSD>4*xnum)
        {
            Disks.push_back(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
            DisksCenterPoints.push_back(Point(((int)i%WidthNum)*disL+R,((int)i/WidthNum)*disL+R));
            DiskNum++;
            iarray[i]=i;
        }

    }
    qDebug("真正的方块数量= %d",DiskNum);
    Thresh_MBNSnum=(int)(DiskNum*0.2);
    qDebug("Thresh_MBNSnum= %d",Thresh_MBNSnum);

    //DiskNum<=30的话直接放弃匹配
    if(DiskNum>30)
    {
        //环投影向量
        float P[DiskNum][xnum]={0};//模板图环投影向量
        float DisksCenterPointsDistance[DiskNum*DiskNum]={0};//各个方块圆心坐标距离的平方
        for(i=0;i<DiskNum;i++)
        {
            //环投影向量
            Anvector(Disks[i],AnnulusTable,xnum,anNumber,P[i]);
            //各个方块圆心坐标距离的平方
            for(j=0;j<DiskNum;j++)
            {
                DisksCenterPointsDistance[i*DiskNum+j]=get_distance(DisksCenterPoints[i],DisksCenterPoints[j]);
            }

        }
        //添加颜色特征
        float P_h[DiskNum]={0};//模板图h分量均值
        float P_s[DiskNum]={0};//模板图s分量均值
        float P_v[DiskNum]={0};//模板图v分量均值
        if(channels>=3)
        {
            //分离颜色通道HSV
            Mat TemplateImage_h,TemplateImage_s,TemplateImage_v,TemplateImagehsv;
            vector<Mat> hsv_vec;
            cvtColor(TemplateImage, TemplateImagehsv, CV_BGR2HSV);
            split(TemplateImagehsv, hsv_vec);
            TemplateImage_h = hsv_vec[0];
            TemplateImage_s = hsv_vec[1];
            TemplateImage_v = hsv_vec[2];
            TemplateImage_h.convertTo(TemplateImage_h, CV_8UC1);
            TemplateImage_s.convertTo(TemplateImage_s, CV_8UC1);
            TemplateImage_v.convertTo(TemplateImage_v, CV_8UC1);

            int iii=0;
            for(i=0;i<WidthNum*HeightNum;i++)
            {
                if(iarray[i]==i)
                {
                    Mat Disks_h=TemplateImage_h(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    Scalar tempVal = cv::mean( Disks_h );
                    P_h[iii]= tempVal.val[0];

                    Mat Disks_s=TemplateImage_s(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = cv::mean( Disks_s );
                    P_s[iii]= tempVal.val[0];

                    Mat Disks_v=TemplateImage_v(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = cv::mean( Disks_v );
                    P_v[iii]= tempVal.val[0];

                    iii++;
                }
            }

        }
        //添加灰度均值和方差特征
        float P_grayM[DiskNum]={0};//模板图灰度均值
        float P_grayD[DiskNum]={0};//模板图灰度方差
        int iii=0;
        for(i=0;i<WidthNum*HeightNum;i++)
        {
            if(iarray[i]==i)
            {
                Mat Disks_gray=GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                Scalar Mean,dev;
                cv::meanStdDev(Disks_gray,Mean,dev);
                P_grayM[iii]= Mean.val[0];
                P_grayD[iii]= dev.val[0];

                iii++;
            }
        }

        //*****五、开始匹配************************************
        //*******************************************
        double timeSum;//总耗时
        timeSum=static_cast<double>(getTickCount());
        //***********************************************************
        double timeAnnulus;
        timeAnnulus=static_cast<double>(getTickCount());
        //设置圆投影搜索范围
        int LeftUpx=0;
        int LeftUpy=0;
        int RrightDownx=(int)(GrayImage.cols-Ldisk-1);
        int RrightDowny=(int)(GrayImage.rows-Ldisk-1);

        //获取并标出候选点位置
        double RminValue=0;
        Point RminLoc;
        double RmaxValue=0;
        Point RmaxLoc;
        Mat GrayImageDisp;
        GrayImage.copyTo(GrayImageDisp);

        int CandidateNum=MatchingNum2;
        vector<Point> Points[DiskNum];//精确匹配点坐标数组
        vector<Mat> MatchingValue;//环投影相似度值
        for(int ii=0;ii<DiskNum;ii++)
        {
            Mat MatchingValueTemp(GrayImage.rows-Ldisk,GrayImage.cols-Ldisk,CV_32FC1,Scalar(999999999));//float型单通道，每个点初始化
            MatchingValue.push_back(MatchingValueTemp);
        }
        for(i=LeftUpy;i<RrightDowny+1;i++)
        {

            for(j=LeftUpx;j<RrightDownx;j++)
            {

                Mat ScanROI;//待匹配的子图
                ScanROI=GrayImage(Rect(j,i,Ldisk,Ldisk));
                float T[xnum]={0};//模板图环投影向量
                Anvector(ScanROI,AnnulusTable,xnum,anNumber,T);
                Scalar Mean,dev;
                cv::meanStdDev(ScanROI,Mean,dev);
                float T_grayM= Mean.val[0];
                float T_grayD= dev.val[0];

                //添加颜色特征
                float T_h=0;//h分量均值
                float T_s=0;//s分量均值
                float T_v=0;//v分量均值
                if(channels>=3)
                {
                    Mat ScanROI_hsv;//待匹配的子图
                    ScanROI_hsv=Image_h(Rect(j,i,Ldisk,Ldisk));
                    Scalar tempVal = cv::mean( ScanROI_hsv );
                    T_h= tempVal.val[0];

                    ScanROI_hsv=Image_s(Rect(j,i,Ldisk,Ldisk));
                    tempVal = cv::mean( ScanROI_hsv );
                    T_s= tempVal.val[0];

                    ScanROI_hsv=Image_v(Rect(j,i,Ldisk,Ldisk));
                    tempVal = cv::mean( ScanROI_hsv );
                    T_v= tempVal.val[0];

                }


                for(int ii=0;ii<DiskNum;ii++)
                {
                    float d_h=fabs(P_h[ii]-T_h);
                    float d_s=fabs(P_s[ii]-T_s);
                    float d_v=fabs(P_v[ii]-T_v);
                    float d_d=fabs(P_grayD[ii]-T_grayD);
                    float d_m=fabs(P_grayM[ii]-T_grayM);

                    if((d_h<20) && (d_s<40) && (d_v<60) && (d_d<20) && (d_m<60))
                    {
                        float* buffer;
                        buffer=MatchingValue[ii].ptr<float>(i);
                        //相似度计算
                        int tempbuffer=0;
                        for(int k=0;k<xnum;k++)
                        {
                            tempbuffer=tempbuffer+(P[ii][k]-T[k])*(P[ii][k]-T[k]);
                            //tempbuffer=tempbuffer+fabs(P[ii][k]-T[k]);

                        }
                        if(channels>=3)
                        {
                            //buffer[j]=tempbuffer+(P_h[ii]-T_h)*(P_h[ii]-T_h)+(P_s[ii]-T_s)*(P_s[ii]-T_s)+(P_v[ii]-T_v)*(P_v[ii]-T_v)+(P_grayM[ii]-T_grayM)*(P_grayM[ii]-T_grayM);//+(P_grayD[ii]-T_grayD)*(P_grayD[ii]-T_grayD);
                            buffer[j]=tempbuffer+d_h*d_h+d_s*d_s+d_v*d_v+d_m*d_m;
                        }
                        else
                        {
                            buffer[j]=tempbuffer;
                        }
                    }
                }

            }
        }

        for(int ii=0;ii<DiskNum;ii++)
        {
            for(i=0;i<CandidateNum;i++)
            {
                //1、选取候选匹配点
                minMaxLoc(MatchingValue[ii],&RminValue,&RmaxValue,&RminLoc,&RmaxLoc);
                Point BestMatch=RminLoc;
                Point Centerpoint;
                if(RminValue==RmaxValue)
                {
                    Centerpoint=Point(0,0);
                }
                else
                {
                    Centerpoint=Point(BestMatch.x+R,BestMatch.y+R);
                }

                //画圆
                circle(GrayImageDisp,Centerpoint,2,CV_RGB(255,255,255),2,8,0);
                Points[ii].push_back(Centerpoint);

                float* buffervalue=MatchingValue[ii].ptr<float>(BestMatch.y);
                buffervalue[BestMatch.x]=999999999;
            }
        }
        timeAnnulus=((double)getTickCount() - timeAnnulus)/getTickFrequency();//运行时间
        //qDebug("环投影匹配耗时：%f秒",timeAnnulus);


        double timeMBNS;//MBNS耗时
        timeMBNS=static_cast<double>(getTickCount());
        //***************************************************
        //Neighbors Constraint Similarity确定最终匹配点
        //***************************************************
        Point BestPoints[DiskNum];//最佳匹配点
        int BestPointNum[DiskNum]={0};//最佳匹配点邻居数量
        Mat ImageBestMatches;//最佳匹配点集
        Mat ImageFinalResult;//FinalOne的邻居点集
        GrayImage.copyTo(ImageFinalResult);
        GrayImage.copyTo(ImageBestMatches);

        for(int ii=0;ii<DiskNum;ii++)
        {

            for(i=0;i<Points[ii].size();i++)
            {

                //第二多的邻居数量
                int PointNum=0;
                Point Pointss[DiskNum];
                for(j=0;j<DiskNum;j++)
                {
                    Pointss[j]=Point(0,0);
                    if(j==ii)   Pointss[j]=Points[ii][i];
                }

                for(int jj=0;jj<DiskNum;jj++)
                {
                    float distemp=DisksCenterPointsDistance[ii*DiskNum+jj];
                    //获取邻居点坐标
                    for(j=0;j<Points[jj].size();j++)
                    {
                        if(jj!=ii)
                        {
                            float distance=get_distance(Points[ii][i],Points[jj][j]);
                            distance=fabs(distance-distemp);//距离
                            if(distance<Thresh_distance)
                            {
                                Pointss[jj]=Points[jj][j];
                                PointNum++;//第一多的邻居数量
                                break;
                            }
                        }

                    }

                }
                //阈值：第一多的邻居数量不少于Thresh_MBNSnum个
                if(PointNum>Thresh_MBNSnum)
                {
                    PointNum=0;
                    //计算第二多的邻居数量
                    int PointNumtemp=0;
                    for(int tt=0;tt<DiskNum;tt++)
                    {
                        if(tt!=ii)
                        {
                            PointNumtemp=CountNeighborsFast(tt, Pointss, DiskNum,DisksCenterPointsDistance, Thresh_distance);
                            if(PointNum<PointNumtemp)
                            {
                                PointNum=PointNumtemp;//第二多的邻居数量
                            }
                        }
                    }

                    //获取最佳点坐标
                    if(PointNum>BestPointNum[ii])
                    {
                        BestPoints[ii]=Points[ii][i];
                        BestPointNum[ii]=PointNum;
                    }

                }

            }
            if(BestPointNum[ii]<Thresh_MBNSnum)  BestPoints[ii]=Point(0,0);//阈值：第二多邻居数量不少于Thresh_MBNSnum
            circle(ImageBestMatches,BestPoints[ii],2,CV_RGB(255,255,255),2,8,0);


        }

        //****从精确匹配得到的WidthNum*HeightNum个点中确定最终匹配点****
        Point FirstBestPoint=BestPoints[0];
        float FirstBestPointNum=0;
        int FirstBestPointSort=0;
        /*
        for(i=0;i<DiskNum;i++)
        {
            int PointNum=0;

            PointNum=CountNeighbors(i, BestPoints, DiskNum,DisksCenterPoints, Thresh_distance);//Thresh_distance=R
            //获取最佳点
            if(PointNum>FirstBestPointNum)
            {
                FirstBestPoint=BestPoints[i];
                FirstBestPointNum=PointNum;
                FirstBestPointSort=i;
            }


        }
        */
        for(int ii=0;ii<DiskNum;ii++)
        {

            if(BestPoints[ii]==Point(0,0))
            {
                continue;
            }
            else
            {
                int PointNum=0;//邻居数量总和
                Point Pointss[DiskNum];
                for(j=0;j<DiskNum;j++)
                {
                    Pointss[j]=Point(0,0);
                    if(j==ii)   Pointss[j]=BestPoints[ii];
                }

                for(int jj=0;jj<DiskNum;jj++)
                {

                    //获取邻居点坐标
                    if(jj!=ii)
                    {
                        float distance=get_distance(BestPoints[ii],BestPoints[jj]);
                        float distemp=DisksCenterPointsDistance[ii*DiskNum+jj];
                        distance=fabs(distance-distemp);//距离
                        if(distance<Thresh_distance)
                        {
                            Pointss[jj]=BestPoints[jj];
                            PointNum++;//第一多的邻居数量
                        }
                    }
                }

                //******
                float PointNum1=PointNum;//第一多的邻居数量
                //计算第一多的邻居中的所有点的的邻居数量和
                int PointNumtemp=0;
                for(int tt=0;tt<DiskNum;tt++)
                {
                    if(tt!=ii)
                    {
                        PointNumtemp=CountNeighborsFast(tt, Pointss, DiskNum,DisksCenterPointsDistance, Thresh_distance);
                        PointNum=PointNum+PointNumtemp;
                    }
                }

                //获取最佳点坐标
                if((float)PointNum/PointNum1>FirstBestPointNum)
                {
                    FirstBestPoint=BestPoints[ii];
                    FirstBestPointNum=(float)PointNum/PointNum1;
                    FirstBestPointSort=ii;
                }
            }
        }

        //剔除错误点，与最佳匹配点是邻居的点认为是正确点
        for(i=0;i<DiskNum;i++)
        {
            if(BestPoints[i].x==0 && BestPoints[i].y==0)
            {
                BestPoints[i]=Point(0,0);
            }
            else
            {
                float distance=get_distance(FirstBestPoint,BestPoints[i]);
                float distemp=DisksCenterPointsDistance[FirstBestPointSort*DiskNum+i];
                distance=fabs(distance-distemp);//距离
                if(distance>Thresh_distance/2)
                {
                    BestPoints[i]=Point(0,0);
                }
            }
            circle(ImageFinalResult,BestPoints[i],2,CV_RGB(255,255,255),2,8,0);
        }

        //确定矩形框坐标
        Point RectUplPoint=Point(0,0);//矩形左上角坐标
        int rectupnum=0;
        for(i=0;i<DiskNum;i++)
        {
            if(BestPoints[i].x==0 && BestPoints[i].y==0)
            {
                BestPoints[i]=Point(0,0);
            }
            else
            {
                //计算每个正确点的邻居数量，此数量作为矩形框坐标计算的权重
                int PointNum=0;
                PointNum=CountNeighborsFast(i, BestPoints, DiskNum,DisksCenterPointsDistance, Thresh_distance);//Thresh_distance=R

                RectUplPoint=RectUplPoint+PointNum*(BestPoints[i]-DisksCenterPoints[i]);
                rectupnum=rectupnum+PointNum;

            }

        }
        if(rectupnum==0) rectupnum=1;

        timeMBNS=((double)getTickCount() - timeMBNS)/getTickFrequency();//运行时间
        qDebug("MBNS耗时：%f秒",timeMBNS);
        Mat ImageFinalResult2;//FinalOne
        GrayImage.copyTo(ImageFinalResult2);
        circle(ImageFinalResult2,FirstBestPoint,2,CV_RGB(255,255,255),2,8,0);


        RectUplPoint.x=(int)RectUplPoint.x/rectupnum;
        RectUplPoint.y=(int)RectUplPoint.y/rectupnum;
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)0);

        //***********************************************************************
        //***********************************************************************
        timeSum=((double)getTickCount() - timeSum)/getTickFrequency();//运行时间
        qDebug("模板匹配结束，总耗时：%f秒",timeSum);
        qDebug("***********************");


    /*
        //利用2个最佳匹配点画矩形框
        //计算两向量夹角
        float AvectorAngle=AnglePolar(DisksCenterPoints[SecondBestPointSort]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
        float BvectorAngle=AnglePolar(SecondBestPoint-FirstBestPoint);//实际向量
        //两向量夹角0-360°
        float Sita=BvectorAngle-AvectorAngle;
        if(Sita<0) Sita=Sita+360;


        Point RectUplPoint;//矩形左上角坐标
        RectUplPoint.x=(int)(-DisksCenterPoints[FirstBestPointSort].x*cos(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*sin(Sita/180*3.14) + FirstBestPoint.x);
        RectUplPoint.y=(int)(-(-DisksCenterPoints[FirstBestPointSort].x)*sin(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*cos(Sita/180*3.14)  + FirstBestPoint.y);
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)Sita);
    */
/*
        //显示结果
        qDebug("最终匹配的矩形左上角坐标=(%d,%d);旋转角=%f°",RectUplPoint.x,RectUplPoint.y,0);
        imshow("ImageCandidates",GrayImageDisp);//局部特征粗匹配得到候选匹配点集
        imshow("ImageBestMatches",ImageBestMatches);
        imshow("ImageFinalResult",ImageFinalResult);//Most Neighbors Similarity: 获取9个最佳匹配点
        imshow("ImageFinalOne",ImageFinalResult2);//Most Neighbors Similarity: 从9个匹配点中获取最佳的2个匹配点

*/

    /*
        Mat srcImageDisp;
        //resize函数前加cv：：，因为Qt也有resize函数，故加cv：：加以区别，不然会出现错误
        int resizeW=(int)ImageFinalResult2.cols/(beta+1);
        int resizeH=(int)ImageFinalResult2.rows/(beta+1);
        cv::resize(ImageFinalResult2,srcImageDisp,Size(resizeW,resizeH),0,0,3);
        Imgview->imshow(srcImageDisp);//显示到绑定的控件上

    */

        //返回结果参数
        MatchUpLPoint[0]=RectUplPoint;
        MatchTime[0]=timeSum;

        //MBNS耗时
        MNS_second_SumTime=MNS_second_SumTime+timeMBNS;
    }
    else
    {
        //返回结果参数
        MatchUpLPoint[0]=Point(0,0);
        MatchTime[0]=0;
    }

}

//特征：环投影均值、灰度均值、HSV各分量均值
void MNShsv_SSDtMatching2_Fast(Mat Image,Mat TemplateImage, Point* MatchUpLPoint, float* MatchTime){

    Scalar temprg=cv::mean(Image);
    float mean_1=temprg.val[0];
    float mean_2=temprg.val[1];
    qDebug("mean_1=%f,mean_2=%f",mean_1,mean_2);
    int channels=3;
    if(fabs(mean_1-mean_2)<1)   channels=1;
    qDebug("channels=%d",channels);

    //分离颜色通道HSV
    Mat Image_h,Image_s,Image_v,Imagehsv;
    if(channels>=3)
    {
        vector<Mat> hsv_vec;
        cvtColor(Image, Imagehsv, CV_BGR2HSV);
        split(Imagehsv, hsv_vec);
        Image_h = hsv_vec[0];
        Image_s = hsv_vec[1];
        Image_v = hsv_vec[2];
        Image_h.convertTo(Image_h, CV_8UC1);
        Image_s.convertTo(Image_s, CV_8UC1);
        Image_v.convertTo(Image_v, CV_8UC1);
   }

    int i,j;
    //一、图像和模板都转为单通道的灰度图
    Mat GrayImage;
    cvtColor(Image,GrayImage,CV_RGB2GRAY,1);
    Mat GrayTemplate;
    cvtColor(TemplateImage,GrayTemplate,CV_RGB2GRAY,1);
    qDebug("待匹配图宽高=【%d x %d】；模板图宽高=【%d x %d】",GrayImage.cols,GrayImage.rows,GrayTemplate.cols,GrayTemplate.rows);

    //二、将模板图分成多个Ldisk*Ldisk的小方块，每个方块圆心相距
    R=(int)Ldisk/2;//局部方块半径
    int tt=2;//初始距离倍数
    int disL=(int)Ldisk/tt;//每个方块圆心相距的距离
    int WidthNum=(int)(GrayTemplate.cols-Ldisk)/disL+1;//一行的方块数
    int HeightNum=(int)(GrayTemplate.rows-Ldisk)/disL+1;//一列的方块数
    if(Ldisk==9 || WidthNum*HeightNum<70)
    {
        while(WidthNum*HeightNum<70)
        {
            tt++;
            disL=(int)(Ldisk/tt);//每个方块圆心相距的距离
            WidthNum=(int)(GrayTemplate.cols-Ldisk)/disL+1;//一行的方块数
            HeightNum=(int)(GrayTemplate.rows-Ldisk)/disL+1;//一列的方块数
        }
    }
    Thresh_distance=(float)(PSMode*disL);//邻居距离阈值
    qDebug("邻居距离阈值=%f",Thresh_distance);
    qDebug("模板方块边长=%d",Ldisk);
    qDebug("模板方块数量=%d",WidthNum*HeightNum);

    //四（1）、建立环投影的环查找表
    int x[R]={0};//环距离
    x[0]=1;
    int xnum=1;
    for(i=0;i<R;i++)
    {
        x[i+1]=(int)(R-sqrt((R-x[i])*(R-x[i])-(2*R-1))+0.5);
        xnum++;
        if(x[i+1]<=x[i])
        {
            x[i+1]=0;
            xnum--;
            break;
        }
    }

    uint AnnulusTable[Ldisk*Ldisk];//环查找表
    uint anNumber[xnum]={0};//统计同个环的像素数量
    uint Rtable[Ldisk*Ldisk];//半径查找表
    uint rNumber[R+1]={0};//统计同个半径值的数量
    uint aNumber[360]={0};//统计同个角度值的数量
    uint Atable[Ldisk*Ldisk];//角度查找表
    double YdivX;//斜率
    int A=0;//角度
    int r;//半径

    for(i=0;i<Ldisk;i++)
    {
        for(j=0;j<Ldisk;j++)
        {
            r=(int)(sqrt((i-R)*(i-R)+(j-R)*(j-R)) + 0.5);

            //建立极坐标（半径和角度）查找表
            //角度转换
             if(j>R-1 && i==R)
             {
                 A=0;
             }
             else if(j<R && i==R)
             {
                 A=180;
             }
             else if(j==R && i<R)
             {
                 A=90;
             }
             else if(j==R && i>R)
             {
                 A=270;
             }
             else if(j>R && i<R)
             {
                 YdivX=(double)(R-i)/(j-R);
                 A=(int)(atan(YdivX)*180/3.14159);
             }
             else if(j<R && i<R)
             {
                 YdivX=(double)(R-j)/(R-i);
                 A=(int)(atan(YdivX)*180/3.14159)+90;
             }
             else if(j<R && i>R)
             {
                 YdivX=(double)(i-R)/(R-j);
                 A=(int)(atan(YdivX)*180/3.14159)+180;
             }
             else if(j>R && i>R)
             {
                 YdivX=(double)(j-R)/(i-R);
                 A=(int)(atan(YdivX)*180/3.14159)+270;
             }


             if(r>R)
             {
                 Rtable[i*Ldisk+j] = R+1;
                 Atable[i*Ldisk+j] = 361;
             }
             else
             {
                 Rtable[i*Ldisk+j] = r;
                 Atable[i*Ldisk+j] = A;
                 rNumber[r]++;
                 aNumber[A]++;
             }

            // **************************
            // **************************
            //建立环查找表
            if(r>R)
            {
                AnnulusTable[i*Ldisk+j] = xnum;
            }
            else
            {
                for(int ii=0;ii<xnum;ii++)
                {
                    if(r>(R-x[ii]))
                    {
                        AnnulusTable[i*Ldisk+j] = ii;
                        anNumber[ii]++;
                        break;
                    }
                }

                if(r<=(R-x[xnum-1]))
                {
                    AnnulusTable[i*Ldisk+j] = xnum-1;
                    anNumber[xnum-1]++;
                }
            }

        }
    }

    //四（2）、计算模板图的环投影向量
    vector<Mat> Disks;//方块
    vector<Point> DisksCenterPoints;//每个方块的中心坐标点
    int DiskNum=0;//真正的方块数量
    int iarray[WidthNum*HeightNum]={-1};//正真方块的序号
    for(i=0;i<WidthNum*HeightNum;i++)
    {
        Mat GrayTemplatetemp(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
        float Ptemp[xnum]={0};
        Anvector(GrayTemplatetemp,AnnulusTable,xnum,anNumber,Ptemp);
        float PtempAver=0;
        for(j=0;j<xnum;j++)
        {
            PtempAver=PtempAver+Ptemp[j];
        }
        PtempAver=PtempAver/xnum;
        float P_SSD=0;
        for(j=0;j<xnum;j++)
        {
            P_SSD=P_SSD+(Ptemp[j]-PtempAver)*(Ptemp[j]-PtempAver);
        }

        if(P_SSD>4*xnum)
        {
            Disks.push_back(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
            DisksCenterPoints.push_back(Point(((int)i%WidthNum)*disL+R,((int)i/WidthNum)*disL+R));
            DiskNum++;
            iarray[i]=i;
        }

    }
    qDebug("真正的方块数量= %d",DiskNum);
    Thresh_MBNSnum=(int)(DiskNum*0.2);
    qDebug("Thresh_MBNSnum= %d",Thresh_MBNSnum);

    //DiskNum<=30的话直接放弃匹配
    if(DiskNum>30)
    {
        //环投影向量
        float P[DiskNum][xnum]={0};//模板图环投影向量
        float DisksCenterPointsDistance[DiskNum*DiskNum]={0};//各个方块圆心坐标距离的平方
        for(i=0;i<DiskNum;i++)
        {
            //环投影向量
            Anvector(Disks[i],AnnulusTable,xnum,anNumber,P[i]);
            //各个方块圆心坐标距离的平方
            for(j=0;j<DiskNum;j++)
            {
                DisksCenterPointsDistance[i*DiskNum+j]=get_distance(DisksCenterPoints[i],DisksCenterPoints[j]);
            }

        }
        //添加颜色特征
        float P_h[DiskNum]={0};//模板图h分量均值
        float P_s[DiskNum]={0};//模板图s分量均值
        float P_v[DiskNum]={0};//模板图v分量均值
        if(channels>=3)
        {
            //分离颜色通道HSV
            Mat TemplateImage_h,TemplateImage_s,TemplateImage_v,TemplateImagehsv;
            vector<Mat> hsv_vec;
            cvtColor(TemplateImage, TemplateImagehsv, CV_BGR2HSV);
            split(TemplateImagehsv, hsv_vec);
            TemplateImage_h = hsv_vec[0];
            TemplateImage_s = hsv_vec[1];
            TemplateImage_v = hsv_vec[2];
            TemplateImage_h.convertTo(TemplateImage_h, CV_8UC1);
            TemplateImage_s.convertTo(TemplateImage_s, CV_8UC1);
            TemplateImage_v.convertTo(TemplateImage_v, CV_8UC1);

            int iii=0;
            for(i=0;i<WidthNum*HeightNum;i++)
            {
                if(iarray[i]==i)
                {
                    Mat Disks_h=TemplateImage_h(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    Scalar tempVal = cv::mean( Disks_h );
                    P_h[iii]= tempVal.val[0];

                    Mat Disks_s=TemplateImage_s(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = cv::mean( Disks_s );
                    P_s[iii]= tempVal.val[0];

                    Mat Disks_v=TemplateImage_v(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = cv::mean( Disks_v );
                    P_v[iii]= tempVal.val[0];

                    iii++;
                }
            }

        }
        //添加灰度均值和方差特征
        float P_grayM[DiskNum]={0};//模板图灰度均值
        float P_grayD[DiskNum]={0};//模板图灰度方差
        int iii=0;
        for(i=0;i<WidthNum*HeightNum;i++)
        {
            if(iarray[i]==i)
            {
                Mat Disks_gray=GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                Scalar Mean,dev;
                cv::meanStdDev(Disks_gray,Mean,dev);
                P_grayM[iii]= Mean.val[0];
                P_grayD[iii]= dev.val[0];

                iii++;
            }
        }

        //*****五、开始匹配************************************
        //*******************************************
        double timeSum;//总耗时
        timeSum=static_cast<double>(getTickCount());
        //***********************************************************
        double timeAnnulus;
        timeAnnulus=static_cast<double>(getTickCount());
        //设置圆投影搜索范围
        int LeftUpx=0;
        int LeftUpy=0;
        int RrightDownx=(int)(GrayImage.cols-Ldisk-1);
        int RrightDowny=(int)(GrayImage.rows-Ldisk-1);

        //获取并标出候选点位置
        double RminValue=0;
        Point RminLoc;
        double RmaxValue=0;
        Point RmaxLoc;
        Mat GrayImageDisp;
        GrayImage.copyTo(GrayImageDisp);

        int CandidateNum=MatchingNum2;
        vector<Point> Points[DiskNum];//精确匹配点坐标数组
        vector<Mat> MatchingValue;//环投影相似度值
        for(int ii=0;ii<DiskNum;ii++)
        {
            Mat MatchingValueTemp(GrayImage.rows-Ldisk,GrayImage.cols-Ldisk,CV_32FC1,Scalar(999999999));//float型单通道，每个点初始化
            MatchingValue.push_back(MatchingValueTemp);
        }
        for(i=LeftUpy;i<RrightDowny+1;i++)
        {

            for(j=LeftUpx;j<RrightDownx;j++)
            {

                Mat ScanROI;//待匹配的子图
                ScanROI=GrayImage(Rect(j,i,Ldisk,Ldisk));
                float T[xnum]={0};//模板图环投影向量
                Anvector(ScanROI,AnnulusTable,xnum,anNumber,T);
                Scalar Mean,dev;
                cv::meanStdDev(ScanROI,Mean,dev);
                float T_grayM= Mean.val[0];
                float T_grayD= dev.val[0];

                //添加颜色特征
                float T_h=0;//h分量均值
                float T_s=0;//s分量均值
                float T_v=0;//v分量均值
                if(channels>=3)
                {
                    Mat ScanROI_hsv;//待匹配的子图
                    ScanROI_hsv=Image_h(Rect(j,i,Ldisk,Ldisk));
                    Scalar tempVal = cv::mean( ScanROI_hsv );
                    T_h= tempVal.val[0];

                    ScanROI_hsv=Image_s(Rect(j,i,Ldisk,Ldisk));
                    tempVal = cv::mean( ScanROI_hsv );
                    T_s= tempVal.val[0];

                    ScanROI_hsv=Image_v(Rect(j,i,Ldisk,Ldisk));
                    tempVal = cv::mean( ScanROI_hsv );
                    T_v= tempVal.val[0];

                }


                for(int ii=0;ii<DiskNum;ii++)
                {
                    float d_h=fabs(P_h[ii]-T_h);
                    float d_s=fabs(P_s[ii]-T_s);
                    float d_v=fabs(P_v[ii]-T_v);
                    float d_d=fabs(P_grayD[ii]-T_grayD);
                    float d_m=fabs(P_grayM[ii]-T_grayM);

                    if((d_h<20) && (d_s<40) && (d_v<60) && (d_d<20) && (d_m<60))
                    {
                        float* buffer;
                        buffer=MatchingValue[ii].ptr<float>(i);
                        //相似度计算
                        int tempbuffer=0;
                        for(int k=0;k<xnum;k++)
                        {
                            tempbuffer=tempbuffer+(P[ii][k]-T[k])*(P[ii][k]-T[k]);
                            //tempbuffer=tempbuffer+fabs(P[ii][k]-T[k]);

                        }
                        if(channels>=3)
                        {
                            //buffer[j]=tempbuffer+(P_h[ii]-T_h)*(P_h[ii]-T_h)+(P_s[ii]-T_s)*(P_s[ii]-T_s)+(P_v[ii]-T_v)*(P_v[ii]-T_v)+(P_grayM[ii]-T_grayM)*(P_grayM[ii]-T_grayM);//+(P_grayD[ii]-T_grayD)*(P_grayD[ii]-T_grayD);
                            buffer[j]=tempbuffer+d_h*d_h+d_s*d_s+d_v*d_v+d_m*d_m;
                        }
                        else
                        {
                            buffer[j]=tempbuffer;
                        }
                    }
                }

            }
        }

        for(int ii=0;ii<DiskNum;ii++)
        {
            for(i=0;i<CandidateNum;i++)
            {
                //1、选取候选匹配点
                minMaxLoc(MatchingValue[ii],&RminValue,&RmaxValue,&RminLoc,&RmaxLoc);
                Point BestMatch=RminLoc;
                Point Centerpoint;
                if(RminValue==RmaxValue)
                {
                    Centerpoint=Point(0,0);
                }
                else
                {
                    Centerpoint=Point(BestMatch.x+R,BestMatch.y+R);
                }

                //画圆
                circle(GrayImageDisp,Centerpoint,2,CV_RGB(255,255,255),2,8,0);
                Points[ii].push_back(Centerpoint);

                float* buffervalue=MatchingValue[ii].ptr<float>(BestMatch.y);
                buffervalue[BestMatch.x]=999999999;
            }
        }
        timeAnnulus=((double)getTickCount() - timeAnnulus)/getTickFrequency();//运行时间
        //qDebug("环投影匹配耗时：%f秒",timeAnnulus);


        double timeMBNS;//MBNS耗时
        timeMBNS=static_cast<double>(getTickCount());
        //***************************************************
        //Neighbors Constraint Similarity确定最终匹配点
        //***************************************************
        Point BestPoints[DiskNum];//最佳匹配点
        int BestPointNum[DiskNum]={0};//最佳匹配点邻居数量
        Mat ImageBestMatches;//最佳匹配点集
        Mat ImageFinalResult;//FinalOne的邻居点集
        GrayImage.copyTo(ImageFinalResult);
        GrayImage.copyTo(ImageBestMatches);

        for(int ii=0;ii<DiskNum;ii++)
        {

            for(i=0;i<Points[ii].size();i++)
            {

                //第二多的邻居数量
                int PointNum=0;
                Point Pointss[DiskNum];
                for(j=0;j<DiskNum;j++)
                {
                    Pointss[j]=Point(0,0);
                    if(j==ii)   Pointss[j]=Points[ii][i];
                }

                for(int jj=0;jj<DiskNum;jj++)
                {
                    float distemp=DisksCenterPointsDistance[ii*DiskNum+jj];
                    //获取邻居点坐标
                    for(j=0;j<Points[jj].size();j++)
                    {
                        if(jj!=ii)
                        {
                            float distance=get_distance(Points[ii][i],Points[jj][j]);
                            distance=fabs(distance-distemp);//距离
                            if(distance<Thresh_distance)
                            {
                                Pointss[jj]=Points[jj][j];
                                PointNum++;//第一多的邻居数量
                                break;
                            }
                        }

                    }

                }
                //阈值：第一多的邻居数量不少于Thresh_MBNSnum个
                if(PointNum>Thresh_MBNSnum)
                {
                    //获取最佳点坐标
                    if(PointNum>BestPointNum[ii])
                    {
                        BestPoints[ii]=Points[ii][i];
                        BestPointNum[ii]=PointNum;
                    }

                }

            }
            if(BestPointNum[ii]<Thresh_MBNSnum)  BestPoints[ii]=Point(0,0);//阈值：第二多邻居数量不少于Thresh_MBNSnum
            circle(ImageBestMatches,BestPoints[ii],2,CV_RGB(255,255,255),2,8,0);


        }

        //****从精确匹配得到的WidthNum*HeightNum个点中确定最终匹配点****
        Point FirstBestPoint=BestPoints[0];
        float FirstBestPointNum=0;
        int FirstBestPointSort=0;
        /*
        for(i=0;i<DiskNum;i++)
        {
            int PointNum=0;

            PointNum=CountNeighbors(i, BestPoints, DiskNum,DisksCenterPoints, Thresh_distance);//Thresh_distance=R
            //获取最佳点
            if(PointNum>FirstBestPointNum)
            {
                FirstBestPoint=BestPoints[i];
                FirstBestPointNum=PointNum;
                FirstBestPointSort=i;
            }


        }
        */
        for(int ii=0;ii<DiskNum;ii++)
        {

            if(BestPoints[ii]==Point(0,0))
            {
                continue;
            }
            else
            {
                int PointNum=0;//邻居数量总和
                Point Pointss[DiskNum];
                for(j=0;j<DiskNum;j++)
                {
                    Pointss[j]=Point(0,0);
                    if(j==ii)   Pointss[j]=BestPoints[ii];
                }

                for(int jj=0;jj<DiskNum;jj++)
                {

                    //获取邻居点坐标
                    if(jj!=ii)
                    {
                        float distance=get_distance(BestPoints[ii],BestPoints[jj]);
                        float distemp=DisksCenterPointsDistance[ii*DiskNum+jj];
                        distance=fabs(distance-distemp);//距离
                        if(distance<Thresh_distance)
                        {
                            Pointss[jj]=BestPoints[jj];
                            PointNum++;//第一多的邻居数量
                        }
                    }
                }

                //******
                float PointNum1=PointNum;//第一多的邻居数量
                //计算第一多的邻居中的所有点的的邻居数量和
                int PointNumtemp=0;
                for(int tt=0;tt<DiskNum;tt++)
                {
                    if(tt!=ii)
                    {
                        PointNumtemp=CountNeighborsFast(tt, Pointss, DiskNum,DisksCenterPointsDistance, Thresh_distance);
                        PointNum=PointNum+PointNumtemp;
                    }
                }

                //获取最佳点坐标
                if((float)PointNum/PointNum1>FirstBestPointNum)
                {
                    FirstBestPoint=BestPoints[ii];
                    FirstBestPointNum=(float)PointNum/PointNum1;
                    FirstBestPointSort=ii;
                }
            }
        }

        //剔除错误点，与最佳匹配点是邻居的点认为是正确点
        for(i=0;i<DiskNum;i++)
        {
            if(BestPoints[i].x==0 && BestPoints[i].y==0)
            {
                BestPoints[i]=Point(0,0);
            }
            else
            {
                float distance=get_distance(FirstBestPoint,BestPoints[i]);
                float distemp=DisksCenterPointsDistance[FirstBestPointSort*DiskNum+i];
                distance=fabs(distance-distemp);//距离
                if(distance>Thresh_distance/2)
                {
                    BestPoints[i]=Point(0,0);
                }
            }
            circle(ImageFinalResult,BestPoints[i],2,CV_RGB(255,255,255),2,8,0);
        }

        //确定矩形框坐标
        Point RectUplPoint=Point(0,0);//矩形左上角坐标
        int rectupnum=0;
        for(i=0;i<DiskNum;i++)
        {
            if(BestPoints[i].x==0 && BestPoints[i].y==0)
            {
                BestPoints[i]=Point(0,0);
            }
            else
            {
                //计算每个正确点的邻居数量，此数量作为矩形框坐标计算的权重
                int PointNum=0;
                PointNum=CountNeighborsFast(i, BestPoints, DiskNum,DisksCenterPointsDistance, Thresh_distance);//Thresh_distance=R

                RectUplPoint=RectUplPoint+PointNum*(BestPoints[i]-DisksCenterPoints[i]);
                rectupnum=rectupnum+PointNum;

            }

        }
        if(rectupnum==0) rectupnum=1;

        timeMBNS=((double)getTickCount() - timeMBNS)/getTickFrequency();//运行时间
        qDebug("MBNS耗时：%f秒",timeMBNS);
        Mat ImageFinalResult2;//FinalOne
        GrayImage.copyTo(ImageFinalResult2);
        circle(ImageFinalResult2,FirstBestPoint,2,CV_RGB(255,255,255),2,8,0);


        RectUplPoint.x=(int)RectUplPoint.x/rectupnum;
        RectUplPoint.y=(int)RectUplPoint.y/rectupnum;
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)0);

        //***********************************************************************
        //***********************************************************************
        timeSum=((double)getTickCount() - timeSum)/getTickFrequency();//运行时间
        qDebug("模板匹配结束，总耗时：%f秒",timeSum);
        qDebug("***********************");


    /*
        //利用2个最佳匹配点画矩形框
        //计算两向量夹角
        float AvectorAngle=AnglePolar(DisksCenterPoints[SecondBestPointSort]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
        float BvectorAngle=AnglePolar(SecondBestPoint-FirstBestPoint);//实际向量
        //两向量夹角0-360°
        float Sita=BvectorAngle-AvectorAngle;
        if(Sita<0) Sita=Sita+360;


        Point RectUplPoint;//矩形左上角坐标
        RectUplPoint.x=(int)(-DisksCenterPoints[FirstBestPointSort].x*cos(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*sin(Sita/180*3.14) + FirstBestPoint.x);
        RectUplPoint.y=(int)(-(-DisksCenterPoints[FirstBestPointSort].x)*sin(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*cos(Sita/180*3.14)  + FirstBestPoint.y);
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)Sita);
    */
/*
        //显示结果
        qDebug("最终匹配的矩形左上角坐标=(%d,%d);旋转角=%f°",RectUplPoint.x,RectUplPoint.y,0);
        imshow("ImageCandidates",GrayImageDisp);//局部特征粗匹配得到候选匹配点集
        imshow("ImageBestMatches",ImageBestMatches);
        imshow("ImageFinalResult",ImageFinalResult);//Most Neighbors Similarity: 获取9个最佳匹配点
        imshow("ImageFinalOne",ImageFinalResult2);//Most Neighbors Similarity: 从9个匹配点中获取最佳的2个匹配点

*/

    /*
        Mat srcImageDisp;
        //resize函数前加cv：：，因为Qt也有resize函数，故加cv：：加以区别，不然会出现错误
        int resizeW=(int)ImageFinalResult2.cols/(beta+1);
        int resizeH=(int)ImageFinalResult2.rows/(beta+1);
        cv::resize(ImageFinalResult2,srcImageDisp,Size(resizeW,resizeH),0,0,3);
        Imgview->imshow(srcImageDisp);//显示到绑定的控件上

    */

        //返回结果参数
        MatchUpLPoint[0]=RectUplPoint;
        MatchTime[0]=timeSum;

        //MBNS耗时
        MNS_second_SumTime=MNS_second_SumTime+timeMBNS;
    }
    else
    {
        //返回结果参数
        MatchUpLPoint[0]=Point(0,0);
        MatchTime[0]=0;
    }

}

//特征：环投影均值、灰度均值、HSV各分量均值
void MBNShsv_SSDtMatching2_Fast_Angle(Mat Image,Mat TemplateImage, Point* MatchUpLPoint,Point* MatchCenterPoint, float* MatchSita, float* MatchTime){

    Scalar temprg=cv::mean(Image);
    float mean_1=temprg.val[0];
    float mean_2=temprg.val[1];
    qDebug("mean_1=%f,mean_2=%f",mean_1,mean_2);
    int channels=3;
    if(fabs(mean_1-mean_2)<1)   channels=1;
    qDebug("channels=%d",channels);

    //分离颜色通道HSV
    Mat Image_h,Image_s,Image_v,Imagehsv;
    if(channels>=3)
    {
        vector<Mat> hsv_vec;
        cvtColor(Image, Imagehsv, CV_BGR2HSV);
        split(Imagehsv, hsv_vec);
        Image_h = hsv_vec[0];
        Image_s = hsv_vec[1];
        Image_v = hsv_vec[2];
        Image_h.convertTo(Image_h, CV_8UC1);
        Image_s.convertTo(Image_s, CV_8UC1);
        Image_v.convertTo(Image_v, CV_8UC1);
   }

    int i,j;
    //一、图像和模板都转为单通道的灰度图
    Mat GrayImage;
    cvtColor(Image,GrayImage,CV_RGB2GRAY,1);
    Mat GrayTemplate;
    cvtColor(TemplateImage,GrayTemplate,CV_RGB2GRAY,1);
    qDebug("待匹配图宽高=【%d x %d】；模板图宽高=【%d x %d】",GrayImage.cols,GrayImage.rows,GrayTemplate.cols,GrayTemplate.rows);

    //二、将模板图分成多个Ldisk*Ldisk的小方块，每个方块圆心相距
    R=(int)Ldisk/2;//局部方块半径
    int tt=2;//初始距离倍数
    int disL=(int)Ldisk/tt;//每个方块圆心相距的距离
    int WidthNum=(int)(GrayTemplate.cols-Ldisk)/disL+1;//一行的方块数
    int HeightNum=(int)(GrayTemplate.rows-Ldisk)/disL+1;//一列的方块数
    if(Ldisk==9 || WidthNum*HeightNum<70)
    {
        while(WidthNum*HeightNum<70)
        {
            tt++;
            disL=(int)(Ldisk/tt);//每个方块圆心相距的距离
            WidthNum=(int)(GrayTemplate.cols-Ldisk)/disL+1;//一行的方块数
            HeightNum=(int)(GrayTemplate.rows-Ldisk)/disL+1;//一列的方块数
        }
    }
    Thresh_distance=(float)(PSMode*disL);//邻居距离阈值
    qDebug("邻居距离阈值=%f",Thresh_distance);
    qDebug("模板方块边长=%d",Ldisk);
    qDebug("模板方块数量=%d",WidthNum*HeightNum);

    //四（1）、建立环投影的环查找表
    int x[R]={0};//环距离
    x[0]=1;
    int xnum=1;
    for(i=0;i<R;i++)
    {
        x[i+1]=(int)(R-sqrt((R-x[i])*(R-x[i])-(2*R-1))+0.5);
        xnum++;
        if(x[i+1]<=x[i])
        {
            x[i+1]=0;
            xnum--;
            break;
        }
    }

    uint AnnulusTable[Ldisk*Ldisk];//环查找表
    uint anNumber[xnum]={0};//统计同个环的像素数量
    uint Rtable[Ldisk*Ldisk];//半径查找表
    uint rNumber[R+1]={0};//统计同个半径值的数量
    uint aNumber[360]={0};//统计同个角度值的数量
    uint Atable[Ldisk*Ldisk];//角度查找表
    double YdivX;//斜率
    int A=0;//角度
    int r;//半径

    for(i=0;i<Ldisk;i++)
    {
        for(j=0;j<Ldisk;j++)
        {
            r=(int)(sqrt((i-R)*(i-R)+(j-R)*(j-R)) + 0.5);

            //建立极坐标（半径和角度）查找表
            //角度转换
             if(j>R-1 && i==R)
             {
                 A=0;
             }
             else if(j<R && i==R)
             {
                 A=180;
             }
             else if(j==R && i<R)
             {
                 A=90;
             }
             else if(j==R && i>R)
             {
                 A=270;
             }
             else if(j>R && i<R)
             {
                 YdivX=(double)(R-i)/(j-R);
                 A=(int)(atan(YdivX)*180/3.14159);
             }
             else if(j<R && i<R)
             {
                 YdivX=(double)(R-j)/(R-i);
                 A=(int)(atan(YdivX)*180/3.14159)+90;
             }
             else if(j<R && i>R)
             {
                 YdivX=(double)(i-R)/(R-j);
                 A=(int)(atan(YdivX)*180/3.14159)+180;
             }
             else if(j>R && i>R)
             {
                 YdivX=(double)(j-R)/(i-R);
                 A=(int)(atan(YdivX)*180/3.14159)+270;
             }


             if(r>R)
             {
                 Rtable[i*Ldisk+j] = R+1;
                 Atable[i*Ldisk+j] = 361;
             }
             else
             {
                 Rtable[i*Ldisk+j] = r;
                 Atable[i*Ldisk+j] = A;
                 rNumber[r]++;
                 aNumber[A]++;
             }

            // **************************
            // **************************
            //建立环查找表
            if(r>R)
            {
                AnnulusTable[i*Ldisk+j] = xnum;
            }
            else
            {
                for(int ii=0;ii<xnum;ii++)
                {
                    if(r>(R-x[ii]))
                    {
                        AnnulusTable[i*Ldisk+j] = ii;
                        anNumber[ii]++;
                        break;
                    }
                }

                if(r<=(R-x[xnum-1]))
                {
                    AnnulusTable[i*Ldisk+j] = xnum-1;
                    anNumber[xnum-1]++;
                }
            }

        }
    }

    //四（2）、计算模板图的环投影向量
    vector<Mat> Disks;//方块
    vector<Point> DisksCenterPoints;//每个方块的中心坐标点
    int DiskNum=0;//真正的方块数量
    int iarray[WidthNum*HeightNum]={-1};//正真方块的序号
    for(i=0;i<WidthNum*HeightNum;i++)
    {
        Mat GrayTemplatetemp(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
        float Ptemp[xnum]={0};
        Anvector(GrayTemplatetemp,AnnulusTable,xnum,anNumber,Ptemp);
        float PtempAver=0;
        for(j=0;j<xnum;j++)
        {
            PtempAver=PtempAver+Ptemp[j];
        }
        PtempAver=PtempAver/xnum;
        float P_SSD=0;
        for(j=0;j<xnum;j++)
        {
            P_SSD=P_SSD+(Ptemp[j]-PtempAver)*(Ptemp[j]-PtempAver);
        }

        if(P_SSD>4*xnum)
        {
            Disks.push_back(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
            DisksCenterPoints.push_back(Point(((int)i%WidthNum)*disL+R,((int)i/WidthNum)*disL+R));
            DiskNum++;
            iarray[i]=i;
        }

    }
    qDebug("真正的方块数量= %d",DiskNum);
    Thresh_MBNSnum=(int)(DiskNum*0.2);
    qDebug("Thresh_MBNSnum= %d",Thresh_MBNSnum);

    //DiskNum<=30的话直接放弃匹配
    if(DiskNum>30)
    {
        //环投影向量
        float P[DiskNum][xnum]={0};//模板图环投影向量
        float DisksCenterPointsDistance[DiskNum*DiskNum]={0};//各个方块圆心坐标距离的平方
        for(i=0;i<DiskNum;i++)
        {
            //环投影向量
            Anvector(Disks[i],AnnulusTable,xnum,anNumber,P[i]);
            //各个方块圆心坐标距离的平方
            for(j=0;j<DiskNum;j++)
            {
                DisksCenterPointsDistance[i*DiskNum+j]=get_distance(DisksCenterPoints[i],DisksCenterPoints[j]);
            }

        }
        //添加颜色特征
        float P_h[DiskNum]={0};//模板图h分量均值
        float P_s[DiskNum]={0};//模板图s分量均值
        float P_v[DiskNum]={0};//模板图v分量均值
        if(channels>=3)
        {
            //分离颜色通道HSV
            Mat TemplateImage_h,TemplateImage_s,TemplateImage_v,TemplateImagehsv;
            vector<Mat> hsv_vec;
            cvtColor(TemplateImage, TemplateImagehsv, CV_BGR2HSV);
            split(TemplateImagehsv, hsv_vec);
            TemplateImage_h = hsv_vec[0];
            TemplateImage_s = hsv_vec[1];
            TemplateImage_v = hsv_vec[2];
            TemplateImage_h.convertTo(TemplateImage_h, CV_8UC1);
            TemplateImage_s.convertTo(TemplateImage_s, CV_8UC1);
            TemplateImage_v.convertTo(TemplateImage_v, CV_8UC1);

            int iii=0;
            for(i=0;i<WidthNum*HeightNum;i++)
            {
                if(iarray[i]==i)
                {
                    Mat Disks_h=TemplateImage_h(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    Scalar tempVal = cv::mean( Disks_h );
                    P_h[iii]= tempVal.val[0];

                    Mat Disks_s=TemplateImage_s(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = cv::mean( Disks_s );
                    P_s[iii]= tempVal.val[0];

                    Mat Disks_v=TemplateImage_v(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = cv::mean( Disks_v );
                    P_v[iii]= tempVal.val[0];

                    iii++;
                }
            }

        }
        //添加灰度均值和方差特征
        float P_grayM[DiskNum]={0};//模板图灰度均值
        float P_grayD[DiskNum]={0};//模板图灰度方差
        int iii=0;
        for(i=0;i<WidthNum*HeightNum;i++)
        {
            if(iarray[i]==i)
            {
                Mat Disks_gray=GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                Scalar Mean,dev;
                cv::meanStdDev(Disks_gray,Mean,dev);
                P_grayM[iii]= Mean.val[0];
                P_grayD[iii]= dev.val[0];

                iii++;
            }
        }

        //*****五、开始匹配************************************
        //*******************************************
        double timeSum;//总耗时
        timeSum=static_cast<double>(getTickCount());
        //***********************************************************
        double timeAnnulus;
        timeAnnulus=static_cast<double>(getTickCount());
        //设置圆投影搜索范围
        int LeftUpx=0;
        int LeftUpy=0;
        int RrightDownx=(int)(GrayImage.cols-Ldisk-1);
        int RrightDowny=(int)(GrayImage.rows-Ldisk-1);

        //获取并标出候选点位置
        double RminValue=0;
        Point RminLoc;
        double RmaxValue=0;
        Point RmaxLoc;
        Mat GrayImageDisp;
        GrayImage.copyTo(GrayImageDisp);

        int CandidateNum=MatchingNum2;
        vector<Point> Points[DiskNum];//精确匹配点坐标数组
        vector<Mat> MatchingValue;//环投影相似度值
        for(int ii=0;ii<DiskNum;ii++)
        {
            Mat MatchingValueTemp(GrayImage.rows-Ldisk,GrayImage.cols-Ldisk,CV_32FC1,Scalar(999999999));//float型单通道，每个点初始化
            MatchingValue.push_back(MatchingValueTemp);
        }
        for(i=LeftUpy;i<RrightDowny+1;i++)
        {

            for(j=LeftUpx;j<RrightDownx;j++)
            {

                Mat ScanROI;//待匹配的子图
                ScanROI=GrayImage(Rect(j,i,Ldisk,Ldisk));
                float T[xnum]={0};//模板图环投影向量
                Anvector(ScanROI,AnnulusTable,xnum,anNumber,T);
                Scalar Mean,dev;
                cv::meanStdDev(ScanROI,Mean,dev);
                float T_grayM= Mean.val[0];
                float T_grayD= dev.val[0];

                //添加颜色特征
                float T_h=0;//h分量均值
                float T_s=0;//s分量均值
                float T_v=0;//v分量均值
                if(channels>=3)
                {
                    Mat ScanROI_hsv;//待匹配的子图
                    ScanROI_hsv=Image_h(Rect(j,i,Ldisk,Ldisk));
                    Scalar tempVal = cv::mean( ScanROI_hsv );
                    T_h= tempVal.val[0];

                    ScanROI_hsv=Image_s(Rect(j,i,Ldisk,Ldisk));
                    tempVal = cv::mean( ScanROI_hsv );
                    T_s= tempVal.val[0];

                    ScanROI_hsv=Image_v(Rect(j,i,Ldisk,Ldisk));
                    tempVal = cv::mean( ScanROI_hsv );
                    T_v= tempVal.val[0];

                }


                for(int ii=0;ii<DiskNum;ii++)
                {
                    float d_h=fabs(P_h[ii]-T_h);
                    float d_s=fabs(P_s[ii]-T_s);
                    float d_v=fabs(P_v[ii]-T_v);
                    float d_d=fabs(P_grayD[ii]-T_grayD);
                    float d_m=fabs(P_grayM[ii]-T_grayM);

                    //if((d_h<20) && (d_s<40) && (d_v<60) && (d_d<20) && (d_m<60))
                    //{
                        float* buffer;
                        buffer=MatchingValue[ii].ptr<float>(i);
                        //相似度计算
                        int tempbuffer=0;
                        for(int k=0;k<xnum;k++)
                        {
                            tempbuffer=tempbuffer+(P[ii][k]-T[k])*(P[ii][k]-T[k]);
                            //tempbuffer=tempbuffer+fabs(P[ii][k]-T[k]);

                        }
                        if(channels>=3)
                        {
                            //buffer[j]=tempbuffer+(P_h[ii]-T_h)*(P_h[ii]-T_h)+(P_s[ii]-T_s)*(P_s[ii]-T_s)+(P_v[ii]-T_v)*(P_v[ii]-T_v)+(P_grayM[ii]-T_grayM)*(P_grayM[ii]-T_grayM);//+(P_grayD[ii]-T_grayD)*(P_grayD[ii]-T_grayD);
                            buffer[j]=tempbuffer+d_h*d_h+d_s*d_s+d_v*d_v+d_m*d_m;
                        }
                        else
                        {
                            buffer[j]=tempbuffer;
                        }
                    //}
                }

            }
        }

        for(int ii=0;ii<DiskNum;ii++)
        {
            for(i=0;i<CandidateNum;i++)
            {
                //1、选取候选匹配点
                minMaxLoc(MatchingValue[ii],&RminValue,&RmaxValue,&RminLoc,&RmaxLoc);
                Point BestMatch=RminLoc;
                Point Centerpoint;           
                if(RminValue==RmaxValue)
                {
                    Centerpoint=Point(0,0);
                }
                else
                {
                    Centerpoint=Point(BestMatch.x+R,BestMatch.y+R);
                }

                //画圆
                circle(GrayImageDisp,Centerpoint,2,CV_RGB(255,255,255),2,8,0);
                Points[ii].push_back(Centerpoint);

                float* buffervalue=MatchingValue[ii].ptr<float>(BestMatch.y);
                buffervalue[BestMatch.x]=999999999;
            }
        }
        timeAnnulus=((double)getTickCount() - timeAnnulus)/getTickFrequency();//运行时间
        //qDebug("环投影匹配耗时：%f秒",timeAnnulus);


        double timeMBNS;//MBNS耗时
        timeMBNS=static_cast<double>(getTickCount());
        //***************************************************
        //Neighbors Constraint Similarity确定最终匹配点
        //***************************************************
        Point BestPoints[DiskNum];//最佳匹配点
        int BestPointNum[DiskNum]={0};//最佳匹配点邻居数量
        Mat ImageBestMatches;//最佳匹配点集
        Mat ImageFinalResult;//FinalOne的邻居点集
        GrayImage.copyTo(ImageFinalResult);
        GrayImage.copyTo(ImageBestMatches);

        for(int ii=0;ii<DiskNum;ii++)
        {

            for(i=0;i<Points[ii].size();i++)
            {

                //第二多的邻居数量
                int PointNum=0;
                Point Pointss[DiskNum];
                for(j=0;j<DiskNum;j++)
                {
                    Pointss[j]=Point(0,0);
                    if(j==ii)   Pointss[j]=Points[ii][i];
                }

                for(int jj=0;jj<DiskNum;jj++)
                {
                    float distemp=DisksCenterPointsDistance[ii*DiskNum+jj];
                    //获取邻居点坐标
                    for(j=0;j<Points[jj].size();j++)
                    {
                        if(jj!=ii)
                        {
                            float distance=get_distance(Points[ii][i],Points[jj][j]);
                            distance=fabs(distance-distemp);//距离
                            if(distance<Thresh_distance)
                            {
                                Pointss[jj]=Points[jj][j];
                                PointNum++;//第一多的邻居数量
                                break;
                            }
                        }

                    }

                }
                //阈值：第一多的邻居数量不少于Thresh_MBNSnum个
                if(PointNum>Thresh_MBNSnum)
                {
                    PointNum=0;
                    //计算第二多的邻居数量
                    int PointNumtemp=0;
                    for(int tt=0;tt<DiskNum;tt++)
                    {
                        if(tt!=ii)
                        {
                            PointNumtemp=CountNeighborsFast(tt, Pointss, DiskNum,DisksCenterPointsDistance, Thresh_distance);
                            if(PointNum<PointNumtemp)
                            {
                                PointNum=PointNumtemp;//第二多的邻居数量
                            }
                        }
                    }

                    //获取最佳点坐标
                    if(PointNum>BestPointNum[ii])
                    {
                        BestPoints[ii]=Points[ii][i];
                        BestPointNum[ii]=PointNum;
                    }

                }

            }
            if(BestPointNum[ii]<Thresh_MBNSnum)  BestPoints[ii]=Point(0,0);//阈值：第二多邻居数量不少于Thresh_MBNSnum
            circle(ImageBestMatches,BestPoints[ii],2,CV_RGB(255,255,255),2,8,0);


        }

        //****从精确匹配得到的WidthNum*HeightNum个点中确定最终匹配点****
        Point FirstBestPoint=BestPoints[0];
        float FirstBestPointNum=0;
        int FirstBestPointSort=0;
        /*
        for(i=0;i<DiskNum;i++)
        {
            int PointNum=0;

            PointNum=CountNeighbors(i, BestPoints, DiskNum,DisksCenterPoints, Thresh_distance);//Thresh_distance=R
            //获取最佳点
            if(PointNum>FirstBestPointNum)
            {
                FirstBestPoint=BestPoints[i];
                FirstBestPointNum=PointNum;
                FirstBestPointSort=i;
            }


        }
        */
        for(int ii=0;ii<DiskNum;ii++)
        {

            if(BestPoints[ii]==Point(0,0))
            {
                continue;
            }
            else
            {
                int PointNum=0;//邻居数量总和
                Point Pointss[DiskNum];
                for(j=0;j<DiskNum;j++)
                {
                    Pointss[j]=Point(0,0);
                    if(j==ii)   Pointss[j]=BestPoints[ii];
                }

                for(int jj=0;jj<DiskNum;jj++)
                {

                    //获取邻居点坐标
                    if(jj!=ii)
                    {
                        float distance=get_distance(BestPoints[ii],BestPoints[jj]);
                        float distemp=DisksCenterPointsDistance[ii*DiskNum+jj];
                        distance=fabs(distance-distemp);//距离
                        if(distance<Thresh_distance)
                        {
                            Pointss[jj]=BestPoints[jj];
                            PointNum++;//第一多的邻居数量
                        }
                    }
                }

                //******
                float PointNum1=PointNum;//第一多的邻居数量
                //计算第一多的邻居中的所有点的的邻居数量和
                int PointNumtemp=0;
                for(int tt=0;tt<DiskNum;tt++)
                {
                    if(tt!=ii)
                    {
                        PointNumtemp=CountNeighborsFast(tt, Pointss, DiskNum,DisksCenterPointsDistance, Thresh_distance);
                        PointNum=PointNum+PointNumtemp;
                    }
                }

                //获取最佳点坐标
                if((float)PointNum/PointNum1>FirstBestPointNum)
                {
                    FirstBestPoint=BestPoints[ii];
                    FirstBestPointNum=(float)PointNum/PointNum1;
                    FirstBestPointSort=ii;
                }
            }
        }

        //确定矩形框坐标和角度
        //得到第一、二、三和四多邻居数量的点，作为角度计算点
        //注意：此处第一多邻居数量的点是FirstBestPoint1，不等于最佳匹配点FirstBestPoint。
        Point FirstBestPoint1=BestPoints[0];
        int FirstBestPointNum1=0;
        int FirstBestPointSort1=0;

        Point SecondBestPoint=BestPoints[1];
        int SecondBestPointNum=0;
        int SecondBestPointSort=1;

        Point ThirdBestPoint=BestPoints[2];
        int ThirdBestPointNum=0;
        int ThirdBestPointSort=2;

        Point FourthBestPoint=BestPoints[3];
        int FourthBestPointNum=0;
        int FourthBestPointSort=3;

        for(i=0;i<DiskNum;i++)
        {
            if(BestPoints[i].x==0 && BestPoints[i].y==0)
            {
                BestPoints[i]=Point(0,0);
            }
            else
            {
                //计算每个正确点的邻居数量，此数量作为矩形框坐标计算的权重
                int PointNum=0;
                PointNum=CountNeighbors(i, BestPoints, DiskNum,DisksCenterPoints, Thresh_distance);//Thresh_distance=R

                //角度点
                if(PointNum>FourthBestPointNum)
                {
                    if(PointNum>ThirdBestPointNum)
                    {
                        if(PointNum>SecondBestPointNum)
                        {
                            //获取最佳点
                            if(PointNum>FirstBestPointNum1)
                            {
                                FirstBestPoint1=BestPoints[i];
                                FirstBestPointNum1=PointNum;
                                FirstBestPointSort1=i;
                            }
                            else
                            {
                                SecondBestPoint=BestPoints[i];
                                SecondBestPointNum=PointNum;
                                SecondBestPointSort=i;
                            }
                        }
                        else
                        {
                            ThirdBestPoint=BestPoints[i];
                            ThirdBestPointNum=PointNum;
                            ThirdBestPointSort=i;
                        }
                    }
                    else
                    {
                        FourthBestPoint=BestPoints[i];
                        FourthBestPointNum=PointNum;
                        FourthBestPointSort=i;
                    }
                }

            }

        }

        //计算两向量夹角
        //11
        float AvectorAngle11=AnglePolar(DisksCenterPoints[FirstBestPointSort1]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
        float BvectorAngle11=AnglePolar(FirstBestPoint1-FirstBestPoint);//实际向量
        //两向量夹角0-360°
        float Sita11=BvectorAngle11-AvectorAngle11;
        if(Sita11<0) Sita11=Sita11+360;
        //12
        float AvectorAngle12=AnglePolar(DisksCenterPoints[SecondBestPointSort]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
        float BvectorAngle12=AnglePolar(SecondBestPoint-FirstBestPoint);//实际向量
        //两向量夹角0-360°
        float Sita12=BvectorAngle12-AvectorAngle12;
        if(Sita12<0) Sita12=Sita12+360;
        //13
        float AvectorAngle13=AnglePolar(DisksCenterPoints[ThirdBestPointSort]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
        float BvectorAngle13=AnglePolar(ThirdBestPoint-FirstBestPoint);//实际向量
        //两向量夹角0-360°
        float Sita13=BvectorAngle13-AvectorAngle13;
        if(Sita13<0) Sita13=Sita13+360;
        //14
        float AvectorAngle14=AnglePolar(DisksCenterPoints[FourthBestPointSort]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
        float BvectorAngle14=AnglePolar(FourthBestPoint-FirstBestPoint);//实际向量
        //两向量夹角0-360°
        float Sita14=BvectorAngle14-AvectorAngle14;
        if(Sita14<0) Sita14=Sita14+360;


        //剔除错误点，与最佳匹配点是邻居的点认为是正确点
        int Sita11Num=0;
        int Sita12Num=0;
        int Sita13Num=0;
        int Sita14Num=0;
        float Sita11Sum=0;
        float Sita12Sum=0;
        float Sita13Sum=0;
        float Sita14Sum=0;
        for(i=0;i<DiskNum;i++)
        {
            if(BestPoints[i].x==0 && BestPoints[i].y==0)
            {
                BestPoints[i]=Point(0,0);
            }
            else
            {
                float AvectorAngle=AnglePolar(DisksCenterPoints[i]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
                float BvectorAngle=AnglePolar(BestPoints[i]-FirstBestPoint);//实际向量
                //两向量夹角0-360°
                float Sita=BvectorAngle-AvectorAngle;
                if(Sita<0) Sita=Sita+360;

                if(fabs(Sita-Sita11)<20)
                {
                    Sita11Sum=Sita11Sum+Sita11;
                    Sita11Num++;
                }
                if(fabs(Sita-Sita12)<20)
                {
                    Sita12Sum=Sita12Sum+Sita12;
                    Sita12Num++;
                }
                if(fabs(Sita-Sita13)<20)
                {
                    Sita13Sum=Sita13Sum+Sita13;
                    Sita13Num++;
                }
                if(fabs(Sita-Sita14)<20)
                {
                    Sita14Sum=Sita14Sum+Sita14;
                    Sita14Num++;
                }
            }
        }


        timeMBNS=((double)getTickCount() - timeMBNS)/getTickFrequency();//运行时间
        qDebug("MBNS耗时：%f秒",timeMBNS);
        Mat ImageFinalResult2;//FinalOne
        GrayImage.copyTo(ImageFinalResult2);
        circle(ImageFinalResult2,FirstBestPoint,2,CV_RGB(255,255,255),2,8,0);


        int SitaMostNum=Sita11Num;
        if(Sita11Num==0)    Sita11Num=1;
        float Sita=Sita11Sum/Sita11Num;
        if(SitaMostNum<Sita12Num)
        {
            SitaMostNum=Sita12Num;
            if(Sita12Num==0)    Sita12Num=1;
            Sita=Sita12Sum/Sita12Num;
        }
        if(SitaMostNum<Sita13Num)
        {
            SitaMostNum=Sita13Num;
            if(Sita13Num==0)    Sita13Num=1;
            Sita=Sita13Sum/Sita13Num;
        }
        if(SitaMostNum<Sita14Num)
        {
            SitaMostNum=Sita14Num;
            if(Sita14Num==0)    Sita14Num=1;
            Sita=Sita14Sum/Sita14Num;
        }

        Point RectUplPoint=Point(0,0);//矩形左上角坐标
        RectUplPoint.x=(int)(-DisksCenterPoints[FirstBestPointSort].x*cos(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*sin(Sita/180*3.14) + FirstBestPoint.x);
        RectUplPoint.y=(int)(-(-DisksCenterPoints[FirstBestPointSort].x)*sin(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*cos(Sita/180*3.14)  + FirstBestPoint.y);
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)Sita);

        //***********************************************************************
        //***********************************************************************
        timeSum=((double)getTickCount() - timeSum)/getTickFrequency();//运行时间
        qDebug("模板匹配结束，总耗时：%f秒",timeSum);
        qDebug("***********************");


    /*
        //利用2个最佳匹配点画矩形框
        //计算两向量夹角
        float AvectorAngle=AnglePolar(DisksCenterPoints[SecondBestPointSort]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
        float BvectorAngle=AnglePolar(SecondBestPoint-FirstBestPoint);//实际向量
        //两向量夹角0-360°
        float Sita=BvectorAngle-AvectorAngle;
        if(Sita<0) Sita=Sita+360;


        Point RectUplPoint;//矩形左上角坐标
        RectUplPoint.x=(int)(-DisksCenterPoints[FirstBestPointSort].x*cos(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*sin(Sita/180*3.14) + FirstBestPoint.x);
        RectUplPoint.y=(int)(-(-DisksCenterPoints[FirstBestPointSort].x)*sin(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*cos(Sita/180*3.14)  + FirstBestPoint.y);
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)Sita);
    */

        //显示结果
        qDebug("最终匹配的矩形左上角坐标=(%d,%d);旋转角=%f°",RectUplPoint.x,RectUplPoint.y,0);
        imshow("ImageCandidates",GrayImageDisp);//局部特征粗匹配得到候选匹配点集
        imshow("ImageBestMatches",ImageBestMatches);
        imshow("ImageFinalResult",ImageFinalResult);//Most Neighbors Similarity: 获取9个最佳匹配点
        imshow("ImageFinalOne",ImageFinalResult2);//Most Neighbors Similarity: 从9个匹配点中获取最佳的2个匹配点



    /*
        Mat srcImageDisp;
        //resize函数前加cv：：，因为Qt也有resize函数，故加cv：：加以区别，不然会出现错误
        int resizeW=(int)ImageFinalResult2.cols/(beta+1);
        int resizeH=(int)ImageFinalResult2.rows/(beta+1);
        cv::resize(ImageFinalResult2,srcImageDisp,Size(resizeW,resizeH),0,0,3);
        Imgview->imshow(srcImageDisp);//显示到绑定的控件上

    */

        Point RectCenterPoint=Point(0,0);//矩形中心坐标
        RectCenterPoint.x=(int)((-DisksCenterPoints[FirstBestPointSort].x+(int)(TemplateImage.cols/2))*cos(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y+(int)(TemplateImage.rows/2))*sin(Sita/180*3.14) + FirstBestPoint.x);
        RectCenterPoint.y=(int)(-(-DisksCenterPoints[FirstBestPointSort].x+(int)(TemplateImage.cols/2))*sin(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y+(int)(TemplateImage.rows/2))*cos(Sita/180*3.14)  + FirstBestPoint.y);

        //返回结果参数
        MatchUpLPoint[0]=RectUplPoint;
        MatchCenterPoint[0]=RectCenterPoint;
        MatchSita[0]=Sita;
        MatchTime[0]=timeSum;

        //MBNS耗时
        MNS_second_SumTime=MNS_second_SumTime+timeMBNS;
    }
    else
    {
        //返回结果参数
        MatchUpLPoint[0]=Point(0,0);
        MatchCenterPoint[0]=Point(0,0);
        MatchSita[0]=0;
        MatchTime[0]=0;
    }

}

//特征：环投影均值、灰度均值、HSV各分量均值
void MNShsv_SSDtMatching2_Fast_Angle(Mat Image,Mat TemplateImage, Point* MatchUpLPoint, Point* MatchCenterPoint, float* MatchSita,float* MatchTime){

    Scalar temprg=cv::mean(Image);
    float mean_1=temprg.val[0];
    float mean_2=temprg.val[1];
    qDebug("mean_1=%f,mean_2=%f",mean_1,mean_2);
    int channels=3;
    if(fabs(mean_1-mean_2)<1)   channels=1;
    qDebug("channels=%d",channels);

    //分离颜色通道HSV
    Mat Image_h,Image_s,Image_v,Imagehsv;
    if(channels>=3)
    {
        vector<Mat> hsv_vec;
        cvtColor(Image, Imagehsv, CV_BGR2HSV);
        split(Imagehsv, hsv_vec);
        Image_h = hsv_vec[0];
        Image_s = hsv_vec[1];
        Image_v = hsv_vec[2];
        Image_h.convertTo(Image_h, CV_8UC1);
        Image_s.convertTo(Image_s, CV_8UC1);
        Image_v.convertTo(Image_v, CV_8UC1);
   }

    int i,j;
    //一、图像和模板都转为单通道的灰度图
    Mat GrayImage;
    cvtColor(Image,GrayImage,CV_RGB2GRAY,1);
    Mat GrayTemplate;
    cvtColor(TemplateImage,GrayTemplate,CV_RGB2GRAY,1);
    qDebug("待匹配图宽高=【%d x %d】；模板图宽高=【%d x %d】",GrayImage.cols,GrayImage.rows,GrayTemplate.cols,GrayTemplate.rows);

    //二、将模板图分成多个Ldisk*Ldisk的小方块，每个方块圆心相距
    R=(int)Ldisk/2;//局部方块半径
    int tt=2;//初始距离倍数
    int disL=(int)Ldisk/tt;//每个方块圆心相距的距离
    int WidthNum=(int)(GrayTemplate.cols-Ldisk)/disL+1;//一行的方块数
    int HeightNum=(int)(GrayTemplate.rows-Ldisk)/disL+1;//一列的方块数
    if(Ldisk==9 || WidthNum*HeightNum<70)
    {
        while(WidthNum*HeightNum<70)
        {
            tt++;
            disL=(int)(Ldisk/tt);//每个方块圆心相距的距离
            WidthNum=(int)(GrayTemplate.cols-Ldisk)/disL+1;//一行的方块数
            HeightNum=(int)(GrayTemplate.rows-Ldisk)/disL+1;//一列的方块数
        }
    }
    Thresh_distance=(float)(PSMode*disL);//邻居距离阈值
    qDebug("邻居距离阈值=%f",Thresh_distance);
    qDebug("模板方块边长=%d",Ldisk);
    qDebug("模板方块数量=%d",WidthNum*HeightNum);

    //四（1）、建立环投影的环查找表
    int x[R]={0};//环距离
    x[0]=1;
    int xnum=1;
    for(i=0;i<R;i++)
    {
        x[i+1]=(int)(R-sqrt((R-x[i])*(R-x[i])-(2*R-1))+0.5);
        xnum++;
        if(x[i+1]<=x[i])
        {
            x[i+1]=0;
            xnum--;
            break;
        }
    }

    uint AnnulusTable[Ldisk*Ldisk];//环查找表
    uint anNumber[xnum]={0};//统计同个环的像素数量
    uint Rtable[Ldisk*Ldisk];//半径查找表
    uint rNumber[R+1]={0};//统计同个半径值的数量
    uint aNumber[360]={0};//统计同个角度值的数量
    uint Atable[Ldisk*Ldisk];//角度查找表
    double YdivX;//斜率
    int A=0;//角度
    int r;//半径

    for(i=0;i<Ldisk;i++)
    {
        for(j=0;j<Ldisk;j++)
        {
            r=(int)(sqrt((i-R)*(i-R)+(j-R)*(j-R)) + 0.5);

            //建立极坐标（半径和角度）查找表
            //角度转换
             if(j>R-1 && i==R)
             {
                 A=0;
             }
             else if(j<R && i==R)
             {
                 A=180;
             }
             else if(j==R && i<R)
             {
                 A=90;
             }
             else if(j==R && i>R)
             {
                 A=270;
             }
             else if(j>R && i<R)
             {
                 YdivX=(double)(R-i)/(j-R);
                 A=(int)(atan(YdivX)*180/3.14159);
             }
             else if(j<R && i<R)
             {
                 YdivX=(double)(R-j)/(R-i);
                 A=(int)(atan(YdivX)*180/3.14159)+90;
             }
             else if(j<R && i>R)
             {
                 YdivX=(double)(i-R)/(R-j);
                 A=(int)(atan(YdivX)*180/3.14159)+180;
             }
             else if(j>R && i>R)
             {
                 YdivX=(double)(j-R)/(i-R);
                 A=(int)(atan(YdivX)*180/3.14159)+270;
             }


             if(r>R)
             {
                 Rtable[i*Ldisk+j] = R+1;
                 Atable[i*Ldisk+j] = 361;
             }
             else
             {
                 Rtable[i*Ldisk+j] = r;
                 Atable[i*Ldisk+j] = A;
                 rNumber[r]++;
                 aNumber[A]++;
             }

            // **************************
            // **************************
            //建立环查找表
            if(r>R)
            {
                AnnulusTable[i*Ldisk+j] = xnum;
            }
            else
            {
                for(int ii=0;ii<xnum;ii++)
                {
                    if(r>(R-x[ii]))
                    {
                        AnnulusTable[i*Ldisk+j] = ii;
                        anNumber[ii]++;
                        break;
                    }
                }

                if(r<=(R-x[xnum-1]))
                {
                    AnnulusTable[i*Ldisk+j] = xnum-1;
                    anNumber[xnum-1]++;
                }
            }

        }
    }

    //四（2）、计算模板图的环投影向量
    vector<Mat> Disks;//方块
    vector<Point> DisksCenterPoints;//每个方块的中心坐标点
    int DiskNum=0;//真正的方块数量
    int iarray[WidthNum*HeightNum]={-1};//正真方块的序号
    for(i=0;i<WidthNum*HeightNum;i++)
    {
        Mat GrayTemplatetemp(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
        float Ptemp[xnum]={0};
        Anvector(GrayTemplatetemp,AnnulusTable,xnum,anNumber,Ptemp);
        float PtempAver=0;
        for(j=0;j<xnum;j++)
        {
            PtempAver=PtempAver+Ptemp[j];
        }
        PtempAver=PtempAver/xnum;
        float P_SSD=0;
        for(j=0;j<xnum;j++)
        {
            P_SSD=P_SSD+(Ptemp[j]-PtempAver)*(Ptemp[j]-PtempAver);
        }

        if(P_SSD>4*xnum)
        {
            Disks.push_back(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
            DisksCenterPoints.push_back(Point(((int)i%WidthNum)*disL+R,((int)i/WidthNum)*disL+R));
            DiskNum++;
            iarray[i]=i;
        }

    }
    qDebug("真正的方块数量= %d",DiskNum);
    Thresh_MBNSnum=(int)(DiskNum*0.2);
    qDebug("Thresh_MBNSnum= %d",Thresh_MBNSnum);

    //DiskNum<=30的话直接放弃匹配
    if(DiskNum>30)
    {
        //环投影向量
        float P[DiskNum][xnum]={0};//模板图环投影向量
        float DisksCenterPointsDistance[DiskNum*DiskNum]={0};//各个方块圆心坐标距离的平方
        for(i=0;i<DiskNum;i++)
        {
            //环投影向量
            Anvector(Disks[i],AnnulusTable,xnum,anNumber,P[i]);
            //各个方块圆心坐标距离的平方
            for(j=0;j<DiskNum;j++)
            {
                DisksCenterPointsDistance[i*DiskNum+j]=get_distance(DisksCenterPoints[i],DisksCenterPoints[j]);
            }

        }
        //添加颜色特征
        float P_h[DiskNum]={0};//模板图h分量均值
        float P_s[DiskNum]={0};//模板图s分量均值
        float P_v[DiskNum]={0};//模板图v分量均值
        if(channels>=3)
        {
            //分离颜色通道HSV
            Mat TemplateImage_h,TemplateImage_s,TemplateImage_v,TemplateImagehsv;
            vector<Mat> hsv_vec;
            cvtColor(TemplateImage, TemplateImagehsv, CV_BGR2HSV);
            split(TemplateImagehsv, hsv_vec);
            TemplateImage_h = hsv_vec[0];
            TemplateImage_s = hsv_vec[1];
            TemplateImage_v = hsv_vec[2];
            TemplateImage_h.convertTo(TemplateImage_h, CV_8UC1);
            TemplateImage_s.convertTo(TemplateImage_s, CV_8UC1);
            TemplateImage_v.convertTo(TemplateImage_v, CV_8UC1);

            int iii=0;
            for(i=0;i<WidthNum*HeightNum;i++)
            {
                if(iarray[i]==i)
                {
                    Mat Disks_h=TemplateImage_h(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    Scalar tempVal = cv::mean( Disks_h );
                    P_h[iii]= tempVal.val[0];

                    Mat Disks_s=TemplateImage_s(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = cv::mean( Disks_s );
                    P_s[iii]= tempVal.val[0];

                    Mat Disks_v=TemplateImage_v(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = cv::mean( Disks_v );
                    P_v[iii]= tempVal.val[0];

                    iii++;
                }
            }

        }
        //添加灰度均值和方差特征
        float P_grayM[DiskNum]={0};//模板图灰度均值
        float P_grayD[DiskNum]={0};//模板图灰度方差
        int iii=0;
        for(i=0;i<WidthNum*HeightNum;i++)
        {
            if(iarray[i]==i)
            {
                Mat Disks_gray=GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                Scalar Mean,dev;
                cv::meanStdDev(Disks_gray,Mean,dev);
                P_grayM[iii]= Mean.val[0];
                P_grayD[iii]= dev.val[0];

                iii++;
            }
        }

        //*****五、开始匹配************************************
        //*******************************************
        double timeSum;//总耗时
        timeSum=static_cast<double>(getTickCount());
        //***********************************************************
        double timeAnnulus;
        timeAnnulus=static_cast<double>(getTickCount());
        //设置圆投影搜索范围
        int LeftUpx=0;
        int LeftUpy=0;
        int RrightDownx=(int)(GrayImage.cols-Ldisk-1);
        int RrightDowny=(int)(GrayImage.rows-Ldisk-1);

        //获取并标出候选点位置
        double RminValue=0;
        Point RminLoc;
        double RmaxValue=0;
        Point RmaxLoc;
        Mat GrayImageDisp;
        GrayImage.copyTo(GrayImageDisp);

        int CandidateNum=MatchingNum2;
        vector<Point> Points[DiskNum];//精确匹配点坐标数组
        vector<Mat> MatchingValue;//环投影相似度值
        for(int ii=0;ii<DiskNum;ii++)
        {
            Mat MatchingValueTemp(GrayImage.rows-Ldisk,GrayImage.cols-Ldisk,CV_32FC1,Scalar(999999999));//float型单通道，每个点初始化
            MatchingValue.push_back(MatchingValueTemp);
        }
        for(i=LeftUpy;i<RrightDowny+1;i++)
        {

            for(j=LeftUpx;j<RrightDownx;j++)
            {

                Mat ScanROI;//待匹配的子图
                ScanROI=GrayImage(Rect(j,i,Ldisk,Ldisk));
                float T[xnum]={0};//模板图环投影向量
                Anvector(ScanROI,AnnulusTable,xnum,anNumber,T);
                Scalar Mean,dev;
                cv::meanStdDev(ScanROI,Mean,dev);
                float T_grayM= Mean.val[0];
                float T_grayD= dev.val[0];

                //添加颜色特征
                float T_h=0;//h分量均值
                float T_s=0;//s分量均值
                float T_v=0;//v分量均值
                if(channels>=3)
                {
                    Mat ScanROI_hsv;//待匹配的子图
                    ScanROI_hsv=Image_h(Rect(j,i,Ldisk,Ldisk));
                    Scalar tempVal = cv::mean( ScanROI_hsv );
                    T_h= tempVal.val[0];

                    ScanROI_hsv=Image_s(Rect(j,i,Ldisk,Ldisk));
                    tempVal = cv::mean( ScanROI_hsv );
                    T_s= tempVal.val[0];

                    ScanROI_hsv=Image_v(Rect(j,i,Ldisk,Ldisk));
                    tempVal = cv::mean( ScanROI_hsv );
                    T_v= tempVal.val[0];

                }


                for(int ii=0;ii<DiskNum;ii++)
                {
                    float d_h=fabs(P_h[ii]-T_h);
                    float d_s=fabs(P_s[ii]-T_s);
                    float d_v=fabs(P_v[ii]-T_v);
                    float d_d=fabs(P_grayD[ii]-T_grayD);
                    float d_m=fabs(P_grayM[ii]-T_grayM);

                    if((d_h<20) && (d_s<40) && (d_v<60) && (d_d<20) && (d_m<60))
                    {
                        float* buffer;
                        buffer=MatchingValue[ii].ptr<float>(i);
                        //相似度计算
                        int tempbuffer=0;
                        for(int k=0;k<xnum;k++)
                        {
                            tempbuffer=tempbuffer+(P[ii][k]-T[k])*(P[ii][k]-T[k]);
                            //tempbuffer=tempbuffer+fabs(P[ii][k]-T[k]);

                        }
                        if(channels>=3)
                        {
                            //buffer[j]=tempbuffer+(P_h[ii]-T_h)*(P_h[ii]-T_h)+(P_s[ii]-T_s)*(P_s[ii]-T_s)+(P_v[ii]-T_v)*(P_v[ii]-T_v)+(P_grayM[ii]-T_grayM)*(P_grayM[ii]-T_grayM);//+(P_grayD[ii]-T_grayD)*(P_grayD[ii]-T_grayD);
                            buffer[j]=tempbuffer+d_h*d_h+d_s*d_s+d_v*d_v+d_m*d_m;
                        }
                        else
                        {
                            buffer[j]=tempbuffer;
                        }
                    }
                }

            }
        }

        for(int ii=0;ii<DiskNum;ii++)
        {
            for(i=0;i<CandidateNum;i++)
            {
                //1、选取候选匹配点
                minMaxLoc(MatchingValue[ii],&RminValue,&RmaxValue,&RminLoc,&RmaxLoc);
                Point BestMatch=RminLoc;
                Point Centerpoint;
                if(RminValue==RmaxValue)
                {
                    Centerpoint=Point(0,0);
                }
                else
                {
                    Centerpoint=Point(BestMatch.x+R,BestMatch.y+R);
                }

                //画圆
                circle(GrayImageDisp,Centerpoint,2,CV_RGB(255,255,255),2,8,0);
                Points[ii].push_back(Centerpoint);

                float* buffervalue=MatchingValue[ii].ptr<float>(BestMatch.y);
                buffervalue[BestMatch.x]=999999999;
            }
        }
        timeAnnulus=((double)getTickCount() - timeAnnulus)/getTickFrequency();//运行时间
        //qDebug("环投影匹配耗时：%f秒",timeAnnulus);


        double timeMBNS;//MBNS耗时
        timeMBNS=static_cast<double>(getTickCount());
        //***************************************************
        //Neighbors Constraint Similarity确定最终匹配点
        //***************************************************
        Point BestPoints[DiskNum];//最佳匹配点
        int BestPointNum[DiskNum]={0};//最佳匹配点邻居数量
        Mat ImageBestMatches;//最佳匹配点集
        Mat ImageFinalResult;//FinalOne的邻居点集
        GrayImage.copyTo(ImageFinalResult);
        GrayImage.copyTo(ImageBestMatches);

        for(int ii=0;ii<DiskNum;ii++)
        {

            for(i=0;i<Points[ii].size();i++)
            {

                //第一多的邻居数量
                int PointNum=0;
                Point Pointss[DiskNum];
                for(j=0;j<DiskNum;j++)
                {
                    Pointss[j]=Point(0,0);
                    if(j==ii)   Pointss[j]=Points[ii][i];
                }

                for(int jj=0;jj<DiskNum;jj++)
                {
                    float distemp=DisksCenterPointsDistance[ii*DiskNum+jj];
                    //获取邻居点坐标
                    for(j=0;j<Points[jj].size();j++)
                    {
                        if(jj!=ii)
                        {
                            float distance=get_distance(Points[ii][i],Points[jj][j]);
                            distance=fabs(distance-distemp);//距离
                            if(distance<Thresh_distance)
                            {
                                Pointss[jj]=Points[jj][j];
                                PointNum++;//第一多的邻居数量
                                break;
                            }
                        }

                    }

                }
                //阈值：第一多的邻居数量不少于Thresh_MBNSnum个
                if(PointNum>Thresh_MBNSnum)
                {

                    //获取最佳点坐标
                    if(PointNum>BestPointNum[ii])
                    {
                        BestPoints[ii]=Points[ii][i];
                        BestPointNum[ii]=PointNum;
                    }

                }

            }
            if(BestPointNum[ii]<Thresh_MBNSnum)  BestPoints[ii]=Point(0,0);//阈值：第二多邻居数量不少于Thresh_MBNSnum
            circle(ImageBestMatches,BestPoints[ii],2,CV_RGB(255,255,255),2,8,0);


        }

        //****从精确匹配得到的WidthNum*HeightNum个点中确定最终匹配点****
        Point FirstBestPoint=BestPoints[0];
        float FirstBestPointNum=0;
        int FirstBestPointSort=0;
        /*
        for(i=0;i<DiskNum;i++)
        {
            int PointNum=0;

            PointNum=CountNeighbors(i, BestPoints, DiskNum,DisksCenterPoints, Thresh_distance);//Thresh_distance=R
            //获取最佳点
            if(PointNum>FirstBestPointNum)
            {
                FirstBestPoint=BestPoints[i];
                FirstBestPointNum=PointNum;
                FirstBestPointSort=i;
            }


        }
        */
        for(int ii=0;ii<DiskNum;ii++)
        {

            if(BestPoints[ii]==Point(0,0))
            {
                continue;
            }
            else
            {
                int PointNum=0;//邻居数量总和
                Point Pointss[DiskNum];
                for(j=0;j<DiskNum;j++)
                {
                    Pointss[j]=Point(0,0);
                    if(j==ii)   Pointss[j]=BestPoints[ii];
                }

                for(int jj=0;jj<DiskNum;jj++)
                {

                    //获取邻居点坐标
                    if(jj!=ii)
                    {
                        float distance=get_distance(BestPoints[ii],BestPoints[jj]);
                        float distemp=DisksCenterPointsDistance[ii*DiskNum+jj];
                        distance=fabs(distance-distemp);//距离
                        if(distance<Thresh_distance)
                        {
                            Pointss[jj]=BestPoints[jj];
                            PointNum++;//第一多的邻居数量
                        }
                    }
                }

                //******
                float PointNum1=PointNum;//第一多的邻居数量
                //计算第一多的邻居中的所有点的的邻居数量和
                int PointNumtemp=0;
                for(int tt=0;tt<DiskNum;tt++)
                {
                    if(tt!=ii)
                    {
                        PointNumtemp=CountNeighborsFast(tt, Pointss, DiskNum,DisksCenterPointsDistance, Thresh_distance);
                        PointNum=PointNum+PointNumtemp;
                    }
                }

                //获取最佳点坐标
                if((float)PointNum/PointNum1>FirstBestPointNum)
                {
                    FirstBestPoint=BestPoints[ii];
                    FirstBestPointNum=(float)PointNum/PointNum1;
                    FirstBestPointSort=ii;
                }
            }
        }

        //确定矩形框坐标和角度
        //得到第一、二、三和四多邻居数量的点，作为角度计算点
        //注意：此处第一多邻居数量的点是FirstBestPoint1，不等于最佳匹配点FirstBestPoint。
        Point FirstBestPoint1=BestPoints[0];
        int FirstBestPointNum1=0;
        int FirstBestPointSort1=0;

        Point SecondBestPoint=BestPoints[1];
        int SecondBestPointNum=0;
        int SecondBestPointSort=1;

        Point ThirdBestPoint=BestPoints[2];
        int ThirdBestPointNum=0;
        int ThirdBestPointSort=2;

        Point FourthBestPoint=BestPoints[3];
        int FourthBestPointNum=0;
        int FourthBestPointSort=3;

        for(i=0;i<DiskNum;i++)
        {
            if(BestPoints[i].x==0 && BestPoints[i].y==0)
            {
                BestPoints[i]=Point(0,0);
            }
            else
            {
                //计算每个正确点的邻居数量，此数量作为矩形框坐标计算的权重
                int PointNum=0;
                PointNum=CountNeighbors(i, BestPoints, DiskNum,DisksCenterPoints, Thresh_distance);//Thresh_distance=R

                //角度点
                if(PointNum>FourthBestPointNum)
                {
                    if(PointNum>ThirdBestPointNum)
                    {
                        if(PointNum>SecondBestPointNum)
                        {
                            //获取最佳点
                            if(PointNum>FirstBestPointNum1)
                            {
                                FirstBestPoint1=BestPoints[i];
                                FirstBestPointNum1=PointNum;
                                FirstBestPointSort1=i;
                            }
                            else
                            {
                                SecondBestPoint=BestPoints[i];
                                SecondBestPointNum=PointNum;
                                SecondBestPointSort=i;
                            }
                        }
                        else
                        {
                            ThirdBestPoint=BestPoints[i];
                            ThirdBestPointNum=PointNum;
                            ThirdBestPointSort=i;
                        }
                    }
                    else
                    {
                        FourthBestPoint=BestPoints[i];
                        FourthBestPointNum=PointNum;
                        FourthBestPointSort=i;
                    }
                }

            }

        }

        //计算两向量夹角
        //11
        float AvectorAngle11=AnglePolar(DisksCenterPoints[FirstBestPointSort1]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
        float BvectorAngle11=AnglePolar(FirstBestPoint1-FirstBestPoint);//实际向量
        //两向量夹角0-360°
        float Sita11=BvectorAngle11-AvectorAngle11;
        if(Sita11<0) Sita11=Sita11+360;
        //12
        float AvectorAngle12=AnglePolar(DisksCenterPoints[SecondBestPointSort]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
        float BvectorAngle12=AnglePolar(SecondBestPoint-FirstBestPoint);//实际向量
        //两向量夹角0-360°
        float Sita12=BvectorAngle12-AvectorAngle12;
        if(Sita12<0) Sita12=Sita12+360;
        //13
        float AvectorAngle13=AnglePolar(DisksCenterPoints[ThirdBestPointSort]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
        float BvectorAngle13=AnglePolar(ThirdBestPoint-FirstBestPoint);//实际向量
        //两向量夹角0-360°
        float Sita13=BvectorAngle13-AvectorAngle13;
        if(Sita13<0) Sita13=Sita13+360;
        //14
        float AvectorAngle14=AnglePolar(DisksCenterPoints[FourthBestPointSort]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
        float BvectorAngle14=AnglePolar(FourthBestPoint-FirstBestPoint);//实际向量
        //两向量夹角0-360°
        float Sita14=BvectorAngle14-AvectorAngle14;
        if(Sita14<0) Sita14=Sita14+360;


        //剔除错误点，与最佳匹配点是邻居的点认为是正确点
        int Sita11Num=0;
        int Sita12Num=0;
        int Sita13Num=0;
        int Sita14Num=0;
        float Sita11Sum=0;
        float Sita12Sum=0;
        float Sita13Sum=0;
        float Sita14Sum=0;
        for(i=0;i<DiskNum;i++)
        {
            if(BestPoints[i].x==0 && BestPoints[i].y==0)
            {
                BestPoints[i]=Point(0,0);
            }
            else
            {
                float AvectorAngle=AnglePolar(DisksCenterPoints[i]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
                float BvectorAngle=AnglePolar(BestPoints[i]-FirstBestPoint);//实际向量
                //两向量夹角0-360°
                float Sita=BvectorAngle-AvectorAngle;
                if(Sita<0) Sita=Sita+360;

                if(fabs(Sita-Sita11)<20)
                {
                    Sita11Sum=Sita11Sum+Sita11;
                    Sita11Num++;
                }
                if(fabs(Sita-Sita12)<20)
                {
                    Sita12Sum=Sita12Sum+Sita12;
                    Sita12Num++;
                }
                if(fabs(Sita-Sita13)<20)
                {
                    Sita13Sum=Sita13Sum+Sita13;
                    Sita13Num++;
                }
                if(fabs(Sita-Sita14)<20)
                {
                    Sita14Sum=Sita14Sum+Sita14;
                    Sita14Num++;
                }
            }
        }


        timeMBNS=((double)getTickCount() - timeMBNS)/getTickFrequency();//运行时间
        qDebug("MBNS耗时：%f秒",timeMBNS);
        Mat ImageFinalResult2;//FinalOne
        GrayImage.copyTo(ImageFinalResult2);
        circle(ImageFinalResult2,FirstBestPoint,2,CV_RGB(255,255,255),2,8,0);

        int SitaMostNum=Sita11Num;
        if(Sita11Num==0)    Sita11Num=1;
        float Sita=Sita11Sum/Sita11Num;
        if(SitaMostNum<Sita12Num)
        {
            SitaMostNum=Sita12Num;
            if(Sita12Num==0)    Sita12Num=1;
            Sita=Sita12Sum/Sita12Num;
        }
        if(SitaMostNum<Sita13Num)
        {
            SitaMostNum=Sita13Num;
            if(Sita13Num==0)    Sita13Num=1;
            Sita=Sita13Sum/Sita13Num;
        }
        if(SitaMostNum<Sita14Num)
        {
            SitaMostNum=Sita14Num;
            if(Sita14Num==0)    Sita14Num=1;
            Sita=Sita14Sum/Sita14Num;
        }

        Point RectUplPoint=Point(0,0);//矩形左上角坐标
        RectUplPoint.x=(int)(-DisksCenterPoints[FirstBestPointSort].x*cos(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*sin(Sita/180*3.14) + FirstBestPoint.x);
        RectUplPoint.y=(int)(-(-DisksCenterPoints[FirstBestPointSort].x)*sin(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*cos(Sita/180*3.14)  + FirstBestPoint.y);
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)Sita);

        //***********************************************************************
        //***********************************************************************
        timeSum=((double)getTickCount() - timeSum)/getTickFrequency();//运行时间
        qDebug("模板匹配结束，总耗时：%f秒",timeSum);
        qDebug("***********************");


    /*
        //利用2个最佳匹配点画矩形框
        //计算两向量夹角
        float AvectorAngle=AnglePolar(DisksCenterPoints[SecondBestPointSort]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
        float BvectorAngle=AnglePolar(SecondBestPoint-FirstBestPoint);//实际向量
        //两向量夹角0-360°
        float Sita=BvectorAngle-AvectorAngle;
        if(Sita<0) Sita=Sita+360;


        Point RectUplPoint;//矩形左上角坐标
        RectUplPoint.x=(int)(-DisksCenterPoints[FirstBestPointSort].x*cos(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*sin(Sita/180*3.14) + FirstBestPoint.x);
        RectUplPoint.y=(int)(-(-DisksCenterPoints[FirstBestPointSort].x)*sin(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*cos(Sita/180*3.14)  + FirstBestPoint.y);
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)Sita);
    */

        //显示结果
        qDebug("最终匹配的矩形左上角坐标=(%d,%d);旋转角=%f°",RectUplPoint.x,RectUplPoint.y,0);
        imshow("ImageCandidates",GrayImageDisp);//局部特征粗匹配得到候选匹配点集
        imshow("ImageBestMatches",ImageBestMatches);
        imshow("ImageFinalResult",ImageFinalResult);//Most Neighbors Similarity: 获取9个最佳匹配点
        imshow("ImageFinalOne",ImageFinalResult2);//Most Neighbors Similarity: 从9个匹配点中获取最佳的2个匹配点



    /*
        Mat srcImageDisp;
        //resize函数前加cv：：，因为Qt也有resize函数，故加cv：：加以区别，不然会出现错误
        int resizeW=(int)ImageFinalResult2.cols/(beta+1);
        int resizeH=(int)ImageFinalResult2.rows/(beta+1);
        cv::resize(ImageFinalResult2,srcImageDisp,Size(resizeW,resizeH),0,0,3);
        Imgview->imshow(srcImageDisp);//显示到绑定的控件上

    */

        Point RectCenterPoint=Point(0,0);;//矩形中心坐标
        RectCenterPoint.x=(int)((-DisksCenterPoints[FirstBestPointSort].x+(int)(TemplateImage.cols/2))*cos(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y+(int)(TemplateImage.rows/2))*sin(Sita/180*3.14) + FirstBestPoint.x);
        RectCenterPoint.y=(int)(-(-DisksCenterPoints[FirstBestPointSort].x+(int)(TemplateImage.cols/2))*sin(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y+(int)(TemplateImage.rows/2))*cos(Sita/180*3.14)  + FirstBestPoint.y);

        //返回结果参数
        MatchUpLPoint[0]=RectUplPoint;
        MatchCenterPoint[0]=RectCenterPoint;
        MatchSita[0]=Sita;
        MatchTime[0]=timeSum;

        //MBNS耗时
        MNS_second_SumTime=MNS_second_SumTime+timeMBNS;
    }
    else
    {
        //返回结果参数
        MatchUpLPoint[0]=Point(0,0);
        MatchCenterPoint[0]=Point(0,0);
        MatchSita[0]=0;
        MatchTime[0]=0;
    }

}

//特征：环投影均值、灰度均值、HSV各分量均值
void MBNShsv_SSDtMatching2_Angle(Mat Image,Mat TemplateImage, Point* MatchUpLPoint, float* MatchTime){

    //分离颜色通道
    Mat Image_h,Image_s,Image_v,Imagehsv;
    if(Image.channels()!=1)
    {
        //分离颜色通道HSV
        vector<Mat> hsv_vec;
        cvtColor(Image, Imagehsv, CV_BGR2HSV);
        split(Imagehsv, hsv_vec);
        Image_h = hsv_vec[0];
        Image_s = hsv_vec[1];
        Image_v = hsv_vec[2];
        Image_h.convertTo(Image_h, CV_8UC1);
        Image_s.convertTo(Image_s, CV_8UC1);
        Image_v.convertTo(Image_v, CV_8UC1);
    }

    int i,j;
    //一、图像和模板都转为单通道的灰度图
    Mat GrayImage;
    cvtColor(Image,GrayImage,CV_RGB2GRAY,1);
    Mat GrayTemplate;
    cvtColor(TemplateImage,GrayTemplate,CV_RGB2GRAY,1);
    qDebug("待匹配图宽高=【%d x %d】；模板图宽高=【%d x %d】",GrayImage.cols,GrayImage.rows,GrayTemplate.cols,GrayTemplate.rows);

    //二、将模板图分成多个Ldisk*Ldisk的小方块，每个方块圆心相距
    R=(int)Ldisk/2;//局部方块半径
    int tt=2;//初始距离倍数
    int disL=(int)Ldisk/tt;//每个方块圆心相距的距离
    int WidthNum=(int)GrayTemplate.cols/Ldisk*tt - tt;//一行的方块数
    int HeightNum=(int)GrayTemplate.rows/Ldisk*tt - tt;//一列的方块数
    if(Ldisk==9 || WidthNum*HeightNum<70)
    {
        while(WidthNum*HeightNum<70)
        {
            tt++;
            disL=(int)(Ldisk/tt);//每个方块圆心相距的距离
            WidthNum=(int)GrayTemplate.cols/Ldisk*tt - tt;//一行的方块数
            HeightNum=(int)GrayTemplate.rows/Ldisk*tt - tt;//一列的方块数
        }
    }
    Thresh_distance=(float)(PSMode*Ldisk/tt);//邻居距离阈值
    qDebug("邻居距离阈值=%f",Thresh_distance);
    qDebug("模板方块边长=%d",Ldisk);
    qDebug("模板方块数量=%d",WidthNum*HeightNum);

    //四（1）、建立环投影的环查找表
    int x[R]={0};//环距离
    x[0]=1;
    int xnum=1;
    for(i=0;i<R;i++)
    {
        x[i+1]=(int)(R-sqrt((R-x[i])*(R-x[i])-(2*R-1))+0.5);
        xnum++;
        if(x[i+1]<=x[i])
        {
            x[i+1]=0;
            xnum--;
            break;
        }
    }

    uint AnnulusTable[Ldisk*Ldisk];//环查找表
    uint anNumber[xnum]={0};//统计同个环的像素数量
    uint Rtable[Ldisk*Ldisk];//半径查找表
    uint rNumber[R+1]={0};//统计同个半径值的数量
    uint aNumber[360]={0};//统计同个角度值的数量
    uint Atable[Ldisk*Ldisk];//角度查找表
    double YdivX;//斜率
    int A=0;//角度
    int r;//半径

    for(i=0;i<Ldisk;i++)
    {
        for(j=0;j<Ldisk;j++)
        {
            r=(int)(sqrt((i-R)*(i-R)+(j-R)*(j-R)) + 0.5);

            //建立极坐标（半径和角度）查找表
            //角度转换
             if(j>R-1 && i==R)
             {
                 A=0;
             }
             else if(j<R && i==R)
             {
                 A=180;
             }
             else if(j==R && i<R)
             {
                 A=90;
             }
             else if(j==R && i>R)
             {
                 A=270;
             }
             else if(j>R && i<R)
             {
                 YdivX=(double)(R-i)/(j-R);
                 A=(int)(atan(YdivX)*180/3.14159);
             }
             else if(j<R && i<R)
             {
                 YdivX=(double)(R-j)/(R-i);
                 A=(int)(atan(YdivX)*180/3.14159)+90;
             }
             else if(j<R && i>R)
             {
                 YdivX=(double)(i-R)/(R-j);
                 A=(int)(atan(YdivX)*180/3.14159)+180;
             }
             else if(j>R && i>R)
             {
                 YdivX=(double)(j-R)/(i-R);
                 A=(int)(atan(YdivX)*180/3.14159)+270;
             }


             if(r>R)
             {
                 Rtable[i*Ldisk+j] = R+1;
                 Atable[i*Ldisk+j] = 361;
             }
             else
             {
                 Rtable[i*Ldisk+j] = r;
                 Atable[i*Ldisk+j] = A;
                 rNumber[r]++;
                 aNumber[A]++;
             }

            // **************************
            // **************************
            //建立环查找表
            if(r>R)
            {
                AnnulusTable[i*Ldisk+j] = xnum;
            }
            else
            {
                for(int ii=0;ii<xnum;ii++)
                {
                    if(r>(R-x[ii]))
                    {
                        AnnulusTable[i*Ldisk+j] = ii;
                        anNumber[ii]++;
                        break;
                    }
                }

                if(r<=(R-x[xnum-1]))
                {
                    AnnulusTable[i*Ldisk+j] = xnum-1;
                    anNumber[xnum-1]++;
                }
            }

        }
    }

    //四（2）、计算模板图的环投影向量
    vector<Mat> Disks;//方块
    vector<Point> DisksCenterPoints;//每个方块的中心坐标点
    int DiskNum=0;//真正的方块数量
    int iarray[WidthNum*HeightNum]={-1};//正真方块的序号
    for(i=0;i<WidthNum*HeightNum;i++)
    {
        Mat GrayTemplatetemp(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
        float Ptemp[xnum]={0};
        Anvector(GrayTemplatetemp,AnnulusTable,xnum,anNumber,Ptemp);
        float PtempAver=0;
        for(j=0;j<xnum;j++)
        {
            PtempAver=PtempAver+Ptemp[j];
        }
        PtempAver=PtempAver/xnum;
        float P_SSD=0;
        for(j=0;j<xnum;j++)
        {
            P_SSD=P_SSD+(Ptemp[j]-PtempAver)*(Ptemp[j]-PtempAver);
        }

        if(P_SSD>4*xnum)
        {
            Disks.push_back(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
            DisksCenterPoints.push_back(Point(((int)i%WidthNum)*disL+R,((int)i/WidthNum)*disL+R));
            DiskNum++;
            iarray[i]=i;
        }

    }
    qDebug("真正的方块数量= %d",DiskNum);

    //DiskNum<=30的话直接放弃匹配
    if(DiskNum>30)
    {
        //环投影向量
        float P[DiskNum][xnum]={0};//模板图环投影向量
        for(i=0;i<DiskNum;i++)
        {
            //环投影向量
            Anvector(Disks[i],AnnulusTable,xnum,anNumber,P[i]);

        }
        //添加颜色特征
        float P_h[DiskNum]={0};//模板图h分量均值
        float P_s[DiskNum]={0};//模板图s分量均值
        float P_v[DiskNum]={0};//模板图v分量均值
        if(TemplateImage.channels()!=1)
        {
            //分离颜色通道HSV
            Mat TemplateImage_h,TemplateImage_s,TemplateImage_v,TemplateImagehsv;
            vector<Mat> hsv_vec;
            cvtColor(TemplateImage, TemplateImagehsv, CV_BGR2HSV);
            split(TemplateImagehsv, hsv_vec);
            TemplateImage_h = hsv_vec[0];
            TemplateImage_s = hsv_vec[1];
            TemplateImage_v = hsv_vec[2];
            TemplateImage_h.convertTo(TemplateImage_h, CV_8UC1);
            TemplateImage_s.convertTo(TemplateImage_s, CV_8UC1);
            TemplateImage_v.convertTo(TemplateImage_v, CV_8UC1);

            int iii=0;
            for(i=0;i<WidthNum*HeightNum;i++)
            {
                if(iarray[i]==i)
                {
                    Mat Disks_h=TemplateImage_h(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    Scalar tempVal = cv::mean( Disks_h );
                    P_h[iii]= tempVal.val[0];

                    Mat Disks_s=TemplateImage_s(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = cv::mean( Disks_s );
                    P_s[iii]= tempVal.val[0];

                    Mat Disks_v=TemplateImage_v(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = cv::mean( Disks_v );
                    P_v[iii]= tempVal.val[0];

                    iii++;
                }
            }

        }
        //添加灰度均值和方差特征
        float P_grayM[DiskNum]={0};//模板图灰度均值
        float P_grayD[DiskNum]={0};//模板图灰度方差
        int iii=0;
        for(i=0;i<WidthNum*HeightNum;i++)
        {
            if(iarray[i]==i)
            {
                Mat Disks_gray=GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                Scalar Mean,dev;
                cv::meanStdDev(Disks_gray,Mean,dev);
                P_grayM[iii]= Mean.val[0];
                P_grayD[iii]= dev.val[0];

                iii++;
            }
        }

        //*****五、开始匹配************************************
        //*******************************************
        double timeSum;//总耗时
        timeSum=static_cast<double>(getTickCount());
        //***********************************************************
        double timeAnnulus;
        timeAnnulus=static_cast<double>(getTickCount());
        //设置圆投影搜索范围
        int LeftUpx=0;
        int LeftUpy=0;
        int RrightDownx=(int)(GrayImage.cols-Ldisk-1);
        int RrightDowny=(int)(GrayImage.rows-Ldisk-1);

        //获取并标出候选点位置
        double RminValue=0;
        Point RminLoc;
        double RmaxValue=0;
        Point RmaxLoc;
        Mat GrayImageDisp;
        GrayImage.copyTo(GrayImageDisp);

        int CandidateNum=MatchingNum2;
        vector<Point> Points[DiskNum];//精确匹配点坐标数组
        vector<Mat> MatchingValue;//环投影相似度值
        for(int ii=0;ii<DiskNum;ii++)
        {
            Mat MatchingValueTemp(GrayImage.rows-Ldisk,GrayImage.cols-Ldisk,CV_32FC1,Scalar(999999999));//float型单通道，每个点初始化
            MatchingValue.push_back(MatchingValueTemp);
        }
        for(i=LeftUpy;i<RrightDowny+1;i++)
        {

            for(j=LeftUpx;j<RrightDownx;j++)
            {

                Mat ScanROI;//待匹配的子图
                ScanROI=GrayImage(Rect(j,i,Ldisk,Ldisk));
                float T[xnum]={0};//模板图环投影向量
                Anvector(ScanROI,AnnulusTable,xnum,anNumber,T);
                Scalar Mean,dev;
                cv::meanStdDev(ScanROI,Mean,dev);
                float T_grayM= Mean.val[0];
                float T_grayD= dev.val[0];

                //添加颜色特征
                float T_h=0;//h分量均值
                float T_s=0;//s分量均值
                float T_v=0;//v分量均值
                if(Image.channels()!=1)
                {
                    Mat ScanROI_hsv;//待匹配的子图
                    ScanROI_hsv=Image_h(Rect(j,i,Ldisk,Ldisk));
                    Scalar tempVal = cv::mean( ScanROI_hsv );
                    T_h= tempVal.val[0];

                    ScanROI_hsv=Image_s(Rect(j,i,Ldisk,Ldisk));
                    tempVal = cv::mean( ScanROI_hsv );
                    T_s= tempVal.val[0];

                    ScanROI_hsv=Image_v(Rect(j,i,Ldisk,Ldisk));
                    tempVal = cv::mean( ScanROI_hsv );
                    T_v= tempVal.val[0];

                }

                for(int ii=0;ii<DiskNum;ii++)
                {
                    float* buffer;
                    buffer=MatchingValue[ii].ptr<float>(i);
                    //相似度计算
                    int tempbuffer=0;
                    for(int k=0;k<xnum;k++)
                    {
                        tempbuffer=tempbuffer+(P[ii][k]-T[k])*(P[ii][k]-T[k]);

                    }
                    buffer[j]=tempbuffer+(P_h[ii]-T_h)*(P_h[ii]-T_h)+(P_s[ii]-T_s)*(P_s[ii]-T_s)+(P_v[ii]-T_v)*(P_v[ii]-T_v)+(P_grayM[ii]-T_grayM)*(P_grayM[ii]-T_grayM)+(P_grayD[ii]-T_grayD)*(P_grayD[ii]-T_grayD);

                }

            }
        }

        for(int ii=0;ii<DiskNum;ii++)
        {
            for(i=0;i<CandidateNum;i++)
            {
                //1、选取候选匹配点
                minMaxLoc(MatchingValue[ii],&RminValue,&RmaxValue,&RminLoc,&RmaxLoc);
                Point BestMatch=RminLoc;
                Point Centerpoint=Point(BestMatch.x+R,BestMatch.y+R);

                //画圆
                circle(GrayImageDisp,Centerpoint,2,CV_RGB(255,255,255),2,8,0);
                Points[ii].push_back(Centerpoint);

                float* buffervalue=MatchingValue[ii].ptr<float>(BestMatch.y);
                buffervalue[BestMatch.x]=999999999;
            }
        }
        timeAnnulus=((double)getTickCount() - timeAnnulus)/getTickFrequency();//运行时间
        //qDebug("环投影匹配耗时：%f秒",timeAnnulus);


        double timeMBNS;//MBNS耗时
        timeMBNS=static_cast<double>(getTickCount());
        //***************************************************
        //Neighbors Constraint Similarity确定最终匹配点
        //***************************************************
        Point BestPoints[DiskNum];//最佳匹配点
        int BestPointNum[DiskNum]={0};//最佳匹配点邻居数量
        Mat ImageBestMatches;//最佳匹配点集
        Mat ImageFinalResult;//FinalOne的邻居点集
        GrayImage.copyTo(ImageFinalResult);
        GrayImage.copyTo(ImageBestMatches);

        for(int ii=0;ii<DiskNum;ii++)
        {

            for(i=0;i<Points[ii].size();i++)
            {

                float tempMinDis=999999999;

                //第二多的邻居数量
                int PointNum=0;
                Point Pointtemp(0,0);
                Point Pointss[DiskNum];
                for(j=0;j<DiskNum;j++)
                {
                    Pointss[j]=Point(0,0);
                    if(j==ii)   Pointss[j]=Points[ii][i];
                }

                for(int jj=0;jj<DiskNum;jj++)
                {
                    //获取邻居点坐标
                    for(j=0;j<Points[jj].size();j++)
                    {
                        if(jj!=ii)
                        {
                            float distance=get_distance(Points[ii][i],Points[jj][j]);
                            float distemp=get_distance(DisksCenterPoints[ii],DisksCenterPoints[jj]);
                            distance=fabs(distance-distemp);//距离
                            if(distance<tempMinDis)
                            {
                                tempMinDis=distance;
                                Pointtemp=Points[jj][j];
                            }
                        }

                    }
                    if(tempMinDis<Thresh_distance)
                    {
                        Pointss[jj]=Pointtemp;
                        PointNum++;//第一多的邻居数量
                    }
                    tempMinDis=999999999;

                }
                //阈值：第一多的邻居数量不少于Thresh_MBNSnum个
                if(PointNum>Thresh_MBNSnum)
                {
                    PointNum=0;
                    //计算第二多的邻居数量
                    int PointNumtemp=0;
                    for(int tt=0;tt<DiskNum;tt++)
                    {
                        if(tt!=ii)
                        {
                            PointNumtemp=CountNeighbors(tt, Pointss, DiskNum,DisksCenterPoints, Thresh_distance);
                            if(PointNum<PointNumtemp)
                            {
                                PointNum=PointNumtemp;//第二多的邻居数量
                            }
                        }
                    }

                    //获取最佳点坐标
                    if(PointNum>BestPointNum[ii])
                    {
                        BestPoints[ii]=Points[ii][i];
                        BestPointNum[ii]=PointNum;
                    }

                }

            }
            if(BestPointNum[ii]<Thresh_MBNSnum)  BestPoints[ii]=Point(0,0);//阈值：第二多邻居数量不少于Thresh_MBNSnum
            circle(ImageBestMatches,BestPoints[ii],2,CV_RGB(255,255,255),2,8,0);


        }

        //****从精确匹配得到的WidthNum*HeightNum个点中确定最终匹配点****
        Point FirstBestPoint=BestPoints[0];
        int FirstBestPointNum=0;
        int FirstBestPointSort=0;
/*
        for(i=0;i<DiskNum;i++)
        {
            int PointNum=0;

            PointNum=CountNeighbors(i, BestPoints, DiskNum,DisksCenterPoints, Thresh_distance);//Thresh_distance=R
            //获取最佳点
            if(PointNum>FirstBestPointNum)
            {
                FirstBestPoint=BestPoints[i];
                FirstBestPointNum=PointNum;
                FirstBestPointSort=i;
            }


        }
        */
        for(int ii=0;ii<DiskNum;ii++)
        {

            int PointNum=0;//邻居数量总和
            Point Pointss[DiskNum];
            for(j=0;j<DiskNum;j++)
            {
                Pointss[j]=Point(0,0);
                if(j==ii)   Pointss[j]=BestPoints[ii];
            }

            for(int jj=0;jj<DiskNum;jj++)
            {
                //获取邻居点坐标
                if(jj!=ii)
                {
                    float distance=get_distance(BestPoints[ii],BestPoints[jj]);
                    float distemp=get_distance(DisksCenterPoints[ii],DisksCenterPoints[jj]);
                    distance=fabs(distance-distemp);//距离
                    if(distance<Thresh_distance)
                    {
                        Pointss[jj]=BestPoints[jj];
                        PointNum++;//第一多的邻居数量
                    }
                }
            }

            //******
            float PointNum1=PointNum;//第一多的邻居数量
            //计算第一多的邻居中的所有点的的邻居数量和
            int PointNumtemp=0;
            for(int tt=0;tt<DiskNum;tt++)
            {
                if(tt!=ii)
                {
                    PointNumtemp=CountNeighbors(tt, Pointss, DiskNum,DisksCenterPoints, Thresh_distance);
                    PointNum=PointNum+PointNumtemp;//邻居数量总和

                }
            }

            //获取最佳点坐标
            if(PointNum/PointNum1>FirstBestPointNum)
            {
                FirstBestPoint=BestPoints[ii];
                FirstBestPointNum=PointNum/PointNum1;
                FirstBestPointSort=ii;
            }
        }

        //剔除错误点，与最佳匹配点是邻居的点认为是正确点
        for(i=0;i<DiskNum;i++)
        {
            if(BestPoints[i].x==0 && BestPoints[i].y==0)
            {
                BestPoints[i]=Point(0,0);
            }
            else
            {
                float distance=get_distance(FirstBestPoint,BestPoints[i]);
                float distemp=get_distance(DisksCenterPoints[FirstBestPointSort],DisksCenterPoints[i]);
                distance=fabs(distance-distemp);//距离
                if(distance>Thresh_distance)
                {
                    BestPoints[i]=Point(0,0);
                }
            }
            circle(ImageFinalResult,BestPoints[i],2,CV_RGB(255,255,255),2,8,0);
        }

        //确定矩形框坐标和角度
        //得到第一、二、三和四多邻居数量的点，作为角度计算点
        //注意：此处第一多邻居数量的点是FirstBestPoint1，不等于最佳匹配点FirstBestPoint。
        Point FirstBestPoint1=BestPoints[0];
        int FirstBestPointNum1=0;
        int FirstBestPointSort1=0;

        Point SecondBestPoint=BestPoints[1];
        int SecondBestPointNum=0;
        int SecondBestPointSort=1;

        Point ThirdBestPoint=BestPoints[2];
        int ThirdBestPointNum=0;
        int ThirdBestPointSort=2;

        Point FourthBestPoint=BestPoints[3];
        int FourthBestPointNum=0;
        int FourthBestPointSort=3;

        for(i=0;i<DiskNum;i++)
        {
            if(BestPoints[i].x==0 && BestPoints[i].y==0)
            {
                BestPoints[i]=Point(0,0);
            }
            else
            {
                //计算每个正确点的邻居数量，此数量作为矩形框坐标计算的权重
                int PointNum=0;
                PointNum=CountNeighbors(i, BestPoints, DiskNum,DisksCenterPoints, Thresh_distance);//Thresh_distance=R

                //角度点
                if(PointNum>FourthBestPointNum)
                {
                    if(PointNum>ThirdBestPointNum)
                    {
                        if(PointNum>SecondBestPointNum)
                        {
                            //获取最佳点
                            if(PointNum>FirstBestPointNum1)
                            {
                                FirstBestPoint1=BestPoints[i];
                                FirstBestPointNum1=PointNum;
                                FirstBestPointSort1=i;
                            }
                            else
                            {
                                SecondBestPoint=BestPoints[i];
                                SecondBestPointNum=PointNum;
                                SecondBestPointSort=i;
                            }
                        }
                        else
                        {
                            ThirdBestPoint=BestPoints[i];
                            ThirdBestPointNum=PointNum;
                            ThirdBestPointSort=i;
                        }
                    }
                    else
                    {
                        FourthBestPoint=BestPoints[i];
                        FourthBestPointNum=PointNum;
                        FourthBestPointSort=i;
                    }
                }

            }

        }

        //计算两向量夹角
        //11
        float AvectorAngle11=AnglePolar(DisksCenterPoints[FirstBestPointSort1]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
        float BvectorAngle11=AnglePolar(FirstBestPoint1-FirstBestPoint);//实际向量
        //两向量夹角0-360°
        float Sita11=BvectorAngle11-AvectorAngle11;
        if(Sita11<0) Sita11=Sita11+360;
        //12
        float AvectorAngle12=AnglePolar(DisksCenterPoints[SecondBestPointSort]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
        float BvectorAngle12=AnglePolar(SecondBestPoint-FirstBestPoint);//实际向量
        //两向量夹角0-360°
        float Sita12=BvectorAngle12-AvectorAngle12;
        if(Sita12<0) Sita12=Sita12+360;
        //13
        float AvectorAngle13=AnglePolar(DisksCenterPoints[ThirdBestPointSort]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
        float BvectorAngle13=AnglePolar(ThirdBestPoint-FirstBestPoint);//实际向量
        //两向量夹角0-360°
        float Sita13=BvectorAngle13-AvectorAngle13;
        if(Sita13<0) Sita13=Sita13+360;
        //14
        float AvectorAngle14=AnglePolar(DisksCenterPoints[FourthBestPointSort]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
        float BvectorAngle14=AnglePolar(FourthBestPoint-FirstBestPoint);//实际向量
        //两向量夹角0-360°
        float Sita14=BvectorAngle14-AvectorAngle14;
        if(Sita14<0) Sita14=Sita14+360;


        //剔除错误点，与最佳匹配点是邻居的点认为是正确点
        int Sita11Num=0;
        int Sita12Num=0;
        int Sita13Num=0;
        int Sita14Num=0;
        float Sita11Sum=0;
        float Sita12Sum=0;
        float Sita13Sum=0;
        float Sita14Sum=0;
        for(i=0;i<DiskNum;i++)
        {
            if(BestPoints[i].x==0 && BestPoints[i].y==0)
            {
                BestPoints[i]=Point(0,0);
            }
            else
            {
                float AvectorAngle=AnglePolar(DisksCenterPoints[i]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
                float BvectorAngle=AnglePolar(BestPoints[i]-FirstBestPoint);//实际向量
                //两向量夹角0-360°
                float Sita=BvectorAngle-AvectorAngle;
                if(Sita<0) Sita=Sita+360;

                if(fabs(Sita-Sita11)<20)
                {
                    Sita11Sum=Sita11Sum+Sita11;
                    Sita11Num++;
                }
                if(fabs(Sita-Sita12)<20)
                {
                    Sita12Sum=Sita12Sum+Sita12;
                    Sita12Num++;
                }
                if(fabs(Sita-Sita13)<20)
                {
                    Sita13Sum=Sita13Sum+Sita13;
                    Sita13Num++;
                }
                if(fabs(Sita-Sita14)<20)
                {
                    Sita14Sum=Sita14Sum+Sita14;
                    Sita14Num++;
                }
            }
        }


        timeMBNS=((double)getTickCount() - timeMBNS)/getTickFrequency();//运行时间
        qDebug("MBNS耗时：%f秒",timeMBNS);
        Mat ImageFinalResult2;//FinalOne
        GrayImage.copyTo(ImageFinalResult2);
        circle(ImageFinalResult2,FirstBestPoint,2,CV_RGB(255,255,255),2,8,0);

        int SitaMostNum=Sita11Num;
        float Sita=Sita11Sum/Sita11Num;
        if(SitaMostNum<Sita12Num)
        {
            SitaMostNum=Sita12Num;
            Sita=Sita12Sum/Sita12Num;
        }
        if(SitaMostNum<Sita13Num)
        {
            SitaMostNum=Sita13Num;
            Sita=Sita13Sum/Sita13Num;
        }
        if(SitaMostNum<Sita14Num)
        {
            SitaMostNum=Sita14Num;
            Sita=Sita14Sum/Sita14Num;
        }

        Point RectUplPoint;//矩形左上角坐标
        RectUplPoint.x=(int)(-DisksCenterPoints[FirstBestPointSort].x*cos(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*sin(Sita/180*3.14) + FirstBestPoint.x);
        RectUplPoint.y=(int)(-(-DisksCenterPoints[FirstBestPointSort].x)*sin(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*cos(Sita/180*3.14)  + FirstBestPoint.y);
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)Sita);


    /*
        //利用2个最佳匹配点画矩形框
        //计算两向量夹角
        float AvectorAngle=AnglePolar(DisksCenterPoints[SecondBestPointSort]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
        float BvectorAngle=AnglePolar(SecondBestPoint-FirstBestPoint);//实际向量
        //两向量夹角0-360°
        float Sita=BvectorAngle-AvectorAngle;
        if(Sita<0) Sita=Sita+360;


        Point RectUplPoint;//矩形左上角坐标
        RectUplPoint.x=(int)(-DisksCenterPoints[FirstBestPointSort].x*cos(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*sin(Sita/180*3.14) + FirstBestPoint.x);
        RectUplPoint.y=(int)(-(-DisksCenterPoints[FirstBestPointSort].x)*sin(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*cos(Sita/180*3.14)  + FirstBestPoint.y);
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)Sita);
    */

        //显示结果
        qDebug("最终匹配的矩形左上角坐标=(%d,%d);旋转角=%f°",RectUplPoint.x,RectUplPoint.y,0);
        imshow("ImageCandidates",GrayImageDisp);//局部特征粗匹配得到候选匹配点集
        imshow("ImageBestMatches",ImageBestMatches);
        imshow("ImageFinalResult",ImageFinalResult);//Most Neighbors Similarity: 获取9个最佳匹配点
        imshow("ImageFinalOne",ImageFinalResult2);//Most Neighbors Similarity: 从9个匹配点中获取最佳的2个匹配点

        //***********************************************************************
        //***********************************************************************
        timeSum=((double)getTickCount() - timeSum)/getTickFrequency();//运行时间
        qDebug("模板匹配结束，总耗时：%f秒",timeSum);
        qDebug("***********************");

    /*
        Mat srcImageDisp;
        //resize函数前加cv：：，因为Qt也有resize函数，故加cv：：加以区别，不然会出现错误
        int resizeW=(int)ImageFinalResult2.cols/(beta+1);
        int resizeH=(int)ImageFinalResult2.rows/(beta+1);
        cv::resize(ImageFinalResult2,srcImageDisp,Size(resizeW,resizeH),0,0,3);
        Imgview->imshow(srcImageDisp);//显示到绑定的控件上

    */

        //返回结果参数
        MatchUpLPoint[0]=RectUplPoint;
        MatchTime[0]=timeSum;
    }
    else
    {
        //返回结果参数
        MatchUpLPoint[0]=Point(0,0);
        MatchTime[0]=0;
    }

}

//特征：环投影均值和方差、灰度均值、HSV各分量均值
void MBNShsv_SSDtMatching3(Mat Image,Mat TemplateImage, Point* MatchUpLPoint, float* MatchTime){

    //分离颜色通道
    Mat Image_h,Image_s,Image_v,Imagehsv;
    if(Image.channels()!=1)
    {
        //分离颜色通道HSV
        vector<Mat> hsv_vec;
        cvtColor(Image, Imagehsv, CV_BGR2HSV);
        split(Imagehsv, hsv_vec);
        Image_h = hsv_vec[0];
        Image_s = hsv_vec[1];
        Image_v = hsv_vec[2];
        Image_h.convertTo(Image_h, CV_8UC1);
        Image_s.convertTo(Image_s, CV_8UC1);
        Image_v.convertTo(Image_v, CV_8UC1);
    }

    int i,j;
    //一、图像和模板都转为单通道的灰度图
    Mat GrayImage;
    cvtColor(Image,GrayImage,CV_RGB2GRAY,1);
    Mat GrayTemplate;
    cvtColor(TemplateImage,GrayTemplate,CV_RGB2GRAY,1);
    qDebug("待匹配图宽高=【%d x %d】；模板图宽高=【%d x %d】",GrayImage.cols,GrayImage.rows,GrayTemplate.cols,GrayTemplate.rows);

    //二、将模板图分成多个Ldisk*Ldisk的小方块，每个方块圆心相距
    R=(int)Ldisk/2;//局部方块半径
    int tt=2;//初始距离倍数
    int disL=(int)Ldisk/tt;//每个方块圆心相距的距离
    int WidthNum=(int)GrayTemplate.cols/Ldisk*tt - tt;//一行的方块数
    int HeightNum=(int)GrayTemplate.rows/Ldisk*tt - tt;//一列的方块数
    if(Ldisk==9 || WidthNum*HeightNum<70)
    {
        while(WidthNum*HeightNum<70)
        {
            tt++;
            disL=(int)(Ldisk/tt);//每个方块圆心相距的距离
            WidthNum=(int)GrayTemplate.cols/Ldisk*tt - tt;//一行的方块数
            HeightNum=(int)GrayTemplate.rows/Ldisk*tt - tt;//一列的方块数
        }
    }
    Thresh_distance=(float)(PSMode*Ldisk/tt);//邻居距离阈值
    qDebug("邻居距离阈值=%f",Thresh_distance);
    qDebug("模板方块边长=%d",Ldisk);
    qDebug("模板方块数量=%d",WidthNum*HeightNum);

    //四（1）、建立环投影的环查找表
    int x[R]={0};//环距离
    x[0]=1;
    int xnum=1;
    for(i=0;i<R;i++)
    {
        x[i+1]=(int)(R-sqrt((R-x[i])*(R-x[i])-(2*R-1))+0.5);
        xnum++;
        if(x[i+1]<=x[i])
        {
            x[i+1]=0;
            xnum--;
            break;
        }
    }

    uint AnnulusTable[Ldisk*Ldisk];//环查找表
    uint anNumber[xnum]={0};//统计同个环的像素数量
    uint Rtable[Ldisk*Ldisk];//半径查找表
    uint rNumber[R+1]={0};//统计同个半径值的数量
    uint aNumber[360]={0};//统计同个角度值的数量
    uint Atable[Ldisk*Ldisk];//角度查找表
    double YdivX;//斜率
    int A=0;//角度
    int r;//半径

    for(i=0;i<Ldisk;i++)
    {
        for(j=0;j<Ldisk;j++)
        {
            r=(int)(sqrt((i-R)*(i-R)+(j-R)*(j-R)) + 0.5);

            //建立极坐标（半径和角度）查找表
            //角度转换
             if(j>R-1 && i==R)
             {
                 A=0;
             }
             else if(j<R && i==R)
             {
                 A=180;
             }
             else if(j==R && i<R)
             {
                 A=90;
             }
             else if(j==R && i>R)
             {
                 A=270;
             }
             else if(j>R && i<R)
             {
                 YdivX=(double)(R-i)/(j-R);
                 A=(int)(atan(YdivX)*180/3.14159);
             }
             else if(j<R && i<R)
             {
                 YdivX=(double)(R-j)/(R-i);
                 A=(int)(atan(YdivX)*180/3.14159)+90;
             }
             else if(j<R && i>R)
             {
                 YdivX=(double)(i-R)/(R-j);
                 A=(int)(atan(YdivX)*180/3.14159)+180;
             }
             else if(j>R && i>R)
             {
                 YdivX=(double)(j-R)/(i-R);
                 A=(int)(atan(YdivX)*180/3.14159)+270;
             }


             if(r>R)
             {
                 Rtable[i*Ldisk+j] = R+1;
                 Atable[i*Ldisk+j] = 361;
             }
             else
             {
                 Rtable[i*Ldisk+j] = r;
                 Atable[i*Ldisk+j] = A;
                 rNumber[r]++;
                 aNumber[A]++;
             }

            // **************************
            // **************************
            //建立环查找表
            if(r>R)
            {
                AnnulusTable[i*Ldisk+j] = xnum;
            }
            else
            {
                for(int ii=0;ii<xnum;ii++)
                {
                    if(r>(R-x[ii]))
                    {
                        AnnulusTable[i*Ldisk+j] = ii;
                        anNumber[ii]++;
                        break;
                    }
                }

                if(r<=(R-x[xnum-1]))
                {
                    AnnulusTable[i*Ldisk+j] = xnum-1;
                    anNumber[xnum-1]++;
                }
            }

        }
    }

    //四（2）、计算模板图的环投影向量
    vector<Mat> Disks;//方块
    vector<Point> DisksCenterPoints;//每个方块的中心坐标点
    int DiskNum=0;//真正的方块数量
    int iarray[WidthNum*HeightNum]={-1};//正真方块的序号
    for(i=0;i<WidthNum*HeightNum;i++)
    {
        Mat GrayTemplatetemp(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
        float Ptemp[xnum]={0};
        Anvector(GrayTemplatetemp,AnnulusTable,xnum,anNumber,Ptemp);
        float PtempAver=0;
        for(j=0;j<xnum;j++)
        {
            PtempAver=PtempAver+Ptemp[j];
        }
        PtempAver=PtempAver/xnum;
        float P_SSD=0;
        for(j=0;j<xnum;j++)
        {
            P_SSD=P_SSD+(Ptemp[j]-PtempAver)*(Ptemp[j]-PtempAver);
        }

        if(P_SSD>4*xnum)
        {
            Disks.push_back(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
            DisksCenterPoints.push_back(Point(((int)i%WidthNum)*disL+R,((int)i/WidthNum)*disL+R));
            DiskNum++;
            iarray[i]=i;
        }

    }
    qDebug("真正的方块数量= %d",DiskNum);

    //DiskNum<=30的话直接放弃匹配
    if(DiskNum>30)
    {
        //环投影向量
        float P[DiskNum][xnum]={0};//模板图环投影向量
        float Pvariance[DiskNum][xnum]={0};//模板图环投影向量方差
        for(i=0;i<DiskNum;i++)
        {
            //环投影向量
            Anvector2(Disks[i],AnnulusTable,xnum,anNumber,P[i],Pvariance[i]);

        }
        //添加颜色特征
        float P_h[DiskNum]={0};//模板图h分量均值
        float P_s[DiskNum]={0};//模板图s分量均值
        float P_v[DiskNum]={0};//模板图v分量均值
        if(TemplateImage.channels()!=1)
        {
            //分离颜色通道HSV
            Mat TemplateImage_h,TemplateImage_s,TemplateImage_v,TemplateImagehsv;
            vector<Mat> hsv_vec;
            cvtColor(TemplateImage, TemplateImagehsv, CV_BGR2HSV);
            split(TemplateImagehsv, hsv_vec);
            TemplateImage_h = hsv_vec[0];
            TemplateImage_s = hsv_vec[1];
            TemplateImage_v = hsv_vec[2];
            TemplateImage_h.convertTo(TemplateImage_h, CV_8UC1);
            TemplateImage_s.convertTo(TemplateImage_s, CV_8UC1);
            TemplateImage_v.convertTo(TemplateImage_v, CV_8UC1);

            int iii=0;
            for(i=0;i<WidthNum*HeightNum;i++)
            {
                if(iarray[i]==i)
                {
                    Mat Disks_h=TemplateImage_h(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    Scalar tempVal = cv::mean( Disks_h );
                    P_h[iii]= tempVal.val[0];

                    Mat Disks_s=TemplateImage_s(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = cv::mean( Disks_s );
                    P_s[iii]= tempVal.val[0];

                    Mat Disks_v=TemplateImage_v(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = cv::mean( Disks_v );
                    P_v[iii]= tempVal.val[0];

                    iii++;
                }
            }

        }
        //添加灰度均值和方差特征
        float P_grayM[DiskNum]={0};//模板图灰度均值
        float P_grayD[DiskNum]={0};//模板图灰度方差
        int iii=0;
        for(i=0;i<WidthNum*HeightNum;i++)
        {
            if(iarray[i]==i)
            {
                Mat Disks_gray=GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                Scalar Mean,dev;
                cv::meanStdDev(Disks_gray,Mean,dev);
                P_grayM[iii]= Mean.val[0];
                P_grayD[iii]= dev.val[0];

                iii++;
            }
        }

        //*****五、开始匹配************************************
        //*******************************************
        double timeSum;//总耗时
        timeSum=static_cast<double>(getTickCount());
        //***********************************************************
        double timeAnnulus;
        timeAnnulus=static_cast<double>(getTickCount());
        //设置圆投影搜索范围
        int LeftUpx=0;
        int LeftUpy=0;
        int RrightDownx=(int)(GrayImage.cols-Ldisk-1);
        int RrightDowny=(int)(GrayImage.rows-Ldisk-1);

        //获取并标出候选点位置
        double RminValue=0;
        Point RminLoc;
        double RmaxValue=0;
        Point RmaxLoc;
        Mat GrayImageDisp;
        GrayImage.copyTo(GrayImageDisp);

        int CandidateNum=MatchingNum2;
        vector<Point> Points[DiskNum];//精确匹配点坐标数组
        vector<Mat> MatchingValue;//环投影相似度值
        for(int ii=0;ii<DiskNum;ii++)
        {
            Mat MatchingValueTemp(GrayImage.rows-Ldisk,GrayImage.cols-Ldisk,CV_32FC1,Scalar(999999999));//float型单通道，每个点初始化
            MatchingValue.push_back(MatchingValueTemp);
        }
        for(i=LeftUpy;i<RrightDowny+1;i++)
        {

            for(j=LeftUpx;j<RrightDownx;j++)
            {

                Mat ScanROI;//待匹配的子图
                ScanROI=GrayImage(Rect(j,i,Ldisk,Ldisk));
                float T[xnum]={0};//模板图环投影向量
                float Tvariance[xnum]={0};//模板图环投影向量方差
                Anvector2(ScanROI,AnnulusTable,xnum,anNumber,T,Tvariance);
                Scalar Mean,dev;
                cv::meanStdDev(ScanROI,Mean,dev);
                float T_grayM= Mean.val[0];
                float T_grayD= dev.val[0];

                //添加颜色特征
                float T_h=0;//h分量均值
                float T_s=0;//s分量均值
                float T_v=0;//v分量均值
                if(Image.channels()!=1)
                {
                    Mat ScanROI_hsv;//待匹配的子图
                    ScanROI_hsv=Image_h(Rect(j,i,Ldisk,Ldisk));
                    Scalar tempVal = cv::mean( ScanROI_hsv );
                    T_h= tempVal.val[0];

                    ScanROI_hsv=Image_s(Rect(j,i,Ldisk,Ldisk));
                    tempVal = cv::mean( ScanROI_hsv );
                    T_s= tempVal.val[0];

                    ScanROI_hsv=Image_v(Rect(j,i,Ldisk,Ldisk));
                    tempVal = cv::mean( ScanROI_hsv );
                    T_v= tempVal.val[0];

                }

                for(int ii=0;ii<DiskNum;ii++)
                {
                    float* buffer;
                    buffer=MatchingValue[ii].ptr<float>(i);
                    //相似度计算
                    int tempbuffer=0;
                    for(int k=0;k<xnum;k++)
                    {
                        tempbuffer=tempbuffer+(P[ii][k]-T[k])*(P[ii][k]-T[k])+(Pvariance[ii][k]-Tvariance[k])*(Pvariance[ii][k]-Tvariance[k]);

                    }
                    buffer[j]=tempbuffer+(P_h[ii]-T_h)*(P_h[ii]-T_h)+(P_s[ii]-T_s)*(P_s[ii]-T_s)+(P_v[ii]-T_v)*(P_v[ii]-T_v)+(P_grayM[ii]-T_grayM)*(P_grayM[ii]-T_grayM)+(P_grayD[ii]-T_grayD)*(P_grayD[ii]-T_grayD);

                }

            }
        }

        for(int ii=0;ii<DiskNum;ii++)
        {
            for(i=0;i<CandidateNum;i++)
            {
                //1、选取候选匹配点
                minMaxLoc(MatchingValue[ii],&RminValue,&RmaxValue,&RminLoc,&RmaxLoc);
                Point BestMatch=RminLoc;
                Point Centerpoint=Point(BestMatch.x+R,BestMatch.y+R);

                //画圆
                circle(GrayImageDisp,Centerpoint,2,CV_RGB(255,255,255),2,8,0);
                Points[ii].push_back(Centerpoint);

                float* buffervalue=MatchingValue[ii].ptr<float>(BestMatch.y);
                buffervalue[BestMatch.x]=999999999;
            }
        }
        timeAnnulus=((double)getTickCount() - timeAnnulus)/getTickFrequency();//运行时间
        //qDebug("环投影匹配耗时：%f秒",timeAnnulus);


        double timeMBNS;//MBNS耗时
        timeMBNS=static_cast<double>(getTickCount());
        //***************************************************
        //Neighbors Constraint Similarity确定最终匹配点
        //***************************************************
        Point BestPoints[DiskNum];//最佳匹配点
        int BestPointNum[DiskNum]={0};//最佳匹配点邻居数量
        Mat ImageFinalResult;//显示结果
        GrayImage.copyTo(ImageFinalResult);

        for(int ii=0;ii<DiskNum;ii++)
        {

            for(i=0;i<Points[ii].size();i++)
            {

                float tempMinDis=999999999;

                //第二多的邻居数量
                int PointNum=0;
                Point Pointtemp(0,0);
                Point Pointss[DiskNum];
                for(j=0;j<DiskNum;j++)
                {
                    Pointss[j]=Point(0,0);
                    if(j==ii)   Pointss[j]=Points[ii][i];
                }

                for(int jj=0;jj<DiskNum;jj++)
                {
                    //获取邻居点坐标
                    for(j=0;j<Points[jj].size();j++)
                    {
                        if(jj!=ii)
                        {
                            float distance=get_distance(Points[ii][i],Points[jj][j]);
                            float distemp=get_distance(DisksCenterPoints[ii],DisksCenterPoints[jj]);
                            distance=fabs(distance-distemp);//距离
                            if(distance<tempMinDis)
                            {
                                tempMinDis=distance;
                                Pointtemp=Points[jj][j];
                            }
                        }

                    }
                    if(tempMinDis<Thresh_distance)
                    {
                        Pointss[jj]=Pointtemp;
                        PointNum++;//第一多的邻居数量
                    }
                    tempMinDis=999999999;

                }
                //阈值：第一多的邻居数量不少于Thresh_MBNSnum个
                if(PointNum>Thresh_MBNSnum)
                {
                    PointNum=0;
                    //计算第二多的邻居数量
                    int PointNumtemp=0;
                    for(int tt=0;tt<DiskNum;tt++)
                    {
                        if(tt!=ii)
                        {
                            PointNumtemp=CountNeighbors(tt, Pointss, DiskNum,DisksCenterPoints, Thresh_distance);
                            if(PointNum<PointNumtemp)
                            {
                                PointNum=PointNumtemp;//第二多的邻居数量
                            }
                        }
                    }

                    //获取最佳点坐标
                    if(PointNum>BestPointNum[ii])
                    {
                        BestPoints[ii]=Points[ii][i];
                        BestPointNum[ii]=PointNum;
                    }

                }

            }
            if(BestPointNum[ii]<Thresh_MBNSnum)  BestPoints[ii]=Point(0,0);//阈值：第二多邻居数量不少于Thresh_MBNSnum


        }

        //****从精确匹配得到的WidthNum*HeightNum个点中确定最终匹配点****
        Point FirstBestPoint=BestPoints[0];
        int FirstBestPointNum=0;
        int FirstBestPointSort=0;
        for(i=0;i<DiskNum;i++)
        {
            int PointNum=0;

            PointNum=CountNeighbors(i, BestPoints, DiskNum,DisksCenterPoints, Thresh_distance);//Thresh_distance=R
            //获取最佳点
            if(PointNum>FirstBestPointNum)
            {
                FirstBestPoint=BestPoints[i];
                FirstBestPointNum=PointNum;
                FirstBestPointSort=i;
            }


        }

        //剔除错误点，与最佳匹配点是邻居的点认为是正确点
        for(i=0;i<DiskNum;i++)
        {
            if(BestPoints[i].x==0 && BestPoints[i].y==0)
            {
                BestPoints[i]=Point(0,0);
            }
            else
            {
                float distance=get_distance(FirstBestPoint,BestPoints[i]);
                float distemp=get_distance(DisksCenterPoints[FirstBestPointSort],DisksCenterPoints[i]);
                distance=fabs(distance-distemp);//距离
                if(distance>Thresh_distance)
                {
                    BestPoints[i]=Point(0,0);
                }
            }
            circle(ImageFinalResult,BestPoints[i],2,CV_RGB(255,255,255),2,8,0);
        }

        //确定矩形框坐标
        Point RectUplPoint=Point(0,0);//矩形左上角坐标
        int rectupnum=0;
        for(i=0;i<DiskNum;i++)
        {
            if(BestPoints[i].x==0 && BestPoints[i].y==0)
            {
                BestPoints[i]=Point(0,0);
            }
            else
            {
                //计算每个正确点的邻居数量，此数量作为矩形框坐标计算的权重
                int PointNum=0;
                PointNum=CountNeighbors(i, BestPoints, DiskNum,DisksCenterPoints, Thresh_distance);//Thresh_distance=R

                RectUplPoint=RectUplPoint+PointNum*(BestPoints[i]-DisksCenterPoints[i]);
                rectupnum=rectupnum+PointNum;

            }


            circle(ImageFinalResult,BestPoints[i],2,CV_RGB(255,255,255),2,8,0);
        }

        timeMBNS=((double)getTickCount() - timeMBNS)/getTickFrequency();//运行时间
        qDebug("MBNS耗时：%f秒",timeMBNS);
        Mat ImageFinalResult2;
        GrayImage.copyTo(ImageFinalResult2);
        circle(ImageFinalResult2,FirstBestPoint,2,CV_RGB(255,255,255),2,8,0);


        RectUplPoint.x=(int)RectUplPoint.x/rectupnum;
        RectUplPoint.y=(int)RectUplPoint.y/rectupnum;
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)0);


    /*
        //利用2个最佳匹配点画矩形框
        //计算两向量夹角
        float AvectorAngle=AnglePolar(DisksCenterPoints[SecondBestPointSort]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
        float BvectorAngle=AnglePolar(SecondBestPoint-FirstBestPoint);//实际向量
        //两向量夹角0-360°
        float Sita=BvectorAngle-AvectorAngle;
        if(Sita<0) Sita=Sita+360;


        Point RectUplPoint;//矩形左上角坐标
        RectUplPoint.x=(int)(-DisksCenterPoints[FirstBestPointSort].x*cos(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*sin(Sita/180*3.14) + FirstBestPoint.x);
        RectUplPoint.y=(int)(-(-DisksCenterPoints[FirstBestPointSort].x)*sin(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*cos(Sita/180*3.14)  + FirstBestPoint.y);
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)Sita);
    */

        //显示结果
        qDebug("最终匹配的矩形左上角坐标=(%d,%d);旋转角=%f°",RectUplPoint.x,RectUplPoint.y,0);
        imshow("GrayImageDisp",GrayImageDisp);//局部特征粗匹配得到候选匹配点集
        imshow("ImageFinalResult",ImageFinalResult);//Most Neighbors Similarity: 获取9个最佳匹配点
        imshow("ImageFinalResult2",ImageFinalResult2);//Most Neighbors Similarity: 从9个匹配点中获取最佳的2个匹配点

        //***********************************************************************
        //***********************************************************************
        timeSum=((double)getTickCount() - timeSum)/getTickFrequency();//运行时间
        qDebug("模板匹配结束，总耗时：%f秒",timeSum);
        qDebug("***********************");

    /*
        Mat srcImageDisp;
        //resize函数前加cv：：，因为Qt也有resize函数，故加cv：：加以区别，不然会出现错误
        int resizeW=(int)ImageFinalResult2.cols/(beta+1);
        int resizeH=(int)ImageFinalResult2.rows/(beta+1);
        cv::resize(ImageFinalResult2,srcImageDisp,Size(resizeW,resizeH),0,0,3);
        Imgview->imshow(srcImageDisp);//显示到绑定的控件上

    */

        //返回结果参数
        MatchUpLPoint[0]=RectUplPoint;
        MatchTime[0]=timeSum;
    }
    else
    {
        //返回结果参数
        MatchUpLPoint[0]=Point(0,0);
        MatchTime[0]=0;
    }

}

//特征：环投影均值、灰度均值、HSV各分量均值
void MBNS2hsv_SSDtMatching2(Mat Image,Mat TemplateImage, Point* MatchUpLPoint, float* MatchTime){

    //分离颜色通道
    Mat Image_h,Image_s,Image_v,Imagehsv;
    if(Image.channels()!=1)
    {
        //分离颜色通道HSV
        vector<Mat> hsv_vec;
        cvtColor(Image, Imagehsv, CV_BGR2HSV);
        split(Imagehsv, hsv_vec);
        Image_h = hsv_vec[0];
        Image_s = hsv_vec[1];
        Image_v = hsv_vec[2];
        Image_h.convertTo(Image_h, CV_8UC1);
        Image_s.convertTo(Image_s, CV_8UC1);
        Image_v.convertTo(Image_v, CV_8UC1);
    }

    int i,j;
    //一、图像和模板都转为单通道的灰度图
    Mat GrayImage;
    cvtColor(Image,GrayImage,CV_RGB2GRAY,1);
    Mat GrayTemplate;
    cvtColor(TemplateImage,GrayTemplate,CV_RGB2GRAY,1);
    qDebug("待匹配图宽高=【%d x %d】；模板图宽高=【%d x %d】",GrayImage.cols,GrayImage.rows,GrayTemplate.cols,GrayTemplate.rows);

    //二、将模板图分成多个Ldisk*Ldisk的小方块，每个方块圆心相距
    R=(int)Ldisk/2;//局部方块半径
    int tt=2;//初始距离倍数
    int disL=(int)Ldisk/tt;//每个方块圆心相距的距离
    int WidthNum=(int)GrayTemplate.cols/Ldisk*tt - tt;//一行的方块数
    int HeightNum=(int)GrayTemplate.rows/Ldisk*tt - tt;//一列的方块数
    if(Ldisk==9 || WidthNum*HeightNum<70)
    {
        while(WidthNum*HeightNum<70)
        {
            tt++;
            disL=(int)(Ldisk/tt);//每个方块圆心相距的距离
            WidthNum=(int)GrayTemplate.cols/Ldisk*tt - tt;//一行的方块数
            HeightNum=(int)GrayTemplate.rows/Ldisk*tt - tt;//一列的方块数
        }
    }
    Thresh_distance=(float)(PSMode*Ldisk/tt);//邻居距离阈值
    qDebug("邻居距离阈值=%f",Thresh_distance);
    qDebug("模板方块边长=%d",Ldisk);
    qDebug("模板方块数量=%d",WidthNum*HeightNum);

    //四（1）、建立环投影的环查找表
    int x[R]={0};//环距离
    x[0]=1;
    int xnum=1;
    for(i=0;i<R;i++)
    {
        x[i+1]=(int)(R-sqrt((R-x[i])*(R-x[i])-(2*R-1))+0.5);
        xnum++;
        if(x[i+1]<=x[i])
        {
            x[i+1]=0;
            xnum--;
            break;
        }
    }

    uint AnnulusTable[Ldisk*Ldisk];//环查找表
    uint anNumber[xnum]={0};//统计同个环的像素数量
    uint Rtable[Ldisk*Ldisk];//半径查找表
    uint rNumber[R+1]={0};//统计同个半径值的数量
    uint aNumber[360]={0};//统计同个角度值的数量
    uint Atable[Ldisk*Ldisk];//角度查找表
    double YdivX;//斜率
    int A=0;//角度
    int r;//半径

    for(i=0;i<Ldisk;i++)
    {
        for(j=0;j<Ldisk;j++)
        {
            r=(int)(sqrt((i-R)*(i-R)+(j-R)*(j-R)) + 0.5);

            //建立极坐标（半径和角度）查找表
            //角度转换
             if(j>R-1 && i==R)
             {
                 A=0;
             }
             else if(j<R && i==R)
             {
                 A=180;
             }
             else if(j==R && i<R)
             {
                 A=90;
             }
             else if(j==R && i>R)
             {
                 A=270;
             }
             else if(j>R && i<R)
             {
                 YdivX=(double)(R-i)/(j-R);
                 A=(int)(atan(YdivX)*180/3.14159);
             }
             else if(j<R && i<R)
             {
                 YdivX=(double)(R-j)/(R-i);
                 A=(int)(atan(YdivX)*180/3.14159)+90;
             }
             else if(j<R && i>R)
             {
                 YdivX=(double)(i-R)/(R-j);
                 A=(int)(atan(YdivX)*180/3.14159)+180;
             }
             else if(j>R && i>R)
             {
                 YdivX=(double)(j-R)/(i-R);
                 A=(int)(atan(YdivX)*180/3.14159)+270;
             }


             if(r>R)
             {
                 Rtable[i*Ldisk+j] = R+1;
                 Atable[i*Ldisk+j] = 361;
             }
             else
             {
                 Rtable[i*Ldisk+j] = r;
                 Atable[i*Ldisk+j] = A;
                 rNumber[r]++;
                 aNumber[A]++;
             }

            // **************************
            // **************************
            //建立环查找表
            if(r>R)
            {
                AnnulusTable[i*Ldisk+j] = xnum;
            }
            else
            {
                for(int ii=0;ii<xnum;ii++)
                {
                    if(r>(R-x[ii]))
                    {
                        AnnulusTable[i*Ldisk+j] = ii;
                        anNumber[ii]++;
                        break;
                    }
                }

                if(r<=(R-x[xnum-1]))
                {
                    AnnulusTable[i*Ldisk+j] = xnum-1;
                    anNumber[xnum-1]++;
                }
            }

        }
    }

    //四（2）、计算模板图的环投影向量
    vector<Mat> Disks;//方块
    vector<Point> DisksCenterPoints;//每个方块的中心坐标点
    int DiskNum=0;//真正的方块数量
    int iarray[WidthNum*HeightNum]={-1};//正真方块的序号
    for(i=0;i<WidthNum*HeightNum;i++)
    {
        Mat GrayTemplatetemp(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
        float Ptemp[xnum]={0};
        Anvector(GrayTemplatetemp,AnnulusTable,xnum,anNumber,Ptemp);
        float PtempAver=0;
        for(j=0;j<xnum;j++)
        {
            PtempAver=PtempAver+Ptemp[j];
        }
        PtempAver=PtempAver/xnum;
        float P_SSD=0;
        for(j=0;j<xnum;j++)
        {
            P_SSD=P_SSD+(Ptemp[j]-PtempAver)*(Ptemp[j]-PtempAver);
        }

        if(P_SSD>4*xnum)
        {
            Disks.push_back(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
            DisksCenterPoints.push_back(Point(((int)i%WidthNum)*disL+R,((int)i/WidthNum)*disL+R));
            DiskNum++;
            iarray[i]=i;
        }

    }
    qDebug("真正的方块数量= %d",DiskNum);

    //DiskNum<=30的话直接放弃匹配
    if(DiskNum>30)
    {
        //环投影向量
        float P[DiskNum][xnum]={0};//模板图环投影向量
        for(i=0;i<DiskNum;i++)
        {
            //环投影向量
            Anvector(Disks[i],AnnulusTable,xnum,anNumber,P[i]);

        }
        //添加颜色特征
        float P_h[DiskNum]={0};//模板图h分量均值
        float P_s[DiskNum]={0};//模板图s分量均值
        float P_v[DiskNum]={0};//模板图v分量均值
        if(TemplateImage.channels()!=1)
        {
            //分离颜色通道HSV
            Mat TemplateImage_h,TemplateImage_s,TemplateImage_v,TemplateImagehsv;
            vector<Mat> hsv_vec;
            cvtColor(TemplateImage, TemplateImagehsv, CV_BGR2HSV);
            split(TemplateImagehsv, hsv_vec);
            TemplateImage_h = hsv_vec[0];
            TemplateImage_s = hsv_vec[1];
            TemplateImage_v = hsv_vec[2];
            TemplateImage_h.convertTo(TemplateImage_h, CV_8UC1);
            TemplateImage_s.convertTo(TemplateImage_s, CV_8UC1);
            TemplateImage_v.convertTo(TemplateImage_v, CV_8UC1);

            int iii=0;
            for(i=0;i<WidthNum*HeightNum;i++)
            {
                if(iarray[i]==i)
                {
                    Mat Disks_h=TemplateImage_h(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    Scalar tempVal = cv::mean( Disks_h );
                    P_h[iii]= tempVal.val[0];

                    Mat Disks_s=TemplateImage_s(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = cv::mean( Disks_s );
                    P_s[iii]= tempVal.val[0];

                    Mat Disks_v=TemplateImage_v(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = cv::mean( Disks_v );
                    P_v[iii]= tempVal.val[0];

                    iii++;
                }
            }

        }
        //添加灰度均值和方差特征
        float P_grayM[DiskNum]={0};//模板图灰度均值
        float P_grayD[DiskNum]={0};//模板图灰度方差
        int iii=0;
        for(i=0;i<WidthNum*HeightNum;i++)
        {
            if(iarray[i]==i)
            {
                Mat Disks_gray=GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                Scalar Mean,dev;
                cv::meanStdDev(Disks_gray,Mean,dev);
                P_grayM[iii]= Mean.val[0];
                P_grayD[iii]= dev.val[0];

                iii++;
            }
        }

        //*****五、开始匹配************************************
        //*******************************************
        double timeSum;//总耗时
        timeSum=static_cast<double>(getTickCount());
        //***********************************************************
        double timeAnnulus;
        timeAnnulus=static_cast<double>(getTickCount());
        //设置圆投影搜索范围
        int LeftUpx=0;
        int LeftUpy=0;
        int RrightDownx=(int)(GrayImage.cols-Ldisk-1);
        int RrightDowny=(int)(GrayImage.rows-Ldisk-1);

        //获取并标出候选点位置
        double RminValue=0;
        Point RminLoc;
        double RmaxValue=0;
        Point RmaxLoc;
        Mat GrayImageDisp;
        GrayImage.copyTo(GrayImageDisp);

        int CandidateNum=MatchingNum2;
        vector<Point> Points[DiskNum];//精确匹配点坐标数组
        vector<Mat> MatchingValue;//环投影相似度值
        for(int ii=0;ii<DiskNum;ii++)
        {
            Mat MatchingValueTemp(GrayImage.rows-Ldisk,GrayImage.cols-Ldisk,CV_32FC1,Scalar(999999999));//float型单通道，每个点初始化
            MatchingValue.push_back(MatchingValueTemp);
        }
        for(i=LeftUpy;i<RrightDowny+1;i++)
        {

            for(j=LeftUpx;j<RrightDownx;j++)
            {

                Mat ScanROI;//待匹配的子图
                ScanROI=GrayImage(Rect(j,i,Ldisk,Ldisk));
                float T[xnum]={0};//模板图环投影向量
                Anvector(ScanROI,AnnulusTable,xnum,anNumber,T);
                Scalar Mean,dev;
                cv::meanStdDev(ScanROI,Mean,dev);
                float T_grayM= Mean.val[0];
                float T_grayD= dev.val[0];

                //添加颜色特征
                float T_h=0;//h分量均值
                float T_s=0;//s分量均值
                float T_v=0;//v分量均值
                if(Image.channels()!=1)
                {
                    Mat ScanROI_hsv;//待匹配的子图
                    ScanROI_hsv=Image_h(Rect(j,i,Ldisk,Ldisk));
                    Scalar tempVal = cv::mean( ScanROI_hsv );
                    T_h= tempVal.val[0];

                    ScanROI_hsv=Image_s(Rect(j,i,Ldisk,Ldisk));
                    tempVal = cv::mean( ScanROI_hsv );
                    T_s= tempVal.val[0];

                    ScanROI_hsv=Image_v(Rect(j,i,Ldisk,Ldisk));
                    tempVal = cv::mean( ScanROI_hsv );
                    T_v= tempVal.val[0];

                }

                for(int ii=0;ii<DiskNum;ii++)
                {
                    float* buffer;
                    buffer=MatchingValue[ii].ptr<float>(i);
                    //相似度计算
                    int tempbuffer=0;
                    for(int k=0;k<xnum;k++)
                    {
                        tempbuffer=tempbuffer+(P[ii][k]-T[k])*(P[ii][k]-T[k]);

                    }
                    buffer[j]=tempbuffer+(P_h[ii]-T_h)*(P_h[ii]-T_h)+(P_s[ii]-T_s)*(P_s[ii]-T_s)+(P_v[ii]-T_v)*(P_v[ii]-T_v)+(P_grayM[ii]-T_grayM)*(P_grayM[ii]-T_grayM)+(P_grayD[ii]-T_grayD)*(P_grayD[ii]-T_grayD);

                }

            }
        }

        for(int ii=0;ii<DiskNum;ii++)
        {
            for(i=0;i<CandidateNum;i++)
            {
                //1、选取候选匹配点
                minMaxLoc(MatchingValue[ii],&RminValue,&RmaxValue,&RminLoc,&RmaxLoc);
                Point BestMatch=RminLoc;
                Point Centerpoint=Point(BestMatch.x+R,BestMatch.y+R);

                //画圆
                circle(GrayImageDisp,Centerpoint,2,CV_RGB(255,255,255),2,8,0);
                Points[ii].push_back(Centerpoint);

                float* buffervalue=MatchingValue[ii].ptr<float>(BestMatch.y);
                buffervalue[BestMatch.x]=999999999;
            }
        }
        timeAnnulus=((double)getTickCount() - timeAnnulus)/getTickFrequency();//运行时间
        //qDebug("环投影匹配耗时：%f秒",timeAnnulus);


        double timeMBNS;//MBNS耗时
        timeMBNS=static_cast<double>(getTickCount());
        //***************************************************
        //Neighbors Constraint Similarity确定最终匹配点
        //***************************************************
        Point BestPoints[DiskNum];//最佳匹配点
        int BestPointNum[DiskNum]={0};//最佳匹配点邻居数量
        Mat ImageBestMatches;
        Mat ImageFinalResult;//最佳点的邻居
        GrayImage.copyTo(ImageFinalResult);
        GrayImage.copyTo(ImageBestMatches);

        for(int ii=0;ii<DiskNum;ii++)
        {

            for(i=0;i<Points[ii].size();i++)
            {

                float tempMinDis=999999999;

                int PointNum=0;//邻居数量总和
                Point Pointtemp(0,0);
                Point Pointss[DiskNum];
                for(j=0;j<DiskNum;j++)
                {
                    Pointss[j]=Point(0,0);
                    if(j==ii)   Pointss[j]=Points[ii][i];
                }

                for(int jj=0;jj<DiskNum;jj++)
                {
                    //获取邻居点坐标
                    for(j=0;j<Points[jj].size();j++)
                    {
                        if(jj!=ii)
                        {
                            float distance=get_distance(Points[ii][i],Points[jj][j]);
                            float distemp=get_distance(DisksCenterPoints[ii],DisksCenterPoints[jj]);
                            distance=fabs(distance-distemp);//距离
                            if(distance<tempMinDis)
                            {
                                tempMinDis=distance;
                                Pointtemp=Points[jj][j];
                            }
                        }

                    }
                    if(tempMinDis<Thresh_distance)
                    {
                        Pointss[jj]=Pointtemp;
                        PointNum++;//第一多的邻居数量
                    }
                    tempMinDis=999999999;

                }

                //******
                float PointNum1=PointNum;//第一多的邻居数量
                //计算第一多的邻居中的所有点的的邻居数量和
                int PointNumtemp=0;
                for(int tt=0;tt<DiskNum;tt++)
                {
                    if(tt!=ii)
                    {
                        PointNumtemp=CountNeighbors(tt, Pointss, DiskNum,DisksCenterPoints, Thresh_distance);
                        PointNum=PointNum+PointNumtemp;//邻居数量总和

                    }
                }

                //获取最佳点坐标
                if(PointNum/PointNum1>BestPointNum[ii])
                {
                    BestPoints[ii]=Points[ii][i];
                    BestPointNum[ii]=PointNum/PointNum1;
                }

            }
            if(BestPointNum[ii]<2)  BestPoints[ii]=Point(0,0);//阈值：邻居数量不少于Thresh_MBNSnum
            circle(ImageBestMatches,BestPoints[ii],2,CV_RGB(255,255,255),2,8,0);

        }

        //****从精确匹配得到的WidthNum*HeightNum个点中确定最终匹配点****
        Point FirstBestPoint=BestPoints[0];
        int FirstBestPointNum=0;
        int FirstBestPointSort=0;
  /*
        for(i=0;i<DiskNum;i++)
        {
            int PointNum=0;

            PointNum=CountNeighbors(i, BestPoints, DiskNum,DisksCenterPoints, Thresh_distance);//Thresh_distance=R
            //获取最佳点
            if(PointNum>FirstBestPointNum)
            {
                FirstBestPoint=BestPoints[i];
                FirstBestPointNum=PointNum;
                FirstBestPointSort=i;
            }


        }
        */
        for(int ii=0;ii<DiskNum;ii++)
        {

            int PointNum=0;//邻居数量总和
            Point Pointss[DiskNum];
            for(j=0;j<DiskNum;j++)
            {
                Pointss[j]=Point(0,0);
                if(j==ii)   Pointss[j]=BestPoints[ii];
            }

            for(int jj=0;jj<DiskNum;jj++)
            {
                //获取邻居点坐标
                if(jj!=ii)
                {
                    float distance=get_distance(BestPoints[ii],BestPoints[jj]);
                    float distemp=get_distance(DisksCenterPoints[ii],DisksCenterPoints[jj]);
                    distance=fabs(distance-distemp);//距离
                    if(distance<Thresh_distance)
                    {
                        Pointss[jj]=BestPoints[jj];
                        PointNum++;//第一多的邻居数量
                    }
                }
            }

            //******
            float PointNum1=PointNum;//第一多的邻居数量
            //计算第一多的邻居中的所有点的的邻居数量和
            int PointNumtemp=0;
            for(int tt=0;tt<DiskNum;tt++)
            {
                if(tt!=ii)
                {
                    PointNumtemp=CountNeighbors(tt, Pointss, DiskNum,DisksCenterPoints, Thresh_distance);
                    PointNum=PointNum+PointNumtemp;//邻居数量总和

                }
            }

            //获取最佳点坐标
            if(PointNum/PointNum1>FirstBestPointNum)
            {
                FirstBestPoint=BestPoints[ii];
                FirstBestPointNum=PointNum/PointNum1;
                FirstBestPointSort=ii;
            }
        }


        //剔除错误点，与最佳匹配点是邻居的点认为是正确点
        for(i=0;i<DiskNum;i++)
        {
            if(BestPoints[i].x==0 && BestPoints[i].y==0)
            {
                BestPoints[i]=Point(0,0);
            }
            else
            {
                float distance=get_distance(FirstBestPoint,BestPoints[i]);
                float distemp=get_distance(DisksCenterPoints[FirstBestPointSort],DisksCenterPoints[i]);
                distance=fabs(distance-distemp);//距离
                if(distance>Thresh_distance)
                {
                    BestPoints[i]=Point(0,0);
                }
            }
            circle(ImageFinalResult,BestPoints[i],2,CV_RGB(255,255,255),2,8,0);
        }

        //确定矩形框坐标
        Point RectUplPoint=Point(0,0);//矩形左上角坐标
        int rectupnum=0;
        for(i=0;i<DiskNum;i++)
        {
            if(BestPoints[i].x==0 && BestPoints[i].y==0)
            {
                BestPoints[i]=Point(0,0);
            }
            else
            {
                //计算每个正确点的邻居数量，此数量作为矩形框坐标计算的权重
                int PointNum=0;
                PointNum=CountNeighbors(i, BestPoints, DiskNum,DisksCenterPoints, Thresh_distance);//Thresh_distance=R

                RectUplPoint=RectUplPoint+PointNum*(BestPoints[i]-DisksCenterPoints[i]);
                rectupnum=rectupnum+PointNum;

            }

        }
        if(rectupnum==0)  rectupnum=1;

        timeMBNS=((double)getTickCount() - timeMBNS)/getTickFrequency();//运行时间
        qDebug("MBNS耗时：%f秒",timeMBNS);
        Mat ImageFinalResult2;
        GrayImage.copyTo(ImageFinalResult2);
        circle(ImageFinalResult2,FirstBestPoint,2,CV_RGB(255,255,255),2,8,0);


        RectUplPoint.x=(int)RectUplPoint.x/rectupnum;
        RectUplPoint.y=(int)RectUplPoint.y/rectupnum;
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)0);


    /*
        //利用2个最佳匹配点画矩形框
        //计算两向量夹角
        float AvectorAngle=AnglePolar(DisksCenterPoints[SecondBestPointSort]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
        float BvectorAngle=AnglePolar(SecondBestPoint-FirstBestPoint);//实际向量
        //两向量夹角0-360°
        float Sita=BvectorAngle-AvectorAngle;
        if(Sita<0) Sita=Sita+360;


        Point RectUplPoint;//矩形左上角坐标
        RectUplPoint.x=(int)(-DisksCenterPoints[FirstBestPointSort].x*cos(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*sin(Sita/180*3.14) + FirstBestPoint.x);
        RectUplPoint.y=(int)(-(-DisksCenterPoints[FirstBestPointSort].x)*sin(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*cos(Sita/180*3.14)  + FirstBestPoint.y);
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)Sita);
    */

        //显示结果
        qDebug("最终匹配的矩形左上角坐标=(%d,%d);旋转角=%f°",RectUplPoint.x,RectUplPoint.y,0);
        imshow("ImageCandidates",GrayImageDisp);//局部特征粗匹配得到候选匹配点集
        imshow("ImageBestMatches",ImageBestMatches);
        imshow("ImageFinalResult",ImageFinalResult);//Most Neighbors Similarity: 获取9个最佳匹配点
        imshow("ImageFinalOne",ImageFinalResult2);//Most Neighbors Similarity: 从9个匹配点中获取最佳的2个匹配点

        //***********************************************************************
        //***********************************************************************
        timeSum=((double)getTickCount() - timeSum)/getTickFrequency();//运行时间
        qDebug("模板匹配结束，总耗时：%f秒",timeSum);
        qDebug("***********************");

    /*
        Mat srcImageDisp;
        //resize函数前加cv：：，因为Qt也有resize函数，故加cv：：加以区别，不然会出现错误
        int resizeW=(int)ImageFinalResult2.cols/(beta+1);
        int resizeH=(int)ImageFinalResult2.rows/(beta+1);
        cv::resize(ImageFinalResult2,srcImageDisp,Size(resizeW,resizeH),0,0,3);
        Imgview->imshow(srcImageDisp);//显示到绑定的控件上

    */

        //返回结果参数
        MatchUpLPoint[0]=RectUplPoint;
        MatchTime[0]=timeSum;
    }
    else
    {
        //返回结果参数
        MatchUpLPoint[0]=Point(0,0);
        MatchTime[0]=0;
    }

}


//特征：环投影均值、灰度均值和方差、HSV和RGB各分量均值
void MBNShsvRGB_SSDtMatching2(Mat Image,Mat TemplateImage, Point* MatchUpLPoint, float* MatchTime){

    //分离颜色通道
    Mat Image_h,Image_s,Image_v,Imagehsv;
    Mat Image_r,Image_g,Image_b;
    if(Image.channels()>=3)
    {
        //分离颜色通道HSV
        vector<Mat> hsv_vec;
        cvtColor(Image, Imagehsv, CV_BGR2HSV);
        split(Imagehsv, hsv_vec);
        Image_h = hsv_vec[0];
        Image_s = hsv_vec[1];
        Image_v = hsv_vec[2];
        Image_h.convertTo(Image_h, CV_8UC1);
        Image_s.convertTo(Image_s, CV_8UC1);
        Image_v.convertTo(Image_v, CV_8UC1);

        //分离颜色通道RGB
        vector<Mat> rgb_vec;
        split(Image, rgb_vec);
        Image_r = rgb_vec[2];
        Image_g = rgb_vec[1];
        Image_b = rgb_vec[0];
    }

    int i,j;
    //一、图像和模板都转为单通道的灰度图
    Mat GrayImage;
    cvtColor(Image,GrayImage,CV_RGB2GRAY,1);
    Mat GrayTemplate;
    cvtColor(TemplateImage,GrayTemplate,CV_RGB2GRAY,1);
    qDebug("待匹配图宽高=【%d x %d】；模板图宽高=【%d x %d】",GrayImage.cols,GrayImage.rows,GrayTemplate.cols,GrayTemplate.rows);

    //二、将模板图分成多个Ldisk*Ldisk的小方块，每个方块圆心相距
    R=(int)Ldisk/2;//局部方块半径
    int tt=2;//初始距离倍数
    int disL=(int)Ldisk/tt;//每个方块圆心相距的距离
    int WidthNum=(int)GrayTemplate.cols/Ldisk*tt - tt;//一行的方块数
    int HeightNum=(int)GrayTemplate.rows/Ldisk*tt - tt;//一列的方块数
    if(Ldisk==9 || WidthNum*HeightNum<70)
    {
        while(WidthNum*HeightNum<70)
        {
            tt++;
            disL=(int)(Ldisk/tt);//每个方块圆心相距的距离
            WidthNum=(int)GrayTemplate.cols/Ldisk*tt - tt;//一行的方块数
            HeightNum=(int)GrayTemplate.rows/Ldisk*tt - tt;//一列的方块数
        }
    }
    Thresh_distance=(float)(PSMode*Ldisk/tt);//邻居距离阈值
    qDebug("邻居距离阈值=%f",Thresh_distance);
    qDebug("模板方块边长=%d",Ldisk);
    qDebug("模板方块数量=%d",WidthNum*HeightNum);

    //四（1）、建立环投影的环查找表
    int x[R]={0};//环距离
    x[0]=1;
    int xnum=1;
    for(i=0;i<R;i++)
    {
        x[i+1]=(int)(R-sqrt((R-x[i])*(R-x[i])-(2*R-1))+0.5);
        xnum++;
        if(x[i+1]<=x[i])
        {
            x[i+1]=0;
            xnum--;
            break;
        }
    }

    uint AnnulusTable[Ldisk*Ldisk];//环查找表
    uint anNumber[xnum]={0};//统计同个环的像素数量
    uint Rtable[Ldisk*Ldisk];//半径查找表
    uint rNumber[R+1]={0};//统计同个半径值的数量
    uint aNumber[360]={0};//统计同个角度值的数量
    uint Atable[Ldisk*Ldisk];//角度查找表
    double YdivX;//斜率
    int A=0;//角度
    int r;//半径

    for(i=0;i<Ldisk;i++)
    {
        for(j=0;j<Ldisk;j++)
        {
            r=(int)(sqrt((i-R)*(i-R)+(j-R)*(j-R)) + 0.5);

            //建立极坐标（半径和角度）查找表
            //角度转换
             if(j>R-1 && i==R)
             {
                 A=0;
             }
             else if(j<R && i==R)
             {
                 A=180;
             }
             else if(j==R && i<R)
             {
                 A=90;
             }
             else if(j==R && i>R)
             {
                 A=270;
             }
             else if(j>R && i<R)
             {
                 YdivX=(double)(R-i)/(j-R);
                 A=(int)(atan(YdivX)*180/3.14159);
             }
             else if(j<R && i<R)
             {
                 YdivX=(double)(R-j)/(R-i);
                 A=(int)(atan(YdivX)*180/3.14159)+90;
             }
             else if(j<R && i>R)
             {
                 YdivX=(double)(i-R)/(R-j);
                 A=(int)(atan(YdivX)*180/3.14159)+180;
             }
             else if(j>R && i>R)
             {
                 YdivX=(double)(j-R)/(i-R);
                 A=(int)(atan(YdivX)*180/3.14159)+270;
             }


             if(r>R)
             {
                 Rtable[i*Ldisk+j] = R+1;
                 Atable[i*Ldisk+j] = 361;
             }
             else
             {
                 Rtable[i*Ldisk+j] = r;
                 Atable[i*Ldisk+j] = A;
                 rNumber[r]++;
                 aNumber[A]++;
             }

            // **************************
            // **************************
            //建立环查找表
            if(r>R)
            {
                AnnulusTable[i*Ldisk+j] = xnum;
            }
            else
            {
                for(int ii=0;ii<xnum;ii++)
                {
                    if(r>(R-x[ii]))
                    {
                        AnnulusTable[i*Ldisk+j] = ii;
                        anNumber[ii]++;
                        break;
                    }
                }

                if(r<=(R-x[xnum-1]))
                {
                    AnnulusTable[i*Ldisk+j] = xnum-1;
                    anNumber[xnum-1]++;
                }
            }

        }
    }

    //四（2）、计算模板图的环投影向量
    vector<Mat> Disks;//方块
    vector<Point> DisksCenterPoints;//每个方块的中心坐标点
    int DiskNum=0;//真正的方块数量
    int iarray[WidthNum*HeightNum]={-1};//正真方块的序号
    for(i=0;i<WidthNum*HeightNum;i++)
    {
        Mat GrayTemplatetemp(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
        float Ptemp[xnum]={0};
        Anvector(GrayTemplatetemp,AnnulusTable,xnum,anNumber,Ptemp);
        float PtempAver=0;
        for(j=0;j<xnum;j++)
        {
            PtempAver=PtempAver+Ptemp[j];
        }
        PtempAver=PtempAver/xnum;
        float P_SSD=0;
        for(j=0;j<xnum;j++)
        {
            P_SSD=P_SSD+(Ptemp[j]-PtempAver)*(Ptemp[j]-PtempAver);
        }

        if(P_SSD>4*xnum)
        {
            Disks.push_back(GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk)));
            DisksCenterPoints.push_back(Point(((int)i%WidthNum)*disL+R,((int)i/WidthNum)*disL+R));
            DiskNum++;
            iarray[i]=i;
        }

    }
    qDebug("真正的方块数量= %d",DiskNum);

    //DiskNum<=30的话直接放弃匹配
    if(DiskNum>30)
    {
        //环投影向量
        float P[DiskNum][xnum]={0};//模板图环投影向量
        for(i=0;i<DiskNum;i++)
        {
            //环投影向量
            Anvector(Disks[i],AnnulusTable,xnum,anNumber,P[i]);

        }
        //添加颜色特征
        float P_h[DiskNum]={0};//模板图h分量均值
        float P_s[DiskNum]={0};//模板图s分量均值
        float P_v[DiskNum]={0};//模板图v分量均值
        float P_r[DiskNum]={0};//模板图r分量均值
        float P_g[DiskNum]={0};//模板图g分量均值
        float P_b[DiskNum]={0};//模板图b分量均值
        if(TemplateImage.channels()>=3)
        {
            //分离颜色通道HSV
            Mat TemplateImage_h,TemplateImage_s,TemplateImage_v,TemplateImagehsv;
            vector<Mat> hsv_vec;
            cvtColor(TemplateImage, TemplateImagehsv, CV_BGR2HSV);
            split(TemplateImagehsv, hsv_vec);
            TemplateImage_h = hsv_vec[0];
            TemplateImage_s = hsv_vec[1];
            TemplateImage_v = hsv_vec[2];
            TemplateImage_h.convertTo(TemplateImage_h, CV_8UC1);
            TemplateImage_s.convertTo(TemplateImage_s, CV_8UC1);
            TemplateImage_v.convertTo(TemplateImage_v, CV_8UC1);

            //分离颜色通道RGB
            Mat TemplateImage_r,TemplateImage_g,TemplateImage_b;
            vector<Mat> rgb_vec;
            split(TemplateImage, rgb_vec);
            TemplateImage_r = rgb_vec[2];
            TemplateImage_g = rgb_vec[1];
            TemplateImage_b = rgb_vec[0];

            int iii=0;
            for(i=0;i<WidthNum*HeightNum;i++)
            {
                if(iarray[i]==i)
                {
                    Mat Disks_h=TemplateImage_h(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    Scalar tempVal = cv::mean( Disks_h );
                    P_h[iii]= tempVal.val[0];

                    Mat Disks_s=TemplateImage_s(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = cv::mean( Disks_s );
                    P_s[iii]= tempVal.val[0];

                    Mat Disks_v=TemplateImage_v(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = cv::mean( Disks_v );
                    P_v[iii]= tempVal.val[0];

                    Mat Disks_r=TemplateImage_r(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = cv::mean( Disks_r );
                    P_r[iii]= tempVal.val[0];

                    Mat Disks_g=TemplateImage_g(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = cv::mean( Disks_g );
                    P_g[iii]= tempVal.val[0];

                    Mat Disks_b=TemplateImage_b(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                    tempVal = cv::mean( Disks_b );
                    P_b[iii]= tempVal.val[0];


                    iii++;
                }
            }

        }
        //添加灰度均值和方差特征
        float P_grayM[DiskNum]={0};//模板图灰度均值
        float P_grayD[DiskNum]={0};//模板图灰度方差
        int iii=0;
        for(i=0;i<WidthNum*HeightNum;i++)
        {
            if(iarray[i]==i)
            {
                Mat Disks_gray=GrayTemplate(Rect(((int)i%WidthNum)*disL,((int)i/WidthNum)*disL,Ldisk,Ldisk));
                Scalar Mean,dev;
                cv::meanStdDev(Disks_gray,Mean,dev);
                P_grayM[iii]= Mean.val[0];
                P_grayD[iii]= dev.val[0];

                iii++;
            }
        }

        //*****五、开始匹配************************************
        //*******************************************
        double timeSum;//总耗时
        timeSum=static_cast<double>(getTickCount());
        //***********************************************************
        double timeAnnulus;
        timeAnnulus=static_cast<double>(getTickCount());
        //设置圆投影搜索范围
        int LeftUpx=0;
        int LeftUpy=0;
        int RrightDownx=(int)(GrayImage.cols-Ldisk-1);
        int RrightDowny=(int)(GrayImage.rows-Ldisk-1);

        //获取并标出候选点位置
        double RminValue=0;
        Point RminLoc;
        double RmaxValue=0;
        Point RmaxLoc;
        Mat GrayImageDisp;
        GrayImage.copyTo(GrayImageDisp);

        int CandidateNum=MatchingNum2;
        vector<Point> Points[DiskNum];//精确匹配点坐标数组
        vector<Mat> MatchingValue;//环投影相似度值
        for(int ii=0;ii<DiskNum;ii++)
        {
            Mat MatchingValueTemp(GrayImage.rows-Ldisk,GrayImage.cols-Ldisk,CV_32FC1,Scalar(999999999));//float型单通道，每个点初始化
            MatchingValue.push_back(MatchingValueTemp);
        }
        for(i=LeftUpy;i<RrightDowny+1;i++)
        {

            for(j=LeftUpx;j<RrightDownx;j++)
            {

                Mat ScanROI;//待匹配的子图
                ScanROI=GrayImage(Rect(j,i,Ldisk,Ldisk));
                float T[xnum]={0};//模板图环投影向量
                Anvector(ScanROI,AnnulusTable,xnum,anNumber,T);
                Scalar Mean,dev;
                cv::meanStdDev(ScanROI,Mean,dev);
                float T_grayM= Mean.val[0];
                float T_grayD= dev.val[0];

                //添加颜色特征
                float T_h=0;//h分量均值
                float T_s=0;//s分量均值
                float T_v=0;//v分量均值
                float T_r=0;//r分量均值
                float T_g=0;//g分量均值
                float T_b=0;//b分量均值
                if(Image.channels()>=3)
                {
                    Mat ScanROI_hsv;//待匹配的子图
                    ScanROI_hsv=Image_h(Rect(j,i,Ldisk,Ldisk));
                    Scalar tempVal = cv::mean( ScanROI_hsv );
                    T_h= tempVal.val[0];

                    ScanROI_hsv=Image_s(Rect(j,i,Ldisk,Ldisk));
                    tempVal = cv::mean( ScanROI_hsv );
                    T_s= tempVal.val[0];

                    ScanROI_hsv=Image_v(Rect(j,i,Ldisk,Ldisk));
                    tempVal = cv::mean( ScanROI_hsv );
                    T_v= tempVal.val[0];

                    Mat ScanROI_r=Image_r(Rect(j,i,Ldisk,Ldisk));
                    tempVal = cv::mean( ScanROI_r );
                    T_r= tempVal.val[0];

                    Mat ScanROI_g=Image_g(Rect(j,i,Ldisk,Ldisk));
                    tempVal = cv::mean( ScanROI_g );
                    T_g= tempVal.val[0];

                    Mat ScanROI_b=Image_b(Rect(j,i,Ldisk,Ldisk));
                    tempVal = cv::mean( ScanROI_b );
                    T_b= tempVal.val[0];


                }

                for(int ii=0;ii<DiskNum;ii++)
                {
                    float* buffer;
                    buffer=MatchingValue[ii].ptr<float>(i);
                    //相似度计算
                    int tempbuffer=0;
                    for(int k=0;k<xnum;k++)
                    {
                        tempbuffer=tempbuffer+(P[ii][k]-T[k])*(P[ii][k]-T[k]);

                    }
                    float temp_hsv=(P_h[ii]-T_h)*(P_h[ii]-T_h)+(P_s[ii]-T_s)*(P_s[ii]-T_s)+(P_v[ii]-T_v)*(P_v[ii]-T_v);
                    float temp_rgb=(P_r[ii]-T_r)*(P_r[ii]-T_r)+(P_g[ii]-T_g)*(P_g[ii]-T_g)+(P_b[ii]-T_b)*(P_b[ii]-T_b);
                    float temp_gray=(P_grayM[ii]-T_grayM)*(P_grayM[ii]-T_grayM)+(P_grayD[ii]-T_grayD)*(P_grayD[ii]-T_grayD);
                    buffer[j]=tempbuffer+temp_hsv+temp_rgb+temp_gray;

                }

            }
        }

        for(int ii=0;ii<DiskNum;ii++)
        {
            for(i=0;i<CandidateNum;i++)
            {
                //1、选取候选匹配点
                minMaxLoc(MatchingValue[ii],&RminValue,&RmaxValue,&RminLoc,&RmaxLoc);
                Point BestMatch=RminLoc;
                Point Centerpoint=Point(BestMatch.x+R,BestMatch.y+R);

                //画圆
                circle(GrayImageDisp,Centerpoint,2,CV_RGB(255,255,255),2,8,0);
                Points[ii].push_back(Centerpoint);

                float* buffervalue=MatchingValue[ii].ptr<float>(BestMatch.y);
                buffervalue[BestMatch.x]=999999999;
            }
        }
        timeAnnulus=((double)getTickCount() - timeAnnulus)/getTickFrequency();//运行时间
        //qDebug("环投影匹配耗时：%f秒",timeAnnulus);


        double timeMBNS;//MBNS耗时
        timeMBNS=static_cast<double>(getTickCount());
        //***************************************************
        //Neighbors Constraint Similarity确定最终匹配点
        //***************************************************
        Point BestPoints[DiskNum];//最佳匹配点
        int BestPointNum[DiskNum]={0};//最佳匹配点邻居数量
        Mat ImageFinalResult;//显示结果
        GrayImage.copyTo(ImageFinalResult);

        for(int ii=0;ii<DiskNum;ii++)
        {

            for(i=0;i<Points[ii].size();i++)
            {

                float tempMinDis=999999999;

                //第二多的邻居数量
                int PointNum=0;
                Point Pointtemp(0,0);
                Point Pointss[DiskNum];
                for(j=0;j<DiskNum;j++)
                {
                    Pointss[j]=Point(0,0);
                    if(j==ii)   Pointss[j]=Points[ii][i];
                }

                for(int jj=0;jj<DiskNum;jj++)
                {
                    //获取邻居点坐标
                    for(j=0;j<Points[jj].size();j++)
                    {
                        if(jj!=ii)
                        {
                            float distance=get_distance(Points[ii][i],Points[jj][j]);
                            float distemp=get_distance(DisksCenterPoints[ii],DisksCenterPoints[jj]);
                            distance=fabs(distance-distemp);//距离
                            if(distance<tempMinDis)
                            {
                                tempMinDis=distance;
                                Pointtemp=Points[jj][j];
                            }
                        }

                    }
                    if(tempMinDis<Thresh_distance)
                    {
                        Pointss[jj]=Pointtemp;
                        PointNum++;//第一多的邻居数量
                    }
                    tempMinDis=999999999;

                }
                //阈值：第一多的邻居数量不少于Thresh_MBNSnum个
                if(PointNum>Thresh_MBNSnum)
                {
                    PointNum=0;
                    //计算第二多的邻居数量
                    int PointNumtemp=0;
                    for(int tt=0;tt<DiskNum;tt++)
                    {
                        if(tt!=ii)
                        {
                            PointNumtemp=CountNeighbors(tt, Pointss, DiskNum,DisksCenterPoints, Thresh_distance);
                            if(PointNum<PointNumtemp)
                            {
                                PointNum=PointNumtemp;//第二多的邻居数量
                            }
                        }
                    }

                    //获取最佳点坐标
                    if(PointNum>BestPointNum[ii])
                    {
                        BestPoints[ii]=Points[ii][i];
                        BestPointNum[ii]=PointNum;
                    }

                }

            }
            if(BestPointNum[ii]<Thresh_MBNSnum)  BestPoints[ii]=Point(0,0);//阈值：第二多邻居数量不少于Thresh_MBNSnum


        }

        //****从精确匹配得到的WidthNum*HeightNum个点中确定最终匹配点****
        Point FirstBestPoint=BestPoints[0];
        int FirstBestPointNum=0;
        int FirstBestPointSort=0;
        for(i=0;i<DiskNum;i++)
        {
            int PointNum=0;

            PointNum=CountNeighbors(i, BestPoints, DiskNum,DisksCenterPoints, Thresh_distance);//Thresh_distance=R
            //获取最佳点
            if(PointNum>FirstBestPointNum)
            {
                FirstBestPoint=BestPoints[i];
                FirstBestPointNum=PointNum;
                FirstBestPointSort=i;
            }


        }

        //剔除错误点，与最佳匹配点是邻居的点认为是正确点
        Point RectUplPoint=(FirstBestPoint-DisksCenterPoints[FirstBestPointSort]);//矩形左上角坐标
        int rectupnum=1;
        for(i=0;i<DiskNum;i++)
        {
            if(BestPoints[i].x==0 && BestPoints[i].y==0)
            {
                BestPoints[i]=Point(0,0);
            }
            else
            {
                if(FirstBestPointSort!=i)
                {
                    float distance=get_distance(FirstBestPoint,BestPoints[i]);
                    float distemp=get_distance(DisksCenterPoints[FirstBestPointSort],DisksCenterPoints[i]);
                    distance=fabs(distance-distemp);//距离
                    if(distance>Thresh_distance)
                    {
                        BestPoints[i]=Point(0,0);
                    }
                    else
                    {
                        RectUplPoint=RectUplPoint+(BestPoints[i]-DisksCenterPoints[i]);
                        rectupnum++;
                    }
                }
            }


            circle(ImageFinalResult,BestPoints[i],2,CV_RGB(255,255,255),2,8,0);
        }

        timeMBNS=((double)getTickCount() - timeMBNS)/getTickFrequency();//运行时间
        qDebug("MBNS耗时：%f秒",timeMBNS);
        Mat ImageFinalResult2;
        GrayImage.copyTo(ImageFinalResult2);
        circle(ImageFinalResult2,FirstBestPoint,2,CV_RGB(255,255,255),2,8,0);


        RectUplPoint.x=(int)RectUplPoint.x/rectupnum;
        RectUplPoint.y=(int)RectUplPoint.y/rectupnum;
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)0);


    /*
        //利用2个最佳匹配点画矩形框
        //计算两向量夹角
        float AvectorAngle=AnglePolar(DisksCenterPoints[SecondBestPointSort]-DisksCenterPoints[FirstBestPointSort]);//原始参考向量
        float BvectorAngle=AnglePolar(SecondBestPoint-FirstBestPoint);//实际向量
        //两向量夹角0-360°
        float Sita=BvectorAngle-AvectorAngle;
        if(Sita<0) Sita=Sita+360;


        Point RectUplPoint;//矩形左上角坐标
        RectUplPoint.x=(int)(-DisksCenterPoints[FirstBestPointSort].x*cos(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*sin(Sita/180*3.14) + FirstBestPoint.x);
        RectUplPoint.y=(int)(-(-DisksCenterPoints[FirstBestPointSort].x)*sin(Sita/180*3.14)+(-DisksCenterPoints[FirstBestPointSort].y)*cos(Sita/180*3.14)  + FirstBestPoint.y);
        DrawRotationRect(ImageFinalResult2,RectUplPoint,GrayTemplate.cols,GrayTemplate.rows,(double)Sita);
    */
    /*
        //显示结果
        qDebug("最终匹配的矩形左上角坐标=(%d,%d);旋转角=%f°",RectUplPoint.x,RectUplPoint.y,0);
        imshow("GrayImageDisp",GrayImageDisp);//局部特征粗匹配得到候选匹配点集
        imshow("ImageFinalResult",ImageFinalResult);//Most Neighbors Similarity: 获取9个最佳匹配点
        imshow("ImageFinalResult2",ImageFinalResult2);//Most Neighbors Similarity: 从9个匹配点中获取最佳的2个匹配点
    */
        //***********************************************************************
        //***********************************************************************
        timeSum=((double)getTickCount() - timeSum)/getTickFrequency();//运行时间
        qDebug("模板匹配结束，总耗时：%f秒",timeSum);
        qDebug("***********************");

    /*
        Mat srcImageDisp;
        //resize函数前加cv：：，因为Qt也有resize函数，故加cv：：加以区别，不然会出现错误
        int resizeW=(int)ImageFinalResult2.cols/(beta+1);
        int resizeH=(int)ImageFinalResult2.rows/(beta+1);
        cv::resize(ImageFinalResult2,srcImageDisp,Size(resizeW,resizeH),0,0,3);
        Imgview->imshow(srcImageDisp);//显示到绑定的控件上

    */

        //返回结果参数
        MatchUpLPoint[0]=RectUplPoint;
        MatchTime[0]=timeSum;
    }
    else
    {
        //返回结果参数
        MatchUpLPoint[0]=Point(0,0);
        MatchTime[0]=0;
    }

}

void SAD_tMatching(Mat Image,Mat TemplateImage, Point* MatchUpLPoint, float* MatchTime){

    int i,j;
    //一、图像和模板都转为单通道的灰度图
    Mat GrayImage;
    cvtColor(Image,GrayImage,CV_RGB2GRAY,1);
    Mat GrayTemplate;
    cvtColor(TemplateImage,GrayTemplate,CV_RGB2GRAY,1);


    //*****五、开始匹配************************************
    double timeSum;//总耗时
    timeSum=static_cast<double>(getTickCount());

    //遍历范围
    int LeftUpx=0;
    int LeftUpy=0;
    int RrightDownx=GrayImage.cols-GrayTemplate.cols-1;
    int RrightDowny=GrayImage.rows-GrayTemplate.rows-1;

    Mat MatchingValue(RrightDowny,RrightDownx,CV_32FC1,Scalar(0));//float型单通道，每个点初始化为0
    float* buffer;//MatchingValue的行指针
    uchar* buffer_AD_Img;

    //遍历待匹配图
    for(i=LeftUpy;i<RrightDowny;i++)
    {
        buffer=MatchingValue.ptr<float>(i);
        for(j=LeftUpx;j<RrightDownx;j++)
        {

            Mat Sub_MatchImg;//待匹配子图
            Sub_MatchImg = GrayImage(Rect(j,i,GrayTemplate.cols,GrayTemplate.rows));
            Mat AD_Img;//绝对差值图：两图相减再求绝对值
            absdiff(GrayTemplate,Sub_MatchImg,AD_Img);

            //遍历计算模板和子图的相似度
            float tempsum=0;
            for(int ii=0;ii<GrayTemplate.rows;ii++)
            {
                buffer_AD_Img=AD_Img.ptr<uchar>(ii);
                for(int jj=0;jj<GrayTemplate.cols;jj++)
                {
                    uchar x=buffer_AD_Img[jj];

                    //各类相似度公式
                    float SAD=x;//1、SAD:h(x)=x
                    //float SSD=x*x;//2、SSD:h(x)=x*x
                    tempsum=tempsum+SAD;

                }
            }

            buffer[j]=tempsum;
        }

    }

    Point MinLoc,MaxLoc,BestLoc;
    minMaxLoc(MatchingValue,NULL,NULL,&MinLoc,&MaxLoc);

    BestLoc=MinLoc;

    timeSum=((double)getTickCount() - timeSum)/getTickFrequency();//运行时间
    qDebug("模板匹配结束，总耗时：%f秒",timeSum);

    Point Centerpoint=Point(BestLoc.x+(int)GrayTemplate.cols/2,BestLoc.y+(int)GrayTemplate.rows/2);

    //返回参数结果
    MatchUpLPoint[0]=BestLoc;
    MatchTime[0]=timeSum;
}


void SSD_tMatching(Mat Image,Mat TemplateImage, Point* MatchUpLPoint, float* MatchTime){

    int i,j;
    //一、图像和模板都转为单通道的灰度图
    Mat GrayImage;
    cvtColor(Image,GrayImage,CV_RGB2GRAY,1);
    Mat GrayTemplate;
    cvtColor(TemplateImage,GrayTemplate,CV_RGB2GRAY,1);


    //*****五、开始匹配************************************
    double timeSum;//总耗时
    timeSum=static_cast<double>(getTickCount());

    //遍历范围
    int LeftUpx=0;
    int LeftUpy=0;
    int RrightDownx=GrayImage.cols-GrayTemplate.cols-1;
    int RrightDowny=GrayImage.rows-GrayTemplate.rows-1;

    Mat MatchingValue(RrightDowny,RrightDownx,CV_32FC1,Scalar(0));//float型单通道，每个点初始化为0
    float* buffer;//MatchingValue的行指针
    uchar* buffer_AD_Img;

    //遍历待匹配图
    for(i=LeftUpy;i<RrightDowny;i++)
    {
        buffer=MatchingValue.ptr<float>(i);
        for(j=LeftUpx;j<RrightDownx;j++)
        {

            Mat Sub_MatchImg;//待匹配子图
            Sub_MatchImg = GrayImage(Rect(j,i,GrayTemplate.cols,GrayTemplate.rows));
            Mat AD_Img;//绝对差值图：两图相减再求绝对值
            absdiff(GrayTemplate,Sub_MatchImg,AD_Img);

            //遍历计算模板和子图的相似度
            float tempsum=0;
            for(int ii=0;ii<GrayTemplate.rows;ii++)
            {
                buffer_AD_Img=AD_Img.ptr<uchar>(ii);
                for(int jj=0;jj<GrayTemplate.cols;jj++)
                {
                    uchar x=buffer_AD_Img[jj];

                    //各类相似度公式
                    //float SAD=x;//1、SAD:h(x)=x
                    float SSD=x*x;//2、SSD:h(x)=x*x
                    tempsum=tempsum+SSD;

                }
            }

            buffer[j]=tempsum;
        }

    }

    Point MinLoc,MaxLoc,BestLoc;
    minMaxLoc(MatchingValue,NULL,NULL,&MinLoc,&MaxLoc);

    BestLoc=MinLoc;

    timeSum=((double)getTickCount() - timeSum)/getTickFrequency();//运行时间
    qDebug("模板匹配结束，总耗时：%f秒",timeSum);

    Point Centerpoint=Point(BestLoc.x+(int)GrayTemplate.cols/2,BestLoc.y+(int)GrayTemplate.rows/2);


    //返回参数结果
    MatchUpLPoint[0]=BestLoc;
    MatchTime[0]=timeSum;

}

void NCC_tMatching(Mat Image,Mat TemplateImage, Point* MatchUpLPoint, float* MatchTime){

    int i,j;
    //一、图像和模板都转为单通道的灰度图
    Mat GrayImage;
    cvtColor(Image,GrayImage,CV_RGB2GRAY,1);
    Mat GrayTemplate;
    cvtColor(TemplateImage,GrayTemplate,CV_RGB2GRAY,1);


    Scalar Templatemean;
    Scalar Templatedev;
    cv::meanStdDev ( GrayTemplate, Templatemean, Templatedev );
    float Templatem = Templatemean.val[0];
    //float Templates = Templatedev.val[0];


    //*****五、开始匹配************************************
    double timeSum;//总耗时
    timeSum=static_cast<double>(getTickCount());

    //遍历范围
    int LeftUpx=0;
    int LeftUpy=0;
    int RrightDownx=GrayImage.cols-GrayTemplate.cols-1;
    int RrightDowny=GrayImage.rows-GrayTemplate.rows-1;

    Mat MatchingValue(RrightDowny,RrightDownx,CV_32FC1,Scalar(0));//float型单通道，每个点初始化为0
    float* buffer;//MatchingValue的行指针
    uchar* buffer_Img;
    uchar* buffer_template;

    //遍历待匹配图
    for(i=LeftUpy;i<RrightDowny;i++)
    {
        buffer=MatchingValue.ptr<float>(i);
        for(j=LeftUpx;j<RrightDownx;j++)
        {

            Mat Sub_MatchImg;//待匹配子图
            Sub_MatchImg = GrayImage(Rect(j,i,GrayTemplate.cols,GrayTemplate.rows));
            Scalar Matchmean;
            Scalar Matchdev;
            cv::meanStdDev ( Sub_MatchImg, Matchmean, Matchdev );
            float Matchm = Matchmean.val[0];
            //float Matchs = Matchdev.val[0];

            //遍历计算模板和子图的相似度
            float NCC1=0;
            float NCC2=0;
            float NCC3=0;
            for(int ii=0;ii<GrayTemplate.rows;ii++)
            {
                buffer_Img=Sub_MatchImg.ptr<uchar>(ii);
                buffer_template=GrayTemplate.ptr<uchar>(ii);

                for(int jj=0;jj<GrayTemplate.cols;jj++)
                {
                    //NCC
                    NCC1=NCC1+fabs(buffer_Img[jj]-Matchm)*fabs(buffer_template[jj]-Templatem);
                    NCC2=NCC2+(buffer_Img[jj]-Matchm)*(buffer_Img[jj]-Matchm);
                    NCC3=NCC3+(buffer_template[jj]-Templatem)*(buffer_template[jj]-Templatem);

                }
            }

            buffer[j]=NCC1/sqrt(NCC2)/sqrt(NCC3);
        }

    }

    Point MinLoc,MaxLoc,BestLoc;
    minMaxLoc(MatchingValue,NULL,NULL,&MinLoc,&MaxLoc);

    BestLoc=MaxLoc;

    timeSum=((double)getTickCount() - timeSum)/getTickFrequency();//运行时间
    qDebug("模板匹配结束，总耗时：%f秒",timeSum);

    Point Centerpoint=Point(BestLoc.x+(int)GrayTemplate.cols/2,BestLoc.y+(int)GrayTemplate.rows/2);


    //返回参数结果
    MatchUpLPoint[0]=BestLoc;
    MatchTime[0]=timeSum;

}

//实验测试相关函数

//获取图片编号：int转char
void intTOcharNum(int input, char* output)
{
    if(input/10>=1)
    {
        if(input/100>=1)
        {
            if(input/1000>=1)
            {
                sprintf(output,"%d",input);
            }
            else
            {
                sprintf(output,"0%d",input);
            }

        }
        else
        {
            sprintf(output,"00%d",input);
        }
    }
    else
    {
        sprintf(output,"000%d",input);
    }
}

//两个矩形的重叠度
float DecideOverlap(cv::Rect r1,cv::Rect r2)
{
    int x1 = r1.x;
    int y1 = r1.y;
    int width1 = r1.width;
    int height1 = r1.height;

    int x2 = r2.x;
    int y2 = r2.y;
    int width2 = r2.width;
    int height2 = r2.height;

    int endx = cv::max(x1+width1,x2+width2);
    int startx = cv::min(x1,x2);
    int width = width1+width2-(endx-startx);

    int endy = cv::max(y1+height1,y2+height2);
    int starty = cv::min(y1,y2);
    int height = height1+height2-(endy-starty);

    float ratio = 0.0f;
    float Area,Area1,Area2;

    if (width<=0||height<=0)
        return 0.0f;
    else
    {
        Area = width*height;
        Area1 = width1*height1;
        Area2 = width2*height2;
        ratio = Area /(Area1+Area2-Area);
    }

    return ratio;
}

//统计匹配率，ratioThresh为匹配率阈值，通过对应阈值则对应mSuccessNum[]累加
void SuccessNum(int* mSuccessNum, float ratioThresh)
{
    int Thresh=(int)(100*ratioThresh);
    for(int i=1;i<=100;i++)
    {
        if(i<=Thresh)
        {
            mSuccessNum[i-1]++;
        }
        else
        {
            break;
        }
    }
}


#include <fstream>
#include <string>
#include <iostream>
#include <streambuf>
void MainWindow::on_pushButtonMatch_clicked()
{
/*
    //图片序号重排
    int start=1;
    int end=698;
    int n=5;
    for(int i=start;i<=end;i++)
    {
        char strNumimg[200];//图的序号
        intTOcharNum(i,strNumimg);
        char path_img[200];
        sprintf(path_img,"F:/MyAlgorithmProject/20170601_TemplateMatching_FeatureBase_RPT_Zernike/QtCode/BBS(20180507)/dataset/TB-50_2/%d/img/0%s.jpg",n,strNumimg);
        Mat img=imread(path_img);
        Mat img2;
        img.copyTo(img2);
        char strNumimg2[200];//图的新序号
        intTOcharNum(i-start+1,strNumimg2);
        char path_img2[200];
        sprintf(path_img2,"F:/MyAlgorithmProject/20170601_TemplateMatching_FeatureBase_RPT_Zernike/QtCode/BBS(20180507)/dataset/TB-50_2/%d/img2/%s.jpg",n,strNumimg2);
        imwrite(path_img2,img2);
    }
*/
/*
    //开始匹配（单次匹配）
    //模板图必须大于20*20
    //开始匹配
    Ldisk=(int)sqrt(ROI.cols*ROI.rows/30);
    if(Ldisk<9) Ldisk=9;
    R=(int)Ldisk/2;//局部方块半径
    Point MatchUpLPoint[1],MatchCenterPoint[1];
    float MatchTime[1],MatchSita[1];
    //MNShsv_SSDtMatching(Img,TemplateROI,MatchUpLPoint,MatchTime);//特征：环投影均值、灰度均值、HSV分量均值
    //MBNShsv_SSDtMatching(Img,TemplateROI,MatchUpLPoint,MatchTime);//特征：环投影均值、灰度均值、HSV分量均值
    //MNShsv_SSDtMatching2(Img,TemplateROI,MatchUpLPoint,MatchTime);//特征：环投影均值、灰度均值和方差、HSV分量均值
    //MBNShsv_SSDtMatching2_Angle(Image,ROI,MatchUpLPoint,MatchTime);//特征：环投影均值、灰度均值和方差、HSV分量均值
    MBNShsv_SSDtMatching2_Fast_Angle(Image,ROI,MatchUpLPoint,MatchCenterPoint,MatchSita,MatchTime);//特征：环投影均值、灰度均值和方差、HSV分量均值
*/


    /*
    //分离颜色通道HSV
    Mat Image_h,Imagehsv;
    vector<Mat> hsv_vec;
    cvtColor(Image, Imagehsv, CV_BGR2HSV);
    split(Imagehsv, hsv_vec);
    Image_h = hsv_vec[0];
    Image_h.convertTo(Image_h, CV_8UC1);
    imshow("Image",Image);
    imshow("Image_h",Image_h);
*/

    int ClassNum=40;// 图像类数量（即文件夹数量）
    int ImgNum=35;//每类的图像数量（即每个文件夹里面的图片数量）

    //实验返回结果参数
    //结果：时间和匹配率
    float MNS_total_SumTime=0;
    int MNS_SuccessNum=0;
    float MNS_Sum_Sita=0;//角度误差和

    int perNum=0;

    for(int i=1;i<=ClassNum;i++)
    {
        //获取模板图路径
        char Path_template[200];
        sprintf(Path_template,"F:/MyAlgorithmProject/20170601_TemplateMatching_FeatureBase_RPT_Zernike/QtCode/QtCode_MNS_MostNeighborsSimilarity(20180223)/TextImage/0Rotation/%d/00template.bmp",i);
        cv::Mat TemplateImage=cv::imread(Path_template);
        //获取原图路径
        char Path_origin[200];
        sprintf(Path_origin,"F:/MyAlgorithmProject/20170601_TemplateMatching_FeatureBase_RPT_Zernike/QtCode/QtCode_MNS_MostNeighborsSimilarity(20180223)/TextImage/0Rotation/%d/0.jpg",i);
        cv::Mat originImage=cv::imread(Path_origin);//原图
        //第一次匹配，获取标准匹配中心坐标
        Ldisk=(int)sqrt(TemplateImage.cols*TemplateImage.rows/30);
        if(Ldisk<9) Ldisk=9;
        R=(int)Ldisk/2;//局部方块半径
        Point TrueMatchUpLPoint[1];
        float TrueMatchTime[1];
        MBNShsv_SSDtMatching2_Fast(originImage,TemplateImage,TrueMatchUpLPoint,TrueMatchTime);//特征：环投影均值、灰度均值和方差、HSV分量均值
        Point TrueMatchCenterPoint=TrueMatchUpLPoint[0]+Point((int)(TemplateImage.cols/2),(int)(TemplateImage.rows/2));

        //正式匹配实验测试开始
        for(int j=10;j<=350;j=j+10)
        {
            //获取待匹配图路径
            char Path_img[200];
            sprintf(Path_img,"F:/MyAlgorithmProject/20170601_TemplateMatching_FeatureBase_RPT_Zernike/QtCode/QtCode_MNS_MostNeighborsSimilarity(20180223)/TextImage/0Rotation/%d/%d.bmp",i,j);
            cv::Mat Img=cv::imread(Path_img);//待匹配图
            //开始匹配
            Ldisk=(int)sqrt(TemplateImage.cols*TemplateImage.rows/30);
            if(Ldisk<9) Ldisk=9;
            R=(int)Ldisk/2;//局部方块半径
            Point MatchUpLPoint[1],MatchCenterPoint[1];
            float MatchTime[1],MatchSita[1];
            MNShsv_SSDtMatching2_Fast_Angle(Img,TemplateImage,MatchUpLPoint,MatchCenterPoint,MatchSita,MatchTime);//特征：环投影均值、灰度均值和方差、HSV分量均值


            //统计匹配时间和匹配率    
            MNS_total_SumTime=MNS_total_SumTime+MatchTime[0];
            float tempSita=fabs(MatchSita[0]-j);
            if(tempSita>180)    tempSita=360-tempSita;
            //角度误差
            MNS_Sum_Sita=MNS_Sum_Sita+tempSita;
            //匹配率
            float distance1=get_distance(MatchCenterPoint[0],TrueMatchCenterPoint);
            float distance2=(sqrt(TemplateImage.cols*TemplateImage.rows))/2;
            if(distance1<=distance2)
                MNS_SuccessNum++;

            perNum++;
            qDebug("MNS_SuccessNum=%d",MNS_SuccessNum);
            qDebug("perNum=%d",perNum);
            qDebug("MatchRate=%f",(float)MNS_SuccessNum/perNum);
        }

    }

    float MNS_aver_SumTime=MNS_total_SumTime/(ClassNum*ImgNum);
    float MNS_aver_Sita=MNS_Sum_Sita/(ClassNum*ImgNum);
    float MNS_SuccessRate=(float)MNS_SuccessNum/(ClassNum*ImgNum);
    float MNS_averSecond_SumTime=MNS_second_SumTime/(ClassNum*ImgNum);

    //输出到exel
    //定义文件输出流
    ofstream oFile;
    //打开要输出的文件
    oFile.open("Results.csv", ios::out | ios::trunc);    // 这样就很容易的输出一个需要的excel 文件
    oFile << "Method" << "," << "Total Images" << "," << "Matching Rate" << "," << "AngleERROR"<< "," << "Total Aver Time" << "," << "First Aver Time"<< "," << "Second Aver Time"<< endl;
    oFile << "MNS" << "," << ClassNum*ImgNum << "," << MNS_SuccessRate << "," << MNS_aver_Sita<< "," << MNS_aver_SumTime<< "," << MNS_aver_SumTime-MNS_averSecond_SumTime << "," << MNS_averSecond_SumTime << endl;
    oFile.close();

    qDebug("OK!");

}

void MainWindow::on_pushButtonROI_3_clicked()
{
    QString str0=ui->lineEdit_3->text();
    PSMode=str0.toFloat();
    qDebug("%f",PSMode);
    Thresh_distance=(float)(PSMode*R);//邻居距离阈值

    QString str=ui->lineEdit->text();
    Ldisk=str.toInt();
    qDebug("%d",Ldisk);
    R=(int)Ldisk/2;//局部方块半径

    QString str2=ui->lineEdit_2->text();
    Rthre=str2.toFloat();
    qDebug("%f",Rthre);

    QString str6=ui->lineEdit_6->text();
    Thresh_MBNSnum=str6.toInt();
    qDebug("%d",Thresh_MBNSnum);

    QString str4=ui->lineEdit_4->text();
    MatchingNum2=str4.toFloat();
    qDebug("%d",MatchingNum2);

    QString str5=ui->lineEdit_5->text();
    N=str5.toFloat();
    qDebug("%d",N);

    QString str8=ui->lineEdit_8->text();
    popular=str8.toInt();
    qDebug("%d",popular);

    QString str7=ui->lineEdit_7->text();
    Maxgen=str7.toInt();
    qDebug("%d",Maxgen);

}


