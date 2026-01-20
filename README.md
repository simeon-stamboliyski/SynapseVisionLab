# SynapseVisionLab

# EEGData.h - Core Data Model Component
📁 File: src/DataModels/EEGData.h
📋 Category: Data Model / Core Container
🎯 Purpose
This is the central data container for the entire EEG application. It stores all EEG signals, metadata, and provides an interface for data manipulation and processing. Think of it as the "brain" of your application where all EEG data lives.

🔑 Key Responsibilities
1. Data Storage
Stores multiple EEG channels (signals) in QVector<EEGChannel>

Maintains metadata: patient info, recording details, timestamps

Manages channel properties: labels, units, sampling rates, calibration

2. Data Manipulation
Add/remove EEG channels

Apply signal processing operations (via SignalProcessor)

Normalize, filter, adjust gain/offset of signals

3. Data Access & Statistics
Provide access to raw signal data

Calculate basic statistics (means, standard deviations)

Extract time series segments for visualization

4. File Operations Interface
Delegates actual file I/O to EEGFileHandler class

Provides clean API for loading/saving data

🔗 Dependencies
Internal: SignalProcessor.h (for signal processing algorithms)

Qt Core: QObject, QVector, QString, QDateTime

Signal/Slot: Uses Qt's signal system for notifications

🤝 Interactions with Other Components
Used By:
MainWindow - Primary controller that manipulates EEG data

EEGChartView - Visualization component that reads data for display

EEGFileHandler - Called for file operations

main.cpp - Application entry may create initial instance

Uses:
SignalProcessor - For all signal processing operations

EEGFileHandler - For file I/O (through forward declaration/delegation)

# EEGData.cpp - Core Data Model Implementation

## 📁 **File**: `src/DataModels/EEGData.cpp`

## 📋 **Category**: Data Model / Core Container (Implementation)

## 🎯 **Purpose**
This is the implementation file for the `EEGData` class. It provides the actual logic for data manipulation, signal processing delegation, and maintains data integrity throughout the application lifecycle.

## 🔑 **Key Implementation Details**

### **1. Construction/Destruction**
```cpp
EEGData::EEGData(QObject *parent) : QObject(parent) {
    m_startDateTime = QDateTime::currentDateTime();  // Default timestamp
}
```
- Inherits from `QObject` for Qt's signal/slot system
- Initializes with current timestamp as default

```cpp
EEGData::~EEGData() {
    clear();  // Clean up all data
}
```
- Automatically clears all data on destruction
- Ensures proper memory cleanup

### **2. File Operations (Delegation Pattern)**
```cpp
bool EEGData::loadFromFile(const QString &filePath) {
    clear();  // Clear existing data
    bool success = EEGFileHandler::loadFile(filePath, *this);
    if (success) emit dataChanged();  // Notify UI
    return success;
}
```
- Delegates actual file reading to `EEGFileHandler`
- Clears existing data before loading new data
- Emits `dataChanged()` signal on successful load

```cpp
bool EEGData::saveToFile(const QString &filePath) {
    return EEGFileHandler::saveFile(filePath, *this);
}
```
- Delegates file writing to `EEGFileHandler`
- Returns success/failure status

### **3. Data Management**
```cpp
void EEGData::clear() {
    m_channels.clear();           // Remove all channels
    m_patientInfo.clear();        // Clear metadata
    m_recordingInfo.clear();      // Clear metadata
    m_startDateTime = QDateTime::currentDateTime();  // Reset timestamp
    emit dataChanged();           // Notify UI
}
```
- Completely resets the data model
- Updates timestamp to current time
- Notifies all connected UI components

### **4. Channel Operations**
```cpp
void EEGData::addChannel(const EEGChannel &channel) {
    m_channels.append(channel);
    emit channelAdded(m_channels.size() - 1);  // Signal with index
}
```
- Appends new channel to the collection
- Emits signal with the new channel's index

