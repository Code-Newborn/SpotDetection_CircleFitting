#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "opencv2/opencv.hpp"
#include "opencv2/imgproc.hpp"
#include <QDebug>
#include <QMainWindow>
#include <iostream>

using namespace cv;

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

struct Circle
{
    double x;
    double y;
    double r;
    Circle() {}
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_load_clicked();

private:
    Ui::MainWindow *ui;

    Point DetectSpot(Mat inputImage, std::string index);
    std::vector< cv::String > pathstr;
    std::string path = "../GreenPositions";
    std::string path_tosave = "../GreenPositions_result";
    Circle circle_Fitting(std::vector<Point> sample_list);
};
#endif // MAINWINDOW_H
