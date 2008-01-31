// qtractorPluginListView.cpp
//
/****************************************************************************
   Copyright (C) 2005-2008, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "qtractorPluginListView.h"

#include "qtractorPluginCommand.h"
#include "qtractorPluginSelectForm.h"
#include "qtractorPluginForm.h"

#include "qtractorRubberBand.h"

#include "qtractorMainForm.h"

#include <QItemDelegate>
#include <QPainter>
#include <QMenu>
#include <QToolTip>
#include <QScrollBar>

#include <QMouseEvent>
#include <QDragLeaveEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QContextMenuEvent>


//----------------------------------------------------------------------------
// qtractorTinyScrollBarStyle -- Custom style to have some tiny scrollbars
//
#include <QCDEStyle>

class qtractorTinyScrollBarStyle : public QCDEStyle
{
public:

	// Custom virtual override.
	int pixelMetric( PixelMetric pm,
		const QStyleOption *option = 0, const QWidget *pWidget = 0 ) const
	{
		if (pm == QStyle::PM_ScrollBarExtent)
			return 8;

		return QCDEStyle::pixelMetric(pm, option, pWidget);
	}
};

static qtractorTinyScrollBarStyle g_tinyScrollBarStyle;



//----------------------------------------------------------------------------
// qtractorPluginListItemDelegate -- Plugin list view item delgate.

class qtractorPluginListItemDelegate : public QItemDelegate
{
public:

	// Constructor.
	qtractorPluginListItemDelegate(QListWidget *pListWidget)
		: QItemDelegate(pListWidget), m_pListWidget(pListWidget) {}

	// Overridden paint method.
	void paint(QPainter *pPainter, const QStyleOptionViewItem& option,
		const QModelIndex& index) const
	{
		// Unselectable items get special grayed out painting...
		QListWidgetItem *pItem = m_pListWidget->item(index.row());
		if (pItem) {
			pPainter->save();
			const QPalette& pal = option.palette;
			QColor rgbBack;
			QColor rgbFore;
			if (option.state & QStyle::State_HasFocus) {
				rgbBack = pal.highlight().color();
				rgbFore = pal.highlightedText().color();
			} else {
				rgbBack = pal.window().color();
				rgbFore = pal.windowText().color();
			}
			// Fill the background...
			pPainter->fillRect(option.rect, rgbBack);
			// Draw the icon...
			QRect rect = option.rect;
			const QSize& iconSize = m_pListWidget->iconSize();
			pPainter->drawPixmap(1,
				rect.top() + ((rect.height() - iconSize.height()) >> 1),
				pItem->icon().pixmap(iconSize));
			// Draw the text...
			rect.setLeft(iconSize.width());
			pPainter->setPen(rgbFore);
			pPainter->drawText(rect,
				Qt::AlignLeft | Qt::AlignVCenter, pItem->text());
			// Draw frame lines...
			pPainter->setPen(rgbBack.light(150));
			pPainter->drawLine(
				option.rect.left(),  option.rect.top(),
				option.rect.right(), option.rect.top());
			pPainter->setPen(rgbBack.dark(150));
			pPainter->drawLine(
				option.rect.left(),  option.rect.bottom(),
				option.rect.right(), option.rect.bottom());
			pPainter->restore();
		//	if (option.state & QStyle::State_HasFocus)
		//		QItemDelegate::drawFocus(pPainter, option, option.rect);
		} else {
			// Others do as default...
			QItemDelegate::paint(pPainter, option, index);
		}
	}

private:

	QListWidget *m_pListWidget;
};


//----------------------------------------------------------------------------
// qtractorPluginListItem -- Plugins list item.

// Constructors.
qtractorPluginListItem::qtractorPluginListItem ( qtractorPlugin *pPlugin )
	: QListWidgetItem()
{
	m_pPlugin = pPlugin;

	m_pPlugin->addItem(this);

	QListWidgetItem::setText((m_pPlugin->type())->name());

	updateActivated();
}

// Destructor.
qtractorPluginListItem::~qtractorPluginListItem (void)
{
	m_pPlugin->removeItem(this);
}


// Plugin container accessor.
qtractorPlugin *qtractorPluginListItem::plugin (void) const
{
	return m_pPlugin;
}


// Activation methods.
void qtractorPluginListItem::updateActivated (void)
{
	QListWidgetItem::setIcon(
		*qtractorPluginListView::itemIcon(
			m_pPlugin && m_pPlugin->isActivated() ? 1 : 0));
}


//----------------------------------------------------------------------------
// qtractorPluginListView -- Plugin chain list widget instance.
//
int    qtractorPluginListView::g_iItemRefCount = 0;
QIcon *qtractorPluginListView::g_pItemIcons[2] = { NULL, NULL };

// Construcctor.
qtractorPluginListView::qtractorPluginListView ( QWidget *pParent )
	: QListWidget(pParent), m_pPluginList(NULL), m_pClickedItem(NULL)
{
	if (++g_iItemRefCount == 1) {
		g_pItemIcons[0] = new QIcon(":/icons/itemLedOff.png");
		g_pItemIcons[1] = new QIcon(":/icons/itemLedOn.png");
	}

	// Drag-and-drop stuff.
	m_pDragItem   = NULL;
	m_pDropItem   = NULL;
	m_pRubberBand = NULL;

//	QListWidget::setDragEnabled(true);
	QListWidget::setAcceptDrops(true);
	QListWidget::setDropIndicatorShown(true);
	QListWidget::setAutoScroll(true);

	QListWidget::setFont(QFont(QListWidget::font().family(), 6));
	QListWidget::verticalScrollBar()->setStyle(&g_tinyScrollBarStyle);
	QListWidget::horizontalScrollBar()->setStyle(&g_tinyScrollBarStyle);

	QListWidget::setIconSize(QSize(10, 10));
	QListWidget::setItemDelegate(new qtractorPluginListItemDelegate(this));
	QListWidget::setSelectionMode(QAbstractItemView::SingleSelection);

	QListWidget::viewport()->setBackgroundRole(QPalette::Window);

	QListWidget::setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//	QListWidget::setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	// Trap for help/tool-tips events.
	QListWidget::viewport()->installEventFilter(this);

	// Double-click handling...
	QObject::connect(this,
		SIGNAL(itemDoubleClicked(QListWidgetItem*)),
		SLOT(itemDoubleClickedSlot(QListWidgetItem*)));
	QObject::connect(this,
		SIGNAL(itemActivated(QListWidgetItem*)),
		SLOT(itemActivatedSlot(QListWidgetItem*)));
}


// Destructor.
qtractorPluginListView::~qtractorPluginListView (void)
{
	// No need to delete child widgets, Qt does it all for us
	clear();

	setPluginList(NULL);

	if (--g_iItemRefCount == 0) {
		for (int i = 0; i < 2; i++) {
			delete g_pItemIcons[i];
			g_pItemIcons[i] = NULL;
		}
	}
}


// Plugin list accessors.
void qtractorPluginListView::setPluginList ( qtractorPluginList *pPluginList )
{
	if (m_pPluginList)
		m_pPluginList->removeView(this);

	m_pPluginList = pPluginList;

	if (m_pPluginList) {
		m_pPluginList->setAutoDelete(true);
		m_pPluginList->addView(this);
	}

	refresh();
}

qtractorPluginList *qtractorPluginListView::pluginList (void) const
{
	return m_pPluginList;
}

// Plugin list refreshner
void qtractorPluginListView::refresh (void)
{
	clear();

	QListWidget::setUpdatesEnabled(false);
	if (m_pPluginList) {
		for (qtractorPlugin *pPlugin = m_pPluginList->first();
				pPlugin; pPlugin = pPlugin->next()) {
			QListWidget::addItem(new qtractorPluginListItem(pPlugin));
		}
	}
	QListWidget::setUpdatesEnabled(true);
}


// Master clean-up.
void qtractorPluginListView::clear (void)
{
	dragLeaveEvent(NULL);

	QListWidget::clear();
}


// Get an item index, given the plugin reference...
int qtractorPluginListView::pluginItem ( qtractorPlugin *pPlugin )
{
	int iItemCount = QListWidget::count();
	for (int iItem = 0; iItem < iItemCount; ++iItem) {
		qtractorPluginListItem *pItem
			= static_cast<qtractorPluginListItem *> (QListWidget::item(iItem));
		if (pItem && pItem->plugin() == pPlugin)
			return iItem;
	}

	return -1;
}


// Move item on list.
void qtractorPluginListView::moveItem (
	qtractorPluginListItem *pItem, qtractorPluginListItem *pNextItem )
{
	if (pItem == NULL)
		return;

	if (m_pPluginList == NULL)
		return;

	// The plugin to be moved...	
	qtractorPlugin *pPlugin = pItem->plugin();
	if (pPlugin == NULL)
		return;

	// To be after this one...
	qtractorPlugin *pNextPlugin = NULL;
	if (pNextItem)
		pNextPlugin = pNextItem->plugin();

	// Make it an undoable command...
	qtractorMainForm *pMainForm = qtractorMainForm::getInstance();
	if (pMainForm)
		pMainForm->commands()->exec(
			new qtractorMovePluginCommand(pPlugin, pNextPlugin, m_pPluginList));
}


// Copy item on list.
void qtractorPluginListView::copyItem (
	qtractorPluginListItem *pItem, qtractorPluginListItem *pNextItem )
{
	if (pItem == NULL)
		return;

	if (m_pPluginList == NULL)
		return;

	// The plugin to be moved...	
	qtractorPlugin *pPlugin = pItem->plugin();
	// Clone/copy the new plugin here...
	if (pPlugin)
		pPlugin = m_pPluginList->copyPlugin(pPlugin);
	if (pPlugin == NULL)
		return;

	// To be after this one...
	qtractorPlugin *pNextPlugin = NULL;
	if (pNextItem)
		pNextPlugin = pNextItem->plugin();

	// Make it an undoable command...
	qtractorMainForm *pMainForm = qtractorMainForm::getInstance();
	if (pMainForm) {
		pMainForm->commands()->exec(
			new qtractorInsertPluginCommand(
				tr("copy plugin"), pPlugin, pNextPlugin));
	}
}


// Add a new plugin slot.
void qtractorPluginListView::addPlugin (void)
{
	if (m_pPluginList == NULL)
		return;

	qtractorPluginSelectForm selectForm(this);
	selectForm.setChannels(m_pPluginList->channels(), m_pPluginList->isMidi());
	if (!selectForm.exec())
		return;

	qtractorMainForm *pMainForm = qtractorMainForm::getInstance();
	if (pMainForm == NULL)
		return;

	// Make it a undoable command...
	qtractorAddPluginCommand *pAddPluginCommand
		= new qtractorAddPluginCommand();

	for (int i = 0; i < selectForm.pluginCount(); ++i) {
		// Add an actual plugin item...
		qtractorPlugin *pPlugin
			= qtractorPluginFile::createPlugin(m_pPluginList,
				selectForm.pluginFilename(i),
				selectForm.pluginIndex(i),
				selectForm.pluginTypeHint(i));
		pAddPluginCommand->addPlugin(pPlugin);
		// Show the plugin form right away...
		(pPlugin->form())->activateForm();
	}

	pMainForm->commands()->exec(pAddPluginCommand);
}


// Remove an existing plugin slot.
void qtractorPluginListView::removePlugin (void)
{
	if (m_pPluginList == NULL)
		return;

	qtractorPluginListItem *pItem
		= static_cast<qtractorPluginListItem *> (QListWidget::currentItem());
	if (pItem == NULL)
		return;

	qtractorPlugin *pPlugin = pItem->plugin();
	if (pPlugin == NULL)
		return;

	// Make it a undoable command...
	qtractorMainForm *pMainForm = qtractorMainForm::getInstance();
	if (pMainForm)
		pMainForm->commands()->exec(
			new qtractorRemovePluginCommand(pPlugin));
}


// Activate existing plugin slot.
void qtractorPluginListView::activatePlugin (void)
{
	qtractorPluginListItem *pItem
		= static_cast<qtractorPluginListItem *> (QListWidget::currentItem());
	if (pItem == NULL)
		return;

	qtractorPlugin *pPlugin = pItem->plugin();
	if (pPlugin == NULL)
		return;

	// Make it a undoable command...
	qtractorMainForm *pMainForm = qtractorMainForm::getInstance();
	if (pMainForm) {
		pMainForm->commands()->exec(
			new qtractorActivatePluginCommand(pPlugin,
				!pPlugin->isActivated()));
	}
}


// Activate all plugins.
void qtractorPluginListView::activateAllPlugins (void)
{
	if (m_pPluginList == NULL)
		return;

	// Check whether everyone is already activated...
	if (m_pPluginList->count() < 1 || m_pPluginList->isActivatedAll())
		return;

	// Make it an undoable command...
	qtractorMainForm *pMainForm = qtractorMainForm::getInstance();
	if (pMainForm == NULL)
		return;

	qtractorActivatePluginCommand *pActivateAllCommand
		= new qtractorActivatePluginCommand(NULL, true);
	pActivateAllCommand->setName(tr("activate all plugins"));

	for (qtractorPlugin *pPlugin = m_pPluginList->first();
			pPlugin; pPlugin = pPlugin->next()) {
		if (!pPlugin->isActivated())
			pActivateAllCommand->addPlugin(pPlugin);
	}

	pMainForm->commands()->exec(pActivateAllCommand);
}


// Dectivate all plugins.
void qtractorPluginListView::deactivateAllPlugins (void)
{
	if (m_pPluginList == NULL)
		return;

	// Check whether everyone is already dectivated...
	if (m_pPluginList->activated() < 1)
		return;

	// Make it an undoable command...
	qtractorMainForm *pMainForm = qtractorMainForm::getInstance();
	if (pMainForm == NULL)
		return;

	qtractorActivatePluginCommand *pDeactivateAllCommand
		= new qtractorActivatePluginCommand(NULL, false);
	pDeactivateAllCommand->setName(tr("deactivate all plugins"));

	for (qtractorPlugin *pPlugin = m_pPluginList->first();
			pPlugin; pPlugin = pPlugin->next()) {
		if (pPlugin->isActivated())
			pDeactivateAllCommand->addPlugin(pPlugin);
	}

	pMainForm->commands()->exec(pDeactivateAllCommand);
}


// Remove all plugins.
void qtractorPluginListView::removeAllPlugins (void)
{
	if (m_pPluginList == NULL)
		return;

	// Check whether there's any...
	if (m_pPluginList->count() < 1)
		return;

	// Make it an undoable command...
	qtractorMainForm *pMainForm = qtractorMainForm::getInstance();
	if (pMainForm == NULL)
		return;

	qtractorRemovePluginCommand *pRemoveAllCommand
		= new qtractorRemovePluginCommand();
	pRemoveAllCommand->setName(tr("remove all plugins"));

	for (qtractorPlugin *pPlugin = m_pPluginList->first();
			pPlugin; pPlugin = pPlugin->next()) {
		pRemoveAllCommand->addPlugin(pPlugin);
	}

	pMainForm->commands()->exec(pRemoveAllCommand);
}


// Move an existing plugin upward slot.
void qtractorPluginListView::moveUpPlugin (void)
{
	if (m_pPluginList == NULL)
		return;

	qtractorPluginListItem *pItem
		= static_cast<qtractorPluginListItem *> (QListWidget::currentItem());
	if (pItem == NULL)
		return;

	int iNextItem = QListWidget::row(pItem) - 1;
	if (iNextItem < 0)
		return;

	qtractorPluginListItem *pNextItem
		= static_cast<qtractorPluginListItem *> (
			QListWidget::item(iNextItem));
	if (pNextItem == NULL)
		return;

	moveItem(pItem, pNextItem);
}


// Move an existing plugin downward slot.
void qtractorPluginListView::moveDownPlugin (void)
{
	if (m_pPluginList == NULL)
		return;

	qtractorPluginListItem *pItem
		= static_cast<qtractorPluginListItem *> (QListWidget::currentItem());
	if (pItem == NULL)
		return;

	int iNextItem = QListWidget::row(pItem) + 1;
	if (iNextItem >= QListWidget::count())
		return;

	qtractorPluginListItem *pNextItem
		= static_cast<qtractorPluginListItem *> (
			QListWidget::item(iNextItem + 1));

	moveItem(pItem, pNextItem);
}


// Show/hide an existing plugin form slot.
void qtractorPluginListView::editPlugin (void)
{
	itemActivatedSlot(QListWidget::currentItem());
}


// Show an existing plugin form slot.
void qtractorPluginListView::itemDoubleClickedSlot ( QListWidgetItem *item )
{
	itemActivatedSlot(item);
}

void qtractorPluginListView::itemActivatedSlot ( QListWidgetItem *item )
{
	if (m_pPluginList == NULL)
		return;

	qtractorPluginListItem *pItem
		= static_cast<qtractorPluginListItem *> (item);
	if (pItem == NULL)
		return;

	qtractorPlugin *pPlugin = pItem->plugin();
	if (pPlugin == NULL)
		return;

	(pPlugin->form())->activateForm();
}


// Trap for help/tool-tip events.
bool qtractorPluginListView::eventFilter ( QObject *pObject, QEvent *pEvent )
{
	QWidget *pViewport = QListWidget::viewport();
	if (static_cast<QWidget *> (pObject) == pViewport
		&& pEvent->type() == QEvent::ToolTip) {
		QHelpEvent *pHelpEvent = static_cast<QHelpEvent *> (pEvent);
		if (pHelpEvent) {
			qtractorPluginListItem *pItem
				= static_cast<qtractorPluginListItem *> (
					QListWidget::itemAt(pHelpEvent->pos()));
			qtractorPlugin *pPlugin = NULL;
			if (pItem)
				pPlugin = pItem->plugin();
			if (pPlugin) {
				QToolTip::showText(pHelpEvent->globalPos(),
					(pPlugin->type())->name(), pViewport);
				return true;
			}
		}
	}

	// Not handled here.
	return QListWidget::eventFilter(pObject, pEvent);
}


// To get precize clicking for in-place (de)activation;
// handle mouse events for drag-and-drop stuff.
void qtractorPluginListView::mousePressEvent ( QMouseEvent *pMouseEvent )
{
	dragLeaveEvent(NULL);

	const QPoint& pos = pMouseEvent->pos();
	qtractorPluginListItem *pItem
		= static_cast<qtractorPluginListItem *> (QListWidget::itemAt(pos));

	if (pItem && pos.x() > 0 && pos.x() < QListWidget::iconSize().width())
		m_pClickedItem = pItem;

	if (pMouseEvent->button() == Qt::LeftButton) {
		m_posDrag   = pos;
		m_pDragItem = pItem;
	}

	QListWidget::mousePressEvent(pMouseEvent);	
}


void qtractorPluginListView::mouseMoveEvent ( QMouseEvent *pMouseEvent )
{
	QListWidget::mouseMoveEvent(pMouseEvent);

	if ((pMouseEvent->buttons() & Qt::LeftButton) && m_pDragItem
		&& ((pMouseEvent->pos() - m_posDrag).manhattanLength()
			>= QApplication::startDragDistance())) {
		// We'll start dragging something alright...
		QMimeData *pMimeData = new QMimeData();
		encodeItem(pMimeData, m_pDragItem);
		QDrag *pDrag = new QDrag(this);
		pDrag->setMimeData(pMimeData);
		pDrag->setPixmap(m_pDragItem->icon().pixmap(16));
		pDrag->setHotSpot(QPoint(-4, -12));
		pDrag->start(Qt::MoveAction);
		// We've dragged and maybe dropped it by now...
		m_pDragItem = NULL;
	}
}


void qtractorPluginListView::mouseReleaseEvent ( QMouseEvent *pMouseEvent )
{
	QListWidget::mouseReleaseEvent(pMouseEvent);
	
	const QPoint& pos = pMouseEvent->pos();
	qtractorPluginListItem *pItem
		= static_cast<qtractorPluginListItem *> (QListWidget::itemAt(pos));

	if (pItem && pos.x() > 0 && pos.x() < QListWidget::iconSize().width()
		&& m_pClickedItem == pItem) {
		qtractorPlugin *pPlugin = pItem->plugin();
		if (pPlugin) {
			// Make it a undoable command...
			qtractorMainForm *pMainForm = qtractorMainForm::getInstance();
			if (pMainForm) {
				pMainForm->commands()->exec(
					new qtractorActivatePluginCommand(pPlugin,
						!pPlugin->isActivated()));
			}
		}
	}

	dragLeaveEvent(NULL);
}


// Drag-n-drop stuff;
// track on the current drop item position.
bool qtractorPluginListView::canDropEvent ( QDropEvent *pDropEvent )
{
	if (m_pPluginList == NULL)
		return false;

	if (!canDecodeItem(pDropEvent->mimeData()))
		return false;

	if (m_pDragItem == NULL)
		m_pDragItem	= decodeItem(pDropEvent->mimeData());
	if (m_pDragItem == NULL)
		return false;

	qtractorPluginListItem *pDropItem
		= static_cast<qtractorPluginListItem *> (
			QListWidget::itemAt(pDropEvent->pos()));
	if (pDropItem && pDropItem != m_pDropItem)
		ensureVisibleItem(pDropItem);

	// Cannot drop onto itself or over the one below...
	qtractorPlugin *pPlugin = m_pDragItem->plugin();
	if (pPlugin == NULL)
		return false;
	if (pDropItem) {
		if (pDropItem->plugin() == pPlugin
			|| (pDropItem->plugin())->prev() == pPlugin)
			return false;
	} else if (m_pPluginList->last() == pPlugin)
		return false;

	// All that to check whether it will get properly instantiated...
	if ((pPlugin->type())->instances(
		m_pPluginList->channels(), m_pPluginList->isMidi()) < 1)
		return false;

	// This is the place we'll drop something
	// (append if null)
	m_pDropItem = pDropItem;

	return true;
}

bool qtractorPluginListView::canDropItem (  QDropEvent *pDropEvent  )
{
	bool bCanDropItem = canDropEvent(pDropEvent);
	
	if (bCanDropItem)
		moveRubberBand(m_pDropItem);
	else 
	if (m_pRubberBand && m_pRubberBand->isVisible())
		m_pRubberBand->hide();

	return bCanDropItem;
}


// Ensure given item is brought to viewport visibility...
void qtractorPluginListView::ensureVisibleItem ( qtractorPluginListItem *pItem )
{
	int iItem = QListWidget::row(pItem);
	if (iItem > 0) {
		qtractorPluginListItem *pItemAbove
			= static_cast<qtractorPluginListItem *> (
				QListWidget::item(iItem - 1));
		if (pItemAbove)
			QListWidget::scrollToItem(pItemAbove);
	}

	QListWidget::scrollToItem(pItem);
}


void qtractorPluginListView::dragEnterEvent ( QDragEnterEvent *pDragEnterEvent )
{
#if 0
	if (canDropItem(pDragEnterEvent)) {
		if (!pDragEnterEvent->isAccepted()) {
			pDragEnterEvent->setDropAction(Qt::MoveAction);
			pDragEnterEvent->accept();
		}
	} else {
		pDragEnterEvent->ignore();
	}
#else
	// Always accept the drag-enter event,
	// so let we deal with it during move later...
	pDragEnterEvent->accept();
#endif
}


void qtractorPluginListView::dragMoveEvent ( QDragMoveEvent *pDragMoveEvent )
{
	if (canDropItem(pDragMoveEvent)) {
		if (!pDragMoveEvent->isAccepted()) {
			pDragMoveEvent->setDropAction(Qt::MoveAction);
			pDragMoveEvent->accept();
		}
	} else {
		pDragMoveEvent->ignore();
	}
}


void qtractorPluginListView::dragLeaveEvent ( QDragLeaveEvent * )
{
	if (m_pRubberBand) {
		delete m_pRubberBand;
		m_pRubberBand = NULL;
	}

	m_pClickedItem = NULL;

	m_pDragItem = NULL;
	m_pDropItem = NULL;
}


void qtractorPluginListView::dropEvent ( QDropEvent *pDropEvent )
{
	if (!canDropItem(pDropEvent)) {
		dragLeaveEvent(NULL);
		return;
	}

	// If we aren't in the same list,
	// or care for keyboard modifiers...
	if (((m_pDragItem->plugin())->list() == m_pPluginList) ||
		(pDropEvent->keyboardModifiers() & Qt::ShiftModifier)) {
		dropMove();
	}
	else
	if (pDropEvent->keyboardModifiers() & Qt::ControlModifier) {
		dropCopy();
	} else {
		// We'll better have a drop menu...
		QMenu menu(this);
		menu.addAction(tr("&Move Here"), this, SLOT(dropMove()));
		menu.addAction(tr("&Copy Here"), this, SLOT(dropCopy()));
		menu.addSeparator();
		menu.addAction(QIcon(":/icons/formReject.png"),
			tr("C&ancel"), this, SLOT(dropCancel()));
		menu.exec(QListWidget::mapToGlobal(pDropEvent->pos()));
	}

	dragLeaveEvent(NULL);
}


// Drop item slots.
void qtractorPluginListView::dropMove (void)
{
	moveItem(m_pDragItem, m_pDropItem);
	QListWidget::setFocus();
}

void qtractorPluginListView::dropCopy (void)
{
	copyItem(m_pDragItem, m_pDropItem);
	QListWidget::setFocus();
}

void qtractorPluginListView::dropCancel (void)
{
	// Do nothing...
}


// Draw a dragging separator line.
void qtractorPluginListView::moveRubberBand ( qtractorPluginListItem *pDropItem )
{
	// Create the rubber-band if there's none...
	if (m_pRubberBand == NULL) {
		m_pRubberBand = new qtractorRubberBand(
			QRubberBand::Line, QListWidget::viewport());
	//	QPalette pal(m_pRubberBand->palette());
	//	pal.setColor(m_pRubberBand->foregroundRole(), Qt::blue);
	//	m_pRubberBand->setPalette(pal);
	//	m_pRubberBand->setBackgroundRole(QPalette::NoRole);
	}

	// Just move it...
	QRect rect;
	if (pDropItem) {
		rect = QListWidget::visualItemRect(pDropItem);
		rect.setTop(rect.top() - 1);
	} else {
		pDropItem = static_cast<qtractorPluginListItem *> (
			QListWidget::item(QListWidget::count() - 1));
		if (pDropItem) {
			rect = QListWidget::visualItemRect(pDropItem);
			rect.setTop(rect.bottom() - 1);
		} else {
			rect = QListWidget::viewport()->rect();
		}
	}
	// Set always this height:
	rect.setHeight(2);

	m_pRubberBand->setGeometry(rect);

	// Ah, and make it visible, of course...
	if (!m_pRubberBand->isVisible())
		m_pRubberBand->show();
}


// Context menu event handler.
void qtractorPluginListView::contextMenuEvent (
	QContextMenuEvent *pContextMenuEvent )
{
	if (m_pPluginList == NULL)
		return;

	QMenu menu(this);
	QAction *pAction;

	int iItem = -1;
	int iItemCount = QListWidget::count();
	bool bEnabled  = (iItemCount > 0);

	qtractorPlugin *pPlugin = NULL;
	qtractorPluginListItem *pItem
		= static_cast<qtractorPluginListItem *> (QListWidget::currentItem());
	if (pItem) {
		iItem = QListWidget::row(pItem);
		pPlugin = pItem->plugin();
	}

	pAction = menu.addAction(
		QIcon(":/icons/formCreate.png"),
		tr("&Add Plugin..."), this, SLOT(addPlugin()));
//	pAction->setEnabled(true);

	pAction = menu.addAction(
		QIcon(":/icons/formRemove.png"),
		tr("&Remove"), this, SLOT(removePlugin()));
	pAction->setEnabled(pPlugin != NULL);

	menu.addSeparator();

	pAction = menu.addAction(
		tr("Act&ivate"), this, SLOT(activatePlugin()));
	pAction->setCheckable(true);
	pAction->setChecked(pPlugin && pPlugin->isActivated());
	pAction->setEnabled(pPlugin != NULL);

	menu.addSeparator();

	bool bActivatedAll = m_pPluginList->isActivatedAll();
	pAction = menu.addAction(
		tr("Acti&vate All"), this, SLOT(activateAllPlugins()));
	pAction->setCheckable(true);
	pAction->setChecked(bActivatedAll);
	pAction->setEnabled(bEnabled && !bActivatedAll);

	bool bDeactivatedAll = (m_pPluginList->activated() < 1);
	pAction = menu.addAction(
		tr("Deactivate Al&l"), this, SLOT(deactivateAllPlugins()));
	pAction->setCheckable(true);
	pAction->setChecked(bDeactivatedAll);
	pAction->setEnabled(bEnabled && !bDeactivatedAll);

	menu.addSeparator();

	pAction = menu.addAction(
		tr("Rem&ove All"), this, SLOT(removeAllPlugins()));
	pAction->setEnabled(bEnabled);

	menu.addSeparator();

	pAction = menu.addAction(
		QIcon(":/icons/formMoveUp.png"),
		tr("Move &Up"), this, SLOT(moveUpPlugin()));
	pAction->setEnabled(pItem && iItem > 0);

	pAction = menu.addAction(
		QIcon(":/icons/formMoveDown.png"),
		tr("Move &Down"), this, SLOT(moveDownPlugin()));
	pAction->setEnabled(pItem && iItem < iItemCount - 1);

	menu.addSeparator();

	pAction = menu.addAction(
		QIcon(":/icons/formEdit.png"),
		tr("&Edit Plugin..."), this, SLOT(editPlugin()));
	pAction->setCheckable(true);
	pAction->setChecked(pPlugin && pPlugin->isVisible());
	pAction->setEnabled(pItem != NULL);

	menu.exec(pContextMenuEvent->globalPos());
}


// Common pixmap accessors.
QIcon *qtractorPluginListView::itemIcon ( int iIndex )
{
	return g_pItemIcons[iIndex];
}


static const char *c_pszPluginListItemMimeType = "qtractor/plugin-item";

// Encoder method (static).
void qtractorPluginListView::encodeItem ( QMimeData *pMimeData,
	qtractorPluginListItem *pItem )
{
	// Allocate the data array...
	pMimeData->setData(c_pszPluginListItemMimeType,
		QByteArray((const char *) &pItem, sizeof(qtractorPluginListItem *)));
}

// Decode trial method (static).
bool qtractorPluginListView::canDecodeItem ( const QMimeData *pMimeData )
{
	return pMimeData->hasFormat(c_pszPluginListItemMimeType);
}

// Decode method (static).
qtractorPluginListItem *qtractorPluginListView::decodeItem (
	const QMimeData *pMimeData )
{
	qtractorPluginListItem *pItem = NULL;

	QByteArray data = pMimeData->data(c_pszPluginListItemMimeType);
	if (data.size() == sizeof(qtractorPluginListItem *))
		::memcpy(&pItem, data.constData(), sizeof(qtractorPluginListItem *));

	return pItem;
}


// end of qtractorPluginListView.cpp
