#include "MainWindow.h"
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
    
    // Connect signals
    connect(m_eegData, &EEGData::dataChanged, this, &MainWindow::updateStatusBar);
    connect(m_eegData, &EEGData::dataChanged, this, &MainWindow::updateChannelList);
    connect(m_eegData, &EEGData::channelAdded, this, &MainWindow::updateChannelList);
    connect(m_eegData, &EEGData::channelRemoved, this, &MainWindow::updateChannelList);

    connect(m_chartView, &EEGChartView::visibleChannelsChanged,
            this, &MainWindow::onVisibleChannelsChanged);
    
    updateStatusBar();
}

MainWindow::~MainWindow() {
    // All QObjects are deleted automatically by parent-child hierarchy
}

void MainWindow::createActions() {
    // File actions
    m_actOpen = new QAction("&Open...", this);  // CHANGED LINE 51
    m_actOpen->setShortcut(QKeySequence::Open);
    m_actOpen->setStatusTip("Open EEG data file");
    connect(m_actOpen, &QAction::triggered, this, &MainWindow::onFileOpen);
    
    m_actSave = new QAction("&Save", this);  // CHANGED LINE 56
    m_actSave->setShortcut(QKeySequence::Save);
    m_actSave->setStatusTip("Save EEG data");
    connect(m_actSave, &QAction::triggered, this, &MainWindow::onFileSave);
    
    m_actSaveAs = new QAction("Save &As...", this);  // CHANGED LINE 61
    m_actSaveAs->setShortcut(QKeySequence::SaveAs);
    m_actSaveAs->setStatusTip("Save EEG data as...");
    connect(m_actSaveAs, &QAction::triggered, this, &MainWindow::onFileSaveAs);

    m_actExit = new QAction("E&xit", this);
    m_actExit->setShortcut(QKeySequence::Quit);
    m_actExit->setStatusTip("Exit application");
    connect(m_actExit, &QAction::triggered, this, &MainWindow::onFileExit);
    
    // View actions
    m_actShowGrid = new QAction("&Show Grid", this);  // CHANGED LINE 71
    m_actShowGrid->setCheckable(true);
    m_actShowGrid->setChecked(true);
    m_actShowGrid->setStatusTip("Toggle grid display");
    connect(m_actShowGrid, &QAction::toggled, m_chartView, &EEGChartView::setShowGrid);
    
    m_actZoomIn = new QAction("Zoom &In", this);  // CHANGED LINE 77
    m_actZoomIn->setShortcut(QKeySequence::ZoomIn);
    m_actZoomIn->setStatusTip("Zoom in");
    connect(m_actZoomIn, &QAction::triggered, [this]() {
        m_chartView->setTimeRange(m_chartView->currentStartTime(), 
                                  m_chartView->currentDuration() * 0.8);
    });
    
    m_actZoomOut = new QAction("Zoom &Out", this);  // CHANGED LINE 84
    m_actZoomOut->setShortcut(QKeySequence::ZoomOut);
    m_actZoomOut->setStatusTip("Zoom out");
    connect(m_actZoomOut, &QAction::triggered, [this]() {
        m_chartView->setTimeRange(m_chartView->currentStartTime(), 
                                  m_chartView->currentDuration() * 1.25);
    });
    
    m_actPanLeft = new QAction("Pan &Left", this);  // CHANGED LINE 91
    m_actPanLeft->setShortcut(Qt::Key_Left);
    m_actPanLeft->setStatusTip("Pan left");
    connect(m_actPanLeft, &QAction::triggered, [this]() {
        double duration = m_chartView->currentDuration();
        m_chartView->setTimeRange(m_chartView->currentStartTime() - duration * 0.1, duration);
    });
    
    m_actPanRight = new QAction("Pan &Right", this);  // CHANGED LINE 98
    m_actPanRight->setShortcut(Qt::Key_Right);
    m_actPanRight->setStatusTip("Pan right");
    connect(m_actPanRight, &QAction::triggered, [this]() {
        double duration = m_chartView->currentDuration();
        m_chartView->setTimeRange(m_chartView->currentStartTime() + duration * 0.1, duration);
    });
    
    // Tools actions
    m_actStatistics = new QAction("&Statistics", this);  // CHANGED LINE 106
    m_actStatistics->setStatusTip("Show channel statistics");
    connect(m_actStatistics, &QAction::triggered, this, &MainWindow::onShowStatistics);

    m_actAbout = new QAction("&About", this);
    m_actAbout->setStatusTip("About EEG Data Processor");
    connect(m_actAbout, &QAction::triggered, this, &MainWindow::onShowAbout);

}

