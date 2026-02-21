#include "MainWindow.h"
#include "../NotchPreviewDialog/NotchPreviewDialog.h"
#include "qcustomplot.h"
#include <QCoreApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QCheckBox>
#include <QHeaderView>
#include <QTableWidget>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPlainTextEdit>
#include <QDateTime>
#include <QCloseEvent>
#include <cmath>
#include <numeric>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_eegData(new EEGData(this)),
      m_chartView(new EEGChartView()),
      m_currentFilePath("") {
    
    setWindowTitle("EEG Data Processor");
    setMinimumSize(1200, 800);
    
    createActions();
    createMenus();
    createToolBars();
    createDockWidgets();
    createCentralWidget();
    createStatusBar();
    
    connect(m_eegData, &EEGData::dataChanged, this, &MainWindow::updateStatusBar);
    connect(m_eegData, &EEGData::dataChanged, this, &MainWindow::updateChannelList);
    connect(m_eegData, &EEGData::channelAdded, this, &MainWindow::updateChannelList);
    connect(m_eegData, &EEGData::channelRemoved, this, &MainWindow::updateChannelList);
    connect(m_eegData, &EEGData::channelCountChanged, [this](int newCount) {
        // Update channel list
        updateChannelList();
        
        // Reset visible channels to all available channels
        QVector<int> allChannels;
        for (int i = 0; i < newCount; ++i) {
            allChannels.append(i);
        }
        m_chartView->setVisibleChannels(allChannels);
        
        // Update spin box range
        m_channelSelectSpin->setRange(-1, newCount - 1);
    });

    connect(m_chartView, &EEGChartView::visibleChannelsChanged,
            this, &MainWindow::onVisibleChannelsChanged);
    
    updateStatusBar();
    raise();
    activateWindow();
    show();
}

MainWindow::~MainWindow() {
    // All QObjects are deleted automatically by parent-child hierarchy
}

void MainWindow::createActions() {
    // File actions
    m_actOpen = new QAction("&Open...", this);  
    m_actOpen->setShortcut(QKeySequence::Open);
    m_actOpen->setStatusTip("Open EEG data file");
    connect(m_actOpen, &QAction::triggered, this, &MainWindow::onFileOpen);
    
    m_actSave = new QAction("&Save", this); 
    m_actSave->setShortcut(QKeySequence::Save);
    m_actSave->setStatusTip("Save EEG data");
    connect(m_actSave, &QAction::triggered, this, &MainWindow::onFileSave);
    
    m_actSaveAs = new QAction("Save &As...", this);  
    m_actSaveAs->setShortcut(QKeySequence::SaveAs);
    m_actSaveAs->setStatusTip("Save EEG data as...");
    connect(m_actSaveAs, &QAction::triggered, this, &MainWindow::onFileSaveAs);

    m_actExit = new QAction("E&xit", this);
    m_actExit->setShortcut(QKeySequence::Quit);
    m_actExit->setStatusTip("Exit application");
    connect(m_actExit, &QAction::triggered, this, &MainWindow::onFileExit);
    
    // View actions
    m_actShowGrid = new QAction("&Show Grid", this);  
    m_actShowGrid->setCheckable(true);
    m_actShowGrid->setChecked(true);
    m_actShowGrid->setStatusTip("Toggle grid display");
    connect(m_actShowGrid, &QAction::toggled, m_chartView, &EEGChartView::setShowGrid);

    m_actZoomIn = new QAction("Zoom &In", this); 
    m_actZoomIn->setShortcut(QKeySequence::ZoomIn);
    m_actZoomIn->setStatusTip("Zoom in");
    connect(m_actZoomIn, &QAction::triggered, [this]() {
        m_chartView->setTimeRange(m_chartView->currentStartTime(), 
                                  m_chartView->currentDuration() * 0.8);
    });
    
    m_actZoomOut = new QAction("Zoom &Out", this);
    m_actZoomOut->setShortcut(QKeySequence::ZoomOut);
    m_actZoomOut->setStatusTip("Zoom out");
    connect(m_actZoomOut, &QAction::triggered, [this]() {
    double newDuration = m_chartView->currentDuration() * 1.25;
    m_chartView->setTimeRange(m_chartView->currentStartTime(), newDuration);
    });
    
    m_actPanLeft = new QAction("Pan &Left", this);  
    m_actPanLeft->setShortcut(Qt::Key_Left);
    m_actPanLeft->setStatusTip("Pan left");
    connect(m_actPanLeft, &QAction::triggered, [this]() {
        double duration = m_chartView->currentDuration();
        m_chartView->setTimeRange(m_chartView->currentStartTime() - duration * 0.1, duration);
    });
    
    m_actPanRight = new QAction("Pan &Right", this); 
    m_actPanRight->setShortcut(Qt::Key_Right);
    m_actPanRight->setStatusTip("Pan right");
    connect(m_actPanRight, &QAction::triggered, [this]() {
        double duration = m_chartView->currentDuration();
        m_chartView->setTimeRange(m_chartView->currentStartTime() + duration * 0.1, duration);
    });
    
    // Tools actions
    m_actStatistics = new QAction("&Statistics", this);  
    m_actStatistics->setStatusTip("Show channel statistics");
    connect(m_actStatistics, &QAction::triggered, this, &MainWindow::onShowStatistics);

    m_actAbout = new QAction("&About", this);
    m_actAbout->setStatusTip("About EEG Data Processor");
    connect(m_actAbout, &QAction::triggered, this, &MainWindow::onShowAbout);

    m_actShowChannels = new QAction("Channels Panel", this);
    m_actShowChannels->setMenuRole(QAction::NoRole);
    m_actShowChannels->setCheckable(true);
    m_actShowChannels->setChecked(true);
    m_actShowChannels->setStatusTip("Show/hide channels panel");
    m_actShowChannels->setVisible(true);
    connect(m_actShowChannels, &QAction::toggled, [this](bool checked) {
        if (m_channelDock) {
            m_channelDock->setVisible(checked);
        }
    });
    
    m_actShowProcessing = new QAction("Signal Processing Panel", this);
    m_actShowProcessing->setMenuRole(QAction::NoRole);
    m_actShowProcessing->setCheckable(true);
    m_actShowProcessing->setChecked(true);
    m_actShowProcessing->setStatusTip("Show/hide signal processing panel");
    m_actShowProcessing->setVisible(true);
    connect(m_actShowProcessing, &QAction::toggled, [this](bool checked) {
        if (m_processingDock) {
            m_processingDock->setVisible(checked);
        }
    });
}

