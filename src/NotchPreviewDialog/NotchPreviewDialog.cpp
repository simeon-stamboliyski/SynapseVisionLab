#include "NotchPreviewDialog.h"
#include <QFileDialog>
#include <QSplitter>
#include <QGroupBox>

NotchPreviewDialog::NotchPreviewDialog(EEGData *originalData, EEGData *filteredData, 
                                       double notchFreq, QWidget *parent)
    : QDialog(parent), m_originalData(originalData), m_filteredData(filteredData), 
      m_notchFreq(notchFreq) {
    
    setWindowTitle("Notch Filter Preview");
    setMinimumSize(1200, 600);

    m_tempData = filteredData->clone();
    m_tempData->setParent(this);
    
    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Instructions
    QLabel *infoLabel = new QLabel(
        QString("Preview of %1 Hz Notch Filter - Compare original vs filtered")
        .arg(notchFreq)
    );
    infoLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(infoLabel);
    
    // Splitter for side-by-side comparison
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    
    // Original signal
    QGroupBox *originalBox = new QGroupBox("Original Signal");
    QVBoxLayout *originalLayout = new QVBoxLayout(originalBox);
    m_originalChart = new EEGChartView();
    m_originalChart->setEEGData(originalData);
    m_originalChart->selectAllChannels();
    originalLayout->addWidget(m_originalChart);
    splitter->addWidget(originalBox);
    
    // Filtered signal
    QGroupBox *filteredBox = new QGroupBox("Filtered Signal");
    QVBoxLayout *filteredLayout = new QVBoxLayout(filteredBox);
    m_filteredChart = new EEGChartView();
    m_filteredChart->setEEGData(m_tempData);
    m_filteredChart->selectAllChannels();
    filteredLayout->addWidget(m_filteredChart);
    splitter->addWidget(filteredBox);
    
    mainLayout->addWidget(splitter);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    QPushButton *overwriteBtn = new QPushButton("Overwrite Current File");
    QPushButton *saveAsBtn = new QPushButton("Save As New File");
    QPushButton *cancelBtn = new QPushButton("Cancel");
    
    connect(overwriteBtn, &QPushButton::clicked, this, &NotchPreviewDialog::onOverwrite);
    connect(saveAsBtn, &QPushButton::clicked, this, &NotchPreviewDialog::onSaveAs);
    connect(cancelBtn, &QPushButton::clicked, this, &NotchPreviewDialog::onCancel);
    
    buttonLayout->addWidget(overwriteBtn);
    buttonLayout->addWidget(saveAsBtn);
    buttonLayout->addWidget(cancelBtn);
    
    mainLayout->addLayout(buttonLayout);
}

void NotchPreviewDialog::onOverwrite() {
    int reply = QMessageBox::question(this, "Confirm Overwrite",
        "This will permanently replace the original file. Continue?",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        m_originalData->copyFrom(m_tempData);
        
        QMessageBox::information(this, "Success", 
            "File has been updated with notch filter");
        accept();
    }
}

void NotchPreviewDialog::onSaveAs() {
    
    QString defaultName = QFileInfo(m_originalData->fileName()).baseName() + "_notch.csv";
    
    // Create dialog manually instead of using static function
    QFileDialog dialog(nullptr);
    dialog.setWindowTitle("Save Filtered EEG Data");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilter("EDF Files (*.edf);;CSV Files (*.csv);;All Files (*)");
    dialog.selectFile(defaultName);
    dialog.setOption(QFileDialog::DontUseNativeDialog, true); // Force Qt dialog
    dialog.setModal(true);
    
    // Force it to the front
    dialog.raise();
    dialog.activateWindow();
    
    if (dialog.exec() == QDialog::Accepted) {
        QString filePath = dialog.selectedFiles().first();
        
        bool success = m_tempData->saveToFile(filePath);
        
        if (success) {
            QMessageBox::information(this, "Success", 
                "Filtered data saved to:\n" + QFileInfo(filePath).absoluteFilePath());
            accept();
        } else {
            QMessageBox::warning(this, "Error", "Failed to save file");
        }
    } else {
        qDebug() << "Dialog rejected or cancelled";
    }
}

void NotchPreviewDialog::onCancel() {
    reject();
}