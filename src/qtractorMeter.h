// qtractorMeter.h
//
/****************************************************************************
   Copyright (C) 2005-2006, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#ifndef __qtractorMeter_h
#define __qtractorMeter_h

#include <QWidget>


// Forward declarations.
class qtractorMeter;
class qtractorMonitor;
class qtractorSlider;

class QDoubleSpinBox;
class QHBoxLayout;
class QVBoxLayout;
class QLabel;

class QPaintEvent;
class QResizeEvent;


//----------------------------------------------------------------------------
// qtractorMeterScale -- Meter bridge scale widget.

class qtractorMeterScale : public QWidget
{
	Q_OBJECT

public:

	// Constructor.
	qtractorMeterScale(qtractorMeter *pMeter, QWidget *pParent = 0);

	// Default destructor.
	~qtractorMeterScale();

	// Meter accessor.
	qtractorMeter *meter() const;

protected:
	
	// Specific event handlers.
	void paintEvent(QPaintEvent *);

	// Draw IEC scale line and label.
	void drawLineLabel(QPainter *p, int y, const QString& sLabel);

	// Actual scale drawing method.
	virtual void paintScale(QPainter *p) = 0;

private:

	// Local instance variables.
	qtractorMeter *m_pMeter;

	// Running variables.
	int m_iLastY;
};


//----------------------------------------------------------------------------
// qtractorMeter -- Meter bridge slot widget.

class qtractorMeter : public QWidget
{
	Q_OBJECT

public:

	// Constructor.
	qtractorMeter(QWidget *pParent = 0);
	// Default destructor.
	virtual ~qtractorMeter();

	// Dynamic layout accessors.
	QLabel  *topLabel() const;
	QWidget *hbox() const;

	// Common slider/spin-box accessors.
	qtractorSlider *panSlider() const;
	QDoubleSpinBox *panSpinBox() const;
	qtractorSlider *gainSlider() const;
	QDoubleSpinBox *gainSpinBox() const;

	// Panning accessors.
	void setPanning(float fPanning);
	float panning() const;

	// Gain accessors.
	void setGain(float fGain);
	float gainScale() const;
	float gain() const;

	// Monitor accessors.
	virtual void setMonitor(qtractorMonitor *pMonitor) = 0;
	virtual qtractorMonitor *monitor() const = 0;

	// Local slider update methods.
	virtual void updatePanning() = 0;
	virtual void updateGain() = 0;

	// Slot refreshment.
	virtual void refresh() = 0;

	// Meter reset.
	virtual void reset() = 0;

	// Reset peak holder.
	virtual void peakReset() = 0;

	// Peak falloff mode setting.
	void setPeakFalloff(int bPeakFalloff);
	int peakFalloff() const;

protected:

	// Gain-scale converters...
	virtual float gainFromScale(float fScale) const { return fScale; }
	virtual float scaleFromGain(float fGain)  const { return fGain;  }
	virtual float gainFromValue(float fValue) const { return fValue; }
	virtual float valueFromGain(float fGain)  const { return fGain; }

protected slots:

	// Slider value-changed slots.
	void panSliderChangedSlot(int);
	void panSpinBoxChangedSlot(const QString&);
	void gainSliderChangedSlot(int);
	void gainSpinBoxChangedSlot(const QString&);

signals:

	// Slider change signals.
	void panChangedSignal(float fPanning);
	void gainChangedSignal(float fGain);
	
private:

	// Local instance variables.
	QVBoxLayout    *m_pVBoxLayout;
	qtractorSlider *m_pPanSlider;
	QDoubleSpinBox *m_pPanSpinBox;
	QLabel         *m_pTopLabel;
	QWidget        *m_pHBox;
	QHBoxLayout    *m_pHBoxLayout;
	qtractorSlider *m_pGainSlider;
	QDoubleSpinBox *m_pGainSpinBox;

	// Update exclusiveness flag.
	int m_iUpdate;

	// Peak falloff mode setting (0=no peak falloff).
	int m_iPeakFalloff;
};

	
#endif  // __qtractorMeter_h

// end of qtractorMeter.h
