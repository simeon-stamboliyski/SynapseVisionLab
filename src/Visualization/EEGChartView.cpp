#include "EEGChartView.h"
#include <QValueAxis>
#include <QDateTimeAxis>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDebug>
#include <cmath>

EEGChartView::EEGChartView(QWidget *parent) 
    : QChartView(parent), 
      m_eegData(nullptr),
      m_chart(new QChart()),
      m_startTime(0.0),
      m_duration(10.0),
      m_verticalScale(1.0),
      m_offsetScale(100.0),
      m_showGrid(true),
      m_isPanning(false),
      m_isZooming(false),
      m_selectedChannel(-1) {
    
    setChart(m_chart);
    setRenderHint(QPainter::Antialiasing);
    setRubberBand(QChartView::RectangleRubberBand);
    
    createChart();
    
    // Default visible channels
    for (int i = 0; i < 8; ++i) {
        m_visibleChannels.append(i);
    }
}

EEGChartView::~EEGChartView() {
    qDeleteAll(m_series);
    m_series.clear();
}

void EEGChartView::setEEGData(EEGData *data) {
    m_eegData = data;
    if (data) {
        connect(data, &EEGData::dataChanged, this, &EEGChartView::updateChart);
    }
    updateChart();
}

void EEGChartView::createChart() {
    m_chart->setTitle("EEG Signals");
    m_chart->setAnimationOptions(QChart::NoAnimation);
    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignBottom);
    
    QValueAxis *axisX = new QValueAxis;
    axisX->setTitleText("Time (s)");
    axisX->setLabelFormat("%.2f");
    axisX->setGridLineVisible(m_showGrid);
    axisX->setMinorGridLineVisible(false);
    
    QValueAxis *axisY = new QValueAxis;
    axisY->setTitleText("Amplitude (μV)");
    axisY->setLabelFormat("%.0f");
    axisY->setGridLineVisible(m_showGrid);
    axisY->setMinorGridLineVisible(false);
    
    m_chart->addAxis(axisX, Qt::AlignBottom);
    m_chart->addAxis(axisY, Qt::AlignLeft);
    
    setChart(m_chart);
}

void EEGChartView::selectAllChannels() {
    m_visibleChannels.clear();
    if (m_eegData) {
        for (int i = 0; i < m_eegData->channelCount(); ++i) {
            m_visibleChannels.append(i);
        }
    }
    updateChart();
    emit visibleChannelsChanged(m_visibleChannels);
}

void EEGChartView::selectFirstNChannels(int n) {
    m_visibleChannels.clear();
    if (m_eegData) {
        int maxChannels = qMin(n, m_eegData->channelCount());
        for (int i = 0; i < maxChannels; ++i) {
            m_visibleChannels.append(i);
        }
    }
    updateChart();
    emit visibleChannelsChanged(m_visibleChannels);
}

void EEGChartView::clearVisibleChannels() {
    m_visibleChannels.clear();
    updateChart();
    emit visibleChannelsChanged(m_visibleChannels);
}

bool EEGChartView::isChannelVisible(int channelIndex) const {
    return m_visibleChannels.contains(channelIndex);
}

void EEGChartView::ensureVisibleChannels() {
    // If no channels are visible, select first 8 by default
    if (m_visibleChannels.isEmpty() && m_eegData && !m_eegData->isEmpty()) {
        int defaultChannels = qMin(8, m_eegData->channelCount());
        for (int i = 0; i < defaultChannels; ++i) {
            m_visibleChannels.append(i);
        }
        qDebug() << "No channels selected, defaulting to first" << defaultChannels;
    }
}

