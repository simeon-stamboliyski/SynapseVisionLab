#include "EEGData.h"
#include "EEGFileHandler.h"
#include "SignalProcessor.h"  
#include <QDebug>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <QtGlobal>

EEGData::EEGData(QObject *parent) : QObject(parent) {
    m_startDateTime = QDateTime::currentDateTime();
}

EEGData::~EEGData() {
    clear();
}

bool EEGData::loadFromFile(const QString &filePath) {
    clear();
    bool success = EEGFileHandler::loadFile(filePath, *this);
    if (success) emit dataChanged();
    return success;
}

bool EEGData::saveToFile(const QString &filePath) {
    return EEGFileHandler::saveFile(filePath, *this);
}

void EEGData::clear() {
    m_channels.clear();
    m_patientInfo.clear();
    m_recordingInfo.clear();
    m_startDateTime = QDateTime::currentDateTime();
    emit dataChanged();
}

void EEGData::addChannel(const EEGChannel &channel) {
    m_channels.append(channel);
    emit channelAdded(m_channels.size() - 1);
}

void EEGData::removeChannel(int index) {
    if (index >= 0 && index < m_channels.size()) {
        m_channels.removeAt(index);
        emit channelRemoved(index);
    }
}

double EEGData::maxSamplingRate() const {
    if (m_channels.isEmpty()) return 0.0;
    
    double maxRate = 0.0;
    for (const auto &ch : m_channels) {
        maxRate = std::max(maxRate, ch.samplingRate);
    }
    return maxRate;
}

double EEGData::duration() const {
    if (m_channels.isEmpty()) return 0.0;
    
    double maxDuration = 0.0;
    for (const auto &ch : m_channels) {
        maxDuration = std::max(maxDuration, ch.duration());
    }
    return maxDuration;
}

void EEGData::normalizeChannel(int channelIndex) {
    if (channelIndex < 0 || channelIndex >= m_channels.size()) return;
    
    EEGChannel &channel = m_channels[channelIndex];
    SignalProcessor::normalize(channel.data);
    
    // Update physical range
    channel.physicalMin = 0.0;
    channel.physicalMax = 1.0;
    
    emit dataChanged();
}

void EEGData::applyGain(int channelIndex, double gain) {
    if (channelIndex < 0 || channelIndex >= m_channels.size()) return;
    
    EEGChannel &channel = m_channels[channelIndex];
    SignalProcessor::applyGain(channel.data, gain);
    
    // Update physical range
    channel.physicalMin *= gain;
    channel.physicalMax *= gain;
    
    emit dataChanged();
}

void EEGData::applyOffset(int channelIndex, double offset) {
    if (channelIndex < 0 || channelIndex >= m_channels.size()) return;
    
    EEGChannel &channel = m_channels[channelIndex];
    SignalProcessor::applyOffset(channel.data, offset);
    
    // Update physical range
    channel.physicalMin += offset;
    channel.physicalMax += offset;
    
    emit dataChanged();
}

void EEGData::applyFilter(int channelIndex, double lowCut, double highCut) {
    if (channelIndex < 0 || channelIndex >= m_channels.size()) return;
    
    EEGChannel &channel = m_channels[channelIndex];
    SignalProcessor::bandpassFilter(channel.data, channel.samplingRate, lowCut, highCut);
    
    emit dataChanged();
}

void EEGData::removeDC(int channelIndex) {
    if (channelIndex < 0 || channelIndex >= m_channels.size()) return;
    
    EEGChannel &channel = m_channels[channelIndex];
    SignalProcessor::removeDC(channel.data);
    
    // Update mean
    double mean = SignalProcessor::mean(channel.data);
    channel.physicalMin -= mean;
    channel.physicalMax -= mean;
    
    emit dataChanged();
}

QVector<double> EEGData::channelMeans() const {
    QVector<double> means;
    means.reserve(m_channels.size());
    
    for (const auto &channel : m_channels) {
        means.append(SignalProcessor::mean(channel.data));
    }
    
    return means;
}

QVector<double> EEGData::channelStdDevs() const {
    QVector<double> stddevs;
    stddevs.reserve(m_channels.size());
    
    for (const auto &channel : m_channels) {
        stddevs.append(SignalProcessor::standardDeviation(channel.data));
    }
    
    return stddevs;
}

QVector<double> EEGData::getTimeSeries(int channelIndex, double startTime, double duration) const {
    if (channelIndex < 0 || channelIndex >= m_channels.size()) return QVector<double>();
    
    const EEGChannel &channel = m_channels[channelIndex];
    return SignalProcessor::extractTimeWindow(channel.data, channel.samplingRate, startTime, duration);
}

// Add new method for montage support
void EEGData::applyMontage(SignalProcessor::MontageType montage) {
    QVector<QVector<double>> allData;
    QVector<QString> labels;
    
    for (const auto &channel : m_channels) {
        allData.append(channel.data);
        labels.append(channel.label);
    }
    
    SignalProcessor::applyMontage(allData, labels, montage);
    
    // Update channels with processed data
    for (int i = 0; i < m_channels.size() && i < allData.size(); ++i) {
        m_channels[i].data = allData[i];
    }
    
    emit dataChanged();
}

// Add new method for notch filtering
void EEGData::applyNotchFilter(int channelIndex, double notchFreq) {
    if (channelIndex < 0 || channelIndex >= m_channels.size()) return;
    
    EEGChannel &channel = m_channels[channelIndex];
    SignalProcessor::notchFilter(channel.data, channel.samplingRate, notchFreq);
    
    emit dataChanged();
}

bool EEGData::isEmpty() const {
    return m_channels.isEmpty();
}