void MainWindow::onVisibleChannelsChanged(const QVector<int> &channels) {
    qDebug() << "Visible channels changed. Count:" << channels.size();
    
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
    viewMenu->addSeparator();
    viewMenu->addAction(m_actZoomIn);
    viewMenu->addAction(m_actZoomOut);
    viewMenu->addAction(m_actPanLeft);
    viewMenu->addAction(m_actPanRight);
    
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
    viewToolBar->addSeparator();
    viewToolBar->addAction(m_actZoomIn);
    viewToolBar->addAction(m_actZoomOut);
    viewToolBar->addAction(m_actPanLeft);
    viewToolBar->addAction(m_actPanRight);
    
    // Tools toolbar
    QToolBar *toolsToolBar = addToolBar("Tools");
    toolsToolBar->addAction(m_actStatistics);
}

void MainWindow::createDockWidgets() {
    // Channel list dock
    m_channelDock = new QDockWidget("Channels", this);
    m_channelDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    
    m_channelList = new QListWidget();
    m_channelList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(m_channelList, &QListWidget::itemSelectionChanged, 
            this, &MainWindow::onChannelListSelectionChanged);
    
    m_channelDock->setWidget(m_channelList);
    addDockWidget(Qt::LeftDockWidgetArea, m_channelDock);
    
    // Processing dock
    m_processingDock = new QDockWidget("Signal Processing", this);
    m_processingDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    
    m_processingWidget = new QWidget();
    QVBoxLayout *procLayout = new QVBoxLayout(m_processingWidget);
    
    // Channel selection
    QGroupBox *channelGroup = new QGroupBox("Channel Selection");
    QFormLayout *channelLayout = new QFormLayout(channelGroup);
    m_channelSelectSpin = new QSpinBox();
    m_channelSelectSpin->setRange(0, 0);
    m_channelSelectSpin->setValue(0);
    channelLayout->addRow("Channel:", m_channelSelectSpin);
    procLayout->addWidget(channelGroup);
    
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
    
    m_notchFreqSpin = new QDoubleSpinBox();
    m_notchFreqSpin->setRange(50.0, 60.0);
    m_notchFreqSpin->setValue(50.0);
    m_notchFreqSpin->setSuffix(" Hz");
    
    QPushButton *notchButton = new QPushButton("Apply Notch Filter");
    connect(notchButton, &QPushButton::clicked, this, &MainWindow::onNotchFilterApply);
    
    notchLayout->addRow("Frequency:", m_notchFreqSpin);
    notchLayout->addRow(notchButton);
    
    procLayout->addWidget(notchGroup);
    
    // Montage
    QGroupBox *montageGroup = new QGroupBox("Montage");
    QFormLayout *montageLayout = new QFormLayout(montageGroup);
    
    m_montageCombo = new QComboBox();
    m_montageCombo->addItems({"Bipolar", "Average Reference", "Laplacian"});
    
    QPushButton *montageButton = new QPushButton("Apply Montage");
    connect(montageButton, &QPushButton::clicked, this, &MainWindow::onMontageApply);
    
    montageLayout->addRow("Type:", m_montageCombo);
    montageLayout->addRow(montageButton);
    
    procLayout->addWidget(montageGroup);
    
    // Display controls
    QGroupBox *displayGroup = new QGroupBox("Display");
    QFormLayout *displayLayout = new QFormLayout(displayGroup);
    
    m_timeStartSpin = new QDoubleSpinBox();
    m_timeStartSpin->setRange(0.0, 1000.0);
    m_timeStartSpin->setValue(0.0);
    m_timeStartSpin->setSuffix(" s");
    
    m_timeDurationSpin = new QDoubleSpinBox();
    m_timeDurationSpin->setRange(0.1, 60.0);
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
        this, "Open EEG Data File", "",
        "EEG Files (*.edf *.csv *.txt *.dat);;All Files (*.*)");
    
    if (filePath.isEmpty()) return;
    
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    
    // Simulate progress (in real app, update from loading thread)
    for (int i = 0; i <= 100; i += 10) {
        m_progressBar->setValue(i);
        QCoreApplication::processEvents();
    }
    
    if (m_eegData->loadFromFile(filePath)) {
        m_currentFilePath = filePath;

        m_chartView->selectAllChannels();

        m_chartView->updateChart();
        setWindowTitle(QString("EEG Data Processor - %1").arg(QFileInfo(filePath).fileName()));
        
        // Update channel selection spin box
        int channelCount = m_eegData->channelCount();
        m_channelSelectSpin->setRange(0, qMax(0, channelCount - 1));
        
        // Update display controls
        double duration = m_eegData->duration();
        m_timeDurationSpin->setRange(0.1, qMin(60.0, duration));
        m_timeDurationSpin->setValue(qMin(10.0, duration));
        
        m_chartView->setTimeRange(0, qMin(10.0, duration));
        
        QMessageBox::information(this, "Success", 
                                QString("Loaded %1 channels with %2 seconds of data")
                                .arg(channelCount).arg(duration, 0, 'f', 2));
    } else {
        QMessageBox::warning(this, "Error", "Failed to load EEG data file");
    }
    
    m_progressBar->setVisible(false);
}