```cpp
void EEGData::removeChannel(int index) {
    if (index >= 0 && index < m_channels.size()) {
        m_channels.removeAt(index);
        emit channelRemoved(index);  // Signal which channel was removed
    }
}
```
- Bounds checking before removal
- Emits signal with removed channel index

### **5. Statistical Calculations**
```cpp
double EEGData::maxSamplingRate() const {
    if (m_channels.isEmpty()) return 0.0;
    
    double maxRate = 0.0;
    for (const auto &ch : m_channels) {
        maxRate = std::max(maxRate, ch.samplingRate);
    }
    return maxRate;
}
```
- Iterates through all channels to find maximum sampling rate
- Returns 0.0 if no channels exist

```cpp
double EEGData::duration() const {
    if (m_channels.isEmpty()) return 0.0;
    
    double maxDuration = 0.0;
    for (const auto &ch : m_channels) {
        maxDuration = std::max(maxDuration, ch.duration());
    }
    return maxDuration;
}
```
- Finds the longest duration among all channels
- Uses channel's `duration()` method (samples / samplingRate)

### **6. Signal Processing Delegation**
**Key Pattern**: Each processing method:
1. Validates channel index
2. Delegates to `SignalProcessor` static methods
3. Updates channel metadata (physical range)
4. Emits `dataChanged()` signal

```cpp
void EEGData::normalizeChannel(int channelIndex) {
    if (channelIndex < 0 || channelIndex >= m_channels.size()) return;
    
    EEGChannel &channel = m_channels[channelIndex];
    SignalProcessor::normalize(channel.data);  // Delegate to SignalProcessor
    
    // Update physical range to normalized range [0, 1]
    channel.physicalMin = 0.0;
    channel.physicalMax = 1.0;
    
    emit dataChanged();  // Notify UI
}
```

```cpp
void EEGData::applyGain(int channelIndex, double gain) {
    // ... validation ...
    SignalProcessor::applyGain(channel.data, gain);  // Delegate
    
    // Update physical range proportionally
    channel.physicalMin *= gain;
    channel.physicalMax *= gain;
    
    emit dataChanged();
}
```

```cpp
void EEGData::applyFilter(int channelIndex, double lowCut, double highCut) {
    // ... validation ...
    SignalProcessor::bandpassFilter(channel.data, channel.samplingRate, lowCut, highCut);
    emit dataChanged();  // Filter doesn't change physical range
}
```

### **7. Advanced Processing Methods**
```cpp
void EEGData::applyMontage(SignalProcessor::MontageType montage) {
    // Collect all channel data and labels
    QVector<QVector<double>> allData;
    QVector<QString> labels;
    
    for (const auto &channel : m_channels) {
        allData.append(channel.data);
        labels.append(channel.label);
    }
    
    // Delegate montage calculation to SignalProcessor
    SignalProcessor::applyMontage(allData, labels, montage);
    
    // Update channels with processed data
    for (int i = 0; i < m_channels.size() && i < allData.size(); ++i) {
        m_channels[i].data = allData[i];
    }
    
    emit dataChanged();
}
```
- Collects all channel data into a single structure
- Delegates montage calculation (e.g., bipolar, average reference)
- Updates all channels with processed data

```cpp
void EEGData::applyNotchFilter(int channelIndex, double notchFreq) {
    // ... validation ...
    SignalProcessor::notchFilter(channel.data, channel.samplingRate, notchFreq);
    emit dataChanged();
}
```
- Applies notch filter (e.g., 50/60Hz powerline interference removal)

### **8. Data Extraction**
```cpp
QVector<double> EEGData::getTimeSeries(int channelIndex, double startTime, double duration) const {
    if (channelIndex < 0 || channelIndex >= m_channels.size()) return QVector<double>();
    
    const EEGChannel &channel = m_channels[channelIndex];
    return SignalProcessor::extractTimeWindow(channel.data, channel.samplingRate, startTime, duration);
}
```
- Delegates time window extraction to `SignalProcessor`
- Returns empty vector if invalid channel index
- Used by visualization components to get display data