void EEGChartView::updateChart() {
    qDebug() << "=== updateChart called ===";

    ensureVisibleChannels();
    qDebug() << "m_eegData:" << m_eegData;
    qDebug() << "m_visibleChannels count:" << m_visibleChannels.size();
    
    if (!m_eegData || m_eegData->isEmpty()) {
        qDebug() << "No data or data is empty, clearing chart";
        // Clear all series
        for (auto series : m_series) {
            m_chart->removeSeries(series);
            delete series;
        }
        m_series.clear();
        return;
    }
    
    qDebug() << "Data exists, channel count:" << m_eegData->channelCount();
    qDebug() << "Visible channels:" << m_visibleChannels;
    
    // Check if visible channels are valid
    if (m_visibleChannels.isEmpty()) {
        qDebug() << "WARNING: No visible channels selected!";
        // Clear chart but don't crash
        for (auto series : m_series) {
            m_chart->removeSeries(series);
            delete series;
        }
        m_series.clear();
        return;
    }
    
    // Remove old series
    for (auto series : m_series) {
        m_chart->removeSeries(series);
        delete series;
    }
    m_series.clear();
    
    // Get axes first
    QList<QAbstractAxis*> axesX = m_chart->axes(Qt::Horizontal);
    QList<QAbstractAxis*> axesY = m_chart->axes(Qt::Vertical);
    QValueAxis *axisX = axesX.isEmpty() ? nullptr : qobject_cast<QValueAxis*>(axesX.first());
    QValueAxis *axisY = axesY.isEmpty() ? nullptr : qobject_cast<QValueAxis*>(axesY.first());
    
    if (!axisX || !axisY) {
        qWarning() << "Failed to get chart axes, recreating chart";
        createChart();
        axesX = m_chart->axes(Qt::Horizontal);
        axesY = m_chart->axes(Qt::Vertical);
        axisX = axesX.isEmpty() ? nullptr : qobject_cast<QValueAxis*>(axesX.first());
        axisY = axesY.isEmpty() ? nullptr : qobject_cast<QValueAxis*>(axesY.first());
        if (!axisX || !axisY) {
            qCritical() << "Cannot get chart axes even after recreation";
            return;
        }
    }
    
    // Create new series for visible channels
    int channelCount = m_eegData->channelCount();
    qDebug() << "Processing" << m_visibleChannels.size() << "channels";
    
    for (int i = 0; i < m_visibleChannels.size(); ++i) {
        int channelIndex = m_visibleChannels[i];
        
        // Bounds check
        if (channelIndex < 0 || channelIndex >= channelCount) {
            qWarning() << "Invalid channel index:" << channelIndex << "(max:" << channelCount - 1 << ")";
            continue;
        }
        
        const EEGChannel &channel = m_eegData->channel(channelIndex);
        
        // Empty data check
        if (channel.data.isEmpty()) {
            qWarning() << "Channel" << channelIndex << "has empty data";
            continue;
        }
        
        qDebug() << "Creating series for channel" << channelIndex 
                 << "label:" << channel.label 
                 << "samples:" << channel.data.size()
                 << "samplingRate:" << channel.samplingRate;
        
        QLineSeries *series = new QLineSeries();
        series->setName(channel.label);

        bool isSelected = (channelIndex == m_selectedChannel);
        int penWidth = isSelected ? 3 : 1;
        QColor color = getChannelColor(i, isSelected);
        series->setPen(QPen(color, penWidth));
        
        // Add data points with bounds checking
        int startSample = static_cast<int>(m_startTime * channel.samplingRate);
        int endSample = static_cast<int>((m_startTime + m_duration) * channel.samplingRate);
        startSample = qMax(0, startSample);
        endSample = qMin(channel.data.size() - 1, endSample);
        
        qDebug() << "Time range:" << m_startTime << "to" << m_startTime + m_duration
                 << "Sample range:" << startSample << "to" << endSample;
        
        if (startSample <= endSample) {
            // Downsample for performance
            int step = qMax(1, (endSample - startSample) / 2000);
            double offset = i * m_offsetScale;
            
            int pointCount = 0;
            for (int s = startSample; s <= endSample; s += step) {
                // Extra bounds check
                if (s >= 0 && s < channel.data.size()) {
                    double time = s / channel.samplingRate;
                    double value = channel.data[s] * m_verticalScale + offset;
                    series->append(time, value);
                    pointCount++;
                }
            }
            qDebug() << "Added" << pointCount << "points to series";
        } else {
            qWarning() << "Invalid sample range for channel" << channelIndex;
        }
        
        m_series.append(series);
        m_chart->addSeries(series);
        
        // Attach to axes
        series->attachAxis(axisX);
        series->attachAxis(axisY);
    }
    
    // Update axes ranges
    axisX->setRange(m_startTime, m_startTime + m_duration);
    
    // Calculate Y-axis range based on visible channels
    if (!m_visibleChannels.isEmpty()) {
        double yMin = 0;
        double yMax = m_visibleChannels.size() * m_offsetScale;
        axisY->setRange(yMin - m_offsetScale * 0.5, yMax + m_offsetScale * 0.5);
        qDebug() << "Y-axis range:" << yMin - m_offsetScale * 0.5 << "to" << yMax + m_offsetScale * 0.5;
    }
    
    m_chart->update();
    qDebug() << "Chart update complete. Series count:" << m_series.size();
}

void EEGChartView::setVisibleChannels(const QVector<int> &channels) {
    m_visibleChannels = channels;
    updateChart();
}

void EEGChartView::setTimeRange(double startTime, double duration) {
    if (!m_eegData) {
        m_startTime = 0;
        m_duration = 10;
        updateChart();
        return;
    }
    
    double totalDuration = m_eegData->duration();
    
    m_duration = qMin(duration, totalDuration);
    m_duration = qMax(0.1, m_duration);
    
    m_startTime = qBound(0.0, startTime, totalDuration - m_duration);
    
    updateChart();
    emit timeRangeChanged(m_startTime, m_duration);
}

void EEGChartView::setVerticalScale(double scale) {
    m_verticalScale = qMax(0.1, qMin(10.0, scale));
    updateChart();
}

void EEGChartView::setOffsetScale(double offset) {
    m_offsetScale = qMax(10.0, qMin(500.0, offset));
    updateChart();
}

