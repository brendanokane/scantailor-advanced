/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ImageView.h"
#include "ImageTransformation.h"
#include "ImagePresentation.h"
#include <QMouseEvent>
#include <QMenu>
#include <QPainter>
#include <QDebug>
#include <boost/bind.hpp>

namespace select_content {
    ImageView::ImageView(const QImage& image,
                         const QImage& downscaled_image,
                         const ImageTransformation& xform,
                         const QRectF& content_rect,
                         const QRectF& page_rect,
                         bool page_rect_enabled)
            : ImageViewBase(
            image, downscaled_image,
            ImagePresentation(xform.transform(), xform.resultingPreCropArea())
    ),
              m_dragHandler(*this),
              m_zoomHandler(*this),
              m_pNoContentMenu(new QMenu(this)),
              m_pHaveContentMenu(new QMenu(this)),
              m_contentRect(content_rect),
              m_pageRect(page_rect),
              m_minBoxSize(10.0, 10.0),
              m_pageRectEnabled(page_rect_enabled) {
        setMouseTracking(true);

        interactionState().setDefaultStatusTip(
                tr("Use the context menu to enable / disable the content box. Hold Shift to drag a box.")
        );

        const QString content_rect_drag_tip(tr("Drag lines or corners to resize the content box."));
        // Setup corner drag handlers.
        static const int masks_by_corner[] = { TOP | LEFT, TOP | RIGHT, BOTTOM | RIGHT, BOTTOM | LEFT };
        for (int i = 0; i < 4; ++i) {
            m_contentRectCorners[i].setPositionCallback(
                    boost::bind(&ImageView::contentRectCornerPosition, this, masks_by_corner[i])
            );
            m_contentRectCorners[i].setMoveRequestCallback(
                    boost::bind(&ImageView::contentRectCornerMoveRequest, this, masks_by_corner[i], _1)
            );
            m_contentRectCorners[i].setDragFinishedCallback(
                    boost::bind(&ImageView::contentRectDragFinished, this)
            );
            m_contentRectCornerHandlers[i].setObject(&m_contentRectCorners[i]);
            m_contentRectCornerHandlers[i].setProximityStatusTip(content_rect_drag_tip);
            Qt::CursorShape cursor = (i & 1) ? Qt::SizeBDiagCursor : Qt::SizeFDiagCursor;
            m_contentRectCornerHandlers[i].setProximityCursor(cursor);
            m_contentRectCornerHandlers[i].setInteractionCursor(cursor);
            makeLastFollower(m_contentRectCornerHandlers[i]);
        }
        // Setup edge drag handlers.
        static const int masks_by_edge[] = { TOP, RIGHT, BOTTOM, LEFT };
        for (int i = 0; i < 4; ++i) {
            m_contentRectEdges[i].setPositionCallback(
                    boost::bind(&ImageView::contentRectEdgePosition, this, masks_by_edge[i])
            );
            m_contentRectEdges[i].setMoveRequestCallback(
                    boost::bind(&ImageView::contentRectEdgeMoveRequest, this, masks_by_edge[i], _1)
            );
            m_contentRectEdges[i].setDragFinishedCallback(
                    boost::bind(&ImageView::contentRectDragFinished, this)
            );
            m_contentRectEdgeHandlers[i].setObject(&m_contentRectEdges[i]);
            m_contentRectEdgeHandlers[i].setProximityStatusTip(content_rect_drag_tip);
            Qt::CursorShape cursor = (i & 1) ? Qt::SizeHorCursor : Qt::SizeVerCursor;
            m_contentRectEdgeHandlers[i].setProximityCursor(cursor);
            m_contentRectEdgeHandlers[i].setInteractionCursor(cursor);
            makeLastFollower(m_contentRectEdgeHandlers[i]);
        }

        if (page_rect_enabled) {
            const QString page_rect_drag_tip(tr("Drag lines or corners to resize the page box."));
            // Setup corner drag handlers.
            for (int i = 0; i < 4; ++i) {
                m_pageRectCorners[i].setPositionCallback(
                        boost::bind(&ImageView::pageRectCornerPosition, this, masks_by_corner[i])
                );
                m_pageRectCorners[i].setMoveRequestCallback(
                        boost::bind(&ImageView::pageRectCornerMoveRequest, this, masks_by_corner[i], _1)
                );
                m_pageRectCorners[i].setDragFinishedCallback(
                        boost::bind(&ImageView::pageRectDragFinished, this)
                );
                m_pageRectCornerHandlers[i].setObject(&m_pageRectCorners[i]);
                m_pageRectCornerHandlers[i].setProximityStatusTip(page_rect_drag_tip);
                Qt::CursorShape cursor = (i & 1) ? Qt::SizeBDiagCursor : Qt::SizeFDiagCursor;
                m_pageRectCornerHandlers[i].setProximityCursor(cursor);
                m_pageRectCornerHandlers[i].setInteractionCursor(cursor);
                makeLastFollower(m_pageRectCornerHandlers[i]);
            }
            // Setup edge drag handlers.
            for (int i = 0; i < 4; ++i) {
                m_pageRectEdges[i].setPositionCallback(
                        boost::bind(&ImageView::pageRectEdgePosition, this, masks_by_edge[i])
                );
                m_pageRectEdges[i].setMoveRequestCallback(
                        boost::bind(&ImageView::pageRectEdgeMoveRequest, this, masks_by_edge[i], _1)
                );
                m_pageRectEdges[i].setDragFinishedCallback(
                        boost::bind(&ImageView::pageRectDragFinished, this)
                );
                m_pageRectEdgeHandlers[i].setObject(&m_pageRectEdges[i]);
                m_pageRectEdgeHandlers[i].setProximityStatusTip(page_rect_drag_tip);
                Qt::CursorShape cursor = (i & 1) ? Qt::SizeHorCursor : Qt::SizeVerCursor;
                m_pageRectEdgeHandlers[i].setProximityCursor(cursor);
                m_pageRectEdgeHandlers[i].setInteractionCursor(cursor);
                makeLastFollower(m_pageRectEdgeHandlers[i]);
            }
        }

        {
            m_contentRectArea.setPositionCallback(
                    boost::bind(&ImageView::contentRectPosition, this)
            );
            m_contentRectArea.setMoveRequestCallback(
                    boost::bind(&ImageView::contentRectMoveRequest, this, _1)
            );
            m_contentRectArea.setDragFinishedCallback(
                    boost::bind(&ImageView::contentRectDragFinished, this)
            );
            m_contentRectAreaHandler.setObject(&m_contentRectArea);
            m_contentRectAreaHandler.setProximityStatusTip(tr("Hold left mouse button to drag the content box."));
            m_contentRectAreaHandler.setInteractionStatusTip(tr("Release left mouse button to finish dragging."));
            Qt::CursorShape cursor = Qt::DragMoveCursor;
            m_contentRectAreaHandler.setKeyboardModifiers(Qt::ShiftModifier);
            m_contentRectAreaHandler.setProximityCursor(cursor);
            m_contentRectAreaHandler.setInteractionCursor(cursor);
            makeLastFollower(m_contentRectAreaHandler);
        }

        if (page_rect_enabled) {
            m_pageRectArea.setPositionCallback(
                    boost::bind(&ImageView::pageRectPosition, this)
            );
            m_pageRectArea.setMoveRequestCallback(
                    boost::bind(&ImageView::pageRectMoveRequest, this, _1)
            );
            m_pageRectArea.setDragFinishedCallback(
                    boost::bind(&ImageView::pageRectDragFinished, this)
            );
            m_pageRectAreaHandler.setObject(&m_pageRectArea);
            m_pageRectAreaHandler.setProximityStatusTip(tr("Hold left mouse button to drag the page box."));
            m_pageRectAreaHandler.setInteractionStatusTip(tr("Release left mouse button to finish dragging."));
            Qt::CursorShape cursor = Qt::DragMoveCursor;
            m_pageRectAreaHandler.setKeyboardModifiers(Qt::ShiftModifier);
            m_pageRectAreaHandler.setProximityCursor(cursor);
            m_pageRectAreaHandler.setInteractionCursor(cursor);
            makeLastFollower(m_pageRectAreaHandler);
        }

        rootInteractionHandler().makeLastFollower(*this);
        rootInteractionHandler().makeLastFollower(m_dragHandler);
        rootInteractionHandler().makeLastFollower(m_zoomHandler);

        QAction* create = m_pNoContentMenu->addAction(tr("Create Content Box"));
        QAction* remove = m_pHaveContentMenu->addAction(tr("Remove Content Box"));
        create->setShortcut(QKeySequence("Ins"));
        remove->setShortcut(QKeySequence("Delete"));
        addAction(create);
        addAction(remove);
        connect(create, SIGNAL(triggered(bool)), this, SLOT(createContentBox()));
        connect(remove, SIGNAL(triggered(bool)), this, SLOT(removeContentBox()));
    }

