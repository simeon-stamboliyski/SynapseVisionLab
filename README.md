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

# EEGFileHandler.h - File I/O Interface
📁 File: src/FileHandlers/EEGFileHandler.h
📋 Category: File I/O Handler (Interface)
🎯 Purpose
This header file provides the public interface for EEG file operations in the application. It defines a clean, simple API that abstracts away the complexities of different file formats (EDF, CSV) and provides unified loading/saving functionality.

🏗️ Architecture Design
Namespace-Based Design
cpp
namespace EEGFileHandler {
    bool loadFile(const QString &filePath, EEGData &data);
    bool saveFile(const QString &filePath, const EEGData &data);
}
Why use a namespace instead of a class?

Utility Pattern: File handlers are typically stateless utilities

Simplified API: No need to instantiate objects

Clear Intent: Communicates "these are related functions"

Avoids Singleton Overhead: No need for instance management

Forward Declaration Pattern
cpp
class EEGData;  // Forward declaration instead of #include
Reduces Compilation Dependencies: Only includes what's absolutely necessary

Faster Compilation: Header doesn't need full EEGData definition

Cleaner Separation: Interface doesn't depend on implementation details

📖 API Functions
1. loadFile() - Universal Loader
cpp
bool loadFile(const QString &filePath, EEGData &data);
Parameters:

filePath: Full path to the EEG file (supports .edf, .csv, .txt, .dat)

data: Reference to EEGData object that will be populated

Returns: true if successful, false if failed

Behavior:

Auto-detects format from file extension

Clears existing data in the EEGData object

Populates channels, metadata, and signal data

Emits signals through EEGData when complete

Supported Formats:

.edf - European Data Format (clinical standard)

.csv/.txt/.dat - Comma/tab-separated values

2. saveFile() - Universal Saver
cpp
bool saveFile(const QString &filePath, const EEGData &data);
Parameters:

filePath: Destination path (format determined by extension)

data: Const reference to EEGData object to save

Returns: true if successful, false if failed

Behavior:

Format determined by file extension

Creates new file or overwrites existing

Includes metadata where supported by format

Handles all channels in the data object

Output Formats:

.edf - Basic EDF format (limited metadata support)

Any other extension → CSV format

# MainWindow.h - Main Application Window Interface

## 📁 **File**: `src/MainWindow/MainWindow.h`

## 📋 **Category**: User Interface / Main Controller

## 🎯 **Purpose**
This is the **central controller and main window** of the EEG visualization application. It orchestrates all user interactions, manages the UI layout, and coordinates between data models, signal processing, and visualization components. Think of it as the "conductor" of the entire application.

## 🏗️ **Architecture Role**

### **MVC/MVP Pattern Role**
- **Model**: `EEGData` (data storage and processing)
- **View**: `EEGChartView` and UI widgets (visualization)
- **Controller/Presenter**: `MainWindow` (user interaction handling)

### **Central Orchestrator Responsibilities**
1. **UI Management**: Creates and manages all windows, dialogs, and widgets
2. **Event Handling**: Routes user actions to appropriate processing
3. **Data Flow Control**: Connects data loading → processing → visualization
4. **State Management**: Maintains application state (current file, settings, etc.)

## 📊 **UI Layout Structure**

```
MainWindow (QMainWindow)
├── Menu Bar
├── Tool Bars
├── Central Widget: EEGChartView (visualization area)
├── Dock Widgets (left/right panels):
│   ├── m_channelDock: Channel list & selection
│   └── m_processingDock: Signal processing controls
└── Status Bar (real-time info)
```

## 🔧 **Core Components**

### **1. Data & Visualization Core**
```cpp
EEGData *m_eegData;           // Central data model
EEGChartView *m_chartView;    // Visualization widget
```

### **2. UI Panels (Dock Widgets)**
```cpp
QDockWidget *m_channelDock;   // Left panel: channel list
QListWidget *m_channelList;   // Selectable channel list

QDockWidget *m_processingDock; // Right panel: processing tools
QWidget *m_processingWidget;   // Container for controls
```

### **3. Signal Processing Controls**
```cpp
// Filtering
QComboBox *m_filterTypeCombo;    // Filter type selection
QDoubleSpinBox *m_lowCutSpin;    // Low cutoff frequency (Hz)
QDoubleSpinBox *m_highCutSpin;   // High cutoff frequency (Hz)

// Amplitude adjustments
QDoubleSpinBox *m_gainSpin;      // Gain multiplier
QDoubleSpinBox *m_offsetSpin;    // DC offset adjustment

// Specialized filters
QDoubleSpinBox *m_notchFreqSpin; // Notch filter frequency (e.g., 50/60Hz)

// Montage selection
QComboBox *m_montageCombo;       // EEG montage (bipolar, average ref, etc.)

// Channel selection for processing
QSpinBox *m_channelSelectSpin;   // Target channel for operations
```

### **4. Display/View Controls**
```cpp
QDoubleSpinBox *m_timeStartSpin;     // Start time (seconds)
QDoubleSpinBox *m_timeDurationSpin;  // Display duration (seconds)
QDoubleSpinBox *m_verticalScaleSpin; // Amplitude scaling (µV/div)
QDoubleSpinBox *m_offsetScaleSpin;   // Channel spacing (vertical offset)
```

### **5. Menu & Toolbar Actions**
```cpp
// File operations
QAction *m_actOpen;     // Open EEG file
QAction *m_actSave;     // Save current data
QAction *m_actSaveAs;   // Save as new file
QAction *m_actExit;     // Exit application

// View controls
QAction *m_actShowGrid; // Toggle grid visibility
QAction *m_actZoomIn;   // Zoom in (time/amplitude)
QAction *m_actZoomOut;  // Zoom out
QAction *m_actPanLeft;  // Scroll left in time
QAction *m_actPanRight; // Scroll right in time

// Tools & Help
QAction *m_actStatistics; // Show data statistics
QAction *m_actAbout;      // About dialog
```

### **6. Status Bar Components**
```cpp
QLabel *m_statusFile;          // Current file name
QLabel *m_statusChannels;      // Number of channels
QLabel *m_statusDuration;      // Recording duration
QLabel *m_statusSamplingRate;  // Sampling rate info
QProgressBar *m_progressBar;   // Processing progress
```

## 🔗 **Dependencies**
- **Data Layer**: `EEGData.h` (central data model)
- **Visualization**: `EEGChartView.h` (chart display)
- **Qt Widgets**: Various UI components (QMainWindow, QDockWidget, etc.)

