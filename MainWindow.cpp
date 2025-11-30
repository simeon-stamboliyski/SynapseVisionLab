#include "MainWindow.h"
#include <cmath>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("SynapseVisionLab");
    resize(1920, 1080);

    QLineSeries *series = new QLineSeries();
    for (int i = 0; i < 500; i++) {
        series->append(i, 50 * sin(i * 0.1));
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("EEG Signal (Dummy)");
    chart->createDefaultAxes();

    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText("Time");
    axisX->setRange(0, 500);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("Amplitude");
    axisY->setRange(-60, 60);

    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    setCentralWidget(chartView);
}