    ImageView::~ImageView() = default;

    void ImageView::createContentBox() {
        if (!m_contentRect.isEmpty()) {
            return;
        }
        if (interactionState().captured()) {
            return;
        }

        const QRectF virtual_rect(virtualDisplayRect());
        QRectF content_rect(0, 0, virtual_rect.width() * 0.7, virtual_rect.height() * 0.7);
        content_rect.moveCenter(virtual_rect.center());
        m_contentRect = content_rect;
        update();
        emit manualContentRectSet(m_contentRect);
    }

    void ImageView::removeContentBox() {
        if (m_contentRect.isEmpty()) {
            return;
        }
        if (interactionState().captured()) {
            return;
        }

        m_contentRect = QRectF();
        update();
        emit manualContentRectSet(m_contentRect);
    }

    void ImageView::onPaint(QPainter& painter, const InteractionState& interaction) {
        if (m_contentRect.isNull() && !m_pageRectEnabled) {
            return;
        }

        painter.setRenderHints(QPainter::Antialiasing, true);

        if (m_pageRectEnabled) {
            QPen pen(QColor(0xff, 0x7f, 0x00));
            pen.setWidthF(1.0);
            pen.setCosmetic(true);
            painter.setPen(pen);

            painter.setBrush(Qt::NoBrush);

            painter.drawRect(m_pageRect);
        }

        if (m_contentRect.isNull()) {
            return;
        }

        // Draw the content bounding box.
        QPen pen(QColor(0x00, 0x00, 0xff));
        pen.setWidthF(1.0);
        pen.setCosmetic(true);
        painter.setPen(pen);

        painter.setBrush(QColor(0x00, 0x00, 0xff, 50));

        // Pen strokes will be outside of m_contentRect - that's how drawRect() works.
        painter.drawRect(m_contentRect);
    }      // ImageView::onPaint