void MainWindow::onVisibleChannelsChanged(const QVector<int> &channels) {
    
    // Update the checkboxes in the channel list to match what's visible
    for (int i = 0; i < m_channelList->count(); ++i) {
        QListWidgetItem *item = m_channelList->item(i);
        if (item) {
            // Check if this channel index is in the visible channels list
            bool isVisible = channels.contains(i);
            item->setCheckState(isVisible ? Qt::Checked : Qt::Unchecked);
        }
    }
}

void MainWindow::createMenus() {
    // File menu
    QMenu *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(m_actOpen);
    fileMenu->addAction(m_actSave);
    fileMenu->addAction(m_actSaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction(m_actExit);
    
    // View menu
    QMenu *viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction(m_actShowGrid);
    viewMenu->addAction(m_actZoomIn);
    viewMenu->addAction(m_actZoomOut);
    viewMenu->addAction(m_actPanLeft);
    viewMenu->addAction(m_actPanRight);

    QMenu *panelsMenu = menuBar()->addMenu("&Panels");
    panelsMenu->addAction(m_actShowChannels);
    panelsMenu->addAction(m_actShowProcessing);
    
    // Tools menu
    QMenu *toolsMenu = menuBar()->addMenu("&Tools");
    toolsMenu->addAction(m_actStatistics);
    
    // Help menu
    QMenu *helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction(m_actAbout);
}

void MainWindow::createToolBars() {
    // File toolbar
    QToolBar *fileToolBar = addToolBar("File");
    fileToolBar->addAction(m_actOpen);
    fileToolBar->addAction(m_actSave);
    fileToolBar->addAction(m_actSaveAs);
    
    // View toolbar
    QToolBar *viewToolBar = addToolBar("View");
    viewToolBar->addAction(m_actShowGrid);
    viewToolBar->addAction(m_actZoomIn);
    viewToolBar->addAction(m_actZoomOut);
    viewToolBar->addAction(m_actPanLeft);
    viewToolBar->addAction(m_actPanRight);

    m_panelToolBar = addToolBar("Panels");
    m_panelToolBar->addAction(m_actShowChannels);
    m_panelToolBar->addAction(m_actShowProcessing);
    
    // Tools toolbar
    QToolBar *toolsToolBar = addToolBar("Tools");
    toolsToolBar->addAction(m_actStatistics);
}

void MainWindow::createDockWidgets() {
    // Channel list dock
    m_channelDock = new QDockWidget("Channels", this);
    m_channelDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_channelDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);

    m_channelList = new QListWidget();
    m_channelList->setSelectionMode(QAbstractItemView::NoSelection);

    // Connect item changes (for checkboxes)
    connect(m_channelList, &QListWidget::itemChanged, 
            this, &MainWindow::onChannelItemChanged);

    m_channelDock->setWidget(m_channelList);
    addDockWidget(Qt::LeftDockWidgetArea, m_channelDock);
    
    // Processing dock
    m_processingDock = new QDockWidget("Signal Processing", this);
    m_processingDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_processingDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);

    // Create a scroll area
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true); 
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    m_processingWidget = new QWidget();
    QVBoxLayout *procLayout = new QVBoxLayout(m_processingWidget);
    
    // Channel selection
    int channelCount = 0;
    if (m_eegData && !m_eegData->isEmpty()) {
        channelCount = m_eegData->channelCount();
    }
    QGroupBox *channelGroup = new QGroupBox("Channel Selection");
    QFormLayout *channelLayout = new QFormLayout(channelGroup);
    m_channelSelectSpin = new QSpinBox();
    m_channelSelectSpin->setRange(-1, qMax(0, channelCount - 1));
    m_channelSelectSpin->setSpecialValueText("None");  
    m_channelSelectSpin->setValue(0);
    channelLayout->addRow("Channel:", m_channelSelectSpin);
    procLayout->addWidget(channelGroup);

    connect(m_channelSelectSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int channel) {
        static int lastChannel = -1;
        
        
        if (channel == -1) {
            // User selected "None"
            m_chartView->clearSelectedChannel();
            lastChannel = -1;
        }
        else if (channel == lastChannel) {
            // Same channel clicked again - deselect
            m_chartView->clearSelectedChannel();
            lastChannel = -1;
            m_channelSelectSpin->setValue(-1);  // Set to "None"
        } else {
            // New channel selected
            m_chartView->setSelectedChannel(channel);
            lastChannel = channel;
        }
    });
    
    // Filter controls
    QGroupBox *filterGroup = new QGroupBox("Filter");
    QFormLayout *filterLayout = new QFormLayout(filterGroup);
    
    m_filterTypeCombo = new QComboBox();
    m_filterTypeCombo->addItems({"Bandpass", "Highpass", "Lowpass", "Notch"});
    
    m_lowCutSpin = new QDoubleSpinBox();
    m_lowCutSpin->setRange(0.1, 100.0);
    m_lowCutSpin->setValue(0.5);
    m_lowCutSpin->setSuffix(" Hz");
    
    m_highCutSpin = new QDoubleSpinBox();
    m_highCutSpin->setRange(0.1, 100.0);
    m_highCutSpin->setValue(30.0);
    m_highCutSpin->setSuffix(" Hz");
    
    filterLayout->addRow("Type:", m_filterTypeCombo);
    filterLayout->addRow("Low Cut:", m_lowCutSpin);
    filterLayout->addRow("High Cut:", m_highCutSpin);
    
    QPushButton *filterButton = new QPushButton("Apply Filter");
    connect(filterButton, &QPushButton::clicked, this, &MainWindow::onFilterApply);
    filterLayout->addRow(filterButton);
    
    procLayout->addWidget(filterGroup);
    
    // Gain/Offset controls
    QGroupBox *gainGroup = new QGroupBox("Gain/Offset");
    QFormLayout *gainLayout = new QFormLayout(gainGroup);
    
    m_gainSpin = new QDoubleSpinBox();
    m_gainSpin->setRange(0.1, 10.0);
    m_gainSpin->setValue(1.0);
    m_gainSpin->setSingleStep(0.1);
    
    m_offsetSpin = new QDoubleSpinBox();
    m_offsetSpin->setRange(-1000.0, 1000.0);
    m_offsetSpin->setValue(0.0);
    m_offsetSpin->setSuffix(" μV");
    
    gainLayout->addRow("Gain:", m_gainSpin);
    gainLayout->addRow("Offset:", m_offsetSpin);
    
    QHBoxLayout *gainButtons = new QHBoxLayout();
    QPushButton *gainButton = new QPushButton("Apply Gain");
    QPushButton *offsetButton = new QPushButton("Apply Offset");
    QPushButton *normalizeButton = new QPushButton("Normalize");
    QPushButton *dcButton = new QPushButton("Remove DC");
    
    connect(gainButton, &QPushButton::clicked, this, &MainWindow::onGainApply);
    connect(offsetButton, &QPushButton::clicked, this, &MainWindow::onOffsetApply);
    connect(normalizeButton, &QPushButton::clicked, this, &MainWindow::onNormalizeApply);
    connect(dcButton, &QPushButton::clicked, this, &MainWindow::onDCRemoveApply);
    
    gainButtons->addWidget(gainButton);
    gainButtons->addWidget(offsetButton);
    gainButtons->addWidget(normalizeButton);
    gainButtons->addWidget(dcButton);
    
    gainLayout->addRow(gainButtons);
    procLayout->addWidget(gainGroup);
    
    // Notch filter
    QGroupBox *notchGroup = new QGroupBox("Notch Filter");
    QFormLayout *notchLayout = new QFormLayout(notchGroup);

    // Frequency selection combo box
    m_notchFreqCombo = new QComboBox();
    m_notchFreqCombo->addItem("50 Hz (Europe/Asia)", 50);
    m_notchFreqCombo->addItem("60 Hz (North America)", 60);
    m_notchFreqCombo->setCurrentIndex(0);  // Default to 50 Hz

    // Apply button
    QPushButton *notchButton = new QPushButton("Apply Notch Filter");
    connect(notchButton, &QPushButton::clicked, this, &MainWindow::onNotchFilterApply);

    notchLayout->addRow("Frequency:", m_notchFreqCombo);
    notchLayout->addRow(notchButton);

    procLayout->addWidget(notchGroup);
    
    // Montage
    QGroupBox *montageGroup = new QGroupBox("Montage");
    QFormLayout *montageLayout = new QFormLayout(montageGroup);
    
    m_montageCombo = new QComboBox();
    m_montageCombo->addItems({"Bipolar", "Average Reference", "Laplacian"});
    
    QPushButton *montageButton = new QPushButton("Apply Montage");
    connect(montageButton, &QPushButton::clicked, this, &MainWindow::onMontageApply);
    
    QPushButton *montageButton1 = new QPushButton("Reset Montage");
    connect(montageButton1, &QPushButton::clicked, this, &MainWindow::onResetMontage);
    
    montageLayout->addRow("Type:", m_montageCombo);
    montageLayout->addRow(montageButton);
    montageLayout->addRow(montageButton1);
    
    procLayout->addWidget(montageGroup);
    
    // Display controls
    QGroupBox *displayGroup = new QGroupBox("Display");
    QFormLayout *displayLayout = new QFormLayout(displayGroup);
    
    m_timeStartSpin = new QDoubleSpinBox();
    m_timeStartSpin->setRange(0.0, 1000.0);
    m_timeStartSpin->setValue(0.0);
    m_timeStartSpin->setSuffix(" s");

    m_timeStartSpin->setEnabled(true);
    m_timeStartSpin->setSingleStep(0.1); 
    m_timeStartSpin->setKeyboardTracking(true);
    
    m_timeDurationSpin = new QDoubleSpinBox();
    m_timeDurationSpin->setRange(0.1, 3600.0);
    m_timeDurationSpin->setValue(10.0);
    m_timeDurationSpin->setSuffix(" s");
    
    m_verticalScaleSpin = new QDoubleSpinBox();
    m_verticalScaleSpin->setRange(0.1, 10.0);
    m_verticalScaleSpin->setValue(1.0);
    
    m_offsetScaleSpin = new QDoubleSpinBox();
    m_offsetScaleSpin->setRange(10.0, 500.0);
    m_offsetScaleSpin->setValue(100.0);
    m_offsetScaleSpin->setSuffix(" μV");
    
    displayLayout->addRow("Start Time:", m_timeStartSpin);
    displayLayout->addRow("Duration:", m_timeDurationSpin);
    displayLayout->addRow("Vertical Scale:", m_verticalScaleSpin);
    displayLayout->addRow("Offset Scale:", m_offsetScaleSpin);
    
    QPushButton *updateDisplayButton = new QPushButton("Update Display");
    connect(updateDisplayButton, &QPushButton::clicked, [this]() {
        m_chartView->setTimeRange(m_timeStartSpin->value(), m_timeDurationSpin->value());
        m_chartView->setVerticalScale(m_verticalScaleSpin->value());
        m_chartView->setOffsetScale(m_offsetScaleSpin->value());
    });
    
    displayLayout->addRow(updateDisplayButton);
    procLayout->addWidget(displayGroup);
    
    procLayout->addStretch();
    m_processingDock->setWidget(m_processingWidget);
    addDockWidget(Qt::RightDockWidgetArea, m_processingDock);

    if (m_channelDock && m_actShowChannels) {
        connect(m_channelDock, &QDockWidget::visibilityChanged,
                m_actShowChannels, &QAction::setChecked);
    }

    if (m_processingDock && m_actShowProcessing) {
        connect(m_processingDock, &QDockWidget::visibilityChanged,
                m_actShowProcessing, &QAction::setChecked);
    }

    connect(m_timeStartSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        [this](double) {  
    m_chartView->setTimeRange(m_timeStartSpin->value(), m_timeDurationSpin->value());
    });

    connect(m_timeDurationSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double) {
        m_chartView->setTimeRange(m_timeStartSpin->value(), m_timeDurationSpin->value());
    });

    connect(m_verticalScaleSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            m_chartView, &EEGChartView::setVerticalScale);

    connect(m_offsetScaleSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            m_chartView, &EEGChartView::setOffsetScale);

    // Frequency Analysis Group
    QGroupBox *freqGroup = new QGroupBox("Frequency Analysis");
    QFormLayout *freqLayout = new QFormLayout(freqGroup);

    // Channel selection for frequency analysis
    QComboBox *freqChannelCombo = new QComboBox();
    freqChannelCombo->setObjectName("freqChannelCombo"); 
    freqChannelCombo->addItem("All Channels", -1);
    for (int i = 0; i < m_eegData->channelCount(); ++i) {
        const EEGChannel &channel = m_eegData->channel(i);
        freqChannelCombo->addItem(QString("%1: %2").arg(i).arg(channel.label), i);
    }

    // Window size for FFT
    QComboBox *windowSizeCombo = new QComboBox();
    windowSizeCombo->addItem("256 samples", 256);
    windowSizeCombo->addItem("512 samples", 512);
    windowSizeCombo->addItem("1024 samples", 1024);
    windowSizeCombo->addItem("2048 samples", 2048);
    windowSizeCombo->addItem("4096 samples", 4096);
    windowSizeCombo->addItem("8192 samples", 8192);
    windowSizeCombo->addItem("16384 samples", 16384);
    windowSizeCombo->addItem("32768 samples", 32768);
    windowSizeCombo->addItem("65536 samples", 65536);
    windowSizeCombo->addItem("131072 samples", 131072);
    windowSizeCombo->setCurrentIndex(4); // Default to 4096

    // Frequency range display (updates based on sampling rate)
    QLabel *freqRangeLabel = new QLabel();
    if (!m_eegData->isEmpty()) {
        double maxFreq = m_eegData->channel(0).samplingRate / 2;
        freqRangeLabel->setText(QString("Frequency Range: 0-%1 Hz").arg(maxFreq, 0, 'f', 1));
    } else {
        freqRangeLabel->setText("Frequency Range: 0-125 Hz (default)");
    }

    // Buttons for different analyses
    QPushButton *powerSpectrumBtn = new QPushButton("Show Power Spectrum");
    QPushButton *bandPowerBtn = new QPushButton("Show Band Powers");
    QPushButton *spectrogramBtn = new QPushButton("Show Spectrogram");

    freqLayout->addRow("Channel:", freqChannelCombo);
    freqLayout->addRow("FFT Window:", windowSizeCombo);
    freqLayout->addRow(freqRangeLabel);
    freqLayout->addRow(powerSpectrumBtn);
    freqLayout->addRow(bandPowerBtn);
    freqLayout->addRow(spectrogramBtn);

    procLayout->addWidget(freqGroup);

    // Connect buttons
    connect(powerSpectrumBtn, &QPushButton::clicked, [this, freqChannelCombo, windowSizeCombo]() {
        int channelIndex = freqChannelCombo->currentData().toInt();
        int windowSizeIndex = windowSizeCombo->currentIndex(); // Use index, not value
        showPowerSpectrum(channelIndex, windowSizeIndex);
    });

    connect(bandPowerBtn, &QPushButton::clicked, [this, freqChannelCombo]() {
        int channelIndex = freqChannelCombo->currentData().toInt();
        showBandPower(channelIndex);
    });

    connect(spectrogramBtn, &QPushButton::clicked, [this, freqChannelCombo]() {
        int channelIndex = freqChannelCombo->currentData().toInt();
        showSpectrogram(channelIndex);
    });

    procLayout->addStretch(); 

    // Set the processing widget as the scroll area's widget
    scrollArea->setWidget(m_processingWidget);

    // Set the scroll area as the dock widget's widget
    m_processingDock->setWidget(scrollArea);
    addDockWidget(Qt::RightDockWidgetArea, m_processingDock);
}

