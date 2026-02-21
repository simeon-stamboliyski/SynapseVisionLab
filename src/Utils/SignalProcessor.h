#pragma once
#include <fftw3.h>
#include <Eigen/Dense>
#include <iir/Butterworth.h>
#include <QVector>
#include <QDebug>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <vector>
#include <complex>
#include <memory>

namespace SignalProcessor {

// ================== BASIC OPERATIONS ==================

inline void applyGain(QVector<double> &data, double gain) {
    if (data.isEmpty()) return;
    for (auto &val : data) val *= gain;
}

inline void applyOffset(QVector<double> &data, double offset) {
    if (data.isEmpty()) return;
    for (auto &val : data) val += offset;
}

inline void normalize(QVector<double> &data, double minVal = 0.0, double maxVal = 1.0) {
    if (data.isEmpty()) return;
    
    auto minMax = std::minmax_element(data.begin(), data.end());
    double currentMin = *minMax.first;
    double currentMax = *minMax.second;
    double range = currentMax - currentMin;
    
    if (range > 0) {
        double targetRange = maxVal - minVal;
        for (auto &val : data) {
            val = minVal + ((val - currentMin) / range) * targetRange;
        }
    }
}

// ================== PROFESSIONAL BANDPASS FILTER ==================

class BandpassFilter {
private:
    Iir::Butterworth::BandPass<4> filter;
    double lastSamplingRate;
    double lastLowCut;
    double lastHighCut;
    
public:
    BandpassFilter() : lastSamplingRate(0), lastLowCut(0), lastHighCut(0) {}
    
    void design(double lowCut, double highCut, double fs) {
        if (fs == lastSamplingRate && lowCut == lastLowCut && highCut == lastHighCut) {
            return;
        }
        
        filter.setup(fs, lowCut, highCut);
        
        lastSamplingRate = fs;
        lastLowCut = lowCut;
        lastHighCut = highCut;
    }
    
    void apply(QVector<double> &data) {
        for (auto &sample : data) {
            sample = filter.filter(sample);
        }
    }
    