## 🎮 **User Interaction Flow**

### **File Operations**
```
User clicks "Open" → onFileOpen() → EEGData::loadFromFile() → 
EEGFileHandler::loadFile() → updateChannelList() → updateStatusBar()
```

### **Signal Processing**
```
User adjusts filter controls → onFilterApply() → 
EEGData::applyFilter() → m_chartView->updateDisplay() → 
updateStatusBar()
```

### **Channel Selection**
```
User selects channel in list → onChannelSelected() → 
m_chartView->setActiveChannels() → chart updates
```

## 📋 **Slot Methods (Event Handlers)**

### **File Operations**
```cpp
void onFileOpen();      // Handle File → Open
void onFileSave();      // Handle File → Save
void onFileSaveAs();    // Handle File → Save As
void onFileExit();      // Handle File → Exit
```

### **Channel Management**
```cpp
void onChannelSelected();               // Single channel selection
void onChannelListSelectionChanged();   // Multiple channel selection
```

### **Signal Processing**
```cpp
void onFilterApply();       // Apply bandpass filter
void onGainApply();         // Apply gain adjustment
void onOffsetApply();       // Apply DC offset
void onNormalizeApply();    // Normalize channel
void onDCRemoveApply();     // Remove DC component
void onNotchFilterApply();  // Apply notch filter
void onMontageApply();      // Apply EEG montage
```

### **Display Controls**
```cpp
void onTimeRangeChanged(double start, double duration);  // Time window change
void onVerticalScaleChanged(double value);               // Amplitude scaling
void onOffsetScaleChanged(double value);                 // Channel spacing
```

### **Tools & Updates**
```cpp
void onShowStatistics();    // Show data statistics dialog
void onShowAbout();         // Show about dialog
void updateStatusBar();     // Refresh status display
void updateChannelList();   // Update channel list widget
```

## 🔧 **Setup Methods**

### **UI Construction Methods**
```cpp
void createActions();        // Create menu/toolbar actions
void createMenus();          // Build menu bar structure
void createToolBars();       // Build toolbar(s)
void createDockWidgets();    // Create side panels
void createCentralWidget();  // Create main chart area
void createStatusBar();      // Create status bar
```

### **Processing Coordination**
```cpp
void applySignalProcessing(); // Coordinate multiple processing steps
```

## 🛡️ **Lifecycle Management**

### **Construction/Destruction**
```cpp
explicit MainWindow(QWidget *parent = nullptr);  // Constructor
~MainWindow();                                   // Destructor
```

### **Window Events**
```cpp
void closeEvent(QCloseEvent *event) override;    // Handle window close
```

## 📊 **State Management**

### **Application State**
```cpp
QString m_currentFilePath;  // Currently loaded file
```

### **Data State (via EEGData)**
- Loaded channels and their data
- Processing history
- Metadata (patient info, recording details)

## 🔄 **Integration Points**

### **With EEGData**
```cpp
// Data loading
m_eegData->loadFromFile(filePath);

// Signal processing
m_eegData->applyFilter(channelIndex, lowCut, highCut);

// Data access for display
QVector<double> data = m_eegData->getTimeSeries(channelIndex, start, duration);
```

### **With EEGChartView**
```cpp
// Display configuration
m_chartView->setTimeRange(startTime, duration);
m_chartView->setVerticalScale(scale);

// Data update
m_chartView->updateData(*m_eegData);
```

## 🎨 **UI Features**

### **Dockable Panels**
- **Channel List**: Left dock, shows all available EEG channels
- **Processing Tools**: Right dock, contains all signal processing controls
- **Floating/Closing**: Panels can be floated, closed, or rearranged

### **Central Visualization Area**
- Full-featured EEG chart display
- Interactive zoom/pan
- Real-time updates

### **Status Information**
- File name and path
- Number of channels loaded
- Recording duration
- Sampling rate information
- Processing progress indicator

## 🚀 **Extensibility Points**

### **Adding New Processing Features**
1. Add control widgets to `m_processingWidget`
2. Create corresponding slot method
3. Connect signals in `createActions()`/`createDockWidgets()`
4. Update `applySignalProcessing()` if needed

### **Adding New Display Features**
1. Add actions to menus/toolbars
2. Create slot methods for handling
3. Connect to `EEGChartView` functionality

## ⚡ **Performance Considerations**

### **UI Responsiveness**
- Signal/slot connections ensure non-blocking UI
- Status bar updates provide feedback during long operations
- Progress bar for extended processing tasks

### **Memory Management**
- `EEGData` owns the actual data (potentially large)
- UI components only reference data, don't duplicate
- Cleanup in destructor prevents memory leaks

## 🔍 **Debugging Support**

### **Status Indicators**
- Real-time channel count
- File information
- Processing state

### **Error Handling**
- File operation failures shown to user
- Invalid parameter validation
- Graceful handling of empty/missing data

This header file defines the complete **command center** for the EEG application, providing a comprehensive interface that connects user interactions with data processing and visualization in a cohesive, professional desktop application interface.

# MainWindow.cpp - Main Application Window Implementation

**📁 File:** `src/MainWindow/MainWindow.cpp`

**📋 Category:** User Interface / Main Controller (Implementation)

**🎯 Purpose**
This is the complete implementation of the main application window. It brings together all the UI components, connects user interactions to data processing, and provides the complete user experience for the EEG analysis application.

## 🏗️ Construction & Initialization