void MainWindow::createCentralWidget() {
    m_chartView->setEEGData(m_eegData);
    connect(m_chartView, &EEGChartView::timeRangeChanged,
            this, &MainWindow::onTimeRangeChanged);
    
    setCentralWidget(m_chartView);
}

void MainWindow::createStatusBar() {
    m_statusFile = new QLabel("No file loaded");
    m_statusChannels = new QLabel("Channels: 0");
    m_statusDuration = new QLabel("Duration: 0.0 s");
    m_statusSamplingRate = new QLabel("Rate: 0.0 Hz");
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    m_progressBar->setMaximumWidth(200);
    
    statusBar()->addWidget(m_statusFile);
    statusBar()->addWidget(m_statusChannels);
    statusBar()->addWidget(m_statusDuration);
    statusBar()->addWidget(m_statusSamplingRate);
    statusBar()->addPermanentWidget(m_progressBar);
}

void MainWindow::onFileOpen() {
    
    QString filePath = QFileDialog::getOpenFileName(
        nullptr,
        QString("Open EEG Data File"),
        QDir::homePath(),
        "EEG Files (*.edf *.csv *.txt *.dat);;All Files (*)",
        nullptr,
        QFileDialog::DontUseNativeDialog
    );
    
    
    if (filePath.isEmpty()) {
        return;
    }
    
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    
    for (int i = 0; i <= 100; i += 10) {
        m_progressBar->setValue(i);
        QCoreApplication::processEvents();
    }
    
    if (m_eegData->loadFromFile(filePath)) {
        m_currentFilePath = filePath;
        m_chartView->selectAllChannels();
        m_chartView->updateChart();
        setWindowTitle(QString("EEG Data Processor - %1").arg(QFileInfo(filePath).fileName()));
        
        int channelCount = m_eegData->channelCount();
        m_channelSelectSpin->setRange(-1, qMax(0, channelCount - 1));
        m_channelSelectSpin->setSpecialValueText("None"); 
        m_channelSelectSpin->setValue(-1);
        
        double duration = m_eegData->duration();
        
        m_timeDurationSpin->blockSignals(true);
        m_timeDurationSpin->setRange(0.1, duration);
        m_timeDurationSpin->setValue(qMin(10.0, duration));
        m_timeDurationSpin->blockSignals(false);

        QComboBox *freqChannelCombo = findChild<QComboBox*>("freqChannelCombo"); // If you set object name
        if (freqChannelCombo) {
            freqChannelCombo->clear();
            freqChannelCombo->addItem("All Channels", -1);
            for (int i = 0; i < m_eegData->channelCount(); ++i) {
                const EEGChannel &channel = m_eegData->channel(i);
                freqChannelCombo->addItem(QString("%1: %2").arg(i).arg(channel.label), i);
            }
        }
        
        QMessageBox::information(this, "Success", 
                                QString("Loaded %1 channels with %2 seconds of data")
                                .arg(channelCount).arg(duration, 0, 'f', 2));
    } else {
        QMessageBox::warning(this, "Error", "Failed to load EEG data file");
    }
    
    m_progressBar->setVisible(false);
}

