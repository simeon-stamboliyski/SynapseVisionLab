#pragma once
#include <fftw3.h>
#include <QVector>
#include <cmath>
#include <algorithm>
#include <numeric>

namespace SignalProcessor {

// ================== BASIC OPERATIONS ==================

inline void applyGain(QVector<double> &data, double gain) {
    if (data.isEmpty()) return;
    for (auto &val : data) {
        val *= gain;
    }
}

inline void applyOffset(QVector<double> &data, double offset) {
    if (data.isEmpty()) return;
    for (auto &val : data) {
        val += offset;
    }
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

// ================== FILTERS ==================

inline void removeDC(QVector<double> &data) {
    if (data.isEmpty()) return;
    
    double sum = std::accumulate(data.begin(), data.end(), 0.0);
    double mean = sum / data.size();
    
    for (auto &val : data) {
        val -= mean;
    }
}

inline void movingAverage(QVector<double> &data, int windowSize) {
    if (data.isEmpty() || windowSize < 1) return;
    
    QVector<double> result = data;
    int halfWindow = windowSize / 2;
    
    for (int i = halfWindow; i < data.size() - halfWindow; ++i) {
        double sum = 0.0;
        for (int j = i - halfWindow; j <= i + halfWindow; ++j) {
            sum += data[j];
        }
        result[i] = sum / windowSize;
    }
    
    data = result;
}

// Simple bandpass filter (moving average for high-cut + DC removal for low-cut)
inline void bandpassFilter(QVector<double> &data, double samplingRate, 
                          double lowCutHz, double highCutHz) {
    if (data.isEmpty() || samplingRate <= 0) return;
    
    // Remove DC (high-pass effect)
    removeDC(data);
    
    // Apply moving average for low-pass (anti-aliasing)
    if (highCutHz > 0 && highCutHz < samplingRate / 2) {
        int windowSize = static_cast<int>(samplingRate / highCutHz);
        windowSize = std::max(3, std::min(51, windowSize));
        movingAverage(data, windowSize);
    }
}

// Notch filter for powerline interference (50/60 Hz)
inline void notchFilter(QVector<double> &data, double samplingRate, 
                       double notchFreq = 50.0) {
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
    double sum = std::accumulate(data.begin(), data.end(), 0.0);
    return sum / data.size();
}

inline double standardDeviation(const QVector<double> &data) {
    if (data.size() < 2) return 0.0;
    
    double m = mean(data);
    double variance = 0.0;
    for (double val : data) {
        double diff = val - m;
        variance += diff * diff;
    }
    variance /= data.size();
    
    return std::sqrt(variance);
}

inline double minValue(const QVector<double> &data) {
    if (data.isEmpty()) return 0.0;
    return *std::min_element(data.begin(), data.end());
}

inline double maxValue(const QVector<double> &data) {
    if (data.isEmpty()) return 0.0;
    return *std::max_element(data.begin(), data.end());
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

// ================== EEG-SPECIFIC PROCESSING ==================

// Re-reference to average of all channels
inline void applyAverageReference(QVector<QVector<double>> &allChannelData) {
    if (allChannelData.isEmpty()) return;
    
    int numSamples = allChannelData[0].size();
    for (const auto &channel : allChannelData) {
        if (channel.size() != numSamples) return;  // All channels must have same length
    }
    
    // Calculate average at each time point
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

// Common EEG montages
enum MontageType {
    MontageBipolar,
    MontageAverageReference,
    MontageLaplacian,
    MontageLongitudinalBipolar
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
            // Example: Fp1-F7, F7-T3, etc. (simplified)
            if (allChannelData.size() >= 2) {
                QVector<QVector<double>> bipolarData;
                for (int i = 0; i < allChannelData.size() - 1; ++i) {
                    QVector<double> diff(allChannelData[i].size());
                    for (int j = 0; j < diff.size(); ++j) {
                        diff[j] = allChannelData[i][j] - allChannelData[i+1][j];
                    }
                    bipolarData.append(diff);
                }
                allChannelData = bipolarData;
            }
            break;
        default:
            // Other montages not implemented yet
            break;
    }
}

// ================== FREQUENCY ANALYSIS ==================

inline QVector<double> powerSpectrum(const QVector<double> &data, double samplingRate) {
    QVector<double> spectrum;
    if (data.isEmpty() || samplingRate <= 0) return spectrum;
    
    int N = data.size();
    spectrum.resize(N/2 + 1);
    
    // FFTW plans
    fftw_complex *in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    fftw_complex *out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    
    // Copy data
    for (int i = 0; i < N; ++i) {
        in[i][0] = data[i];
        in[i][1] = 0.0;
    }
    
    // Execute FFT
    fftw_plan p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(p);
    
    // Calculate power spectrum
    for (int i = 0; i <= N/2; ++i) {
        double real = out[i][0];
        double imag = out[i][1];
        spectrum[i] = sqrt(real*real + imag*imag) / N;
    }
    
    // Cleanup
    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(out);
    
    return spectrum;
}

// Calculate power in specific frequency bands
struct BandPower {
    double delta;    // 0.5-4 Hz
    double theta;    // 4-8 Hz  
    double alpha;    // 8-13 Hz
    double beta;     // 13-30 Hz
    double gamma;    // 30-100 Hz
};

inline BandPower calculateBandPower(const QVector<double> &data, 
                                   double samplingRate) {
    BandPower power = {0.0, 0.0, 0.0, 0.0, 0.0};
    
    auto spectrum = powerSpectrum(data, samplingRate);
    if (spectrum.isEmpty()) return power;
    
    double freqResolution = samplingRate / (2.0 * spectrum.size());
    
    for (int i = 0; i < spectrum.size(); ++i) {
        double freq = i * freqResolution;
        double p = spectrum[i] * spectrum[i];  // Power = amplitude^2
        
        if (freq >= 0.5 && freq < 4.0) power.delta += p;
        else if (freq >= 4.0 && freq < 8.0) power.theta += p;
        else if (freq >= 8.0 && freq < 13.0) power.alpha += p;
        else if (freq >= 13.0 && freq < 30.0) power.beta += p;
        else if (freq >= 30.0 && freq < 100.0) power.gamma += p;
    }
    
    return power;
}

}