### Constructor Setup
```cpp
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_eegData(new EEGData(this)),        // Create data model
      m_chartView(new EEGChartView()),     // Create visualization
      m_currentFilePath("") {
    
    setWindowTitle("EEG Data Processor");
    setMinimumSize(1200, 800);            // Minimum window size
    
    createActions();      // Create menu/toolbar actions
    createMenus();        // Build menu structure
    createToolBars();     // Create toolbars
    createDockWidgets();  // Create side panels
    createCentralWidget(); // Create main chart area
    createStatusBar();    // Create status bar
    
    // Connect data model signals to UI updates
    connect(m_eegData, &EEGData::dataChanged, this, &MainWindow::updateStatusBar);
    connect(m_eegData, &EEGData::dataChanged, this, &MainWindow::updateChannelList);
    connect(m_eegData, &EEGData::channelAdded, this, &MainWindow::updateChannelList);
    connect(m_eegData, &EEGData::channelRemoved, this, &MainWindow::updateChannelList);
    
    updateStatusBar();    // Initial status update
}
Key Design Decisions:

Parent-Child Hierarchy: m_eegData has this as parent for automatic cleanup

Minimum Size: Ensures usable interface even when window is small

Signal Connections: Real-time UI updates when data changes

🎮 UI Component Creation
1. Action Creation (createActions())
Creates all menu and toolbar actions with shortcuts and connections:

cpp
// File actions with standard shortcuts
m_actOpen = new QAction("&Open...", this);
m_actOpen->setShortcut(QKeySequence::Open);  // Ctrl+O
connect(m_actOpen, &QAction::triggered, this, &MainWindow::onFileOpen);

// View actions with lambda connections for custom behavior
m_actZoomIn = new QAction("Zoom &In", this);
m_actZoomIn->setShortcut(QKeySequence::ZoomIn);  // Ctrl++
connect(m_actZoomIn, &QAction::triggered, [this]() {
    m_chartView->setTimeRange(m_chartView->currentStartTime(), 
                              m_chartView->currentDuration() * 0.8);  // Zoom to 80%
});
Key Features:

Standard keyboard shortcuts (Ctrl+S, Ctrl+O, etc.)

Status tips for user guidance

Checkable actions for toggles (grid visibility)

Lambda functions for simple inline behavior

2. Menu Creation (createMenus())
Builds the hierarchical menu structure:

cpp
QMenu *fileMenu = menuBar()->addMenu("&File");
fileMenu->addAction(m_actOpen);
fileMenu->addAction(m_actSave);
fileMenu->addAction(m_actSaveAs);
fileMenu->addSeparator();  // Visual separator
fileMenu->addAction(m_actExit);
Menu Structure:

File: Open, Save, Save As, Exit

View: Grid toggle, zoom/pan controls

Tools: Statistics dialog

Help: About dialog

3. Toolbar Creation (createToolBars())
Creates three toolbars for quick access:

File Toolbar: Open, Save, Save As

View Toolbar: Grid toggle, zoom/pan controls

Tools Toolbar: Statistics

4. Dock Widgets (createDockWidgets())
Creates the two main side panels:

Channel List Dock (Left Side)
cpp
m_channelDock = new QDockWidget("Channels", this);
m_channelDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
m_channelList = new QListWidget();
m_channelList->setSelectionMode(QAbstractItemView::ExtendedSelection);  // Multi-select
connect(m_channelList, &QListWidget::itemSelectionChanged, 
        this, &MainWindow::onChannelListSelectionChanged);
m_channelDock->setWidget(m_channelList);
addDockWidget(Qt::LeftDockWidgetArea, m_channelDock);
Processing Dock (Right Side)
A comprehensive panel with grouped controls:

Channel Selection Group:

Spin box for selecting which channel to process

Filter Group:

Filter type dropdown (Bandpass, Highpass, Lowpass, Notch)

Low/High cutoff frequency controls

Apply Filter button

Gain/Offset Group:

Gain multiplier control

DC offset adjustment

Buttons for: Apply Gain, Apply Offset, Normalize, Remove DC

Notch Filter Group:

Frequency selector (50/60 Hz for powerline interference)

Apply button

Montage Group:

Montage type dropdown (Bipolar, Average Reference, Laplacian)

Apply button

Display Controls Group:

Start time and duration controls

Vertical scaling (amplitude)

Offset scaling (channel spacing)

Update Display button

5. Central Widget (createCentralWidget())
cpp
void MainWindow::createCentralWidget() {
    m_chartView->setEEGData(m_eegData);  // Connect chart to data
    connect(m_chartView, &EEGChartView::timeRangeChanged,
            this, &MainWindow::onTimeRangeChanged);  // Sync controls
    setCentralWidget(m_chartView);  // Make chart the main area
}
6. Status Bar (createStatusBar())
Creates a multi-section status bar:

cpp
m_statusFile = new QLabel("No file loaded");
m_statusChannels = new QLabel("Channels: 0");
m_statusDuration = new QLabel("Duration: 0.0 s");
m_statusSamplingRate = new QLabel("Rate: 0.0 Hz");
m_progressBar = new QProgressBar();
m_progressBar->setVisible(false);  // Hidden until needed

statusBar()->addWidget(m_statusFile);      // Left-aligned items
statusBar()->addWidget(m_statusChannels);
statusBar()->addWidget(m_statusDuration);
statusBar()->addWidget(m_statusSamplingRate);
statusBar()->addPermanentWidget(m_progressBar);  // Right-aligned
📁 File Operations
Opening Files (onFileOpen())
cpp
void MainWindow::onFileOpen() {
    // Show file dialog with supported formats
    QString filePath = QFileDialog::getOpenFileName(
        this, "Open EEG Data File", "",
        "EEG Files (*.edf *.csv *.txt *.dat);;All Files (*.*)");
    
    if (filePath.isEmpty()) return;  // User cancelled
    
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    
    // Simulate progress (in real app, would update from loading thread)
    for (int i = 0; i <= 100; i += 10) {
        m_progressBar->setValue(i);
        QCoreApplication::processEvents();  // Keep UI responsive
    }
    
    if (m_eegData->loadFromFile(filePath)) {
        m_currentFilePath = filePath;
        m_chartView->updateChart();  // Refresh display
        
        // Update window title with filename
        setWindowTitle(QString("EEG Data Processor - %1").arg(QFileInfo(filePath).fileName()));
        
        // Update UI controls based on loaded data
        int channelCount = m_eegData->channelCount();
        m_channelSelectSpin->setRange(0, qMax(0, channelCount - 1));
        
        double duration = m_eegData->duration();
        m_timeDurationSpin->setRange(0.1, qMin(60.0, duration));
        m_timeDurationSpin->setValue(qMin(10.0, duration));
        m_chartView->setTimeRange(0, qMin(10.0, duration));
        
        // Show success message
        QMessageBox::information(this, "Success", 
                                QString("Loaded %1 channels with %2 seconds of data")
                                .arg(channelCount).arg(duration, 0, 'f', 2));
    } else {
        QMessageBox::warning(this, "Error", "Failed to load EEG data file");
    }
    
    m_progressBar->setVisible(false);
}
Key Features:

Progress indicator during loading

Automatic UI updates after loading

Range validation for controls

Informative success/error messages

Saving Files (onFileSave() / onFileSaveAs())
cpp
void MainWindow::onFileSave() {
    if (m_currentFilePath.isEmpty()) {
        onFileSaveAs();  // First save -> show Save As dialog
        return;
    }
    
    if (m_eegData->saveToFile(m_currentFilePath)) {
        QMessageBox::information(this, "Success", "Data saved successfully");
    } else {
        QMessageBox::warning(this, "Error", "Failed to save data");
    }
}
Save As Dialog:

cpp
void MainWindow::onFileSaveAs() {
    QString filePath = QFileDialog::getSaveFileName(
        this, "Save EEG Data File", m_currentFilePath,
        "EDF Files (*.edf);;CSV Files (*.csv);;Text Files (*.txt);;All Files (*.*)");
    
    if (filePath.isEmpty()) return;  // User cancelled
    
    m_currentFilePath = filePath;
    onFileSave();  // Save to new location
}
⚡ Signal Processing Handlers
Filter Application (onFilterApply())
cpp
void MainWindow::onFilterApply() {
    int channel = m_channelSelectSpin->value();
    double lowCut = m_lowCutSpin->value();
    double highCut = m_highCutSpin->value();
    
    if (channel < 0 || channel >= m_eegData->channelCount()) {
        QMessageBox::warning(this, "Error", "Invalid channel selection");
        return;
    }
    
    m_eegData->applyFilter(channel, lowCut, highCut);
    m_chartView->updateChart();  // Refresh display
}
Pattern for all processing functions:

Get parameters from UI controls

Validate channel selection

Call corresponding EEGData method

Update chart display

Montage Application (onMontageApply())
cpp
void MainWindow::onMontageApply() {
    int montageIndex = m_montageCombo->currentIndex();
    
    switch (montageIndex) {
    case 0:
        m_eegData->applyMontage(SignalProcessor::MontageBipolar);
        break;
    case 1:
        m_eegData->applyMontage(SignalProcessor::MontageAverageReference);
        break;
    case 2:
        m_eegData->applyMontage(SignalProcessor::MontageLaplacian);
        break;
    }
    
    m_chartView->updateChart();
    updateChannelList();  // Montage may change channel data
}
📊 Data Visualization & UI Sync
Channel Selection Management
cpp
void MainWindow::onChannelListSelectionChanged() {
    QVector<int> selectedChannels;
    for (auto item : m_channelList->selectedItems()) {
        selectedChannels.append(m_channelList->row(item));
    }
    m_chartView->setVisibleChannels(selectedChannels);  // Update display
}
Display Control Synchronization
cpp
void MainWindow::onTimeRangeChanged(double start, double duration) {
    m_timeStartSpin->setValue(start);
    m_timeDurationSpin->setValue(duration);
}

// Called when user changes controls, updates chart
connect(updateDisplayButton, &QPushButton::clicked, [this]() {
    m_chartView->setTimeRange(m_timeStartSpin->value(), m_timeDurationSpin->value());
    m_chartView->setVerticalScale(m_verticalScaleSpin->value());
    m_chartView->setOffsetScale(m_offsetScaleSpin->value());
});
📈 Statistics Dialog (onShowStatistics())
Creates a comprehensive statistics table:

cpp
void MainWindow::onShowStatistics() {
    if (m_eegData->isEmpty()) {
        QMessageBox::information(this, "Statistics", "No data loaded");
        return;
    }
    
    QDialog statsDialog(this);
    statsDialog.setWindowTitle("Channel Statistics");
    statsDialog.resize(600, 400);
    
    QVBoxLayout *layout = new QVBoxLayout(&statsDialog);
    
    QTableWidget *table = new QTableWidget();
    table->setColumnCount(8);
    table->setHorizontalHeaderLabels({"Channel", "Label", "Samples", 
                                      "Rate (Hz)", "Mean (μV)", "StdDev (μV)", 
                                      "Min (μV)", "Max (μV)"});
    
    // Populate table with channel data
    auto means = m_eegData->channelMeans();
    auto stddevs = m_eegData->channelStdDevs();
    
    for (int i = 0; i < m_eegData->channelCount(); ++i) {
        const EEGChannel &channel = m_eegData->channel(i);
        
        table->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        table->setItem(i, 1, new QTableWidgetItem(channel.label));
        // ... more columns
    }
    
    table->resizeColumnsToContents();
    layout->addWidget(table);
    
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, &statsDialog, &QDialog::reject);
    layout->addWidget(buttons);
    
    statsDialog.exec();
}
🔄 UI State Management
Status Bar Updates (updateStatusBar())
cpp
void MainWindow::updateStatusBar() {
    if (m_eegData->isEmpty()) {
        m_statusFile->setText("No file loaded");
        m_statusChannels->setText("Channels: 0");
        m_statusDuration->setText("Duration: 0.0 s");
        m_statusSamplingRate->setText("Rate: 0.0 Hz");
    } else {
        QString fileName = m_currentFilePath.isEmpty() ? "Untitled" 
                          : QFileInfo(m_currentFilePath).fileName();
        m_statusFile->setText(fileName);
        m_statusChannels->setText(QString("Channels: %1").arg(m_eegData->channelCount()));
        m_statusDuration->setText(QString("Duration: %1 s").arg(m_eegData->duration(), 0, 'f', 2));
        m_statusSamplingRate->setText(QString("Rate: %1 Hz").arg(m_eegData->maxSamplingRate(), 0, 'f', 1));
    }
}
Channel List Updates (updateChannelList())
cpp
void MainWindow::updateChannelList() {
    m_channelList->clear();
    
    for (int i = 0; i < m_eegData->channelCount(); ++i) {
        const EEGChannel &channel = m_eegData->channel(i);
        QString itemText = QString("%1: %2 (%3 samples, %4 Hz)")
                          .arg(i + 1, 2)
                          .arg(channel.label)
                          .arg(channel.data.size())
                          .arg(channel.samplingRate, 0, 'f', 1);
        
        QListWidgetItem *item = new QListWidgetItem(itemText);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);  // All channels visible by default
        m_channelList->addItem(item);
    }
    
    // Update processing channel selector
    int channelCount = m_eegData->channelCount();
    m_channelSelectSpin->setRange(0, qMax(0, channelCount - 1));
}
🛡️ Application Lifecycle
Window Close Handling (closeEvent())
cpp
void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_eegData && !m_eegData->isEmpty()) {
        // Prompt to save unsaved changes
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Save Changes",
            "Do you want to save changes before exiting?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        
        if (reply == QMessageBox::Save) {
            onFileSave();
            event->accept();
        } else if (reply == QMessageBox::Discard) {
            event->accept();
        } else {
            event->ignore();  // Cancel close
        }
    } else {
        event->accept();  // No data to save
    }
}
Destructor
cpp
MainWindow::~MainWindow() {
    // All QObjects are deleted automatically by parent-child hierarchy
    // No manual cleanup needed
}

# SignalProcessor.h - Signal Processing Utilities

## 📁 **File**: `src/Utils/SignalProcessor.h`

## 📋 **Category**: Signal Processing / Algorithms

## 🎯 **Purpose**
This header-only library provides **real-time signal processing algorithms** specifically designed for EEG data analysis. It contains mathematical and digital signal processing functions that operate directly on EEG signal data. This is the "computational engine" of the application.

## 🏗️ **Architecture Design**

### **Namespace-Based Utility Library**
```cpp
namespace SignalProcessor {
    // All functions are inline for performance
    // No class instances - pure functional programming
}
```

**Key Design Decisions:**
1. **Header-only**: No separate compilation unit needed
2. **Inline functions**: Optimized for performance, avoids function call overhead
3. **Functional style**: Stateless, pure functions that transform data
4. **Template-ready**: Could be templated for different data types in future

### **Performance-Oriented Design**
- **In-place operations**: Most functions modify data in-place to avoid copies
- **Minimal allocations**: Prefers modifying existing vectors over creating new ones
- **Algorithmic efficiency**: Uses standard library algorithms where possible

## 📊 **Function Categories**

### **1. Basic Signal Operations**

#### **`applyGain()` - Amplitude Scaling**
```cpp
inline void applyGain(QVector<double> &data, double gain) {
    for (auto &val : data) {
        val *= gain;  // Simple element-wise multiplication
    }
}
```
**Purpose**: Scale signal amplitude by a constant factor
**Use Case**: Adjust signal sensitivity, normalize across channels
**Complexity**: O(n) - linear time

#### **`applyOffset()` - DC Offset Adjustment**
```cpp
inline void applyOffset(QVector<double> &data, double offset) {
    for (auto &val : data) {
        val += offset;  // Add constant to all samples
    }
}
```
**Purpose**: Shift entire signal up or down
**Use Case**: Remove baseline drift, center signal around zero
**Complexity**: O(n)

#### **`normalize()` - Min-Max Normalization**
```cpp
inline void normalize(QVector<double> &data, double minVal = 0.0, double maxVal = 1.0) {
    auto minMax = std::minmax_element(data.begin(), data.end());  // Find range
    double currentMin = *minMax.first;
    double currentMax = *minMax.second;
    double range = currentMax - currentMin;
    
    if (range > 0) {
        double targetRange = maxVal - minVal;
        for (auto &val : data) {
            // Linear scaling: new = minVal + ((old - min)/(max-min)) * targetRange
            val = minVal + ((val - currentMin) / range) * targetRange;
        }
    }
}
```
**Purpose**: Scale data to specified range (default 0.0-1.0)
**Use Case**: Standardize signals for comparison, prepare for machine learning
**Complexity**: O(n) - two passes (find min/max, then scale)

### **2. Filtering Operations**

#### **`removeDC()` - DC Component Removal**
```cpp
inline void removeDC(QVector<double> &data) {
    double sum = std::accumulate(data.begin(), data.end(), 0.0);
    double mean = sum / data.size();  // Calculate DC offset
    
    for (auto &val : data) {
        val -= mean;  // Subtract mean from all samples
    }
}
```
**Purpose**: Remove DC (0Hz) component from signal
**Use Case**: EEG signals often have DC offset that doesn't contain useful information
**Note**: This is essentially a high-pass filter with cutoff at 0Hz

#### **`movingAverage()` - Simple Low-Pass Filter**
```cpp
inline void movingAverage(QVector<double> &data, int windowSize) {
    QVector<double> result = data;  // Copy for safe in-place operation
    
    for (int i = halfWindow; i < data.size() - halfWindow; ++i) {
        double sum = 0.0;
        for (int j = i - halfWindow; j <= i + halfWindow; ++j) {
            sum += data[j];  // Sum samples in window
        }
        result[i] = sum / windowSize;  // Average = low-pass filtered
    }
    
    data = result;
}
```
**Purpose**: Smooth signal by averaging nearby samples
**Use Case**: Reduce high-frequency noise, act as anti-aliasing filter
**Characteristics**: 
- Cutoff frequency ≈ sampling_rate / window_size
- Linear phase (no distortion)
- Poor frequency response (sinc function)

#### **`bandpassFilter()` - Combined High/Low Pass**
```cpp
inline void bandpassFilter(QVector<double> &data, double samplingRate, 
                          double lowCutHz, double highCutHz) {
    // Remove DC (acts as high-pass with cutoff at ~0Hz)
    removeDC(data);
    
    // Apply moving average for low-pass
    if (highCutHz > 0 && highCutHz < samplingRate / 2) {
        int windowSize = static_cast<int>(samplingRate / highCutHz);
        windowSize = std::max(3, std::min(51, windowSize));  // Clamp to reasonable range
        movingAverage(data, windowSize);
    }
}
```
**Purpose**: Filter signal to specific frequency range
**Use Case**: EEG analysis typically uses 0.5-45Hz bandpass
**Limitations**: Simple implementation - real applications would use Butterworth or Chebyshev filters

#### **`notchFilter()` - Powerline Interference Removal**
```cpp
inline void notchFilter(QVector<double> &data, double samplingRate, 
                       double notchFreq = 50.0) {
    // Simple IIR notch filter (2nd order)
    double w0 = 2.0 * M_PI * notchFreq / samplingRate;  // Normalized frequency
    double alpha = sin(w0) / 2.0;  // Bandwidth control
    double b0 = 1.0;
    double b1 = -2.0 * cos(w0);
    double b2 = 1.0;
    double a0 = 1.0 + alpha;
    double a1 = -2.0 * cos(w0);
    double a2 = 1.0 - alpha;
    
    // Direct Form II implementation
    for (int i = 2; i < data.size(); ++i) {
        y[i] = (b0 * data[i] + b1 * data[i-1] + b2 * data[i-2] 
                - a1 * y[i-1] - a2 * y[i-2]) / a0;
    }
}
```
**Purpose**: Remove specific frequency (typically 50Hz or 60Hz powerline noise)
**Algorithm**: 2nd order IIR notch filter
**Transfer Function**: H(z) = (1 - 2cos(w0)z⁻¹ + z⁻²) / (1 - 2αcos(w0)z⁻¹ + (1-α)z⁻²)
**Characteristics**: 
- Sharp notch at specified frequency
- Minimal effect on other frequencies
- Phase distortion near notch (acceptable for EEG)

### **3. Statistical Functions**

#### **`mean()` / `standardDeviation()`**
```cpp
inline double mean(const QVector<double> &data) {
    return std::accumulate(data.begin(), data.end(), 0.0) / data.size();
}