void MainWindow::onFileSave() {
    if (m_currentFilePath.isEmpty()) {
        onFileSaveAs();
        return;
    }
    
    if (m_eegData->saveToFile(m_currentFilePath)) {
        QMessageBox::information(this, "Success", "Data saved successfully");
    } else {
        QMessageBox::warning(this, "Error", "Failed to save data");
    }
}

void MainWindow::onFileSaveAs() {
    QString filePath = QFileDialog::getSaveFileName(
        this, "Save EEG Data File", m_currentFilePath,
        "EDF Files (*.edf);;CSV Files (*.csv);;Text Files (*.txt);;All Files (*.*)");
    
    if (filePath.isEmpty()) return;
    
    m_currentFilePath = filePath;
    onFileSave();
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
    int channel = m_channelSelectSpin->value();
    double notchFreq = m_notchFreqSpin->value();
    
    if (channel < 0 || channel >= m_eegData->channelCount()) {
        QMessageBox::warning(this, "Error", "Invalid channel selection");
        return;
    }
    
    m_eegData->applyNotchFilter(channel, notchFreq);
    m_chartView->updateChart();
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

void MainWindow::onChannelListSelectionChanged() {
    QVector<int> selectedChannels;
    for (auto item : m_channelList->selectedItems()) {
        selectedChannels.append(m_channelList->row(item));
    }
    m_chartView->setVisibleChannels(selectedChannels);
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
    statsDialog.resize(600, 400);
    
    QVBoxLayout *layout = new QVBoxLayout(&statsDialog);
    
    QTableWidget *table = new QTableWidget();
    table->setColumnCount(8);
    table->setHorizontalHeaderLabels({"Channel", "Label", "Samples", 
                                      "Rate (Hz)", "Mean (μV)", "StdDev (μV)", 
                                      "Min (μV)", "Max (μV)"});
    
    int channelCount = m_eegData->channelCount();
    table->setRowCount(channelCount);
    
    auto means = m_eegData->channelMeans();
    auto stddevs = m_eegData->channelStdDevs();
    
    for (int i = 0; i < channelCount; ++i) {
        const EEGChannel &channel = m_eegData->channel(i);
        
        table->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        table->setItem(i, 1, new QTableWidgetItem(channel.label));
        table->setItem(i, 2, new QTableWidgetItem(QString::number(channel.data.size())));
        table->setItem(i, 3, new QTableWidgetItem(QString::number(channel.samplingRate, 'f', 1)));
        table->setItem(i, 4, new QTableWidgetItem(QString::number(means[i], 'f', 2)));
        table->setItem(i, 5, new QTableWidgetItem(QString::number(stddevs[i], 'f', 2)));
        
        auto minMax = std::minmax_element(channel.data.begin(), channel.data.end());
        table->setItem(i, 6, new QTableWidgetItem(QString::number(*minMax.first, 'f', 2)));
        table->setItem(i, 7, new QTableWidgetItem(QString::number(*minMax.second, 'f', 2)));
    }
    
    table->resizeColumnsToContents();
    layout->addWidget(table);
    
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, &statsDialog, &QDialog::reject);
    layout->addWidget(buttons);
    
    statsDialog.exec();
}

void MainWindow::onShowAbout() {
    QMessageBox::about(this, "About EEG Data Processor",
        "<h3>EEG Data Processor</h3>"
        "<p>Version 1.0.0</p>"
        "<p>A comprehensive application for viewing and processing "
        "electroencephalography (EEG) data.</p>"
        "<p>Features include:</p>"
        "<ul>"
        "<li>Load and save EDF/CSV files</li>"
        "<li>Multi-channel EEG visualization</li>"
        "<li>Signal processing filters (bandpass, notch, etc.)</li>"
        "<li>Gain, offset, and normalization</li>"
        "<li>EEG montage support</li>"
        "<li>Interactive chart navigation</li>"
        "</ul>"
        "<p>© 2024 NeuroLab Research</p>");
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
        m_channelList->addItem(item);
    }
    
    // Update channel selection spin box
    int channelCount = m_eegData->channelCount();
    m_channelSelectSpin->setRange(0, qMax(0, channelCount - 1));
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_eegData && !m_eegData->isEmpty()) {
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
            event->ignore();
        }
    } else {
        event->accept();
    }
}

void MainWindow::onFileExit() {
    close(); 
}

void MainWindow::onChannelSelected() {
    // For now, empty implementation:
    Q_UNUSED(this);
}

void MainWindow::applySignalProcessing() {
    // Empty placeholder or implement as needed
    Q_UNUSED(this);
}