    void ImageView::onContextMenuEvent(QContextMenuEvent* event, InteractionState& interaction) {
        if (interaction.captured()) {
            // No context menus during resizing.
            return;
        }

        if (m_contentRect.isEmpty()) {
            m_pNoContentMenu->popup(event->globalPos());
        } else {
            m_pHaveContentMenu->popup(event->globalPos());
        }
    }

    QPointF ImageView::contentRectCornerPosition(int edge_mask) const {
        const QRectF rect(virtualToWidget().mapRect(m_contentRect));
        QPointF pt;

        if (edge_mask & TOP) {
            pt.setY(rect.top());
        } else if (edge_mask & BOTTOM) {
            pt.setY(rect.bottom());
        }

        if (edge_mask & LEFT) {
            pt.setX(rect.left());
        } else if (edge_mask & RIGHT) {
            pt.setX(rect.right());
        }

        return pt;
    }

    void ImageView::contentRectCornerMoveRequest(int edge_mask, const QPointF& pos) {
        QRectF r(virtualToWidget().mapRect(m_contentRect));
        const qreal minw = m_minBoxSize.width();
        const qreal minh = m_minBoxSize.height();

        if (edge_mask & TOP) {
            r.setTop(std::min(pos.y(), r.bottom() - minh));
        } else if (edge_mask & BOTTOM) {
            r.setBottom(std::max(pos.y(), r.top() + minh));
        }

        if (edge_mask & LEFT) {
            r.setLeft(std::min(pos.x(), r.right() - minw));
        } else if (edge_mask & RIGHT) {
            r.setRight(std::max(pos.x(), r.left() + minw));
        }

        forceInsideImage(r, edge_mask);
        m_contentRect = widgetToVirtual().mapRect(r);

        forcePageRectDescribeContent();

        update();
    }