inline double standardDeviation(const QVector<double> &data) {
    double m = mean(data);
    double variance = 0.0;
    for (double val : data) {
        double diff = val - m;
        variance += diff * diff;  // Sum of squares
    }
    variance /= data.size();
    return std::sqrt(variance);  // Standard deviation
}
```
**Purpose**: Calculate basic statistical measures
**Use Case**: Signal quality assessment, normalization, feature extraction

#### **`extractTimeWindow()` - Signal Segmentation**
```cpp
inline QVector<double> extractTimeWindow(const QVector<double> &data, 
                                        double samplingRate,
                                        double startTime, double duration) {
    int startSample = static_cast<int>(startTime * samplingRate);
    int endSample = static_cast<int>((startTime + duration) * samplingRate);
    
    // Clamp to valid range
    startSample = std::max(0, startSample);
    endSample = std::min(data.size() - 1, endSample);
    
    QVector<double> result;
    result.reserve(endSample - startSample + 1);  // Pre-allocate
    
    for (int i = startSample; i <= endSample; ++i) {
        result.append(data[i]);  // Copy selected samples
    }
    
    return result;
}
```
**Purpose**: Extract a segment of the signal for display/analysis
**Use Case**: Visualization of specific time windows, event-related analysis

### **4. EEG-Specific Processing**

#### **Montage System**
```cpp
enum MontageType {
    MontageBipolar,           // Adjacent channel differences
    MontageAverageReference,  // Reference to average of all channels
    MontageLaplacian,         // Local average reference (not implemented)
    MontageLongitudinalBipolar // Chain of differences (not implemented)
};
```

#### **`applyAverageReference()` - Common Average Reference**
```cpp
inline void applyAverageReference(QVector<QVector<double>> &allChannelData) {
    // Calculate average across all channels at each time point
    QVector<double> average(numSamples, 0.0);
    for (int sample = 0; sample < numSamples; ++sample) {
        double sum = 0.0;
        for (int ch = 0; ch < allChannelData.size(); ++ch) {
            sum += allChannelData[ch][sample];
        }
        average[sample] = sum / allChannelData.size();
    }
    
    // Subtract average from each channel
    for (int ch = 0; ch < allChannelData.size(); ++ch) {
        for (int sample = 0; sample < numSamples; ++sample) {
            allChannelData[ch][sample] -= average[sample];
        }
    }
}
```
**Purpose**: Rereference EEG signals to the average of all channels
**Use Case**: Remove common noise, standardize for comparison
**EEG Significance**: Helps eliminate artifacts common to all electrodes

#### **`applyMontage()` - EEG Montage Application**
```cpp
inline void applyMontage(QVector<QVector<double>> &allChannelData, 
                        const QVector<QString> &channelLabels,
                        MontageType montage) {
    switch (montage) {
        case MontageAverageReference:
            applyAverageReference(allChannelData);
            break;
        case MontageBipolar:
            // Calculate differences between adjacent channels
            for (int i = 0; i < allChannelData.size() - 1; ++i) {
                for (int j = 0; j < diff.size(); ++j) {
                    diff[j] = allChannelData[i][j] - allChannelData[i+1][j];
                }
            }
            break;
        // Other montages would be implemented similarly
    }
}
```
**Purpose**: Apply different EEG display/reference configurations
**Clinical Importance**: Different montages highlight different aspects of EEG signals

### **5. Frequency Analysis**

#### **`powerSpectrum()` - Discrete Fourier Transform**
```cpp
inline QVector<double> powerSpectrum(const QVector<double> &data, 
                                    double samplingRate) {
    // Simple DFT implementation (not FFT - O(n²) complexity)
    for (int k = 0; k <= N/2; ++k) {  // Only need first half (Nyquist)
        double real = 0.0;
        double imag = 0.0;
        
        for (int n = 0; n < N; ++n) {
            double angle = 2.0 * M_PI * k * n / N;
            real += data[n] * cos(angle);  // Real component
            imag -= data[n] * sin(angle);  // Imaginary component
        }
        
        spectrum[k] = sqrt(real*real + imag*imag) / N;  // Amplitude
    }
    
    return spectrum;
}
```
**Purpose**: Calculate frequency content of signal
**Limitations**: Uses naive DFT - for production, would use FFTW or similar
**Note**: Returns single-sided spectrum up to Nyquist frequency

#### **`calculateBandPower()` - EEG Frequency Bands**
```cpp
struct BandPower {
    double delta;    // 0.5-4 Hz   (deep sleep, infants)
    double theta;    // 4-8 Hz     (drowsiness, meditation)
    double alpha;    // 8-13 Hz    (relaxed, eyes closed)
    double beta;     // 13-30 Hz   (active thinking, focus)
    double gamma;    // 30-100 Hz  (cognitive processing)
};
```
**Purpose**: Calculate power in standard EEG frequency bands
**Use Case**: Quantitative EEG (qEEG), brain state analysis, sleep staging
**Clinical Relevance**: Different bands associated with different brain states

## ⚡ **Performance Characteristics**

| Function | Time Complexity | Space Complexity | Notes |
|----------|----------------|------------------|-------|
| `applyGain`/`applyOffset` | O(n) | O(1) | In-place, very fast |
| `normalize` | O(n) | O(1) | Two passes (find min/max, then scale) |
| `removeDC` | O(n) | O(1) | One accumulation pass |
| `movingAverage` | O(n × w) | O(n) | w = window size, can be slow for large windows |
| `notchFilter` | O(n) | O(n) | Needs output buffer for IIR |
| `powerSpectrum` | O(n²) | O(n) | Naive DFT - very slow for large n |

## 🔧 **Algorithmic Details**

### **Filter Design Considerations**

#### **Moving Average Filter**
- **Cutoff Frequency**: \( f_c ≈ \frac{f_s}{N} \) where N = window size
- **Frequency Response**: Sinc function with nulls at multiples of \( f_s/N \)
- **Advantages**: Linear phase, simple implementation
- **Disadvantages**: Poor stopband attenuation

#### **Notch Filter Design**
Based on standard 2nd order digital notch filter:
- **Center Frequency**: \( f_0 \) (50Hz or 60Hz)
- **Bandwidth**: Controlled by α parameter
- **Q Factor**: \( Q = \frac{f_0}{BW} = \frac{1}{2α} \)

### **Numerical Stability**
- Uses `double` precision throughout
- No division by zero checks in some functions (caller's responsibility)
- Accumulation uses `std::accumulate` for better floating-point stability

## 🎯 **EEG-Specific Considerations**

### **Typical EEG Parameters**
- **Sampling Rate**: 250-1000 Hz (clinical), up to 10kHz (research)
- **Frequency Range of Interest**: 0.5-45 Hz
- **Notch Frequencies**: 50 Hz (Europe), 60 Hz (Americas)
- **Amplitude Range**: Typically ±100 µV

### **Common Processing Pipeline**
```
Raw EEG → Remove DC → Bandpass (0.5-45Hz) → Notch (50/60Hz) → Montage → Analysis
```

## 🚀 **Extensibility**

### **Adding New Filters**
```cpp
// Example: Butterworth filter implementation
inline void butterworthFilter(QVector<double> &data, double samplingRate,
                             double cutoffHz, int order) {
    // Implementation would go here
}
```

### **Adding Feature Extraction**
```cpp
// Example: Hjorth parameters
struct HjorthParameters {
    double activity;    // Variance
    double mobility;    // sqrt(variance of derivative / variance)
    double complexity;  // Similar for second derivative
};

