#ifndef EEGCHARTVIEW_H
#define EEGCHARTVIEW_H

#include <QWidget>
#include <QChartView>
#include <QtCharts>
#include "../DataModels/EEGData.h"

QT_CHARTS_USE_NAMESPACE

class EEGChartView : public QChartView {
    Q_OBJECT
    
public:
    explicit EEGChartView(QWidget *parent = nullptr);
    ~EEGChartView();
    
    void setEEGData(EEGData *data);
    void updateChart();
    void setVisibleChannels(const QVector<int> &channels);

    void selectAllChannels();
    void selectFirstNChannels(int n);
    void clearVisibleChannels();
    QVector<int> getVisibleChannels() const { return m_visibleChannels; }
    bool isChannelVisible(int channelIndex) const;

    void setTimeRange(double startTime, double duration);
    void setVerticalScale(double scale);
    void setOffsetScale(double offset);
    void setShowGrid(bool show);
    void setSelectedChannel(int channel);
    void clearSelectedChannel();
    
    double currentStartTime() const { return m_startTime; }
    double currentDuration() const { return m_duration; }
    
signals:
    void timeRangeChanged(double start, double duration);
    void channelSelected(int channelIndex);
    void visibleChannelsChanged(const QVector<int> &channels);
    
protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    
private:
    void createChart();
    void updateSeries();
    void zoomChart(double factor, const QPointF &centerPoint);
    void panChart(double dx, double dy);
    QColor getChannelColor(int index, bool isSelected) const;
    void ensureVisibleChannels();
    
private:
    EEGData *m_eegData;
    QChart *m_chart;
    QVector<QLineSeries*> m_series;
    QVector<int> m_visibleChannels;
    
    double m_startTime;
    double m_duration;
    double m_verticalScale;
    double m_offsetScale;
    bool m_showGrid;
    int m_selectedChannel;
    
    QPoint m_lastMousePos;
    bool m_isPanning;
    bool m_isZooming;
};
#endif // EEGCHARTVIEW_H