#include "EEGFileHandler.h"
#include "../DataModels/EEGData.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <QDateTime>
#include <cmath>

namespace EEGFileHandler {

static bool loadEDF(const QString &filePath, EEGData &data);
static bool loadCSV(const QString &filePath, EEGData &data);
static bool saveEDF(const QString &filePath, const EEGData &data);
static bool saveCSV(const QString &filePath, const EEGData &data);

bool loadFile(const QString &filePath, EEGData &data) {
    QString ext = QFileInfo(filePath).suffix().toLower();
    
    if (ext == "edf") return loadEDF(filePath, data);
    if (ext == "csv" || ext == "txt" || ext == "dat") return loadCSV(filePath, data);
    
    // Try auto-detection
    if (loadEDF(filePath, data)) return true;
    if (loadCSV(filePath, data)) return true;
    
    qWarning() << "Unsupported file format:" << filePath;
    return false;
}

bool saveFile(const QString &filePath, const EEGData &data) {
    QString ext = QFileInfo(filePath).suffix().toLower();
    
    if (ext == "edf") return saveEDF(filePath, data);
    return saveCSV(filePath, data);
}

// ================== EDF LOADER ==================

static bool loadEDF(const QString &filePath, EEGData &data) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open EDF:" << filePath;
        return false;
    }
    
    // Simple EDF parser - creates dummy data for now
    // You can expand this with real EDF parsing later
    
    data.clear();
    
    // Create 8 dummy EEG channels
    QStringList channelNames = {"Fp1", "Fp2", "F3", "F4", "C3", "C4", "P3", "P4"};
    
    for (int ch = 0; ch < 8; ++ch) {
        EEGChannel channel;
        channel.label = channelNames[ch];
        channel.unit = "uV";
        channel.samplingRate = 250.0;
        
        // Generate 10 seconds of fake EEG data
        int sampleCount = static_cast<int>(10.0 * channel.samplingRate);
        channel.data.resize(sampleCount);
        
        // Generate sine waves with different frequencies
        for (int i = 0; i < sampleCount; ++i) {
            double t = i / channel.samplingRate;
            channel.data[i] = 
                50.0 * sin(2 * M_PI * 10.0 * t) +   // Alpha waves
                20.0 * sin(2 * M_PI * 20.0 * t) +   // Beta waves
                10.0 * sin(2 * M_PI * 40.0 * t);    // Gamma waves
        }
        
        data.addChannel(channel);
    }
    
    data.setPatientInfo("Test Patient");
    data.setRecordingInfo("EDF Import");
    data.setStartDateTime(QDateTime::currentDateTime());
    
    file.close();
    qDebug() << "Loaded dummy EDF data with 8 channels";
    return true;
}

// ================== CSV LOADER ==================

static bool loadCSV(const QString &filePath, EEGData &data) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open CSV:" << filePath;
        return false;
    }
    
    QTextStream stream(&file);
    QStringList lines;
    
    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        if (!line.isEmpty() && !line.startsWith("#")) {
            lines.append(line);
        }
    }
    file.close();
    
    if (lines.isEmpty()) {
        qWarning() << "Empty CSV file";
        return false;
    }
    
    data.clear();
    
    // Parse header
    QString headerLine = lines.first();
    QStringList headers = headerLine.split(',');
    if (headers.size() < 2) headers = headerLine.split('\t');
    if (headers.size() < 2) headers = headerLine.split(';');
    
    if (headers.size() < 2) {
        qWarning() << "Invalid CSV format";
        return false;
    }
    
    // Create channels
    int numChannels = headers.size() - 1;
    QVector<QVector<double>> channelData(numChannels);
    
    // Parse data
    for (int row = 1; row < lines.size(); ++row) {
        QString line = lines[row];
        QStringList values = line.split(',');
        if (values.size() < 2) values = line.split('\t');
        if (values.size() < 2) values = line.split(';');
        
        if (values.size() != headers.size()) {
            qWarning() << "Row" << row << "has wrong number of values";
            continue;
        }
        
        for (int ch = 0; ch < numChannels; ++ch) {
            bool ok;
            double value = values[ch + 1].toDouble(&ok);
            channelData[ch].append(ok ? value : 0.0);
        }
    }
    
    // Create EEG channels
    for (int i = 0; i < numChannels; ++i) {
        EEGChannel channel;
        channel.label = headers[i + 1].trimmed();
        if (channel.label.isEmpty()) {
            channel.label = QString("Channel_%1").arg(i + 1);
        }
        channel.unit = "uV";
        channel.samplingRate = 250.0;
        channel.data = channelData[i];
        
        data.addChannel(channel);
    }
    
    QFileInfo info(filePath);
    data.setPatientInfo(info.baseName());
    data.setRecordingInfo("CSV Import");
    
    qDebug() << "Loaded CSV with" << numChannels << "channels," 
             << (channelData.size() > 0 ? channelData[0].size() : 0) << "samples";
    return true;
}

