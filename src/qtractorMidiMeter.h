// qtractorMidiMeter.h
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

#ifndef __qtractorMidiMeter_h
#define __qtractorMidiMeter_h

#include "qtractorMeter.h"

// Color/level indexes.
#define QTRACTOR_MIDI_METER_PEAK	0
#define QTRACTOR_MIDI_METER_OVER	1
#define QTRACTOR_MIDI_METER_BACK	2
#define QTRACTOR_MIDI_METER_FORE	3

#define QTRACTOR_MIDI_METER_COLORS	4

// Forward declarations.
class qtractorMidiMeter;
class qtractorMidiMeterValue;
class qtractorMidiMonitor;


//----------------------------------------------------------------------------
// qtractorMidiMeterScale -- MIDI meter bridge scale widget.

class qtractorMidiMeterScale : public qtractorMeterScale
{
	Q_OBJECT

public:

	// Constructor.
	qtractorMidiMeterScale(qtractorMidiMeter *pMidiMeter,
		QWidget *pParent = 0, const char *pszName = 0);

protected:

	// Actual scale drawing method.
	void paintScale(QPainter *p);
};


//----------------------------------------------------------------------------
// qtractorMidiMeterValue -- MIDI meter bridge value widget.

class qtractorMidiMeterValue : public QFrame
{
	Q_OBJECT

public:

	// Constructor.
	qtractorMidiMeterValue(qtractorMidiMeter *pMidiMeter,
		QWidget *pParent = 0, const char *pszName = 0);
	// Default destructor.
	~qtractorMidiMeterValue();

	// Reset peak holder.
	void peakReset();

protected:

	// Specific event handlers.
	void paintEvent(QPaintEvent *);
	void resizeEvent(QResizeEvent *);

private:

	// Local instance variables.
	qtractorMidiMeter *m_pMidiMeter;

	// Running variables.
	int   m_iValue;
	float m_fValueDecay;
	int   m_iPeak;
	int   m_iPeakHold;
	float m_fPeakDecay;
};


//----------------------------------------------------------------------------
// qtractorMidiMeter -- MIDI meter bridge slot widget.

class qtractorMidiMeter : public qtractorMeter
{
	Q_OBJECT

public:

	// Constructor.
	qtractorMidiMeter(qtractorMidiMonitor *pMidiMonitor,
		QWidget *pParent = 0, const char *pszName = 0);
	// Default destructor.
	~qtractorMidiMeter();

	// Virtual monitor accessor.
	void setMonitor(qtractorMonitor *pMonitor);
	qtractorMonitor *monitor() const;

	// MIDI monitor accessor.
	void setMidiMonitor(qtractorMidiMonitor *pMidiMonitor);
	qtractorMidiMonitor *midiMonitor() const;

	// Local slider update methods.
	void updatePanning();
	void updateGain();

	// Monitor reset.
	void reset();

	// Slot refreshment.
	void refresh();

	// Reset peak holder.
	void peakReset();

	// Common resource accessors.
	const QColor& color(int iIndex) const;

protected:

	// Gain-value (percent) converters...
	float gainFromValue(float fValue) const;
	float valueFromGain(float fGain)  const;

	// Specific event handlers.
	void resizeEvent(QResizeEvent *);

private:

	// Local instance variables.
	qtractorMidiMonitor    *m_pMidiMonitor;
	qtractorMidiMeterScale *m_pMidiScale;
	qtractorMidiMeterValue *m_pMidiValue;

	QColor m_colors[QTRACTOR_MIDI_METER_COLORS];

	// MIDI I/O LED pixmap studd.
	QPixmap     *m_pMidiPixmap[2];
	unsigned int m_iMidiCount;
};

	
#endif  // __qtractorMidiMeter_h

// end of qtractorMidiMeter.h
