/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Charts module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "xyseriesiodevice.h"
#include <QtCharts/QXYSeries>

const double PI = 3.14159265358979323846264338;
XYSeriesIODevice::XYSeriesIODevice(QXYSeries * series, QXYSeries *s_series, QObject *parent) :
    QIODevice(parent),
    m_series(series),
    spect_series(s_series)
{
}

XYSeriesIODevice::~XYSeriesIODevice()
{
    delete[] fft_Complex_array;
    delete[] fft_amplitude_array;
}

qint64 XYSeriesIODevice::readData(char * data, qint64 maxSize)
{
    Q_UNUSED(data)
    Q_UNUSED(maxSize)
    return -1;
}

void Rearrange(std::complex<double> *samples, int length)
{
    int *rearrange;
    rearrange = new int[length];

    rearrange[0] = 0;
    for(int limit = 1, bit = length / 2; limit < length; limit <<= 1, bit>>=1 )
     for(int i = 0; i < limit; i++)
        rearrange[i + limit] = rearrange[i] + bit;

    for(int i = 0; i < length; i++) {
     if (rearrange[i] == i) rearrange[i] = 0;
     else rearrange[ rearrange[i] ] = 0;
    }

   std::complex<double> t;
   for (int i = 0; i < length; i++)
      if (rearrange[i]) {
         t = samples[i];
         samples[i] = samples[ rearrange[i] ];
         samples[ rearrange[i] ] = t;
      }
   delete[] rearrange;
}

void ForwardFft(std::complex<double> *samples, int length )
{

   Rearrange(samples,length);
   for(int halfSize = 1; halfSize < length; halfSize *= 2)
   {

      std::complex<double> phaseShiftStep = std::polar(1.0, -PI / halfSize);
      std::complex<double> currentPhaseShift(1, 0);

      for(int fftStep = 0; fftStep < halfSize; fftStep++)
      {

         for(int i = fftStep; i < length; i += 2 * halfSize)
         {
            std::complex<double> t = currentPhaseShift * samples[i + halfSize];
            samples[i + halfSize] = samples[i] - t;
            samples[i] += t;
         }

         currentPhaseShift *= phaseShiftStep;
      }
   }
}

qint64 XYSeriesIODevice::writeData(const char * data, qint64 maxSize)
{
    qint64 range = 1024;
    QVector<QPointF> oldPoints = m_series->pointsVector();
    QVector<QPointF> points;
    QVector<QPointF> spectrumePoints;

    if (oldPoints.count() < range) {
        points = m_series->pointsVector();
    } else {
        for (int i = maxSize; i < oldPoints.count(); i++)
            points.append(QPointF(i - maxSize, oldPoints.at(i).y()));
    }

    qint64 size = points.count();
    for (int k = 0; k < maxSize; k++){
        points.append(QPointF(k + size, (quint8)data[k]));
    }

    if(points.length() > range)
    {
        fft_Complex_array = new std::complex<double>[points.length()];
        fft_amplitude_array = new double[points.length() / 2];
        for(int i = 0; i < points.length(); i++){
            fft_Complex_array[i] = std::complex<double>(points.at(i).y());
        }
        ForwardFft(fft_Complex_array, range);
        for(int i = 0; i < points.length() / 2; i++)
        {
            fft_amplitude_array[i] = std::abs(fft_Complex_array[i]);
            spectrumePoints.append(QPointF((double)i / range * 8000 / 1.0, fft_amplitude_array[i]));
        }
    }
    m_series->replace(points);
    spect_series->replace(spectrumePoints);
    return maxSize;
}