### **9. Statistical Methods**
```cpp
QVector<double> EEGData::channelMeans() const {
    QVector<double> means;
    means.reserve(m_channels.size());  // Pre-allocate for efficiency
    
    for (const auto &channel : m_channels) {
        means.append(SignalProcessor::mean(channel.data));
    }
    
    return means;
}
```
- Calculates mean for each channel
- Uses `reserve()` for performance optimization

```cpp
QVector<double> EEGData::channelStdDevs() const {
    QVector<double> stddevs;
    stddevs.reserve(m_channels.size());
    
    for (const auto &channel : m_channels) {
        stddevs.append(SignalProcessor::standardDeviation(channel.data));
    }
    
    return stddevs;
}
```
- Calculates standard deviation for each channel
- Delegates to `SignalProcessor` for computation

## 🔧 **Error Handling Patterns**
1. **Bounds Checking**: All methods validate `channelIndex` before proceeding
2. **Empty State Handling**: Methods return safe defaults when no data exists
3. **Signal Emission**: Always emits appropriate signals after modifications

## ⚡ **Performance Considerations**
1. **Pre-allocation**: Uses `reserve()` for vectors when size is known
2. **Delegation**: Heavy computations delegated to `SignalProcessor`
3. **In-place Modification**: Most processing modifies data in-place rather than creating copies
4. **Memory Efficiency**: Large data vectors stored contiguously in `QVector`

## 🔄 **Signal Flow**
```
User Action → MainWindow → EEGData::processMethod()
                                ↓
                    SignalProcessor::algorithm()
                                ↓
                    Update channel data/metadata
                                ↓
                    emit dataChanged()
                                ↓
            EEGChartView/MainWindow update display
```

## 📊 **Memory Management**
- **Ownership**: `EEGData` owns all `EEGChannel` objects and their data vectors
- **Cleanup**: `clear()` method properly removes all data
- **Copy Semantics**: Methods like `getTimeSeries()` return copies for safety

This implementation follows the **Facade Pattern** - providing a simplified interface to complex signal processing operations while delegating actual computations to specialized utility classes.

# EEGFileHandler.cpp - File I/O Implementation

## 📁 **File**: `src/FileHandlers/EEGFileHandler.cpp`

## 📋 **Category**: File I/O Handler

## 🎯 **Purpose**
This file provides **multi-format file loading and saving capabilities** for EEG data. It serves as the bridge between raw EEG files on disk and the application's internal `EEGData` model, supporting both EDF (European Data Format) and CSV formats with auto-detection.

## 🏗️ **Architecture Overview**

### **Namespace Pattern**
```cpp
namespace EEGFileHandler {
    // Static functions only - no class instances
    bool loadFile(const QString &filePath, EEGData &data);
    bool saveFile(const QString &filePath, const EEGData &data);
    // ... internal helper functions
}
```
- Uses a **namespace** rather than a class (utility pattern)
- All functions are **static** - no object instantiation needed
- Provides a clean, functional API for file operations

### **Supported Formats**
| Format | Extension | Description | Status |
|--------|-----------|-------------|---------|
| **EDF** | `.edf` | European Data Format - clinical standard | Full read, basic write |
| **CSV** | `.csv`, `.txt`, `.dat` | Comma/tab-separated values | Full read/write |

## 📖 **EDF File Loading (`loadEDF`)**

### **EDF Format Structure**
EDF files have a **fixed header structure**:
```
Header (256 bytes)
├── Version (8 bytes)
├── Patient ID (80 bytes)
├── Recording Info (80 bytes)
├── Start Date (8 bytes)
├── Start Time (8 bytes)
├── Header Bytes (8 bytes)
├── Reserved (44 bytes)
├── Data Records (8 bytes)
├── Record Duration (8 bytes)
└── Number of Signals (4 bytes)
```

### **Key Implementation Details**

#### **1. Header Parsing**
```cpp
// Parse basic info from fixed header section
QString version = QString::fromLatin1(header, 8);
QString patientID = QString::fromLatin1(header + 8, 80);
QString startDate = QString::fromLatin1(header + 168, 8);
QString startTime = QString::fromLatin1(header + 176, 8);
```
- Uses **fixed byte offsets** as per EDF specification
- Handles different character encoding (Latin1)
- Parses metadata for patient info, recording details

