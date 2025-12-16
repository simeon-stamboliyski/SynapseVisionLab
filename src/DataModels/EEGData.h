#pragma once
#include <QObject>
#include <QVector>
#include <QString>
#include <QDateTime>
#include "../Utils/SignalProcessor.h"

struct EEGChannel {
    QString label;
    QString unit;
    double physicalMin = -1000.0;
    double physicalMax = 1000.0;
    double digitalMin = -32768.0;
    double digitalMax = 32767.0;
    double samplingRate = 250.0; // Hz
    QVector<double> data;

    double duration() const {
        return data.size() / samplingRate;
    }

    int sampleCount() const {
        return data.size();
    }
};

class EEGData : public QObject {
    Q_OBJECT

public:
    explicit EEGData(QObject *parent = nullptr);
    ~EEGData();

    // File operations (now delegates to EEGFileHandler)
    bool loadFromFile(const QString &filePath);
    bool saveToFile(const QString &filePath);
    void clear();

    // Data manipulation
    void addChannel(const EEGChannel &channel);
    void removeChannel(int index);
    void normalizeChannel(int channelIndex);
    void applyGain(int channelIndex, double gain);
    void applyOffset(int channelIndex, double offset);
    void applyFilter(int channelIndex, double lowCut, double highCut);
    void removeDC(int channelIndex);

    // Data access
    const QVector<EEGChannel>& channels() const { return m_channels; }
    EEGChannel& channel(int index) { return m_channels[index]; }
    const EEGChannel& channel(int index) const { return m_channels[index]; }
    
    int channelCount() const { return m_channels.size(); }
    bool isEmpty() const { return m_channels.isEmpty(); }
    
    double maxSamplingRate() const;
    double duration() const;

    // Statistics
    QVector<double> channelMeans() const;
    QVector<double> channelStdDevs() const;
    QVector<double> getTimeSeries(int channelIndex, double startTime, double duration) const;

    // Metadata
    QString patientInfo() const { return m_patientInfo; }
    void setPatientInfo(const QString &info) { m_patientInfo = info; }
    
    QString recordingInfo() const { return m_recordingInfo; }
    void setRecordingInfo(const QString &info) { m_recordingInfo = info; }
    
    QDateTime startDateTime() const { return m_startDateTime; }
    void setStartDateTime(const QDateTime &dt) { m_startDateTime = dt; }

    void applyMontage(SignalProcessor::MontageType montage);
    void applyNotchFilter(int channelIndex, double notchFreq);
signals:
    void dataChanged();
    void channelAdded(int index);
    void channelRemoved(int index);

private:
    QVector<EEGChannel> m_channels;
    QString m_patientInfo;
    QString m_recordingInfo;
    QDateTime m_startDateTime;
};