inline HjorthParameters calculateHjorth(const QVector<double> &data) {
    // Implementation
}
```

## ⚠️ **Limitations & Notes**

### **Current Limitations**
1. **Simple Algorithms**: Production systems would use more sophisticated filters
2. **No Windowing**: FFT doesn't use window functions (Hamming, Hann, etc.)
3. **No Multithreading**: All operations are single-threaded
4. **Memory Intensive**: Some functions create temporary copies

### **For Production Use Consider:**
1. Replace DFT with FFTW or similar library
2. Implement proper IIR/FIR filter design
3. Add windowing functions for spectral analysis
4. Consider GPU acceleration for large datasets

## 💡 **Usage Examples**

```cpp
// Basic signal processing
QVector<double> eegSignal = ...;
SignalProcessor::removeDC(eegSignal);
SignalProcessor::bandpassFilter(eegSignal, 250.0, 0.5, 45.0);
SignalProcessor::notchFilter(eegSignal, 250.0, 50.0);

// Statistical analysis
double mean = SignalProcessor::mean(eegSignal);
double stdDev = SignalProcessor::standardDeviation(eegSignal);
auto bandPower = SignalProcessor::calculateBandPower(eegSignal, 250.0);

// Extract segment for display
auto segment = SignalProcessor::extractTimeWindow(eegSignal, 250.0, 10.0, 5.0);
```

This signal processing library provides the **mathematical foundation** for EEG analysis, implementing both general-purpose signal operations and EEG-specific algorithms essential for clinical and research applications.

# EEGChartView - EEG Data Visualization Component

**📁 Files:**
- `src/Visualization/EEGChartView.h` (Interface)
- `src/Visualization/EEGChartView.cpp` (Implementation)

**📋 Category:** Visualization / Chart Display

**🎯 Purpose**
This is the primary visualization component that displays EEG signals in a multi-channel, time-series chart format. It provides interactive visualization of EEG data with real-time updates, zooming, panning, and channel management capabilities. Think of it as the "oscilloscope" of your EEG application.

## 🏗️ Architecture Overview

### Inheritance Hierarchy
QWidget
└── QChartView (Qt Charts)
└── EEGChartView (Custom)

text

### Design Patterns
- **Observer Pattern**: Listens to EEGData changes for real-time updates
- **Strategy Pattern**: Different interaction modes (pan, zoom, select)
- **Composite Pattern**: Multiple channel series combined into single chart

## 📊 Key Components

### Core Data & Chart
```cpp
EEGData *m_eegData;           // Reference to data model
QChart *m_chart;              // Qt Charts container
QVector<QLineSeries*> m_series; // Individual channel plot series
QVector<int> m_visibleChannels; // Which channels to display
Display Parameters
cpp
double m_startTime;      // Current view start time (seconds)
double m_duration;       // Current view duration (seconds)
double m_verticalScale;  // Amplitude scaling factor
double m_offsetScale;    // Vertical spacing between channels (µV)
bool m_showGrid;         // Grid visibility toggle
Interaction State
cpp
QPoint m_lastMousePos;   // For tracking mouse movements
bool m_isPanning;        // Pan mode active
bool m_isZooming;        // Zoom mode active
🎨 Visualization Features
Multi-Channel Display
Stacked Layout: Channels displayed with vertical offset for clarity