#### **2. Signal Header Parsing**
After the fixed header, each signal has:
- **Label** (16 bytes)
- **Transducer Type** (80 bytes) - skipped
- **Physical Dimension** (8 bytes) - skipped  
- **Physical Min/Max** (8 bytes each)
- **Digital Min/Max** (8 bytes each)
- **Prefiltering** (80 bytes) - skipped
- **Reserved** (32 bytes) - skipped
- **Samples per Record** (8 bytes)

```cpp
// Read signal labels
for (int i = 0; i < numSignals; ++i) {
    char label[17] = {0};
    stream.readRawData(label, 16);
    labels[i] = QString::fromLatin1(label, 16).trimmed();
}
```

#### **3. Data Reading Strategy**
```cpp
// Calculate data structure
int totalSamplesPerRecord = 0;
for (int i = 0; i < numSignals; ++i) {
    totalSamplesPerRecord += samplesPerRecord[i];
}
int bytesPerRecord = totalSamplesPerRecord * 2;  // 2 bytes per sample
```
- **Interleaved format**: Signals are stored record-by-record
- **Fixed record size**: All records have same number of samples
- **16-bit integers**: Each sample is 2 bytes (short)

#### **4. Smart Data Reading**
```cpp
// Limit reading for performance during development
numRecords = qMin(numRecords, 10000);

// Allocate raw data storage
QVector<QVector<short>> rawData(numSignals);
for (int i = 0; i < numSignals; ++i) {
    int totalSamples = samplesPerRecord[i] * numRecords;
    rawData[i].resize(totalSamples);
}
```
- **Performance optimization**: Limits to first 10,000 records during development
- **Pre-allocation**: Reserves memory before reading for efficiency
- **Batch reading**: Processes entire records at once

#### **5. Advanced Calibration & Scaling**

The most critical part of EDF loading is **calibration conversion**:

```cpp
// Dynamic scaling with three fallback strategies:
// 1. EDF calibration values (preferred)
if (qAbs(digMax[sig] - digMin[sig]) > 0.1 && qAbs(physMax[sig] - physMin[sig]) > 0.1) {
    scale = (physMax[sig] - physMin[sig]) / (digMax[sig] - digMin[sig]);
    offset = physMin[sig] - digMin[sig] * scale;
}
// 2. Data-driven dynamic scaling
else if (!rawData[sig].empty()) {
    short minVal = *std::min_element(rawData[sig].begin(), rawData[sig].end());
    short maxVal = *std::max_element(rawData[sig].begin(), rawData[sig].end());
    double range = maxVal - minVal;
    
    if (range > 0.1) {
        // Smart scaling based on data characteristics
        if (range < 100) {
            scale = 1.0;  // Already in µV range
        } else if (range > 30000) {
            scale = 200.0 / 65536.0;  // 16-bit ADC scaling
        } else {
            double targetRange = 100.0;  // Custom normalization
            scale = targetRange / range;
        }
    }
}
// 3. Ultimate fallback
else {
    scale = 1.0;
    offset = 0.0;
}
```

**Why this complex scaling?**
- Some EDF files have corrupted calibration values
- Different EEG systems use different ADC ranges
- Need to normalize to meaningful µV ranges for visualization

#### **6. Data Conversion**
```cpp
// Convert integer samples to calibrated µV values
for (int i = 0; i < numSamples; ++i) {
    channel.data[i] = rawData[sig][i] * scale + offset;
}
```

### **6. Metadata Handling**
```cpp
// Parse EDF date/time format (dd.MM.yy hh.mm.ss)
QDateTime startDT = QDateTime::fromString(
    startDate.trimmed() + " " + startTime.trimmed(), 
    "dd.MM.yy hh.mm.ss"
);
if (startDT.isValid()) {
    data.setStartDateTime(startDT);
}
```

