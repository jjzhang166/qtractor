// qtractorTrackTime.h
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

#ifndef __qtractorTrackTime_h
#define __qtractorTrackTime_h

#include <qscrollview.h>

// Forward declarations.
class qtractorTracks;


//----------------------------------------------------------------------------
// qtractorTrackTime -- Track time scale widget.

class qtractorTrackTime : public QScrollView
{
	Q_OBJECT

public:

	// Constructor.
	qtractorTrackTime(qtractorTracks *pTracks,
		QWidget *pParent, const char *pszName = 0);
	// Destructor.
	~qtractorTrackTime();

	// Rectangular contents update.
	void updateContents(const QRect& rect, bool bRefresh = true);
	// Overall contents update.
	void updateContents(bool bRefresh = true);

protected:

	// Resize event handler.
	void resizeEvent(QResizeEvent *pResizeEvent);

	// Draw the time scale.
	void drawContents(QPainter *p,
		int clipx, int clipy, int clipw, int cliph);

	// Check if some position header is to be dragged...
	bool dragHeadStart(const QPoint& pos);

	// Handle selection with mouse.
	void contentsMousePressEvent(QMouseEvent *pMouseEvent);
	void contentsMouseMoveEvent(QMouseEvent *pMouseEvent);
	void contentsMouseReleaseEvent(QMouseEvent *pMouseEvent);

	// Reset drag/select state.
	void resetDragState();

	// Keyboard event handler.
	void keyPressEvent(QKeyEvent *pKeyEvent);

protected slots:

	// To have timeline in h-sync with main track view.
	void contentsMovingSlot(int cx, int cy);

	// (Re)create the time scale pixmap.
	void updatePixmap(int cx, int cy);

private:

	// The logical parent binding.
	qtractorTracks *m_pTracks;

	// Local double-buffering pixmap.
	QPixmap *m_pPixmap;

	// The current selecting/dragging head stuff.
	enum DragState {
		DragNone = 0, DragStart, DragSelect,
		DragPlayHead, DragEditHead, DragEditTail,
		DragLoopStart, DragLoopEnd
	} m_dragState;

	QRect  m_rectDrag;
	QPoint m_posDrag;
};


#endif  // __qtractorTrackTime_h


// end of qtractorTrackTime.h
