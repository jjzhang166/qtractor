// qtractorSession.h
//
/****************************************************************************
   Copyright (C) 2005, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*****************************************************************************/

#ifndef __qtractorSession_h
#define __qtractorSession_h

#include "qtractorTrack.h"

#include <qstring.h>

// Forward declarations.
class qtractorClip;

// Special forward declarations.
class qtractorMidiEngine;
class qtractorAudioEngine;
class qtractorAudioPeakFactory;
class qtractorSessionCursor;
class qtractorSessionDocument;

class QDomElement;


//-------------------------------------------------------------------------
// qtractorSession -- Session container.

class qtractorSession
{
public:

	// Constructor.
	qtractorSession();
	// Default destructor.
	~qtractorSession();

	// Open/close session engine(s).
	bool open(const QString& sClientName);
	void close();

	// Reset session.
	void clear();

	// Session filename accessors.
	void setSessionName(const QString& sSessionName);
	const QString& sessionName() const;

	// Session description accessors.
	void setDescription(const QString& sDescription);
	const QString& description() const;

	// Session length accessors.
	bool updateSessionLength();
	unsigned long sessionLength() const;

	// Sample rate accessors.
	void setSampleRate(unsigned int iSampleRate);
	unsigned int sampleRate() const;

	// Session tempo accessors.
	void setTempo(float fTempo);
	float tempo() const;

	// Resolution accessors.
	void setTicksPerBeat(unsigned short iTicksPerBeat);
	unsigned short ticksPerBeat() const;

	// Beats/Bar(measure) accessors.
	void setBeatsPerBar(unsigned short iBeatsPerBar);
	unsigned short beatsPerBar() const;
	bool beatIsBar(unsigned int iBeat) const;

	// Horizontal zoom factor.
	void setHorizontalZoom(unsigned short iHorizontalZoom);
	unsigned short horizontalZoom() const;

	// Vertical zoom factor.
	void setVerticalZoom(unsigned short iVerticalZoom);
	unsigned short verticalZoom() const;

	// Pixels per beat (width).	
	void setPixelsPerBeat(unsigned short iPixelsPerBeat);
	unsigned short pixelsPerBeat() const;

	// Beat divisor (snap) accesors.
	void setSnapPerBeat(unsigned short iSnapPerBeat);
	unsigned short snapPerBeat(void) const;

	// Pixel/Beat number conversion.
	unsigned int beatFromPixel(unsigned int x) const;
	unsigned int pixelFromBeat(unsigned int iBeat) const;

	// Pixel/Tick number conversion.
	unsigned long tickFromPixel(unsigned int x) const;
	unsigned int pixelFromTick(unsigned long iTick) const;

	// Pixel/Frame number conversion.
	unsigned long frameFromPixel(unsigned int x) const;
	unsigned int pixelFromFrame(unsigned long iFrame) const;

	// Beat/frame conversion.
	unsigned long frameFromBeat(unsigned int iBeat) const;
	unsigned int beatFromFrame(unsigned long iFrame) const;

	// Tick/Frame number conversion.
	unsigned long frameFromTick(unsigned long iTick) const;
	unsigned long tickFromFrame(unsigned long iFrame) const;

	// Beat/frame snap filters.
	unsigned long frameSnap(unsigned long iFrame) const;
	unsigned int pixelSnap(unsigned int x) const;

	// Convert frame to time string.
	QString timeFromFrame(unsigned long iFrame) const;

	// Update time scale divisor factors.
	void updateTimeScale();

	// Track list management methods.
	qtractorList<qtractorTrack>& tracks();

	void addTrack(qtractorTrack *pTrack);
	void updateTrack(qtractorTrack *pTrack);
	void removeTrack(qtractorTrack *pTrack);

	qtractorTrack *trackAt(int iTrack) const;

	// Current number of solo tracks.
	void setSoloTracks(bool bSolo);
	unsigned int soloTracks() const;

	// Session cursor factory methods.
	qtractorSessionCursor *createSessionCursor(unsigned long iFrame = 0,
		qtractorTrack::TrackType syncType = qtractorTrack::None);
	void unlinkSessionCursor(qtractorSessionCursor *pSessionCursor);

	// Reset all linked session cursors.
	void reset();

	// Device engine accessors.
	qtractorMidiEngine  *midiEngine() const;
	qtractorAudioEngine *audioEngine() const;

	// Wait for application stabilization.
	static void stabilize(int msecs = 100);

	// Consolidated session engine activation status.
	bool isActivated() const;

	// Consolidated session engine start status.
	void setPlaying(bool bPlaying);
	bool isPlaying() const;

	// Playhead positioning.
	void setPlayhead(unsigned long iFrame);
	unsigned long playhead() const;

	// Special track-immediate methods.
	void trackMute(qtractorTrack *pTrack, bool bMute);
	void trackSolo(qtractorTrack *pTrack, bool bSolo);

	// Audio peak factory accessor.
	qtractorAudioPeakFactory *audioPeakFactory() const;

	// MIDI track tagging specifics.
	void acquireMidiTag(qtractorTrack *pTrack);
	void releaseMidiTag(qtractorTrack *pTrack);

	// Document element methods.
	bool loadElement(qtractorSessionDocument *pDocument,
		QDomElement *pElement);
	bool saveElement(qtractorSessionDocument *pDocument,
		QDomElement *pElement);

private:

	QString        m_sSessionName;      // Session filename.
	QString        m_sDescription;      // Session description.

	unsigned long  m_iSessionLength;    // Session length in frames.

	unsigned int   m_iSampleRate;       // Sample rate (frames per second)
	float          m_fTempo;            // Tempo (beats per minute; BPM)
	unsigned short m_iTicksPerBeat;     // Resolution (pulses per beat; PPQN)
	unsigned short m_iBeatsPerBar;      // Measure (beats per bar)

	unsigned short m_iPixelsPerBeat;    // Pixels per beat (width).
	unsigned short m_iHorizontalZoom;   // Horizontal zoom factor.
	unsigned short m_iVerticalZoom;     // Vertical zoom factor.
	unsigned short m_iSnapPerBeat;      // Beat snap divisor.

	// Internal time scaling factors.
	unsigned long  m_iScale_a;
	float          m_fScale_b;
	float          m_fScale_c;
	float          m_fScale_d;

	unsigned int   m_iSoloTracks;       // Current number of solo tracks.

	// The track list.
	qtractorList<qtractorTrack> m_tracks;

	// Managed session cursors.
	qtractorList<qtractorSessionCursor> m_cursors;

	// Device engine instances.
	qtractorMidiEngine  *m_pMidiEngine;
	qtractorAudioEngine *m_pAudioEngine;

	// Audio peak factory (singleton) instance.
	qtractorAudioPeakFactory *m_pAudioPeakFactory;

	// MIDI track tagging specifics.
	unsigned short m_iMidiTag;
	QValueList<unsigned short> m_midiTags;
};


#endif  // __qtractorSession_h


// end of qtractorSession.h