void MainWindow::onFileSave() {
    // Check if there's data to save
    if (!m_eegData || m_eegData->isEmpty()) {
        QMessageBox::warning(this, "Error", "No data to save");
        return;
    }
    
    // If no file path (new unsaved file), use Save As instead
    if (m_currentFilePath.isEmpty()) {
        onFileSaveAs();
        return;
    }
    
    // Ask for confirmation before overwriting
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        "Confirm Save",
        "This will overwrite the current file. Continue?",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        if (m_eegData->saveToFile(m_currentFilePath)) {
            QMessageBox::information(this, "Success", "Data saved successfully");
        } else {
            QMessageBox::warning(this, "Error", "Failed to save data");
        }
    }
}

void MainWindow::onFileSaveAs() {
    // Check if there's data to save
    if (!m_eegData || m_eegData->isEmpty()) {
        QMessageBox::warning(this, "Error", "No data to save");
        return;
    }
    
    // Generate suggested filename based on current file
    QString baseName = "untitled";
    if (!m_currentFilePath.isEmpty()) {
        baseName = QFileInfo(m_currentFilePath).baseName();
    }
    
    // Force Qt dialog (not native macOS dialog)
    QString filePath = QFileDialog::getSaveFileName(
        this, 
        "Save EEG Data As",
        baseName,
        "EDF Files (*.edf);;CSV Files (*.csv);;Text Files (*.txt);;All Files (*)",
        nullptr,  // selected filter
        QFileDialog::DontUseNativeDialog 
    );
    
    if (filePath.isEmpty()) {
        return; // User cancelled
    }
    
    // Save to the new file
    if (m_eegData->saveToFile(filePath)) {
        m_currentFilePath = filePath;
        setWindowTitle(QString("EEG Data Processor - %1").arg(QFileInfo(filePath).fileName()));
        QMessageBox::information(this, "Success", "Data saved to:\n" + filePath);
    } else {
        QMessageBox::warning(this, "Error", "Failed to save file");
    }
}

