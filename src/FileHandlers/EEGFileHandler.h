#pragma once
#include <QString>

class EEGData;

namespace EEGFileHandler {
    bool loadFile(const QString &filePath, EEGData &data);
    bool saveFile(const QString &filePath, const EEGData &data);
}