    QLineF ImageView::contentRectEdgePosition(int edge) const {
        const QRectF rect(virtualToWidget().mapRect(m_contentRect));

        if (edge == TOP) {
            return QLineF(rect.topLeft(), rect.topRight());
        } else if (edge == BOTTOM) {
            return QLineF(rect.bottomLeft(), rect.bottomRight());
        } else if (edge == LEFT) {
            return QLineF(rect.topLeft(), rect.bottomLeft());
        } else {
            return QLineF(rect.topRight(), rect.bottomRight());
        }
    }

    void ImageView::contentRectEdgeMoveRequest(int edge, const QLineF& line) {
        contentRectCornerMoveRequest(edge, line.p1());
    }

    void ImageView::contentRectDragFinished() {
        emit manualContentRectSet(m_contentRect);
        if (m_pageRectReloadRequested) {
            emit manualPageRectSet(m_pageRect);
            m_pageRectReloadRequested = false;
        }
    }

    QPointF ImageView::pageRectCornerPosition(int edge_mask) const {
        const QRectF rect(virtualToWidget().mapRect(m_pageRect));
        QPointF pt;

        if (edge_mask & TOP) {
            pt.setY(rect.top());
        } else if (edge_mask & BOTTOM) {
            pt.setY(rect.bottom());
        }

        if (edge_mask & LEFT) {
            pt.setX(rect.left());
        } else if (edge_mask & RIGHT) {
            pt.setX(rect.right());
        }

        return pt;
    }

    void ImageView::pageRectCornerMoveRequest(int edge_mask, const QPointF& pos) {
        QRectF r(virtualToWidget().mapRect(m_pageRect));
        const qreal minw = m_minBoxSize.width();
        const qreal minh = m_minBoxSize.height();

        if (edge_mask & TOP) {
            r.setTop(std::min(pos.y(), r.bottom() - minh));
        } else if (edge_mask & BOTTOM) {
            r.setBottom(std::max(pos.y(), r.top() + minh));
        }

        if (edge_mask & LEFT) {
            r.setLeft(std::min(pos.x(), r.right() - minw));
        } else if (edge_mask & RIGHT) {
            r.setRight(std::max(pos.x(), r.left() + minw));
        }

        m_pageRect = widgetToVirtual().mapRect(r);
        forcePageRectDescribeContent();

        update();
        emit pageRectSizeChanged(m_pageRect.size());
    }

    QLineF ImageView::pageRectEdgePosition(int edge) const {
        const QRectF rect(virtualToWidget().mapRect(m_pageRect));

        if (edge == TOP) {
            return QLineF(rect.topLeft(), rect.topRight());
        } else if (edge == BOTTOM) {
            return QLineF(rect.bottomLeft(), rect.bottomRight());
        } else if (edge == LEFT) {
            return QLineF(rect.topLeft(), rect.bottomLeft());
        } else {
            return QLineF(rect.topRight(), rect.bottomRight());
        }
    }

    void ImageView::pageRectEdgeMoveRequest(int edge, const QLineF& line) {
        pageRectCornerMoveRequest(edge, line.p1());
    }

    void ImageView::pageRectDragFinished() {
        emit manualPageRectSet(m_pageRect);
    }