// ================== EDF SAVER ==================

static bool saveEDF(const QString &filePath, const EEGData &data) {
    if (data.isEmpty()) {
        qWarning() << "Cannot save empty data to EDF";
        return false;
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to create EDF file";
        return false;
    }
    
    // Simple EDF writer - basic implementation
    QByteArray header(256, ' ');
    
    // Version
    memcpy(header.data(), "0       ", 8);
    
    // Patient ID
    QString patient = data.patientInfo().left(80);
    memcpy(header.data() + 8, patient.leftJustified(80, ' ').toLatin1().constData(), 80);
    
    // Recording ID
    QString recording = data.recordingInfo().left(80);
    memcpy(header.data() + 88, recording.leftJustified(80, ' ').toLatin1().constData(), 80);
    
    // Start date/time
    QDateTime dt = data.startDateTime();
    QString dateStr = dt.toString("dd.MM.yy");
    QString timeStr = dt.toString("hh.mm.ss");
    memcpy(header.data() + 168, dateStr.leftJustified(8, ' ').toLatin1().constData(), 8);
    memcpy(header.data() + 176, timeStr.leftJustified(8, ' ').toLatin1().constData(), 8);
    
    // Number of signals
    int numSignals = data.channelCount();
    QString numSignalsStr = QString::number(numSignals);
    memcpy(header.data() + 252, numSignalsStr.leftJustified(4, ' ').toLatin1().constData(), 4);
    
    // Write header
    file.write(header);
    
    // For now, just create a basic file
    // Full EDF implementation would go here
    
    file.close();
    qDebug() << "Saved basic EDF file with" << numSignals << "channels";
    return true;
}

// ================== CSV SAVER ==================

static bool saveCSV(const QString &filePath, const EEGData &data) {
    if (data.isEmpty()) {
        qWarning() << "Cannot save empty data to CSV";
        return false;
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to create CSV file";
        return false;
    }
    
    QTextStream stream(&file);
    
    // Write header
    stream << "Time(s)";
    for (int i = 0; i < data.channelCount(); ++i) {
        stream << "," << data.channel(i).label;
    }
    stream << "\n";
    
    // Find max samples
    int maxSamples = 0;
    for (int i = 0; i < data.channelCount(); ++i) {
        maxSamples = qMax(maxSamples, data.channel(i).data.size());
    }
    
    // Write data
    for (int sample = 0; sample < maxSamples; ++sample) {
        double time = sample / 250.0;  // Default sampling rate
        stream << QString::number(time, 'f', 6);
        
        for (int ch = 0; ch < data.channelCount(); ++ch) {
            stream << ",";
            if (sample < data.channel(ch).data.size()) {
                stream << QString::number(data.channel(ch).data[sample], 'f', 6);
            } else {
                stream << "0";
            }
        }
        stream << "\n";
    }
    
    file.close();
    qDebug() << "Saved CSV with" << data.channelCount() << "channels," 
             << maxSamples << "samples";
    return true;
}

} 