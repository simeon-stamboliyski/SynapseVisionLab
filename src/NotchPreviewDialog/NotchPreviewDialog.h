#ifndef NOTCHPREVIEWDIALOG_H
#define NOTCHPREVIEWDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QFileInfo>
#include "../DataModels/EEGData.h"
#include "../Visualization/EEGChartView.h"

class NotchPreviewDialog : public QDialog {
    Q_OBJECT
    
public:
    NotchPreviewDialog(EEGData *originalData, EEGData *filteredData, 
                      double notchFreq, QWidget *parent = nullptr);
    
private slots:
    void onOverwrite();
    void onSaveAs();
    void onCancel();
    
private:
    EEGData *m_originalData;
    EEGData *m_filteredData;
    EEGData *m_tempData;  // Copy for preview
    double m_notchFreq;
    
    EEGChartView *m_originalChart;
    EEGChartView *m_filteredChart;
};

#endif