Color Coding: Each channel has distinct color for easy identification

Dynamic Scaling: Automatic Y-axis range based on visible channels

Legend: Shows channel labels with corresponding colors

Time-Series Representation
X-axis: Time in seconds

Y-axis: Amplitude in microvolts (µV)

Grid Lines: Optional for precise measurements

Downsampling: For performance with large datasets

Performance Optimizations
cpp
// In updateChart():
int step = qMax(1, (endSample - startSample) / 2000);  // Downsample to ~2000 points
Limits to ~2000 data points per series for smooth rendering

Dynamic sampling based on visible time window

Only renders visible channels

🔧 Implementation Details
Chart Creation (createChart())
cpp
void EEGChartView::createChart() {
    m_chart->setTitle("EEG Signals");
    m_chart->setAnimationOptions(QChart::NoAnimation);  // Performance
    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignBottom);
    
    // Create and configure axes
    QValueAxis *axisX = new QValueAxis;
    axisX->setTitleText("Time (s)");
    axisX->setLabelFormat("%.2f");
    axisX->setGridLineVisible(m_showGrid);
    
    QValueAxis *axisY = new QValueAxis;
    axisY->setTitleText("Amplitude (μV)");
    axisY->setLabelFormat("%.0f");
    axisY->setGridLineVisible(m_showGrid);
    
    // Attach axes to chart
    m_chart->addAxis(axisX, Qt::AlignBottom);
    m_chart->addAxis(axisY, Qt::AlignLeft);
}
Data Update Pipeline (updateChart())
cpp
void EEGChartView::updateChart() {
    // 1. Clear existing series
    for (auto series : m_series) {
        m_chart->removeSeries(series);
        delete series;
    }
    m_series.clear();
    
    // 2. Create new series for each visible channel
    for (int i = 0; i < m_visibleChannels.size(); ++i) {
        int channelIndex = m_visibleChannels[i];
        const EEGChannel &channel = m_eegData->channel(channelIndex);
        
        // 3. Create Qt Charts series
        QLineSeries *series = new QLineSeries();
        series->setName(channel.label);
        series->setPen(QPen(getChannelColor(i), 1));
        
        // 4. Calculate visible time window
        int startSample = static_cast<int>(m_startTime * channel.samplingRate);
        int endSample = static_cast<int>((m_startTime + m_duration) * channel.samplingRate);
        
        // 5. Add data points with downsampling
        int step = qMax(1, (endSample - startSample) / 2000);
        double offset = i * m_offsetScale;  // Vertical stacking
        
        for (int s = startSample; s <= endSample; s += step) {
            double time = s / channel.samplingRate;
            double value = channel.data[s] * m_verticalScale + offset;
            series->append(time, value);
        }
        
        // 6. Add to chart and attach axes
        m_series.append(series);
        m_chart->addSeries(series);
        series->attachAxis(axisX);
        series->attachAxis(axisY);
    }
    
    // 7. Update axis ranges
    axisX->setRange(m_startTime, m_startTime + m_duration);
    
    // Calculate Y range based on stacked channels
    if (!m_visibleChannels.isEmpty()) {
        double yMin = 0;
        double yMax = m_visibleChannels.size() * m_offsetScale;
        axisY->setRange(yMin - m_offsetScale * 0.5, yMax + m_offsetScale * 0.5);
    }
    
    m_chart->update();
}
Channel Color Management
cpp
QColor EEGChartView::getChannelColor(int index) const {
    static const QVector<QColor> colors = {
        Qt::blue, Qt::red, Qt::green, Qt::magenta,
        Qt::darkCyan, Qt::darkYellow, Qt::darkMagenta, Qt::darkBlue,
        Qt::darkRed, Qt::darkGreen, Qt::cyan, Qt::yellow
    };
    return colors[index % colors.size()];  // Cycle through colors
}
Event Handling Overrides
Wheel Event - Zooming
cpp
void EEGChartView::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        // Vertical zoom (amplitude)
        double factor = event->angleDelta().y() > 0 ? 1.1 : 0.9;
        m_verticalScale *= factor;
        updateChart();
    } else if (event->modifiers() & Qt::ShiftModifier) {
        // Channel spacing adjustment
        double factor = event->angleDelta().y() > 0 ? 1.1 : 0.9;
        m_offsetScale *= factor;
        updateChart();
    } else {
        // Horizontal zoom (time)
        double factor = event->angleDelta().y() > 0 ? 0.8 : 1.2;
        m_duration *= factor;
        m_duration = qMax(0.1, qMin(60.0, m_duration));  // Limit: 0.1-60 seconds
        updateChart();
        emit timeRangeChanged(m_startTime, m_duration);
    }
    event->accept();
}
Mouse Events - Panning & Zooming
cpp
void EEGChartView::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton || 
        (event->button() == Qt::LeftButton && event->modifiers() & Qt::ShiftModifier)) {
        m_isPanning = true;
        m_lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);  // Visual feedback
    } else if (event->button() == Qt::RightButton) {
        m_isZooming = true;
        m_lastMousePos = event->pos();
        setCursor(Qt::SizeAllCursor);  // Visual feedback
    } else {
        QChartView::mousePressEvent(event);  // Default behavior (rubber band)
    }
}
🔗 Integration Points
With EEGData
cpp
// Set data source
void EEGChartView::setEEGData(EEGData *data) {
    m_eegData = data;
    if (data) {
        connect(data, &EEGData::dataChanged, this, &EEGChartView::updateChart);
    }
    updateChart();
}
With MainWindow
cpp
// In MainWindow.cpp:
m_chartView->setEEGData(m_eegData);
connect(m_chartView, &EEGChartView::timeRangeChanged,
        this, &MainWindow::onTimeRangeChanged);