    void ImageView::forceInsideImage(QRectF& widget_rect, const int edge_mask) const {
        const qreal minw = m_minBoxSize.width();
        const qreal minh = m_minBoxSize.height();
        const QRectF image_rect(virtualToWidget().mapRect(virtualDisplayRect()));

        if ((edge_mask & LEFT) && (widget_rect.left() < image_rect.left())) {
            widget_rect.setLeft(image_rect.left());
            widget_rect.setRight(std::max(widget_rect.right(), widget_rect.left() + minw));
        }
        if ((edge_mask & RIGHT) && (widget_rect.right() > image_rect.right())) {
            widget_rect.setRight(image_rect.right());
            widget_rect.setLeft(std::min(widget_rect.left(), widget_rect.right() - minw));
        }
        if ((edge_mask & TOP) && (widget_rect.top() < image_rect.top())) {
            widget_rect.setTop(image_rect.top());
            widget_rect.setBottom(std::max(widget_rect.bottom(), widget_rect.top() + minh));
        }
        if ((edge_mask & BOTTOM) && (widget_rect.bottom() > image_rect.bottom())) {
            widget_rect.setBottom(image_rect.bottom());
            widget_rect.setTop(std::min(widget_rect.top(), widget_rect.bottom() - minh));
        }
    }

    void ImageView::forcePageRectDescribeContent() {
        const QRectF oldPageRect = m_pageRect;
        m_pageRect |= m_contentRect;
        if (m_pageRectEnabled && (m_pageRect != oldPageRect)) {
            m_pageRectReloadRequested = true;
            emit pageRectSizeChanged(m_pageRect.size());
        }
    }

    QRectF ImageView::contentRectPosition() const {
        return virtualToWidget().mapRect(m_contentRect);
    }

    void ImageView::contentRectMoveRequest(const QPolygonF& poly_pos) {
        QRectF contentRectInWidget(poly_pos.boundingRect());

        const QRectF image_rect(virtualToWidget().mapRect(virtualDisplayRect()));
        if (contentRectInWidget.left() < image_rect.left()) {
            contentRectInWidget.translate(image_rect.left() - contentRectInWidget.left(), 0);
        }
        if (contentRectInWidget.right() > image_rect.right()) {
            contentRectInWidget.translate(image_rect.right() - contentRectInWidget.right(), 0);
        }
        if (contentRectInWidget.top() < image_rect.top()) {
            contentRectInWidget.translate(0, image_rect.top() - contentRectInWidget.top());
        }
        if (contentRectInWidget.bottom() > image_rect.bottom()) {
            contentRectInWidget.translate(0, image_rect.bottom() - contentRectInWidget.bottom());
        }

        m_contentRect = widgetToVirtual().mapRect(contentRectInWidget);

        forcePageRectDescribeContent();

        update();
    }

    QRectF ImageView::pageRectPosition() const {
        return virtualToWidget().mapRect(m_pageRect);
    }

    void ImageView::pageRectMoveRequest(const QPolygonF& poly_pos) {
        QRectF pageRectInWidget(poly_pos.boundingRect());

        const QRectF content_rect(virtualToWidget().mapRect(m_contentRect));
        if (pageRectInWidget.left() > content_rect.left()) {
            pageRectInWidget.translate(content_rect.left() - pageRectInWidget.left(), 0);
        }
        if (pageRectInWidget.right() < content_rect.right()) {
            pageRectInWidget.translate(content_rect.right() - pageRectInWidget.right(), 0);
        }
        if (pageRectInWidget.top() > content_rect.top()) {
            pageRectInWidget.translate(0, content_rect.top() - pageRectInWidget.top());
        }
        if (pageRectInWidget.bottom() < content_rect.bottom()) {
            pageRectInWidget.translate(0, content_rect.bottom() - pageRectInWidget.bottom());
        }

        m_pageRect = widgetToVirtual().mapRect(pageRectInWidget);

        update();
    }

    void ImageView::pageRectSetExternally(const QRectF& pageRect) {
        if (!m_pageRectEnabled) {
            return;
        }
        m_pageRect = pageRect;
        forcePageRectDescribeContent();

        update();
        emit manualPageRectSet(m_pageRect);
    }
}  // namespace select_content