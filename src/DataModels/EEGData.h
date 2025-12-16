#pragma once
#include <QObject>
#include <QVector>
#include <QString>
#include <QMap>

struct EEGChannel {
    Qstring label;
    Qstring unit;
    double physicalMin;
    double physicalMax;
    double digitalMin;
    double digitalMax;
    double samplingRate;
    QVector<double> data;
};

class EEGData : class QObject {
    Q_OBJECT

public:
    explicit EEGData(QObject *parent = nullptr);

    // These functions will be for file loading
    bool loadFromFile(const QString &filePath);
    bool loadEDF(const QString &filePath);
    bool loadCSV(const QString &filePath);

    // functions for data accessing
    const QVector<EEGChannel>& channels const { return m_channels; }
    int channelCount const { return m_channels.size(); }
    double duration() const;
    double maxSamplingRate() const;

    // functions for data processing
    void applyFilter(int channelIndex, double lowCut, double highCut);
    void removeDC(int channelIndex);
    QVector<double> getTimeSeries(int channelIndex, double startTime, double duration);

signals:
    void dataLoaded(bool success, const QString &error = "");
    void channelsUpdated();

private:
    QVector<EEGCHannel> m_channels;
    QString m_filePath;
    QDateTime m_recording_start;
};