void MainWindow::onFilterApply() {
    int channel = m_channelSelectSpin->value();
    double lowCut = m_lowCutSpin->value();
    double highCut = m_highCutSpin->value();
    
    if (channel < 0 || channel >= m_eegData->channelCount()) {
        QMessageBox::warning(this, "Error", "Invalid channel selection");
        return;
    }
    
    m_eegData->applyFilter(channel, lowCut, highCut);
    m_chartView->updateChart();
}

void MainWindow::onGainApply() {
    int channel = m_channelSelectSpin->value();
    double gain = m_gainSpin->value();
    
    if (channel < 0 || channel >= m_eegData->channelCount()) {
        QMessageBox::warning(this, "Error", "Invalid channel selection");
        return;
    }
    
    m_eegData->applyGain(channel, gain);
    m_chartView->updateChart();
}

void MainWindow::onOffsetApply() {
    int channel = m_channelSelectSpin->value();
    double offset = m_offsetSpin->value();
    
    if (channel < 0 || channel >= m_eegData->channelCount()) {
        QMessageBox::warning(this, "Error", "Invalid channel selection");
        return;
    }
    
    m_eegData->applyOffset(channel, offset);
    m_chartView->updateChart();
}

void MainWindow::onNormalizeApply() {
    int channel = m_channelSelectSpin->value();
    
    if (channel < 0 || channel >= m_eegData->channelCount()) {
        QMessageBox::warning(this, "Error", "Invalid channel selection");
        return;
    }
    
    m_eegData->normalizeChannel(channel);
    m_chartView->updateChart();
}

void MainWindow::onDCRemoveApply() {
    int channel = m_channelSelectSpin->value();
    
    if (channel < 0 || channel >= m_eegData->channelCount()) {
        QMessageBox::warning(this, "Error", "Invalid channel selection");
        return;
    }
    
    m_eegData->removeDC(channel);
    m_chartView->updateChart();
}

