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
    if (allChannelData.isEmpty()) return;
    
    int numSamples = allChannelData[0].size();
    int numChannels = allChannelData.size();
    
    // Safer manual implementation instead of Eigen
    QVector<double> average(numSamples, 0.0);
    
    // Compute average at each time point
    for (int s = 0; s < numSamples; ++s) {
        double sum = 0.0;
        for (int ch = 0; ch < numChannels; ++ch) {
            sum += allChannelData[ch][s];
        }
        average[s] = sum / numChannels;
    }
    
    // Subtract average from each channel
    for (int ch = 0; ch < numChannels; ++ch) {
        for (int s = 0; s < numSamples; ++s) {
            allChannelData[ch][s] -= average[s];
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
    if (allChannelData.size() < 2) return;
    
    // Standard bipolar pairs for 10-20 system
    QVector<QPair<int, int>> pairs;
    
    // Left hemisphere
    pairs.append({findChannelIndex(channelLabels, "Fp1"), findChannelIndex(channelLabels, "F7")});
    pairs.append({findChannelIndex(channelLabels, "F7"), findChannelIndex(channelLabels, "T3")});
    pairs.append({findChannelIndex(channelLabels, "T3"), findChannelIndex(channelLabels, "T5")});
    pairs.append({findChannelIndex(channelLabels, "T5"), findChannelIndex(channelLabels, "O1")});
    
    // Right hemisphere
    pairs.append({findChannelIndex(channelLabels, "Fp2"), findChannelIndex(channelLabels, "F8")});
    pairs.append({findChannelIndex(channelLabels, "F8"), findChannelIndex(channelLabels, "T4")});
    pairs.append({findChannelIndex(channelLabels, "T4"), findChannelIndex(channelLabels, "T6")});
    pairs.append({findChannelIndex(channelLabels, "T6"), findChannelIndex(channelLabels, "O2")});
    
    // Midline
    pairs.append({findChannelIndex(channelLabels, "Fz"), findChannelIndex(channelLabels, "Cz")});
    pairs.append({findChannelIndex(channelLabels, "Cz"), findChannelIndex(channelLabels, "Pz")});
    
    // Calculate bipolar channels
    QVector<QVector<double>> bipolarData;
    for (const auto &pair : pairs) {
        int idx1 = pair.first;
        int idx2 = pair.second;
        
        if (idx1 >= 0 && idx1 < allChannelData.size() &&
            idx2 >= 0 && idx2 < allChannelData.size()) {
            
            QVector<double> diff(allChannelData[idx1].size());
            for (int j = 0; j < diff.size(); ++j) {
                diff[j] = allChannelData[idx1][j] - allChannelData[idx2][j];
            }
            bipolarData.append(diff);
        }
    }
    
    if (!bipolarData.isEmpty()) {
        allChannelData = bipolarData;
    }
}

inline void applyLaplacianMontage(QVector<QVector<double>> &allChannelData) {
    if (allChannelData.size() < 3) return;
    
    QVector<QVector<double>> laplacianData = allChannelData;
    int numChannels = allChannelData.size();
    int numSamples = allChannelData[0].size();
    
    for (int s = 0; s < numSamples; ++s) {
        for (int ch = 0; ch < numChannels; ++ch) {
            double neighborSum = 0.0;
            int neighborCount = 0;
            
            if (ch > 0) {
                neighborSum += allChannelData[ch-1][s];
                neighborCount++;
            }
            if (ch < numChannels - 1) {
                neighborSum += allChannelData[ch+1][s];
                neighborCount++;
            }
            
            if (neighborCount > 0) {
                laplacianData[ch][s] = allChannelData[ch][s] - (neighborSum / neighborCount);
            } else {
                laplacianData[ch][s] = allChannelData[ch][s];
            }
        }
    }
    allChannelData = laplacianData;
}

enum MontageType {
    MontageBipolar,
    MontageAverageReference,
    MontageLaplacian
};

inline void applyMontage(QVector<QVector<double>> &allChannelData, 
                        const QVector<QString> &channelLabels,
                        MontageType montage) {
    if (allChannelData.isEmpty()) return;
    
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