    void applyZeroPhase(QVector<double> &data) {
        apply(data);
        std::reverse(data.begin(), data.end());
        apply(data);
        std::reverse(data.begin(), data.end());
    }
};

// Global instance
inline BandpassFilter gBandpassFilter;

// Standalone function
inline void bandpassFilter(QVector<double> &data, double samplingRate, 
                          double lowCutHz, double highCutHz) {
    if (data.isEmpty() || samplingRate <= 0) return;
    if (lowCutHz <= 0 || highCutHz <= lowCutHz || highCutHz >= samplingRate / 2) {
        qWarning() << "Invalid bandpass frequencies";
        return;
    }
    
    gBandpassFilter.design(lowCutHz, highCutHz, samplingRate);
    gBandpassFilter.applyZeroPhase(data);
}

// ================== NOTCH FILTER ==================

inline void notchFilter(QVector<double> &data, double samplingRate, double notchFreq = 50.0) {
    if (data.size() < 4 || samplingRate <= 0) return;
    
    // Simple IIR notch filter
    double w0 = 2.0 * M_PI * notchFreq / samplingRate;
    double alpha = sin(w0) / 2.0;
    double b0 = 1.0;
    double b1 = -2.0 * cos(w0);
    double b2 = 1.0;
    double a0 = 1.0 + alpha;
    double a1 = -2.0 * cos(w0);
    double a2 = 1.0 - alpha;
    
    QVector<double> y(data.size());
    if (data.size() >= 2) {
        y[0] = data[0];
        y[1] = data[1];
    }
    
    for (int i = 2; i < data.size(); ++i) {
        y[i] = (b0 * data[i] + b1 * data[i-1] + b2 * data[i-2] 
                - a1 * y[i-1] - a2 * y[i-2]) / a0;
    }
    
    data = y;
}

// ================== STATISTICS ==================

inline double mean(const QVector<double> &data) {
    if (data.isEmpty()) return 0.0;
    return std::accumulate(data.begin(), data.end(), 0.0) / data.size();
}

inline double standardDeviation(const QVector<double> &data) {
    if (data.size() < 2) return 0.0;
    double m = mean(data);
    double variance = 0.0;
    for (double val : data) {
        double diff = val - m;
        variance += diff * diff;
    }
    return std::sqrt(variance / data.size());
}

inline double minValue(const QVector<double> &data) {
    if (data.isEmpty()) return 0.0;
    return *std::min_element(data.begin(), data.end());
}

inline double maxValue(const QVector<double> &data) {
    if (data.isEmpty()) return 0.0;
    return *std::max_element(data.begin(), data.end());
}

// ================== MONTAGES ==================
inline void applyAverageReference(QVector<QVector<double>> &allChannelData) {
    if (allChannelData.isEmpty()) {
        qWarning() << "Average Reference: No data";
        return;
    }
    
    int numSamples = allChannelData[0].size();
    int numChannels = allChannelData.size();
    
    if (numChannels == 0 || numSamples == 0) {
        qWarning() << "Average Reference: Invalid dimensions";
        return;
    }
    
    QVector<double> average(numSamples, 0.0);
    
    // Compute average at each time point
    for (int s = 0; s < numSamples; ++s) {
        double sum = 0.0;
        int validChannels = 0;
        bool hasValidData = false;
        
        for (int ch = 0; ch < numChannels; ++ch) {
            double val = allChannelData[ch][s];
            
            // More robust check
            if (std::isfinite(val)) {  // isfinite checks for both NaN and Inf
                sum += val;
                validChannels++;
                hasValidData = true;
            } else {
                // Replace invalid value with 0 to prevent propagation
                allChannelData[ch][s] = 0.0;
            }
        }
        
        if (validChannels > 0 && hasValidData) {
            average[s] = sum / validChannels;
        } else {
            average[s] = 0.0;
        }
        
        // Ensure average itself is valid
        if (!std::isfinite(average[s])) {
            average[s] = 0.0;
        }
    }
    
    // Subtract average from each channel
    for (int ch = 0; ch < numChannels; ++ch) {
        for (int s = 0; s < numSamples; ++s) {
            double original = allChannelData[ch][s];
            double result = original - average[s];
            
            // Check if result is valid
            if (std::isfinite(result)) {
                allChannelData[ch][s] = result;
            } else {
                qDebug() << "NaN detected in channel" << ch << "at sample" << s 
                         << "original:" << original << "avg:" << average[s];
                allChannelData[ch][s] = 0.0;
            }
        }
    }
}

inline int findChannelIndex(const QVector<QString> &labels, const QString &name) {
    for (int i = 0; i < labels.size(); ++i) {
        if (labels[i].contains(name, Qt::CaseInsensitive)) {
            return i;
        }
    }
    return -1;
}

// Professional bipolar montage with standard pairs
inline void applyBipolarMontage(QVector<QVector<double>> &allChannelData,
                                const QVector<QString> &channelLabels) {
    if (allChannelData.size() < 2) {
        qWarning() << "Bipolar Montage: Need at least 2 channels, have" << allChannelData.size();
        return;
    }
    
    qDebug() << "Generating bipolar pairs from" << channelLabels.size() << "channels";
    
    QVector<QPair<int, int>> pairs;
    
    // Strategy 1: Pair adjacent channels by numeric suffix
    // This works for channels like F3, F4, C3, C4, P3, P4, O1, O2, etc.
    QMap<QString, QVector<int>> channelGroups;
    
    for (int i = 0; i < channelLabels.size(); ++i) {
        QString label = channelLabels[i];
        
        // Extract base name (remove numbers)
        QString baseName = label;
        while (!baseName.isEmpty() && baseName.back().isDigit()) {
            baseName.chop(1);
        }
        
        channelGroups[baseName].append(i);
    }
    
    // Pair channels within the same group (e.g., F3 with F4, C3 with C4)
    for (const auto &group : channelGroups) {
        const QVector<int> &indices = group;
        
        // Pair odd/even numbered channels
        QVector<int> left, right;
        for (int idx : indices) {
            QString label = channelLabels[idx];
            if (label.contains('3') || label.contains('1') || label.contains('5') || 
                label.contains('7') || label.contains('9')) {
                left.append(idx);
            } else if (label.contains('4') || label.contains('2') || label.contains('6') || 
                      label.contains('8') || label.contains('0')) {
                right.append(idx);
            }
        }
        
        // Pair left and right channels
        for (int i = 0; i < qMin(left.size(), right.size()); ++i) {
            pairs.append({left[i], right[i]});
            qDebug() << "Created bipolar pair:" << channelLabels[left[i]] 
                     << "-" << channelLabels[right[i]];
        }
    }
    
    if (pairs.isEmpty()) {
        qDebug() << "No pattern-based pairs found, using consecutive channels";
        for (int i = 0; i < channelLabels.size() - 1; ++i) {
            pairs.append({i, i+1});
            qDebug() << "Created consecutive pair:" << channelLabels[i] 
                     << "-" << channelLabels[i+1];
        }
    }
    
    if (pairs.isEmpty()) {
        qWarning() << "Bipolar Montage: Could not create any pairs";
        return;
    }
    
    // Calculate bipolar channels
    QVector<QVector<double>> bipolarData;
    int numSamples = allChannelData[0].size();
    
    for (const auto &pair : pairs) {
        int idx1 = pair.first;
        int idx2 = pair.second;
        
        QVector<double> diff(numSamples, 0.0);
        bool hasValidData = false;
        
        for (int j = 0; j < numSamples; ++j) {
            double val1 = allChannelData[idx1][j];
            double val2 = allChannelData[idx2][j];
            
            if (std::isfinite(val1) && std::isfinite(val2)) {
                diff[j] = val1 - val2;
                hasValidData = true;
            } else {
                diff[j] = 0.0;
            }
        }
        
        if (hasValidData) {
            bipolarData.append(diff);
        }
    }
    
    if (!bipolarData.isEmpty()) {
        allChannelData = bipolarData;
        qDebug() << "Bipolar montage created" << bipolarData.size() << "channels";
    } else {
        qWarning() << "Bipolar Montage: No valid bipolar data generated";
    }
}

inline void applyLaplacianMontage(QVector<QVector<double>> &allChannelData) {
    if (allChannelData.size() < 3) {
        qWarning() << "Laplacian Montage: Need at least 3 channels, have" << allChannelData.size();
        return;
    }
    
    // First, clean the input data
    for (int ch = 0; ch < allChannelData.size(); ++ch) {
        for (int s = 0; s < allChannelData[ch].size(); ++s) {
            if (!std::isfinite(allChannelData[ch][s])) {
                allChannelData[ch][s] = 0.0;
            }
        }
    }
    
    QVector<QVector<double>> laplacianData = allChannelData;
    int numChannels = allChannelData.size();
    int numSamples = allChannelData[0].size();
    int nanCount = 0;
    
    for (int s = 0; s < numSamples; ++s) {
        for (int ch = 0; ch < numChannels; ++ch) {
            double neighborSum = 0.0;
            int neighborCount = 0;
            
            // Check left neighbor
            if (ch > 0) {
                double val = allChannelData[ch-1][s];
                if (std::isfinite(val)) {
                    neighborSum += val;
                    neighborCount++;
                }
            }
            
            // Check right neighbor
            if (ch < numChannels - 1) {
                double val = allChannelData[ch+1][s];
                if (std::isfinite(val)) {
                    neighborSum += val;
                    neighborCount++;
                }
            }
            
            double currentVal = allChannelData[ch][s];
            
            if (neighborCount > 0 && std::isfinite(currentVal)) {
                double neighborAvg = neighborSum / neighborCount;
                // Ensure neighborAvg is finite
                if (std::isfinite(neighborAvg)) {
                    laplacianData[ch][s] = currentVal - neighborAvg;
                } else {
                    laplacianData[ch][s] = currentVal;
                    nanCount++;
                }
            } else {
                laplacianData[ch][s] = std::isfinite(currentVal) ? currentVal : 0.0;
            }
            
            // Final safety check
            if (!std::isfinite(laplacianData[ch][s])) {
                laplacianData[ch][s] = 0.0;
                nanCount++;
            }
        }
    }
    
    allChannelData = laplacianData;
    
    if (nanCount > 0) {
        qDebug() << "Laplacian montage applied with" << nanCount << "NaN corrections";
    } else {
        qDebug() << "Laplacian montage applied successfully";
    }
}

enum MontageType {
    MontageBipolar,
    MontageAverageReference,
    MontageLaplacian
};

inline void applyMontage(QVector<QVector<double>> &allChannelData, 
                        const QVector<QString> &channelLabels,
                        MontageType montage) {
    if (allChannelData.isEmpty()) {
        qWarning() << "Montage: No data";
        return;
    }

    for (int ch = 0; ch < allChannelData.size(); ++ch) {
        for (int s = 0; s < allChannelData[ch].size(); ++s) {
            if (std::isnan(allChannelData[ch][s])) {
                qDebug() << "NaN found in input - channel" << ch << "sample" << s;
                allChannelData[ch][s] = 0.0; 
            }
        }
    }
    
    qDebug() << "Applying montage type:" << montage;
    
    switch (montage) {
        case MontageAverageReference:
            applyAverageReference(allChannelData);
            break;
        case MontageBipolar:
            applyBipolarMontage(allChannelData, channelLabels);
            break;
        case MontageLaplacian:
            applyLaplacianMontage(allChannelData);
            break;
        default:
            qWarning() << "Unknown montage type";
            return;
    }
    
    // Final validation
    if (!allChannelData.isEmpty()) {
        qDebug() << "Montage complete. Channels:" << allChannelData.size() 
                 << "Samples:" << allChannelData[0].size();
    }
}

// ================== FREQUENCY ANALYSIS ==================

inline QVector<double> powerSpectrum(const QVector<double> &data, double samplingRate) {
    QVector<double> spectrum;
    if (data.isEmpty() || samplingRate <= 0) return spectrum;
    
    int N = data.size();
    spectrum.resize(N/2 + 1);
    
    fftw_complex *in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    fftw_complex *out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    
    for (int i = 0; i < N; ++i) {
        in[i][0] = data[i];
        in[i][1] = 0.0;
    }
    
    fftw_plan p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(p);
    
    for (int i = 0; i <= N/2; ++i) {
        double real = out[i][0];
        double imag = out[i][1];
        spectrum[i] = std::sqrt(real*real + imag*imag) / N;
    }
    
    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(out);
    
    return spectrum;
}

struct BandPower {
    double delta;    // 0.5-4 Hz
    double theta;    // 4-8 Hz  
    double alpha;    // 8-13 Hz
    double beta;     // 13-30 Hz
    double gamma;    // 30-100 Hz
};

inline BandPower calculateBandPower(const QVector<double> &data, double samplingRate) {
    BandPower power = {0.0, 0.0, 0.0, 0.0, 0.0};
    
    auto spectrum = powerSpectrum(data, samplingRate);
    if (spectrum.isEmpty()) return power;
    
    double freqResolution = samplingRate / (2.0 * spectrum.size());
    
    for (int i = 0; i < spectrum.size(); ++i) {
        double freq = i * freqResolution;
        double p = spectrum[i] * spectrum[i];
        
        if (freq >= 0.5 && freq < 4.0) power.delta += p;
        else if (freq >= 4.0 && freq < 8.0) power.theta += p;
        else if (freq >= 8.0 && freq < 13.0) power.alpha += p;
        else if (freq >= 13.0 && freq < 30.0) power.beta += p;
        else if (freq >= 30.0 && freq < 100.0) power.gamma += p;
    }
    
    return power;
}

inline void removeDC(QVector<double> &data) {
    if (data.isEmpty()) return;
    double mean = std::accumulate(data.begin(), data.end(), 0.0) / data.size();
    for (auto &val : data) val -= mean;
}

inline QVector<double> extractTimeWindow(const QVector<double> &data, 
                                        double samplingRate,
                                        double startTime, double duration) {
    QVector<double> result;
    if (data.isEmpty() || samplingRate <= 0) return result;
    
    int startSample = static_cast<int>(startTime * samplingRate);
    int endSample = static_cast<int>((startTime + duration) * samplingRate);
    
    startSample = std::max(0, startSample);
    endSample = std::min(data.size() - 1, endSample);
    
    if (startSample <= endSample) {
        result.reserve(endSample - startSample + 1);
        for (int i = startSample; i <= endSample; ++i) {
            result.append(data[i]);
        }
    }
    return result;
}

}