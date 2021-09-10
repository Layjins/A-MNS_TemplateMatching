/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QGroupBox *groupBox;
    QGraphicsView *graphicsViewROI;
    QLabel *label_2;
    QGraphicsView *graphicsViewTemp;
    QPushButton *pushButtonTemp;
    QPushButton *pushButtonROI;
    QLabel *label_7;
    QPushButton *pushButtonROI_4;
    QGroupBox *groupBox_2;
    QGraphicsView *graphicsViewImg;
    QLabel *label_3;
    QPushButton *pushButtonROI_2;
    QPushButton *pushButtonMatch;
    QLabel *label_4;
    QLabel *label_5;
    QLineEdit *lineEdit;
    QLabel *label_6;
    QLineEdit *lineEdit_2;
    QPushButton *pushButtonROI_3;
    QLabel *label_8;
    QLineEdit *lineEdit_3;
    QLineEdit *lineEdit_4;
    QLabel *label_9;
    QLabel *label_10;
    QLineEdit *lineEdit_5;
    QLabel *label_11;
    QLineEdit *lineEdit_6;
    QLineEdit *lineEdit_7;
    QLabel *label_12;
    QLabel *label_13;
    QLineEdit *lineEdit_8;
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->resize(868, 689);
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        groupBox = new QGroupBox(centralWidget);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        groupBox->setGeometry(QRect(10, 0, 831, 301));
        QFont font;
        font.setFamily(QString::fromUtf8("\351\273\221\344\275\223"));
        font.setPointSize(11);
        font.setBold(true);
        font.setWeight(75);
        groupBox->setFont(font);
        groupBox->setFlat(false);
        graphicsViewROI = new QGraphicsView(groupBox);
        graphicsViewROI->setObjectName(QStringLiteral("graphicsViewROI"));
        graphicsViewROI->setGeometry(QRect(530, 10, 280, 280));
        label_2 = new QLabel(groupBox);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setGeometry(QRect(480, 10, 51, 21));
        QFont font1;
        font1.setFamily(QStringLiteral("Arial"));
        font1.setPointSize(9);
        font1.setBold(false);
        font1.setWeight(50);
        label_2->setFont(font1);
        label_2->setFrameShape(QFrame::Box);
        graphicsViewTemp = new QGraphicsView(groupBox);
        graphicsViewTemp->setObjectName(QStringLiteral("graphicsViewTemp"));
        graphicsViewTemp->setGeometry(QRect(160, 10, 280, 280));
        pushButtonTemp = new QPushButton(groupBox);
        pushButtonTemp->setObjectName(QStringLiteral("pushButtonTemp"));
        pushButtonTemp->setGeometry(QRect(20, 110, 75, 23));
        pushButtonTemp->setFont(font1);
        pushButtonROI = new QPushButton(groupBox);
        pushButtonROI->setObjectName(QStringLiteral("pushButtonROI"));
        pushButtonROI->setGeometry(QRect(20, 160, 75, 23));
        pushButtonROI->setFont(font1);
        label_7 = new QLabel(groupBox);
        label_7->setObjectName(QStringLiteral("label_7"));
        label_7->setGeometry(QRect(110, 10, 51, 21));
        label_7->setFont(font1);
        label_7->setFrameShape(QFrame::Box);
        pushButtonROI_4 = new QPushButton(groupBox);
        pushButtonROI_4->setObjectName(QStringLiteral("pushButtonROI_4"));
        pushButtonROI_4->setGeometry(QRect(20, 210, 75, 23));
        pushButtonROI_4->setFont(font1);
        groupBox_2 = new QGroupBox(centralWidget);
        groupBox_2->setObjectName(QStringLiteral("groupBox_2"));
        groupBox_2->setGeometry(QRect(10, 310, 831, 321));
        groupBox_2->setFont(font);
        groupBox_2->setFlat(false);
        graphicsViewImg = new QGraphicsView(groupBox_2);
        graphicsViewImg->setObjectName(QStringLiteral("graphicsViewImg"));
        graphicsViewImg->setGeometry(QRect(530, 10, 280, 280));
        label_3 = new QLabel(groupBox_2);
        label_3->setObjectName(QStringLiteral("label_3"));
        label_3->setGeometry(QRect(470, 10, 61, 21));
        label_3->setFont(font1);
        label_3->setFrameShape(QFrame::Box);
        pushButtonROI_2 = new QPushButton(groupBox_2);
        pushButtonROI_2->setObjectName(QStringLiteral("pushButtonROI_2"));
        pushButtonROI_2->setGeometry(QRect(10, 30, 101, 23));
        pushButtonROI_2->setFont(font1);
        pushButtonMatch = new QPushButton(groupBox_2);
        pushButtonMatch->setObjectName(QStringLiteral("pushButtonMatch"));
        pushButtonMatch->setGeometry(QRect(390, 120, 101, 23));
        pushButtonMatch->setFont(font1);
        label_4 = new QLabel(groupBox_2);
        label_4->setObjectName(QStringLiteral("label_4"));
        label_4->setGeometry(QRect(10, 100, 91, 21));
        label_4->setFont(font1);
        label_4->setFrameShape(QFrame::Box);
        label_5 = new QLabel(groupBox_2);
        label_5->setObjectName(QStringLiteral("label_5"));
        label_5->setGeometry(QRect(390, 150, 61, 21));
        label_5->setFont(font1);
        label_5->setFrameShape(QFrame::Box);
        lineEdit = new QLineEdit(groupBox_2);
        lineEdit->setObjectName(QStringLiteral("lineEdit"));
        lineEdit->setGeometry(QRect(110, 100, 61, 20));
        label_6 = new QLabel(groupBox_2);
        label_6->setObjectName(QStringLiteral("label_6"));
        label_6->setGeometry(QRect(10, 130, 91, 21));
        label_6->setFont(font1);
        label_6->setFrameShape(QFrame::Box);
        lineEdit_2 = new QLineEdit(groupBox_2);
        lineEdit_2->setObjectName(QStringLiteral("lineEdit_2"));
        lineEdit_2->setGeometry(QRect(110, 130, 61, 20));
        pushButtonROI_3 = new QPushButton(groupBox_2);
        pushButtonROI_3->setObjectName(QStringLiteral("pushButtonROI_3"));
        pushButtonROI_3->setGeometry(QRect(200, 200, 101, 23));
        pushButtonROI_3->setFont(font1);
        label_8 = new QLabel(groupBox_2);
        label_8->setObjectName(QStringLiteral("label_8"));
        label_8->setGeometry(QRect(10, 70, 91, 21));
        label_8->setFont(font1);
        label_8->setFrameShape(QFrame::Box);
        lineEdit_3 = new QLineEdit(groupBox_2);
        lineEdit_3->setObjectName(QStringLiteral("lineEdit_3"));
        lineEdit_3->setGeometry(QRect(110, 70, 61, 20));
        lineEdit_4 = new QLineEdit(groupBox_2);
        lineEdit_4->setObjectName(QStringLiteral("lineEdit_4"));
        lineEdit_4->setGeometry(QRect(300, 70, 61, 20));
        label_9 = new QLabel(groupBox_2);
        label_9->setObjectName(QStringLiteral("label_9"));
        label_9->setGeometry(QRect(200, 70, 91, 21));
        label_9->setFont(font1);
        label_9->setFrameShape(QFrame::Box);
        label_10 = new QLabel(groupBox_2);
        label_10->setObjectName(QStringLiteral("label_10"));
        label_10->setGeometry(QRect(200, 100, 91, 21));
        label_10->setFont(font1);
        label_10->setFrameShape(QFrame::Box);
        lineEdit_5 = new QLineEdit(groupBox_2);
        lineEdit_5->setObjectName(QStringLiteral("lineEdit_5"));
        lineEdit_5->setGeometry(QRect(300, 100, 61, 20));
        label_11 = new QLabel(groupBox_2);
        label_11->setObjectName(QStringLiteral("label_11"));
        label_11->setGeometry(QRect(10, 160, 91, 21));
        label_11->setFont(font1);
        label_11->setFrameShape(QFrame::Box);
        lineEdit_6 = new QLineEdit(groupBox_2);
        lineEdit_6->setObjectName(QStringLiteral("lineEdit_6"));
        lineEdit_6->setGeometry(QRect(110, 160, 61, 20));
        lineEdit_7 = new QLineEdit(groupBox_2);
        lineEdit_7->setObjectName(QStringLiteral("lineEdit_7"));
        lineEdit_7->setGeometry(QRect(300, 160, 61, 20));
        label_12 = new QLabel(groupBox_2);
        label_12->setObjectName(QStringLiteral("label_12"));
        label_12->setGeometry(QRect(200, 160, 91, 21));
        label_12->setFont(font1);
        label_12->setFrameShape(QFrame::Box);
        label_13 = new QLabel(groupBox_2);
        label_13->setObjectName(QStringLiteral("label_13"));
        label_13->setGeometry(QRect(200, 130, 91, 21));
        label_13->setFont(font1);
        label_13->setFrameShape(QFrame::Box);
        lineEdit_8 = new QLineEdit(groupBox_2);
        lineEdit_8->setObjectName(QStringLiteral("lineEdit_8"));
        lineEdit_8->setGeometry(QRect(300, 130, 61, 20));
        MainWindow->setCentralWidget(centralWidget);
        groupBox_2->raise();
        groupBox->raise();
        menuBar = new QMenuBar(MainWindow);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 868, 23));
        MainWindow->setMenuBar(menuBar);
        mainToolBar = new QToolBar(MainWindow);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        MainWindow->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        MainWindow->setStatusBar(statusBar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", 0));
        groupBox->setTitle(QApplication::translate("MainWindow", "\351\200\211\345\217\226\346\250\241\346\235\277ROI", 0));
        label_2->setText(QApplication::translate("MainWindow", "ROI", 0));
        pushButtonTemp->setText(QApplication::translate("MainWindow", "\351\200\211\346\213\251\346\250\241\346\235\277\345\233\276", 0));
        pushButtonROI->setText(QApplication::translate("MainWindow", "\346\210\252\345\217\226ROI", 0));
        label_7->setText(QApplication::translate("MainWindow", "\346\250\241\346\235\277\345\233\276", 0));
        pushButtonROI_4->setText(QApplication::translate("MainWindow", "\347\233\264\346\216\245\350\257\273\345\217\226ROI", 0));
        groupBox_2->setTitle(QApplication::translate("MainWindow", "\346\250\241\346\235\277\345\214\271\351\205\215", 0));
        label_3->setText(QApplication::translate("MainWindow", "\345\276\205\345\214\271\351\205\215\345\233\276", 0));
        pushButtonROI_2->setText(QApplication::translate("MainWindow", "\351\200\211\346\213\251\345\276\205\345\214\271\351\205\215\345\233\276", 0));
        pushButtonMatch->setText(QApplication::translate("MainWindow", "\345\274\200\345\247\213\346\250\241\346\235\277\345\214\271\351\205\215", 0));
        label_4->setText(QApplication::translate("MainWindow", "\345\261\200\351\203\250\346\226\271\345\235\227\350\276\271\351\225\277", 0));
        label_5->setText(QApplication::translate("MainWindow", "\350\276\223\345\207\272\345\217\202\346\225\260", 0));
        label_6->setText(QApplication::translate("MainWindow", "Zernike\351\230\210\345\200\274", 0));
        pushButtonROI_3->setText(QApplication::translate("MainWindow", "\344\277\256\346\224\271\350\276\223\345\205\245\345\217\202\346\225\260", 0));
        label_8->setText(QApplication::translate("MainWindow", "\351\202\273\345\261\205\350\267\235\347\246\273\345\200\215\346\225\260", 0));
        label_9->setText(QApplication::translate("MainWindow", "\345\200\231\351\200\211\347\202\271\346\225\260\351\207\217", 0));
        label_10->setText(QApplication::translate("MainWindow", "Zernike\351\230\266\346\225\260", 0));
        label_11->setText(QApplication::translate("MainWindow", "MBNS\351\230\210\345\200\274", 0));
        label_12->setText(QApplication::translate("MainWindow", "\350\277\255\344\273\243\346\254\241\346\225\260", 0));
        label_13->setText(QApplication::translate("MainWindow", "\347\247\215\347\276\244\346\225\260\351\207\217", 0));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
