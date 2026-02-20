#include "EEGFileHandler.h"
#include "../DataModels/EEGData.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <QDateTime>
#include <QDataStream>     
#include <QString>
#include <QSysInfo>
#include <cmath>
#include <algorithm> 
#include <numeric>

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
    
    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);
    
    // ===== READ HEADER =====
    char header[256];
    if (stream.readRawData(header, 256) != 256) {
        qWarning() << "Failed to read EDF header";
        file.close();
        return false;
    }
    
    // Parse basic info from header
    QString version = QString::fromLatin1(header, 8);
    QString patientID = QString::fromLatin1(header + 8, 80);
    QString recordingInfo = QString::fromLatin1(header + 88, 80);
    QString startDate = QString::fromLatin1(header + 168, 8);
    QString startTime = QString::fromLatin1(header + 176, 8);
    QString numSignalsStr = QString::fromLatin1(header + 252, 4);
    
    bool ok;
    int numSignals = numSignalsStr.trimmed().toInt(&ok);
    if (!ok || numSignals <= 0) {
        qWarning() << "Invalid number of signals:" << numSignalsStr;
        file.close();
        return false;
    }
    
    // ===== READ SIGNAL HEADERS =====
    QVector<QString> labels(numSignals);
    QVector<int> samplesPerRecord(numSignals);
    
    // Labels (16 chars each)
    for (int i = 0; i < numSignals; ++i) {
        char label[17] = {0};
        if (stream.readRawData(label, 16) != 16) {
            qWarning() << "Failed to read label for signal" << i;
            file.close();
            return false;
        }
        labels[i] = QString::fromLatin1(label, 16).trimmed();
    }
    
    // Skip transducer type (80 chars per signal)
    stream.skipRawData(80 * numSignals);
    
    // Skip physical dimension (8 chars per signal) - e.g., "uV", "V"
    stream.skipRawData(8 * numSignals);
    
    // Read physical/digital min/max values
    QVector<double> physMin(numSignals);
    QVector<double> physMax(numSignals);
    QVector<double> digMin(numSignals);
    QVector<double> digMax(numSignals);
    
    for (int i = 0; i < numSignals; ++i) {
        char physMinStr[9] = {0};
        char physMaxStr[9] = {0};
        char digMinStr[9] = {0};
        char digMaxStr[9] = {0};
        
        if (stream.readRawData(physMinStr, 8) != 8 ||
            stream.readRawData(physMaxStr, 8) != 8 ||
            stream.readRawData(digMinStr, 8) != 8 ||
            stream.readRawData(digMaxStr, 8) != 8) {
            qWarning() << "Failed to read min/max values for signal" << i;
            file.close();
            return false;
        }
        
        QString physMinQStr = QString::fromLatin1(physMinStr, 8).trimmed();
        QString physMaxQStr = QString::fromLatin1(physMaxStr, 8).trimmed();
        QString digMinQStr = QString::fromLatin1(digMinStr, 8).trimmed();
        QString digMaxQStr = QString::fromLatin1(digMaxStr, 8).trimmed();
        
        physMin[i] = physMinQStr.toDouble(&ok);
        if (!ok) physMin[i] = -500.0;
        
        physMax[i] = physMaxQStr.toDouble(&ok);
        if (!ok) physMax[i] = 500.0;
        
        digMin[i] = digMinQStr.toDouble(&ok);
        if (!ok) digMin[i] = -32768.0;
        
        digMax[i] = digMaxQStr.toDouble(&ok);
        if (!ok) digMax[i] = 32767.0;
        
        // Debug check for corrupted values
        if (qAbs(physMax[i] - physMin[i]) < 0.1 || qAbs(digMax[i] - digMin[i]) < 0.1) {
            qWarning() << "WARNING: Corrupted calibration values for signal" << i << labels[i]
                       << "phys:" << physMin[i] << "to" << physMax[i]
                       << "dig:" << digMin[i] << "to" << digMax[i];
        }
    }
    
    // Skip prefiltering (80 chars per signal)
    stream.skipRawData(80 * numSignals);
    
    // Skip reserved (32 chars per signal) - THIS IS CRITICAL!
    stream.skipRawData(32 * numSignals);
    
    // Read samples per data record
    for (int i = 0; i < numSignals; ++i) {
        char samplesStr[9] = {0};
        if (stream.readRawData(samplesStr, 8) != 8) {
            qWarning() << "Failed to read samples per record for signal" << i;
            file.close();
            return false;
        }
        samplesPerRecord[i] = QString(samplesStr).trimmed().toInt(&ok);
        if (!ok) samplesPerRecord[i] = 1;
    }
    
    // Read duration of data record
    char durationStr[9] = {0};
    stream.readRawData(durationStr, 8);
    double recordDuration = QString::fromLatin1(durationStr, 8).trimmed().toDouble(&ok);
    if (!ok || recordDuration <= 0) {
        recordDuration = 1.0; // Default
    }
    
    for (int i = 0; i < qMin(8, numSignals); ++i) {
        double samplingRate = samplesPerRecord[i] / recordDuration;
    }
    
    // ===== READ DATA =====
    data.clear();
    
    // Get current position (end of headers)
    qint64 headerEndPos = file.pos();
    
    // Calculate total samples per record
    int totalSamplesPerRecord = 0;
    for (int i = 0; i < numSignals; ++i) {
        totalSamplesPerRecord += samplesPerRecord[i];
    }
    
    // Calculate bytes per record (2 bytes per sample in EDF)
    int bytesPerRecord = totalSamplesPerRecord * 2;
    
    // Calculate number of records from file size
    qint64 fileSize = file.size();
    qint64 dataSize = fileSize - headerEndPos;
    int numRecords = dataSize / bytesPerRecord;
    
    numRecords = qMin(numRecords, 10000);
    
    // Allocate raw data storage
    QVector<QVector<short>> rawData(numSignals);
    for (int i = 0; i < numSignals; ++i) {
        int totalSamples = samplesPerRecord[i] * numRecords;
        rawData[i].resize(totalSamples);
        if (i < 8) {
            qDebug() << "Allocating" << totalSamples << "samples for channel" << i << labels[i];
        }
    }
    
    // Seek to data start
    file.seek(headerEndPos);
    
    // Read data record by record
    int totalSamplesRead = 0;
    for (int rec = 0; rec < numRecords; ++rec) {
        for (int sig = 0; sig < numSignals; ++sig) {
            int offset = rec * samplesPerRecord[sig];
            
            // Read each sample
            for (int s = 0; s < samplesPerRecord[sig]; ++s) {
                short sample;
                qint64 bytesRead = file.read((char*)&sample, sizeof(short));
                
                if (bytesRead != sizeof(short)) {
                    qWarning() << "Failed to read sample" << s << "for signal" << sig 
                              << "in record" << rec;
                    file.close();
                    return false;
                }
                
                // Byte swap if needed (EDF is little-endian)
                if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
                    sample = ((sample & 0xFF) << 8) | ((sample >> 8) & 0xFF);
                }
                
                rawData[sig][offset + s] = sample;
                totalSamplesRead++;
            }
        }
    }
    
    
    // Convert raw data to EEG channels
    int channelsToLoad = qMin(numSignals, 32); // Limit to 32 channels
    for (int sig = 0; sig < channelsToLoad; ++sig) {
        // Skip annotation channels
        if (labels[sig].contains("EDF Annotations", Qt::CaseInsensitive) ||
            labels[sig].contains("Annotation", Qt::CaseInsensitive)) {
            qDebug() << "Skipping annotation channel:" << sig << labels[sig];
            continue;
        }
        
        EEGChannel channel;
        channel.label = labels[sig];
        if (channel.label.isEmpty()) {
            channel.label = QString("CH%1").arg(sig + 1);
        }
        channel.unit = "µV";
        
        // Calculate sampling rate
        channel.samplingRate = samplesPerRecord[sig] / recordDuration;
        
        // ===== DYNAMIC SCALING SECTION =====
        double scale, offset;
        
        if (qAbs(digMax[sig] - digMin[sig]) > 0.1 && qAbs(physMax[sig] - physMin[sig]) > 0.1) {
            // 1. Use EDF calibration if valid
            scale = (physMax[sig] - physMin[sig]) / (digMax[sig] - digMin[sig]);
            offset = physMin[sig] - digMin[sig] * scale;
        } 
        else if (!rawData[sig].empty() && rawData[sig].size() > 10) {
            // 2. Dynamic scaling based on data statistics
            short minVal = *std::min_element(rawData[sig].begin(), rawData[sig].end());
            short maxVal = *std::max_element(rawData[sig].begin(), rawData[sig].end());
            double range = maxVal - minVal;
            
            if (range > 0.1) {
                // Calculate mean for centering
                double sum = 0.0;
                for (short val : rawData[sig]) {
                    sum += val;
                }
                double mean = sum / rawData[sig].size();
                
                // Smart scaling logic:
                if (range < 100) {  // Already in good µV range
                    scale = 1.0;
                    offset = 0.0;
                }
                else if (range > 30000) {  // Full 16-bit range
                    scale = 200.0 / 65536.0;  // Map to ±100µV
                    offset = -mean * scale;
                }
                else {  // Custom scaling - normalize to reasonable range
                    // Scale to ±50µV range (adjustable)
                    double targetRange = 100.0;  // ±50µV
                    scale = targetRange / range;
                    offset = -mean * scale;
                }
            } else {
                // Very small range
                scale = 1.0;
                offset = 0.0;
            }
        } 
        else {
            // 3. Ultimate fallback
            scale = 1.0;
            offset = 0.0;
        }
        // ===== END DYNAMIC SCALING =====
        
        // Convert samples
        int numSamples = rawData[sig].size();
        channel.data.resize(numSamples);
        
        for (int i = 0; i < numSamples; ++i) {
            channel.data[i] = rawData[sig][i] * scale + offset;
        }
        
        data.addChannel(channel);
        
        // Debug output for first few channels
        if (sig < 5 && numSamples > 0) {
            double minVal = *std::min_element(channel.data.begin(), channel.data.end());
            double maxVal = *std::max_element(channel.data.begin(), channel.data.end());
            double meanVal = std::accumulate(channel.data.begin(), channel.data.end(), 0.0) / numSamples;
        }
    }
    
    // Set metadata
    data.setPatientInfo(patientID.trimmed());
    data.setRecordingInfo(recordingInfo.trimmed());
    
    // Parse date/time
    QDateTime startDT = QDateTime::fromString(
        startDate.trimmed() + " " + startTime.trimmed(), 
        "dd.MM.yy hh.mm.ss"
    );
    if (startDT.isValid()) {
        data.setStartDateTime(startDT);
    }
    
    file.close();
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
    
    // Find max samples and get actual sampling rate
    int maxSamples = 0;
    double samplingRate = 250.0; // Default
    
    for (int i = 0; i < data.channelCount(); ++i) {
        maxSamples = qMax(maxSamples, data.channel(i).data.size());
        if (data.channel(i).samplingRate > 0) {
            samplingRate = data.channel(i).samplingRate; // Use actual rate
        }
    }
    
    // Write data
    for (int sample = 0; sample < maxSamples; ++sample) {
        double time = sample / samplingRate;  // Use actual sampling rate
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
    return true;
}

} 