#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_load_clicked()
{
    glob(path, pathstr, false);
    std::vector< Point > center_list;
    Mat curtain_sample = imread(pathstr[0], COLOR_BGR2GRAY);    // 绘制效果的幕布
    Mat curtain = Mat(curtain_sample.rows, curtain_sample.cols, CV_8UC3, Scalar(255, 255, 255));

    for (uint index = 1; index <= pathstr.size(); index++)  // 逐张图片读取
    {
        std::string Index_str = "";
        Mat img = imread(pathstr[index - 1], COLOR_BGR2GRAY); // 读取单帧光斑轨迹图像
        for (uint j = 0; j < ((pathstr[index - 1]).length()); j++)
        {
            if ((pathstr[index - 1])[j] >= '0' && (pathstr[index - 1])[j] <= '9')
                Index_str.append((pathstr[index - 1]), j, 1);   // 获取图片索引编号
        }
        if (Index_str == "")
        {
            qDebug() << "文件命名格式不正确";
            return;
        }
        Point spot_center = DetectSpot(img, Index_str); // 单帧检测光斑中心

        cv::circle(curtain, spot_center, 10, Scalar(0, 0, 255), -1, LINE_8);    // 绘制光斑中心
        center_list.push_back(spot_center);
        qDebug() << "第" << QString::fromStdString(Index_str).toInt() << "图"
                 << "(" << spot_center.x << "," << spot_center.y << ")";
    }

    // 光斑中心轨迹拟合
    Circle circle = circle_Fitting(center_list);
    cv::circle(curtain, Point(circle.x, circle.y), circle.r, Scalar(255, 0, 0), 5, LINE_8);
    cv::circle(curtain, Point(circle.x, circle.y), 10, Scalar(0, 0, 255), -1);

    // Qlabel载入图片文件并显示
    cv::resize(curtain, curtain, Size(0, 0), 0.25, 0.25, INTER_LINEAR);
    QImage image_show = QImage((uchar *)(curtain.data), curtain.rows, curtain.cols, QImage::Format_BGR888);
    ui->label_img->setPixmap(QPixmap::fromImage(image_show));
    ui->label_img->resize(image_show.size());
    ui->label_img->show();

    //    namedWindow("拟合圆", WINDOW_NORMAL);
    //    imshow("拟合圆", curtain);
    qDebug() << circle.x << "," << circle.y << "半径" << circle.r;
}

Point MainWindow::DetectSpot(Mat inputImage, std::string index)
{
    Mat inputImage_copy, resultimg;
    inputImage.copyTo(inputImage_copy);
    inputImage.copyTo(resultimg);
    cvtColor(inputImage_copy, inputImage_copy, COLOR_BGR2GRAY);
    GaussianBlur(inputImage_copy, inputImage_copy, Size(3, 3), 0, 0);
    threshold(inputImage_copy, inputImage_copy, 230, 255, THRESH_BINARY_INV); // 阈值化，THRESH_BINARY_INV（视情况反转黑白）

    // 显示效果
    namedWindow("THRESH_BINARY", WINDOW_NORMAL); //命名窗口
    imshow("THRESH_BINARY", inputImage_copy);

    // 腐蚀操作
    int erosion_size = 1;
    Mat erosionElement = getStructuringElement(MORPH_RECT, Size(2 * erosion_size + 1, 2 * erosion_size + 1));
    erode(inputImage_copy, inputImage_copy, erosionElement);

    // 显示效果
    namedWindow("Erode", WINDOW_NORMAL);
    imshow("Erode", inputImage_copy);

    // 膨胀操作
    int dilation_size = 3;
    Mat dilationElement = getStructuringElement(MORPH_RECT, Size(dilation_size * 2 + 1, dilation_size * 2 + 1));
    dilate(inputImage_copy, inputImage_copy, dilationElement);

    // 显示效果
    namedWindow("Dilate", WINDOW_NORMAL);
    imshow("Dilate", inputImage_copy);

    std::vector< std::vector< Point > > contours;
    std::vector< Vec4i > hierarcy;
    findContours(inputImage_copy, contours, hierarcy, RETR_TREE, CHAIN_APPROX_NONE); // 检索轮廓
    std::vector< RotatedRect > box(contours.size());
    int max_index = 0;
    float max_area = 0;
    for (uint i = 0; i < contours.size(); i++) // 遍历轮廓
    {
        if (contours[i].size() >= 5)
        {
            box[i] = fitEllipse(contours[i]); // 最小外接矩形
            Size2f size = box[i].size;
            if (size.height > 5 && size.width > 5 && size.height * size.width > max_area)
            {
                max_area = size.height * size.width; // 记录最大面积及索引
                max_index = i;
            }
        }
    }
    if (max_area != 0) // 最大面积不为零
    {
        Point2d center = box[max_index].center;

        // 绘制中心十字线和椭圆
        line(resultimg, Point2f(center.x, center.y - 6), Point2f(center.x, center.y + 6), Scalar(0, 0, 255), 2, 8);
        line(resultimg, Point2f(center.x - 6, center.y), Point2f(center.x + 6, center.y), Scalar(0, 0, 255), 2, 8);
        ellipse(resultimg, box[max_index], Scalar(255, 0, 0), 1, 8);
        imwrite(path_tosave + "/" + index + ".png", resultimg);
        return center;
    }
    else
    {
        return Point(0, 0);
    }
}

Circle MainWindow::circle_Fitting(std::vector< Point > sample_list)
{
    Circle circle;
    circle.x = 0.0f;
    circle.y = 0.0f;
    circle.r = 0.0f;

    if (sample_list.size() < 3)
    {
        qDebug() << "采样点较少";
        return circle;
    }

    double sum_Xi = 0.0f, sum_Yi = 0.0f;
    double sum_Xi2 = 0.0f, sum_Yi2 = 0.0f;
    double sum_Xi3 = 0.0f, sum_Yi3 = 0.0f;
    double sum_XiYi = 0.0f, sum_XiYi2 = 0.0f, sum_Xi2Yi = 0.0f;

    int N = sample_list.size();
    for (int i = 0; i < N; i++)
    {
        double x = sample_list[i].x;
        double y = sample_list[i].y;
        double x2 = x * x;
        double y2 = y * y;
        sum_Xi += x;
        sum_Yi += y;
        sum_Xi2 += x2;
        sum_Yi2 += y2;
        sum_Xi3 += x2 * x;
        sum_Yi3 += y2 * y;
        sum_XiYi += x * y;
        sum_XiYi2 += x * y2;
        sum_Xi2Yi += x2 * y;
    }

    double C, D, E, G, H;
    double a, b, c;

    C = N * sum_Xi2 - sum_Xi * sum_Xi;
    D = N * sum_XiYi - sum_Xi * sum_Yi;
    E = N * sum_Xi3 + N * sum_XiYi2 - (sum_Xi2 + sum_Yi2) * sum_Xi;
    G = N * sum_Yi2 - sum_Yi * sum_Yi;
    H = N * sum_Xi2Yi + N * sum_Yi3 - (sum_Xi2 + sum_Yi2) * sum_Yi;
    a = (H * D - E * G) / (C * G - D * D);
    b = (H * C - E * D) / (D * D - G * C);
    c = -(a * sum_Xi + b * sum_Yi + sum_Xi2 + sum_Yi2) / N;
    circle.x = a / (-2);
    circle.y = b / (-2);
    circle.r = sqrt(a * a + b * b - 4 * c) / 2;

    return circle;
}