void MainWindow::onNotchFilterApply() {
    // Get selected frequency
    double notchFreq = m_notchFreqCombo->currentData().toDouble();
    
    // Check if we have data
    if (!m_eegData || m_eegData->isEmpty()) {
        QMessageBox::warning(this, "Error", "No data loaded");
        return;
    }
    
    // Show progress
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    
    // Create a copy for filtering
    EEGData *filteredData = new EEGData(this);

    for (int i = 0; i < m_eegData->channelCount(); ++i) {
        const EEGChannel &originalChannel = m_eegData->channel(i);
        EEGChannel newChannel;
        newChannel.label = originalChannel.label;
        newChannel.samplingRate = originalChannel.samplingRate;
        newChannel.data = originalChannel.data;  // QVector can be copied
        filteredData->addChannel(newChannel);
        m_progressBar->setValue((i + 1) * 50 / m_eegData->channelCount());
    }
    
    int channel = m_channelSelectSpin->value();
    if (channel >= 0) {
        filteredData->applyNotchFilter(channel, notchFreq);
    } else {
        for (int i = 0; i < filteredData->channelCount(); ++i) {
            filteredData->applyNotchFilter(i, notchFreq);
            m_progressBar->setValue(50 + (i + 1) * 50 / filteredData->channelCount());
            QCoreApplication::processEvents();
        }
    }
    
    m_progressBar->setVisible(false);
    
    // Show preview dialog
    NotchPreviewDialog dialog(m_eegData, filteredData, notchFreq, this);
    
    if (dialog.exec() == QDialog::Accepted) {
        // If user saved/overwrote, update the chart
        m_chartView->updateChart();
        updateChannelList();
        updateStatusBar();
    }

    delete filteredData;
}

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
    updateChannelList();
}

void MainWindow::onResetMontage() {
    if (!m_currentFilePath.isEmpty()) {
        m_eegData->loadFromFile(m_currentFilePath);

        m_chartView->selectAllChannels();

        m_chartView->updateChart();
        updateChannelList();
        updateStatusBar();
        
        QMessageBox::information(this, "Reset", "Data restored to original");
    } else {
        QMessageBox::warning(this, "Error", "No original file to restore from");
    }
}


void MainWindow::onTimeRangeChanged(double start, double duration) {
    m_timeStartSpin->setValue(start);
    m_timeDurationSpin->setValue(duration);
}

void MainWindow::onVerticalScaleChanged(double value) {
    m_verticalScaleSpin->setValue(value);
}

void MainWindow::onOffsetScaleChanged(double value) {
    m_offsetScaleSpin->setValue(value);
}

void MainWindow::onShowStatistics() {
    if (m_eegData->isEmpty()) {
        QMessageBox::information(this, "Statistics", "No data loaded");
        return;
    }
    
    QDialog statsDialog(this);
    statsDialog.setWindowTitle("Channel Statistics");
    statsDialog.resize(800, 500);  // Made wider
    
    QVBoxLayout *layout = new QVBoxLayout(&statsDialog);
    
    // Add more columns for min/max
    QTableWidget *table = new QTableWidget();
    table->setColumnCount(10); 
    table->setHorizontalHeaderLabels({
        "Channel", "Label", "Samples", "Rate (Hz)", 
        "Mean (μV)", "StdDev (μV)", "Min (μV)", "Max (μV)",
        "Peak-Peak", "Variance"
    });
    
    int channelCount = m_eegData->channelCount();
    table->setRowCount(channelCount);
    
    auto means = m_eegData->channelMeans();
    auto stddevs = m_eegData->channelStdDevs();
    
    for (int i = 0; i < channelCount; ++i) {
        const EEGChannel &channel = m_eegData->channel(i);
        
        auto minMax = std::minmax_element(channel.data.begin(), channel.data.end());
        double minVal = *minMax.first;
        double maxVal = *minMax.second;
        double peakToPeak = maxVal - minVal;
        double variance = stddevs[i] * stddevs[i];
        
        table->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        table->setItem(i, 1, new QTableWidgetItem(channel.label));
        table->setItem(i, 2, new QTableWidgetItem(QString::number(channel.data.size())));
        table->setItem(i, 3, new QTableWidgetItem(QString::number(channel.samplingRate, 'f', 1)));
        table->setItem(i, 4, new QTableWidgetItem(QString::number(means[i], 'f', 2)));
        table->setItem(i, 5, new QTableWidgetItem(QString::number(stddevs[i], 'f', 2)));
        table->setItem(i, 6, new QTableWidgetItem(QString::number(minVal, 'f', 2)));  // Min
        table->setItem(i, 7, new QTableWidgetItem(QString::number(maxVal, 'f', 2)));  // Max
        table->setItem(i, 8, new QTableWidgetItem(QString::number(peakToPeak, 'f', 2)));  // Peak-Peak
        table->setItem(i, 9, new QTableWidgetItem(QString::number(variance, 'f', 2)));  // Variance
    }
    
    table->resizeColumnsToContents();
    layout->addWidget(table);
    
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, &statsDialog, &QDialog::reject);
    layout->addWidget(buttons);
    
    statsDialog.exec();
}

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

void MainWindow::updateChannelList() {
    m_channelList->clear();
    QVector<int> visibleChannels = m_chartView->getVisibleChannels();
    
    for (int i = 0; i < m_eegData->channelCount(); ++i) {
        const EEGChannel &channel = m_eegData->channel(i);
        QString itemText = QString("%1: %2 (%3 samples, %4 Hz)")
                          .arg(i + 1, 2)
                          .arg(channel.label)
                          .arg(channel.data.size())
                          .arg(channel.samplingRate, 0, 'f', 1);
        
        QListWidgetItem *item = new QListWidgetItem(itemText);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
        bool isVisible = visibleChannels.contains(i);
        item->setCheckState(isVisible ? Qt::Checked : Qt::Unchecked);

        m_channelList->addItem(item);
    }
    
    // Update channel selection spin box
    int channelCount = m_eegData->channelCount();
    m_channelSelectSpin->setRange(-1, qMax(0, channelCount - 1));
}

void MainWindow::onFileExit() {
    close(); 
}