## 📊 **CSV File Loading (`loadCSV`)**

### **CSV Format Flexibility**
```cpp
// Try multiple delimiters
QStringList values = line.split(',');
if (values.size() < 2) values = line.split('\t');
if (values.size() < 2) values = line.split(';');
```
- Supports comma, tab, or semicolon delimiters
- Auto-detects format based on successful parsing
- Skips comments (lines starting with `#`)

### **CSV Structure Assumptions**
1. First row contains **column headers**
2. First column is **time** (optional, but expected)
3. Subsequent columns are **EEG channel data**
4. Uniform sampling rate (default 250 Hz)

## 💾 **File Saving Functions**

### **EDF Saving (`saveEDF`)**
```cpp
// Simple EDF writer - creates basic compliant file
memcpy(header.data(), "0       ", 8);  // EDF version
memcpy(header.data() + 8, patient.leftJustified(80, ' ').toLatin1().constData(), 80);
// ... other header fields
```
**Note**: This is a **basic implementation** that creates valid EDF files but may not preserve all original metadata or data precision.

### **CSV Saving (`saveCSV`)**
```cpp
// Write header
stream << "Time(s)";
for (int i = 0; i < data.channelCount(); ++i) {
    stream << "," << data.channel(i).label;
}

// Write data with time column
for (int sample = 0; sample < maxSamples; ++sample) {
    double time = sample / 250.0;  // Default 250 Hz
    stream << QString::number(time, 'f', 6);
    // ... channel data
}
```
 - Creates time column based on default sampling rate
 - Uses 6 decimal places for precision
 - Handles channels with different lengths

## 🔧 **Error Handling & Robustness**

### **1. File Access Errors**
```cpp
if (!file.open(QIODevice::ReadOnly)) {
    qWarning() << "Failed to open EDF:" << filePath;
    return false;
}
```

### **2. Format Validation**
```cpp
if (!ok || numSignals <= 0) {
    qWarning() << "Invalid number of signals:" << numSignalsStr;
    file.close();
    return false;
}
```

### **3. Data Integrity Checks**
```cpp
// Check for corrupted calibration values
if (qAbs(physMax[i] - physMin[i]) < 0.1 || qAbs(digMax[i] - digMin[i]) < 0.1) {
    qWarning() << "WARNING: Corrupted calibration values for signal" << i;
}
```

### **4. Resource Management**
```cpp
file.close();  // Always close files in all code paths
```

## 📈 **Performance Optimizations**

1. **Pre-allocation**: `resize()` vectors before reading data
2. **Batch Reading**: Reads entire records rather than sample-by-sample
3. **Memory Limits**: Limits to 10,000 records during development
4. **Channel Limits**: Loads maximum 32 channels (configurable)

## 🚨 **Limitations & Notes**

### **EDF Limitations**
1. **Partial Implementation**: Full EDF+ (with annotations) not supported
2. **Calibration Assumptions**: Some EDF files may not calibrate correctly
3. **Memory Usage**: Large files (>10,000 records) are truncated
4. **Channel Count**: Limited to 32 channels for performance

### **CSV Limitations**
1. **No Metadata**: CSV doesn't preserve patient info, sampling rates
2. **Fixed Sampling**: Assumes 250 Hz if not specified
3. **Simple Format**: No support for complex CSV variations

## 🔄 **Integration with EEGData**

```cpp
// In MainWindow or other controllers:
bool success = EEGFileHandler::loadFile("recording.edf", *eegData);
if (success) {
    // Data is now populated in eegData
    updateDisplay();
}
```

## 🎯 **Key Features**
1. **Multi-format Support**: EDF and CSV with auto-detection
2. **Robust EDF Parsing**: Handles various EDF file quirks
3. **Smart Calibration**: Dynamic scaling for corrupted files
4. **Error Resilience**: Graceful handling of malformed files
5. **Performance Conscious**: Memory and speed optimizations

This file handler is **crucial** for the application's usability, providing the entry point for real EEG data while handling the complexities of clinical data formats.