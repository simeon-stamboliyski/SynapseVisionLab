#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QAction>
#include <QMenu>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QListWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QProgressBar>
#include "../DataModels/EEGData.h"
#include "../Visualization/EEGChartView.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    
private slots:
    void onFileOpen();
    void onFileSave();
    void onFileSaveAs();
    void onFileExit();
    
    void onChannelSelected();
    void onChannelListSelectionChanged();
    
    void onFilterApply();
    void onGainApply();
    void onOffsetApply();
    void onNormalizeApply();
    void onDCRemoveApply();
    void onNotchFilterApply();
    void onMontageApply();
    
    void onTimeRangeChanged(double start, double duration);
    void onVerticalScaleChanged(double value);
    void onOffsetScaleChanged(double value);
    
    void onShowStatistics();
    void onShowAbout();
    
    void updateStatusBar();
    void updateChannelList();
    
private:
    void createActions();
    void createMenus();
    void createToolBars();
    void createDockWidgets();
    void createCentralWidget();
    void createStatusBar();
    
    void applySignalProcessing();
    
private:
    // Core data and view
    EEGData *m_eegData;
    EEGChartView *m_chartView;
    
    // UI components
    QDockWidget *m_channelDock;
    QListWidget *m_channelList;
    
    QDockWidget *m_processingDock;
    QWidget *m_processingWidget;
    
    // Processing controls
    QComboBox *m_filterTypeCombo;
    QDoubleSpinBox *m_lowCutSpin;
    QDoubleSpinBox *m_highCutSpin;
    QDoubleSpinBox *m_gainSpin;
    QDoubleSpinBox *m_offsetSpin;
    QDoubleSpinBox *m_notchFreqSpin;
    QComboBox *m_montageCombo;
    QSpinBox *m_channelSelectSpin;
    
    // Display controls
    QDoubleSpinBox *m_timeStartSpin;
    QDoubleSpinBox *m_timeDurationSpin;
    QDoubleSpinBox *m_verticalScaleSpin;
    QDoubleSpinBox *m_offsetScaleSpin;
    
    // Menu actions
    QAction *m_actOpen;
    QAction *m_actSave;
    QAction *m_actSaveAs;
    QAction *m_actExit;
    
    QAction *m_actShowGrid;
    QAction *m_actZoomIn;
    QAction *m_actZoomOut;
    QAction *m_actPanLeft;
    QAction *m_actPanRight;
    
    QAction *m_actStatistics;
    QAction *m_actAbout;
    
    // Status bar
    QLabel *m_statusFile;
    QLabel *m_statusChannels;
    QLabel *m_statusDuration;
    QLabel *m_statusSamplingRate;
    QProgressBar *m_progressBar;
    
    QString m_currentFilePath;
};

#endif // MAINWINDOW_H