void MainWindow::onChannelItemChanged(QListWidgetItem *item) {
    if (!item) return;
    
    int channelIndex = m_channelList->row(item);
    bool isChecked = (item->checkState() == Qt::Checked);
    
    // Get current visible channels
    QVector<int> visibleChannels = m_chartView->getVisibleChannels();
    
    if (isChecked) {
        // Add channel if not already visible
        if (!visibleChannels.contains(channelIndex)) {
            visibleChannels.append(channelIndex);
            std::sort(visibleChannels.begin(), visibleChannels.end());
        }
    } else {
        // Remove channel if visible
        visibleChannels.removeAll(channelIndex);
    }
    
    // Update chart with new visible channels
    m_chartView->setVisibleChannels(visibleChannels);
}

void MainWindow::showPowerSpectrum(int channelIndex, int windowSizeIndex) {
    if (m_eegData->isEmpty()) {
        QMessageBox::warning(this, "Error", "No data loaded");
        return;
    }
    
    // Determine window size
    int windowSizes[] = {256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072};
    int windowSize = windowSizes[windowSizeIndex];
    
    // Create dialog
    QDialog spectrumDialog(this);
    spectrumDialog.setWindowTitle("Power Spectrum");
    spectrumDialog.resize(800, 600);
    
    QVBoxLayout *layout = new QVBoxLayout(&spectrumDialog);
    
    // Create chart view for spectrum
    QChartView *chartView = new QChartView();
    chartView->setRenderHint(QPainter::Antialiasing);
    
    QChart *chart = new QChart();
    chart->setTitle(channelIndex >= 0 
        ? QString("Power Spectrum - Channel %1").arg(channelIndex)
        : "Power Spectrum - All Channels (Average)");
    
    if (channelIndex >= 0) {
        // Single channel spectrum
        const EEGChannel &channel = m_eegData->channel(channelIndex);
        
        // Take a window of data
        int startSample = 0;
        int endSample = std::min(windowSize, channel.data.size());
        QVector<double> windowData(channel.data.begin() + startSample, 
                                   channel.data.begin() + endSample);
        
        // Calculate spectrum
        QVector<double> spectrum = SignalProcessor::powerSpectrum(windowData, channel.samplingRate);
        
        // Create series
        QLineSeries *series = new QLineSeries();
        series->setName(channel.label);
        
        double freqResolution = channel.samplingRate / (2.0 * spectrum.size());
        for (int i = 0; i < spectrum.size(); ++i) {
            double freq = i * freqResolution;
            series->append(freq, spectrum[i]);
        }
        
        chart->addSeries(series);
        
        // Create axes
        QValueAxis *axisX = new QValueAxis();
        axisX->setTitleText("Frequency (Hz)");
        axisX->setRange(0, channel.samplingRate / 2);
        
        QValueAxis *axisY = new QValueAxis();
        axisY->setTitleText("Amplitude");
        
        chart->addAxis(axisX, Qt::AlignBottom);
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisX);
        series->attachAxis(axisY);
    } else {
        // Average spectrum across all channels
        QVector<double> avgSpectrum;
        
        for (int ch = 0; ch < m_eegData->channelCount(); ++ch) {
            const EEGChannel &channel = m_eegData->channel(ch);
            
            int startSample = 0;
            int endSample = std::min(windowSize, channel.data.size());
            QVector<double> windowData(channel.data.begin() + startSample, 
                                       channel.data.begin() + endSample);
            
            QVector<double> spectrum = SignalProcessor::powerSpectrum(windowData, channel.samplingRate);
            
            if (ch == 0) {
                avgSpectrum = spectrum;
            } else {
                for (int i = 0; i < spectrum.size(); ++i) {
                    avgSpectrum[i] += spectrum[i];
                }
            }
        }
        
        // Average
        for (int i = 0; i < avgSpectrum.size(); ++i) {
            avgSpectrum[i] /= m_eegData->channelCount();
        }
        
        // Create series
        QLineSeries *series = new QLineSeries();
        series->setName("Average Spectrum");
        
        double samplingRate = m_eegData->channel(0).samplingRate;
        double freqResolution = samplingRate / (2.0 * avgSpectrum.size());
        for (int i = 0; i < avgSpectrum.size(); ++i) {
            double freq = i * freqResolution;
            series->append(freq, avgSpectrum[i]);
        }
        
        chart->addSeries(series);
        
        QValueAxis *axisX = new QValueAxis();
        axisX->setTitleText("Frequency (Hz)");
        axisX->setRange(0, samplingRate / 2);
        
        QValueAxis *axisY = new QValueAxis();
        axisY->setTitleText("Amplitude");
        
        chart->addAxis(axisX, Qt::AlignBottom);
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisX);
        series->attachAxis(axisY);
    }
    
    chartView->setChart(chart);
    layout->addWidget(chartView);
    
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, &spectrumDialog, &QDialog::reject);
    layout->addWidget(buttons);
    
    spectrumDialog.exec();
}