void EEGChartView::setShowGrid(bool show) {
    m_showGrid = show;
    QList<QAbstractAxis*> axesX = m_chart->axes(Qt::Horizontal);
    QList<QAbstractAxis*> axesY = m_chart->axes(Qt::Vertical);
    QValueAxis *axisX = axesX.isEmpty() ? nullptr : static_cast<QValueAxis*>(axesX.first());
    QValueAxis *axisY = axesY.isEmpty() ? nullptr : static_cast<QValueAxis*>(axesY.first());
    axisX->setGridLineVisible(show);
    axisY->setGridLineVisible(show);
    m_chart->update();
}

void EEGChartView::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        // Vertical zoom
        double factor = event->angleDelta().y() > 0 ? 1.1 : 0.9;
        m_verticalScale *= factor;
        updateChart();
    } else if (event->modifiers() & Qt::ShiftModifier) {
        // Vertical offset adjustment
        double factor = event->angleDelta().y() > 0 ? 1.1 : 0.9;
        m_offsetScale *= factor;
        updateChart();
    } else {
        // Horizontal zoom
        double factor = event->angleDelta().y() > 0 ? 0.8 : 1.2;
        m_duration *= factor;
        m_duration = qMax(0.1, qMin(60.0, m_duration)); // Limit between 0.1-60 seconds
        updateChart();
        emit timeRangeChanged(m_startTime, m_duration);
    }
    event->accept();
}

void EEGChartView::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton || 
        (event->button() == Qt::LeftButton && event->modifiers() & Qt::ShiftModifier)) {
        m_isPanning = true;
        m_lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    } else if (event->button() == Qt::RightButton) {
        m_isZooming = true;
        m_lastMousePos = event->pos();
        setCursor(Qt::SizeAllCursor);
    } else {
        QChartView::mousePressEvent(event);
    }
}

void EEGChartView::mouseMoveEvent(QMouseEvent *event) {
    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastMousePos;
        QRectF plotArea = m_chart->plotArea();
        QList<QAbstractAxis*> axesX = m_chart->axes(Qt::Horizontal);
        QValueAxis *axisX = axesX.isEmpty() ? nullptr : static_cast<QValueAxis*>(axesX.first());
        
        double xRange = axisX->max() - axisX->min();
        double dx = -delta.x() * xRange / plotArea.width();
        
        m_startTime += dx;
        if (m_eegData) {
            double totalDuration = m_eegData->duration();
            m_startTime = qMax(0.0, qMin(m_startTime, totalDuration - m_duration));
        }
        
        m_lastMousePos = event->pos();
        updateChart();
        emit timeRangeChanged(m_startTime, m_duration);
    } else if (m_isZooming) {
        QPoint delta = event->pos() - m_lastMousePos;
        double factor = 1.0 + delta.y() * 0.01;
        m_verticalScale *= factor;
        m_verticalScale = qMax(0.1, qMin(10.0, m_verticalScale));
        m_lastMousePos = event->pos();
        updateChart();
    } else {
        QChartView::mouseMoveEvent(event);
    }
}

void EEGChartView::mouseReleaseEvent(QMouseEvent *event) {
    if (m_isPanning && (event->button() == Qt::MiddleButton || 
                       (event->button() == Qt::LeftButton && event->modifiers() & Qt::ShiftModifier))) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
    } else if (m_isZooming && event->button() == Qt::RightButton) {
        m_isZooming = false;
        setCursor(Qt::ArrowCursor);
    } else {
        QChartView::mouseReleaseEvent(event);
    }
}

void EEGChartView::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
    case Qt::Key_Left:
        m_startTime = qMax(0.0, m_startTime - m_duration * 0.1);
        updateChart();
        emit timeRangeChanged(m_startTime, m_duration);
        break;
    case Qt::Key_Right:
        if (m_eegData) {
            double totalDuration = m_eegData->duration();
            m_startTime = qMin(totalDuration - m_duration, m_startTime + m_duration * 0.1);
        }
        updateChart();
        emit timeRangeChanged(m_startTime, m_duration);
        break;
    case Qt::Key_Up:
        m_verticalScale = qMin(10.0, m_verticalScale * 1.1);
        updateChart();
        break;
    case Qt::Key_Down:
        m_verticalScale = qMax(0.1, m_verticalScale * 0.9);
        updateChart();
        break;
    case Qt::Key_G:
        setShowGrid(!m_showGrid);
        break;
    default:
        QChartView::keyPressEvent(event);
    }
}

void EEGChartView::setSelectedChannel(int channel) {
    m_selectedChannel = channel;
    updateChart(); 
}

QColor EEGChartView::getChannelColor(int index, bool isSelected) const {
    if (isSelected) {
        return Qt::yellow; 
    }
    
    static const QVector<QColor> colors = {
        Qt::blue, Qt::red, Qt::green, Qt::magenta,
        Qt::darkCyan, Qt::darkYellow, Qt::darkMagenta, Qt::darkBlue,
        Qt::darkRed, Qt::darkGreen, Qt::cyan, Qt::yellow
    };
    return colors[index % colors.size()];
}