Channel Visibility Control
cpp
// Show only selected channels
void EEGChartView::setVisibleChannels(const QVector<int> &channels) {
    m_visibleChannels = channels;
    updateChart();
}

// Called from channel list selection in MainWindow
void MainWindow::onChannelListSelectionChanged() {
    QVector<int> selectedChannels;
    for (auto item : m_channelList->selectedItems()) {
        selectedChannels.append(m_channelList->row(item));
    }
    m_chartView->setVisibleChannels(selectedChannels);
}
⚡ Performance Optimizations
1. Downsampling Strategy
cpp
// Calculate optimal step size for ~2000 points
int step = qMax(1, (endSample - startSample) / 2000);
Goal: ~2000 points per channel for smooth rendering

Dynamic: Adjusts based on visible time window

Balance: Enough detail for visualization, not too many for performance

2. Memory Management
Series cleanup: Proper deletion of old series before creating new ones

Point limiting: Prevents Qt Charts from storing excessive points

Color caching: Static color array avoids dynamic allocation

3. Rendering Optimizations
cpp
setRenderHint(QPainter::Antialiasing);  // Smooth lines
m_chart->setAnimationOptions(QChart::NoAnimation);  // Disable animations for performance
🎯 Clinical EEG Display Features
Standard EEG Display Conventions
Time Scale: Typically 10-30 seconds per screen (adjustable)

Amplitude Scale: Typically 50-100 µV per centimeter (adjustable)

Channel Spacing: Enough to prevent overlap, typically 50-200 µV

Grid: 1-second major divisions, 0.2-second minor divisions (optional)

Multi-Channel Stacking
text
Channel 8: ──────────────── (offset: 7 × offsetScale)
Channel 7: ──────────────── (offset: 6 × offsetScale)
Channel 6: ──────────────── (offset: 5 × offsetScale)
Channel 5: ──────────────── (offset: 4 × offsetScale)
Channel 4: ──────────────── (offset: 3 × offsetScale)
Channel 3: ──────────────── (offset: 2 × offsetScale)
Channel 2: ──────────────── (offset: 1 × offsetScale)
Channel 1: ──────────────── (offset: 0 × offsetScale)
Time (s):  0    1    2    3    4
main.cpp - Application Entry Point
📁 File: src/main.cpp

📋 Category: Application Bootstrap / Entry Point

🎯 Purpose
This is the application entry point - the first code that executes when the EEG application starts. It initializes the Qt framework, creates the main application window, and starts the event loop. Think of it as the "starter motor" for the entire application.