void MainWindow::showBandPower(int channelIndex) {
    if (m_eegData->isEmpty()) {
        QMessageBox::warning(this, "Error", "No data loaded");
        return;
    }
    
    QDialog bandDialog(this);
    bandDialog.setWindowTitle("Band Power Analysis");
    bandDialog.resize(600, 400);
    
    QVBoxLayout *layout = new QVBoxLayout(&bandDialog);
    
    QTableWidget *table = new QTableWidget();
    table->setColumnCount(6);
    table->setHorizontalHeaderLabels({
        "Channel", "Delta (0.5-4Hz)", "Theta (4-8Hz)", 
        "Alpha (8-13Hz)", "Beta (13-30Hz)", "Gamma (30-100Hz)"
    });
    
    if (channelIndex >= 0) {
        // Single channel
        table->setRowCount(1);
        const EEGChannel &channel = m_eegData->channel(channelIndex);
        auto power = SignalProcessor::calculateBandPower(channel.data, channel.samplingRate);
        
        table->setItem(0, 0, new QTableWidgetItem(channel.label));
        table->setItem(0, 1, new QTableWidgetItem(QString::number(power.delta, 'e', 3)));
        table->setItem(0, 2, new QTableWidgetItem(QString::number(power.theta, 'e', 3)));
        table->setItem(0, 3, new QTableWidgetItem(QString::number(power.alpha, 'e', 3)));
        table->setItem(0, 4, new QTableWidgetItem(QString::number(power.beta, 'e', 3)));
        table->setItem(0, 5, new QTableWidgetItem(QString::number(power.gamma, 'e', 3)));
    } else {
        // All channels
        table->setRowCount(m_eegData->channelCount());
        for (int i = 0; i < m_eegData->channelCount(); ++i) {
            const EEGChannel &channel = m_eegData->channel(i);
            auto power = SignalProcessor::calculateBandPower(channel.data, channel.samplingRate);
            
            table->setItem(i, 0, new QTableWidgetItem(channel.label));
            table->setItem(i, 1, new QTableWidgetItem(QString::number(power.delta, 'e', 3)));
            table->setItem(i, 2, new QTableWidgetItem(QString::number(power.theta, 'e', 3)));
            table->setItem(i, 3, new QTableWidgetItem(QString::number(power.alpha, 'e', 3)));
            table->setItem(i, 4, new QTableWidgetItem(QString::number(power.beta, 'e', 3)));
            table->setItem(i, 5, new QTableWidgetItem(QString::number(power.gamma, 'e', 3)));
        }
    }
    
    table->resizeColumnsToContents();
    layout->addWidget(table);
    
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, &bandDialog, &QDialog::reject);
    layout->addWidget(buttons);
    
    bandDialog.exec();
}

void MainWindow::showSpectrogram(int channelIndex) {
    // Validate channel index
    if (channelIndex < 0 || channelIndex >= m_eegData->channelCount()) {
        QMessageBox::warning(this, "Error", "Please select a specific channel");
        return;
    }

    const EEGChannel &channel = m_eegData->channel(channelIndex);

    // Spectrogram parameters
    const int windowSize = 256;
    const int hopSize = 64;
    const double samplingRate = channel.samplingRate;

    int numWindows = (channel.data.size() - windowSize) / hopSize + 1;
    if (numWindows < 1) {
        QMessageBox::warning(this, "Error", "Not enough data for spectrogram");
        return;
    }

    const int numFreqBins = windowSize / 2 + 1;

    // Precompute Hann window
    QVector<double> window(windowSize);
    for (int i = 0; i < windowSize; ++i)
        window[i] = 0.5 * (1.0 - cos(2.0 * M_PI * i / (windowSize - 1)));
    double windowSum = std::accumulate(window.begin(), window.end(), 0.0);

    // Spectrogram data container
    QVector<double> spectrogramData(numWindows * numFreqBins, -100.0);

    // Show progress
    QProgressDialog progress("Computing spectrogram...", "Cancel", 0, numWindows, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.setValue(0);

    // FFTW setup
    fftw_complex *in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * windowSize);
    fftw_complex *out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * windowSize);
    fftw_plan plan = fftw_plan_dft_1d(windowSize, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    bool wasCancelled = false;

    // Compute spectrogram
    for (int win = 0; win < numWindows; ++win) {
        if (progress.wasCanceled()) {
            wasCancelled = true;
            break;
        }
        progress.setValue(win);
        QCoreApplication::processEvents();

        int startIdx = win * hopSize;

        for (int i = 0; i < windowSize; ++i) {
            in[i][0] = channel.data[startIdx + i] * window[i];
            in[i][1] = 0.0;
        }

        fftw_execute(plan);

        for (int freq = 0; freq < numFreqBins; ++freq) {
            double real = out[freq][0];
            double imag = out[freq][1];
            double power = (real * real + imag * imag) / (windowSum * windowSum);
            spectrogramData[win * numFreqBins + freq] = (power > 1e-10) ? 10.0 * log10(power) : -100.0;
        }
    }

    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);

    if (wasCancelled) {
        progress.reset();
        return;
    }

    progress.reset();
    QCoreApplication::processEvents();

    // ===== DIALOG SETUP =====
    QDialog *specDialog = new QDialog(this);
    specDialog->setAttribute(Qt::WA_DeleteOnClose, true);
    specDialog->setWindowTitle(QString("Spectrogram - Channel %1 (%2)").arg(channelIndex).arg(channel.label));
    specDialog->resize(900, 600);

    QVBoxLayout *layout = new QVBoxLayout(specDialog);

    // QCustomPlot widget
    QCustomPlot *customPlot = new QCustomPlot(specDialog);
    layout->addWidget(customPlot);

    // Create color map (heatmap)
    QCPColorMap *colorMap = new QCPColorMap(customPlot->xAxis, customPlot->yAxis);
    colorMap->data()->setSize(numWindows, numFreqBins); // width = time, height = frequency
    colorMap->data()->setRange(QCPRange(0, numWindows * hopSize / samplingRate),
                               QCPRange(0, samplingRate / 2.0));

    // Fill data
    for (int x = 0; x < numWindows; ++x) {
        for (int y = 0; y < numFreqBins; ++y) {
            colorMap->data()->setCell(x, y, spectrogramData[x * numFreqBins + y]);
        }
    }

    // Color gradient
    QCPColorGradient gradient;
    gradient.setColorStopAt(0.0, Qt::darkBlue);
    gradient.setColorStopAt(0.25, Qt::blue);
    gradient.setColorStopAt(0.5, Qt::green);
    gradient.setColorStopAt(0.75, Qt::yellow);
    gradient.setColorStopAt(1.0, Qt::darkRed);
    colorMap->setGradient(gradient);
    colorMap->rescaleDataRange();

    // Rescale axes
    customPlot->rescaleAxes();
    customPlot->xAxis->setLabel("Time (s)");
    customPlot->yAxis->setLabel("Frequency (Hz)");

    // Close button
    QPushButton *closeButton = new QPushButton("Close", specDialog);
    connect(closeButton, &QPushButton::clicked, specDialog, &QDialog::accept);
    layout->addWidget(closeButton);

    specDialog->show();
}